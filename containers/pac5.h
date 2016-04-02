//
// open horizon -- undefined_darkness@outlook.com
//

// ac5 and acz base container

#pragma once

#include "resources/resources.h"
#include <vector>
#include <stdint.h>

//------------------------------------------------------------

class pac5_file
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
        uint32_t offset;
        uint32_t size;
        uint32_t unpacked_size;
    };

    std::vector<entry> m_entries;
    nya_resources::resource_data *m_data = 0;
    bool m_compressed = false;
};

//------------------------------------------------------------
