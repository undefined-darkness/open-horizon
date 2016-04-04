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

    const float kdt = dt * 0.001f;

    auto p = m_plane.lock();

    vec3 next_pos = p->phys->pos + p->phys->vel * kdt;

    p->controls.mgun = false;
    p->controls.brake = 0.0f;

    if (m_state == state_pursuit)
    {
        if ((m_target.expired() || m_target.lock()->hp <= 0) && m_state != state_follow)
            m_state = state_wander;
    }

    //if (m_state != state_pursuit)
        find_best_target();

    if (!m_target.expired())
    {
        p->select_target(std::static_pointer_cast<object>(m_target.lock()));

        auto t = m_target.lock();
        auto target_pos = t->phys->pos + t->phys->vel * (dt * 0.001f);

        if (m_state == state_pursuit)
            go_to(target_pos - 20.0 * vec3::normalize(t->phys->vel), dt); //6 o'clock
        else
            go_to(target_pos, dt);
    }

    if (m_state == state_wander)
    {
        if (fabsf(p->get_pos().x) > 4096 || fabsf(p->get_pos().z) > 4096 ) //ToDo: set operation area
        {
            auto dir = p->get_dir();
            dir.y = 0;
            dir.normalize();

            if (stabilize_course(dir))
                p->controls.rot = nya_math::vec3(0.0, 1.0, 0.0);

            p->controls.throttle = 1.0;
        }
    }

    //evade ground
    const float minimal_height = 600.0f + w.get_height(next_pos.x, next_pos.z);
    if (next_pos.y < minimal_height && p->get_dir().y < 0.0f)
    {
        if (p->phys->get_speed_kmh() > 700.0f)
            p->controls.brake = 1.0f;

        p->controls.rot.x = vec3::up().dot(p->phys->rot.rotate(vec3::up())) < 0.0f ? 1.0f : -1.0f;
    }

    if (!p->targets.empty() && p->targets.front().locked)
    {
        p->controls.missile = !p->controls.missile;
        p->controls.mgun = true; //ToDo
    }
    else
        p->controls.missile = false;
}

//------------------------------------------------------------

bool ai::stabilize_course(const vec3 &dir)
{
    //ToDo
    return true;
}

//------------------------------------------------------------
/*
inline float get_yaw(const vec3 &v)
{
    const float xz_sqdist=v.x*v.x+v.z*v.z;
    return atan2(v.x,v.z);
}
*/
//------------------------------------------------------------

void ai::go_to(const vec3 &pos, int dt)
{
    if (m_plane.expired())
        return;

    const float kdt = dt * 0.001f;

    auto p = m_plane.lock();

    vec3 next_pos = p->phys->pos + p->phys->vel * kdt;

    vec3 target_diff = pos - next_pos;
    vec3 target_dir = vec3::normalize(target_diff);

    auto dir = p->get_dir();

    //pitch
    if (target_dir.y > 0.01f && dir.y < 0.6)
        p->controls.rot.x = -1.0f;
    else if (target_dir.y < -0.01f && dir.y > -0.6)
        p->controls.rot.x = 1.0f;
    else
        p->controls.rot.x = 0.0f;

    auto dir_normal = vec3::cross(p->get_dir(), target_dir);

    //yaw
    if (dir_normal.y > 0.01f)
        p->controls.rot.y = -1.0f;
    else if (dir_normal.y < -0.01f)
        p->controls.rot.y = 1.0f;

    //ToDo: roll
}

//------------------------------------------------------------

void ai::find_best_target()
{
    if (m_plane.expired())
        return;

    auto p = m_plane.lock();
    auto dir = p->get_dir();

    if (m_state != state_follow)
        m_state = state_wander;
    m_target.reset();

    float best_dir = 1000000000.0f;
    for (int i = 0; i < 2; ++i) //at first, front hemisphere, then, any target
    {
        if (i > 0 && !m_target.expired())
            return;

        for (auto &t: p->targets)
        {
            if (t.locked)
            {
                m_target = t.target_plane;
                return;
            }

            if (t.target_plane.expired())
                continue;

            auto tp = t.target_plane.lock();
            if (tp->hp <= 0)
                continue;

            auto tdir = tp->phys->pos - p->phys->pos;
            const bool in_front = tdir.dot(dir) < 0.0;
            if ((i == 0) == in_front)
                return;

            auto d = tdir.length();
            if (d > best_dir)
                continue;

            best_dir = d;
            m_target = t.target_plane;
            if (m_state == state_follow)
                continue;

            m_state = state_approach;
            if (i == 0 && p->phys->vel.dot(tp->phys->vel) >=0)
                m_state = state_pursuit;
        }
    }
}

//------------------------------------------------------------

void ai::set_follow(plane_ptr &p, const vec3 &formation_offset)
{
    //ToDo
}

//------------------------------------------------------------
}
