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

    void set_hide(bool value) { m_hide = value; }
    void set_project_pos(const nya_math::vec3 &pos) { m_project_pos = pos; }
    void set_speed(int value) { m_speed = value; }
    void set_alt(int value) { m_alt = value; }
    void set_missiles(const char *id, int icon);
    void set_missile_reload(int idx, float value);

    enum target_type
    {
        target_air,
        target_air_lock,
        target_air_ally
    };

    enum select_type
    {
        select_not,
        select_current,
        select_next
    };

    void clear_targets() { m_targets.clear(); }
    void add_target(const nya_math::vec3 &pos, target_type target, select_type select);

    hud(): m_common_loaded(false), m_hide(false) {}

private:
    nya_math::vec3 m_project_pos;
    ivalue m_speed;
    ivalue m_alt;
    ivalue m_missiles_icon;
    ivalue m_missiles_cross;

    struct target
    {
        nya_math::vec3 pos;
        target_type t;
        select_type s;
    };

    std::vector<target> m_targets;

private:
    bool m_common_loaded;
    bool m_hide;
    fonts m_fonts;
    tiles m_common;
    tiles m_aircraft;
};

//------------------------------------------------------------
}
