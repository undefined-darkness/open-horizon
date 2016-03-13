//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/plane_params.h"
#include "model.h"
#include "location_params.h"
#include "particles.h"

namespace renderer
{
//------------------------------------------------------------

class scene;

class aircraft
{
public:
    bool load(const char *name, unsigned int color_idx, const location_params &params, bool player);
    void load_missile(const char *name, const location_params &params);
    void load_special(const char *name, const location_params &params);
    void apply_location(const char *location_name, const location_params &params);
    void draw(int lod_idx);
    void draw_player();
    void draw_trails(const scene &s);
    void draw_fire_trail(const scene &s);
    void draw_mgun_flash(const scene &s);
    int get_lods_count() const { return m_mesh.get_lods_count(); }
    void update(int dt);
    void update_trail(int dt, scene &s);
    void set_hide(bool value) { m_hide = value; }

    void set_pos(const nya_math::vec3 &pos) { m_mesh.set_pos(pos); }
    void set_rot(const nya_math::quat &rot) { m_mesh.set_rot(rot); }
    const nya_math::vec3 &get_pos() { return m_mesh.get_pos(); }
    nya_math::quat get_rot() { return m_mesh.get_rot(); }
    nya_math::vec3 get_bone_pos(const char *name);

    int get_mguns_count() const;
    nya_math::vec3 get_mgun_pos(int idx);

    void set_damage(float value) { m_damage = value; }
    float get_damage() const { return m_damage; }
    void set_dead(bool dead);

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
    void set_mgun_fire(bool value) { m_fire_mgun = value; }

    //weapons
    bool has_special_bay();
    bool is_special_bay_opened();
    bool is_special_bay_closed();
    bool is_missile_ready();
    bool is_mgun_ready();
    int get_missile_mount_count() { return (int)m_msls_mount.size(); }
    nya_math::vec3 get_missile_mount_pos(int idx);
    nya_math::quat get_missile_mount_rot(int idx);
    void set_missile_visible(int idx, bool visible);
    int get_special_mount_count() { return (int)m_special_mount.size(); }
    nya_math::vec3 get_special_mount_pos(int idx);
    nya_math::quat get_special_mount_rot(int idx);
    void set_special_visible(int idx, bool visible);

    //weapon models
    const renderer::model &get_missile_model();
    const renderer::model &get_special_model();

    //cockpit
    void set_time(unsigned int time) { m_time = time * 1000; } //in seconds
    void set_speed(float speed) { m_speed = speed; }
    void set_aoa(float aoa) { m_aoa = aoa; }

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

    aircraft(): m_hide(false), m_time(0), m_camera_mode(camera_mode_third), m_half_flaps_flag(false),
                m_engine_lod_idx(0), m_dead(false), m_has_trail(false), m_fire_mgun(false)
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

    model m_mesh;
    bool m_hide;
    params::fvalue m_damage;
    params::fvalue m_speed;
    params::fvalue m_aoa;

    unsigned int m_time;

    int m_adimx_bone_idx, m_adimx2_bone_idx;
    int m_adimz_bone_idx, m_adimz2_bone_idx;
    int m_adimxz_bone_idx, m_adimxz2_bone_idx;

    bool m_half_flaps_flag;

    int m_engine_lod_idx;

    model m_missile;
    model m_special;

    struct wpn_mount
    {
        bool visible;
        int bone_idx;
    };

    std::vector<wpn_mount> m_msls_mount;
    std::vector<wpn_mount> m_special_mount;

    nya_math::vec3 m_camera_offset;
    camera_mode m_camera_mode;

    fire_trail m_fire_trail;
    bool m_dead;

    bool m_has_trail;
    std::pair<plane_trail, int> m_trails[2];

    struct mgun
    {
        int bone_idx;
        muzzle_flash flash;
    };

    std::vector<mgun> m_mguns;
    bool m_fire_mgun;
};

//------------------------------------------------------------
}
