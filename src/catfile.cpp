#include "commands.hpp"
#include "zlibhelp.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <zlib.h>

namespace fs = std::filesystem;

int catfile(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: cat-file <flag> <object>\n";
        return EXIT_FAILURE;
    }
    std::string flag = argv[2];
    if(flag == "-p"){
        std::string hash = argv[3];
        fs::path path = ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
        if (!fs::exists(path)) {
            std::cerr << "Object not found: " << hash << '\n';
            return EXIT_FAILURE;
        }
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open object file: " << path << '\n';
            return EXIT_FAILURE;
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string buffer(size, '\0');
        if (!file.read(buffer.data(), size)) {
            std::cerr << "Failed to read object file: " << path << '\n';
            return EXIT_FAILURE;
        }
        file.close();
        try {
            std::string decompressed = zlibhelp::decompress(buffer);
            std::string content = decompressed.substr(decompressed.find('\0') + 1);
            std::cout << content;
            return EXIT_SUCCESS;
        } catch (const std::runtime_error& e) {
            std::cerr << "Error decompressing object: " << e.what() << '\n';
            return EXIT_FAILURE;
        } 
    }
    if(flag == "-s"){
        std::string hash = argv[3];
    }
    return EXIT_FAILURE;
}