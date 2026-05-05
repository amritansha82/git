#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include "objstore.hpp"
#include "sha1.hpp"
#include "zlibhelp.hpp"

namespace fs = std::filesystem;

void write_git_object(const std::string& dir, const std::string& type, const std::string& data) {
    write_git_object_return_sha(dir, type, data);
}

std::string write_git_object_return_sha(const std::string& dir, const std::string& type, const std::string& data) {
    std::ostringstream header;
    header << type << " " << data.size();
    header << '\0';
    header << data;
    std::string store = header.str();

    std::string binhash = sha1bin(store);
    std::string sha = bin2hex(binhash);

    std::string obj_dir = dir + "/.git/objects/" + sha.substr(0, 2);
    if (!fs::exists(obj_dir)) {
        fs::create_directories(obj_dir);
    }

    std::string obj_path = obj_dir + "/" + sha.substr(2);
    if (fs::exists(obj_path)) {
        return sha;
    }

    std::string compressed = zlibhelp::compress(store);
    std::ofstream file(obj_path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not write object file: " + obj_path);
    }
    file.write(compressed.data(), compressed.size());
    return sha;
}

std::string read_git_object(const std::string& dir, const std::string& sha, std::string& type) {
    std::string obj_path = dir + "/.git/objects/" + sha.substr(0, 2) + "/" + sha.substr(2);
    std::ifstream file(obj_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open object: " + sha);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string buffer(size, '\0');
    file.read(buffer.data(), size);
    file.close();

    std::string decompressed = zlibhelp::decompress(buffer);
    size_t space = decompressed.find(' ');
    type = decompressed.substr(0, space);
    size_t null_pos = decompressed.find('\0');
    return decompressed.substr(null_pos + 1);
}
