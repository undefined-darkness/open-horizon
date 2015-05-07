//
// open horizon -- undefined_darkness@outlook.com
//

#include "game.h"
#include "math/scalar.h"
#include "util/util.h"
#include <algorithm>
#include <time.h>

namespace game
{
//------------------------------------------------------------

plane_ptr world::add_plane(const char *name, int color, bool player)
{
    plane_ptr p(true);
    p->phys = m_phys_world.add_plane(name);
    p->render = m_render_world.add_aircraft(name, color, player);

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    p->render->set_time(tm_now->tm_sec + tm_now->tm_min * 60 + tm_now->tm_hour * 60 * 60); //ToDo

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
        update_plane(p);

    m_render_world.update(dt);
}

//------------------------------------------------------------

void world::update_plane(plane_ptr &p)
{
    p->render->set_pos(p->phys->pos);
    p->render->set_rot(p->phys->rot);

    //aircraft animations

    const float speed = p->phys->vel.length();
    const float speed_k = nya_math::max((p->phys->params.move.speed.speedMax - speed) / p->phys->params.move.speed.speedMax, 0.1f);

    const float el = nya_math::clamp(-p->controls.rot.z - p->controls.rot.x, -1.0f, 1.0f) * speed_k;
    const float er = nya_math::clamp(p->controls.rot.z - p->controls.rot.x, -1.0f, 1.0f) * speed_k;
    p->render->set_elev(el, er);

    const float rl = nya_math::clamp(-p->controls.rot.y + p->controls.brake, -1.0f, 1.0f) * speed_k;
    const float rr = nya_math::clamp(-p->controls.rot.y - p->controls.brake, -1.0f, 1.0f) * speed_k;
    p->render->set_rudder(rl, rr, -p->controls.rot.y);

    p->render->set_aileron(-p->controls.rot.z * speed_k, p->controls.rot.z * speed_k);
    p->render->set_canard(p->controls.rot.x * speed_k);
    p->render->set_brake(p->controls.brake);
    p->render->set_flaperon(speed < p->phys->params.move.speed.speedCruising - 100 ? -1.0 : 1.0);
    p->render->set_wing_sweep(speed >  p->phys->params.move.speed.speedCruising + 250);

    p->render->set_intake_ramp(p->phys->thrust_time >= p->phys->params.move.accel.thrustMinWait ? 1.0 : -1.0);

    //cockpit animations and ui

    if (p->render == m_render_world.get_player_aircraft())
    {
        p->render->set_speed(speed);

        if (p->controls.change_camera && p->controls.change_camera != p->last_controls.change_camera)
        {
            switch (p->render->get_camera_mode())
            {
                case renderer::aircraft::camera_mode_third: p->render->set_camera_mode(renderer::aircraft::camera_mode_cockpit); break;
                case renderer::aircraft::camera_mode_cockpit: p->render->set_camera_mode(renderer::aircraft::camera_mode_first); break;
                case renderer::aircraft::camera_mode_first: p->render->set_camera_mode(renderer::aircraft::camera_mode_third); break;
            }
        }

        //ToDo
    }

    p->last_controls = p->controls;
}

//------------------------------------------------------------
}
