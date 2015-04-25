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

public:
    ui(): m_width(0), m_height(0) {}

private:
    void init();

private:
    struct chr
    {
        uint16_t x,y;
        uint8_t w,h;
    };

    struct font
    {
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
