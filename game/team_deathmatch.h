//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "deathmatch.h"

namespace game
{
//------------------------------------------------------------

class team_deathmatch: public deathmatch
{
public:
    virtual void start(const char *plane, int color, int special, const char *location, int players_count);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end() override;

    team_deathmatch(world &w): deathmatch(w) {}

protected:
    enum team
    {
        team_ally,
        team_enemies
    };

    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    void on_kill(const plane_ptr &k, const plane_ptr &v);

    respawn_point get_respawn_point(team t);

    std::vector<respawn_point> m_respawn_points[2];
    ivalue m_last_respawn[2];

    struct tdm_plane: public dm_plane { team t; };
    std::map<plane_ptr, tdm_plane> m_planes;
    ivalue m_score[2];
};

//------------------------------------------------------------
}
