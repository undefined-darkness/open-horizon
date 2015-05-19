//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class ai
{
public:
    void set_plane(plane_ptr &p) { m_plane = p; }
    void update(const world &w, int dt);
    void reset_state() { m_state = state_wander; }

    ai() { reset_state(); }

private:
    bool stabilize_course(const vec3 &dir);
    void find_best_target();
    void go_to(const vec3 &pos, int dt);

private:
    w_ptr<plane> m_plane;
    w_ptr<plane> m_target;

    enum state
    {
        state_wander,
        state_approach,
        state_pursuit,
        state_follow,
    };

    state m_state;
};

//------------------------------------------------------------
}
