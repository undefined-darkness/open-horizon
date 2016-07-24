//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"

namespace gui
{
//------------------------------------------------------------

class circle_mesh
{
public:
    void init() { init(nya_math::vec3(), 1.0f, 36, 0); }

    typedef std::function<float(float x, float z)> height_function;
    void init(nya_math::vec3 pos, float radius, int num_segments, height_function get_height)
    {
        *this = circle_mesh(); //release

        m_material.get_default_pass().set_shader("shaders/color.nsh");
        auto &s = m_material.get_default_pass().get_state();
        s.set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
        s.depth_test = s.zwrite = false;

        std::vector<nya_math::vec3> verts;

        for(int i = 0; i < num_segments; ++i)
        {
            const float a = 2.0f * nya_math::constants::pi * float(i) / float(num_segments);
            nya_math::vec3 p(radius * cosf(a), 0.0f, radius * sinf(a));
            p += pos;
            verts.push_back(p);
        }

        if (get_height)
        {
            for (auto &v: verts)
                v.y = get_height(v.x, v.z);
        }

        verts.push_back(verts.front()); //loop

        m_vbo.create();
        m_vbo->set_vertex_data(verts.data(), sizeof(verts[0]), (int)verts.size());
        m_vbo->set_element_type(nya_render::vbo::line_strip);
    }

    void set_pos(nya_math::vec3 pos) { m_transform.set_pos(pos); }
    void set_radius(float radius) { m_transform.set_scale(radius, 1.0f, radius); }
    void set_color(nya_math::vec4 color) { m_material.set_param(m_material.get_param_idx("color"), color); }

    void draw()
    {
        if (!m_vbo.is_valid())
            return;

        nya_scene::transform::set(m_transform);
        m_material.internal().set();
        m_vbo->bind();
        m_vbo->draw();
        m_vbo->unbind();
        m_material.internal().unset();
    }

    ~circle_mesh() { if (m_vbo.get_ref_count() == 1) m_vbo->release(); }

private:
    nya_scene::transform m_transform;
    nya_scene::material m_material;
    nya_scene::proxy<nya_render::vbo> m_vbo;
};

//------------------------------------------------------------
}
