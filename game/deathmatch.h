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
    virtual void end() override;

    deathmatch(world &w): game_mode(w) {}

protected:
    void on_kill(const plane_ptr &k, const plane_ptr &v);
    virtual void update_scores();
    virtual void respawn(plane_ptr p);

    typedef std::pair<vec3, quat> respawn_point;
    respawn_point get_respawn_point();

    std::vector<respawn_point> m_respawn_points;
    ivalue m_last_respawn;

    std::vector<plane_ptr> m_planes;
    std::vector<ai> m_bots;
};

//------------------------------------------------------------
}
