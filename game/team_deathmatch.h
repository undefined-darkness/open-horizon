//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ai.h"

namespace game
{
//------------------------------------------------------------

class team_deathmatch: public game_mode
{
public:
    virtual void start(const char *plane, int color, int special, const char *location, int players_count);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end() override;

    team_deathmatch(world &w): game_mode(w) {}

protected:
    enum team
    {
        team_ally,
        team_enemies
    };

    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    void on_kill(const plane_ptr &k, const plane_ptr &v);

    typedef std::pair<vec3, quat> respawn_point;
    respawn_point get_respawn_point(team t);

    std::vector<respawn_point> m_respawn_points[2];
    ivalue m_last_respawn[2];

    struct tdm_plane
    {
        team t;
        ivalue score;
        ivalue respawn_time;
    };

    plane_ptr m_player;
    std::map<plane_ptr, tdm_plane> m_planes;
    std::vector<ai> m_bots;
    ivalue m_score[2];
};

//------------------------------------------------------------
}
