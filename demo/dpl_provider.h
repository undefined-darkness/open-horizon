//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "dpl.h"
#include "xml.h"
#include "resources/resources.h"
#include <string.h>

//------------------------------------------------------------

class dpl_resources_provider: public nya_resources::resources_provider
{
public:
    nya_resources::resource_data *access(const char *resource_name)
    {
        for (auto &e:m_entries)
        {
            if (e.name == resource_name)
                return new res_data(m_archive, e.idx, e.sub);
        }

        return 0;
    }

    bool has(const char *resource_name)
    {
        if (!resource_name)
            return false;

        for (auto &e:m_entries)
        {
            if (e.name == resource_name)
                return true;
        }

        return false;
    }

public:
    int get_resources_count() { return (int)m_entries.size(); }
    const char *get_resource_name(int idx) { return m_entries[idx].name.c_str(); }

public:
    bool open_archive(const char *name, const char *xml_name)
    {
        if (!m_archive.open(name))
            return false;

        pugi::xml_document doc;
        if (!load_xml(xml_name, doc))
            return false;

        pugi::xml_node root = doc.child("dpl");
        if (!root)
            return false;

        std::string curr_path;

        for (pugi::xml_node object = root.first_child(); object; object = object.next_sibling())
        {
            if (strcmp(object.name(), "path") == 0)
                curr_path = object.attribute("name").as_string("");

            if (strcmp(object.name(), "entry") == 0)
            {
                entry e;
                e.name = curr_path + object.attribute("name").as_string("");
                e.idx = object.attribute("idx").as_int();
                e.sub = object.attribute("sub").as_int();
                m_entries.push_back(e);
            }
        }

        return true;
    }

private:
    dpl_file m_archive;

    struct entry { std::string name; int idx, sub; };
    std::vector<entry> m_entries;

    struct res_data: nya_resources::resource_data
    {
        dpl_entry entry;
        int idx;
        nya_memory::tmp_buffer_ref buf;

        res_data(dpl_file &a, int i, int s): idx(s), entry(0, 0)
        {
            buf.allocate(a.get_file_size(i));
            a.read_file_data(i, buf.get_data());
            entry = dpl_entry(buf.get_data(), (uint32_t)buf.get_size());
        }

        size_t get_size() { return entry.get_file_size(idx); }
        bool read_all(void*data) { return memcpy(data, entry.get_file_data(idx), get_size()); }
        bool read_chunk(void *data, size_t size, size_t offset = 0)
        {
            return memcpy(data, (char *)entry.get_file_data(idx) + offset, size);
        }

        void release() { buf.free(); delete this; }
    };
};

//------------------------------------------------------------
