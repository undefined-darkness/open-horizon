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
    void load(const char *aircraft_name, const char *location_name);
    void draw(const render &r);
    void update(int dt);

    void set_location(const char *location_name);
    void set_hide(bool value);
    void set_project_pos(const nya_math::vec3 &pos) { m_project_pos = pos; }
    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    void set_yaw(float yaw) { m_yaw = yaw; }
    void set_speed(int value) { m_speed = value; }
    void set_alt(int value) { m_alt = value; }
    void set_missiles(const char *id, int icon);
    void set_missiles_count(int count) { m_missiles_count = count; }
    void set_missile_reload(int idx, float value);
    void set_locks(int count, int icon);
    void set_lock(int idx, bool locked, bool active);
    void set_mgun(bool visible) { m_mgun = visible; }
    void set_saam_circle(bool visible, float angle);
    void set_saam(bool locked, bool tracking) { m_saam_locked = locked; m_saam_tracking = tracking; }
    void set_mgp(bool active) { m_mgp = active; }
    void set_jammed(bool active) { m_jammed = active; }
    void change_radar() { m_show_map = !m_show_map; }

    void clear_scores();
    void set_team_score(int allies, int enemies);
    void set_score(int line, int place, const std::wstring &name, const std::wstring &value);
    void remove_score(int line);

    void clear_alerts() { m_alerts.clear(); }
    void add_alert(float v) { m_alerts.push_back(v); }

    enum target_type
    {
        target_air,
        target_air_lock,
        target_air_ally,
        target_missile
    };

    enum select_type
    {
        select_not,
        select_current,
        select_next
    };

    void clear_targets() { m_targets.clear(); }
    void add_target(const nya_math::vec3 &pos, float yaw, target_type target, select_type select);

    void clear_ecm() { m_ecms.clear(); }
    void add_ecm(const nya_math::vec3 &pos) { m_ecms.push_back(pos); }

    void clear_texts() { m_texts.clear(); }
    void add_text(int idx, const std::wstring &text, const std::string &font, int x, int y, const nya_math::vec4 &color);

    hud(): m_common_loaded(false), m_hide(false) {}

public:
    const static nya_math::vec4 white;
    const static nya_math::vec4 green;
    const static nya_math::vec4 red;
    const static nya_math::vec4 blue;

private:
    nya_math::vec3 m_project_pos;
    nya_math::vec3 m_pos;
    ivalue m_speed;
    fvalue m_yaw;
    ivalue m_alt;
    ivalue m_missiles_icon;
    ivalue m_missiles_cross;
    bvalue m_show_map;
    std::vector<float> m_alerts;

    struct target
    {
        nya_math::vec3 pos;
        float yaw;
        target_type t;
        select_type s;
    };

    std::vector<target> m_targets;

    std::vector<nya_math::vec3> m_ecms;

    struct lock { bool locked = false, active = false; };
    std::vector<lock> m_locks;
    ivalue m_lock_icon, m_lock_icon_active;

    ivalue m_anim_time;
    std::wstring m_missiles_name;
    ivalue m_missiles_count;
    bvalue m_mgun;
    bvalue m_saam_visible;
    bvalue m_saam_locked;
    bvalue m_saam_tracking;
    bvalue m_mgp_icon;
    bvalue m_mgp;
    bvalue m_jammed;

    std::vector<nya_math::vec2> m_saam_mesh;

    struct score_line
    {
        int place = -1;
        std::wstring name, value;
    };

    std::vector<score_line> m_scores;
    bvalue m_show_team_score;
    std::wstring m_team_score[2];

    struct text
    {
        std::wstring t;
        std::string f;
        int x = 0, y = 0;
        nya_math::vec4 c;
    };

    std::vector<text> m_texts;

private:
    bool m_common_loaded;
    bool m_hide;
    fonts m_fonts;
    tiles m_common;
    tiles m_aircraft;
    tiles m_location;
    std::string m_location_name;
    nya_math::vec2 m_map_offset;
    float m_map_scale = 1.0f;
};

//------------------------------------------------------------
}
