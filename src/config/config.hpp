#pragma once

#include <string>
#include <vector>

namespace config {

// Database configuration
inline std::vector<std::string> get_db_connections() {
    return {
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node1",
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node2",
        "postgresql://contractuser:secure_password@localhost:5432/contracts_node3"
    };
}

// Server configuration
constexpr int SERVER_PORT = 8080;

// Blockchain configuration
constexpr int MINING_DIFFICULTY = 4;

} // namespace config
