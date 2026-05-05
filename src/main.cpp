#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include "commands.hpp"
#include "zlibhelp.hpp"

int main(int argc, char *argv[])
{
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    if (argc < 2) {
        std::cerr << "No command provided.\n";
        return EXIT_FAILURE;
    }
    
    std::string command = argv[1];
    
    if (command == "init") {
        return init(argc, argv);
    }
    else if (command == "cat-file") {
        return catfile(argc, argv);
    } 
    else if (command == "hash-object") {
        return hashobject(argc, argv);
    }
    else if (command == "ls-tree") {
        return lstree(argc, argv);
    }
    else if (command == "write-tree") {
        return writetree(argc, argv);
    }
    else if (command == "commit-tree") {
        return committree(argc, argv);
    }
    else if (command == "clone") {
        return clone(argc, argv);
    }
    else {
        std::cerr << "Unknown command " << command << '\n';
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
