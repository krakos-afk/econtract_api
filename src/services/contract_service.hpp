#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "../database/database_manager.hpp"
#include "../crypto/crypto_manager.hpp"

using json = nlohmann::json;

class ContractService {
private:
    DatabaseManager& db_manager;
    CryptoManager crypto;

    std::string generate_contract_id();

public:
    explicit ContractService(DatabaseManager& db);
    
    std::string create_contract(const json& contract_data);
    json get_contract(const std::string& contract_id);
    bool verify_integrity();
};
