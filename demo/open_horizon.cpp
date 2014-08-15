//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "plane_params.h"
#include "fhm_mesh.h"
#include "fhm_location.h"
#include "qdf_provider.h"

#include "render/vbo.h"
#include "render/fbo.h"
#include "render/render.h"
#include "math/vector.h"
#include "math/quaternion.h"
#include "scene/camera.h"
#include "scene/shader.h"
#include "system/system.h"
#include "memory/memory_reader.h"
#include "stdio.h"
#include "shared.h"
#include "resources/file_resources_provider.h"

#include <math.h>
#include <vector>
#include <assert.h>

#include "render/debug_draw.h"
nya_render::debug_draw test;

//------------------------------------------------------------

class plane_camera
{
public:
    void add_delta_rot(float dx, float dy);
    void reset_delta_rot() { m_drot.x = 0.0f; m_drot.y = 3.14f; }
    void add_delta_pos(float dx, float dy, float dz);

    void set_aspect(float aspect);
    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; update(); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; update(); }

private:
    void update();

public:
    plane_camera(): m_dpos(0.0f, 3.0f, 10.0f) { m_drot.y = 3.14f; }

private:
    nya_math::vec3 m_drot;
    nya_math::vec3 m_dpos;

    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
};

//------------------------------------------------------------

void plane_camera::add_delta_rot(float dx, float dy)
{
    m_drot.x += dx;
    m_drot.y += dy;
    
    const float max_angle = 3.14f * 2.0f;
    
    if (m_drot.x > max_angle)
        m_drot.x -= max_angle;
    
    if (m_drot.x < -max_angle)
        m_drot.x += max_angle;
    
    if (m_drot.y > max_angle)
        m_drot.y -= max_angle;
    
    if (m_drot.y< -max_angle)
        m_drot.y+=max_angle;
    
    update();
}

//------------------------------------------------------------

void plane_camera::add_delta_pos(float dx, float dy, float dz)
{
    m_dpos.x -= dx;
    m_dpos.y -= dy;
    m_dpos.z -= dz;
    if (m_dpos.z < 5.0f)
        m_dpos.z = 5.0f;

    if (m_dpos.z > 20000.0f)
        m_dpos.z = 20000.0f;

    update();
}

//------------------------------------------------------------

void plane_camera::set_aspect(float aspect)
{
    nya_scene::get_camera().set_proj(55.0, aspect, 1.0, 25000.0);
                      //1300.0);
    update();
}

//------------------------------------------------------------

void plane_camera::update()
{
    nya_math::quat r = m_rot;
    r.v.x = -r.v.x, r.v.y = -r.v.y;

    r = r*nya_math::quat(m_drot.x, m_drot.y, 0.0f);
    //nya_math::vec3 r=m_drot+m_rot;

    //cam->set_rot(r.y*180.0f/3.14f,r.x*180.0f/3.14f,0.0);
    nya_scene::get_camera().set_rot(r);

    //nya_math::quat rot(-r.x,-r.y,0.0f);
    //rot=rot*m_rot;

    //nya_math::vec3 pos=m_pos+rot.rotate(m_dpos);
    r.v.x = -r.v.x, r.v.y = -r.v.y;
    nya_math::vec3 pos = m_pos + r.rotate(m_dpos);

    nya_scene::get_camera().set_pos(pos.x, pos.y, pos.z);
}

//------------------------------------------------------------

