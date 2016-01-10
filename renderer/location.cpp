//
// open horizon -- undefined_darkness@outlook.com
//

#include "location.h"
#include "scene/camera.h"
#include "shared.h"
#include "math/constants.h"

namespace renderer
{
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
        for (int r = 0; r < rings; r++) for (int s = 0; s < sectors; s++)
        {
            float sr = radius * sin( nya_math::constants::pi * r * ir );
            v->x = cos(2*nya_math::constants::pi * s * is) * sr;
            v->y = radius * sin( -nya_math::constants::pi_2 + nya_math::constants::pi * r * ir );
            v->z = sin(2*nya_math::constants::pi * s * is) * sr;
            ++v;
        }

        indices.resize(vertices.size()*2);
        auto i = indices.begin();
        for (int r = 0; r < rings-1; r++) for (int s = 0; s < sectors-1; s++)
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
    solid_sphere s(20000.0,32,24);

    m_mesh.set_vertex_data(&s.vertices[0], sizeof(float)*3, (unsigned int)s.vertices.size());
    m_mesh.set_index_data(&s.indices[0], nya_render::vbo::index2b, (unsigned int)s.indices.size());

    auto env = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + name + ".nut").c_str()));
    auto dithering = shared::get_texture(shared::load_texture("PostProcess/dithering.nut"));

    m_material.get_default_pass().set_shader(nya_scene::shader("shaders/sky.nsh"));
    m_material.get_default_pass().get_state().zwrite = false;
    m_material.set_texture("diffuse", env);
    m_material.set_texture("dithering", dithering);

    nya_math::vec3 about_fog_color = params.sky.low.ambient * params.sky.low.skysphere_intensity; //ToDo
    m_material.set_param(m_material.get_param_idx("fog color"), about_fog_color);

    return true;
}

//------------------------------------------------------------

void sky_mesh::draw()
{
    nya_render::cull_face::disable();
    nya_scene::camera sky_cam=nya_scene::get_camera();
    sky_cam.set_pos(0, sky_cam.get_pos().y, 0);
    nya_render::set_modelview_matrix(sky_cam.get_view_matrix());

    m_material.internal().set();
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------

void sky_mesh::release()
{
    m_mesh.release();
    m_material.unload();
}

//------------------------------------------------------------

bool sun_mesh::init()
{
    auto texture = shared::get_texture(shared::load_texture("Map/sun.nut"));
    auto tex_data = texture.internal().get_shared_data();
    if (!tex_data.is_valid())
        return false;

    nya_render::texture tex = tex_data->tex;
    tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    auto &pass = m_material.get_default_pass();
    pass.set_shader(nya_scene::shader("shaders/sun.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    pass.get_state().zwrite=true;
    pass.get_state().depth_test=true;
    pass.get_state().depth_comparsion=nya_render::depth_test::not_greater;
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
    if (dir.dot(m_dir.xyz()) < 0.0)
        return;

    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_material.internal().set();
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------

bool location::load(const char *name)
{
    m_trees = fhm_mesh();

    if (!name || !name[0])
    {
        m_params = location_params();
        m_location.m_map_parts_material.set_texture("reflection", nya_scene::texture_proxy());
        m_location.m_map_parts_material.set_texture("detail", nya_scene::texture_proxy());
        m_location.m_map_parts_material.set_texture("ocean", nya_scene::texture_proxy());
        m_location.m_map_parts_material.set_texture("normal", nya_scene::texture_proxy());
        m_sky.release();
        return false;
    }
    else if (strcmp(name, "def") == 0)
    {
        m_params = location_params();

        auto e = shared::get_texture(shared::load_texture("Map/envmap_def.nut"));
        m_location.m_map_parts_material.set_texture("reflection", e);
    }
    else
    {
        m_params.load((std::string("Map/mapset_") + name + ".bin").c_str());

        m_location.load((std::string("Map/") + name + ".fhm").c_str(), m_params);
        m_location.load((std::string("Map/") + name + "_mpt.fhm").c_str(), m_params);
        m_trees.load((std::string("Map/") + name + "_tree_nut.fhm").c_str());
        m_trees.load((std::string("Map/") + name + "_tree_nud.fhm").c_str());

        auto e = shared::get_texture(shared::load_texture((std::string("Map/envmap_mapparts_") + name + ".nut").c_str()));
        m_location.m_map_parts_material.set_texture("reflection", e);

        auto &t = m_location.get_trees_texture();
        if (t.is_valid() && t->get_height() > 0)
        {
            m_tree_depth.build(0, t->get_width(), t->get_height(), nya_render::texture::depth16);
            m_tree_fbo.set_color_target(t->internal().get_shared_data()->tex);
            m_tree_fbo.set_depth_target(m_tree_depth.internal().get_shared_data()->tex);
        }
    }

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
    m_location.draw_mptx();
    m_location.draw_trees();
    m_location.draw_landscape();
    m_location.draw_mptx_transparent();

    m_sky.draw();
    m_sun.draw();

    //nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    //m_location.draw_cols();
}

//------------------------------------------------------------

void location::update_tree_texture()
{
    if (!m_location.get_trees_texture().is_valid())
        return;

    const auto width = m_location.get_trees_texture()->get_width();
    const auto height = m_location.get_trees_texture()->get_height();
    if (!height)
        return;

    auto prev_cam = nya_scene::get_camera_proxy();

    nya_render::state_override s; //ToDo: fix tree material instead
    s.override_cull_face = true;
    s.cull_face = false;
    auto prev_s = nya_render::get_state_override();
    nya_render::set_state_override(s);

    nya_scene::camera_proxy tree_cam;
    tree_cam.create();
    nya_scene::set_camera(tree_cam);
    float size = 0.5f;
    nya_math::mat4 pm;
    pm.ortho(-size, size, -size, size, -size, size);
    pm.scale(1.99);
    tree_cam->set_proj(pm);
    tree_cam->set_pos(0.0, size, 0.0);

    auto r = prev_cam->get_rot().get_euler();
    tree_cam->set_rot(nya_math::quat(-r.x, -r.y, 0.0f));

    m_tree_fbo.bind();
    const auto prev_vp = nya_render::get_viewport();
    nya_render::set_viewport(0, 0, width, height);
    nya_render::clear(true, true);
    for (int i = 0; i < width / height; ++i)
    {
        nya_render::set_viewport(height * i, 0, height, height);
        nya_render::scissor::enable(height * i, 0, height, height);

        m_trees.draw(i);
    }
    nya_render::scissor::disable();

    nya_render::set_state_override(prev_s);

    //nya_render::set_clear_color(0.0, 0.0, 0.0, 0.0);
    nya_render::set_viewport(prev_vp);
    nya_scene::set_camera(prev_cam);
    m_tree_fbo.unbind();
}

//------------------------------------------------------------
}