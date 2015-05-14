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

    void set_project_pos(const nya_math::vec3 &pos) { m_project_pos = pos; }
    void set_speed(int value) { m_speed = value; }
    void set_alt(int value) { m_alt = value; }
    void set_missiles(const char *id, int icon);
    void set_missile_reload(int idx, float value);

    void clear_targets() { m_targets.clear(); }
    void add_target(const nya_math::vec3 &pos, bool locked);

    hud(): m_common_loaded(false) {}

private:
    nya_math::vec3 m_project_pos;
    ivalue m_speed;
    ivalue m_alt;
    ivalue m_missiles_icon;
    ivalue m_missiles_cross;

    std::vector<std::pair<nya_math::vec3, bool> > m_targets;

private:
    bool m_common_loaded;
    fonts m_fonts;
    tiles m_common;
    tiles m_aircraft;
};

//------------------------------------------------------------
}
