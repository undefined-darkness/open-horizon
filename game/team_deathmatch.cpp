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
                m_player = p = m_world.add_plane(plane, color, is_player);
                is_player = false;
            }
            else
            {
                ai b;
                if (easter_edge)
                {
                    p = m_world.add_plane("f14d", 3, is_player);
                    b.set_follow(m_player, vec3(10.0, 0.0, -10.0));
                    easter_edge = false;
                }
                else
                {
                    const char *plane_name = planes[plane_idx = (plane_idx + 1) % planes.size()].c_str(); //ToDo
                    p = m_world.add_plane(plane_name, 0, is_player);
                }

                b.set_plane(p);
                m_bots.push_back(b);
            }

            m_planes[p] = {team(t)};

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
    if (m_player && m_player->hp > 0)
        m_player->controls = player_controls;

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
    m_player.reset();
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

    const bool ally = is_ally(m_player, v);

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

    typedef std::pair<int, plane_ptr> score_e;
    std::vector<score_e> score_table;
    for (auto &p: m_planes)
        score_table.push_back({p.second.score, p.first});
    std::sort(score_table.rbegin(), score_table.rend());
    size_t pidx = std::find_if(score_table.begin(), score_table.end(), [this](score_e &p){ return p.second == m_player; }) - score_table.begin();

    auto &h = m_world.get_hud();
    if (pidx == 0)
    {
        h.set_score(0, 1, L"Player", std::to_wstring(score_table[0].first));

        if (score_table.size() > 1)
            h.set_score(1, 2, L"Bot", std::to_wstring(score_table[1].first));
        else
            h.remove_score(1);

        if (score_table.size() > 2)
            h.set_score(2, 3, L"Bot", std::to_wstring(score_table[2].first));
        else
            h.remove_score(2);
    }
    else
    {
        const int place = (int)pidx + 1;
        h.set_score(0, place - 1, L"Bot", std::to_wstring(score_table[pidx - 1].first));
        h.set_score(1, place, L"Player", std::to_wstring(score_table[pidx].first));

        if (score_table.size() > pidx + 1)
            h.set_score(2, place + 1, L"Bot", std::to_wstring(score_table[pidx + 1].first));
        else
            h.remove_score(2);
    }
}

//------------------------------------------------------------
}
