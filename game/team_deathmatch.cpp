//
// open horizon -- undefined_darkness@outlook.com
//

#include "team_deathmatch.h"

namespace game
{
//------------------------------------------------------------

void team_deathmatch::start(const char *plane, int color, int special, const char *location, int players_count)
{
    m_world.set_location(location);

    world::is_ally_handler fia = std::bind(&team_deathmatch::is_ally, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);

    for (int t = 0; t < 2; ++t)
    {
        m_respawn_points[t].resize(players_count / 2);
        for (int i = 0; i < (int)m_respawn_points[t].size(); ++i)
        {
            auto &p = m_respawn_points[t][i];

            const int game_area_size = 8096;
            p.first.x = 0.25 * (-game_area_size/2 + (game_area_size / int(m_respawn_points[t].size())) * i);
            p.first.y = 400;
            p.first.z = game_area_size/2 * (t == team_ally ? 1 : -1);

            p.second = quat(0.0, t == team_ally ? nya_math::constants::pi : 0.0, 0.0);
        }
    }

    bool is_player = true;

    for (int t = 0; t < 2; ++t)
    {
        for (int i = 0; i < players_count / 2; ++i)
        {
            plane_ptr p = m_world.add_plane(plane, color, is_player);
            if (is_player)
                m_player = p;

            m_planes[p] = {team(t)};
            is_player = false;

            auto rp = get_respawn_point(team(t));
            p->set_pos(rp.first);
            p->set_rot(rp.second);
        }
    }
}

//------------------------------------------------------------

void team_deathmatch::update(int dt, const plane_controls &player_controls)
{
    if (m_player.is_valid())
        m_player->controls = player_controls;

    m_world.update(dt);
}

//------------------------------------------------------------

void team_deathmatch::end()
{
    m_planes.clear();
    m_player.free();
}

//------------------------------------------------------------

team_deathmatch::respawn_point team_deathmatch::get_respawn_point(team t)
{
    if (m_respawn_points[t].empty())
        return respawn_point();

    return m_respawn_points[t][m_last_respawn[t] = (m_last_respawn[t] + 1) % m_respawn_points[t].size()];
}

//------------------------------------------------------------

bool team_deathmatch::is_ally(const plane_ptr &a, const plane_ptr &b)
{
    return m_planes[a].t == m_planes[b].t;
}

//------------------------------------------------------------
}
