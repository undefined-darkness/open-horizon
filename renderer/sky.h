//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class sky_mesh
{
public:
    bool load(const char *name);
    void draw();
    void release();
    nya_math::vec3 get_fog_color();

private:
    nya_render::vbo m_mesh;
    nya_scene::material m_material;
    nya_math::vec3 m_fog_color;
};

//------------------------------------------------------------

class sun_mesh
{
public:
    bool init();
    void apply_location(const location_params &params);
    void draw() const;
    void release();

private:
    nya_scene::material m_material;
    nya_render::vbo m_mesh;
    nya_math::vec4 m_dir;
};

//------------------------------------------------------------
}
