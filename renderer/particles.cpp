//
// open horizon -- undefined_darkness@outlook.com
//

#include "particles.h"

#include "scene/camera.h"
#include "shared.h"
#include "math/constants.h"

namespace renderer
{

static const int max_trail_points = 240;
static const int max_points = 100;
static const int max_bullets = 240;

static const float hdr_coeff = 1.6f;

//------------------------------------------------------------

void plane_trail::update(const nya_math::vec3 &pos, int dt)
{
    int curr_tr_count = m_trail_params.tr.get_count();
    if (!curr_tr_count)
    {
        m_trail_params.tr.set_count(2);
        m_trail_params.tr.set(0, pos);
        m_trail_params.tr.set(1, pos);
        m_trail_params.dir.set_count(2);
        return;
    }

    auto diff = pos - m_trail_params.tr.get(curr_tr_count - 2).xyz();
    const float diff_len = diff.length();

    const float fragment_minimal_len = 1.0f;
    if (diff_len > fragment_minimal_len)
    {
        diff /= diff_len;

        if (diff.dot(m_trail_params.dir.get(curr_tr_count - 2).xyz()) < 1.0f)
        {
            ++curr_tr_count;
            m_trail_params.tr.set_count(curr_tr_count);
            m_trail_params.dir.set_count(curr_tr_count);
        }
    }
    else if (diff_len > 0.01f)
        diff /= diff_len;

    m_trail_params.tr.set(curr_tr_count - 1, pos);
    m_trail_params.dir.set(curr_tr_count - 1, diff, diff_len + m_trail_params.dir.get(curr_tr_count-2).w);
}

//------------------------------------------------------------

bool plane_trail::is_full()
{
    return m_trail_params.tr.get_count() >= max_trail_points;
}

//------------------------------------------------------------

void fire_trail::update(const nya_math::vec3 &pos, int dt)
{
    if (!m_count || (m_pos[(m_offset + max_count - 1) % max_count].xyz() - pos).length_sq() > m_radius * m_radius * 0.4f)
    {
        m_pos[m_offset].set(pos, random(m_radius * 0.6f, m_radius));
        m_tci[m_offset] = random(0, 31);
        m_tcia[m_offset] = random(0, 4);
        m_rot[m_offset] = random(-nya_math::constants::pi, nya_math::constants::pi);

        m_offset = (m_offset + 1) % max_count;
        if (m_count < 20)
            ++m_count;
    }

    auto kdt = dt * 0.001f;
    for (auto &p: m_pos)
        p.w += kdt * 15.0f;

    m_time += kdt;
}

//------------------------------------------------------------

void muzzle_flash::update(const nya_math::vec3 &pos, const nya_math::vec3 &dir, int dt)
{
    m_pos = pos;
    m_dir = dir;
    m_time += dt;
}

//------------------------------------------------------------

explosion::explosion(const nya_math::vec3 &pos, float r): m_pos(pos), m_radius(r), m_time(0)
{
    for (int i = 0; i < 2; ++i)
    {
        auto &rots = i == 0 ? m_shrapnel_rots : m_shrapnel2_rots;
        auto &tis = i == 0 ? m_shrapnel_alpha_tc_idx : m_shrapnel2_alpha_tc_idx;

        rots.resize(i == 0 ? random(7, 10) : random(3, 7));

        for (auto &r: rots)
            r = random(-nya_math::constants::pi, nya_math::constants::pi);

        tis.resize(rots.size());
        for (auto &t: tis)
            t = random(0, 1);
    }

    m_fire_dirs.resize(random(5, 7));
    for (auto &d: m_fire_dirs)
    {
        d.set(random(-1.0f, 1.0f), random(-1.0f, 1.0f), random(-1.0f, 1.0f));
        d.normalize();
    }

    m_fire_rots.resize(m_fire_dirs.size());
    for (auto &r: m_fire_rots)
        r = random(-nya_math::constants::pi, nya_math::constants::pi);

    m_fire_alpha_tc_idx.resize(m_fire_dirs.size());
    for (auto &t: m_fire_alpha_tc_idx)
        t = random(0, 4);
}

//------------------------------------------------------------

void explosion::update(int dt)
{
    m_time += dt * 0.001f;
}

//------------------------------------------------------------

bool explosion::is_finished() const
{
    return m_time > 5.0f;
}

//------------------------------------------------------------

void bullets::clear()
{
    m_params.clear();
}

//------------------------------------------------------------

void bullets::add_bullet(const nya_math::vec3 &pos, const nya_math::vec3 &vel)
{
    if (m_params.empty())
        m_params.resize(1);

    int count = m_params.back().tr.get_count();
    if (count >= max_bullets)
    {
        m_params.resize(m_params.size() + 1);
        count = 0;
    }

    m_params.back().tr.set_count(count + 1);
    m_params.back().tr.set(count, pos);
    m_params.back().dir.set_count(count + 1);
    m_params.back().dir.set(count, vel);
}

//------------------------------------------------------------

void particles_render::init()
{
    auto t = shared::get_texture(shared::load_texture("Effect/Effect.nut"));

    //trails

    m_trail_tr.create();
    m_trail_dir.create();

    auto &p = m_trail_material.get_default_pass();
    p.set_shader(nya_scene::shader("shaders/plane_trail.nsh"));
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

    m_trail_mesh.set_vertex_data(trail_verts.data(), 2 * 4, (int)trail_verts.size());
    m_trail_mesh.set_vertices(0, 2);
    m_trail_mesh.set_element_type(nya_render::vbo::triangle_strip);

    //points

    int pcount = max_points > max_bullets ? max_points : max_bullets;

    struct quad_vert { float pos[2], i, tc[2]; };
    std::vector<quad_vert> verts(pcount * 4);

    for (int i = 0, idx = 0; i < (int)verts.size(); i += 4, ++idx)
    {
        verts[i+0].pos[0] = -1.0f, verts[i+0].pos[1] = -1.0f;
        verts[i+1].pos[0] = -1.0f, verts[i+1].pos[1] =  1.0f;
        verts[i+2].pos[0] =  1.0f, verts[i+2].pos[1] =  1.0f;
        verts[i+3].pos[0] =  1.0f, verts[i+3].pos[1] = -1.0f;

        for (int j = 0; j < 4; ++j)
        {
            verts[i+j].tc[0] = 0.5f * (verts[i+j].pos[0] + 1.0f);
            verts[i+j].tc[1] = 0.5f * (verts[i+j].pos[1] + 1.0f);
            verts[i+j].i = float(idx);
        }
    }

    std::vector<unsigned short> indices(pcount * 6);
    for (int i = 0, v = 0; i < (int)indices.size(); i += 6, v+=4)
    {
        indices[i] = v;
        indices[i + 1] = v + 1;
        indices[i + 2] = v + 2;
        indices[i + 3] = v;
        indices[i + 4] = v + 2;
        indices[i + 5] = v + 3;
    }


    m_point_mesh.set_vertex_data(verts.data(), sizeof(quad_vert), (unsigned int)verts.size());
    m_point_mesh.set_index_data(indices.data(), nya_render::vbo::index2b, (unsigned int)indices.size());
    m_point_mesh.set_tc(0, sizeof(float) * 3, 2);

    m_tr_pos.create();
    m_tr_off_rot_asp.create();
    m_tr_tc_rgb.create();
    m_tr_tc_a.create();
    m_col.create();

    auto &p2 = m_material.get_default_pass();
    p2.set_shader(nya_scene::shader("shaders/particles.nsh"));
    p2.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    p2.get_state().zwrite = false;
    p2.get_state().cull_face = false;
    m_material.set_param_array(m_material.get_param_idx("tr pos"), m_tr_pos);
    m_material.set_param_array(m_material.get_param_idx("tr off rot asp"), m_tr_off_rot_asp);
    m_material.set_param_array(m_material.get_param_idx("tr tc_rgb"), m_tr_tc_rgb);
    m_material.set_param_array(m_material.get_param_idx("tr tc_a"), m_tr_tc_a);
    m_material.set_param_array(m_material.get_param_idx("color"), m_col);
    m_material.set_texture("diffuse", t);

    m_b_dir.create();
    m_b_tc.create();
    m_b_color.create();
    m_b_size.create();

    auto &p3 = m_bullet_material.get_default_pass();
    p3.set_shader(nya_scene::shader("shaders/bullets.nsh"));
    p3.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::one);
    p3.get_state().zwrite = false;
    p3.get_state().cull_face = false;
    m_bullet_material.set_param_array(m_bullet_material.get_param_idx("tr pos"), m_tr_pos);
    m_bullet_material.set_param_array(m_bullet_material.get_param_idx("b dir"), m_b_dir);
    m_bullet_material.set_param(m_bullet_material.get_param_idx("b color"), m_b_color);
    m_bullet_material.set_param(m_bullet_material.get_param_idx("b tc"), m_b_tc);
    m_bullet_material.set_param(m_bullet_material.get_param_idx("b size"), m_b_size);
    m_bullet_material.set_texture("diffuse", t);

}

