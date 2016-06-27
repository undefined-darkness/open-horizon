//
// open horizon -- undefined_darkness@outlook.com
//

#include "mission.h"
#include "extensions/zip_resources_provider.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

void mission::start(const char *plane, int color, const char *mission)
{
    nya_resources::zip_resources_provider zprov;
    if (!zprov.open_archive(mission))
        return;

    pugi::xml_document doc;
    if (!load_xml(zprov.access("objects.xml"), doc))
        return;

    auto root = doc.first_child();

    m_world.set_location(root.attribute("location").as_string());

    auto p = root.child("player");
    if (p)
    {
        m_player = m_world.add_plane(plane, m_world.get_player_name(), color, true);
        m_player->set_pos(read_vec3(p));
        m_player->set_rot(quat(0.0, angle_deg(p.attribute("yaw").as_float()), 0.0));
    }

    world::is_ally_handler fia = std::bind(&mission::is_ally, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);
}

//------------------------------------------------------------

void mission::update(int dt, const plane_controls &player_controls)
{
    if (!m_player)
        return;

    m_player->controls = player_controls;

    m_world.update(dt);

    if (!m_world.is_host())
        return;

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        if (p->hp <= 0)
        {
            //ToDo
        }
    }
}

//------------------------------------------------------------

void mission::end()
{
    m_player.reset();
}

//------------------------------------------------------------
}
