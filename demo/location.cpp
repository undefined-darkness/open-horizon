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

bool sky_mesh::load(const char *name, const location_params &params)
{
    solid_sphere s(20000.0,9,12);

    m_mesh.set_vertex_data(&s.vertices[0], sizeof(float)*3, (unsigned int)s.vertices.size());
    m_mesh.set_index_data(&s.indices[0], nya_render::vbo::index2b, (unsigned int)s.indices.size());

    m_sky_shader.load("shaders/sky.nsh");
    m_envmap = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + name + ".nut").c_str()));

    nya_math::vec3 about_fog_color = params.sky.low.ambient * params.sky.low.skysphere_intensity; //ToDo
    for (int i = 0; i < m_sky_shader.internal().get_uniforms_count(); ++i)
    {
        if (m_sky_shader.internal().get_uniform(i).name == "fog color")
            m_sky_shader.internal().set_uniform_value(i, about_fog_color.x, about_fog_color.y, about_fog_color.z, 1.0);
    }

    return true;
}

//------------------------------------------------------------

void sky_mesh::draw()
{
    nya_render::cull_face::disable();
    nya_scene::camera sky_cam=nya_scene::get_camera();
    sky_cam.set_pos(0, sky_cam.get_pos().y, 0);
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

bool sun_mesh::init()
{
    auto texture = shared::get_texture(shared::load_texture("Map/sun.nut"));
    auto tex_data = texture.internal().get_shared_data();
    if(!tex_data.is_valid())
        return false;

    nya_render::texture tex = tex_data->tex;
    tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    auto &pass = m_material.get_pass(m_material.add_pass(nya_scene::material::default_pass));
    pass.set_shader(nya_scene::shader("shaders/sun.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    pass.get_state().zwrite=false;
    pass.get_state().depth_test=false;
    m_material.set_texture("diffuse", texture);

    struct vert
    {
        nya_math::vec2 pos;
    };

    vert verts[6];
    vert *v = &verts[0];

    v[0].pos = nya_math::vec2( -1.0f, -1.0f );
    v[1].pos = nya_math::vec2( -1.0f,  1.0f );
    v[2].pos = nya_math::vec2(  1.0f,  1.0f );
    v[3].pos = nya_math::vec2( -1.0f, -1.0f );
    v[4].pos = nya_math::vec2(  1.0f,  1.0f );
    v[5].pos = nya_math::vec2(  1.0f, -1.0f );

    m_mesh.set_vertex_data(verts, uint32_t(sizeof(vert)), uint32_t(sizeof(verts) / sizeof(verts[0])));
    m_mesh.set_vertices(0, 2);

    return true;
}

//------------------------------------------------------------

void sun_mesh::apply_location(const location_params &params)
{
    m_dir.xyz() = -params.sky.sun_dir;
    m_dir.w = params.sky.low.sun_flare_size; //params.sky.sun_size;
    m_material.set_param(m_material.get_param_idx("light dir"), m_dir);
    m_material.set_param(m_material.get_param_idx("color"), nya_math::vec3(1.0,1.0,1.0)); //params.sky.low.sun_flare_rgb
}

//------------------------------------------------------------

void sun_mesh::draw() const
{
    auto dir = nya_scene::get_camera().get_dir();
    dir.z = -dir.z;
    if(dir * m_dir.xyz() < 0.0)
        return;

    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_material.internal().set(nya_scene::material::default_pass);
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------

bool location::load(const char *name)
{
    m_params.load((std::string("Map/mapset_") + name + ".bin").c_str());

    m_location.load((std::string("Map/") + name + ".fhm").c_str(), m_params);
    m_location.load((std::string("Map/") + name + "_mpt.fhm").c_str(), m_params);

    auto e = shared::get_texture(shared::load_texture((std::string("Map/envmap_mapparts_") + name + ".nut").c_str()));
    m_location.m_map_parts_material.set_texture("reflection", e);

    auto t = shared::get_texture(shared::load_texture((std::string("Map/detail_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("detail", t);
    auto t2 = shared::get_texture(shared::load_texture((std::string("Map/ocean_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("ocean", t2);
    auto t3 = shared::get_texture(shared::load_texture((std::string("Map/ocean_nrm_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("normal", t3);

    m_sky.load(name, m_params);
    m_sun.init();
    m_sun.apply_location(m_params);

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

    m_location.draw_mptx();
    m_location.draw_landscape();
    m_sky.draw();
    m_sun.draw();

    //nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    //m_location.draw_cols();
}

//------------------------------------------------------------
