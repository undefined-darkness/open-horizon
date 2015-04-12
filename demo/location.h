//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_location.h"
#include "location_params.h"

//------------------------------------------------------------

class sky_mesh
{
public:
    bool load(const char *name, const location_params &params);
    void draw();

private:
    nya_render::vbo m_mesh;
    nya_scene::shader m_sky_shader;
    nya_scene::texture m_envmap;
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
};

//------------------------------------------------------------
