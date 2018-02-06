//
// open horizon -- undefined_darkness@outlook.com
//

#include "ui.h"
#include "containers/fhm.h"
#include "renderer/shared.h"
#include <algorithm>
#include "render/platform_specific_gl.h"

#ifdef min
#undef min
#endif

namespace gui
{
//------------------------------------------------------------

const int elements_per_batch = 100;

//------------------------------------------------------------

void render::init()
{
    if (m_tex.is_valid()) //already initialised
        return;

    m_tex.create();
    m_color.create();
    m_tr.create();
    m_tc_tr.create();
    m_urot.create();
    m_utr.create();

    struct vert { float x,y,s,t,i; } verts[elements_per_batch][4];

    uint16_t inds[elements_per_batch][6];

    for (int i = 0; i < elements_per_batch; ++i)
    {
        auto &v = verts[i];
        for (int j=0;j<4;++j)
        {
            v[j].x=j>1?0.0f:1.0f,v[j].y=j%2?1.0f:0.0f;
            v[j].s=j>1? 0.0f:1.0f,v[j].t=j%2?1.0f:0.0f;
            v[j].i = i;
        }

        inds[i][0] = i * 4;
        inds[i][1] = i * 4 + 1;
        inds[i][2] = i * 4 + 2;

        inds[i][3] = i * 4 + 2;
        inds[i][4] = i * 4 + 1;
        inds[i][5] = i * 4 + 3;
    }

    m_quads_mesh.set_vertex_data(verts,sizeof(verts[0][0]), elements_per_batch * 4);
    m_quads_mesh.set_vertices(0,2);
    m_quads_mesh.set_tc(0,2*4,3);
    m_quads_mesh.set_index_data(inds, nya_render::vbo::index2b, elements_per_batch * 6);

    vert line_verts[elements_per_batch];
    for (int i = 0; i < elements_per_batch; ++i)
    {
        auto &v = line_verts[i];
        v.x = v.y = 0.0f;
        v.i = i;
    }

    m_lines_mesh.set_vertex_data(line_verts, sizeof(line_verts[0]), elements_per_batch);
    m_lines_mesh.set_vertices(0, 2);
    m_lines_mesh.set_tc(0, 2*4, 3);
    m_lines_mesh.set_element_type(nya_render::vbo::lines);

    m_lines_loop_mesh.set_vertex_data(line_verts, sizeof(line_verts[0]), elements_per_batch);
    m_lines_loop_mesh.set_vertices(0, 2);
    m_lines_loop_mesh.set_tc(0, 2*4, 3);
    m_lines_loop_mesh.set_element_type(nya_render::vbo::line_strip);

    auto &pass = m_material.get_pass(m_material.add_pass(nya_scene::material::default_pass));
    pass.set_shader(nya_scene::shader("shaders/ui.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    pass.get_state().zwrite = false;
    pass.get_state().depth_test = false;
    m_material.set_texture("diffuse", m_tex);
    m_material.set_param(m_material.get_param_idx("color"), m_color);
    m_material.set_param_array(m_material.get_param_idx("transform"), m_tr);
    m_material.set_param_array(m_material.get_param_idx("tc transform"), m_tc_tr);
    m_material.set_param(m_material.get_param_idx("uniform rotate"), m_urot);
    m_material.set_param(m_material.get_param_idx("uniform transform"), m_utr);
}

//------------------------------------------------------------

void render::draw(const std::vector<rect_pair> &elements, const nya_scene::texture &tex, const nya_math::vec4 &color, const transform &t) const
{
    if (elements.empty())
        return;

    if (!tex.get_width() || !tex.get_height())
        return;

    if (!set_transform(t))
        return;

    const float itwidth = 1.0f / tex.get_width();
    const float itheight = 1.0f / tex.get_height();

    m_color.set(color);
    m_tex.set(tex);

    for (int b = 0; b < (int)elements.size();b+=elements_per_batch)
    {
        const int count=std::min((int)elements.size()-b,elements_per_batch);
        for (int i = 0; i < count; ++i)
        {
            auto &e = elements[i+b];
            m_tr->set_count(i + 1);
            m_tc_tr->set_count(i + 1);

            nya_math::vec4 pos;
            pos.z = float(e.r.w);
            pos.w = float(e.r.h);
            pos.x = e.r.x;
            pos.y = e.r.y;
            m_tr->set(i, pos);

            nya_math::vec4 tc;
            tc.z = float(e.tc.w) * itwidth;
            tc.w = float(e.tc.h) * itheight;
            tc.x = float(e.tc.x) * itwidth;
            tc.y = float(e.tc.y) * itheight;
            m_tc_tr->set(i, tc);
        }

        m_material.internal().set(nya_scene::material::default_pass);
        m_quads_mesh.bind();
        m_quads_mesh.draw((unsigned int)count * 6);
        m_quads_mesh.unbind();
        m_material.internal().unset();
    }

    static nya_scene::texture empty;
    m_tex.set(empty);
}

//------------------------------------------------------------

void render::draw(const std::vector<nya_math::vec2> &elements, const nya_math::vec4 &color, const transform &t, bool loop) const
{
    if (elements.empty())
        return;

    if (!set_transform(t))
        return;

    m_tex.set(shared::get_white_texture());
    m_color.set(color);

    glLineWidth(1.0);
    for (int b = 0; b < (int)elements.size();b+=elements_per_batch)
    {
        const int count=std::min((int)elements.size()-b,elements_per_batch);
        for (int i = 0; i < count; ++i)
        {
            auto &e = elements[i];
            m_tr->set_count(i + 1);
            m_tc_tr->set_count(i + 1);

            nya_math::vec4 pos;
            pos.x = e.x;
            pos.y = e.y;
            m_tr->set(i, pos);
        }

        m_material.internal().set(nya_scene::material::default_pass);
        if (loop)
        {
            m_lines_loop_mesh.bind();
            m_lines_loop_mesh.draw((unsigned int)count);
            m_lines_loop_mesh.unbind();
        }
        else
        {
            m_lines_mesh.bind();
            m_lines_mesh.draw((unsigned int)count);
            m_lines_mesh.unbind();
        }
        m_material.internal().unset();
    }

    static nya_scene::texture empty;
    m_tex.set(empty);
}

//------------------------------------------------------------

bool render::set_transform(const transform &t) const
{
    if (!m_width || !m_height)
        return false;

    if (!m_urot.is_valid()) //not initialised
        return false;

    m_urot->set(cosf(t.yaw), -sinf(t.yaw), sinf(t.yaw), cosf(t.yaw));

    const float aspect = float(m_width) / m_height / (float(get_width()) / get_height() );
    const float iwidth = 1.0f / get_width();
    const float iheight = 1.0f / get_height();

    nya_math::vec4 utr(t.x * iwidth, t.y * iheight, t.sx * iwidth, t.sy * iheight);
    if (aspect > 1.0)
    {
        utr.x = (utr.x - 0.5f) / aspect + 0.5f;
        utr.z /= aspect;
    }
    else
    {
        utr.y = (utr.y - 0.5f) * aspect + 0.5f;
        utr.w *= aspect;
    }
    m_utr.set(utr);

    return true;
}

//------------------------------------------------------------

inline float fixed_to_float(uint16_t half) { return (((int8_t *)&half)[1] + ((uint8_t *)&half)[0] / 256.0f) * 4.0f; }

//------------------------------------------------------------

bool fonts::load(const char *name)
{
    fhm_file m;
    if (!m.open(name))
        return false;

    bool need_load_tex = false;
    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);
        if (t == 'RXTN')
        {
            if (m.get_chunk_size(i) <= 96)
                continue;

            if (!need_load_tex)
                continue;

            need_load_tex = false;
            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            m_font_texture = shared::get_texture(shared::load_texture(b.get_data(), b.get_size()));
            continue;
        }

        if (t == '\0FCA')
        {
            need_load_tex = true;
            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            nya_memory::memory_reader reader(b.get_data(),b.get_size());

            struct acf_header
            {
                char sign[4];
                uint32_t uknown;
                uint32_t uknown2;
                uint16_t unknown3_1;
                uint8_t unknown4_1;
                uint8_t fonts_count;
            };

            auto header = reader.read<acf_header>();

            std::vector<uint32_t> offsets(header.fonts_count);
            for (auto &o:offsets)
                o = reader.read<uint32_t>();

            m_fonts.resize(header.fonts_count);
            for (uint8_t i = 0; i < header.fonts_count; ++i)
            {
                reader.seek(offsets[i]);
                nya_memory::memory_reader r(reader.get_data(),reader.get_remained());
                auto &f = m_fonts[i];
                assert(r.get_data());
                f.name.assign((char *)r.get_data());
                r.seek(32);

                auto header = r.read<acf_font_header>();
                r.skip(1024);

                f.height=fixed_to_float(header.height);

                f.unknown[0]=fixed_to_float(header.unknown[0]);
                f.unknown[1]=fixed_to_float(header.unknown[1]);
                f.unknown[2]=fixed_to_float(header.unknown[2]);
                f.unknown[3]=fixed_to_float(header.unknown2[0]);
                f.unknown[4]=fixed_to_float(header.unknown2[1]);
                f.unknown[5]=fixed_to_float(header.unknown2[2]);
                f.unknown[6]=fixed_to_float(header.unknown2[3]);

                for (uint32_t j = 0; j < header.char_count; ++j)
                {
                    const auto c = r.read<acf_char>();

                    wchar_t char_code = swap_bytes(c.char_code);
                    auto &fc = f.chars[char_code];
                    fc.x = c.x, fc.y = c.y;
                    fc.w = c.width, fc.h = c.height;
                    fc.yoffset = fixed_to_float(c.yoffset);
                    fc.xadvance = fixed_to_float(c.xadvance);
                    fc.unknown = fixed_to_float(c.unknown3[0]);
                    fc.unknown2 = fixed_to_float(c.unknown3[1]);
                }

                for (uint32_t j = 0; j < header.kern_count; ++j)
                {
                    kern_key k;
                    k.c[0] = swap_bytes(r.read<uint16_t>());
                    k.c[1] = swap_bytes(r.read<uint16_t>());
                    f.kerning[k.key] = fixed_to_float(r.read<uint16_t>());
                }
            }

            continue;
        }
 
        continue;

        char *c = (char *)&t;
        printf("%c%c%c%c\n",c[3],c[2],c[1],c[0]);

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader r(b.get_data(),b.get_size());
        print_data(r);
    }

    m.close();
    return true;
}

//------------------------------------------------------------

int fonts::draw_text(const render &r, const wchar_t *text, const char *font_name, int x, int y, const nya_math::vec4 &color) const
{
    if(!text || !font_name)
        return 0;

    const auto f = std::find_if(m_fonts.begin(), m_fonts.end(), [font_name](const font &fnt) { return fnt.name == font_name; });
    if (f == m_fonts.end())
        return 0;

    int width = 0;
    std::vector<rect_pair> elements;
    for (const wchar_t *c = text; *c; ++c)
    {
        rect_pair e;
        auto fc = f->chars.find(*c);
        if (fc == f->chars.end())
            continue;

        e.r.w = fc->second.w;
        e.r.h = fc->second.h;
        e.r.x = x + width;
        e.r.y = y - fc->second.yoffset + f->height;

        e.tc.w = fc->second.w;
        e.tc.h = fc->second.h;
        e.tc.x = fc->second.x;
        e.tc.y = fc->second.y;

        width += fc->second.xadvance;

        kern_key kk;
        kk.c[0] = *c;
        kk.c[1] = *(c+1);
        auto k = f->kerning.find(kk.key);
        if (k != f->kerning.end())
            width += k->second;

        elements.push_back(e);
    }

    r.draw(elements, m_font_texture, color);
    return width;
}

//------------------------------------------------------------

int fonts::get_text_width(const wchar_t *text, const char *font_name) const
{
    if(!text || !font_name)
        return 0;

    const auto f = std::find_if(m_fonts.begin(), m_fonts.end(), [font_name](const font &fnt) { return fnt.name == font_name; });
    if (f == m_fonts.end())
        return 0;

    int width = 0;
    for (const wchar_t *c = text; *c; ++c)
    {
        rect_pair e;
        auto fc = f->chars.find(*c);
        if (fc == f->chars.end())
            continue;

        width += fc->second.xadvance;

        kern_key kk;
        kk.c[0] = *c;
        kk.c[1] = *(c+1);
        auto k = f->kerning.find(kk.key);
        if (k != f->kerning.end())
            width += k->second;
    }

    return width;
}

//------------------------------------------------------------

bool tiles::load(const char *name)
{
    fhm_file m;
    if (!m.open(name))
        return false;

    bool skip_fonts_texture = false;
    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);

