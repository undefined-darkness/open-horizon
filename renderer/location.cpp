//
// open horizon -- undefined_darkness@outlook.com
//

#include "location.h"
#include "scene/camera.h"
#include "extensions/zip_resources_provider.h"
#include "util/location.h"
#include "render/platform_specific_gl.h"
#include "shared.h"

namespace renderer
{
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

    m_sky.load(name);

    if (is_native_location(name))
    {
        m_params.load_native(get_native_location_provider(name), "effects.xml");
        m_location.load_native(name, m_params, m_sky.get_fog_color());

        name = "def"; //ToDo

        //ToDo
    }

    if (strcmp(name, "def") == 0)
    {
        m_params = location_params();

        auto e = shared::get_texture(shared::load_texture("Map/envmap_def.nut"));
        m_location.m_map_parts_material.set_texture("reflection", e);
    }
    else
    {
        m_params.load((std::string("Map/mapset_") + name + ".bin").c_str());

        m_location.load((std::string("Map/") + name + ".fhm").c_str(), m_params, m_sky.get_fog_color());
        m_location.load((std::string("Map/") + name + "_mpt.fhm").c_str(), m_params, m_sky.get_fog_color());
        m_trees.load((std::string("Map/") + name + "_tree_nut.fhm").c_str());
        m_trees.load((std::string("Map/") + name + "_tree_nud.fhm").c_str());

        auto e = shared::get_texture(shared::load_texture((std::string("Map/envmap_mapparts_") + name + ".nut").c_str()));
        m_location.m_map_parts_material.set_texture("reflection", e);

        auto &t = m_location.get_trees_texture();
        if (t.is_valid() && t->get_height() > 0)
        {
            m_tree_depth.build(0, t->get_width(), t->get_height(), nya_render::texture::depth16);
            auto tex = t->internal().get_shared_data()->tex;
            tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
            m_tree_fbo.set_color_target(tex);
            m_tree_fbo.set_depth_target(m_tree_depth.internal().get_shared_data()->tex);
        }
    }

    auto t = shared::get_texture(shared::load_texture((std::string("Map/detail_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("detail", t);
    auto t2 = shared::get_texture(shared::load_texture((std::string("Map/ocean_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("ocean", t2);
    auto t3 = shared::get_texture(shared::load_texture((std::string("Map/ocean_nrm_") + name + ".nut").c_str()));
    m_location.m_land_material.set_texture("normal", t3);

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
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    m_location.draw_landscape();
    glPolygonOffset(-1.0f, 1.0f);
    m_location.draw_mptx();
    m_location.draw_trees();
    glDisable(GL_POLYGON_OFFSET_FILL);
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

    static nya_scene::camera_proxy tree_cam = nya_scene::camera_proxy(nya_scene::camera());
    float size = 0.5f;
    nya_math::mat4 pm;
    pm.ortho(-size, size, -size, size, -size, size);
    pm.scale(1.99);
    tree_cam->set_proj(pm);
    tree_cam->set_pos(0.0, size, 0.0);

    auto r = prev_cam->get_rot().get_euler();
    tree_cam->set_rot(nya_math::quat(-r.x, -r.y, 0.0f));

    nya_scene::set_camera(tree_cam);

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