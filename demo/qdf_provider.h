//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "qdf.h"
#include "resources/resources.h"

//------------------------------------------------------------

class qdf_resources_provider: public nya_resources::resources_provider
{
public:
    nya_resources::resource_data *access(const char *resource_name)
    {
        const int idx = m_archive.get_file_idx(resource_name);
        if (idx < 0)
            return 0;

        return new res_data(m_archive, idx);
    }

    bool has(const char *resource_name)
    {
        return m_archive.get_file_idx(resource_name) >= 0;
    }

public:
    bool open_archive(const char *name)
    {
        return m_archive.open(name);
    }

private:
    qdf_archive m_archive;

    struct res_data: nya_resources::resource_data
    {
        qdf_archive &arch;
        int idx;

        res_data(qdf_archive &a, int i): arch(a), idx(i) {}

        size_t get_size() { return arch.get_file_size(idx); }
        bool read_all(void*data) { return arch.read_file_data(idx, data); }
        bool read_chunk(void *data, size_t size, size_t offset = 0)
        {
            return arch.read_file_data(idx, data, size, offset);
        }

        void release() { delete this; }
    };
};

//------------------------------------------------------------
