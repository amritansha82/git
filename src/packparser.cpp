#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdexcept>
#include <functional>
#include <zlib.h>
#include "packparser.hpp"
#include "sha1.hpp"

static const std::string OBJ_TYPES[] = {
    "", "commit", "tree", "blob", "tag", "", "ofs_delta", "ref_delta"
};

static uint32_t read_32int(const std::string& data, size_t offset) {
    return ((uint8_t)data[offset] << 24) |
           ((uint8_t)data[offset + 1] << 16) |
           ((uint8_t)data[offset + 2] << 8) |
           ((uint8_t)data[offset + 3]);
}

static size_t read_size_encoding(const std::string& data, size_t& offset) {
    size_t result = 0;
    int shift = 0;
    while (true) {
        uint8_t byte = (uint8_t)data[offset++];
        result |= ((size_t)(byte & 0x7f)) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return result;
}

static std::string zlib_decompress_from(const std::string& data, size_t& offset) {
    z_stream strm;
    memset(&strm, 0, sizeof(strm));
    strm.avail_in = data.size() - offset;
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data() + offset));

    if (inflateInit(&strm) != Z_OK) {
        throw std::runtime_error("Failed to initialize zlib");
    }

    std::string decompressed;
    char buff[8192];
    int ret;
    do {
        strm.avail_out = sizeof(buff);
        strm.next_out = reinterpret_cast<Bytef*>(buff);
        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            throw std::runtime_error("Decompression error in packfile: " + std::to_string(ret));
        }
        decompressed.append(buff, sizeof(buff) - strm.avail_out);
    } while (ret != Z_STREAM_END);

    offset += (data.size() - offset) - strm.avail_in;
    inflateEnd(&strm);
    return decompressed;
}

static void apply_delta(const std::string& base, const std::string& delta, std::string& result) {
    size_t pos = 0;

    read_size_encoding(delta, pos);
    size_t result_size = read_size_encoding(delta, pos);

    result.clear();
    result.reserve(result_size);

    while (pos < delta.size()) {
        uint8_t cmd = (uint8_t)delta[pos++];

        if (cmd & 0x80) {
            size_t copy_offset = 0;
            size_t copy_size = 0;

            if (cmd & 0x01) copy_offset  = (uint8_t)delta[pos++];
            if (cmd & 0x02) copy_offset |= ((size_t)(uint8_t)delta[pos++]) << 8;
            if (cmd & 0x04) copy_offset |= ((size_t)(uint8_t)delta[pos++]) << 16;
            if (cmd & 0x08) copy_offset |= ((size_t)(uint8_t)delta[pos++]) << 24;

            if (cmd & 0x10) copy_size  = (uint8_t)delta[pos++];
            if (cmd & 0x20) copy_size |= ((size_t)(uint8_t)delta[pos++]) << 8;
            if (cmd & 0x40) copy_size |= ((size_t)(uint8_t)delta[pos++]) << 16;

            if (copy_size == 0) copy_size = 0x10000;

            result.append(base, copy_offset, copy_size);
        }
        else if (cmd > 0) {
            result.append(delta, pos, cmd);
            pos += cmd;
        }
        else {
            throw std::runtime_error("Unexpected delta command 0");
        }
    }
}

struct PackEntry {
    int type;
    std::string data;
    std::string ref_sha;
    size_t ofs_offset;
};

