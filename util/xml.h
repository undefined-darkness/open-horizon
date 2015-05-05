//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "pugixml.hpp"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"

//------------------------------------------------------------

inline bool load_xml(const char *name, pugi::xml_document &doc)
{
    if (!name)
        return false;

    nya_resources::resource_data *res = nya_resources::get_resources_provider().access(name);
    if (!res)
        return false;

    nya_memory::tmp_buffer_scoped buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();

    return doc.load_buffer((const char *)buf.get_data(), buf.get_size());
}

//------------------------------------------------------------
