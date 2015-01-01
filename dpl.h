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

    dpl_file(): m_data(0) {}

private:
    struct info
    {
        uint64_t offset;
        uint32_t size;
    };

    std::vector<info> m_infos;
    nya_resources::resource_data *m_data;
};

//------------------------------------------------------------
