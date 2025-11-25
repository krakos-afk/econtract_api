#include "contract_service.hpp"

#include <chrono>
#include <sstream>
#include <cstdlib>

ContractService::ContractService(DatabaseManager& db) : db_manager(db) {}

std::string ContractService::create_contract(const json& contract_data) {
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

json ContractService::get_contract(const std::string& contract_id) {
    Block block = db_manager.get_contract(contract_id);
    
    // Decrypt the contract data
    std::string plaintext = crypto.decrypt(block.encrypted_data);
    
    json result = json::parse(plaintext);
    result["contract_id"] = block.contract_id;
    result["hash"] = block.hash;
    result["timestamp"] = block.timestamp;
    
    return result;
}

bool ContractService::verify_integrity() {
    return db_manager.verify_chain();
}

std::string ContractService::generate_contract_id() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::stringstream ss;
    ss << "CONTRACT_" << now << "_" << (rand() % 10000);
    return CryptoManager::sha256(ss.str()).substr(0, 16);
}
