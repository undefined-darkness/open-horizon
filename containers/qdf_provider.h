//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "qdf.h"
#include "resources/resources.h"
#include <map>

//------------------------------------------------------------

class qdf_resources_provider: public nya_resources::resources_provider
{
public:
    nya_resources::resource_data *access(const char *resource_name)
    {
        if (!resource_name)
            return 0;

        auto e = m_files.find(resource_name);
        if (e == m_files.end())
            return 0;

        return new res_data(m_archive, e->second);
    }

    bool has(const char *resource_name)
    {
        return resource_name && m_files.find(resource_name) != m_files.end();
    }

public:
    int get_resources_count() { return m_archive.get_files_count(); }
    const char *get_resource_name(int idx) { return m_archive.get_file_name(idx); }

public:
    bool open_archive(const char *name)
    {
        if (!m_archive.open(name))
            return false;

        for (int i = 0; i < m_archive.get_files_count(); ++i)
            m_files[m_archive.get_file_name(i)] = i;

        return true;
    }

private:
    qdf_archive m_archive;
    std::map<std::string,int> m_files;

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
