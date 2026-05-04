#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <openssl/sha.h>  

std::string sha1bin(std::string text) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)text.c_str(), text.size(), hash);
    return std::string((char*)hash, SHA_DIGEST_LENGTH);
}

std::string bin2hex(const std::string& bin) {
    std::string hex;
    hex.reserve(bin.size() * 2);
    
    for (size_t i = 0; i < bin.size(); i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned char>(bin[i]));
        hex += buf;
    }
    return hex;
}