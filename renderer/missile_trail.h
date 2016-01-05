//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/vbo.h"
#include "render/debug_draw.h"

namespace renderer
{
//------------------------------------------------------------

class missile_trail
{
    friend class missile_trails_render;

public:
    void update(const nya_math::vec3 &pos, int dt);

    missile_trail();

private:
    struct param { nya_scene::material::param_array tr, dir; };
    std::vector<param> m_trail_params;
    std::vector<nya_scene::material::param_array> m_smoke_params;
};

//------------------------------------------------------------

class missile_trails_render
{
public:
    void init();
    void draw(const missile_trail &t) const;
    void release();

private:
    nya_scene::material m_trail_material, m_smoke_material;
    nya_render::vbo m_trail_mesh, m_smoke_mesh;
    mutable nya_scene::material::param_array_proxy m_trail_tr, m_trail_dir, m_smoke_params;
};

//------------------------------------------------------------
}
