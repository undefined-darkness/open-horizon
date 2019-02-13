//
// open horizon -- undefined_darkness@outlook.com
//

#include "game.h"
#include "weapon_information.h"
#include "world.h"
#include "math/scalar.h"
#include "util/util.h"
#include "util/config.h"

namespace game
{
//------------------------------------------------------------

static const float kmph_to_meps = 1.0 / 3.6f;

//------------------------------------------------------------

wpn_params::wpn_params(std::string id, std::string model)
{
    const std::string action = "." + id + ".action.";
    const std::string lockon = "." + id + ".lockon.";
    const std::string lockon_dfm = "." + id + ".lockon_dfm.";

    this->id = id;
    this->model = model;

    auto &param = get_arms_param();

    lockon_range = param.get_float(lockon + "range");
    lockon_time = param.get_float(lockon_dfm + "timeLockon") * 1000;
    lockon_count = param.get_float(lockon + "lockonNum");
    lockon_air = param.get_int(lockon + "target_air") > 0;
    lockon_ground = param.get_int(lockon + "target_grd") > 0;
    action_range = param.get_float(action + "range");
    reload_time = param.get_float(lockon + "reload") * 1000;

    speed_init = param.get_float("." + id + ".action.speedInit") * kmph_to_meps;
    gravity = param.get_float("." + id + ".action.gravity");

    float lon_angle = param.get_float(lockon + "angle") * 0.5f * nya_math::constants::pi / 180.0f;
    //I dunno
    if (id == "SAAM")
        lon_angle *= 0.3f;
    lockon_angle_cos = cosf(lon_angle);
}

//------------------------------------------------------------

std::vector<std::string> get_aircraft_ids(const std::vector<std::string> &roles)
{
    std::vector<std::string> planes;
    for (auto &r: roles)
    {
        auto &l = weapon_information::get().get_aircraft_ids(r);
        planes.insert(planes.end(), l.begin(), l.end());
    }

    return planes;
}

//------------------------------------------------------------

const std::wstring &get_aircraft_name(const std::string &id)
{
    auto *a = weapon_information::get().get_aircraft_weapons(id.c_str());
    if(!a)
    {
        static std::wstring invalid;
        return invalid;
    }

    return a->name;
}

//------------------------------------------------------------

void missile::update_homing(int dt, world &w)
{
    if (net && !net->source)
    {
        auto p = w.get_plane(target);
        if (p)
            p->alert_dirs.push_back(p->get_pos() - phys->pos);
        return;
    }

    if (target.expired())
        return;

    const vec3 dir = phys->rot.rotate(vec3::forward());
    auto t = target.lock();
    auto diff = t->get_pos() - phys->pos + (t->get_vel() - phys->vel) * (dt * 0.001f);

    switch (mode)
    {
        case mode_missile: break;

        case mode_saam:
        {
            if (owner.expired() || owner.lock()->hp <= 0)
            {
                target.reset();
                return;
            }
        }
        break;

        case mode_4agm:
        {
            phys->target_dir = diff;
            if (nya_math::vec2(diff.x, diff.z).length() > 50.0f)
                diff.y = phys->rot.rotate(vec3::forward()).y;

            phys->target_dir = diff.normalize();
            return;
        }
        break;

        case mode_lagm:
        {
            if (phys->pos.y - w.get_height(phys->pos.x, phys->pos.z) > 50.0f)
                diff = diff.normalize() - vec3::up();
            else if (diff.length() > 500.0)
                diff.y = 0;

            phys->target_dir = diff.normalize();
            return;
        }
        break;
    }

    const vec3 target_dir = diff.normalize();
    if (dir.dot(target_dir) > homing_angle_cos || !t->is_targetable(true, true))
    {
        phys->target_dir = target_dir;
        auto p = w.get_plane(target);
        if (p)
            p->alert_dirs.push_back(diff);
    }
    else
    {
        target.reset();
        if (mode == mode_saam && owner.lock() == w.get_player())
            w.popup_miss();
    }
}

//------------------------------------------------------------

void missile::update(int dt, world &w)
{
    render->mdl.set_pos(phys->pos);
    render->mdl.set_rot(phys->rot);

    render->engine_started = phys->accel_started;

    if (time > 0)
        time -= dt;

    if (net && !net->source)
        return;

    if (!target.expired())
    {
        auto t = target.lock();
        bool hit = line_sphere_intersect(phys->pos, phys->pos + phys->vel * (dt * 0.001f), t->get_pos(), t->get_hit_radius());

        if (!hit)
        {
            auto dir = t->get_pos() - phys->pos;
            hit = dir.length() < 5.0; //proximity detonation
        }

        if(hit)
        {
            time = 0;
            //if (vec3::normalize(target.lock()->phys->vel) * dir.normalize() < -0.5)  //direct shoot
            //    missile_damage *= 3;

            w.spawn_explosion(phys->pos, dmg_radius * 0.5f);

            const bool target_alive = t->hp > 0;
            const bool hit = w.area_damage(phys->pos, dmg_radius, dmg, owner.lock());
            if (!hit && target_alive)
                w.direct_damage(t, dmg, owner.lock());

            const bool destroyed = t->hp <= 0;
            if (owner.lock() == w.get_player() && target_alive)
                w.popup_hit(destroyed);
        }

        if (t->hp < 0)
            target.reset();
    }
}

//------------------------------------------------------------

void bomb::update(int dt, world &w)
{
    render->mdl.set_pos(phys->pos);
    render->mdl.set_rot(phys->rot);
}

//------------------------------------------------------------
}

