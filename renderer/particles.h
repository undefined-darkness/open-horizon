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

    plane_trail();

private:
    struct param { nya_scene::material::param_array tr, dir; };
    std::vector<param> m_trail_params;
};

//------------------------------------------------------------

class plane_engine
{
    friend class particles_render;

public:
    void update(const nya_math::vec3 &pos, const nya_math::vec3 &ab, float power, int dt);

    plane_engine(): m_radius(0) {}
    plane_engine(float r): m_radius(r) {}

private:
    nya_math::vec3 m_pos;
    float m_heat_rot;
    float m_radius;
};

//------------------------------------------------------------

class particles_render
{
public:
    void init();
    void draw(const plane_trail &t) const;
    void draw(const plane_engine &e) const;
    void draw_heat(const plane_engine &e) const;
    void release();

private:
    nya_scene::material m_trail_material;
    nya_render::vbo m_trail_mesh, m_point_mesh;
    mutable nya_scene::material::param_array_proxy m_trail_tr, m_trail_dir;
};

//------------------------------------------------------------
}
