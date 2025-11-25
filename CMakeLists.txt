cmake_minimum_required(VERSION 3.20)
project(EContractAPI VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Find packages
find_package(OpenSSL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)

# nlohmann_json - try find_package, fallback to manual include
find_package(nlohmann_json 3.2.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found via find_package, using system include")
    set(JSON_INCLUDE_DIR "/usr/include")
endif()

# Boost is header-only for our needs in modern versions
find_package(Boost 1.70 REQUIRED)

# Include directories
include_directories(
    ${OPENSSL_INCLUDE_DIR}
    ${PostgreSQL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/include
    /usr/include
)

# Source files
add_executable(econtract_api
    src/main.cpp
)

# Link libraries
target_link_libraries(econtract_api
    OpenSSL::SSL
    OpenSSL::Crypto
    pqxx
    pq
    Threads::Threads
)

# Link nlohmann_json if found
if(nlohmann_json_FOUND)
    target_link_libraries(econtract_api nlohmann_json::nlohmann_json)
endif()

# Install target
install(TARGETS econtract_api DESTINATION bin)