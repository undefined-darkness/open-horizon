//
// open horizon -- undefined_darkness@outlook.com
//

#include "physics.h"
#include "math/constants.h"
#include "math/scalar.h"
#include "containers/fhm.h"
#include <algorithm>

namespace phys
{
//------------------------------------------------------------

namespace { const static params::text_params &get_arms_param() { static params::text_params ap("Arms/ArmsParam.txt"); return ap; } }

//------------------------------------------------------------

static const float meps_to_kmph = 3.6f;
static const float kmph_to_meps = 1.0 / meps_to_kmph;
static const float ang_to_rad = nya_math::constants::pi / 180.0;

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

void world::set_location(const char *name)
{
    fhm_file fhm;
    if (!fhm.open((std::string("Map/") + name + ".fhm").c_str()))
        return;

    assert(fhm.get_chunk_size(4) == location_size*location_size);
    fhm.read_chunk_data(4, m_height_patches);

    m_heights.resize(fhm.get_chunk_size(5)/4);
    assert(!m_heights.empty());
    fhm.read_chunk_data(5, &m_heights[0]);

    //ToDo: collision meshes
}

//------------------------------------------------------------

plane_ptr world::add_plane(const char *name)
{
    plane_ptr p(new plane());
    p->params.load(("Player/Behavior/param_p_" + std::string(name) + ".bin").c_str());
    p->reset_state();
    m_planes.push_back(p);
    get_arms_param();  //cache
    return p;
}

//------------------------------------------------------------

missile_ptr world::add_missile(const char *name)
{
    missile_ptr m(new missile());

    const std::string pref = "." + std::string(name) + ".action.";

    auto &param = get_arms_param();

    m->no_accel_time = param.get_float(pref + "noAcceleTime") * 1000;
    m->accel = param.get_float(pref + "accele") * kmph_to_meps;
    m->speed_init = param.get_float(pref + "speedInit") * kmph_to_meps;
    m->max_speed = param.get_float(pref + "speedMax") * kmph_to_meps;
    m->gravity = param.get_float(pref + "gravity");

    m->rot_max = param.get_float(pref + "rotAngMax") * ang_to_rad;
    m->rot_max_hi = param.get_float(pref + "rotAngMaxHi") * ang_to_rad;
    m->rot_max_low = param.get_float(pref + "rotAngMaxLow") * ang_to_rad;

    m_missiles.push_back(m);
    return m;
}

//------------------------------------------------------------

void world::update_planes(int dt, hit_hunction on_hit)
{
    m_planes.erase(std::remove_if(m_planes.begin(), m_planes.end(), [](plane_ptr &p){ return p.use_count() <= 1; }), m_planes.end());
    for (auto &p: m_planes)
    {
        p->update(dt);

        if (p->pos.y < get_height(p->pos.x, p->pos.z) + 5.0f)
        {
            p->pos.y = 5.0f;
            //p->rot = quat(-0.5, p->rot.get_euler().y, 0.0);
            p->vel = vec3();

            if (on_hit)
                on_hit(std::static_pointer_cast<object>(p), object_ptr());
        }
    }
}

//------------------------------------------------------------

void world::update_missiles(int dt, hit_hunction on_hit)
{
    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), [](missile_ptr &m){ return m.use_count() <= 1; }), m_missiles.end());
    for (auto &m: m_missiles) m->update(dt);
}

//------------------------------------------------------------

float world::get_height(float x, float z) const
{
    if(m_heights.empty())
        return 0.0f;

    const int patch_size = 1024;
    const unsigned int quads_per_patch = 8, hquads_per_quad = 8;

    const int hpatch_size = patch_size / hquads_per_quad;

    const int base = location_size/2 * patch_size * quads_per_patch;

    const int idx_x = int(x + base) / hpatch_size;
    const int idx_z = int(z + base) / hpatch_size;

    if (idx_x < 0 || idx_x + 1 >= location_size * quads_per_patch * hquads_per_quad)
        return 0.0f;

    if (idx_z < 0 || idx_z + 1 >= location_size * quads_per_patch * hquads_per_quad)
        return 0.0f;

    const int tmp_idx_x = idx_x / hquads_per_quad;
    const int tmp_idx_z = idx_z / hquads_per_quad;

    const int pidx_x = tmp_idx_x / quads_per_patch;
    const int pidx_z = tmp_idx_z / quads_per_patch;

    const int qidx_x = tmp_idx_x - pidx_x * hquads_per_quad;
    const int qidx_z = tmp_idx_z - pidx_z * hquads_per_quad;

    const int hpw = quads_per_patch*quads_per_patch+1;
    const int h_idx = m_height_patches[pidx_z * location_size + pidx_x] * hpw * hpw;

    const int hpp = (hpw-1)/quads_per_patch;
    const float *h = &m_heights[h_idx+(qidx_x + qidx_z * hpw)*hpp];

    const uint hhpw = hquads_per_quad*hquads_per_quad+1;

    const int hidx_x = idx_x - tmp_idx_x * hquads_per_quad;
    const int hidx_z = idx_z - tmp_idx_z * hquads_per_quad;

    const float kx = (x + base) / hpatch_size - idx_x;
    const float kz = (z + base) / hpatch_size - idx_z;

    const float h00 = h[hidx_x + hidx_z * hhpw];
    const float h10 = h[hidx_x + 1 + hidx_z * hhpw];
    const float h01 = h[hidx_x + (hidx_z + 1) * hhpw];
    const float h11 = h[hidx_x + 1 + (hidx_z + 1) * hhpw];

    const float h00_h10 = nya_math::lerp(h00, h10, kx);
    const float h01_h11 = nya_math::lerp(h01, h11, kx);

    return nya_math::lerp(h00_h10, h01_h11, kz);
}

