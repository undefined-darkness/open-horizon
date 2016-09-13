//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/screen_quad.h"
#include "util/params.h"
#include <stdint.h>

namespace gui
{
//------------------------------------------------------------

typedef params::value<bool> bvalue;
typedef params::value<int> ivalue;
typedef params::value<float> fvalue;
typedef params::uvalue uvalue;

//------------------------------------------------------------

struct rect
{
    int x, y, w, h;

    rect(): x(0), y(0), w(0), h(0) {}
    rect(int x, int y, int w, int h): x(x), y(y), w(w), h(h) {}
};

//------------------------------------------------------------

struct rect_pair { rect r, tc; };

//------------------------------------------------------------

class render
{
public:
    void init();
    void resize(int width, int height) { m_width = width, m_height = height; }

public:
    struct transform { fvalue x, y, yaw, sx, sy; transform() { sx = sy = 1.0f; } };

    void draw(const std::vector<rect_pair> &elements, const nya_scene::texture &tex,
              const nya_math::vec4 &color = nya_math::vec4(1.0, 1.0, 1.0, 1.0), const transform &t = transform()) const;
    void draw(const std::vector<nya_math::vec2> &elements,
              const nya_math::vec4 &color = nya_math::vec4(1.0, 1.0, 1.0, 1.0), const transform &t = transform(), bool loop = true) const;
public:
    int get_width(bool real = false) const { return real ? m_width : 1366; }
    int get_height(bool real = false) const { return real ? m_height : 768; }
//1366 x 768
    render(): m_width(0), m_height(0) {}
    
private:
    bool set_transform(const transform &t) const;

    int m_width, m_height;
    nya_render::vbo m_quads_mesh;
    nya_render::vbo m_lines_mesh;
    nya_render::vbo m_lines_loop_mesh;
    nya_scene::material m_material;
    mutable nya_scene::material::param_proxy m_color;
    mutable nya_scene::material::param_array_proxy m_tr;
    mutable nya_scene::material::param_array_proxy m_tc_tr;
    mutable nya_scene::material::param_proxy m_urot;
    mutable nya_scene::material::param_proxy m_utr;
    mutable nya_scene::texture_proxy m_tex;
};

//------------------------------------------------------------

class fonts
{
public:
    bool load(const char *name);

public:
    //returns width of drawn text
    int draw_text(const render &r, const wchar_t *text, const char *font, int x, int y, const nya_math::vec4 &color) const;
    int get_text_width(const wchar_t *text, const char *font) const;

private:
    struct acf_font_header
    {
        uint16_t height;
        uint16_t unknown[3];
        uint16_t char_count;
        uint16_t kern_count;
        uint16_t unknown2[4];
    };

    struct acf_char
    {
        uint16_t unknown, x;
        uint16_t y, unknown2;
        uint16_t yoffset, xadvance;
        uint8_t width, height;
        uint16_t char_code;
        uint16_t unknown3[2];
        uint32_t unknown5;
    };

private:
    struct chr
    {
        uint16_t x,y;
        uint8_t w,h;
        float yoffset;
        float xadvance;
        float unknown;
        float unknown2;
        uint32_t unknown3;
    };

    union kern_key
    {
        uint32_t key;
        uint16_t c[2];
    };

    struct font
    {
        float height;
        float unknown[7];

        std::string name;
        std::map<wchar_t, chr> chars;
        std::map<uint32_t, float> kerning;
    };

    std::vector<font> m_fonts;
    nya_scene::texture m_font_texture;
};

//------------------------------------------------------------

class tiles
{
public:
    bool load(const char *name);
    void draw(const render &r, int id, int x, int y, const nya_math::vec4 &color, float yaw = 0.0f, float scale = 1.0f);
    int get_count() { return (int)m_hud.size(); }
    int get_id(int idx);
    void set_progress(int id, int sub_idx, float value);

    void draw_tx(const render &r, int uitx_idx, int entry_idx, const rect &rct, const nya_math::vec4 &color);

    void debug_draw(const render &r, int idx) { draw(r, get_id(idx), r.get_width() / 2,
                                                     r.get_height() / 2, nya_math::vec4(1.0, 1.0, 1.0, 1.0)); }
    void debug_draw_tx(const render &r);

private:
    enum align_mode
    {
        align_top_left,
        align_bottom_left,
        align_top_right,
        align_bottom_right,
        align_center
    };

    struct hud_type1
    {
        std::vector<std::vector<nya_math::vec2> > line_loops;
    };

    struct hud_type3
    {
        uint32_t tile_idx;
        uint16_t tx, ty;
        uint16_t tw, th;
        float x;
        float y;
        float w;
        float h;
        float ws;
        float hs;
        uint32_t flags;
        align_mode align;
    };

    struct hud_type4
    {
        float x;
        float y;
        uint32_t unknown;
        uint32_t unknown_zero[3];
        float unknown2;
    };

    struct hud
    {
        uint32_t id;
        std::vector<hud_type1> type1;
        std::vector<hud_type3> type3;
        std::vector<hud_type4> type4;

        std::vector<float> type3_progress;
    };

    std::vector<hud> m_hud;
    std::map<int, int> m_hud_map;

    struct uitx_header
    {
        char sign[4];
        uint32_t zero[2];
        uint32_t count;
        uint32_t unknown;

    };

    struct uitx_entry
    {
        uint16_t x,y;
        uint16_t w,h;
        uint32_t tex_idx;
    };

    struct uitx
    {
        uitx_header header;
        std::vector<uitx_entry> entries;
    };

    std::vector<uitx> m_uitxs;
    std::vector<nya_scene::texture> m_textures;
};

//------------------------------------------------------------
}
