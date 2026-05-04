#include <iostream>
#include <filesystem>
#include <fstream>
#include "sha1.hpp"
#include "zlibhelp.hpp"
#include <map>

namespace fs = std::filesystem;

std::string convert_to_hex(const std::string& bin_str) {
    std::string hex_str;
    hex_str.reserve(bin_str.size() * 2);
    char hex[] = "0123456789abcdef";
    for (unsigned char c : bin_str) {
        unsigned char high_nibble = (c >> 4) & 0x0F;
        unsigned char low_nibble = c & 0x0F;
        hex_str += hex[high_nibble];
        hex_str += hex[low_nibble];
    }
    return hex_str;
}
int lstree(int argc, char *argv[]){
    if(argc < 3) {
        std::cerr << "Usage: git-cpp ls-tree <flag> <tree-sha>\n";
        return EXIT_FAILURE;
    }
    std::map<std::string, std::string> mode_to_type = {
        {"40000", "tree"},
        {"100644", "blob"},
        {"100755", "blob"},
        {"120000", "blob"}
    };
    std::string flag = "";
    std::string tree_sha = "";
    if(argc == 3){
        tree_sha = argv[2];
    }
    else if(argc == 4){
        flag = argv[2];
        tree_sha = argv[3];
    }
    fs::path dir = ".git/objects/" + tree_sha.substr(0, 2) + "/" + tree_sha.substr(2);
    if(!fs::exists(dir)){
        std::cerr << "Tree object not found\n";
        return EXIT_FAILURE;
    }
    std::ifstream file(dir, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open object file: " << dir << '\n';
        return EXIT_FAILURE;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string buffer(size, '\0');
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Failed to read object file: " << dir << '\n';
        return EXIT_FAILURE;
    }
    file.close();
    try {
        std::string decompressed = zlibhelp::decompress(buffer);
        size_t pos = decompressed.find('\0');
        if (pos == std::string::npos) {
            std::cerr << "Invalid tree object format\n";
            return EXIT_FAILURE;
        }
        pos++;
        while (pos < decompressed.size()) {
            size_t space_pos = decompressed.find(' ', pos);
            if(space_pos == std::string::npos) {
                std::cerr << "Invalid tree object format\n";
                return EXIT_FAILURE;
            }
            std::string mode = decompressed.substr(pos, space_pos - pos);
            size_t null_pos = decompressed.find('\0', space_pos + 1);
            if(null_pos == std::string::npos) {
                std::cerr << "Invalid tree object format\n";
                return EXIT_FAILURE;
            }
            std::string name = decompressed.substr(space_pos + 1, null_pos - (space_pos + 1));
            if(null_pos + 21 > decompressed.size()) {
                std::cerr << "Invalid tree object format\n";
                return EXIT_FAILURE;
            }
            std::string sha = decompressed.substr(null_pos + 1, 20);
            std::string sha_hex = convert_to_hex(sha);
            if(!mode_to_type.count(mode)) {
                std::cerr << "Unknown mode " << mode << " for entry " << name << '\n';
                return EXIT_FAILURE;
            }
            std::string type = mode_to_type[mode];
            if(flag == "--name-only") {
                std::cout << name << '\n';
            }
            else if(flag == "--object-only") {
                std::cout << sha_hex << '\n';
            }
            else {
                std::cout << "0" << mode << ' ' << type << ' ' << sha_hex << '\t' << name << '\n';
            }
            pos = null_pos + 21;
        }
        return EXIT_SUCCESS;
    } catch (const std::runtime_error& e) {
        std::cerr << "Error decompressing object: " << e.what() << '\n';
        return EXIT_FAILURE;
    } 
    return EXIT_FAILURE;
}