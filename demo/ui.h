//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/material.h"
#include "render/screen_quad.h"

//------------------------------------------------------------

class ui
{
public:
    bool load_fonts(const char *name);
    void resize(int width, int height) { m_width = width, m_height = height; }

public:
    int draw_text(const wchar_t *text, const char *font, int x, int y, const nya_math::vec4 &color) const; //returns width of drawn text
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

public:
    ui(): m_width(0), m_height(0) {}

private:
    void init();

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
