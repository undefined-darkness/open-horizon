//
// open horizon -- undefined_darkness@outlook.com
//

#include "missile_trail.h"

#include "scene/camera.h"
#include "shared.h"

namespace renderer
{

static const int max_trail_points = 240;
static const int max_smoke_points = 500;

//------------------------------------------------------------

missile_trail::missile_trail()
{
    m_trail_params.resize(1);
    m_smoke_params.resize(1);
}

//------------------------------------------------------------

void missile_trail::update(const nya_math::vec3 &pos, int dt)
{
    //trail

    auto &trp = m_trail_params.back();

    int curr_tr_count = trp.tr.get_count();
    if (!curr_tr_count)
    {
        trp.tr.set_count(2);
        trp.tr.set(0, pos);
        trp.tr.set(1, pos);
        return;
    }

    auto diff = pos - trp.tr.get(curr_tr_count - 2).xyz();
    const float diff_len = diff.length();

    const float fragment_minimal_len = 1.0f;
    const float fragment_minimal_angle_cos = 0.99;

    if (diff_len > fragment_minimal_len)
    {
        diff /= diff_len;

        if (diff.dot(trp.dir.get(curr_tr_count - 2).xyz()) < fragment_minimal_angle_cos)
        {
            if (curr_tr_count >= max_trail_points)
            {
                m_trail_params.resize(m_trail_params.size() + 1);

                auto &prev = m_trail_params[m_trail_params.size() - 2];
                auto &trp = m_trail_params.back();

                curr_tr_count = 2;
                trp.tr.set_count(curr_tr_count);
                trp.tr.set(0, prev.tr.get(max_trail_points-1));
                trp.dir.set_count(curr_tr_count);
                trp.dir.set(0, prev.dir.get(max_trail_points-1));
            }
            else
            {
                ++curr_tr_count;
                m_trail_params.back().tr.set_count(curr_tr_count);
                m_trail_params.back().dir.set_count(curr_tr_count);
            }
        }
    }
    else if (diff_len > 0.01f)
        diff /= diff_len;

    m_trail_params.back().tr.set(curr_tr_count - 1, pos);
    m_trail_params.back().dir.set(curr_tr_count - 1, diff, diff_len + m_trail_params.back().dir.get(curr_tr_count-2).w);

    //smoke puffs

    //ToDo
}

//------------------------------------------------------------

void missile_trails_render::init()
{
    auto t = shared::get_texture(shared::load_texture("Effect/EffectTrinity.nut"));

    //trails

    m_trail_tr.create();
    m_trail_dir.create();

    auto &p = m_trail_material.get_default_pass();
    p.set_shader(nya_scene::shader("shaders/missile_trail.nsh"));
    p.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p.get_state().zwrite = false;
    p.get_state().cull_face = false;
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr pos"), m_trail_tr);
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr dir"), m_trail_dir);
    m_trail_material.set_texture("diffuse", t);

    std::vector<nya_math::vec2> trail_verts(max_trail_points * 2);
    for (int i = 0; i < max_trail_points; ++i)
    {
        trail_verts[i * 2].set(-1.0f, float(i));
        trail_verts[i * 2 + 1].set(1.0f, float(i));
    }

    m_trail_mesh.set_vertex_data(trail_verts.data(), 2*4, (int)trail_verts.size());
    m_trail_mesh.set_vertices(0, 2);
    m_trail_mesh.set_element_type(nya_render::vbo::triangle_strip);

    //smoke

    m_smoke_params.create();

    auto &p2 = m_smoke_material.get_default_pass();
    p2.set_shader(nya_scene::shader("shaders/missile_smoke.nsh"));
    p2.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p2.get_state().zwrite = false;
    p2.get_state().cull_face = false;
    m_smoke_material.set_param_array(m_trail_material.get_param_idx("tr pos"), m_smoke_params);
    m_smoke_material.set_texture("diffuse", t);

    struct quad_vert { float pos[2], i, tc[2]; };
    std::vector<quad_vert> verts(max_smoke_points * 6); //ToDo: indices

    for (int i = 0, idx = 0; i < (int)verts.size(); i += 6, ++idx)
    {
        verts[i+0].pos[0] = -1.0f, verts[i+0].pos[1] = -1.0f;
        verts[i+1].pos[0] = -1.0f, verts[i+1].pos[1] =  1.0f;
        verts[i+2].pos[0] =  1.0f, verts[i+2].pos[1] =  1.0f;
        verts[i+3].pos[0] = -1.0f, verts[i+3].pos[1] = -1.0f;
        verts[i+4].pos[0] =  1.0f, verts[i+4].pos[1] =  1.0f;
        verts[i+5].pos[0] =  1.0f, verts[i+5].pos[1] = -1.0f;

        for (int j = 0; j < 6; ++j)
        {
            verts[i+j].tc[0] = 0.5f * (verts[i+j].pos[0] + 1.0f);
            verts[i+j].tc[1] = 0.5f * (verts[i+j].pos[1] + 1.0f);
            verts[i+j].i = float(idx);
        }
    }

    m_smoke_mesh.set_vertex_data(&verts[0], sizeof(quad_vert), (unsigned int)verts.size());
    m_smoke_mesh.set_tc(0, sizeof(float)*2, 2);
}

//------------------------------------------------------------

void missile_trails_render::draw(const missile_trail &t) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    //trail

    m_trail_mesh.bind();
    for (auto &tp: t.m_trail_params)
    {
        m_trail_tr.set(tp.tr);
        m_trail_dir.set(tp.dir);
        m_trail_material.internal().set();
        m_trail_mesh.draw(tp.tr.get_count()*2);
        m_trail_material.internal().unset();
    }
    m_trail_mesh.unbind();

    //smoke

    m_smoke_mesh.bind();
    for (auto &sp: t.m_smoke_params)
    {
        m_smoke_params.set(sp);
        m_smoke_material.internal().set();
        m_smoke_mesh.draw(sp.get_count()*6);
        m_smoke_material.internal().unset();
    }
    m_smoke_mesh.unbind();
}

//------------------------------------------------------------

void missile_trails_render::release()
{
    m_trail_material.unload();
    m_smoke_material.unload();
    m_trail_mesh.release();
    m_smoke_mesh.release();
}

//------------------------------------------------------------
}
