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

 //ToDo
static const int missile_damage = 80;
static const float missile_dmg_radius = 20;
static const float bomb_dmg_radius = 80;

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

struct plane;
typedef ptr<plane> plane_ptr;

//------------------------------------------------------------

struct object
{
    virtual vec3 get_pos() { return vec3(); }
    virtual vec3 get_vel() { return vec3(); }
    virtual quat get_rot() { return quat(); }
    virtual std::wstring get_name() { return L""; }
    virtual std::wstring get_type_name() { return L""; }
    virtual bool get_tgt() { return false; }
    virtual bool is_targetable(bool air, bool ground) { return false; }
    virtual bool is_ally(const plane_ptr &p, world &w) { return false; }

    ivalue hp, max_hp;
    virtual void take_damage(int damage, world &w, bool net_src = true) { hp = damage < hp ? hp - damage : 0; }
    virtual float get_hit_radius() { return 0.0f; }
};

typedef w_ptr<object> object_wptr;
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
    fvalue speed_init;
    fvalue gravity;

    wpn_params() {}
    wpn_params(std::string id, std::string model);
};

//------------------------------------------------------------

struct missile
{
    net_missile_ptr net;
    phys::missile_ptr phys;
    renderer::missile_ptr render;
    ivalue time;
    w_ptr<plane> owner;
    object_wptr target;
    fvalue homing_angle_cos;
    fvalue dmg_radius;
    fvalue dmg;

    enum track_mode
    {
        mode_missile,
        mode_saam,
        mode_4agm,
        mode_lagm
    };

    track_mode mode = mode_missile;

    void update_homing(int dt, world &w);
    void update(int dt, world &w);
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

struct bomb
{
    phys::bomb_ptr phys;
    renderer::object_ptr render;
    w_ptr<plane> owner;
    object_wptr target;
    fvalue dmg_radius;
    fvalue dmg;
    bvalue dead;

    void update(int dt, world &w);
};

typedef ptr<bomb> bomb_ptr;

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

inline bool line_sphere_intersect(const vec3 &start, const vec3 &end, const vec3 &sp_center, const float sp_radius)
{
    const vec3 diff = end - start;
    const float len = nya_math::clamp(diff.dot(sp_center - start) / diff.length_sq(), 0.0f, 1.0f);
    const vec3 inter_pt = start + len  * diff;
    return (inter_pt - sp_center).length_sq() <= sp_radius * sp_radius;
}


//------------------------------------------------------------

inline const params::text_params &get_arms_param() { static params::text_params ap("Arms/ArmsParam.txt"); return ap; }

//------------------------------------------------------------
}

