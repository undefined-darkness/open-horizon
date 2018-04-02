//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "util/xml.h"
#include <string>

namespace game
{

struct difficulty_settings
{
    std::string id, name;
    float ammo_ammount = 1.0f;
    float dmg_take = 1.0f;
    float dmg_inflict = 1.0f;
};

typedef std::vector<difficulty_settings> difficulty_list;

static const difficulty_list &get_difficulty_list()
{
    static difficulty_list list;
    if (list.empty())
    {
        pugi::xml_document doc;
        if (!load_xml("difficulty.xml", doc))
            return list;

        pugi::xml_node root = doc.child("difficulty");
        if (!root)
            return list;

        for (pugi::xml_node s = root.first_child(); s; s = s.next_sibling())
        {
            difficulty_settings d;
            d.id = s.name();
            d.name = s.attribute("name").as_string();
            d.ammo_ammount = s.attribute("ammo_ammount").as_float(1.0f);
            d.dmg_take = s.attribute("dmg_take").as_float();
            d.dmg_inflict = s.attribute("dmg_inflict").as_float();

            list.push_back(d);
        }
    }

    return list;
}

}