//------------------------------------------------------------

void particles_render::draw(const plane_trail &t) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_trail_mesh.bind();

    m_trail_tr.set(t.m_trail_params.tr);
    m_trail_dir.set(t.m_trail_params.dir);
    m_trail_material.internal().set();
    m_trail_mesh.draw(t.m_trail_params.tr.get_count() * 2);
    m_trail_material.internal().unset();
    m_trail_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::draw(const fire_trail &t) const
{
    clear_points();

    for (size_t fi = 0; fi < t.m_count; ++fi)
    {
        const int i = (fi + t.m_offset) % t.max_count;

        const int tc_w_count = 16;
        //const int tc_h_count = 2;
        const int tc_idx = t.m_tci[i];//(tc_w_count * tc_h_count + 1) * nya_math::min(t.m_time / 5.0f, 1.0f);

        bool fire = fi + 7 >= t.m_count;

        color c = color(1.0f, 1.0f, 1.0f, 0.1 + 0.5f * fi / t.max_count);
        if (fire)
            c.xyz() *= hdr_coeff;

        add_point(t.m_pos[i].xyz(), t.m_pos[i].w, tc((tc_idx % tc_w_count) * 128, (fire ? 0 : 128) + (tc_idx / tc_w_count) * 128, 128, 128), !fire,
                  tc(640 + t.m_tcia[i], 1280 + 128, 128, 128), true, c, nya_math::vec2(), t.m_rot[i]);
    }

    draw_points();
}

