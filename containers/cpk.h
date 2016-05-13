//
// open horizon -- undefined_darkness@outlook.com
//

// cri base container

#pragma once

#include "resources/resources.h"
#include <vector>
#include <stdint.h>

//------------------------------------------------------------

struct cri_utf_table
{
    enum value_type
    {
        type_uint = 'u',
        type_float = 'f',
        type_string = 's',
        type_data = 'd'
    };

    struct value
    {
        value_type type;

        uint64_t u = 0;
        float f = 0.0f;
        std::string s;
        std::vector<char> d;
    };

    struct column
    {
        std::string name;
        std::vector<value> values;
    };

    std::string name;
    std::vector<column> columns;
    int num_rows = 0;

    const column &get_column(const std::string &name) const;
    const value &get_value(const std::string &name, int row = 0) const;

    void debug_print() const;

    cri_utf_table() {}
    cri_utf_table(const void *data, size_t size);
    cri_utf_table(const std::vector<char> &d): cri_utf_table(d.data(), d.size()) {}
};

//------------------------------------------------------------

class cpk_file
{
public:
    bool open(const char *name);
    bool open(nya_resources::resource_data *data);
    void close();

    int get_files_count() const { return int(m_entries.size()); }
    uint32_t get_file_size(int idx) const;

    bool read_file_data(int idx, void *data) const;
    bool read_file_data(int idx, void *data, uint32_t size, uint32_t offset = 0) const;

private:
    struct entry
    {
        uint32_t id;
        uint32_t offset;
        uint32_t size;
    };

    std::vector<entry> m_entries;
    nya_resources::resource_data *m_data = 0;
};

//------------------------------------------------------------
