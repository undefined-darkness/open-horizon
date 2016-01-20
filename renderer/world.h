//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "aircraft.h"
#include "util/params.h"
#include "memory/shared_ptr.h"
#include "location.h"
#include "model.h"
#include "clouds.h"
#include "missile_trail.h"
#include "particles.h"
#include <functional>
#include <vector>

namespace renderer
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;
typedef nya_math::quat quat;
typedef params::fvalue fvalue;
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
    model mdl;
};

typedef ptr<object> object_ptr;

//------------------------------------------------------------

typedef ptr<aircraft> aircraft_ptr;

//------------------------------------------------------------

struct missile: public object
{
    bvalue engine_started;
    missile_trail trail;
    void update(int dt);
};

typedef ptr<missile> missile_ptr;

//------------------------------------------------------------

class world
{
public:
    virtual void set_location(const char *name);
    const location_params &get_location_params() { return m_location.get_params(); }
    const char *get_location_name() { return m_location_name.c_str(); }

    object_ptr add_object(const char *name);

    virtual aircraft_ptr add_aircraft(const char *name, int color, bool player);
    aircraft_ptr get_player_aircraft() { return m_player_aircraft; }

    missile_ptr add_missile(const char *name);

    void spawn_explosion(const nya_math::vec3 &pos,float radius);

    virtual void update(int dt);

protected:
    virtual void add_trail(const missile_trail &t) {}

protected:
    std::vector<object_ptr> m_objects;
    std::vector<aircraft_ptr> m_aircrafts;
    std::vector<missile_ptr> m_missiles;
    std::vector<explosion> m_explosions;

protected:
    std::string m_location_name;
    location m_location;
    effect_clouds m_clouds;
    aircraft_ptr m_player_aircraft;
};

//------------------------------------------------------------
}
