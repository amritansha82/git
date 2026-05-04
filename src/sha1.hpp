#pragma once
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <openssl/sha.h>

std::string sha1bin(std::string filename);
std::string bin2hex(const std::string &bin);