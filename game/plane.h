//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

struct missile;
struct bomb;

//------------------------------------------------------------

struct plane: public object, public std::enable_shared_from_this<plane>
{
    std::wstring name;
    std::wstring player_name;

    net_plane_ptr net;
    game_data local_game_data; //local general data
    game_data net_game_data; //sync over network
    plane_controls controls;
    plane_controls last_controls;
    ivalue change_target_hold_time;

    enum { change_target_hold_max_time = 500 };

    phys::plane_ptr phys;
    renderer::aircraft_ptr render;
    bvalue special_weapon_selected;
    bvalue need_fire_missile;
    ivalue missile_bay_time;
    ivalue mgun_fire_update;
    ivalue mgp_fire_update;
    bvalue jammed;
    ivalue ecm_time;

    wpn_params missile;
    ivalue missile_cooldown[2];
    std::vector<ivalue> missile_mount_cooldown;
    ivalue missile_mount_idx;
    ivalue missile_count, missile_max_count;

    wpn_params special;
    ivalue special_cooldown[2];
    std::vector<ivalue> special_mount_cooldown;
    ivalue special_mount_idx;
    ivalue special_count, special_max_count;
    bvalue special_internal;

    fvalue hit_radius;

    sound::pack sounds;
    std::map<std::string, sound::source_ptr> sound_srcs;
    std::map<std::string, ivalue> sounds_ui;
    typedef std::pair<vec3, sound::source_ptr> sound_rel_src;
    std::vector<sound_rel_src> sound_rel_srcs;

    std::vector<nya_math::vec3> alert_dirs;

    struct target_lock
    {
        object_wptr target;
        ivalue locked;
        fvalue dist;
        fvalue cos;

        bool can_lock(const wpn_params &p) const { return dist < p.lockon_range && cos > p.lockon_angle_cos; }
    };

    std::vector<target_lock> targets;
    ivalue lock_timer;

    w_ptr<game::missile> saam_missile;

    struct bomb_mark
    {
        w_ptr<bomb> b;
        vec3 p;
        bvalue need_update;
    };

    std::vector<bomb_mark> bomb_marks;

    void reset_state();
    void set_pos(const vec3 &pos) { if (phys) phys->pos = pos; }
    void set_rot(const quat &rot) { if (phys) phys->rot = rot; }
    vec3 get_dir() { return get_rot().rotate(vec3::forward()); }
    void select_target(const object_wptr &o);
    void update(int dt, world &w);
    void update_hud(world &w, gui::hud &h);
    bool is_ecm_active() const { return special.id=="ECM" && ecm_time > 0;}

    //object
public:
    virtual vec3 get_pos() override { return phys ? phys->pos : vec3(); }
    virtual vec3 get_vel() override { return phys ? phys->vel : vec3(); }
    virtual quat get_rot() override { return phys ? phys->rot : quat(); }
    virtual std::wstring get_name() override { return player_name; }
    virtual std::wstring get_type_name() override { return name; }
    virtual bool is_targetable(bool air, bool ground) override { return air && hp > 0; }
    virtual bool is_ally(const plane_ptr &p, world &w) override;
    virtual void take_damage(int damage, world &w, bool net_src = true) override;
    virtual float get_hit_radius() override { return hit_radius; }

private:
    void update_sound(world &w, std::string name, bool enabled, float volume = 1.0f);
    void update_ui_sound(world &w, std::string name, bool enabled);
    void play_relative(world &w, std::string name, int idx, vec3 offset);
    void update_targets(world &w);
    void update_render(world &w);
    void update_weapons_hud();
    bool is_mg_bay_ready();
    bool is_special_bay_ready();
};

//------------------------------------------------------------
}

