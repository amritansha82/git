#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "checkout.hpp"
#include "objstore.hpp"
#include "sha1.hpp"

namespace fs = std::filesystem;

static void checkout_tree(const std::string& dir, const std::string& tree_sha, const std::string& path) {
    std::string type;
    std::string data = read_git_object(dir, tree_sha, type);

    if (type != "tree") {
        throw std::runtime_error("Expected tree object, got: " + type);
    }

    size_t pos = 0;
    while (pos < data.size()) {
        size_t space_pos = data.find(' ', pos);
        if (space_pos == std::string::npos) break;

        std::string mode = data.substr(pos, space_pos - pos);

        size_t null_pos = data.find('\0', space_pos + 1);
        if (null_pos == std::string::npos) break;

        std::string name = data.substr(space_pos + 1, null_pos - (space_pos + 1));

        if (null_pos + 20 > data.size()) break;
        std::string sha_bin = data.substr(null_pos + 1, 20);
        std::string sha = bin2hex(sha_bin);

        pos = null_pos + 21;

        std::string full_path = path + "/" + name;

        if (mode == "40000") {
            fs::create_directories(full_path);
            checkout_tree(dir, sha, full_path);
        }
        else {
            std::string blob_type;
            std::string blob_data = read_git_object(dir, sha, blob_type);

            std::ofstream file(full_path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Could not create file: " + full_path);
            }
            file.write(blob_data.data(), blob_data.size());
            file.close();

            if (mode == "100755") {
                fs::permissions(full_path,
                    fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add);
            }
        }
    }
}

void checkout_commit(const std::string& dir, const std::string& commit_sha) {
    std::string type;
    std::string data = read_git_object(dir, commit_sha, type);

    if (type != "commit") {
        throw std::runtime_error("Expected commit object, got: " + type);
    }

    size_t tree_pos = data.find("tree ");
    if (tree_pos == std::string::npos) {
        throw std::runtime_error("No tree found in commit");
    }
    size_t newline = data.find('\n', tree_pos);
    std::string tree_sha = data.substr(tree_pos + 5, newline - (tree_pos + 5));

    checkout_tree(dir, tree_sha, dir);
}
