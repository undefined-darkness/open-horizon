//
// open horizon -- undefined_darkness@outlook.com
//

#include "team_deathmatch.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

inline int get_team(std::string t) { return t == "red" ? 1 : 0; }
inline std::string get_team(int t) { return t == 1 ? "red" : "blue"; }

//------------------------------------------------------------

void team_deathmatch::start(const char *plane, int color, int special, const char *location, int bots_count)
{
    m_world.set_location(location);

    world::is_ally_handler fia = std::bind(&team_deathmatch::is_ally, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_ally_handler(fia);

    world::on_kill_handler fok = std::bind(&team_deathmatch::on_kill, this, std::placeholders::_1, std::placeholders::_2);
    m_world.set_on_kill_handler(fok);

    for (int t = 0; t < 2; ++t)
    {
        m_respawn_points[t].resize(5);
        for (int i = 0; i < (int)m_respawn_points[t].size(); ++i)
        {
            auto &p = m_respawn_points[t][i];

            const int game_area_size = 8096;
            p.first.x = 0.25 * (-game_area_size/2 + (game_area_size / int(m_respawn_points[t].size())) * i);
            p.first.y = m_world.get_height(p.first.x, p.first.z) + 400.0f;
            p.first.z = game_area_size/2 * (t == 0 ? 1 : -1);

            p.second = quat(0.0, t == 0 ? nya_math::constants::pi : 0.0, 0.0);
        }
    }

    size_t plane_idx = 0;
    const auto planes = get_aircraft_ids({"fighter", "multirole"});
    assert(!planes.empty());

    m_planes.push_back(m_world.add_plane(plane, m_world.get_player_name(), color, true));

    m_bots.clear();

    for (int i = 0; i < bots_count; ++i)
    {
        plane_ptr p;
        ai b;

        const char *plane_name = planes[plane_idx = (plane_idx + 1) % planes.size()].c_str(); //ToDo
        p = m_world.add_plane(plane_name, "BOT", 0, false);

        b.set_plane(p);
        m_bots.push_back(b);

        m_planes.push_back(p);
    }

    int last_team = 0;
    for (auto &p: m_planes)
    {
        p->net_game_data.set("team", get_team(last_team));
        last_team = !last_team;
        auto rp = get_respawn_point(p->net_game_data.get<std::string>("team"));
        p->set_pos(rp.first);
        p->set_rot(rp.second);
    }

    m_world.get_hud().set_team_score(0, 0);
}

//------------------------------------------------------------

void team_deathmatch::update(int dt, const plane_controls &player_controls)
{
    deathmatch::update(dt, player_controls);
    if (!m_world.is_host())
        return;

    if (m_last_planes_count != m_world.get_planes_count())
    {
        m_last_planes_count = m_world.get_planes_count();
        rebalance();
    }
    else if (m_world.net_data_updated())
        rebalance();
}

//------------------------------------------------------------

void team_deathmatch::respawn(plane_ptr p)
{
    auto rp = get_respawn_point(p->net_game_data.get<std::string>("team"));
    m_world.respawn(p, rp.first, rp.second);
}

//------------------------------------------------------------

void team_deathmatch::end()
{
    m_planes.clear();
}

//------------------------------------------------------------

void team_deathmatch::rebalance()
{
    int count[2] = {0};
    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        ++count[get_team(p->net_game_data.get<std::string>("team"))];
    }

    while (abs(count[1] - count[0]) > 1)
    {
        const int dec_team = count[1] > count[0] ? 1 : 0;
        const int inc_team = 1 - dec_team;
        for (int i = m_world.get_planes_count() - 1; i >= 0; --i)
        {
            auto p = m_world.get_plane(i);
            if (get_team(p->net_game_data.get<std::string>("team")) == dec_team)
            {
                --count[dec_team];
                ++count[inc_team];
                p->net_game_data.set("team", get_team(inc_team));
                respawn(p);
                break;
            }
        }
    }
}

//------------------------------------------------------------

team_deathmatch::respawn_point team_deathmatch::get_respawn_point(std::string team)
{
    const int t = get_team(team);

    if (m_respawn_points[t].empty())
        return respawn_point();

    return m_respawn_points[t][m_last_respawn[t] = (m_last_respawn[t] + 1) % m_respawn_points[t].size()];
}

//------------------------------------------------------------

bool team_deathmatch::is_ally(const plane_ptr &a, const plane_ptr &b)
{
    return a->net_game_data.get<std::string>("team") == b->net_game_data.get<std::string>("team");
}

//------------------------------------------------------------

void team_deathmatch::update_scores()
{
    deathmatch::update_scores();

    m_score[0] = m_score[1] = 0;

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        m_score[get_team(p->net_game_data.get<std::string>("team"))] += p->net_game_data.get<int>("score");
    }

    if (get_team(m_world.get_player()->net_game_data.get<std::string>("team")) == 1)
        std::swap(m_score[0], m_score[1]);

    m_world.get_hud().set_team_score(m_score[0], m_score[1]);
}

//------------------------------------------------------------

void team_deathmatch::on_kill(const plane_ptr &k, const plane_ptr &v)
{
    if (!v)
        return;

    if (k)
    {
        if (!is_ally(k, v))
            k->net_game_data.set("score", k->net_game_data.get<int>("score") + 1);
    }
    else
        v->net_game_data.set("score", v->net_game_data.get<int>("score") - 1);
}

//------------------------------------------------------------
}
