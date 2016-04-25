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
    virtual void start(const char *plane, int color, int special, const char *location, int bots_count);
    virtual void end() override;

    team_deathmatch(world &w): deathmatch(w) {}

protected:
    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    void on_kill(const plane_ptr &k, const plane_ptr &v);
    virtual void respawn(plane_ptr p);
    virtual void update_scores() override;

    respawn_point get_respawn_point(std::string team);

    std::vector<respawn_point> m_respawn_points[2];
    ivalue m_last_respawn[2];
    ivalue m_score[2];
};

//------------------------------------------------------------
}
