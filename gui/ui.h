//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/screen_quad.h"
#include <stdint.h>

namespace gui
{
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
    void draw(const std::vector<rect_pair> &elements, const nya_scene::texture &tex, const nya_math::vec4 &color) const;
    int get_width(bool real = false) const { return real ? m_width : 1280; }
    int get_height(bool real = false) const { return real ? m_height : 720; }

    render(): m_width(0), m_height(0) {}

private:
    int m_width, m_height;
    nya_render::screen_quad m_mesh;
    nya_scene::material m_material;
    mutable nya_scene::material::param_proxy m_color;
    mutable nya_scene::material::param_array_proxy m_tr;
    mutable nya_scene::material::param_array_proxy m_tc_tr;
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

private:
    struct acf_font_header
    {
        uint8_t unknown[4];
        uint8_t unknown2[4];
        uint16_t char_count;
        uint16_t unknown3;
        uint8_t unknown4[4];
        uint8_t unknown5[4];
    };

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

private:
    struct chr
    {
        acf_char t; //temp

        uint16_t x,y;
        uint8_t w,h;
    };

    struct font
    {
        acf_font_header t; //temp

        std::string name;
        std::map<wchar_t, chr> chars;
    };

    std::vector<font> m_fonts;
    nya_scene::texture m_font_texture;
};

//------------------------------------------------------------
}