class aircraft
{
public:
    bool load(const char *name)
    {
        if (!name)
            return false;

        m_mesh.load((std::string("model_id/mech/plyr/p_") + name + "/p_" + name + "_pcom.fhm").c_str());
        m_mesh.set_ndxr_texture(0, "ambient", (std::string("model_id/tdb/mech/plyr/p_") +
                                               name + "/00/p_" + name + "_00_amb.img").c_str());
        if (strcmp(name, "f22a") == 0)
        {
            m_mesh.set_ndxr_texture(0, "diffuse", "model_id/tdb/mech/plyr/p_f22a/00/p_f22a_01_col.img");
            m_mesh.set_ndxr_texture(0, "specular", "model_id/tdb/mech/plyr/p_f22a/00/p_f22a_01_spe.img");
            //m_mesh.set_ndxr_texture(0, "diffuse", "model_id/tdb/mech/plyr/p_f22a/12/p_f22a_12_col.img");
            //m_mesh.set_ndxr_texture(0, "specular", "model_id/tdb/mech/plyr/p_f22a/12/p_f22a_12_spe.img");
        }
        else
        {
            m_mesh.set_ndxr_texture(0, "diffuse", (std::string("model_id/tdb/mech/plyr/p_") +
                                                   name + "/01/p_" + name + "_01_col.img").c_str());
            m_mesh.set_ndxr_texture(0, "specular", (std::string("model_id/tdb/mech/plyr/p_") +
                                                    name + "/01/p_" + name + "_01_spe.img").c_str());
        }

        m_params.load((std::string("Player/Behavior/param_p_") + name + ".bin").c_str());

        m_speed = m_params.move.speed.speedCruising;

        m_mesh.set_relative_anim_time(0, 'cndl', 0.5); //fgo
        m_mesh.set_relative_anim_time(0, 'cndr', 0.5);
        m_mesh.set_relative_anim_time(0, 'rudl', 0.5); //tail vert
        m_mesh.set_relative_anim_time(0, 'rudr', 0.5);
        m_mesh.set_relative_anim_time(0, 'elvl', 0.5); //tail hor
        m_mesh.set_relative_anim_time(0, 'elvr', 0.5);
        m_mesh.set_relative_anim_time(0, 'tefn', 0.5); //elerons, used only with flaperons
        m_mesh.set_relative_anim_time(0, 'alrl', 0.5); //wing sides, used with tail hor
        m_mesh.set_relative_anim_time(0, 'alrr', 0.5);
        m_mesh.set_relative_anim_time(0, 'lefn', 0.0); //flaperons
        m_mesh.set_relative_anim_time(0, 'rmpn', 0.0); //engine air supl
        //m_mesh.set_relative_anim_time(0, 'vctn', 0.0); //engine
        m_mesh.set_relative_anim_time(0, 'gunc', 0.0); //mgun
        m_mesh.set_relative_anim_time(0, 'misc', 0.0); //primary weapon
        m_mesh.set_relative_anim_time(0, 'spwc', 0.0); //special weapon

        return true;
    }

    void draw() { m_mesh.draw(0); }

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    const nya_math::vec3 &get_pos() { return m_pos; }
    nya_math::quat get_rot() { return m_rot; }

    float get_speed() { return m_speed; }
    float get_alt() { return m_pos.y; }

    void set_controls(const nya_math::vec3 &rot, float throttle, float brake)
    {
        m_controls_rot = rot;
        m_controls_throttle = throttle;
        m_controls_brake = brake;
    }

    void fire_mgun() { m_controls_mgun = true; }
    void fire_rocket() { m_controls_rocket = true; }
    void change_weapon() { m_controls_special = true; }

