#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include "blobwriter.hpp"
#include "sha1.hpp"
#include "zlibhelp.hpp"

namespace fs = std::filesystem;

const int MODE_DIR = 40000;
const int MODE_FILE = 100644;
const int MODE_EXEC = 100755;

struct entry {
    std::string name;
    int mode;
    std::string hash;
};

class tree {
public:
    std::vector<entry> entries;
    static tree create(std::string);
    std::string hash();
    std::string to_string();
};

tree tree::create(std::string path) {
    tree t;
    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.is_directory()) {
            if (entry.path().filename() == ".git") {
                continue;
            }
            tree t2 = tree::create(entry.path());
            t.entries.push_back({
                entry.path().filename().string(),
                MODE_DIR,
                t2.hash(),
            });
        }
        else {
            std::ifstream file(entry.path());
            std::string hash = create_blob(file);
            if ((entry.status().permissions() & fs::perms::owner_exec) != fs::perms::none) {
                t.entries.push_back({
                    entry.path().filename().string(),
                    MODE_EXEC,
                    hash
                });
            }
            else {
                t.entries.push_back({
                    entry.path().filename().string(),
                    MODE_FILE,
                    hash
                });
            }
        }
    }
    sort(t.entries.begin(), t.entries.end(), [](const entry& a, const entry& b) {
        return a.name < b.name;
    });

    // write the tree object
    std::string contents = t.to_string();
    std::string binhash = t.hash();
    std::string sha1hash = bin2hex(binhash);

    // zlib compress
    std::string compressed = zlibhelp::compress(contents);
    std::string dir = ".git/objects/" + sha1hash.substr(0, 2);
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }

    std::string objPath = dir + "/" + sha1hash.substr(2, 2 * SHA_DIGEST_LENGTH);
    std::ofstream objectFile(objPath, std::ios::binary);

    if (!objectFile.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    objectFile.write(compressed.data(), compressed.size());

    return t;
}

std::string tree::hash() {
    std::string hash = sha1bin(to_string());
    return hash;
}

std::string tree::to_string() {
    std::ostringstream ss;
    for (const auto& entry : entries) {
        ss << entry.mode << " " << entry.name << '\0' << entry.hash;
    }
    std::ostringstream ss2;
    ss2 << "tree " << ss.str().size();
    ss2 << '\0' << ss.str();
    return ss2.str();
}

int writetree(int argc, char *argv[]) {
    try {
        tree t = tree::create(".");
        std::cout << bin2hex(t.hash()) << std::endl;\
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}