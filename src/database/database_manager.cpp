#include "database_manager.hpp"

#include <iostream>
#include <stdexcept>

DatabaseManager::DatabaseManager(const std::vector<std::string>& connection_strings) {
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

void DatabaseManager::initialize_schema(std::shared_ptr<pqxx::connection> conn) {
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

bool DatabaseManager::store_block(const Block& block) {
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

Block DatabaseManager::get_contract(const std::string& contract_id) {
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

std::string DatabaseManager::get_last_hash() {
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

bool DatabaseManager::verify_chain() {
    for (auto& conn : db_connections) {
        try {
            pqxx::work txn(*conn);
            pqxx::result r = txn.exec(
                "SELECT * FROM contracts ORDER BY timestamp ASC"
            );
            
            for (pqxx::result::size_type i = 1; i < r.size(); i++) {
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
