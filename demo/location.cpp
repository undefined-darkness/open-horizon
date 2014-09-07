//
// open horizon -- undefined_darkness@outlook.com
//

#include "location.h"
#include "scene/camera.h"
#include "shared.h"

//------------------------------------------------------------

bool sky_mesh::load(const char *name)
{
    m_sq.init();
    m_sky_shader.load("shaders/sky.nsh");
    m_envmap = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + name + ".nut").c_str()));
    return true;
}

//------------------------------------------------------------

void sky_mesh::draw()
{
    m_envmap.internal().set();
    m_sky_shader.internal().set();
    m_sq.draw();
    m_sky_shader.internal().unset();
    m_envmap.internal().unset();
}

//------------------------------------------------------------

bool location::load(const char *name)
{
    m_location.load((std::string("Map/") + name + ".fhm").c_str());
    m_location.load((std::string("Map/") + name + "_mpt.fhm").c_str());

    auto t = shared::get_texture(shared::load_texture((std::string("Map/detail_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("detail", t);
    auto t2 = shared::get_texture(shared::load_texture((std::string("Map/ocean_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("ocean", t2);
    auto t3 = shared::get_texture(shared::load_texture((std::string("Map/ocean_nrm_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("normal", t3);

    m_sky.load(name);

   return true;
}

//------------------------------------------------------------

void location::draw(int dt)
{
    nya_render::depth_test::enable(nya_render::depth_test::not_greater);
    nya_render::cull_face::enable(nya_render::cull_face::ccw);

    m_sky.draw();
    m_location.draw_landscape(dt);
    m_location.draw_mptx();

}

//------------------------------------------------------------
