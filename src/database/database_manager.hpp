#pragma once

#include <string>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

#include "../blockchain/block.hpp"
#include "../crypto/crypto_manager.hpp"

class DatabaseManager {
private:
    std::vector<std::shared_ptr<pqxx::connection>> db_connections;
    CryptoManager crypto;

    void initialize_schema(std::shared_ptr<pqxx::connection> conn);

public:
    explicit DatabaseManager(const std::vector<std::string>& connection_strings);
    
    bool store_block(const Block& block);
    Block get_contract(const std::string& contract_id);
    std::string get_last_hash();
    bool verify_chain();
};
