#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "sha1.hpp"
#include "zlibhelp.hpp"

int committree(int argc, char *argv[]) {
    if (argc < 7) {
        std::cerr << "Usage: commit-tree <tree-sha> -p <commit-sha> -m <message>\n";
    }

    std::string treeSha = argv[2];
    std::string commitSha = argv[4];
    std::string message = argv[6];

    std::ostringstream ss;
    ss << "tree " << treeSha << '\n';
    ss << "parent " << commitSha << '\n';
    ss << "author John Doe <john@doe.com> 1683787800 +0200\n";
    ss << "committer John Doe <john@doe.com> 1683787800 +0200\n";
    ss << '\n';
    ss << message << '\n';

    std::string commit = ss.str();

    std::ostringstream commitStream;
    commitStream << "commit " << commit.size();
    commitStream << '\0';
    commitStream << commit;

    std::string commitHash = bin2hex(sha1bin(commitStream.str()));

    std::string dir = ".git/objects/" + commitHash.substr(0, 2);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    std::string path = dir + "/" + commitHash.substr(2, 2 * SHA_DIGEST_LENGTH);
    std::ofstream objectFile(path, std::ios::binary);

    if (!objectFile.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    // zlib compress
    std::string compressed = zlibhelp::compress(commitStream.str());
    objectFile.write(compressed.data(), compressed.size());

    std::cout << commitHash << std::endl;

    return EXIT_SUCCESS;
}
