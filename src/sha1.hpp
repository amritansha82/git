#pragma once
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <openssl/sha.h>

std::string sha1(std::string filename);