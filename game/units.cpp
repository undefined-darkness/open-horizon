//
// open horizon -- undefined_darkness@outlook.com
//

#include "units.h"
#include "world.h"

namespace game
{
//------------------------------------------------------------

static const float kmph_to_meps = 1.0 / 3.6f;

//------------------------------------------------------------

bool unit::is_ally(const unit_ptr &u) const
{
    if (!u)
        return false;

    if (get_align() == align_neutral || u->get_align() == align_neutral)
        return true;

    if (u->get_align() == get_align())
        return true;

    if (u->get_align() == align_ally || get_align() == align_ally)
        return false;

    return true;
}

//------------------------------------------------------------

void unit::take_damage(int damage, world &w, bool net_src)
{
    object::take_damage(damage, w, net_src);
    if (hp <= 0 && m_render.is_valid() && m_render->visible)
    {
        w.spawn_explosion(get_pos(), 30.0f);
        m_render->visible = false;
    }
}

//------------------------------------------------------------

void unit_vehicle::set_speed(float speed)
{
    m_vel = m_vel.normalize() * nya_math::clamp(speed * kmph_to_meps, m_params.speed_min, std::min(m_params.speed_max, m_speed_limit));
}

//------------------------------------------------------------

void unit_vehicle::set_speed_limit(float speed)
{
    m_speed_limit = speed * kmph_to_meps;
    set_speed(std::min(m_vel.length(), m_speed_limit));
}

//------------------------------------------------------------

inline float tend(float value, float target, float accel, float deccel)
{
    const float diff = target - value;
    if (diff > accel) return value + accel;
    if (-diff > deccel) return value - deccel;
    return target;
}

//------------------------------------------------------------

void unit_vehicle::update(int dt, world &w)
{
    if (hp <= 0)
        return;

    if (m_first_update)
    {
        m_first_update = false;

        if (!m_follow.expired())
        {
            auto f = m_follow.lock();
            vec3 diff = get_pos() - f->get_pos();
            if (diff.length() < m_params.formation_radius)
                m_formation_offset = f->get_rot().rotate_inv(diff);
            else
                m_formation_offset.set(0, 0, -15.0f);
        }

        m_ground = (get_pos().y - w.get_height(get_pos().x, get_pos().z)) < 5.0f;
        if (m_ground)
        {
            //ToDo: gear anim
        }
        else
        {
            if (m_params.speed_min > 0.01f)
                m_vel = get_rot().rotate(vec3::forward() * m_params.speed_cruise);
            m_dpos = vec3();
        }

        return;
    }

    float kdt = dt * 0.001f;

    vec3 target_dir;
    bool has_target = false;

    if (m_target.expired())
    {
        if (m_ai > ai_default && m_target_search != search_none)
        {
            const bool air = m_ai == ai_air_to_air || m_ai == ai_air_multirole,
            ground = m_ai == ai_air_to_ground || m_ai == ai_air_multirole;

            float min_dist = 10000.0f;
            for (int i = 0; i < w.get_planes_count() + w.get_units_count(); ++i)
            {
                object_ptr t;
                if (i < w.get_planes_count())
                {
                    auto p = w.get_plane(i);
                    if (is_ally(p, w))
                        continue;

                    t = p;
                }
                else
                {
                    if (m_target_search == search_player)
                        continue;

                    auto u = w.get_unit(i - w.get_planes_count());
                    if (is_ally(u))
                        continue;

                    t = u;
                }

                if (!t->is_targetable(air, ground))
                    continue;

                const float dist = (t->get_pos() - get_pos()).length();
                if(dist >= min_dist)
                    continue;

                m_target = t;
                min_dist = dist;
            }
        }
    }
    else
    {
        if (m_target.lock()->hp <= 0)
            m_target.reset();
    }

    bool not_path_not_follow = false;

    if (!m_path.empty())
    {
        has_target = true;
        target_dir = m_path.front() - get_pos();
        if (target_dir.length() < m_vel.length() * kdt * m_params.target_radius)
        {
            if (m_path_loop)
                m_path.push_back(m_path.front());
            m_path.pop_front();
            if (m_path.empty())
                has_target = false;
            else
                target_dir = m_path.front() - get_pos();
        }
    }
    else if (!m_follow.expired())
    {
        has_target = true;
        auto f = m_follow.lock();
        target_dir = f->get_pos() + f->get_rot().rotate(m_formation_offset) - (get_pos() + get_vel() * kdt);
        //+ f->get_vel() * kdt //player's plane is already updated, ToDo
    }
    else
        not_path_not_follow = true;

    if (!m_target.expired())
    {
        auto t = m_target.lock();
        const auto dir = t->get_pos() + t->get_rot().rotate(vec3(0, 0, -20.0)) - (get_pos() + get_vel() * kdt);

        if (not_path_not_follow)
        {
            target_dir = dir;
            has_target = true;
        }

        for (auto &wp: m_weapons)
        {
            if (wp.cooldown > 0)
            {
                wp.cooldown -= dt;
                continue;
            }

            if (dir.length() > wp.params.lockon_range)
                continue;

            wp.cooldown = wp.params.reload_time;
            auto m = w.add_missile(wp.params.id.c_str(), wp.mdl);
            if (!m)
                continue;

            m->phys->pos = get_pos();
            m->phys->rot = get_rot();
            m->phys->vel = get_vel();

            m->phys->target_dir = dir;
            m->target = m_target;
        }
    }

    if (has_target)
    {
        const float eps=1.0e-6f;
        const float target_dist = target_dir.length();
        const vec3 v=target_dir / target_dist;
        const float xz_sqdist=v.x*v.x+v.z*v.z;

        const auto rot = get_rot();
        const auto pyr = rot.get_euler();

        const nya_math::angle_rad new_yaw=(xz_sqdist>eps*eps)? (atan2(v.x,v.z)) : pyr.y;
        nya_math::angle_rad new_pitch=(fabsf(v.y)>eps)? (-atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;
        nya_math::angle_rad new_roll;
        if (!m_follow.expired())
            new_roll = m_follow.lock()->get_rot().get_euler().z;

        vec3 pos = get_pos() + m_vel * kdt;
        const float h = w.get_height(pos.x, pos.z);
        if ((new_pitch > 0.0f && pos.y <= h + m_params.height_min) || (new_pitch < 0.0f && pos.y >= h + m_params.height_max))
            new_pitch = 0.0f;

        const auto ideal_rot = quat(new_pitch, new_yaw, new_roll);

        const float angle_diff = rot.rotate(vec3::forward()).angle(target_dir).get_deg();
        if (fabsf(angle_diff) > 0.001f)
        {
            const float turn_k = nya_math::clamp((180.0f / angle_diff) * m_params.turn_speed * kdt, 0.0, 1.0);
            m_render->mdl.set_rot(quat::slerp(rot, ideal_rot, turn_k));
        }

        float speed = m_vel.length();
        float want_speed = target_dist / kdt;
        if (!m_follow.expired())
        {
            const float target_speed = m_follow.lock()->get_vel().length();
            float decel_time = (speed - target_speed) / m_params.decel;
            if (target_dist / (speed + m_params.accel * kdt) < decel_time)
                want_speed = target_speed;
        }

        want_speed = nya_math::clamp(want_speed, m_params.speed_min, std::min(m_params.speed_max, m_speed_limit));
        speed = tend(speed, want_speed, m_params.accel * kdt, m_params.decel * kdt);
        m_vel = get_rot().rotate(vec3(0.0, 0.0, speed));
    }
    else if (m_params.speed_min < 0.01f)
    {
        float speed = m_vel.length();
        if (speed > 0.0f)
        {
            speed = tend(speed, 0.0f, m_params.accel * kdt, m_params.decel * kdt);
            m_vel = get_rot().rotate(vec3(0.0, 0.0, speed));
        }
    }

    vec3 pos = get_pos() + m_vel * kdt;

    const float h = w.get_height(pos.x, pos.z);
    pos.y = nya_math::clamp(pos.y, h + m_params.height_min, h + m_params.height_max); //ToDo

    set_pos(pos);
}

//------------------------------------------------------------

unit_vehicle::unit_vehicle(const object_params &p, const location_params &lp): m_params(p)
{
    if (p.ai == "air_to_air")
        m_ai = ai_air_to_air;
    else if (p.ai == "air_to_ground")
        m_ai = ai_air_to_ground;
    else if (p.ai == "air_multirole")
        m_ai = ai_air_multirole;

    for (auto &d: p.weapons)
    {
        wpn w;
        w.params = wpn_params(d.id, d.model);
        w.mdl.load((std::string("w_") + d.model).c_str(), lp);
        m_weapons.push_back(w);
    }
}

//------------------------------------------------------------
}

