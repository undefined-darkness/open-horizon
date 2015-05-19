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
    bool stabilize_course(vec3 dir);
    void select_best_target();

private:
    w_ptr<plane> m_plane;
    w_ptr<plane> m_target;

    enum state
    {
        state_wander,
        state_approach,
        state_pursuit,
        state_follow,
        state_evade
    };

    state m_state;
};

//------------------------------------------------------------
}
