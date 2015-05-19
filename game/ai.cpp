//
// open horizon -- undefined_darkness@outlook.com
//

#include "ai.h"

namespace game
{
//------------------------------------------------------------

void ai::update(const world &w, int dt)
{
    if (m_plane.expired())
        return;

    auto p = m_plane.lock();

    if ((m_target.expired() || m_target.lock()->hp <= 0) && m_state != state_follow)
        m_state = state_wander;

    if (m_state == state_wander)
    {
        if (fabsf(p->get_pos().x) > 4096 || fabsf(p->get_pos().z) > 4096 ) //ToDo: set operation area
        {
            auto dir = p->get_dir();
            dir.y = 0;
            dir.normalize();

            if (stabilize_course(dir))
                p->controls.rot = nya_math::vec3(0.0, 1.0, 0.0);
        }
    }

    //ToDo

    p->controls.throttle = 1.0;

    select_best_target();

    if (!p->targets.empty() && p->targets.front().locked)
        p->controls.missile = !p->controls.missile;
    else
        p->controls.missile = false;
}

//------------------------------------------------------------

bool ai::stabilize_course(vec3 dir)
{
    //ToDo
    return true;
}

//------------------------------------------------------------

void ai::select_best_target()
{
    if (m_plane.expired())
        return;

    auto p = m_plane.lock();
    auto dir = p->get_dir();

    float best_dir = 1000000000.0f;
    for (int i = 0; i < 2; ++i)
    {
        for (auto &t: p->targets)
        {
            if (i == 0)
            {
                if (t.locked)
                {
                    m_target = t.plane;
                    return;
                }

                if (t.plane.expired())
                    continue;

                auto tdir = t.plane.lock()->phys->pos;
                const bool in_front = tdir * dir < 0.0;
                if ((i == 0) == in_front)
                    return;

                auto d = tdir.length();
                if (d > best_dir)
                    continue;

                best_dir = d;
                m_target = t.plane;
            }
        }
    }
}

//------------------------------------------------------------
}