//------------------------------------------------------------

void particles_render::draw(const muzzle_flash &f) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_b_color.set(color(250 * hdr_coeff, 92 * hdr_coeff, 70 * hdr_coeff, 128) / 255.0f);
    m_b_tc.set(tc(0 + 64 * (f.m_time / 10 % 16), 1280 + 128, 64, -128) / 2048.0f);
    m_b_size->set(0.6f, 3.0f, 0.0f, 0.0f);

    static nya_scene::material::param_array tr, dir;
    tr.set_count(1), tr.set(0, f.m_pos);
    dir.set_count(1), dir.set(0, f.m_dir);
    m_tr_pos.set(tr);
    m_b_dir.set(dir);

    m_point_mesh.bind();
    m_bullet_material.internal().set();
    m_point_mesh.draw(6);
    m_bullet_material.internal().unset();
    m_point_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::draw(const explosion &e) const
{
    clear_points();

    const float life_time = 5.0f;

    for (int i = 0; i < 2; ++i)
    {
        auto &rots = i == 0 ? e.m_shrapnel_rots : e.m_shrapnel2_rots;
        auto &tis = i == 0 ? e.m_shrapnel_alpha_tc_idx : e.m_shrapnel2_alpha_tc_idx;

        for (size_t j = 0; j < rots.size(); ++j)
        {
            auto &r = rots[j];
            auto &ti = tis[j];

            auto nt = e.m_time / life_time;

            const int tc_w_count = 16;
            const int tc_h_count = 2;
            const int tc_idx = (tc_w_count * tc_h_count + 1) * nt;

            float l = nya_math::min(e.m_time / 2.0f, 1.0f) * e.m_radius * 1.6f;

            auto ctc = tc(0, 512, 128, 128);
            ctc.x += ctc.z * (tc_idx % tc_w_count);
            ctc.y += ctc.w * (tc_idx / tc_w_count);

            auto atc = i == 0 ? tc(1920, 768, 64, 256) : tc(1664, 768, 128, 256);
            atc.x += atc.z * ti;

            auto c = color(1.0f, 1.0f, 1.0f, 1.0f);
            if (e.m_time > 1.0)
                c.w -= (e.m_time - 1.0);

            add_point(e.m_pos, l, ctc, true, atc, false, c, nya_math::vec2(0.0f, -0.9f), r, i == 0 ? 0.4f : 0.2f);
        }
        //
    }

    //fire

    for (size_t i = 0; i < e.m_fire_dirs.size(); ++i)
    {
        auto &d = e.m_fire_dirs[i];
        auto &ti = e.m_fire_alpha_tc_idx[i];

        auto nt = e.m_time / life_time;
        auto t = nya_math::min(e.m_time, 1.0f) + nt * 0.1f;
        auto p = e.m_pos + d * t * e.m_radius * 0.25f;
        auto r = e.m_radius * t * 0.75f;

        const int tc_w_count = 16;
        const int tc_h_count = 2;
        const int tc_idx = (tc_w_count * tc_h_count + 1) * nya_math::min(e.m_time / 2.0f, 1.0f);

        auto ctc = tc(0, 0, 128, 128);
        ctc.x += ctc.z * (tc_idx % tc_w_count);
        ctc.y += ctc.w * (tc_idx / tc_w_count);

        auto atc = tc(0, 1920, 128, 128);
        atc.x += atc.z * ti;

        auto c = color(hdr_coeff, hdr_coeff, hdr_coeff, 0.7f);
        if (e.m_time > 1.5)
            c.w -= (e.m_time - 1.5);

        add_point(p, r, ctc, false, atc, true, c, nya_math::vec2(), e.m_fire_rots[i]);
    }

    draw_points();
}

