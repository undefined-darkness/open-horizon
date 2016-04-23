//
// open horizon -- undefined_darkness@outlook.com
//

#include "team_deathmatch.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

void team_deathmatch::start(const char *plane, int color, int special, const char *location, int players_count)
{
    m_world.set_location(location);

    world::is_ally_handler fia = std::bind(&team_deathmatch::is_ally, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);

    world::on_kill_handler fok = std::bind(&team_deathmatch::on_kill, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_on_kill_handler(fok);

    for (int t = 0; t < 2; ++t)
    {
        m_respawn_points[t].resize(players_count / 2);
        for (int i = 0; i < (int)m_respawn_points[t].size(); ++i)
        {
            auto &p = m_respawn_points[t][i];

            const int game_area_size = 8096;
            p.first.x = 0.25 * (-game_area_size/2 + (game_area_size / int(m_respawn_points[t].size())) * i);
            p.first.y = m_world.get_height(p.first.x, p.first.z) + 400.0f;
            p.first.z = game_area_size/2 * (t == team_ally ? 1 : -1);

            p.second = quat(0.0, t == team_ally ? nya_math::constants::pi : 0.0, 0.0);
        }
    }

    bool is_player = true;
    bool easter_edge = true;

    size_t plane_idx = 0;
    const auto planes = get_aircraft_ids({"fighter", "multirole"});
    assert(!planes.empty());

    m_bots.clear();
    for (int t = 0; t < 2; ++t)
    {
        for (int i = 0; i < players_count / 2; ++i)
        {
            plane_ptr p;
            if (is_player)
            {
                p = m_world.add_plane(plane, m_world.get_player_name(), color, is_player);
                is_player = false;
            }
            else
            {
                ai b;
                if (easter_edge)
                {
                    p = m_world.add_plane("f14d", "BOT", 3, is_player);
                    b.set_follow(m_world.get_player(), vec3(10.0, 0.0, -10.0));
                    easter_edge = false;
                }
                else
                {
                    const char *plane_name = planes[plane_idx = (plane_idx + 1) % planes.size()].c_str(); //ToDo
                    p = m_world.add_plane(plane_name, "BOT", 0, is_player);
                }

                b.set_plane(p);
                m_bots.push_back(b);
            }

            tdm_plane tp;
            tp.t = team(t);
            m_planes[p] = tp;

            auto rp = get_respawn_point(team(t));
            p->set_pos(rp.first);
            p->set_rot(rp.second);
        }
    }

    m_world.get_hud().set_team_score(0, 0);
}

//------------------------------------------------------------

void team_deathmatch::update(int dt, const plane_controls &player_controls)
{
    if (m_world.get_player()->hp > 0)
        m_world.get_player()->controls = player_controls;

    for (auto &b: m_bots)
        b.update(m_world, dt);

    m_world.update(dt);

    for (auto &p: m_planes)
    {
        if (p.first->hp <= 0)
        {
            if (p.second.respawn_time > 0)
            {
                p.second.respawn_time -= dt;
                if (p.second.respawn_time <= 0)
                {
                    auto rp = get_respawn_point(p.second.t);
                    p.first->set_pos(rp.first);
                    p.first->set_rot(rp.second);
                    p.first->reset_state();
                }
            }
            else
                p.second.respawn_time = 2000;
        }
    }
}

//------------------------------------------------------------

void team_deathmatch::end()
{
    m_planes.clear();
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

void team_deathmatch::on_kill(const plane_ptr &k, const plane_ptr &v)
{
    if (!v)
        return;

    const bool ally = is_ally(m_world.get_player(), v);

    if (k)
    {
        if (!is_ally(k, v))
        {
            ++m_score[ally ? team_enemies : team_ally];
            ++m_planes[k].score;
        }
    }
    else
    {
        --m_score[ally ? team_ally : team_enemies];
        --m_planes[v].score;
    }

    m_world.get_hud().set_team_score(m_score[team_ally], m_score[team_enemies]);

    std::vector<score_e> score_table;
    for (auto &p: m_planes)
        score_table.push_back({p.second.score, p.first});
    update_score_table(score_table);
}

//------------------------------------------------------------
}
