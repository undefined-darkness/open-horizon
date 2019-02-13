//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/vbo.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class missile_trail
{
    friend class missile_trails_render;

public:
    void update(const nya_math::vec3 &pos, int dt);
    void update(int dt);
    bool is_dead() const;

    missile_trail();

private:
    struct param { nya_scene::material::param_array tr, dir; };
    std::vector<param> m_trail_params;
    std::vector<nya_scene::material::param_array> m_smoke_params;
    float m_time = 0.0f, m_fade_time = 0.0f;
    bool m_fade = false;
};

//------------------------------------------------------------

class missile_trails_render
{
public:
    void init();
    void draw(const missile_trail &t) const;
    void apply_location(const location_params &params);
    void release();

private:
    nya_scene::material m_trail_material, m_smoke_material;
    nya_render::vbo m_trail_mesh, m_smoke_mesh;
    mutable nya_scene::material::param_array_proxy m_trail_tr, m_trail_dir, m_smoke_params;
    mutable nya_scene::material::param_proxy m_param;
};

//------------------------------------------------------------
}
