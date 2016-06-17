//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <string>
#include <vector>
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

struct object_desc
{
    std::string id;
    std::string type;
    std::string group;
    std::string model;
    float y = 0.0f, dy = 0.0f;
};

typedef std::vector<object_desc> objects_list;

static const objects_list &get_objects_list()
{
    static objects_list list;
    if (list.empty())
    {
        pugi::xml_document doc;
        if (!load_xml("objects.xml", doc))
            return list;

        pugi::xml_node root = doc.first_child();
        for (pugi::xml_node g = root.child("group"); g; g = g.next_sibling("group"))
        {
            std::string group = g.attribute("name").as_string();
            for (pugi::xml_node o = g.child("object"); o; o = o.next_sibling("object"))
            {
                object_desc obj;
                obj.id = o.attribute("id").as_string();
                obj.type = o.attribute("type").as_string();
                obj.group = group;
                obj.model = o.attribute("model").as_string();
                obj.y = o.attribute("y").as_float(0.0f);
                obj.dy = o.attribute("dy").as_float(0.0f);
                list.push_back(obj);
            }
        }
    }

    return list;
}

//------------------------------------------------------------
}
