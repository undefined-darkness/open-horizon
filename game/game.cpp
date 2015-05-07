//
// open horizon -- undefined_darkness@outlook.com
//

#include "game.h"
#include "math/scalar.h"
#include "util/util.h"
#include "util/xml.h"
#include <algorithm>
#include <time.h>

namespace game
{
//------------------------------------------------------------

class weapon_information
{
public:
    struct weapon
    {
        std::string id;
        std::string model;
        int count;
    };

    struct aircraft_weapons
    {
        weapon missile;
        std::vector<weapon> special;
        weapon flare;
    };

public:
    static weapon_information &get()
    {
        static weapon_information info("weapons.xml", "common/Arms/ArmsParam.txt");
        return info;
    }

    aircraft_weapons *get_aircraft_weapons(const char *name)
    {
        if (!name)
            return 0;

        std::string name_str(name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);
        auto a = m_aircrafts.find(name);
        if (a == m_aircrafts.end())
            return 0;

        return &a->second;
    }

private:
    weapon_information(const char *xml_name, const char *params)
    {
        pugi::xml_document doc;
        if (!load_xml(xml_name, doc))
            return;

        pugi::xml_node root = doc.child("weapons");
        if (!root)
            return;

        for (pugi::xml_node ac = root.child("aircraft"); ac; ac = ac.next_sibling("aircraft"))
        {
            aircraft_weapons &a = m_aircrafts[ac.attribute("name").as_string("")];

            for (pugi::xml_node wpn = ac.first_child(); wpn; wpn = wpn.next_sibling())
            {
                weapon w;
                w.id = wpn.attribute("id").as_string("");
                w.model = wpn.attribute("model").as_string("");
                w.count = wpn.attribute("count").as_int(0);

                std::string name(wpn.name() ? wpn.name() : "");
                if (name == "msl")
                    a.missile = w;
                else if (name == "spc")
                    a.special.push_back(w);
                else if (name == "flr")
                    a.flare = w;
            }
        }
    }

    std::map<std::string, aircraft_weapons> m_aircrafts;
};

//------------------------------------------------------------

plane_ptr world::add_plane(const char *name, int color, bool player)
{
    if (!name)
        return plane_ptr();

    plane_ptr p(true);
    p->phys = m_phys_world.add_plane(name);
    p->render = m_render_world.add_aircraft(name, color, player);

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    p->render->set_time(tm_now->tm_sec + tm_now->tm_min * 60 + tm_now->tm_hour * 60 * 60); //ToDo

    auto wi = weapon_information::get().get_aircraft_weapons(name);
    if (wi)
    {
        p->render->load_missile(wi->missile.model.c_str(), m_render_world.get_location_params());
        if (!wi->special.empty())
            p->render->load_special(wi->special[0].model.c_str(), m_render_world.get_location_params());
    }

    m_planes.push_back(p);
    return p;
}

//------------------------------------------------------------

void world::set_location(const char *name)
{
    m_render_world.set_location(name);
}

//------------------------------------------------------------

void world::update(int dt)
{
    m_planes.erase(std::remove_if(m_planes.begin(), m_planes.end(), [](plane_ptr &p){ return p.get_ref_count() <= 1; }), m_planes.end());

    for (auto &p: m_planes)
        p->phys->controls = p->controls;

    m_phys_world.update(dt, [](phys::object_ptr &a, phys::object_ptr &b) {});

    for (auto &p: m_planes)
        p->update(dt, p->render == m_render_world.get_player_aircraft());

    m_render_world.update(dt);
}

//------------------------------------------------------------

void plane::update(int dt, bool player)
{
    render->set_pos(phys->pos);
    render->set_rot(phys->rot);

    //aircraft animations

    const float speed = phys->vel.length();
    const float speed_k = nya_math::max((phys->params.move.speed.speedMax - speed) / phys->params.move.speed.speedMax, 0.1f);

    const float el = nya_math::clamp(-controls.rot.z - controls.rot.x, -1.0f, 1.0f) * speed_k;
    const float er = nya_math::clamp(controls.rot.z - controls.rot.x, -1.0f, 1.0f) * speed_k;
    render->set_elev(el, er);

    const float rl = nya_math::clamp(-controls.rot.y + controls.brake, -1.0f, 1.0f) * speed_k;
    const float rr = nya_math::clamp(-controls.rot.y - controls.brake, -1.0f, 1.0f) * speed_k;
    render->set_rudder(rl, rr, -controls.rot.y);

    render->set_aileron(-controls.rot.z * speed_k, controls.rot.z * speed_k);
    render->set_canard(controls.rot.x * speed_k);
    render->set_brake(controls.brake);
    render->set_flaperon(speed < phys->params.move.speed.speedCruising - 100 ? -1.0 : 1.0);
    render->set_wing_sweep(speed >  phys->params.move.speed.speedCruising + 250 ? 1.0 : -1.0);

    render->set_intake_ramp(phys->thrust_time >= phys->params.move.accel.thrustMinWait ? 1.0 : -1.0);

    //weapons

    if (controls.change_weapon && controls.change_weapon != last_controls.change_weapon)
    {
        if (!special_weapon && render->is_special_bay_closed())
            special_weapon = true;

        if (special_weapon && render->is_special_bay_opened())
            special_weapon = false;

        render->set_special_bay(special_weapon);
    }

    if (controls.missile && controls.missile != last_controls.missile)
    {
        if (!special_weapon)
        {
            rocket_bay_time = 3000;
            render->set_missile_bay(true);
        }
    }

    if (rocket_bay_time > 0)
    {
        rocket_bay_time -= dt;
        if (rocket_bay_time <= 0)
        {
            render->set_missile_bay(false);
            rocket_bay_time = 0;
        }
    }

    if (controls.mgun != last_controls.mgun)
        render->set_mgun_bay(controls.mgun);

    //cockpit animations and ui

    if (player)
    {
        render->set_speed(speed);

        if (controls.change_camera && controls.change_camera != last_controls.change_camera)
        {
            switch (render->get_camera_mode())
            {
                case renderer::aircraft::camera_mode_third: render->set_camera_mode(renderer::aircraft::camera_mode_cockpit); break;
                case renderer::aircraft::camera_mode_cockpit: render->set_camera_mode(renderer::aircraft::camera_mode_first); break;
                case renderer::aircraft::camera_mode_first: render->set_camera_mode(renderer::aircraft::camera_mode_third); break;
            }
        }
    }

    last_controls = controls;
}

//------------------------------------------------------------
}
