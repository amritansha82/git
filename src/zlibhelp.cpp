#include <iostream>
#include <string>
#include <stdexcept>
#include <cstring>
#include <zlib.h>

class zlibhelp{
public:
    static std::string decompress(const std::string& compressed) {
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        strm.avail_in = compressed.size();
        strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.data()));
        if (inflateInit(&strm) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib");
        }
        std::string decompressed;
        char buff[4096];
        int ret;
        do {
            strm.avail_out = sizeof(buff);
            strm.next_out = reinterpret_cast<Bytef*>(buff);

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                inflateEnd(&strm);
                throw std::runtime_error("Decompression error: " + std::to_string(ret));
            }

            decompressed.append(buff, sizeof(buff) - strm.avail_out);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);
        return decompressed;
    }
    static std::string compress(const std::string& data) {
        z_stream strm;
        memset(&strm, 0, sizeof(strm));
        if (deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK) {
            throw std::runtime_error("Failed to initialize zlib for compression");
        }
        strm.avail_in = data.size();
        strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));
        std::string compressed;
        char buff[4096];
        int ret;
        do {
            strm.avail_out = sizeof(buff);
            strm.next_out = reinterpret_cast<Bytef*>(buff);

            ret = deflate(&strm, Z_FINISH);
            if (ret == Z_STREAM_ERROR || ret == Z_BUF_ERROR) {
                deflateEnd(&strm);
                throw std::runtime_error("Compression error: " + std::to_string(ret));
            }
            compressed.append(buff, sizeof(buff) - strm.avail_out);
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);
        return compressed;
    }
};