        if (t == ' RAL')
            continue;

        if (t == '\0BML') //ToDo
            continue;

        if (t == '\0TCA')
            continue;

        if (t == ' MSH')
            continue;

        if (t == '\0FCA')
        {
            skip_fonts_texture = true;
            continue;
        }

        if (t == 'RXTN')
        {
            if (m.get_chunk_size(i) <= 96)
                continue;

            if (skip_fonts_texture)
            {
                skip_fonts_texture = false;
                continue;
            }

            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            m_textures.resize(m_textures.size() + 1);
            m_textures.back() = shared::get_texture(shared::load_texture(b.get_data(), b.get_size()));
            continue;
        }

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader reader(b.get_data(),b.get_size());

        if (t == ' DUH')
        {
            reader.skip(4);
            auto count = reader.read<uint32_t>();

            struct hud_chunk_info
            {
                uint32_t id;
                uint32_t offset;
                uint32_t size;
            };

            std::vector<hud_chunk_info> infos(count);
            for (auto &inf: infos)
                inf = reader.read<hud_chunk_info>();

            m_hud.resize(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                auto &inf = infos[i];
                reader.seek(inf.offset);
                nya_memory::memory_reader r(reader.get_data(), inf.size);

                struct hud_sub_chunk_info
                {
                    uint32_t unknown_6;
                    uint32_t unknown_1;
                    uint32_t unknown_17988; //DF\0\0
                    uint32_t offset;
                    uint32_t size;
                };

                auto info = reader.read<hud_sub_chunk_info>();
                assume(info.unknown_6 == 6);
                assume(info.unknown_1 == 1);
                assume(info.unknown_17988 == 17988);

                r.seek(info.offset);
                r = nya_memory::memory_reader(r.get_data(), info.size);
                auto count = r.read<uint32_t>();
                struct hud_sub_sub_info { uint32_t offset, size; };
                std::vector<hud_sub_sub_info> infos(count);
                for (auto &inf: infos)
                    inf = r.read<hud_sub_sub_info>();

                auto &h = m_hud[i];
                h.id = inf.id;

                m_hud_map[h.id] = i;

                for (auto &inf: infos)
                {
                    r.seek(inf.offset);

                    struct hud_type_header
                    {
                        uint32_t type;
                        uint32_t unknown; //often 0xffffffff
                        uint32_t unknown_0_or_1_or_2;
                        uint32_t unknown_0;
                    };

                    auto header = r.read<hud_type_header>();
                    assume(header.unknown_0_or_1_or_2 == 0 || header.unknown_0_or_1_or_2 == 1 || header.unknown_0_or_1_or_2 == 2);
                    assume(header.unknown_0 == 0);

                    if (header.type == 1)
                    {
                        hud_type1 ht1;
                        while(r.get_remained())
                        {
                            ht1.line_loops.resize(ht1.line_loops.size() + 1);
                            auto &loop = ht1.line_loops.back();
                            auto count = r.read<uint32_t>();
                            if (count == 1)
                            {
                                struct unknown_struct
                                {
                                    uint8_t unknown[4];
                                    uint32_t unknown_2;
                                    uint32_t unknown_0;
                                };

                                auto u = r.read<unknown_struct>();
                                assume(u.unknown_2 == 2);
                                assume(u.unknown_0 == 0);
                            }
                            else
                            {
                                assume(count != 0);
                                loop.resize(count);
                                for (auto &l: loop)
                                    l = r.read<nya_math::vec2>();
                            }
                        }

                        h.type1.push_back(ht1);
                    }
                    else if (header.type == 3)
                    {
                        h.type3.push_back(r.read<hud_type3>());
                    }
                    else if (header.type == 4)
                    {
                        h.type4.push_back(r.read<hud_type4>());
                        assume(h.type4.back().unknown_zero[0] == 0);
                        assume(h.type4.back().unknown_zero[1] == 0);
                        assume(h.type4.back().unknown_zero[2] == 0);
                    }
                    else if (header.type == 5)
                    {
                        //ToDo
                    }
                    else
                        assume(0 && header.type);
                }

                h.type3_progress.resize(h.type3.size(), 1.0f);

                //print_data(r);
            }

            continue;
        }

