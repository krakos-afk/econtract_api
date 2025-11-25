#include "block.hpp"
#include "../crypto/crypto_manager.hpp"

#include <sstream>

std::string Block::calculate_hash() const {
    std::stringstream ss;
    ss << contract_id << encrypted_data << previous_hash << timestamp << nonce;
    return CryptoManager::sha256(ss.str());
}

void Block::mine_block(int difficulty) {
    std::string target(difficulty, '0');
    do {
        nonce++;
        hash = calculate_hash();
    } while (hash.substr(0, difficulty) != target);
}
