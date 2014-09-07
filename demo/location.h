//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_location.h"
#include "screen_quad.h"

//------------------------------------------------------------

class sky_mesh
{
public:
    bool load(const char *name);
    void draw();

private:
    screen_quad m_sq;
    nya_scene::shader m_sky_shader;
    nya_scene::texture m_envmap;
};

//------------------------------------------------------------

class location
{
public:
    bool load(const char *name);
    void draw(int dt);

private:
    fhm_location m_location;
    sky_mesh m_sky;
};

//------------------------------------------------------------
