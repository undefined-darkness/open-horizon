//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "phys/physics.h"
#include "renderer/scene.h"

namespace game
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef params::fvalue fvalue;
typedef params::uvalue uvalue;
typedef params::value<bool> bvalue;

//------------------------------------------------------------

template<typename t> class ptr: public nya_memory::shared_ptr<t>
{
    friend class world;

public:
    ptr(): nya_memory::shared_ptr<t>() {}
    ptr(const ptr &p): nya_memory::shared_ptr<t>(p) {}

private:
    explicit ptr(bool): nya_memory::shared_ptr<t>(t()) {}
};

//------------------------------------------------------------

struct object
{
};

typedef ptr<object> object_ptr;

//------------------------------------------------------------

struct plane_controls: public phys::plane_controls
{
    bvalue missile;
    bvalue mgun;
    bvalue flares;
    bvalue change_weapon;
    bvalue switch_target;
    bvalue change_camera;
};

//------------------------------------------------------------

struct plane: public object
{
    plane_controls controls;
    plane_controls last_controls;
    phys::plane_ptr phys;
    renderer::aircraft_ptr render;

    void set_pos(const vec3 &pos) { if (phys.is_valid()) phys->pos = pos; }
    void set_rot(const quat &rot) { if (phys.is_valid()) phys->rot = rot; }
    const vec3 &get_pos() { if (phys.is_valid()) return phys->pos; static vec3 p; return p; }
    const quat &get_rot() { if (phys.is_valid()) return phys->rot; static quat r; return r; }
};

typedef ptr<plane> plane_ptr;

//------------------------------------------------------------

struct missile: public object
{
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    void set_location(const char *name);

    plane_ptr add_plane(const char *name, int color, bool player);

    int get_planes_count();
    plane_ptr get_plane(int idx);

    void update(int dt);

    world(renderer::world &w): m_render_world(w) {}

private:
    void update_plane(plane_ptr &p);

private:
    std::vector<plane_ptr> m_planes;
    renderer::world &m_render_world;
    phys::world m_phys_world;
};

//------------------------------------------------------------
}
