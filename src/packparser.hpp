#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct GitObject {
    std::string type;
    std::string data;
    std::string sha;
};

std::unordered_map<std::string, GitObject> parse_packfile(const std::string& pack_data);
