//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/physics.h"
#include "renderer/scene.h"
#include "sound/sound.h"
#include "gui/hud.h"
#include "network_data.h"
#include <memory>
#include <list>

namespace game
{
//------------------------------------------------------------

std::vector<std::string> get_aircraft_ids(const std::vector<std::string> &roles);

const std::wstring &get_aircraft_name(const std::string &id);

//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef nya_math::angle_deg angle_deg;
typedef nya_math::angle_rad angle_rad;
typedef params::fvalue fvalue;
typedef params::uvalue uvalue;
typedef params::value<int> ivalue;
typedef params::value<bool> bvalue;

//------------------------------------------------------------

class world;

//------------------------------------------------------------

template <typename t> using ptr = std::shared_ptr<t>;
template <typename t> using w_ptr = std::weak_ptr<t>;

//------------------------------------------------------------

struct object
{
    ivalue max_hp;
    ivalue hp;

    virtual void take_damage(int damage, world &w, bool net_src = true) { hp = damage < hp ? hp - damage : 0; }
};

typedef ptr<object> object_ptr;

//------------------------------------------------------------

struct plane_controls: public phys::plane_controls
{
    bvalue missile;
    bvalue mgun;
    bvalue flares;
    bvalue change_weapon;
    bvalue change_target;
    bvalue change_camera;
    bvalue change_radar;
    nya_math::vec2 cam_rot;
};

//------------------------------------------------------------

struct wpn_params
{
    std::string id, model;
    fvalue lockon_range;
    fvalue lockon_angle_cos;
    ivalue lockon_time;
    ivalue lockon_count;
    bvalue lockon_air;
    bvalue lockon_ground;
    fvalue action_range;
    ivalue reload_time;

    wpn_params() {}
    wpn_params(std::string id, std::string model);
};

//------------------------------------------------------------

struct missile;

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

    fvalue hit_radius;

    sound::pack sounds;
    std::map<std::string, sound::source_ptr> sound_srcs;
    std::map<std::string, ivalue> sounds_ui;
    typedef std::pair<vec3, sound::source_ptr> sound_rel_src;
    std::vector<sound_rel_src> sound_rel_srcs;

    std::vector<nya_math::vec3> alert_dirs;

    struct target_lock
    {
        w_ptr<plane> target_plane;
        ivalue locked;
        fvalue dist;
        fvalue cos;

        bool can_lock(const wpn_params &p) const { return dist < p.lockon_range && cos > p.lockon_angle_cos; }
    };

    std::vector<target_lock> targets;
    ivalue lock_timer;

    w_ptr<game::missile> saam_missile;

    void reset_state();
    void set_pos(const vec3 &pos) { if (phys) phys->pos = pos; }
    void set_rot(const quat &rot) { if (phys) phys->rot = rot; }
    const vec3 &get_pos() { if (phys) return phys->pos; static vec3 p; return p; }
    const quat &get_rot() { if (phys) return phys->rot; static quat r; return r; }
    vec3 get_dir() { return get_rot().rotate(vec3::forward()); }
    void select_target(const object_ptr &o);
    void update(int dt, world &w);
    void update_hud(world &w, gui::hud &h);
    bool is_ecm_active() const { return special.id=="ECM" && ecm_time > 0;}

    virtual void take_damage(int damage, world &w, bool net_src = true) override;

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

typedef ptr<plane> plane_ptr;

//------------------------------------------------------------

struct missile: public object
{
    net_missile_ptr net;
    phys::missile_ptr phys;
    renderer::missile_ptr render;
    ivalue time;
    w_ptr<plane> owner;
    w_ptr<plane> target;
    fvalue homing_angle_cos;
    bvalue is_saam;

    void update_homing(int dt, world &w);
    void update(int dt, world &w);
    void release();
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    void set_location(const char *name);
    void stop_sounds();

    plane_ptr add_plane(const char *preset, const char *player_name, int color, bool player, net_plane_ptr ptr = net_plane_ptr());
    missile_ptr add_missile(const plane_ptr &p, net_missile_ptr ptr = net_missile_ptr());

    void spawn_explosion(const vec3 &pos, float radius, bool net_src = true);
    void spawn_bullet(const char *type, const vec3 &pos, const vec3 &dir, const plane_ptr &owner);

    void respawn(const plane_ptr &p, const vec3 &pos, const quat &rot);

    int get_planes_count() const { return (int)m_planes.size(); }
    plane_ptr get_plane(int idx);
    plane_ptr get_player() { return m_player.lock(); }
    const char *get_player_name() const;

    int get_missiles_count() const { return (int)m_missiles.size(); }
    missile_ptr get_missile(int idx);

    float get_height(float x, float z) const { return m_phys_world.get_height(x, z, false); }

    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    typedef std::function<bool(const plane_ptr &a, const plane_ptr &b)> is_ally_handler;
    void set_ally_handler(is_ally_handler &handler) { m_ally_handler = handler; }

    void on_kill(const plane_ptr &a, const plane_ptr &b);
    typedef std::function<void(const plane_ptr &k, const plane_ptr &v)> on_kill_handler;
    void set_on_kill_handler(on_kill_handler &handler) { m_on_kill_handler = handler; }

    gui::hud &get_hud() { return m_hud; }
    void popup_hit(bool destroyed);
    void popup_miss();

    void update(int dt);

    void set_network(network_interface *n) { m_network = n; }
    network_interface *get_network() { return m_network; }
    bool is_host() const { return !m_network || m_network->is_server(); }
    bool net_data_updated() const { return m_net_data_updated; }

    unsigned int get_net_time() const { return m_network ? m_network->get_time() : 0; }

    sound::source_ptr add_sound(sound::file &f, bool loop = false) { return m_sound_world.add(f, loop); }
    sound::source_ptr add_sound(std::string name, int idx = 0, bool loop = false);
    unsigned int play_sound_ui(std::string name, bool loop = false);
    void stop_sound_ui(unsigned int id);
    void play_sound(std::string name, int idx, vec3 pos);

    world(renderer::world &w, sound::world &s, gui::hud &h): m_render_world(w), m_sound_world(s), m_hud(h), m_network(0) {}

private:
    missile_ptr add_missile(const char *id, const renderer::model &m, bool add_to_phys_world);

private:
    plane_ptr get_plane(const phys::object_ptr &o);
    missile_ptr get_missile(const phys::object_ptr &o);

private:
    std::vector<plane_ptr> m_planes;
    w_ptr<plane> m_player;
    std::vector<missile_ptr> m_missiles;
    renderer::world &m_render_world;
    gui::hud &m_hud;
    phys::world m_phys_world;
    sound::world &m_sound_world;
    sound::pack m_sounds;
    sound::pack m_sounds_ui;

    is_ally_handler m_ally_handler;
    on_kill_handler m_on_kill_handler;

private:
    network_interface *m_network;
    bool m_net_data_updated = false;
};

//------------------------------------------------------------

class game_mode
{
public:
    virtual void update(int dt, const plane_controls &player_controls) {}
    virtual void end() {}

    game_mode(world &w): m_world(w) {}

protected:
    world &m_world;
};

//------------------------------------------------------------
}
