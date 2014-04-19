//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <stdio.h>
#include <vector>
#include <string>

//------------------------------------------------------------

#ifdef _WIN32
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

//------------------------------------------------------------

class qdf_archive
{
public:
    bool open(const char *name);
    void close();

    int get_files_count() const { return int(m_fis.size()); }
    const char *get_file_name(int idx) const;
    uint64_t get_file_size(int idx) const;
    uint64_t get_file_offset(int idx) const;
    bool read_file_data( int idx, void *data ) const;

    uint64_t get_part_size() const { return m_part_size; }

    qdf_archive(): m_part_size(0) {}

private:
    struct qdf_file_info
    {
        std::string name;
        uint64_t offset;
        uint64_t size;
    };

    uint32_t m_part_size;
    std::vector<FILE *> m_rds;
    std::vector<qdf_file_info> m_fis;
};

//------------------------------------------------------------
