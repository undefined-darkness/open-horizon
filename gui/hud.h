//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"

namespace gui
{
//------------------------------------------------------------

class hud
{
public:
    void load(const char *aircraft_name);
    void draw(const render &r);

    void set_speed(int value) { m_speed = value; }
    void set_alt(int value) { m_alt = value; }

    hud(): m_speed(0), m_alt(0), m_common_loaded(false) {}

private:
    int m_speed;
    int m_alt;

private:
    bool m_common_loaded;
    fonts m_fonts;
    tiles m_common;
    tiles m_aircraft;
};

//------------------------------------------------------------
}
