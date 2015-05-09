//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class lens_flare
{
public:
    bool init(const nya_scene::texture_proxy &color, const nya_scene::texture_proxy &depth);
    void apply_location(const location_params &params);
    void draw() const;

private:
    nya_scene::material m_material;
    nya_render::vbo m_mesh;
    mutable nya_scene::material::param_proxy m_dir_alpha;
    params::fvalue m_brightness;

private:
    params::fvalue f_max;
    params::fvalue f_min;
    params::fvalue ring_radius;
    params::fvalue ring_shiness;
    params::fvalue ring_specular;
    params::color3 glow_color;
    params::fvalue glow_radius;
    params::color3 star_color;
    params::fvalue star_radius;
    params::fvalue aberration;
    
    struct lens_param
    {
        params::fvalue position;
        params::fvalue radius;
        params::color3 color;
        
    } lens[16];
};

//------------------------------------------------------------
}