        if (t == 'XTIU')
        {
            m_uitxs.resize(m_uitxs.size() + 1);
            auto &tx = m_uitxs.back();
            tx.header = reader.read<uitx_header>();
            assume(tx.header.zero[0] == 0 && tx.header.zero[1] == 0);
            tx.entries.resize(tx.header.count);
            for (auto &e: tx.entries)
                e = reader.read<uitx_entry>();
            continue;
        }

        if (m.get_chunk_size(i) == 0)
            continue;

        char *c = (char *)&t;
        printf("%c%c%c%c %d %d %d %d size %d\n",c[3],c[2],c[1],c[0],c[3],c[2],c[1],c[0],m.get_chunk_size(i));
        //continue;

        print_data(reader);
    }

    m.close();
    return true;
}

//------------------------------------------------------------

int tiles::get_id(int idx)
{
    if (idx < 0 || idx >= get_count())
        return -1;

    return m_hud[idx].id;
}

//------------------------------------------------------------

void tiles::draw(const render &r, int id, int x, int y, const nya_math::vec4 &color, float yaw, float scale)
{
    render::transform t;
    t.x = float(x), t.y = float(y);
    t.yaw = yaw;
    t.sx = t.sy = scale;

    auto it = m_hud_map.find(id);
    if (it == m_hud_map.end())
        return;

    auto &h = m_hud[it->second];
    for (auto &t1: h.type1)
    {
        for (auto &l: t1.line_loops)
            r.draw(l, color, t);
    }

    std::vector<rect_pair> rects;
    int tex_idx = -1;
    for (size_t i = 0; i < h.type3.size(); ++i)
    {
        auto &t3 = h.type3[i];
        auto &e = m_uitxs[0].entries[t3.tile_idx];
        rect_pair rp;

        rp.tc.x = e.x;
        rp.tc.y = e.y;
        rp.tc.w = e.w;
        rp.tc.h = e.h * h.type3_progress[i];
        rp.r.x = t3.x;
        rp.r.y = t3.y;
        rp.r.w = t3.w * t3.ws;
        rp.r.h = t3.h * t3.hs * h.type3_progress[i];

        if (t3.flags == 1127481344) //ToDo ? 1127481344 3266576384 3258187776 (flags)
        {
            rp.r.x += rp.r.w;
            rp.r.w = -rp.r.w;
        }

        switch(t3.align)
        {
            case align_top_left: break;
            case align_bottom_left: rp.r.y -= t3.h * t3.hs; break;
            case align_top_right: rp.r.x -= t3.w * t3.ws; break;
            case align_bottom_right: rp.r.x -= t3.w * t3.ws, rp.r.y -= t3.h * t3.hs; break;
            case align_center: rp.r.x -= t3.w/2 * t3.ws, rp.r.y -=t3.h/2 * t3.hs; break;
        };

        assert(tex_idx < 0 || tex_idx == e.tex_idx);

        tex_idx = e.tex_idx;
        rects.push_back(rp);

        //printf("%d %d | %f %f\n", e.w, e.h, t3.x, t3.y);
    }

    /*
    for (auto &t4: h.type4)
    {
        nya_math::vec2 pos(t4.x + x, t4.y + y);
        nya_math::vec2 left(5.0, 0.0);
        nya_math::vec2 top(0.0, 5.0);

        nya_math::vec4 red(1.0, 0.0, 0.0, 1.0);
        std::vector<nya_math::vec2> lines;
        lines.push_back(pos + left + top);
        lines.push_back(pos - left - top);
        r.draw(lines, red);
        lines.clear();
        lines.push_back(pos - left + top);
        lines.push_back(pos + left - top);
        r.draw(lines, red);
    }
    */

    if (tex_idx >= 0)
        r.draw(rects, m_textures[tex_idx], color, t);
}

