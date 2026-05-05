#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <cstdlib>
#include "commands.hpp"
#include "zlibhelp.hpp"
#include "sha1.hpp"
#include "packparser.hpp"
#include "objstore.hpp"
#include "checkout.hpp"
#include <curl/curl.h>

namespace fs = std::filesystem;

static size_t cb_write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    std::string *data = static_cast<std::string *>(userdata);
    data->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

std::string http_request(const std::string& url, const std::string& method, const std::string& payload = "") {
    std::string response;
    CURL *curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb_write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "git/codecrafters");

    struct curl_slist* headers = NULL;
    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size());
        headers = curl_slist_append(headers, "Content-Type: application/x-git-upload-pack-request");
        headers = curl_slist_append(headers, "Accept: application/x-git-upload-pack-result");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
    
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

static std::string extract_head_sha(const std::string& response) {
    size_t pos = response.find("HEAD");
    if (pos == std::string::npos) {
        throw std::runtime_error("Failed to find HEAD reference in response");
    }
    if (pos < 41) {
        throw std::runtime_error("Invalid response format");
    }
    return response.substr(pos - 41, 40);
}

static std::string build_want_request(const std::string& sha) {
    std::string request;
    request += "0032want " + sha + "\n";
    request += "00000009done\n";
    return request;
}

static std::string extract_pack_data(const std::string& response) {
    size_t pack_start = response.find("PACK");
    if (pack_start == std::string::npos) {
        throw std::runtime_error("Failed to find PACK data in response");
    }
    return response.substr(pack_start);
}

int clone(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: <program> clone <repository_url> <directory>\n";
        return EXIT_FAILURE;
    }

    std::string repo_url = argv[2];
    std::string directory = argv[3];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    fs::create_directories(directory + "/.git/objects");
    fs::create_directories(directory + "/.git/refs/heads");
    std::ofstream(directory + "/.git/HEAD") << "ref: refs/heads/master\n";

    std::string refs_url = repo_url + "/info/refs?service=git-upload-pack";
    std::string refs_response = http_request(refs_url, "GET");
    std::string head_sha = extract_head_sha(refs_response);

    std::string pack_url = repo_url + "/git-upload-pack";
    std::string payload = build_want_request(head_sha);
    std::string pack_response = http_request(pack_url, "POST", payload);
    std::string pack_data = extract_pack_data(pack_response);

    auto objects = parse_packfile(pack_data);

    for (auto& [sha, obj] : objects) {
        write_git_object(directory, obj.type, obj.data);
    }

    fs::create_directories(directory + "/.git/refs/heads");
    std::ofstream(directory + "/.git/refs/heads/master") << head_sha << "\n";

    checkout_commit(directory, head_sha);

    curl_global_cleanup();
    return EXIT_SUCCESS;
}