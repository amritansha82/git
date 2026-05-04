#include <string>
#include <filesystem>
#include <fstream>
#include "blobwriter.hpp"
#include "sha1.hpp"
#include "zlibhelp.hpp"

std::string create_blob(std::ifstream& file) {
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::ostringstream contentsStream;
    contentsStream << file.rdbuf();
    std::string contents = contentsStream.str();

    std::ostringstream ss;
    ss << "blob " << contents.size();
    ss << '\0';
    ss << contents;

    std::string blob = ss.str();
    
    std::string binhash = sha1bin(blob);
    std::string sha1hash = bin2hex(binhash);

    // zlib compress
    std::string compressed = zlibhelp::compress(blob);
    std::string dir = ".git/objects/" + sha1hash.substr(0, 2);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    std::string path = dir + "/" + sha1hash.substr(2, 2 * SHA_DIGEST_LENGTH);
    std::ofstream objectFile(path, std::ios::binary);

    if (!objectFile.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    objectFile.write(compressed.data(), compressed.size());

    return binhash;
}