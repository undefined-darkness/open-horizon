//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "pugixml.hpp"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "math/vector.h"

//------------------------------------------------------------

inline bool load_xml(nya_resources::resource_data *res, pugi::xml_document &doc)
{
    if (!res)
        return false;

    nya_memory::tmp_buffer_scoped buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();

    return doc.load_buffer((const char *)buf.get_data(), buf.get_size());
}

//------------------------------------------------------------

inline bool load_xml(const char *name, pugi::xml_document &doc)
{
    if (!name)
        return false;

    return load_xml(nya_resources::get_resources_provider().access(name), doc);
}

//------------------------------------------------------------

inline nya_math::vec3 read_vec3(const pugi::xml_node &n)
{
    return nya_math::vec3(n.attribute("x").as_float(), n.attribute("y").as_float(), n.attribute("z").as_float());
}

//------------------------------------------------------------
