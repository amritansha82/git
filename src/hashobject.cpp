#include <iostream>
#include <filesystem>
#include <fstream>
#include "sha1.hpp"
#include "zlibhelp.hpp"
#include "blobwriter.hpp"

int hashobject(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: hash-object -w <file>\n";
    }

    std::string flag = argv[2];

    if (flag == "-w") {
        std::string filename = argv[3];
        std::ifstream file(filename);

        try {
            std::string hash = create_blob(file);
            std::cout << bin2hex(hash) << std::endl;    
            return EXIT_SUCCESS;
        } catch (std::runtime_error& e) {
            std::cerr << e.what() << '\n';
        }
    }

    return EXIT_FAILURE;
}