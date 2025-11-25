#include "contract_api.hpp"

#include <iostream>
#include <crow.h>

ContractAPI::ContractAPI(ContractService& service, DatabaseManager& db) 
    : contract_service(service), db_manager(db) {}

void ContractAPI::run(int port) {
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
