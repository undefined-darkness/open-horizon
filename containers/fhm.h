//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "resources/resources.h"
#include <string.h>
#include <stdint.h>
#include <vector>

//------------------------------------------------------------

class fhm_file
{
public:
    bool open(const char *name);
    bool open(nya_resources::resource_data *data);
    void close();

    int get_chunks_count() const { return int(m_chunks.size()); }

    unsigned int get_chunk_type(int idx) const;
    size_t get_chunk_size(int idx) const;
    bool read_chunk_data(int idx, void *data) const;

    size_t get_chunk_offset(int idx) const;

    fhm_file(): m_data(0) {}

public:
    struct fhm_header
    {
        char sign[4];
        uint32_t byte_order_20101010;
        uint32_t timestamp;
        uint32_t unknown_struct_count;

        uint32_t unknown;
        uint32_t size;
        uint32_t unknown3;
        uint32_t unknown4;
        uint32_t unknown5;
        uint32_t unknown_16;
        uint32_t unknown_pot;
        uint32_t unknown_pot2;

        bool check_sign() const { return memcmp(sign, "FHM", 3) == 0; }
        bool wrong_byte_order() const { return byte_order_20101010 != 20101010; }
    };

private:
    bool read_chunks_info(size_t base_offset, int nesting, int &group);

    struct chunk
    {
        unsigned int type;
        size_t offset;
        size_t size;
    };

    std::vector<chunk> m_chunks;

    nya_resources::resource_data *m_data;
};

//------------------------------------------------------------
