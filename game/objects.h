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

struct object_params
{
    int hp = 0;
    float hit_radius = 0.0f;
    float speed_min = 0.0f, speed_cruise = 0.0f, speed_max = 0.0f, accel = 0.0f, decel = 0.0f;
    float turn_speed = 0.0f, turn_roll = 0.0f;
};

//------------------------------------------------------------

struct object_desc
{
    std::string id;
    std::string type;
    std::string group;
    std::string model;
    float y = 0.0f, dy = 0.0f;

    object_params params;
};

typedef std::vector<object_desc> objects_list;

//------------------------------------------------------------

static const objects_list &get_objects_list()
{
    static objects_list list;
    if (list.empty())
    {
        pugi::xml_document doc;
        if (!load_xml("objects.xml", doc))
            return list;

        pugi::xml_node root = doc.first_child();

        std::map<std::string, object_params> types;
        for (pugi::xml_node t = root.child("type"); t; t = t.next_sibling("type"))
        {
            auto &tp = types[t.attribute("id").as_string()];
            tp.hp = t.attribute("hp").as_int();
            tp.hit_radius = t.attribute("hit_radius").as_float();
            auto s = t.child("speed");
            if (s)
            {
                static const float kmph_to_meps = 1.0 / 3.6f;
                tp.speed_min = s.attribute("min").as_float() * kmph_to_meps;
                tp.speed_cruise = s.attribute("cruise").as_float() * kmph_to_meps;
                tp.speed_max = s.attribute("max").as_float() * kmph_to_meps;
                tp.accel = s.attribute("accel").as_float() * kmph_to_meps;
                tp.decel = s.attribute("decel").as_float() * kmph_to_meps;
            }

            auto tt = t.child("turn");
            if (tt)
            {
                tp.turn_speed = tt.attribute("speed").as_float();
                tp.turn_roll = tt.attribute("roll").as_float();
            }
        }

        for (pugi::xml_node g = root.child("group"); g; g = g.next_sibling("group"))
        {
            std::string group = g.attribute("name").as_string();
            for (pugi::xml_node o = g.child("object"); o; o = o.next_sibling("object"))
            {
                object_desc obj;
                obj.id = o.attribute("id").as_string();
                obj.type = o.attribute("type").as_string();
                obj.params = types[obj.type];
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
