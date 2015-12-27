//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_location.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class sky_mesh
{
public:
    bool load(const char *name, const location_params &params);
    void draw();
    void release();

private:
    nya_render::vbo m_mesh;
    nya_scene::material m_material;
};

//------------------------------------------------------------

class sun_mesh
{
public:
    bool init();
    void apply_location(const location_params &params);
    void draw() const;

private:
    nya_scene::material m_material;
    nya_render::vbo m_mesh;
    nya_math::vec4 m_dir;
};

//------------------------------------------------------------

class location
{
public:
    bool load(const char *name);
    void update(int dt);
    void draw();

public:
    const location_params &get_params() const { return m_params; }

private:
    fhm_location m_location;
    location_params m_params;
    sky_mesh m_sky;
    sun_mesh m_sun;
};

//------------------------------------------------------------
}