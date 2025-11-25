// E-Contracts Blockchain API - Complete Implementation
// Dependencies: libcurl, openssl, postgresql (libpq), crow (or cpp-httplib)

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <pqxx/pqxx>
#include "crow_all.h" // Crow web framework
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ==================== ENCRYPTION MODULE ====================
class CryptoManager {
private:
    unsigned char key[32]; // AES-256 key
    unsigned char iv[16];  // IV for AES

public:
    CryptoManager() {
        // In production, load from secure key management system
        RAND_bytes(key, sizeof(key));
        RAND_bytes(iv, sizeof(iv));
    }

    std::string encrypt(const std::string& plaintext) {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        
        int len, ciphertext_len;
        
        EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_EncryptUpdate(ctx, ciphertext.data(), &len, 
                         reinterpret_cast<const unsigned char*>(plaintext.c_str()), 
                         plaintext.size());
        ciphertext_len = len;
        
        EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
        ciphertext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        
        return base64_encode(ciphertext.data(), ciphertext_len);
    }

    std::string decrypt(const std::string& ciphertext_b64) {
        std::vector<unsigned char> ciphertext = base64_decode(ciphertext_b64);
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        std::vector<unsigned char> plaintext(ciphertext.size());
        
        int len, plaintext_len;
        
        EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
        EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());
        plaintext_len = len;
        
        EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
        plaintext_len += len;
        EVP_CIPHER_CTX_free(ctx);
        
        return std::string(plaintext.begin(), plaintext.begin() + plaintext_len);
    }

    static std::string sha256(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        SHA256_Update(&sha256, data.c_str(), data.size());
        SHA256_Final(hash, &sha256);
        
        std::stringstream ss;
        for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }

private:
    std::string base64_encode(const unsigned char* buffer, size_t length) {
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, buffer, length);
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        
        std::string result(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        
        return result;
    }

    std::vector<unsigned char> base64_decode(const std::string& encoded) {
        BIO *bio, *b64;
        std::vector<unsigned char> buffer(encoded.size());
        
        bio = BIO_new_mem_buf(encoded.c_str(), encoded.size());
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_push(b64, bio);
        
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        int decodedLength = BIO_read(bio, buffer.data(), encoded.size());
        BIO_free_all(bio);
        
        buffer.resize(decodedLength);
        return buffer;
    }
};

// ==================== BLOCKCHAIN BLOCK ====================
struct Block {
    std::string contract_id;
    std::string encrypted_data;
    std::string previous_hash;
    std::string hash;
    long long timestamp;
    int nonce;
    
    std::string calculate_hash() const {
        std::stringstream ss;
        ss << contract_id << encrypted_data << previous_hash << timestamp << nonce;
        return CryptoManager::sha256(ss.str());
    }
    
    void mine_block(int difficulty = 4) {
        std::string target(difficulty, '0');
        do {
            nonce++;
            hash = calculate_hash();
        } while (hash.substr(0, difficulty) != target);
    }
};

// ==================== DATABASE MANAGER ====================
class DatabaseManager {
private:
    std::vector<std::shared_ptr<pqxx::connection>> db_connections;
    CryptoManager crypto;

public:
    DatabaseManager(const std::vector<std::string>& connection_strings) {
        for (const auto& conn_str : connection_strings) {
            try {
                auto conn = std::make_shared<pqxx::connection>(conn_str);
                db_connections.push_back(conn);
                initialize_schema(conn);
                std::cout << "Connected to database: " << conn_str << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Database connection error: " << e.what() << std::endl;
                throw;
            }
        }
        
        if (db_connections.empty()) {
            throw std::runtime_error("No database connections established");
        }
    }

