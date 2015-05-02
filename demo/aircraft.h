//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "plane_params.h"
#include "fhm_mesh.h"
#include "location_params.h"

//------------------------------------------------------------

class aircraft
{
public:
    bool load(const char *name, unsigned int color_idx = 0);

    void apply_location(const char *location_name, const location_params &params);

    void draw() { m_mesh.draw(0); }

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; }
    const nya_math::vec3 &get_pos() { return m_pos; }
    nya_math::quat get_rot() { return m_rot; }

    float get_speed() { return m_speed; }
    float get_alt() { return m_pos.y; }
    float get_hp() { return m_hp; }
    const nya_math::vec3 &get_camera_offset() const { return m_camera_offset; }
    nya_math::vec3 get_bone_pos(const char *name);

    static unsigned int get_colors_count(const char *plane_name);

    void set_controls(const nya_math::vec3 &rot, float throttle, float brake);

    void fire_mgun() { m_controls_mgun = true; }
    void fire_rocket() { m_controls_rocket = true; }
    void change_weapon() { m_controls_special = true; }

    void update(int dt);

    aircraft(): m_hp(0), m_controls_throttle(0), m_controls_brake(0), m_thrust_time(0),
    m_controls_mgun(false), m_controls_rocket(false), m_controls_special(false),
    m_special_selected(false), m_rocket_bay_time(0) {}

private:
    float clamp(float value, float from, float to) { if (value < from) return from; if (value > to) return to; return value; }
    float tend(float value, float target, float speed)
    {
        const float diff = target - value;
        if (diff > speed) return value + speed;
        if (-diff > speed) return value - speed;
        return target;
    }

    float m_hp;
    fhm_mesh m_mesh;
    float m_speed;
    float m_thrust_time;
    nya_math::vec3 m_pos;
    nya_math::vec3 m_rot_speed;
    nya_math::quat m_rot;

    nya_math::vec3 m_controls_rot;
    float m_controls_throttle;
    float m_controls_brake;
    plane_params m_params;
    bool m_controls_mgun;
    bool m_controls_rocket;
    bool m_controls_special;

    bool m_special_selected;

    float m_rocket_bay_time;

    nya_math::vec3 m_camera_offset;
};

//------------------------------------------------------------
