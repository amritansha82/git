#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include <openssl/sha.h>  


std::string sha1(std::string text) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)text.c_str(), text.size(), hash);

    char sha1[SHA_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(sha1 + i * 2, "%02x", hash[i]);
    }
    sha1[SHA_DIGEST_LENGTH * 2] = '\0';

    return std::string(sha1);
}