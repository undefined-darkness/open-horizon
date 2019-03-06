//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include <functional>

namespace gui
{
//------------------------------------------------------------

class circle_mesh
{
public:
    void init() { init(nya_math::vec3(), 1.0f, 36, 0, 0.0f); }

    typedef std::function<float(float x, float z)> height_function;
    void init(nya_math::vec3 pos, float radius, int num_segments, height_function get_height, float height)
    {
        *this = circle_mesh(); //release

        m_material.get_default_pass().set_shader("shaders/color.nsh");
        auto &s = m_material.get_default_pass().get_state();
        s.set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
        s.depth_test = s.zwrite = false;

        std::vector<nya_math::vec4> verts;

        for(int i = 0; i < num_segments; ++i)
        {
            const float a = 2.0f * nya_math::constants::pi * float(i) / float(num_segments);
            nya_math::vec4 p(radius * cosf(a), 0.0f, radius * sinf(a), 1.0);
            p.xyz() += pos;
            verts.push_back(p);
        }

        if (get_height)
        {
            for (auto &v: verts)
                v.y = get_height(v.x, v.z);
        }

        verts.push_back(verts.front()); //loop

        m_line_count = (unsigned int)verts.size();

        if (height > 0.1f)
        {
            for(unsigned int i = 0; i < m_line_count; ++i)
            {
                auto p = verts[i];
                p.y += height;
                p.w = 0.0f;
                verts.push_back(p);
            }
        }

        m_vbo.create();
        m_vbo->set_vertices(0, 4);
        m_vbo->set_vertex_data(verts.data(), sizeof(verts[0]), (int)verts.size());

        if (height > 0.1f)
        {
            std::vector<unsigned short> inds(m_line_count * 2);
            for (unsigned short i = 0; i < (unsigned short)m_line_count; ++i)
                inds[i * 2] = i, inds[i * 2 + 1] = i + m_line_count;

            m_solid_count = (unsigned int)inds.size();
            m_vbo->set_index_data(inds.data(), nya_render::vbo::index2b, m_solid_count);
        }
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

        if (m_solid_count > 0)
        {
            m_vbo->bind();
            m_vbo->draw(0, m_solid_count, nya_render::vbo::triangle_strip);
        }
        else
        {
            m_vbo->bind_verts();
            m_vbo->draw(0, m_line_count, nya_render::vbo::line_strip);
        }

        m_vbo->unbind();
        m_material.internal().unset();
    }

    ~circle_mesh() { if (m_vbo.get_ref_count() == 1) m_vbo->release(); }

private:
    nya_scene::transform m_transform;
    nya_scene::material m_material;
    nya_scene::proxy<nya_render::vbo> m_vbo;
    unsigned int m_line_count = 0, m_solid_count = 0;
};

//------------------------------------------------------------
}