//------------------------------------------------------------

void particles_render::draw_heat(const explosion &e) const
{
    const float life_time = 5.0f;

    clear_points();
    auto atc = tc(1920, 1664, 128, 128);
    auto nt = e.m_time / life_time;
    auto t = nya_math::min(e.m_time, 1.0f) + nt * 0.1f;
    auto r = e.m_radius * t * 3.0f;

    add_point(e.m_pos, r, atc, false, atc, false, color(1.0f, 1.0f, 1.0f, 1.0f) * 0.2f);
    draw_points();
}

//------------------------------------------------------------

void particles_render::clear_points() const
{
    m_tr_pos->set_count(0);
    m_tr_tc_rgb->set_count(0);
    m_tr_tc_a->set_count(0);
}

//------------------------------------------------------------

void particles_render::add_point(const nya_math::vec3 &pos, float size, const tc &tc_rgb, bool rgb_from_a,
                                 const tc &tc_a, bool a_from_a, const color &c,
                                 const nya_math::vec2 &off, float rot, float aspect) const
{
    const int idx = m_tr_pos->get_count();
    m_tr_pos->set_count(idx + 1);
    m_tr_off_rot_asp->set_count(idx + 1);
    m_tr_tc_rgb->set_count(idx + 1);
    m_tr_tc_a->set_count(idx + 1);
    m_col->set_count(idx + 1);

    const float inv_tex_size = 1.0f / 2048.0f;
    m_tr_pos->set(idx, pos, size);
    m_tr_off_rot_asp->set(idx, off.x, off.y, rot, aspect);
    auto tc = tc_rgb * inv_tex_size;
    if (rgb_from_a)
        tc.w = -tc.w;
    m_tr_tc_rgb->set(idx, tc);
    tc = tc_a * inv_tex_size;
    if (a_from_a)
        tc.w = -tc.w;
    m_tr_tc_a->set(idx, tc);
    m_col->set(idx, c);
}

//------------------------------------------------------------

void particles_render::draw_points() const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_point_mesh.bind();
    m_material.internal().set();
    m_point_mesh.draw(m_tr_pos->get_count() * 6);
    m_material.internal().unset();
    m_point_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::draw(const bullets &b) const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());

    m_b_color.set(color(253 * hdr_coeff, 100 * hdr_coeff, 83 * hdr_coeff, 160) / 255.0f);
    m_b_tc.set(tc(1472, 1152, 64, 64) / 2048.0f);
    m_b_size->set(0.15f, 30.0f, 0.0f, 0.0f);

    m_point_mesh.bind();
    for (auto &p: b.m_params)
    {
        m_tr_pos.set(p.tr);
        m_b_dir.set(p.dir);
        m_bullet_material.internal().set();
        m_point_mesh.draw(p.tr.get_count() * 6);
        m_bullet_material.internal().unset();
    }
    m_point_mesh.unbind();
}

//------------------------------------------------------------

void particles_render::release()
{
    m_trail_material.unload();
    m_material.unload();
    m_trail_mesh.release();
    m_point_mesh.release();
}

//------------------------------------------------------------
}
