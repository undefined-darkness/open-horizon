//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "util/util.h"
#include "util/xml.h"
#include "extensions/zip_resources_provider.h"

namespace game
{
//------------------------------------------------------------

struct mission_info
{
    std::string id;
    std::string name;
    std::string description;
};

typedef std::vector<mission_info> missions_list;

static const missions_list &get_missions_list()
{
    static missions_list list;
    if (list.empty())
    {
        auto missions = list_files("missions/");
        for (auto &m: missions)
        {
            nya_resources::zip_resources_provider zprov;
            if (!zprov.open_archive(m.c_str()))
                continue;

            pugi::xml_document doc;
            if (!load_xml(zprov.access("info.xml"), doc))
                continue;

            mission_info mi;

            auto root = doc.first_child();
            mi.name = root.attribute("name").as_string();
            if (mi.name.empty())
                continue;

            mi.description = root.child("description").first_child().value();
            mi.id = m;
            list.push_back(mi);
        }
    }

    return list;
}

//------------------------------------------------------------
}
