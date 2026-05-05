#pragma once
#include <string>

void write_git_object(const std::string& dir, const std::string& type, const std::string& data);
std::string write_git_object_return_sha(const std::string& dir, const std::string& type, const std::string& data);
std::string read_git_object(const std::string& dir, const std::string& sha, std::string& type);
