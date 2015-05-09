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
public:
    void init();
    void update(const nya_math::vec3 &pos, int dt);
    void draw() const;
    void release();

private:
    nya_scene::material m_material;
    nya_render::vbo m_mesh;
    bool m_first;
    nya_math::vec3 m_prev_point;
    nya_render::debug_draw m_debug;
};

//------------------------------------------------------------
}
