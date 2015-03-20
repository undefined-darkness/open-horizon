//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "resources/resources.h"
#include <vector>

//------------------------------------------------------------

#ifdef _WIN32
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

//------------------------------------------------------------

class dpl_file
{
public:
    bool open(const char *name);
    void close();

    int get_files_count() const { return (int)m_infos.size(); }
    uint32_t get_file_size(int idx) const;
    bool read_file_data(int idx, void *data) const;

    dpl_file(): m_data(0), m_archieved(false), m_byte_order(false) {}

private:
    struct info
    {
        uint64_t offset;
        uint32_t size;
        uint32_t unpacked_size;
        unsigned char key;
    };

    std::vector<info> m_infos;
    nya_resources::resource_data *m_data;
    bool m_archieved;
    bool m_byte_order;
};

//------------------------------------------------------------
