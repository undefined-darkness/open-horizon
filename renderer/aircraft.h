//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/plane_params.h"
#include "renderer/model.h"
#include "renderer/location_params.h"

namespace renderer
{
//------------------------------------------------------------

class aircraft
{
public:
    bool load(const char *name, unsigned int color_idx, const location_params &params);
    void load_missile(const char *name, const location_params &params);
    void load_special(const char *name, const location_params &params);
    void apply_location(const char *location_name, const location_params &params);
    void draw(int lod_idx);
    int get_lods_count() const { return m_mesh.get_lods_count(); }
    void update(int dt);

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; m_mesh.set_pos(pos); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; m_mesh.set_rot(rot); }
    const nya_math::vec3 &get_pos() { return m_pos; }
    nya_math::quat get_rot() { return m_rot; }
    nya_math::vec3 get_bone_pos(const char *name);

    //aircraft animations
    void set_elev(float left, float right); //elevons and tailerons
    void set_rudder(float left, float right, float center);
    void set_aileron(float left, float right);
    void set_flaperon(float value);
    void set_canard(float value);
    void set_brake(float value);
    void set_wing_sweep(float value);
    void set_intake_ramp(float value);
    void set_special_bay(bool value);
    void set_missile_bay(bool value);
    void set_mgun_bay(bool value);

    //weapons
    bool has_special_bay();
    bool is_special_bay_opened();
    bool is_special_bay_closed();
    bool is_missile_ready();
    int get_missile_mount_count() { return (int)m_msls_mount.size(); }
    nya_math::vec3 get_missile_mount_pos(int idx);
    nya_math::quat get_missile_mount_rot(int idx);
    void set_missile_visible(int idx, bool visible);
    int get_special_mount_count() { return (int)m_special_mount.size(); }
    nya_math::vec3 get_special_mount_pos(int idx);
    nya_math::quat get_special_mount_rot(int idx);
    void set_special_visible(int idx, bool visible);

    //cockpit and ui
    void set_time(unsigned int time) { m_time = time * 1000; } //in seconds
    void set_speed(float speed) { m_speed = speed; }
    float get_speed() { return m_speed; }
    float get_alt() { return m_pos.y; }

    //camera
    enum camera_mode
    {
        camera_mode_third,
        camera_mode_cockpit,
        camera_mode_first,
    };
    camera_mode get_camera_mode() const { return m_camera_mode; }
    void set_camera_mode(camera_mode mode) { m_camera_mode = mode; }
    const nya_math::vec3 &get_camera_offset() const { return m_camera_offset; }

    //info
    static unsigned int get_colors_count(const char *plane_name);

    aircraft(): m_time(0), m_camera_mode(camera_mode_third), m_half_flaps_flag(false)
    {
        m_adimx_bone_idx = m_adimx2_bone_idx = -1;
        m_adimz_bone_idx = m_adimz2_bone_idx = -1;
        m_adimxz_bone_idx = m_adimxz2_bone_idx = -1;
    }

private:
    float clamp(float value, float from, float to) { if (value < from) return from; if (value > to) return to; return value; }
    float tend(float value, float target, float speed)
    {
        const float diff = target - value;
        if (diff > speed) return value + speed;
        if (-diff > speed) return value - speed;
        return target;
    }

    renderer::model m_mesh;
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
    params::fvalue m_speed;

    unsigned int m_time;

    int m_adimx_bone_idx, m_adimx2_bone_idx;
    int m_adimz_bone_idx, m_adimz2_bone_idx;
    int m_adimxz_bone_idx, m_adimxz2_bone_idx;

    bool m_half_flaps_flag;

    renderer::model m_missile;
    renderer::model m_special;

    struct wpn_mount
    {
        bool visible;
        int bone_idx;
    };

    std::vector<wpn_mount> m_msls_mount;
    std::vector<wpn_mount> m_special_mount;

    nya_math::vec3 m_camera_offset;
    camera_mode m_camera_mode;
};

//------------------------------------------------------------
}