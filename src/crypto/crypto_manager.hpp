#pragma once

#include <string>
#include <vector>

class CryptoManager {
private:
    unsigned char key[32]; // AES-256 key
    unsigned char iv[16];  // IV for AES

    std::string base64_encode(const unsigned char* buffer, size_t length);
    std::vector<unsigned char> base64_decode(const std::string& encoded);

public:
    CryptoManager();
    
    std::string encrypt(const std::string& plaintext);
    std::string decrypt(const std::string& ciphertext_b64);
    
    static std::string sha256(const std::string& data);
};
