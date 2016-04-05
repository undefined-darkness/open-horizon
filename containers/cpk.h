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
        type_uint,
        type_float,
        type_string,
        type_data
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

    std::vector<column> columns;

    const value &get_value(const std::string &name, unsigned int row = 0) const;

    void debug_print() const;

    cri_utf_table(const void *data, size_t size);
};

class cpk_file
{
public:
    bool open(const char *name);
    void close();

private:
    nya_resources::resource_data *m_data = 0;
};

//------------------------------------------------------------
