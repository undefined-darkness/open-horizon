//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/physics.h"
#include "renderer/scene.h"
#include "gui/hud.h"
#include "network.h"
#include <memory>
#include <list>

namespace game
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef params::fvalue fvalue;
typedef params::uvalue uvalue;
typedef params::value<int> ivalue;
typedef params::value<bool> bvalue;

class world;

//------------------------------------------------------------

template <typename t> using ptr = std::shared_ptr<t>;
template <typename t> using w_ptr = std::weak_ptr<t>;

//------------------------------------------------------------

struct object
{
    ivalue max_hp;
    ivalue hp;

    virtual void take_damage(int damage, world &w) { hp = damage < hp ? hp - damage : 0; }
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
};

//------------------------------------------------------------

struct wpn_missile_params
{
    std::string id, model;
    fvalue lockon_range;
    fvalue lockon_angle_cos;
    ivalue lockon_reload;
    ivalue lockon_count;
    bvalue lockon_air;
    bvalue lockon_ground;

    wpn_missile_params() {}
    wpn_missile_params(std::string id, std::string model);
};

//------------------------------------------------------------

struct plane: public object
{
    plane_controls controls;
    plane_controls last_controls;
    phys::plane_ptr phys;
    renderer::aircraft_ptr render;
    bvalue special_weapon;
    bvalue need_fire_missile;
    ivalue rocket_bay_time;
    ivalue mgun_fire_update;

    wpn_missile_params missile;
    ivalue missile_cooldown[2];
    std::vector<ivalue> missile_mount_cooldown;
    ivalue missile_mount_idx;

    wpn_missile_params special;
    ivalue special_cooldown[2];
    std::vector<ivalue> special_mount_cooldown;
    ivalue special_mount_idx;

    std::vector<nya_math::vec3> alert_dirs;

    struct target_lock
    {
        w_ptr<plane> target_plane;
        bvalue locked;
    };

    std::list<target_lock> targets;

    void reset_state();
    void set_pos(const vec3 &pos) { if (phys) phys->pos = pos; }
    void set_rot(const quat &rot) { if (phys) phys->rot = rot; }
    const vec3 &get_pos() { if (phys) return phys->pos; static vec3 p; return p; }
    const quat &get_rot() { if (phys) return phys->rot; static quat r; return r; }
    vec3 get_dir() { static vec3 fw(0.0, 0.0, 1.0); return get_rot().rotate(fw); }
    void select_target(const object_ptr &o);
    void update(int dt, world &w, gui::hud &h, bool player);

    virtual void take_damage(int damage, world &w) override;
};

typedef ptr<plane> plane_ptr;

//------------------------------------------------------------

struct missile: public object
{
    phys::missile_ptr phys;
    renderer::missile_ptr render;
    ivalue time;
    w_ptr<plane> target;
    fvalue homing_angle_cos;

    void update_homing(int dt);
    void update(int dt, world &w);
    void release();
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    void set_location(const char *name);

    plane_ptr add_plane(const char *name, int color, bool player);
    missile_ptr add_missile(const char *id, const char *model);

    void spawn_explosion(const nya_math::vec3 &pos, int damage, float radius);
    void spawn_bullet(const char *type, const nya_math::vec3 &pos, const nya_math::vec3 &dir);

    int get_planes_count() const { return (int)m_planes.size(); }
    plane_ptr get_plane(int idx);

    int get_missiles_count() const { return (int)m_missiles.size(); }
    missile_ptr get_missile(int idx);

    float get_height(float x, float z) const { return m_phys_world.get_height(x, z); }

    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    typedef std::function<bool(const plane_ptr &a, const plane_ptr &b)> is_ally_handler;
    void set_ally_handler(is_ally_handler &handler) { m_ally_handler = handler; }

    gui::hud &get_hud() { return m_hud; }

    void update(int dt);

    world(renderer::world &w, gui::hud &h, network_client &c, network_server &s):
                           m_render_world(w), m_hud(h), m_client(c), m_server(s) {}
private:
    void update_plane(plane_ptr &p);
    plane_ptr get_plane(const phys::object_ptr &o);
    missile_ptr get_missile(const phys::object_ptr &o);

private:
    std::vector<plane_ptr> m_planes;
    std::vector<missile_ptr> m_missiles;
    renderer::world &m_render_world;
    gui::hud &m_hud;
    phys::world m_phys_world;

    is_ally_handler m_ally_handler;

private:
    network_client &m_client;
    network_server &m_server;
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