//------------------------------------------------------------

void plane::reset_state()
{
    thrust_time = 0.0;
    rot_speed = vec3();
    vel = rot.rotate(vec3(0.0, 0.0, 1.0)) * params.move.speed.speedCruising * kmph_to_meps;
}

//------------------------------------------------------------

void plane::update(int dt)
{
    float kdt = dt * 0.001f;

    vel *= meps_to_kmph;

    //simulation

    const float d2r = 3.1416 / 180.0f;

    float speed = vel.length();
    const float speed_arg = params.rotgraph.speed.get(speed);

    const vec3 rspeed = params.rotgraph.speedRot.get(speed_arg);

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

    vec3 up(0.0, 1.0, 0.0);
    vec3 forward(0.0, 0.0, 1.0);
    vec3 right(1.0, 0.0, 0.0);
    forward = rot.rotate(forward);
    up = rot.rotate(up);
    right = rot.rotate(right);

    //rotation

    float high_g_turn = 1.0f + controls.brake * 0.5;
    rot = rot * quat(vec3(0.0, 0.0, 1.0), rot_speed.z * kdt * d2r * 0.7);
    rot = rot * quat(vec3(0.0, 1.0, 0.0), rot_speed.y * kdt * d2r * 0.12);
    rot = rot * quat(vec3(1.0, 0.0, 0.0), rot_speed.x * kdt * d2r * (0.13 + 0.05 * (1.0f - fabsf(up.y)))
                               * high_g_turn * (rot_speed.x < 0 ? 1.0f : 1.0f * params.rot.pitchUpDownR));

    //nose goes down while upside-down

    const vec3 rot_grav = params.rotgraph.rotGravR.get(speed_arg);
    rot = rot * quat(vec3(1.0, 0.0, 0.0), -(1.0 - up.y) * kdt * d2r * rot_grav.x * 0.5f);
    rot = rot * quat(vec3(0.0, 1.0, 0.0), -right.y * kdt * d2r * rot_grav.y * 0.5f);

    //nose goes down during stall

    if (speed < params.move.speed.speedStall)
    {
        float stallRotSpeed = params.rot.rotStallR * kdt * d2r * 10.0f;
        rot = rot * quat(vec3(1.0, 0.0, 0.0), up.y * stallRotSpeed);
        rot = rot * quat(vec3(0.0, 1.0, 0.0), -right.y * stallRotSpeed);
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

    vel = vec3::lerp(vel, forward * speed, nya_math::min(5.0 * kdt, 1.0));

    vel += forward * (params.move.accel.acceleR * throttle * kdt * 50.0f - params.move.accel.deceleR * brake * kdt * speed * 0.04f);
    speed = vel.length();
    if (speed < params.move.speed.speedMin)
        vel = vel * (params.move.speed.speedMin / speed);
    if (speed > params.move.speed.speedMax)
        vel = vel * (params.move.speed.speedMax / speed);

    //apply speed

    vel *= kmph_to_meps;

    pos += vel * kdt;
}

//------------------------------------------------------------

void missile::update(int dt)
{
    const float kdt = 0.001f * dt;

    if (no_accel_time > 0)
    {
        no_accel_time -= dt;
        vel.y -= gravity * kdt;
    }
    else
    {
        if (!accel_started)
        {
            vel += rot.rotate(vec3(0.0, 0.0, speed_init));
            accel_started = true;
        }

        //ToDo: non-ideal rotation (clamp aoa)

        const float eps=1.0e-6f;
        const nya_math::vec3 v=nya_math::vec3::normalize(target_dir);
        const float xz_sqdist=v.x*v.x+v.z*v.z;

        const float new_yaw=(xz_sqdist>eps*eps)? (atan2(v.x,v.z)) : rot.get_euler().y;
        const float new_pitch=(fabsf(v.y)>eps)? (-atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;
        rot = nya_math::quat(new_pitch, new_yaw, 0.0);

        vec3 forward = rot.rotate(vec3(0.0, 0.0, 1.0));

        vel += forward * accel * kdt;
        float speed = vel.length();
        if (speed > max_speed)
        {
            vel = vel * (max_speed / speed);
            speed = max_speed;
        }

        vel = vec3::lerp(vel, forward * speed, nya_math::min(5.0 * kdt, 1.0));

    }

    pos += vel * kdt;
}

//------------------------------------------------------------
}
