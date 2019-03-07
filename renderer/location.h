//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_location.h"
#include "fhm_mesh.h"
#include "location_params.h"
#include "render/fbo.h"
#include "sky.h"

namespace renderer
{
//------------------------------------------------------------

class location
{
public:
    bool load(const char *name);
    void update(int dt);
    void update_tree_texture();
    void draw();
    void release();

public:
    const location_params &get_params() const { return m_params; }
    const nya_scene::texture &get_ibl() const { return m_ibl; }
    const nya_scene::texture &get_env() const { return m_env; }

private:
    fhm_location m_location;
    location_params m_params;
    sky_mesh m_sky;
    sun_mesh m_sun;
    fhm_mesh m_trees;
    nya_render::fbo m_tree_fbo;
    nya_scene::texture m_tree_depth;
    nya_scene::texture m_ibl, m_env;
};

//------------------------------------------------------------
}
