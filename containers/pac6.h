//
// open horizon -- undefined_darkness@outlook.com
//

// ac6 base container

#pragma once

#include "resources/resources.h"
#include <vector>

//------------------------------------------------------------

#ifdef _WIN32
typedef unsigned __int32 uint32_t;
#endif

//------------------------------------------------------------

class pac6_file
{
public:
    bool open(const char *name);
    void close();

    int get_files_count() const { return int(m_entries.size()); }
    uint32_t get_file_size(int idx) const;

    bool read_file_data(int idx, void *data) const;

private:
    struct entry
    {
        unsigned char tome;
        bool compressed;
        uint32_t offset;
        uint32_t size;
        uint32_t unpacked_size;
    };

    std::vector<entry> m_entries;
    std::vector<nya_resources::resource_data *> m_data;
};

//------------------------------------------------------------
