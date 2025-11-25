#pragma once

#include <string>

struct Block {
    std::string contract_id;
    std::string encrypted_data;
    std::string previous_hash;
    std::string hash;
    long long timestamp;
    int nonce;
    
    std::string calculate_hash() const;
    void mine_block(int difficulty = 4);
};
