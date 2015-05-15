//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class team_deathmatch: public game_mode
{
public:
    virtual void start(const char *plane, int color, int special, const char *location, int players_count);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end();

    team_deathmatch(world &w): game_mode(w) {}

protected:
    enum team
    {
        team_ally,
        team_enemies
    };

    bool is_ally(const plane_ptr &a, const plane_ptr &b);

    typedef std::pair<vec3, quat> respawn_point;
    respawn_point get_respawn_point(team t);

    std::vector<respawn_point> m_respawn_points[2];
    ivalue m_last_respawn[2];

    struct tdm_plane
    {
        team t;
    };

    plane_ptr m_player;
    std::map<plane_ptr, tdm_plane> m_planes;
};

//------------------------------------------------------------
}
