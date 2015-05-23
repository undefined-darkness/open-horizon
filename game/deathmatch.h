//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"
#include "ai.h"

namespace game
{
//------------------------------------------------------------

class deathmatch: public game_mode
{
public:
    virtual void start(const char *plane, int color, int special, const char *location, int players_count);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end();

    deathmatch(world &w): game_mode(w) {}

private:
    typedef std::pair<vec3, quat> respawn_point;
    respawn_point get_respawn_point();

    std::vector<respawn_point> m_respawn_points;
    ivalue m_last_respawn;

    struct dm_plane
    {
        ivalue respawn_time;
    };

    plane_ptr m_player;
    std::map<plane_ptr, dm_plane> m_planes;
    std::vector<ai> m_bots;

};

//------------------------------------------------------------
}
