//
// open horizon -- undefined_darkness@outlook.com
//

#include "free_flight.h"

namespace game
{
//------------------------------------------------------------

void free_flight::start(const char *plane, int color, const char *location)
{
    set_plane(plane, color);
    set_location(location);
    m_respawn_time = 0;

    world::is_ally_handler fia = std::bind(&free_flight::is_ally, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);

    m_spawn_pos.x = -300.0f, m_spawn_pos.z = 2000.0f;
    m_spawn_pos.y = m_world.get_height(m_spawn_pos.x, m_spawn_pos.z) + 100.0f;
}

//------------------------------------------------------------

void free_flight::update(int dt, const plane_controls &player_controls)
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
            auto respawn_time = p->local_game_data.get<int>("respawn_time");
            if (respawn_time > 0)
            {
                respawn_time -= dt;
                if (respawn_time <= 0)
                    m_world.respawn(p, m_spawn_pos, nya_math::quat());
            }
            else
                respawn_time = 2000;

            p->local_game_data.set("respawn_time", respawn_time);
        }
    }
}

//------------------------------------------------------------

void free_flight::end()
{
    m_player.reset();
}

//------------------------------------------------------------

void free_flight::set_location(const char *location)
{
    if (!location)
        return;

    m_world.set_location(location);
    if (!m_player)
        return;

    m_spawn_pos.x = -300.0f, m_spawn_pos.z = 2000.0f;
    m_spawn_pos.y = m_world.get_height(m_spawn_pos.x, m_spawn_pos.z) + 100.0f;

    m_player->set_pos(m_spawn_pos);
    m_player->set_rot(nya_math::quat());
}

//------------------------------------------------------------

void free_flight::set_plane(const char *plane, int color)
{
    if (!plane)
        return;

    nya_math::vec3 p;
    nya_math::quat r;

    if (m_player)
    {
        p = m_player->get_pos();
        r = m_player->get_rot();
    }

    m_player = m_world.add_plane(plane, m_world.get_player_name(), color, true);
    m_player->set_pos(p);
    m_player->set_rot(r);
}

//------------------------------------------------------------
}
