//
// open horizon -- undefined_darkness@outlook.com
//

#include "location.h"
#include "scene/camera.h"
#include "shared.h"

//------------------------------------------------------------

struct solid_sphere
{
    std::vector<nya_math::vec3> vertices;
    std::vector<unsigned short> indices;

    solid_sphere(float radius, unsigned int rings, unsigned int sectors)
    {
        const float ir = 1.0f/(float)(rings-1);
        const float is = 1.0f/(float)(sectors-1);

        vertices.resize(rings * sectors * 3);
        auto v = vertices.begin();
        for(int r = 0; r < rings; r++) for(int s = 0; s < sectors; s++)
        {
            float sr = radius * sin( M_PI * r * ir );
            v->x = cos(2*M_PI * s * is) * sr;
            v->y = radius * sin( -M_PI_2 + M_PI * r * ir );
            v->z = sin(2*M_PI * s * is) * sr;
            ++v;
        }

        indices.resize(vertices.size()*2);
        auto i = indices.begin();
        for(int r = 0; r < rings-1; r++) for(int s = 0; s < sectors-1; s++)
        {
            *i++ = r * sectors + s;
            *i++ = r * sectors + (s+1);
            *i++ = (r+1) * sectors + (s+1);

            *i++ = r * sectors + s;
            *i++ = (r+1) * sectors + (s+1);
            *i++ = (r+1) * sectors + s;
        }
    }
};

//------------------------------------------------------------

bool sky_mesh::load(const char *name)
{
    solid_sphere s(20000.0,9,12);

    m_mesh.set_vertex_data(&s.vertices[0], sizeof(float)*3, (unsigned int)s.vertices.size());
    m_mesh.set_index_data(&s.indices[0], nya_render::vbo::index2b, (unsigned int)s.indices.size());

    m_sky_shader.load("shaders/sky.nsh");
    m_envmap = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + name + ".nut").c_str()));
    return true;
}

//------------------------------------------------------------

void sky_mesh::draw()
{
    nya_render::cull_face::disable();
    nya_scene::camera sky_cam=nya_scene::get_camera();
    //sky_cam.set_pos(0, sky_cam.get_pos().y, 0);
    sky_cam.set_pos(sky_cam.get_pos().x, 0.0, sky_cam.get_pos().z);
    nya_render::set_modelview_matrix(sky_cam.get_view_matrix());
    nya_render::zwrite::disable();

    m_envmap.internal().set();
    m_sky_shader.internal().set();
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_sky_shader.internal().unset();
    m_envmap.internal().unset();
}

//------------------------------------------------------------

bool location::load(const char *name)
{
    m_location.load((std::string("Map/") + name + ".fhm").c_str());
    m_location.load((std::string("Map/") + name + "_mpt.fhm").c_str());

    auto e = shared::get_texture(shared::load_texture((std::string("Map/envmap_mapparts_") + name + ".nut").c_str()));
    m_location.m_map_parts_material.set_texture("reflection", e);

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

void location::update(int dt)
{
    m_location.update(dt);
}

//------------------------------------------------------------

void location::draw()
{
    nya_render::depth_test::enable(nya_render::depth_test::not_greater);
    nya_render::cull_face::enable(nya_render::cull_face::ccw);

    m_sky.draw();
    m_location.draw_landscape();
    m_location.draw_mptx();

}

//------------------------------------------------------------
