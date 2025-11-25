#pragma once

#include "../services/contract_service.hpp"
#include "../database/database_manager.hpp"

class ContractAPI {
private:
    ContractService& contract_service;
    DatabaseManager& db_manager;

public:
    ContractAPI(ContractService& service, DatabaseManager& db);
    
    void run(int port = 8080);
};
