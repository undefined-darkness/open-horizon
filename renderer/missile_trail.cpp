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
static const float fade_time = 5.0f;

//------------------------------------------------------------

missile_trail::missile_trail()
{
    m_trail_params.resize(1);
    m_smoke_params.resize(1);
}

//------------------------------------------------------------

bool missile_trail::is_dead() const { return (m_time - m_fade_time) > fade_time; }

//------------------------------------------------------------

void missile_trail::update(int dt)
{
    m_time += dt * 0.001f;

    if(!m_fade)
    {
        m_fade_time = m_time;
        m_fade = true;
    }
}

//------------------------------------------------------------

void missile_trail::update(const nya_math::vec3 &pos, int dt)
{
    m_time += dt * 0.001f;

    //trail

    auto &trp = m_trail_params.back();

    int curr_tr_count = trp.tr.get_count();
    if (!curr_tr_count)
    {
        trp.tr.set_count(2);
        trp.tr.set(0, pos, m_time);
        trp.tr.set(1, pos, m_time);
        trp.dir.set_count(2);
        return;
    }

    auto diff = pos - trp.tr.get(curr_tr_count - 2).xyz();
    const float diff_len = diff.length();

    const float fragment_minimal_len = 1.0f;
    if (diff_len > fragment_minimal_len)
    {
        diff /= diff_len;

        if (diff.dot(trp.dir.get(curr_tr_count - 2).xyz()) < 1.0f)
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

    m_trail_params.back().tr.set(curr_tr_count - 1, pos, m_time);
    m_trail_params.back().dir.set(curr_tr_count - 1, diff, diff_len * 0.01f + m_trail_params.back().dir.get(curr_tr_count-2).w);

    //smoke puffs

    auto &smp = m_smoke_params.back();

    const int curr_sm_count = smp.get_count();
    if (!curr_sm_count)
    {
        smp.set_count(1);
        smp.set(0, pos, 0.25 * (rand() % 3));
        return;
    }

    const float smoke_interval = 5;
    if ((smp.get(curr_sm_count - 1).xyz() - pos).length_sq() > smoke_interval * smoke_interval)
    {
        if (curr_sm_count >= max_smoke_points)
            m_smoke_params.resize(m_smoke_params.size() + 1);

        auto &smp = m_smoke_params.back();
        const int curr_sm_count = smp.get_count();

        smp.set_count(curr_sm_count + 1);
        smp.set(curr_sm_count, pos, m_time);
    }
}

//------------------------------------------------------------

void missile_trails_render::init()
{
    auto t = shared::get_texture(shared::load_texture("Effect/EffectTrinity.nut"));

    //trails

    m_trail_tr.create();
    m_trail_dir.create();
    m_param.create();

    auto &p = m_trail_material.get_default_pass();
    p.set_shader(nya_scene::shader("shaders/missile_trail.nsh"));
    p.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p.get_state().zwrite = false;
    p.get_state().cull_face = false;
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr pos"), m_trail_tr);
    m_trail_material.set_param_array(m_trail_material.get_param_idx("tr dir"), m_trail_dir);
    m_trail_material.set_param(m_trail_material.get_param_idx("tr param"), m_param);
    m_trail_material.set_texture("diffuse", t);

    std::vector<nya_math::vec2> trail_verts(max_trail_points * 2);
    for (int i = 0; i < max_trail_points; ++i)
    {
        trail_verts[i * 2].set(-1.0f, float(i));
        trail_verts[i * 2 + 1].set(1.0f, float(i));
    }

    m_trail_mesh.set_vertices(0, 2);
    m_trail_mesh.set_element_type(nya_render::vbo::triangle_strip);
    m_trail_mesh.set_vertex_data(trail_verts.data(), 2 * 4, (int)trail_verts.size());

    //smoke

    m_smoke_params.create();

    auto &p2 = m_smoke_material.get_default_pass();
    p2.set_shader(nya_scene::shader("shaders/missile_smoke.nsh"));
    p2.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p2.get_state().zwrite = false;
    p2.get_state().cull_face = false;
    m_smoke_material.set_param_array(m_smoke_material.get_param_idx("tr pos"), m_smoke_params);
    m_smoke_material.set_param(m_smoke_material.get_param_idx("tr param"), m_param);
    m_smoke_material.set_texture("diffuse", t);

    struct quad_vert { float pos[2], i, tc[2]; };
    std::vector<quad_vert> verts(max_smoke_points * 4);

    for (int i = 0, idx = 0; i < (int)verts.size(); i += 4, ++idx)
    {
        verts[i+0].pos[0] = -1.0f, verts[i+0].pos[1] = -1.0f;
        verts[i+1].pos[0] = -1.0f, verts[i+1].pos[1] =  1.0f;
        verts[i+2].pos[0] =  1.0f, verts[i+2].pos[1] =  1.0f;
        verts[i+3].pos[0] =  1.0f, verts[i+3].pos[1] = -1.0f;

        const float f = 0.25f * (rand() % 3);
        const float a = (rand() % 1000) * 0.002f * nya_math::constants::pi;
        const float sa = sinf(a), ca = cosf(a);

        for (int j = i; j < i+4; ++j)
        {
            const float x = verts[j].pos[0];
            const float y = verts[j].pos[1];

            verts[j].tc[0] = 0.5f * (x + 1.0f) * 0.25 + f;
            verts[j].tc[1] = 0.5f * (y + 1.0f) * 0.25 + 0.75;
            verts[j].pos[0] = x * ca - y * sa;
            verts[j].pos[1] = x * sa + y * ca;
            verts[j].i = float(idx);
        }
    }

    std::vector<unsigned short> indices(max_smoke_points * 6);
    for (int i = 0, v = 0; i < (int)indices.size(); i += 6, v+=4)
    {
        indices[i] = v;
        indices[i + 1] = v + 1;
        indices[i + 2] = v + 2;
        indices[i + 3] = v;
        indices[i + 4] = v + 2;
        indices[i + 5] = v + 3;
    }

    m_smoke_mesh.set_tc(0, sizeof(float) * 3, 2);
    m_smoke_mesh.set_vertex_data(verts.data(), sizeof(quad_vert), (unsigned int)verts.size());
    m_smoke_mesh.set_index_data(indices.data(), nya_render::vbo::index2b, (unsigned int)indices.size());
}

//------------------------------------------------------------

void missile_trails_render::draw(const missile_trail &t) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    //trail

    m_param->x = t.m_time;
    m_param->y = t.m_fade ? (1.0f - (t.m_time - t.m_fade_time) / fade_time) : 1.0f;

    if (!t.m_trail_params.empty())
    {
        m_trail_mesh.bind();
        for (auto &tp: t.m_trail_params)
        {
            m_trail_tr.set(tp.tr);
            m_trail_dir.set(tp.dir);
            m_trail_material.internal().set();
            m_trail_mesh.draw(tp.tr.get_count() * 2);
            m_trail_material.internal().unset();
        }
        m_trail_mesh.unbind();
    }

    //smoke

    if (!t.m_smoke_params.empty())
    {
        m_smoke_mesh.bind();
        auto dir = (t.m_smoke_params.back().get(0) - t.m_smoke_params.front().get(t.m_smoke_params.front().get_count()-1)).xyz();
        if (nya_scene::get_camera().get_dir().dot(dir) > 0)
        {
            m_param->z = 0.0f;
            m_param->w = 1.0f;
            for (auto &sp: t.m_smoke_params)
            {
                m_smoke_params.set(sp);
                m_smoke_material.internal().set();
                m_smoke_mesh.draw(sp.get_count() * 6);
                m_smoke_material.internal().unset();
            }
        }
        else
        {
            m_param->z = max_smoke_points-1.0f;
            m_param->w = -1.0f;

            for (int i = (int)t.m_smoke_params.size()-1; i >= 0; --i)
            {
                auto &sp = t.m_smoke_params[i];
                m_smoke_params.set(sp);
                m_smoke_material.internal().set();
                m_smoke_mesh.draw((max_smoke_points-sp.get_count()) * 6, sp.get_count() * 6);
                m_smoke_material.internal().unset();
            }
        }
        m_smoke_mesh.unbind();
    }
}

//------------------------------------------------------------

void missile_trails_render::apply_location(const location_params &params)
{
    m_trail_material.set_param(m_smoke_material.get_param_idx("light dir"), params.sky.sun_dir);
    m_smoke_material.set_param(m_smoke_material.get_param_idx("light dir"), params.sky.sun_dir);
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