std::unordered_map<std::string, GitObject> parse_packfile(const std::string& pack_data) {
    if (pack_data.substr(0, 4) != "PACK") {
        throw std::runtime_error("Invalid pack header");
    }

    read_32int(pack_data, 4);
    uint32_t num_objects = read_32int(pack_data, 8);

    size_t offset = 12;

    std::vector<PackEntry> entries;
    std::vector<size_t> entry_offsets;
    entries.reserve(num_objects);
    entry_offsets.reserve(num_objects);

    for (uint32_t i = 0; i < num_objects; i++) {
        entry_offsets.push_back(offset);

        uint8_t byte = (uint8_t)pack_data[offset++];
        int type = (byte >> 4) & 0x07;
        size_t size = byte & 0x0f;
        int shift = 4;

        while (byte & 0x80) {
            byte = (uint8_t)pack_data[offset++];
            size |= ((size_t)(byte & 0x7f)) << shift;
            shift += 7;
        }

        PackEntry entry;
        entry.type = type;
        entry.ofs_offset = 0;

        if (type == 7) {
            entry.ref_sha = bin2hex(pack_data.substr(offset, 20));
            offset += 20;
        }
        else if (type == 6) {
            uint8_t b = (uint8_t)pack_data[offset++];
            size_t neg_offset = b & 0x7f;
            while (b & 0x80) {
                b = (uint8_t)pack_data[offset++];
                neg_offset = ((neg_offset + 1) << 7) | (b & 0x7f);
            }
            entry.ofs_offset = entry_offsets.back() - neg_offset;
        }

        entry.data = zlib_decompress_from(pack_data, offset);
        entries.push_back(std::move(entry));
    }

    std::unordered_map<size_t, size_t> offset_to_index;
    for (size_t i = 0; i < entry_offsets.size(); i++) {
        offset_to_index[entry_offsets[i]] = i;
    }

    std::vector<std::string> resolved_types(num_objects);
    std::vector<std::string> resolved_data(num_objects);
    std::vector<bool> resolved(num_objects, false);

    std::function<void(size_t)> resolve = [&](size_t idx) {
        if (resolved[idx]) return;

        PackEntry& e = entries[idx];

        if (e.type >= 1 && e.type <= 4) {
            resolved_types[idx] = OBJ_TYPES[e.type];
            resolved_data[idx] = e.data;
            resolved[idx] = true;
            return;
        }

        if (e.type == 6) {
            auto it = offset_to_index.find(e.ofs_offset);
            if (it == offset_to_index.end()) {
                throw std::runtime_error("ofs_delta base not found");
            }
            size_t base_idx = it->second;
            resolve(base_idx);
            std::string result;
            apply_delta(resolved_data[base_idx], e.data, result);
            resolved_types[idx] = resolved_types[base_idx];
            resolved_data[idx] = result;
            resolved[idx] = true;
            return;
        }

        if (e.type == 7) {
            return;
        }
    };

    for (size_t i = 0; i < num_objects; i++) {
        if (entries[i].type != 7) {
            resolve(i);
        }
    }

    std::unordered_map<std::string, size_t> sha_to_index;
    for (size_t i = 0; i < num_objects; i++) {
        if (resolved[i]) {
            std::ostringstream header;
            header << resolved_types[i] << " " << resolved_data[i].size();
            header << '\0';
            header << resolved_data[i];
            std::string sha = bin2hex(sha1bin(header.str()));
            sha_to_index[sha] = i;
        }
    }

    bool progress = true;
    while (progress) {
        progress = false;
        for (size_t i = 0; i < num_objects; i++) {
            if (resolved[i]) continue;
            if (entries[i].type != 7) continue;

            auto it = sha_to_index.find(entries[i].ref_sha);
            if (it == sha_to_index.end()) continue;

            size_t base_idx = it->second;
            std::string result;
            apply_delta(resolved_data[base_idx], entries[i].data, result);
            resolved_types[i] = resolved_types[base_idx];
            resolved_data[i] = result;
            resolved[i] = true;

            std::ostringstream header;
            header << resolved_types[i] << " " << resolved_data[i].size();
            header << '\0';
            header << resolved_data[i];
            std::string sha = bin2hex(sha1bin(header.str()));
            sha_to_index[sha] = i;

            progress = true;
        }
    }

    std::unordered_map<std::string, GitObject> objects;
    for (size_t i = 0; i < num_objects; i++) {
        if (!resolved[i]) {
            std::cerr << "Warning: unresolved object at index " << i << "\n";
            continue;
        }

        std::ostringstream header;
        header << resolved_types[i] << " " << resolved_data[i].size();
        header << '\0';
        header << resolved_data[i];
        std::string sha = bin2hex(sha1bin(header.str()));

        GitObject obj;
        obj.type = resolved_types[i];
        obj.data = resolved_data[i];
        obj.sha = sha;
        objects[sha] = std::move(obj);
    }

    return objects;
}
