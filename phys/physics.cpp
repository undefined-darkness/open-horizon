//
// open horizon -- undefined_darkness@outlook.com
//

#include "physics.h"
#include "math/constants.h"
#include "math/scalar.h"
#include "memory/memory_reader.h"
#include "containers/fhm.h"
#include "util/location.h"
#include "util/xml.h"
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
    m_heights.clear();
    m_meshes.clear();
    m_qtree = nya_math::quadtree();

    m_height_quad_size = 1024;
    m_height_quad_frags = 8;
    m_height_subquads_per_quad = 8;

    if (is_native_location(name))
    {
        auto &zip = get_native_location_provider(name);

        pugi::xml_document doc;
        if (!load_xml(zip.access("info.xml"), doc))
            return;

        auto tiles = doc.first_child().child("tiles");
        m_height_quad_size = tiles.attribute("quad_size").as_int();
        m_height_quad_frags = tiles.attribute("quad_frags").as_int();
        m_height_subquads_per_quad = tiles.attribute("subfrags").as_int();

        auto height_off = load_resource(zip.access("height_offsets.bin"));
        assert(height_off.get_size() >= sizeof(m_height_patches));
        height_off.copy_to(m_height_patches, sizeof(m_height_patches));
        height_off.free();

        auto heights = load_resource(zip.access("heights.bin"));
        std::string format = doc.first_child().child("heightmap").attribute("format").as_string();
        if (format == "byte")
        {
            m_heights.resize(heights.get_size());
            auto hdata = (unsigned char *)heights.get_data();
            const float hscale = doc.first_child().child("heightmap").attribute("scale").as_float(1.0f);
            for (size_t i = 0; i < m_heights.size(); ++i)
                m_heights[i] = hdata[i] * hscale;
        }
        else if (format == "float")
        {
            m_heights.resize(heights.get_size()/4);
            heights.copy_to(m_heights.data(), heights.get_size());
        }
        heights.free();

        //ToDo: objects

        return;
    }

    fhm_file fhm;
    if (!fhm.open((std::string("Map/") + name + ".fhm").c_str()))
        return;

    assert(fhm.get_chunk_size(4) == location_size*location_size);
    fhm.read_chunk_data(4, m_height_patches);

    m_heights.resize(fhm.get_chunk_size(5)/4);
    assert(!m_heights.empty());
    fhm.read_chunk_data(5, &m_heights[0]);

    for (int i = 0; i < fhm.get_chunks_count(); ++i)
    {
        if (fhm.get_chunk_type(i) == 'HLOC')
        {
            nya_memory::tmp_buffer_scoped buf(fhm.get_chunk_size(i));
            fhm.read_chunk_data(i, buf.get_data());
            m_meshes.resize(m_meshes.size() + 1);
            m_meshes.back().load(buf.get_data(), buf.get_size());
        }
    }

    fhm.close();

    if (!fhm.open((std::string("Map/") + name + "_mpt.fhm").c_str()))
        return;

    assert(fhm.get_chunks_count() == m_meshes.size());

    for (int i = 0; i < fhm.get_chunks_count(); ++i)
    {
        assert(fhm.get_chunk_type(i) == 'xtpm');

        nya_memory::tmp_buffer_scoped buf(fhm.get_chunk_size(i));
        fhm.read_chunk_data(i, buf.get_data());
        nya_memory::memory_reader reader(buf.get_data(), buf.get_size());
        reader.seek(128);
        const auto off = m_instances.size();
        const auto count = reader.read<uint32_t>();
        reader.skip(16);
        m_instances.resize(off + count);
        for (uint32_t j = 0; j < count; ++j)
        {
            auto &inst = m_instances[off + j];
            inst.mesh_idx = i;
            inst.pos = reader.read<nya_math::vec3>();
            const float yaw = reader.read<float>();
            inst.yaw_s = sinf(yaw);
            inst.yaw_c = cosf(yaw);
            inst.bbox = nya_math::aabb(m_meshes[i].bbox, inst.pos,
                                       nya_math::quat(0.0f, yaw, 0.0f), nya_math::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    fhm.close();

    const int size = 64000;
    m_qtree = nya_math::quadtree(-size, -size, size * 2, size * 2, 10);

    for(int i=0;i<(int)m_instances.size();++i)
        m_qtree.add_object(m_instances[i].bbox, i);
}

//------------------------------------------------------------

plane_ptr world::add_plane(const char *name, bool add_to_world)
{
    auto p = std::make_shared<plane>();
    p->params.load(("Player/Behavior/param_p_" + std::string(name) + ".bin").c_str());
    p->reset_state();
    if (add_to_world)
        m_planes.push_back(p);
    get_arms_param();  //cache
    return p;
}

//------------------------------------------------------------

missile_ptr world::add_missile(const char *name, bool add_to_world)
{
    auto m = std::make_shared<missile>();

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

    if (add_to_world)
        m_missiles.push_back(m);
    return m;
}

//------------------------------------------------------------

bomb_ptr world::add_bomb(const char *name, bool add_to_world)
{
    auto b = std::make_shared<bomb>();
    const std::string pref = "." + std::string(name) + ".action.";
    auto &param = get_arms_param();
    b->gravity = param.get_float(pref + "gravity");

    if (add_to_world)
        m_bombs.push_back(b);
    return b;
}

//------------------------------------------------------------

bool world::spawn_bullet(const char *type, const vec3 &pos, const vec3 &dir, vec3 &result_pos)
{
    bullet b;
    b.pos = pos;
    b.time = 1500; //ToDo
    b.vel = dir * 1000.0f; //ToDo

    const float t = (b.time * 0.001f);
    auto to_dir = b.vel * t;
    to_dir.y -= 9.8f * t * t * 0.5f;

    const auto to = pos + to_dir;

    static std::vector<int> insts;
    float result = 1.0f, min_result = 1.0f;
    bool hit = false;
    nya_math::aabb box(nya_math::vec3::min(pos,to), nya_math::vec3::max(pos,to));
    if (m_qtree.get_objects(box, insts))
    {
        for (auto &i:insts)
        {
            const auto &mi = m_instances[i];

            const auto lpt = mi.transform_inv(to), lpf = mi.transform_inv(pos);
            auto &m = m_meshes[mi.mesh_idx];
            //if (!m.bbox.test_intersect(lpt)) //ToDo: trace bbox
            //    continue;

            if(!m.trace(lpf, lpt, result))
                continue;

            if(result < min_result)
                min_result = result;

            hit = true;
            //printf("hit instance %d mesh %d\n", i, mi.mesh_idx);
        }
    }

    if (hit)
        b.time *= min_result;

    m_bullets.push_back(b);

    result_pos = pos + to_dir * min_result;

    return hit;
}

//------------------------------------------------------------

void world::update_planes(int dt, const hit_hunction &on_hit)
{
    m_planes.erase(std::remove_if(m_planes.begin(), m_planes.end(), [](const plane_ptr &p){ return p.unique(); }), m_planes.end());
    for (auto &p: m_planes)
    {
        p->update(dt);

        const auto pt = p->pos + p->vel * (dt * 0.001f);
        const auto pt_nose = pt + p->rot.rotate(p->nose_offset);

        auto wing_offset2 = p->wing_offset;
        wing_offset2.x = -wing_offset2.x; //will not work for AD-1, lol
        const auto pt_wing = pt + p->rot.rotate(p->wing_offset);
        const auto pt_wing2 = pt + p->rot.rotate(wing_offset2);

        const float r = nya_math::max(p->nose_offset.length(), p->wing_offset.length()) * 1.5;

        nya_math::aabb box;
        box.origin = pt;
        box.delta.set(r, r, r);

        bool hit = pt.y < get_height(pt.x, pt.z, false) + 5.0f;
        if (!hit)
        {
            static std::vector<int> insts;
            if (m_qtree.get_objects(box, insts))
            {
                for (auto &i:insts)
                {
                    const auto &mi = m_instances[i];
                    const auto &m = m_meshes[mi.mesh_idx];

                    /*
                    box.origin = mi.transform_inv(pt);
                    if (!m.bbox.test_intersect(box))
                        continue;
                    */

                    auto lpt = mi.transform_inv(pt_nose), lpf = mi.transform_inv(p->pos);
                    if (m.trace(lpf, lpt))
                    {
                        hit = true;
                        break;
                    }

                    auto lwt = mi.transform_inv(pt_wing), lwt2 = mi.transform_inv(pt_wing2);

                    if (m.trace(lwt, lwt2) || m.trace(lwt2, lwt)) //trace fails near from point
                    {
                        hit = true;
                        break;
                    }
                }
            }
        }

        if (hit)
        {
            p->vel = vec3();

            if (on_hit)
                on_hit(std::static_pointer_cast<object>(p), object_ptr());
        }
/*
        //test
        get_debug_draw().clear();
        static std::vector<int> insts;

        nya_math::aabb test;
        test.origin = p->pos;
        test.delta.set(10, 10, 10);

        //m_qtree.get_objects(p->pos, insts);
        m_qtree.get_objects(test, insts);
        for (auto &i: insts)
            get_debug_draw().add_aabb(m_instances[i].bbox);
*/
    }
}

//------------------------------------------------------------

template<typename t> void world::update_projectiles(int dt, std::vector<t> &objects, const hit_hunction &on_hit)
{
    objects.erase(std::remove_if(objects.begin(), objects.end(), [](const t &o){ return o.unique(); }), objects.end());
    for (auto &o: objects)
    {
        o->update(dt);

        const auto pt = o->pos + o->vel * (dt * 0.001f);

        bool hit = pt.y < get_height(pt.x, pt.z, false) + 1.0f;
        if (!hit)
        {
            static std::vector<int> insts;
            if (m_qtree.get_objects(pt, insts))
            {
                for (auto &i:insts)
                {
                    const auto &mi = m_instances[i];

                    auto lpt = mi.transform_inv(pt), lpf = mi.transform_inv(o->pos);
                    auto &m = m_meshes[mi.mesh_idx];
                    if (!m.bbox.test_intersect(lpt))
                        continue;

                    if(!m.trace(lpf, lpt))
                        continue;

                    hit = true;
                    break;
                }
            }
        }

        if (hit)
        {
            o->vel = vec3();

            if (on_hit)
                on_hit(std::static_pointer_cast<object>(o), object_ptr());
        }
    }
}

//------------------------------------------------------------

void world::update_missiles(int dt, const hit_hunction &on_hit)
{
    update_projectiles(dt, m_missiles, on_hit);
}

//------------------------------------------------------------

void world::update_bombs(int dt, const hit_hunction &on_hit)
{
    update_projectiles(dt, m_bombs, on_hit);
}

//------------------------------------------------------------

void world::update_bullets(int dt)
{
    m_bullets.erase(std::remove_if(m_bullets.begin(), m_bullets.end(), [](const bullet &b){ return b.time < 0; }), m_bullets.end());

    float kdt = dt * 0.001f;

    for (auto &b: m_bullets)
    {
        b.time -= dt;
        b.pos += b.vel * kdt;
        b.vel.y -= 9.8f * kdt;
    }
}

//------------------------------------------------------------

float world::get_height(float x, float z, bool include_objects) const
{
    if (m_heights.empty())
        return 0.0f;

    const int hpatch_size = m_height_quad_size / m_height_subquads_per_quad;

    const int base = location_size/2 * m_height_quad_size * m_height_quad_frags;

    const int idx_x = int(x + base) / hpatch_size;
    const int idx_z = int(z + base) / hpatch_size;

    if (idx_x < 0 || idx_x + 1 >= location_size * m_height_quad_frags * m_height_subquads_per_quad)
        return 0.0f;

    if (idx_z < 0 || idx_z + 1 >= location_size * m_height_quad_frags * m_height_subquads_per_quad)
        return 0.0f;

    const int pidx_x = idx_x / (m_height_quad_frags * m_height_subquads_per_quad);
    const int pidx_z = idx_z / (m_height_quad_frags * m_height_subquads_per_quad);
    const int hpw = m_height_quad_frags * m_height_subquads_per_quad + 1;
    const int h_idx = m_height_patches[pidx_z * location_size + pidx_x] * hpw * hpw;

    const int qidx_x = idx_x / m_height_subquads_per_quad - pidx_x * m_height_quad_frags;
    const int qidx_z = idx_z / m_height_subquads_per_quad - pidx_z * m_height_quad_frags;

    const float *h = &m_heights[h_idx + (qidx_x + qidx_z * hpw) * m_height_subquads_per_quad];

    const int hidx_x = idx_x % m_height_subquads_per_quad;
    const int hidx_z = idx_z % m_height_subquads_per_quad;

    const float kx = (x + base) / hpatch_size - idx_x;
    const float kz = (z + base) / hpatch_size - idx_z;

    const unsigned int hhpw = m_height_quad_frags * m_height_subquads_per_quad + 1;

    const float h00 = h[hidx_x + hidx_z * hhpw];
    const float h10 = h[hidx_x + 1 + hidx_z * hhpw];
    const float h01 = h[hidx_x + (hidx_z + 1) * hhpw];
    const float h11 = h[hidx_x + 1 + (hidx_z + 1) * hhpw];

    const float h00_h10 = nya_math::lerp(h00, h10, kx);
    const float h01_h11 = nya_math::lerp(h01, h11, kx);

    float height = nya_math::lerp(h00_h10, h01_h11, kz);

    if (!include_objects)
        return height;

    const float max_height = 16000.0f;
    vec3 pos(x, max_height, z), to(x, 0.0f, z);
    static std::vector<int> insts;
    if (m_qtree.get_objects((int)x, (int)z, insts))
    {
        for (auto &i:insts)
        {
            const auto &mi = m_instances[i];
            const auto lpt = mi.transform_inv(to), lpf = mi.transform_inv(pos);
            auto &m = m_meshes[mi.mesh_idx];

            float h;
            if(!m.trace(lpf, lpt, h))
                continue;

            h = max_height * (1.0f - h);
            if(h > height)
                height = h;
        }
    }

    return height;
}

//------------------------------------------------------------

void plane::reset_state()
{
    thrust_time = 0.0;
    rot_speed = vec3();
    vel = rot.rotate(vec3::forward()) * params.move.speed.speedCruising * kmph_to_meps;
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

    vec3 forward = rot.rotate(vec3::forward());
    vec3 up = rot.rotate(vec3::up());
    vec3 right = rot.rotate(vec3::right());

    //rotation

    float high_g_turn = 1.0f + controls.brake * 0.5;
    rot = rot * quat(vec3::forward(), rot_speed.z * kdt * d2r * 0.7);
    rot = rot * quat(vec3::up(), rot_speed.y * kdt * d2r * 0.12);
    rot = rot * quat(vec3::right(), rot_speed.x * kdt * d2r * (0.13 + 0.05 * (1.0f - fabsf(up.y)))
                               * high_g_turn * (rot_speed.x < 0 ? 1.0f : 1.0f * params.rot.pitchUpDownR));

    //nose goes down while upside-down

    const vec3 rot_grav = params.rotgraph.rotGravR.get(speed_arg);
    rot = rot * quat(vec3::right(), -(1.0 - up.y) * kdt * d2r * rot_grav.x * 0.5f);
    rot = rot * quat(vec3::up(), -right.y * kdt * d2r * rot_grav.y * 0.5f);

    //nose goes down during stall

    if (speed < params.move.speed.speedStall)
    {
        float stallRotSpeed = params.rot.rotStallR * kdt * d2r * 10.0f;
        rot = rot * quat(vec3::right(), up.y * stallRotSpeed);
        rot = rot * quat(vec3::up(), -right.y * stallRotSpeed);
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

    if (controls.throttle > 0.3f)
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

    vel = vec3::lerp(vel, forward * speed, nya_math::min(5.0f * kdt, 1.0f));

    const float brake_eff = nya_math::min(1.01f + forward.dot(vec3::up()), 1.0f);

    vel += forward * (params.move.accel.acceleR * throttle * kdt * 50.0f - params.move.accel.deceleR * brake * brake_eff * kdt * speed * 0.04f);
    speed = vel.length();

    const float grav = 9.8f * meps_to_kmph * kdt;
    vel.y -= grav;
    vel += up * grav;

    if (speed < params.move.speed.speedMin)
        vel = vel * (params.move.speed.speedMin / speed);
    if (speed > params.move.speed.speedMax)
        vel = vel * (params.move.speed.speedMax / speed);

    //apply speed

    vel *= kmph_to_meps;

    pos += vel * kdt;
}

//------------------------------------------------------------

float plane::get_speed_kmh() const { return vel.length() * meps_to_kmph; }

//------------------------------------------------------------

float plane::get_thrust() const
{
    return 0.5 * (nya_math::min(nya_math::max(get_speed_kmh() - params.move.speed.speedMin, 0.0)
                                / (params.move.speed.speedCruising - params.move.speed.speedMin), 1.0f)
                  + thrust_time / params.move.accel.thrustMinWait);
}

//------------------------------------------------------------

bool plane::get_ab() const
{
    return thrust_time >= params.move.accel.thrustMinWait;
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

        const float eps=1.0e-6f;
        const vec3 v=vec3::normalize(target_dir);
        const float xz_sqdist=v.x*v.x+v.z*v.z;

        auto pyr = rot.get_euler();

        const nya_math::angle_rad new_yaw=(xz_sqdist>eps*eps)? (atan2(v.x,v.z)) : pyr.y;
        const nya_math::angle_rad new_pitch=(fabsf(v.y)>eps)? (-atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;

        nya_math::angle_rad yaw_diff = new_yaw - pyr.y;
        nya_math::angle_rad pitch_diff = new_pitch - pyr.x;

        if (rot_max > eps)
        {
            const nya_math::angle_rad angle_clamp = rot_max * kdt;
            yaw_diff.normalize().clamp(-angle_clamp, angle_clamp);
            pitch_diff.normalize().clamp(-angle_clamp, angle_clamp);
        }

        rot = quat(pyr.x + pitch_diff, pyr.y + yaw_diff, 0.0f);

        float speed = vel.length() + accel * kdt;
        if (speed > max_speed)
            speed = max_speed;

        vel = rot.rotate(vec3(0.0, 0.0, speed));
    }

    pos += vel * kdt;
}

//------------------------------------------------------------

void bomb::update(int dt)
{
    const float kdt = 0.001f * dt;
    vel.y -= gravity * kdt;
    pos += vel * kdt;
}

//------------------------------------------------------------

vec3 world::instance::transform(const vec3 &v) const
{
    return vec3(yaw_c*v.x+yaw_s*v.z, v.y, yaw_c*v.z-yaw_s*v.x) + pos;
}

//------------------------------------------------------------

vec3 world::instance::transform_inv(const vec3 &v) const
{
    vec3 r = v - pos;
    r.set(yaw_c*r.x-yaw_s*r.z, r.y, yaw_s*r.x+yaw_c*r.z);
    return r;
}

//------------------------------------------------------------
}
