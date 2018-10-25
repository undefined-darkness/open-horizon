//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/vbo.h"

namespace renderer
{
//------------------------------------------------------------

class plane_trail
{
    friend class particles_render;

public:
    void update(const nya_math::vec3 &pos, int dt);
    void clear() { m_trail_params.tr.set_count(0), m_trail_params.dir.set_count(0); }
    bool is_full();

private:
    struct param { nya_scene::material::param_array tr, dir; float len = 0.0f; };
    param m_trail_params;
};

//------------------------------------------------------------

class fire_trail
{
    friend class particles_render;

public:
    void update(const nya_math::vec3 &pos, int dt);

    fire_trail() {}
    fire_trail(float r): m_radius(r), m_time(0.0f), m_offset(0), m_count(0) {}

private:
    static const int max_count = 20;
    nya_math::vec4 m_pos[max_count];
    int m_tci[max_count];
    float m_rot[max_count];
    int m_tcia[max_count];
    int m_offset, m_count;
    float m_radius;
    float m_time;
};

//------------------------------------------------------------

class muzzle_flash
{
    friend class particles_render;

public:
    void update(const nya_math::vec3 &pos, const nya_math::vec3 &dir, int dt);

private:
    nya_math::vec3 m_pos;
    nya_math::vec3 m_dir;
    int m_time = 0;
};

//------------------------------------------------------------

class explosion
{
    friend class particles_render;

public:
    void update(int dt);
    const nya_math::vec3 &get_pos() const { return m_pos; }
    bool is_finished() const;

    explosion(): m_radius(0) {}
    explosion(const nya_math::vec3 &pos, float r);

private:
    nya_math::vec3 m_pos;
    float m_radius;
    float m_time;
    std::vector<float> m_shrapnel_rots;
    std::vector<int> m_shrapnel_alpha_tc_idx;
    std::vector<float> m_shrapnel2_rots;
    std::vector<int> m_shrapnel2_alpha_tc_idx;
    std::vector<nya_math::vec3> m_fire_dirs;
    std::vector<float> m_fire_rots;
    std::vector<int> m_fire_alpha_tc_idx;
};

//------------------------------------------------------------

class plane_engine
{
    friend class particles_render;

public:
    void update(const nya_math::vec3 &pos, const nya_math::quat &rot, float afterburner, int dt);
    void update_tvc(int idx, const nya_math::vec3 &dir) { m_tvc[idx] = dir; }

    plane_engine(): m_radius(0),m_dist(0) {}
    plane_engine(float r,float d): m_radius(r),m_dist(d) { m_tvc[0].z = m_tvc[1].z = -1.0f; }

private:
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
    float m_heat_rot, m_afterburner;
    float m_radius, m_dist;
    nya_math::vec3 m_tvc[2];
};

//------------------------------------------------------------

class bullets
{
    friend class particles_render;

public:
    void clear();
    void add_bullet(const nya_math::vec3 &pos, const nya_math::vec3 &vel);

private:
    struct param { nya_scene::material::param_array tr, dir; };
    std::vector<param> m_params;
};

//------------------------------------------------------------

class particles_render
{
public:
    void init();
    void draw(const plane_trail &t) const;
    void draw(const fire_trail &t) const;
    void draw(const muzzle_flash &f) const;
    void draw(const explosion &e) const;
    void draw(const bullets &b) const;
    void draw(const plane_engine &e) const;
    void draw_heat(const explosion &e) const;
    void draw_heat(const plane_engine &e) const;
    void release();

private:
    typedef nya_math::vec4 tc;
    typedef nya_math::vec4 color;
    void clear_points() const;
    void add_point(const nya_math::vec3 &pos, float size, const tc &tc_rgb, bool rgb_from_a, const tc &tc_a, bool a_from_a,
                   const color &c, const nya_math::vec2 &off = nya_math::vec2(), float rot = 0.0f, float aspect = 1.0f) const;
    void draw_points() const;

private:
    nya_scene::material m_material;
    nya_scene::material m_trail_material;
    nya_scene::material m_bullet_material;
    nya_scene::material m_engine_stream_material;
    nya_render::vbo m_trail_mesh, m_point_mesh, m_engine_stream_mesh;
    mutable nya_scene::material::param_array_proxy m_trail_tr, m_trail_dir;
    mutable nya_scene::material::param_proxy m_trail_param;
    mutable nya_scene::material::param_array_proxy m_tr_pos, m_tr_tc_rgb, m_tr_tc_a, m_tr_off_rot_asp, m_col, m_b_dir;
    mutable nya_scene::material::param_proxy m_b_color, m_b_tc, m_b_size;
    mutable nya_scene::material::param_proxy m_es_params, m_es_tvc[2];
};

//------------------------------------------------------------
}
