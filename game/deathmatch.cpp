//
// open horizon -- undefined_darkness@outlook.com
//

#include "deathmatch.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

void deathmatch::start(const char *plane, int color, int special, const char *location, int players_count)
{
    m_world.set_location(location);

    world::is_ally_handler fia;
    m_world.set_ally_handler(fia);

    world::on_kill_handler fok = std::bind(&deathmatch::on_kill, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_on_kill_handler(fok);

    m_respawn_points.resize(players_count);
    for (int i = 0; i < (int)m_respawn_points.size(); ++i)
    {
        auto &p = m_respawn_points[i];

        nya_math::quat r(0.0, 2.0 * nya_math::constants::pi / m_respawn_points.size() * i, 0.0);

        const int game_area_size = 8096;

        p.first = -r.rotate(nya_math::vec3(0.0, 0.0, game_area_size/2));
        p.first.y = m_world.get_height(p.first.x, p.first.z) + 400.0f;
        p.second = r;
    }

    const auto planes = get_aircraft_ids({"fighter", "multirole"});
    size_t plane_idx = 0;
    assert(!planes.empty());

    m_bots.clear();

    for (int i = 0; i < players_count; ++i)
    {
        const bool is_player = i == 0;

        plane_ptr p;
        if (is_player)
        {
            p = m_world.add_plane(plane, m_world.get_player_name(), color, is_player);
        }
        else
        {
            ai b;
            const char *plane_name = planes[plane_idx = (plane_idx + 1) % planes.size()].c_str(); //ToDo
            p = m_world.add_plane(plane_name, "BOT", 0, is_player);
            b.set_plane(p);
            m_bots.push_back(b);
        }

        m_planes[p] = {};

        auto rp = get_respawn_point();
        p->set_pos(rp.first);
        p->set_rot(rp.second);
    }
}

//------------------------------------------------------------

void deathmatch::update(int dt, const plane_controls &player_controls)
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
                    auto rp = get_respawn_point();
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

void deathmatch::end()
{
    m_planes.clear();
}

//------------------------------------------------------------

deathmatch::respawn_point deathmatch::get_respawn_point()
{
    if (m_respawn_points.empty())
        return respawn_point();

    return m_respawn_points[m_last_respawn = (m_last_respawn + 1) % m_respawn_points.size()];
}

//------------------------------------------------------------

void deathmatch::on_kill(const plane_ptr &k, const plane_ptr &v)
{
    if (!v)
        return;

    if (k)
    {
        ++m_planes[k].score;
    }
    else
        --m_planes[v].score;

    std::vector<score_e> score_table;
    for (auto &p: m_planes)
        score_table.push_back({p.second.score, p.first});
    update_score_table(score_table);
}

//------------------------------------------------------------

void deathmatch::update_score_table(std::vector<score_e> &score_table)
{
    std::sort(score_table.rbegin(), score_table.rend());
    size_t pidx = std::find_if(score_table.begin(), score_table.end(), [this](score_e &p){ return p.second == m_world.get_player(); }) - score_table.begin();

    auto &h = m_world.get_hud();

    const int score_table_size = 4;

    const int start_idx = pidx > 0 ? (int)pidx - 1 : 0;
    for (int i = start_idx; i < start_idx + score_table_size; ++i)
    {
        if (score_table.size() > i)
            h.set_score(i - start_idx, i + 1, score_table[i].second->player_name, std::to_wstring(score_table[i].first));
        else
            h.remove_score(i);
    }
}

//------------------------------------------------------------
}
