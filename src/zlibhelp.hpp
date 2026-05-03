#pragma once
#include <string>
#include <stdexcept>
#include <cstring>
#include <zlib.h>

class zlibhelp {
public:
    static std::string decompress(const std::string& compressed);
    static std::string compress(const std::string& data);
    
};