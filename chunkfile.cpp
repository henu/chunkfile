#include "chunkfile.hpp"

#include <cassert>

Chunkfile::Chunkfile(std::string const& path)
{
    buf = new uint8_t[BUF_SIZE];

    try {
        // Make sure the file exists
// TODO: This is needed because std::ios::app and .seekp() does not work together. And if std::ios::app is not used, then file is not created. Find more elegant way of doing this!
        file.open(path, std::ios::binary | std::ios::out | std::ios::app);
        file.close();

        // Open
        file.open(path, std::ios::binary | std::ios::in | std::ios::out);

        // Get size
        file.seekg(0, std::ios::end);
        file_size = file.tellg();
        file.seekg(0);

        // If file is new
        if (file_size == 0) {
            chunks = 0;
            chunk_space_reserved = 0;
            total_data_part_empty_space = 0;
            writeString("CHUNKFILE");
            writeUInt64(0);
            writeHeader();
            file_size = HEADER_SIZE;
        }
        // If file already exists
        else {
            if (file_size < HEADER_SIZE) {
                throw CorruptedFile();
            }
            // Read header
            std::string magic;
            readString(magic, 9);
            if (magic != "CHUNKFILE") {
                throw CorruptedFile();
            }
            uint64_t version = readUInt64();
            if (version != 0) {
                throw UnsupportedVersion();
            }
            chunks = readUInt64();
            chunk_space_reserved = readUInt64();
            total_data_part_empty_space = readUInt64();
        }
    }
    catch ( ... ) {
        delete[] buf;
        throw;
    }
}

Chunkfile::~Chunkfile()
{
    file.close();
    delete[] buf;
}

void Chunkfile::writeHeader()
{
    file_size = std::max<uint64_t>(file_size, HEADER_SIZE);
    assert(chunks <= chunk_space_reserved);
    assert(HEADER_SIZE + chunk_space_reserved * HEADERPART_SIZE + total_data_part_empty_space <= file_size);
    writeSeek(HEADER_MAGIC_AND_VERSION_SIZE);
    writeUInt64(chunks);
    writeUInt64(chunk_space_reserved);
    writeUInt64(total_data_part_empty_space);
}
