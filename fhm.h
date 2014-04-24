//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "resources/resources.h"
#include <vector>

//------------------------------------------------------------

class fhm_file
{
public:
    bool open(const char *name);
    void close();

    int get_chunks_count() const { return int(m_chunks.size()); }

    unsigned int get_chunk_type(int idx) const;
    size_t get_chunk_size(int idx) const;
    bool read_chunk_data(int idx, void *data) const;

    size_t get_chunk_offset(int idx) const;

    fhm_file(): m_data(0) {}

private:
    bool read_chunks_info(size_t base_offset);

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