    void initialize_schema(std::shared_ptr<pqxx::connection> conn) {
        pqxx::work txn(*conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS contracts (
                contract_id VARCHAR(255) PRIMARY KEY,
                encrypted_data TEXT NOT NULL,
                previous_hash VARCHAR(64),
                hash VARCHAR(64) NOT NULL,
                timestamp BIGINT NOT NULL,
                nonce INTEGER NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            );
            
            CREATE INDEX IF NOT EXISTS idx_hash ON contracts(hash);
            CREATE INDEX IF NOT EXISTS idx_timestamp ON contracts(timestamp);
        )");
        txn.commit();
    }

    bool store_block(const Block& block) {
        bool success = true;
        
        // Store in all databases for redundancy
        for (auto& conn : db_connections) {
            try {
                pqxx::work txn(*conn);
                txn.exec_params(
                    "INSERT INTO contracts (contract_id, encrypted_data, previous_hash, hash, timestamp, nonce) "
                    "VALUES ($1, $2, $3, $4, $5, $6) "
                    "ON CONFLICT (contract_id) DO NOTHING",
                    block.contract_id,
                    block.encrypted_data,
                    block.previous_hash,
                    block.hash,
                    block.timestamp,
                    block.nonce
                );
                txn.commit();
            } catch (const std::exception& e) {
                std::cerr << "Error storing block: " << e.what() << std::endl;
                success = false;
            }
        }
        
        return success;
    }

