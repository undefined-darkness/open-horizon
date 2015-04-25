//
// open horizon -- undefined_darkness@outlook.com
//

#include "ui.h"
#include "fhm.h"
#include "shared.h"
#include "memory/memory_reader.h"

#include "debug.h"
#include <assert.h>

//------------------------------------------------------------

bool ui::load_fonts(const char *name)
{
    fhm_file m;
    if (!m.open(name))
        return false;

    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);
        if (t == 'RXTN')
        {
            if (m.get_chunk_size(i)<96)
                continue;

            nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
            m.read_chunk_data(i, b.get_data());
            m_font_texture = shared::get_texture(shared::load_texture(b.get_data(), b.get_size()));
            continue;
        }

        if (t == '\0FCA')
        {
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

                struct acf_font_header
                {
                    uint8_t unknown[4];
                    uint8_t unknown2[4];
                    uint16_t char_count;
                    uint16_t unknown3;
                    uint8_t unknown4[4];
                    uint8_t unknown5[4];
                };

                auto header = r.read<acf_font_header>();
                r.skip(1024);

                for (uint32_t j = 0; j < header.char_count; ++j)
                {
                    struct acf_char
                    {
                        uint16_t unknown, x;
                        uint16_t y, unknown2;
                        uint8_t unknown3[4];
                        uint8_t width, height;
                        uint16_t char_code;
                        uint8_t unknown4[4];
                        uint32_t unknown5;
                    };

                    auto c = r.read<acf_char>();

                    wchar_t char_code = ((c.char_code & 0xff) << 8) | ((c.char_code & 0xff00) >> 8);
                    auto &fc = f.chars[char_code];
                    fc.x = c.x, fc.y = c.y;
                    fc.w = c.width, fc.h = c.height;
                }
            }

            continue;
        }
/*
        char *c = (char *)&t;
        printf("%c%c%c%c\n",c[3],c[2],c[1],c[0]);

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader r(b.get_data(),b.get_size());
        print_data(r);
*/
    }

    init();
    return true;
}

//------------------------------------------------------------

void ui::init()
{
    if (m_tex.is_valid()) //already initialised
        return;

    m_tex.create();
    m_color.create();
    m_tr.create();
    m_tc_tr.create();
    m_mesh.init();

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
}

//------------------------------------------------------------

int ui::draw_text(const wchar_t *text, const char *font, int x, int y, const nya_math::vec4 &color) const
{
    if (!m_tex.is_valid()) //not initialised
        return 0;

    if (!m_width || !m_height)
        return 0;

    if(!text || !font)
        return 0;

    const auto f = std::find_if(m_fonts.begin(), m_fonts.end(), [font](const ui::font &fnt) { return fnt.name == font; });
    if (f == m_fonts.end())
        return 0;

    if (!m_font_texture.get_width() || !m_font_texture.get_height())
        return 0;

    m_color->set(color);
    m_tex.set(m_font_texture);

    int idx = 0, width = 0;
    for (const wchar_t *c = text; *c; ++c)
    {
        assert(idx<200); //ToDo

        m_tr->set_count(idx + 1);
        m_tc_tr->set_count(idx + 1);

        auto fc = f->chars.find(*c);
        if (fc == f->chars.end())
            continue;

        nya_math::vec4 pos;
        pos.x = -1.0 + 2.0 * float(x + width) / m_width;
        pos.y = -1.0 + 2.0 * float(y) / m_height;
        pos.z = 2.0 * float(fc->second.w) / m_width;
        pos.w = 2.0 * float(fc->second.h) / m_height;
        m_tr->set(idx, pos);

        nya_math::vec4 tc;
        tc.x = float(fc->second.x) / m_tex->get_width();
        tc.y = float(fc->second.y) / m_tex->get_height();
        tc.z = float(fc->second.w) / m_tex->get_width();
        tc.w = -float(fc->second.h) / m_tex->get_height();
        tc.y -= tc.w;
        m_tc_tr->set(idx, tc);

        width += fc->second.w;
        ++idx;
    }

    m_material.internal().set(nya_scene::material::default_pass);
    m_mesh.draw(idx);
    m_material.internal().unset();

    static nya_scene::texture empty;
    m_tex.set(empty);

    return width;
}

//------------------------------------------------------------
