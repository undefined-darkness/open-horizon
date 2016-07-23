//
// open horizon -- undefined_darkness@outlook.com
//

#include "deathmatch.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

void deathmatch::start(const char *plane, int color, int special, const char *location, int bots_count)
{
    m_world.set_location(location);

    world::is_ally_handler fia;
    m_world.set_ally_handler(fia);

    m_world.set_on_kill_handler(std::bind(&deathmatch::on_kill, this, std::placeholders::_1, std::placeholders::_2));

    m_respawn_points.resize(8);
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
    assert(!planes.empty());

    m_bots.clear();

    m_planes.push_back(m_world.add_plane(plane, m_world.get_player_name(), color, true)); //player

    for (int i = 0; i < bots_count; ++i)
    {
        plane_ptr p;

        ai b;

        const char *plane_name = planes[rand() % planes.size()].c_str(); //ToDo
        p = m_world.add_plane(plane_name, "BOT", 0, false);
        b.set_plane(p);
        m_bots.push_back(b);

        m_planes.push_back(p);
    }

    for (auto &p: m_planes)
    {
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

    if (m_world.net_data_updated())
        update_scores();

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
                    respawn(p);
            }
            else
                respawn_time = 2000;

            p->local_game_data.set("respawn_time", respawn_time);
        }
    }
}

//------------------------------------------------------------

void deathmatch::respawn(plane_ptr p)
{
    auto rp = get_respawn_point();
    m_world.respawn(p, rp.first, rp.second);
}

//------------------------------------------------------------

void deathmatch::end()
{
    m_planes.clear();
    m_world.set_on_kill_handler(0);
}

//------------------------------------------------------------

deathmatch::respawn_point deathmatch::get_respawn_point()
{
    if (m_respawn_points.empty())
        return respawn_point();

    return m_respawn_points[m_last_respawn = (m_last_respawn + 1) % m_respawn_points.size()];
}

//------------------------------------------------------------

void deathmatch::on_kill(const object_ptr &k, const object_ptr &v)
{
    auto vp = m_world.get_plane(v);
    if (!vp)
        return;

    auto kp = m_world.get_plane(k);
    if (kp)
        kp->net_game_data.set("score", kp->net_game_data.get<int>("score") + 1);
    else
        vp->net_game_data.set("score", vp->net_game_data.get<int>("score") - 1);
}

//------------------------------------------------------------

void deathmatch::update_scores()
{
    typedef std::pair<int, plane_ptr> score_e;
    std::vector<score_e> score_table(m_world.get_planes_count());

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto &s = score_table[i];
        s.second = m_world.get_plane(i);
        s.first = s.second->net_game_data.get<int>("score");
    }

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
