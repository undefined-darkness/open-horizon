//
// open horizon -- undefined_darkness@outlook.com
//

// plain old container, used as internal container in ac < 6

#pragma once

#include "resources/resources.h"
#include <vector>
#include <stdint.h>

//------------------------------------------------------------

class poc_file
{
public:
    bool open(const char *name);
    bool open(nya_resources::resource_data *data);
    bool open(const void *data, size_t size); //inplace
    void close();

    uint32_t get_chunk_type(int idx) const;
    int get_chunks_count() const { return int(m_entries.size()); }
    uint32_t get_chunk_size(int idx) const;
    uint32_t get_chunk_offset(int idx) const;

    bool read_chunk_data(int idx, void *data) const;
    bool read_chunk_data(int idx, void *data, uint32_t size, uint32_t offset = 0) const;

private:
    bool init(const uint32_t *offsets, uint32_t count, uint32_t size);

private:
    struct entry
    {
        uint32_t type;
        uint32_t offset;
        uint32_t size;
    };

    std::vector<entry> m_entries;
    nya_resources::resource_data *m_data = 0;
    const char *m_raw_data = 0;
};

//------------------------------------------------------------
