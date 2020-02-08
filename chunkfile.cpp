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

void Chunkfile::reserve(uint64_t new_reserve)
{
    if (chunk_space_reserved >= new_reserve) {
        return;
    }

    uint64_t data_area_begin = HEADER_SIZE + chunk_space_reserved * HEADERPART_SIZE;
    uint64_t new_data_area_begin = HEADER_SIZE + new_reserve * HEADERPART_SIZE;
    uint64_t new_space_needed = new_data_area_begin - data_area_begin;

    if (file_size < data_area_begin) {
        throw CorruptedFile();
    }

    // There must be free space in the beginning of data area before header area
    // can be grown. Move data parts away until there is enough free space.
    bool increase_file_size = false;
    bool create_new_empty_data_part = false;
    uint8_t chunk_type = -1;
    uint64_t chunk_size = -1;
    while (true) {
        // If the data area is empty
        if (file_size == data_area_begin) {
            increase_file_size = true;
            break;
        }

        // Read the data part at the beginning of data area
        readSeek(data_area_begin);
        readUInt63AndUInt1(chunk_size, chunk_type);

        // If there is exactly correct amount of free space
        if (chunk_type == DATAPART_TYPE_FREESPACE && chunk_size == new_space_needed) {
            assert(total_data_part_empty_space >= new_space_needed);
            total_data_part_empty_space -= new_space_needed;
            break;
        }

        // If there is enough free space. The space requirement is a little bit
        // bigger, because it needs to include header of the new first data part.
        if (chunk_type == DATAPART_TYPE_FREESPACE && chunk_size >= new_space_needed + DATAPART_FREESPACE_MIN_SIZE) {
            create_new_empty_data_part = true;
            break;
        }

// TODO: Code more complex cases!
throw std::runtime_error("Not implemented yet!");
    }

    // Do required fixes
    if (increase_file_size) {
        file_size += new_space_needed;
    } else if (create_new_empty_data_part) {
        // Replace the first data part with a new one
        assert(chunk_size >= DATAPART_FREESPACE_MIN_SIZE + new_space_needed);
        uint64_t chunk_new_size = chunk_size - new_space_needed;
        writeSeek(new_data_area_begin);
        writeUInt63AndUInt1(chunk_new_size, DATAPART_TYPE_FREESPACE);
        assert(total_data_part_empty_space >= new_space_needed);
        total_data_part_empty_space -= new_space_needed;
    }

    // Initialize new header parts
    writeSeek(data_area_begin);
    for (uint64_t chunk_id = chunk_space_reserved; chunk_id < new_reserve; ++ chunk_id) {
        writeUInt64(MINUS_ONE);
    }

    chunk_space_reserved = new_reserve;
    writeHeader();
}

bool Chunkfile::exists(uint64_t chunk_id)
{
    if (chunk_id >= chunk_space_reserved) {
        return false;
    }

    readSeek(HEADER_SIZE + HEADERPART_SIZE * chunk_id);
    uint64_t datapart_pos = readUInt64();
    return datapart_pos != MINUS_ONE;
}

void Chunkfile::set(uint64_t chunk_id, uint8_t const* bytes, uint64_t size)
{
    // If more chunk space needs to be allocated
    if (chunk_id >= chunk_space_reserved) {
        reserve(std::max(chunk_id + 1, chunk_space_reserved * 2));
    }

    // If old chunk needs to be cleared first
    if (exists(chunk_id)) {
// TODO: Code this!
throw std::runtime_error("Not implemented yet!");
    } else {
        ++ chunks;
    }

    // Get properties of new data part
    uint64_t datapart_size = DATAPART_DATA_MIN_SIZE + size;
    uint64_t datapart_pos = findFreeSpace(size);
    uint64_t datapart_end_pos = datapart_pos + datapart_size;

    // Check if there is an existing data part here. Should be empty if there is.
    if (datapart_pos < file_size) {
        // Read existing data part
        readSeek(datapart_pos);
        uint64_t free_space_size;
        uint8_t free_space_type;
        readUInt63AndUInt1(free_space_size, free_space_type);
        assert(free_space_type == DATAPART_TYPE_FREESPACE);
        // "Move" it after the new data part, if there is space
        if (datapart_end_pos > file_size) {
            throw CorruptedFile();
        }
        if (datapart_end_pos < file_size) {
            assert(datapart_end_pos + DATAPART_FREESPACE_MIN_SIZE <= file_size);
            assert(free_space_size >= DATAPART_FREESPACE_MIN_SIZE + datapart_size);
            writeSeek(datapart_end_pos);
            writeUInt63AndUInt1(free_space_size - datapart_size, DATAPART_TYPE_FREESPACE);
        }

        assert(total_data_part_empty_space >= datapart_size);
        total_data_part_empty_space -= datapart_size;
    } else {
        assert(datapart_pos == file_size);
        file_size += datapart_size;
    }

    // Create new chunk
    // Header part
    writeSeek(HEADER_SIZE + chunk_id * HEADERPART_SIZE);
    writeUInt64(datapart_pos);
    // Data part
    writeSeek(datapart_pos);
    writeUInt63AndUInt1(DATAPART_DATA_MIN_SIZE + size, DATAPART_TYPE_DATA);
    writeUInt64(chunk_id);
    writeBytes(bytes, size);

    writeHeader();
}

uint64_t Chunkfile::getChunkSize(uint64_t chunk_id)
{
    uint64_t data_part_pos = getDataPartPosition(chunk_id);
    readSeek(data_part_pos);
    uint64_t data_part_size;
    uint8_t data_part_type;
    readUInt63AndUInt1(data_part_size, data_part_type);
    if (data_part_type != DATAPART_TYPE_DATA) {
        throw CorruptedFile();
    }
    return data_part_size - DATAPART_DATA_MIN_SIZE;
}

void Chunkfile::get(uint8_t* result, uint64_t chunk_id)
{
    uint64_t data_part_pos = getDataPartPosition(chunk_id);
    readSeek(data_part_pos);
    uint64_t data_part_size;
    uint8_t data_part_type;
    readUInt63AndUInt1(data_part_size, data_part_type);
    if (data_part_type != DATAPART_TYPE_DATA) {
        throw CorruptedFile();
    }
    uint64_t check_chunk_id = readUInt64();
    if (check_chunk_id != chunk_id) {
        throw CorruptedFile();
    }
    readBytes(result, data_part_size - DATAPART_DATA_MIN_SIZE);
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

uint64_t Chunkfile::findFreeSpace(uint64_t size)
{
    // For now, just return the end of the file.
(void)size;
// TODO: In the future, try to do some optimization if there is lots of free space.
    return file_size;
}

uint64_t Chunkfile::getDataPartPosition(uint64_t chunk_id)
{
    if (chunk_id >= chunk_space_reserved) {
        throw ChunkDoesNotExist();
    }
    readSeek(HEADER_SIZE + chunk_id * HEADERPART_SIZE);
    uint64_t data_part_pos = readUInt64();
    if (data_part_pos == MINUS_ONE) {
        throw ChunkDoesNotExist();
    }
    return data_part_pos;
}
