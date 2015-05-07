//
// open horizon -- undefined_darkness@outlook.com
//

#include "physics.h"
#include "math/scalar.h"
#include <algorithm>

namespace phys
{
//------------------------------------------------------------

inline float clamp(float value, float from, float to) { if (value < from) return from; if (value > to) return to; return value; }

//------------------------------------------------------------

inline float tend(float value, float target, float speed)
{
    const float diff = target - value;
    if (diff > speed) return value + speed;
    if (-diff > speed) return value - speed;
    return target;
}

//------------------------------------------------------------

plane_ptr world::add_plane(const char *name)
{
    plane_ptr p(true);
    p->params.load(("Player/Behavior/param_p_" + std::string(name) + ".bin").c_str());
    p->vel = nya_math::vec3(0.0, 0.0, p->params.move.speed.speedCruising);
    m_planes.push_back(p);
    return p;
}

//------------------------------------------------------------

void world::update(int dt, std::function<void(object_ptr &a, object_ptr &b)> on_hit)
{
    m_planes.erase(std::remove_if(m_planes.begin(), m_planes.end(), [](plane_ptr &p){ return p.get_ref_count() <= 1; }), m_planes.end());
    for (auto &p: m_planes) p->update(dt);
}

//------------------------------------------------------------

void plane::update(int dt)
{
    float kdt = dt * 0.001f;

    //simulation

    const float d2r = 3.1416 / 180.0f;

    float speed = vel.length();
    const float speed_arg = params.rotgraph.speed.get(speed);

    const nya_math::vec3 rspeed = params.rotgraph.speedRot.get(speed_arg);

    const float eps = 0.001f;

    if (fabsf(controls.rot.z) > eps)
        rot_speed.z = tend(rot_speed.z, controls.rot.z * rspeed.z, params.rot.addRotR.z * fabsf(controls.rot.z) * kdt * 100.0);
    else
        rot_speed.z = tend(rot_speed.z, 0.0f, params.rot.addRotR.z * kdt * 40.0);

    if (fabsf(controls.rot.y) > eps)
        rot_speed.y = tend(rot_speed.y, -controls.rot.y * rspeed.y, params.rot.addRotR.y * fabsf(controls.rot.y) * kdt * 50.0);
    else
        rot_speed.y = tend(rot_speed.y, 0.0f, params.rot.addRotR.y * kdt * 10.0);

    if (fabsf(controls.rot.x) > eps)
        rot_speed.x = tend(rot_speed.x, controls.rot.x * rspeed.x, params.rot.addRotR.x * fabsf(controls.rot.x) * kdt * 100.0);
    else
        rot_speed.x = tend(rot_speed.x, 0.0f, params.rot.addRotR.x * kdt * 40.0);

    //get axis

    nya_math::vec3 up(0.0, 1.0, 0.0);
    nya_math::vec3 forward(0.0, 0.0, 1.0);
    nya_math::vec3 right(1.0, 0.0, 0.0);
    forward = rot.rotate(forward);
    up = rot.rotate(up);
    right = rot.rotate(right);

    //rotation

    float high_g_turn = 1.0f + controls.brake * 0.5;
    rot = rot * nya_math::quat(nya_math::vec3(0.0, 0.0, 1.0), rot_speed.z * kdt * d2r * 0.7);
    rot = rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), rot_speed.y * kdt * d2r * 0.12);
    rot = rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), rot_speed.x * kdt * d2r * (0.13 + 0.05 * (1.0f - fabsf(up.y)))
                               * high_g_turn * (rot_speed.x < 0 ? 1.0f : 1.0f * params.rot.pitchUpDownR));

    //nose goes down while upside-down

    const nya_math::vec3 rot_grav = params.rotgraph.rotGravR.get(speed_arg);
    rot = rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), -(1.0 - up.y) * kdt * d2r * rot_grav.x * 0.5f);
    rot = rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * kdt * d2r * rot_grav.y * 0.5f);

    //nose goes down during stall

    if (speed < params.move.speed.speedStall)
    {
        float stallRotSpeed = params.rot.rotStallR * kdt * d2r * 10.0f;
        rot = rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), up.y * stallRotSpeed);
        rot = rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * stallRotSpeed);
    }

    //adjust throttle to fit cruising speed

    float throttle = controls.throttle;
    float brake = controls.brake;
    if (brake < 0.01f && throttle < 0.01f)
    {
        if (speed < params.move.speed.speedCruising)
            throttle = nya_math::min((params.move.speed.speedCruising - speed) * 0.1f, 0.5f);
        else if (speed > params.move.speed.speedCruising)
            brake = nya_math::min((speed - params.move.speed.speedCruising) * 0.1f, 0.1f);
    }

    //afterburner

    if (controls.throttle > 0.3)
    {
        thrust_time += kdt;
        if (thrust_time >= params.move.accel.thrustMinWait)
        {
            thrust_time = params.move.accel.thrustMinWait;
            throttle *= params.move.accel.powerAfterBurnerR;
        }
    }
    else if (thrust_time > 0.0f)
    {
        thrust_time -= kdt;
        if (thrust_time < 0.0f)
            thrust_time = 0.0f;
    }

    //apply acceleration

    vel = nya_math::vec3::lerp(vel, forward * speed, nya_math::min(5.0 * kdt, 1.0));

    vel += forward * (params.move.accel.acceleR * throttle * kdt * 50.0f - params.move.accel.deceleR * brake * kdt * speed * 0.04f);
    speed = vel.length();
    if (speed < params.move.speed.speedMin)
        vel = vel * (params.move.speed.speedMin / speed);
    if (speed > params.move.speed.speedMax)
        vel = vel * (params.move.speed.speedMax / speed);

    //apply speed

    const float kmph_to_meps = 1.0 / 3.6f;
    pos += vel * (kmph_to_meps * kdt);
    if (pos.y < 5.0f)
    {
        pos.y = 5.0f;
        rot = nya_math::quat(-0.5, rot.get_euler().y, 0.0);
        vel = rot.rotate(nya_math::vec3(0.0, 0.0, 1.0)) * params.move.speed.speedCruising * 0.8;
    }
}

//------------------------------------------------------------
}