//------------------------------------------------------------

void tiles::set_progress(int id, int sub_idx, float value)
{
    auto it = m_hud_map.find(id);
    if (it == m_hud_map.end())
        return;

    auto &h = m_hud[it->second];
    if (sub_idx < 0 || sub_idx >= (int)h.type3_progress.size())
        return;

    h.type3_progress[sub_idx] = value;
}

//------------------------------------------------------------

void tiles::draw_tx(const render &r, int uitx_idx, int entry_idx, const rect &rct, const nya_math::vec4 &color)
{
    if (uitx_idx < 0 || uitx_idx >= (int)m_uitxs.size())
        return;

    auto &tx = m_uitxs[uitx_idx];

    if (entry_idx < 0 || entry_idx >= (int)tx.entries.size())
        return;

    std::vector<rect_pair> rects(1);

    auto &e = tx.entries[entry_idx];

    rects[0].tc.x = e.x;
    rects[0].tc.y = e.y;
    rects[0].tc.w = e.w;
    rects[0].tc.h = e.h;
    rects[0].r = rct;

    r.draw(rects, m_textures[e.tex_idx]);
}

//------------------------------------------------------------

void tiles::debug_draw_tx(const render &r)
{
    int y = 0, idx = 0;
    std::vector<rect_pair> rects(1);
    for (auto &tx: m_uitxs)
    {
        int height = 0, x = 0;
        for (auto &e: tx.entries)
        {
            if (x + e.w > r.get_width())
            {
                y += height + 5;
                x = height = 0;
            }

            rects[0].tc.x = e.x;
            rects[0].tc.y = e.y;
            rects[0].tc.w = e.w;
            rects[0].tc.h = e.h;
            rects[0].r = rects[0].tc;
            rects[0].r.x = x;
            rects[0].r.y = y;
            if (e.h > height)
                height = e.h;
            x += e.w + 5;

            r.draw(rects, m_textures[e.tex_idx]);
        }
        y += height + 5;
        ++idx;
    }
}

//------------------------------------------------------------
}
