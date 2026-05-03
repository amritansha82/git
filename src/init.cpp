#include "commands.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdlib>

namespace fs = std::filesystem;
int init(int argc, char *argv[]) {
    try {
        fs::create_directory(".git");
        fs::create_directory(".git/objects");
        fs::create_directory(".git/refs");

        std::ofstream headFile(".git/HEAD");
        if (headFile.is_open()) {
            headFile << "ref: refs/heads/main\n";
            headFile.close();
        } else {
            std::cerr << "Failed to create .git/HEAD file.\n";
            return EXIT_FAILURE;
        }

        std::cout << "Initialized git directory\n";
        return EXIT_SUCCESS;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}