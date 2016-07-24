//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"
#include "circle_mesh.h"

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
    void set_angles(float pitch, float yaw, float roll) { m_pitch = pitch, m_yaw = yaw, m_roll = roll; }
    void set_speed(int value) { m_speed = value; }
    void set_ab(bool value) { m_ab = value; }
    void set_pitch_ladder(bool visible) { m_pitch_ladder = visible; }
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
    void set_target_arrow(bool visible, const nya_math::vec3 &dir = nya_math::vec3());
    void change_radar() { m_show_map = !m_show_map; }

    bool is_special_selected() const { return m_missiles_icon > 0; }

    void clear_scores();
    void set_team_score(int allies, int enemies);
    void set_score(int line, int place, const std::wstring &name, const std::wstring &value);
    void remove_score(int line);

    void clear_alerts() { m_alerts.clear(); }
    void add_alert(float v) { m_alerts.push_back(v); }

public:
    typedef nya_math::vec4 color;
    const static color white;
    const static color green;
    const static color red;
    const static color blue;

public:
    enum target_type
    {
        target_air,
        target_air_lock,
        target_air_ally,
        target_ground,
        target_ground_lock,
        target_ground_ally,
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
    void add_target(const std::wstring &name, const std::wstring &player_name, const nya_math::vec3 &pos, float yaw, target_type target, select_type select, bool tgt);

    void set_bomb_target(const nya_math::vec3 &pos, float radius, const color &c);
    void clear_bomb_circles() { m_bomb_targets.clear(); }
    void add_bomb_circle(const nya_math::vec3 &pos, float radius, const color &c);

    void clear_ecm() { m_ecms.clear(); }
    void add_ecm(const nya_math::vec3 &pos) { m_ecms.push_back(pos); }

    void clear_texts() { m_texts.clear(); }
    void add_text(int idx, const std::wstring &text, const std::string &font, int x, int y, const color &c);

    enum { popup_priority_mission_result = 200 };
    void popup(const std::wstring &text, int priority, const color &c = green, int time = 2000);

    void set_radio(std::wstring name, std::wstring message, int time, const color &c = white);

    hud(): m_common_loaded(false), m_hide(false) {}

private:
    typedef nya_math::vec2 vec2;
    nya_math::vec3 m_project_pos;
    nya_math::vec3 m_pos;
    ivalue m_speed;
    bvalue m_ab;
    fvalue m_pitch;
    fvalue m_yaw;
    fvalue m_roll;
    ivalue m_alt;
    ivalue m_missiles_icon;
    ivalue m_missiles_cross;
    bvalue m_show_map;
    bvalue m_pitch_ladder;
    std::vector<float> m_alerts;
    nya_math::vec3 m_target_arrow_dir;
    bvalue m_target_arrow_visible;

    struct target
    {
        std::wstring name;
        std::wstring player_name;
        nya_math::vec3 pos;
        float yaw;
        target_type t;
        select_type s;
        bool tgt;
    };

    std::vector<target> m_targets;

    struct bomb_target { nya_math::vec3 p; float r = 0.0f; color c; };
    std::vector<bomb_target> m_bomb_targets;
    bomb_target m_bomb_target;
    bvalue m_bomb_target_enabled;
    circle_mesh m_bomb_target_mesh;

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

    std::vector<vec2> m_saam_mesh;

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
    vec2 m_map_offset;
    float m_map_scale = 1.0f;

private:
    std::wstring m_popup_text;
    color m_popup_color;
    ivalue m_popup_time;
    ivalue m_popup_priority;

private:
    std::wstring m_radio_name;
    std::vector<std::wstring> m_radio_text;
    color m_radio_color;
    ivalue m_radio_time;
};

//------------------------------------------------------------
}