    Block get_contract(const std::string& contract_id) {
        Block block;
        
        for (auto& conn : db_connections) {
            try {
                pqxx::work txn(*conn);
                pqxx::result r = txn.exec_params(
                    "SELECT * FROM contracts WHERE contract_id = $1",
                    contract_id
                );
                
                if (!r.empty()) {
                    block.contract_id = r[0]["contract_id"].as<std::string>();
                    block.encrypted_data = r[0]["encrypted_data"].as<std::string>();
                    block.previous_hash = r[0]["previous_hash"].as<std::string>();
                    block.hash = r[0]["hash"].as<std::string>();
                    block.timestamp = r[0]["timestamp"].as<long long>();
                    block.nonce = r[0]["nonce"].as<int>();
                    return block;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error retrieving contract: " << e.what() << std::endl;
            }
        }
        
        throw std::runtime_error("Contract not found");
    }

    std::string get_last_hash() {
        for (auto& conn : db_connections) {
            try {
                pqxx::work txn(*conn);
                pqxx::result r = txn.exec(
                    "SELECT hash FROM contracts ORDER BY timestamp DESC LIMIT 1"
                );
                
                if (!r.empty()) {
                    return r[0]["hash"].as<std::string>();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error getting last hash: " << e.what() << std::endl;
            }
        }
        
        return "0"; // Genesis block
    }

    bool verify_chain() {
        for (auto& conn : db_connections) {
            try {
                pqxx::work txn(*conn);
                pqxx::result r = txn.exec(
                    "SELECT * FROM contracts ORDER BY timestamp ASC"
                );
                
                for (size_t i = 1; i < r.size(); i++) {
                    std::string prev_hash = r[i-1]["hash"].as<std::string>();
                    std::string current_prev = r[i]["previous_hash"].as<std::string>();
                    
                    if (prev_hash != current_prev) {
                        return false;
                    }
                    
                    Block block;
                    block.contract_id = r[i]["contract_id"].as<std::string>();
                    block.encrypted_data = r[i]["encrypted_data"].as<std::string>();
                    block.previous_hash = r[i]["previous_hash"].as<std::string>();
                    block.timestamp = r[i]["timestamp"].as<long long>();
                    block.nonce = r[i]["nonce"].as<int>();
                    
                    if (block.calculate_hash() != r[i]["hash"].as<std::string>()) {
                        return false;
                    }
                }
                
                return true;
            } catch (const std::exception& e) {
                std::cerr << "Error verifying chain: " << e.what() << std::endl;
            }
        }
        
        return false;
    }
};

// ==================== CONTRACT SERVICE ====================
class ContractService {
private:
    DatabaseManager& db_manager;
    CryptoManager crypto;

public:
    ContractService(DatabaseManager& db) : db_manager(db) {}

    std::string create_contract(const json& contract_data) {
        // Generate unique contract ID
        std::string contract_id = generate_contract_id();
        
        // Serialize and encrypt contract data
        std::string plaintext = contract_data.dump();
        std::string encrypted_data = crypto.encrypt(plaintext);
        
        // Create blockchain block
        Block block;
        block.contract_id = contract_id;
        block.encrypted_data = encrypted_data;
        block.previous_hash = db_manager.get_last_hash();
        block.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        block.nonce = 0;
        
        // Mine the block (proof of work)
        block.mine_block(4);
        
        // Store in distributed databases
        if (db_manager.store_block(block)) {
            return contract_id;
        }
        
        throw std::runtime_error("Failed to store contract");
    }

    json get_contract(const std::string& contract_id) {
        Block block = db_manager.get_contract(contract_id);
        
        // Decrypt the contract data
        std::string plaintext = crypto.decrypt(block.encrypted_data);
        
        json result = json::parse(plaintext);
        result["contract_id"] = block.contract_id;
        result["hash"] = block.hash;
        result["timestamp"] = block.timestamp;
        
        return result;
    }

    bool verify_integrity() {
        return db_manager.verify_chain();
    }

private:
    std::string generate_contract_id() {
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        std::stringstream ss;
        ss << "CONTRACT_" << now << "_" << (rand() % 10000);
        return CryptoManager::sha256(ss.str()).substr(0, 16);
    }
};

// ==================== API SERVER ====================
class ContractAPI {
private:
    ContractService& contract_service;
    DatabaseManager& db_manager;

public:
    ContractAPI(ContractService& service, DatabaseManager& db) 
        : contract_service(service), db_manager(db) {}

    void run(int port = 8080) {
        crow::SimpleApp app;

        // Health check
        CROW_ROUTE(app, "/health")
        ([](){
            return crow::response(200, "OK");
        });

        // Create contract
        CROW_ROUTE(app, "/api/v1/contracts").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req){
            try {
                auto contract_data = json::parse(req.body);
                
                // Validate required fields
                if (!contract_data.contains("party_a") || !contract_data.contains("party_b")) {
                    return crow::response(400, "Missing required fields");
                }
                
                std::string contract_id = contract_service.create_contract(contract_data);
                
                json response;
                response["success"] = true;
                response["contract_id"] = contract_id;
                
                return crow::response(201, response.dump());
            } catch (const std::exception& e) {
                json error;
                error["success"] = false;
                error["error"] = e.what();
                return crow::response(500, error.dump());
            }
        });

        // Retrieve contract
        CROW_ROUTE(app, "/api/v1/contracts/<string>").methods(crow::HTTPMethod::GET)
        ([this](const std::string& contract_id){
            try {
                json contract = contract_service.get_contract(contract_id);
                return crow::response(200, contract.dump());
            } catch (const std::exception& e) {
                json error;
                error["success"] = false;
                error["error"] = e.what();
                return crow::response(404, error.dump());
            }
        });

        // Verify blockchain integrity
        CROW_ROUTE(app, "/api/v1/verify").methods(crow::HTTPMethod::GET)
        ([this](){
            bool is_valid = contract_service.verify_integrity();
            json response;
            response["valid"] = is_valid;
            return crow::response(200, response.dump());
        });

        std::cout << "E-Contracts API running on port " << port << std::endl;
        app.port(port).multithreaded().run();
    }
};

// ==================== MAIN ====================
int main() {
    // Configure multiple database connections for distributed storage
    // Update with your PostgreSQL credentials
    std::vector<std::string> db_connections = {
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node1",
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node2",
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node3"
    };

    try {
        DatabaseManager db_manager(db_connections);
        ContractService contract_service(db_manager);
        ContractAPI api(contract_service, db_manager);
        
        api.run(8080);
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}