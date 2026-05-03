#include <iostream>
#include <filesystem>
#include <fstream>
#include "sha1.hpp"
#include "zlibhelp.hpp"

int hashobject(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: hash-object -w <file>\n";
    }

    std::string flag = argv[2];

    if (flag == "-w") {
        std::string filename = argv[3];
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Could not open file " << filename << '\n';
            return EXIT_FAILURE;
        }

        std::ostringstream contentsStream;
        contentsStream << file.rdbuf();
        std::string contents = contentsStream.str();

        std::ostringstream ss;
        ss << "blob " << contents.size();
        ss << '\0';
        ss << contents;

        std::string blob = ss.str();
        
        std::string sha1hash = sha1(blob);
        std::cout << sha1hash << '\n';

        // zlib compress
        try {
            std::string compressed = zlibhelp::compress(blob);
            std::string dir = ".git/objects/" + sha1hash.substr(0, 2);
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directory(dir);
            }
            std::string path = dir + "/" + sha1hash.substr(2, 2 * SHA_DIGEST_LENGTH);
            std::ofstream objectFile(path, std::ios::binary);

            if (!objectFile.is_open()) {
                std::cerr << "Could not open file " << path << '\n';
                return EXIT_FAILURE;
            }

            objectFile.write(compressed.data(), compressed.size());
            
            return EXIT_SUCCESS;
        } catch (std::runtime_error &e) {
            std::cerr << "Error: " << e.what() << '\n';

            return EXIT_FAILURE;
        }
    }

    return EXIT_FAILURE;
}