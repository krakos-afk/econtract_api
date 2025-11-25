
#include <iostream>
#include <exception>

#include "config/config.hpp"
#include "database/database_manager.hpp"
#include "services/contract_service.hpp"
#include "api/contract_api.hpp"

int main() {
    try {
        // Initialize database manager with configured connections
        DatabaseManager db_manager(config::get_db_connections());
        
        // Initialize contract service
        ContractService contract_service(db_manager);
        
        // Initialize and run API server
        ContractAPI api(contract_service, db_manager);
        api.run(config::SERVER_PORT);
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
