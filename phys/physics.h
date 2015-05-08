//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "plane_params.h"
#include "memory/shared_ptr.h"
#include "math/quaternion.h"
#include <functional>
#include <vector>

namespace phys
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef params::fvalue fvalue;

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
    vec3 pos;
    quat rot;
    vec3 vel;
};

typedef ptr<object> object_ptr;

//------------------------------------------------------------

struct plane_controls
{
    vec3 rot;
    fvalue throttle;
    fvalue brake;
};

//------------------------------------------------------------

struct plane: public object
{
    fvalue thrust_time;
    vec3 rot_speed;

    plane_controls controls;
    plane_params params;
    //col_mesh mesh;

    void update(int dt);
};

typedef ptr<plane> plane_ptr;

//------------------------------------------------------------

struct missile: public object
{
    void update(int dt);
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    plane_ptr add_plane(const char *name);
    missile_ptr add_missile(const char *name);

    void update(int dt, std::function<void(object_ptr &a, object_ptr &b)> on_hit);

private:
    std::vector<plane_ptr> m_planes;
    std::vector<missile_ptr> m_missiles;
};

//------------------------------------------------------------
}
