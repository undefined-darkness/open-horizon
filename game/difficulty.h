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
    float ammo_ammount;
    float player_missile_dmg;
    float enemy_missile_dmg;
    float missile_dmg_radius;
    float bomb_dmg_radius;
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
            d.player_missile_dmg = s.attribute("player_missile_dmg").as_float();
            d.enemy_missile_dmg = s.attribute("enemy_missile_dmg").as_float();
            d.missile_dmg_radius = s.attribute("missile_dmg_radius").as_float();
            d.bomb_dmg_radius = s.attribute("bomb_dmg_radius").as_float();

            list.push_back(d);
        }
    }

    return list;
}

}
