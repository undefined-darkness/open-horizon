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

class dpl_entry
{
public:
    int get_files_count() const { return (int)m_entries.size(); }
    uint32_t get_file_size(int idx) const { return m_entries[idx].second; }
    const void *get_file_data(int idx) const { return (char *)m_data + m_entries[idx].first; }

    dpl_entry(const void *data, uint32_t size): m_data(data), m_size(size) { read_entries(0); }

private:
    void read_entries(uint32_t offset);

private:
    const void *m_data;
    uint32_t m_size;
    std::vector<std::pair<uint32_t,uint32_t> > m_entries;
};

//------------------------------------------------------------