    void update(int dt)
    {
        float kdt = dt * 0.001f;

        //simulation

        const float d2r = 3.1416 / 180.0f;

        const float speed_arg = m_params.rotgraph.speed.get(m_speed);

        const nya_math::vec3 rot_speed = m_params.rotgraph.speedRot.get(speed_arg);

        const float eps = 0.001f;
        if (fabsf(m_controls_rot.z) > eps)
            m_rot_speed.z = tend(m_rot_speed.z, m_controls_rot.z * rot_speed.z, m_params.rot.addRotR.z * fabsf(m_controls_rot.z) * kdt * 100.0);
        else
            m_rot_speed.z = tend(m_rot_speed.z, 0.0f, m_params.rot.addRotR.z * kdt * 40.0);

        if (fabsf(m_controls_rot.y) > eps)
            m_rot_speed.y = tend(m_rot_speed.y, -m_controls_rot.y * rot_speed.y, m_params.rot.addRotR.y * fabsf(m_controls_rot.y) * kdt * 50.0);
        else
            m_rot_speed.y = tend(m_rot_speed.y, 0.0f, m_params.rot.addRotR.y * kdt * 10.0);

        if (fabsf(m_controls_rot.x) > eps)
            m_rot_speed.x = tend(m_rot_speed.x, m_controls_rot.x * rot_speed.x, m_params.rot.addRotR.x * fabsf(m_controls_rot.x) * kdt * 100.0);
        else
            m_rot_speed.x = tend(m_rot_speed.x, 0.0f, m_params.rot.addRotR.x * kdt * 40.0);

        //get axis

        nya_math::vec3 up(0.0, 1.0, 0.0);
        nya_math::vec3 forward(0.0, 0.0, 1.0);
        nya_math::vec3 right(1.0, 0.0, 0.0);
        forward = m_rot.rotate(forward);
        up = m_rot.rotate(up);
        right = m_rot.rotate(right);

        //rotation

        float high_g_turn = 1.0f + m_controls_brake * 0.5;
        m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 0.0, 1.0), m_rot_speed.z * kdt * d2r * 0.7);
        m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), m_rot_speed.y * kdt * d2r * 0.12);
        m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), m_rot_speed.x * kdt * d2r * (0.13 + 0.05 * (1.0f - fabsf(up.y))) * high_g_turn
                                                               * (m_rot_speed.x < 0 ? 1.0f : 1.0f * m_params.rot.pitchUpDownR));
        //nose goes down while upside-down

        const nya_math::vec3 rot_grav = m_params.rotgraph.rotGravR.get(speed_arg);
        m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), -(1.0 - up.y) * kdt * d2r * rot_grav.x * 0.5f);
        m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * kdt * d2r * rot_grav.y * 0.5f);

        //nose goes down during stall

        if (m_speed < m_params.move.speed.speedStall)
        {
            float stallRotSpeed = m_params.rot.rotStallR * kdt * d2r * 10.0f;
            m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), up.y * stallRotSpeed);
            m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * stallRotSpeed);
        }

        //adjust throttle to fit cruising speed

        float throttle = m_controls_throttle;
        float brake = m_controls_brake;
        if (brake < 0.01f && throttle < 0.01f)
        {
            if (m_speed < m_params.move.speed.speedCruising)
                throttle = std::min((m_params.move.speed.speedCruising - m_speed) * 0.1f, 0.1f);
            else if (m_speed > m_params.move.speed.speedCruising)
                brake = std::min((m_speed - m_params.move.speed.speedCruising) * 0.1f, 0.1f);
        }

        //afterburner

        if (m_controls_throttle > 0.3)
        {
            m_thrust_time += kdt;
            if (m_thrust_time >= m_params.move.accel.thrustMinWait)
            {
                m_thrust_time = m_params.move.accel.thrustMinWait;
                throttle *= m_params.move.accel.powerAfterBurnerR;
                m_mesh.set_anim_speed(0, 'rmpn', 0.7f);
            }
        }
        else if (m_thrust_time > 0.0f)
        {
            m_thrust_time -= kdt;
            if (m_thrust_time < 0.0f)
                m_thrust_time = 0.0f;

            m_mesh.set_anim_speed(0, 'rmpn', -0.5f);
        }

        //apply acceleration

        m_speed += m_params.move.accel.acceleR * throttle * kdt * 50.0f;
        m_speed -= m_params.move.accel.deceleR * brake * kdt * m_speed * 0.04f;
        if (m_speed < m_params.move.speed.speedMin)
            m_speed = m_params.move.speed.speedMin;
        if (m_speed > m_params.move.speed.speedMax)
            m_speed = m_params.move.speed.speedMax;

        //apply speed

        m_pos += forward * (m_speed / 3.6f) * kdt;
        if (m_pos.y < 7.0f)
        {
            m_pos.y = 7.0f;
            m_rot = nya_math::quat(-0.5, m_rot.get_euler().y, 0.0);
        }

        //animations

        float flap_anim_speed = 0.5f;
        if (m_speed < m_params.move.speed.speedCruising - 100)
        {
            m_mesh.set_anim_speed(0, 'lefn', flap_anim_speed);
            m_mesh.set_anim_speed(0, 'tefn', -flap_anim_speed);
        }
        else
        {
            m_mesh.set_anim_speed(0, 'lefn', -flap_anim_speed);
            if (m_mesh.get_relative_anim_time(0, 'tefn') < 0.5f)
                m_mesh.set_anim_speed(0, 'tefn', flap_anim_speed);
            else
            {
                m_mesh.set_anim_speed(0, 'tefn', 0.0f);
                m_mesh.set_relative_anim_time(0, 'tefn', 0.5f);
            }
        }

        const float speed_k = std::max((m_params.move.speed.speedMax - m_speed) / m_params.move.speed.speedMax, 0.1f);
        const float ae_anim_speed_k = 5.0f;
        float ideal_el = clamp(-m_controls_rot.z - m_controls_rot.x, -1.0f, 1.0f) * speed_k;
        float ideal_er = clamp(m_controls_rot.z - m_controls_rot.x, -1.0f, 1.0f) * speed_k;

        m_mesh.set_anim_speed(0, 'elvl', (ideal_el * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvl')) * ae_anim_speed_k);
        m_mesh.set_anim_speed(0, 'elvr', (ideal_er * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvr')) * ae_anim_speed_k);

        const float ideal_al = -m_controls_rot.z * speed_k;
        const float ideal_ar = m_controls_rot.z * speed_k;
        m_mesh.set_anim_speed(0, 'alrl', (ideal_al * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrl')) * ae_anim_speed_k);
        m_mesh.set_anim_speed(0, 'alrr', (ideal_ar * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrr')) * ae_anim_speed_k);

        const float ideal_cl = -m_controls_rot.x * speed_k;
        const float ideal_cr = -m_controls_rot.x * speed_k;
        m_mesh.set_anim_speed(0, 'cndl', (ideal_cl * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndl')) * ae_anim_speed_k);
        m_mesh.set_anim_speed(0, 'cndr', (ideal_cr * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndr')) * ae_anim_speed_k);

        float ideal_rl = clamp(-m_controls_rot.y + brake, -1.0f, 1.0f) * speed_k;
        float ideal_rr = clamp(-m_controls_rot.y - brake, -1.0f, 1.0f) * speed_k;

        m_mesh.set_anim_speed(0, 'rudl', (ideal_rl * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudl')) * ae_anim_speed_k);
        m_mesh.set_anim_speed(0, 'rudr', (ideal_rr * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudr')) * ae_anim_speed_k);

        //weapon animations

        m_mesh.set_anim_speed(0, 'gunc', 5.0 * (m_controls_mgun ? 1.0f : -1.0f));
        m_controls_mgun = false;

        if (m_controls_rocket)
        {
            //if (m_mesh.get_relative_anim_time(0, 'spwc') < 0.01f)
            if (!m_special_selected)
                m_rocket_bay_time = 3.0f;
            m_controls_rocket = false;
        }

        m_rocket_bay_time -= kdt;
        m_mesh.set_anim_speed(0, 'misc', m_rocket_bay_time > 0.01f ? 1.0f : -1.0f);

        if (m_controls_special)
        {
            m_special_selected = !m_special_selected;
            if (m_special_selected)
                m_rocket_bay_time = 0.0f;
            m_controls_special = false;
        }

        m_mesh.set_anim_speed(0, 'spwc', m_special_selected ? 1.0f : -1.0f);

        //alrl
        /*
        if (ideal_el > 0.01)
        {
            m_mesh.set_anim_speed(0, 'elvl', 1.0);
        }
        else if (ideal_el < -0.01)
        {
            m_mesh.set_anim_speed(0, 'elvl', -1.0);
        }
        */

        m_mesh.set_pos(m_pos);
        m_mesh.set_rot(m_rot);
        m_mesh.update(dt);
    }

    aircraft(): m_controls_throttle(0), m_controls_brake(0), m_thrust_time(0),
                m_controls_mgun(false), m_controls_rocket(false), m_controls_special(false),
                m_special_selected(false), m_rocket_bay_time(0) {}
private:
    float clamp(float value, float from, float to) { if (value < from) return from; if (value > to) return to; return value; }
    float tend(float value, float target, float speed)
    {
        const float diff = target - value;
        if (diff > speed) return value + speed;
        if (-diff > speed) return value - speed;
        return target;
    }

    fhm_mesh m_mesh;
    float m_speed;
    float m_thrust_time;
    nya_math::vec3 m_pos;
    nya_math::vec3 m_rot_speed;
    nya_math::quat m_rot;

    nya_math::vec3 m_controls_rot;
    float m_controls_throttle;
    float m_controls_brake;
    plane_params m_params;
    bool m_controls_mgun;
    bool m_controls_rocket;
    bool m_controls_special;

    bool m_special_selected;

    float m_rocket_bay_time;
};

//------------------------------------------------------------

class screen_quad
{
public:
    void init()
    {
        struct { float x,y; } verts[4];
        verts[0].x = verts[1].x = verts[1].y = verts[3].y = 1.0f;
        verts[3].x = verts[2].x = verts[0].y = verts[2].y = 0.0f;

        m_mesh.set_vertex_data(verts, sizeof(verts[0]), 4);
        m_mesh.set_vertices(0,2);
        m_mesh.set_element_type(nya_render::vbo::triangle_strip);
    }

    void draw()
    {
        m_mesh.bind();
        m_mesh.draw();
        m_mesh.unbind();
    }

private:
    nya_render::vbo m_mesh;
};

//------------------------------------------------------------

class postprocess
{
public:
    void init(const char *map_name)
    {
        m_fxaa_shader.load("shaders/fxaa.nsh");
        m_main_shader.load("shaders/post_process.nsh");

        if (!map_name)
            return;

        m_quad.init();
        std::string curve_name = std::string("Map/tonecurve_") + map_name + ".tcb";
        nya_resources::resource_data *res = nya_resources::get_resources_provider().access(curve_name.c_str());
        if (res)
        {
            assert(res->get_size() == 256 * 3);
            nya_memory::tmp_buffer_scoped buf(res->get_size());
            res->read_all(buf.get_data());
            res->release();
            const char *data = (const char *)buf.get_data();
            for (int i = 0; i < 3; ++i)
            {
                m_color_curve_tex[i].build_texture(data, 256, 1, nya_render::texture::greyscale);
                m_color_curve_tex[i].set_wrap(false, false);
                data += 256;
            }
        }
    }

    void resize(int width, int height)
    {
        m_main_target.init(width, height, true);
        m_add_target.init(width, height, false);
    }

    void begin_render()
    {
        m_main_target.bind();
    }

    void end_render()
    {
        m_main_target.unbind();
    }

    void draw()
    {
        m_add_target.bind();
        m_fxaa_shader.internal().set();
        m_main_target.color.bind(0);
        m_quad.draw();
        m_add_target.unbind();

        m_add_target.color.bind(0);
        m_main_shader.internal().set();

        for (int i = 0; i < 3; ++i)
            m_color_curve_tex[i].bind(i + 1);

        m_quad.draw();

        for (int i = 0; i < 4; ++i)
            nya_render::texture::unbind(i);
    }

private:
    struct target
    {
        void init(int w, int h, bool has_depth)
        {
            release();
            if (w <= 0 || h <= 0)
                return;

            width = w, height = h;

            color.build_texture(0, width, height, nya_render::texture::color_rgb);
            fbo.set_color_target(color);

            if (has_depth)
            {
                depth.build_texture(0, width, height, nya_render::texture::depth32);
                fbo.set_depth_target(depth);
            }
        }

        bool is_valid() { return width > 0 && height > 0; }

        void release()
        {
            if (!is_valid())
                return;

            color.release();
            depth.release();
            fbo.release();

            width = height = 0;
        }

        void bind()
        {
            was_set = true;
            fbo.bind();
            restore_rect = nya_render::get_viewport();
            nya_render::set_viewport(0, 0, width, height);
        }

        void unbind()
        {
            if (!was_set)
                return;

            was_set = false;
            fbo.unbind();
            nya_render::set_viewport(restore_rect);
        }

        nya_render::fbo fbo;
        nya_render::texture color;
        nya_render::texture depth;
        int width;
        int height;
        bool was_set;
        nya_render::rect restore_rect;

        target(): width(0), height(0), was_set(false) {}
    };

    target m_main_target;
    target m_add_target;
    screen_quad m_quad;
    nya_render::texture m_color_curve_tex[3];
    nya_scene::shader m_main_shader;
    nya_scene::shader m_fxaa_shader;
};

//------------------------------------------------------------

nya_memory::tmp_buffer_ref load_resource(const char *name)
{
    nya_resources::resource_data *res = nya_resources::get_resources_provider().access(name);
    if (!res)
        return nya_memory::tmp_buffer_ref();

    nya_memory::tmp_buffer_ref buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();

    return buf;
}

//------------------------------------------------------------

void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0);

//------------------------------------------------------------

class effect_clouds
{
public:
    void load(const char *location_name)
    {
        nya_memory::tmp_buffer_ref res = load_resource((std::string("Effect/") + location_name + "/CloudPosition.BDD").c_str());
        assert(res.get_size() > 0);
        {
            nya_memory::memory_reader reader(res.get_data(), res.get_size());
            bdd_header header = reader.read<bdd_header>();
/*
            for (int i = 0;i < header.type1_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p, nya_math::vec4(1,0,0,1));
            }

            for (int i = 0; i < header.type2_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p,nya_math::vec4(0,1,0,1));
            }

            for (int i = 0;i < header.type3_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p, nya_math::vec4(1,1,0,1));
            }
*/
            //print_data(reader, 0, reader.get_remained(), 0, "CloudPosition.txt");
            res.free();
        }

        res = load_resource((std::string("Effect/") + location_name + "/cloud_" + location_name + ".BDD").c_str());
        assert(res.get_size() > 0);
        {
            nya_memory::memory_reader reader(res.get_data(), res.get_size());
            bdd_header header = reader.read<bdd_header>();
/*
            for (int i = 0; i < header.type1_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p, nya_math::vec4(1,0,1,1));
            }

            for (int i = 0; i < header.type2_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p, nya_math::vec4(0,1,1,1));
            }

            for (int i = 0; i < header.type3_count; ++i)
            {
                nya_math::vec3 p;
                p.x = reader.read<float>();
                p.y = 10.0;
                p.z = reader.read<float>();

                test.add_point(p, nya_math::vec4(1,1,1,1));
            }
*/
            //print_data(reader, 0, reader.get_remained(), 0, "cloud.txt");
            res.free();
        }
/*
        for (int i = 1; i <= 4; ++i)
        {
            for (char j = 'A'; j <= 'E'; ++j)
            {
                char buf[512];
                sprintf(buf, "Effect/%s/ObjCloud/Level%d_%c.BOC", location_name, i, j);

                res = load_resource(buf);
                assert(res.get_size() > 0);
                nya_memory::memory_reader reader(res.get_data(), res.get_size());
                level_header header = reader.read<level_header>();
                for (int k = 0; k < header.count; ++k)
                {
                    level_entry entry = reader.read<level_entry>();
                    nya_math::vec3 p(entry.cood[0], header.unknown, entry.cood[1]);
                    nya_math::vec3 p2(entry.cood[2], header.unknown, entry.cood[3]);
                    test.add_line(p, p2);
                }
                //print_data(reader, 0, reader.get_remained(), 0, "Level1_A.txt");
                res.free();
            }
        }
*/
        m_shader.load("shaders/clouds.nsh");
        m_obj_tex = shared::get_texture(shared::load_texture((std::string("Effect/") + location_name + "/ObjCloud.nut").c_str()));
        m_flat_tex = shared::get_texture(shared::load_texture((std::string("Effect/") + location_name + "/FlatCloud.nut").c_str()));
    }

    void draw()
    {
    }

private:
    nya_scene::shader m_shader;
    nya_scene::texture m_obj_tex;
    nya_scene::texture m_flat_tex;

private:
    typedef unsigned int uint;
    struct bdd_header
    {
        char sign[4];
        uint unknown;
        float unknown2[2];

        uint type1_count;
        float type1_params[5];

        uint type2_count;
        float type2_params[3];

        uint type3_count;
        float type3_params[3];

        float params[20];
        uint zero;
    };

    struct level_header
    {
        uint count;
        float unknown;
        uint zero;
    };

    struct level_entry
    {
        float cood[4];
        float tc[6];
        uint zero;
    };
};

//------------------------------------------------------------

int main(void)
{
    std::string plane_name = "f22a"; //b02a pkfa su25 su33 su34 su35 f22a kwmr
    std::string location_name = "ms01"; //ms01 ms50 ms10

#ifndef _WIN32
    chdir(nya_system::get_app_path());
#endif

    qdf_resources_provider qdfp;
    if (!qdfp.open_archive("datafile.qdf"))
    {
        printf("unable to open datafile.qdf");
        return 0;
    }

    nya_resources::set_resources_provider(&qdfp);

    class target_resource_provider: public nya_resources::resources_provider
    {
        nya_resources::resources_provider &m_provider;

    public:
        target_resource_provider(nya_resources::resources_provider &provider): m_provider(provider) {}

    private:
        nya_resources::resource_data *access(const char *resource_name)
        {
            if (!resource_name)
                return 0;

            const std::string str(resource_name);
            if (m_provider.has(("target/" + str).c_str()))
                return m_provider.access(("target/" + str).c_str());

            if (m_provider.has(("common/" + str).c_str()))
                return m_provider.access(("common/" + str).c_str());

            static nya_resources::file_resources_provider fprov;
            static bool dont_care = fprov.set_folder(nya_system::get_app_path());

            return fprov.access(resource_name);
        }

        bool has(const char *resource_name)
        {
            if (!resource_name)
                return false;

            const std::string str(resource_name);
            return m_provider.has(("target/" + str).c_str()) || m_provider.has(("common/" + str).c_str()) || m_provider.has(resource_name);
        }
    } trp(nya_resources::get_resources_provider());

    nya_resources::set_resources_provider(&trp);

    GLFWwindow* window;
    if (!glfwInit())
        return -1;

    window = glfwCreateWindow(1000, 600, "open horizon", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    for (int i = 0; glfwJoystickPresent(i); ++i)
    {
        const char *name = glfwGetJoystickName(i);

        int axis_count = 0, buttons_count = 0;
        glfwGetJoystickAxes(i, &axis_count);
        glfwGetJoystickButtons(i, &buttons_count);
        printf("joy%d: %s %d axis %d buttons\n", i, name, axis_count, buttons_count);
    }

    glfwMakeContextCurrent(window);

    aircraft player_plane;
    player_plane.load(plane_name.c_str());
    player_plane.set_pos(nya_math::vec3(-300, 50, 2000));

    nya_render::set_clear_color(0.4, 0.5, 0.9, 1.0);

    fhm_location location;
    location.load((std::string("Map/") + location_name + ".fhm").c_str());
    location.load((std::string("Map/") + location_name + "_mpt.fhm").c_str());

    plane_camera camera;
    //camera.add_delta_rot(0.1f,0.0f);
    if (plane_name[0] == 'b')
        camera.add_delta_pos(0.0f, -2.0f, -20.0f);

    //nya_render::debug_draw test;
    test.set_point_size(5);
    //test.add_point(nya_math::vec3(0.0, 0.0, 0.0),nya_math::vec4(1.0, 0.0, 0.0, 1.0));
    nya_math::aabb aabb;
//    aabb.delta = nya_math::vec3(0.5, 0.3, 0.2);
  //  test.add_aabb(aabb,nya_math::vec4(0.0, 0.0, 1.0, 1.0));

    double mx,my;
    glfwGetCursorPos(window, &mx, &my);

    int frame_counter = 0;
    int frame_counter_time = 0;
    int fps = 0;

    screen_quad sq;
    sq.init();

    nya_scene::shader sky_shader;
    sky_shader.load("shaders/sky.nsh");
    nya_scene::texture envmap = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + location_name + ".nut").c_str()));
    //envmap_mapparts_

    nya_scene::shader land_shader;
    land_shader.load("shaders/land.nsh");

    effect_clouds clouds;
    clouds.load(location_name.c_str());

    postprocess pp;
    pp.init(location_name.c_str());

    int screen_width = 0, screen_height = 0;
    unsigned long app_time = nya_system::get_time();
    while (!glfwWindowShouldClose(window))
    {
        unsigned long time = nya_system::get_time();
        const int dt = int(time - app_time);
        frame_counter_time += dt;
        ++frame_counter;
        if (frame_counter_time > 1000)
        {
            fps = frame_counter;
            frame_counter = 0;
            frame_counter_time -= 1000;
        }
        app_time = time;

        static bool paused = false;
        static bool speed10x = false;
        if (!paused)
            player_plane.update(speed10x ? dt * 10 : dt);

        char title[255];
        sprintf(title,"speed: %7d alt: %7d \t fps: %3d  %s", int(player_plane.get_speed()), int(player_plane.get_alt()), fps, paused ? "paused" : "");
        glfwSetWindowTitle(window,title);

        int new_screen_width, new_screen_height;
        glfwGetFramebufferSize(window, &new_screen_width, &new_screen_height);
        if (new_screen_width != screen_width || new_screen_height != screen_height)
        {
            screen_width = new_screen_width, screen_height = new_screen_height;
            pp.resize(screen_width, screen_height);
            camera.set_aspect(screen_height > 0 ? float(screen_width) / screen_height : 1.0f);
            nya_render::set_viewport(0, 0, screen_width, screen_height);
        }

        camera.set_pos(player_plane.get_pos());
        camera.set_rot(player_plane.get_rot());

        nya_render::clear(true, true);

        pp.begin_render();

        nya_render::clear(true, true);

        //nya_render::set_color(1,1,1,1);

        nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

        nya_render::depth_test::enable(nya_render::depth_test::not_greater);
        nya_render::cull_face::enable(nya_render::cull_face::ccw);

        land_shader.internal().set();
        location.draw_landscape();
        land_shader.internal().unset();

        //test.draw();

        nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

        location.draw_mptx();

        envmap.internal().set();
        sky_shader.internal().set();
        sq.draw();
        sky_shader.internal().unset();
        envmap.internal().unset();

        player_plane.draw();

        clouds.draw();

        pp.end_render();
        pp.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        if (glfwGetMouseButton(window, 0))
            camera.add_delta_rot((y - my) * 0.03, (x - mx) * 0.03);
        //else
          //  camera.reset_delta_rot();

        if (glfwGetMouseButton(window, 1))
            camera.add_delta_pos(0, 0, my - y);

        mx = x; my = y;

        nya_math::vec3 c_rot;
        float c_throttle = 0.0f, c_brake = 0.0f;

        int axis_count = 0, buttons_count = 0;
        const float *axis = glfwGetJoystickAxes(0, &axis_count);
        const unsigned char *buttons = glfwGetJoystickButtons(0, &buttons_count);
        const float joy_dead_zone = 0.1f;
        if (axis_count > 1)
        {
            if (fabsf(axis[0]) > joy_dead_zone) c_rot.z = axis[0];
            if (fabsf(axis[1]) > joy_dead_zone) c_rot.x = axis[1];
        }

        if (axis_count > 3)
        {
            if (fabsf(axis[2]) > joy_dead_zone) camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
            if (fabsf(axis[3]) > joy_dead_zone) camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
        }

        if (buttons_count > 11)
        {
            if (buttons[8]) c_rot.y = -1.0f;
            if (buttons[9]) c_rot.y = 1.0f;
            if (buttons[10]) c_brake = 1.0f;
            if (buttons[11]) c_throttle = 1.0f;

            if (buttons[2]) camera.reset_delta_rot();

            static bool last_btn3 = false;
            if (buttons[3] !=0 && !last_btn3)
                paused = !paused;

            last_btn3 = buttons[3] != 0;
        }

        if (glfwGetKey(window, GLFW_KEY_W)) c_throttle = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S)) c_brake = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A)) c_rot.y = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_D)) c_rot.y = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_UP)) c_rot.x = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_DOWN)) c_rot.x = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT)) c_rot.z = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) c_rot.z = 1.0f;

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) player_plane.fire_mgun();

        static bool last_control_rocket = false, last_control_special = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE))
        {
            if (!last_control_rocket)
                player_plane.fire_rocket();
            last_control_rocket = true;
        }
        else
            last_control_rocket = false;

        if (glfwGetKey(window, GLFW_KEY_Q))
        {
            if (!last_control_special)
                player_plane.change_weapon();
            last_control_special = true;
        }
        else
            last_control_special = false;

        //if (glfwGetKey(window, GLFW_KEY_L)) location.load((std::string("Map/") + location_name + ".fhm").c_str());

        static bool last_btn_p = false;
        if (glfwGetKey(window, GLFW_KEY_P) && !last_btn_p)
            paused = !paused;

        last_btn_p = glfwGetKey(window, GLFW_KEY_P);

        speed10x = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);

        player_plane.set_controls(c_rot, c_throttle, c_brake);
    }

    glfwTerminate();
    return 0;
}

//------------------------------------------------------------
