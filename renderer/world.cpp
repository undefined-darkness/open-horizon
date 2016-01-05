//
// open horizon -- undefined_darkness@outlook.com
//

#include "world.h"
#include "shared.h"
#include "render/fbo.h"
#include "containers/dpl.h"
#include "render/screen_quad.h"
#include <algorithm>

namespace renderer
{
//------------------------------------------------------------

object_ptr world::add_object(const char *name)
{
    object_ptr o(true);
    o->mdl.load(name, m_location.get_params());
    m_objects.push_back(o);
    return o;
}

//------------------------------------------------------------

aircraft_ptr world::add_aircraft(const char *name, int color, bool player)
{
    aircraft_ptr a(true);
    a->load(name, color, m_location.get_params(), player);
    a->apply_location(m_location_name.c_str(), m_location.get_params());

    if (player)
        m_player_aircraft = a;

    m_aircrafts.push_back(a);
    return a;
}

//------------------------------------------------------------

missile_ptr world::add_missile(const char *name)
{
    missile_ptr m(true);
    m->mdl.load((std::string("w_") + name).c_str(), m_location.get_params());
    m_missiles.push_back(m);
    return m;
}

//------------------------------------------------------------

void world::set_location(const char *name)
{
    if (name && m_location_name == name)
        return;

    m_location_name.assign(name?name:"");

    m_location = location();
    m_clouds = effect_clouds();

    shared::clear_textures();

    m_location.load(m_location_name.c_str());
    if (m_location_name != "def" && !m_location_name.empty())
        m_clouds.load(m_location_name.c_str(), m_location.get_params());
    if (m_player_aircraft.is_valid())
        m_player_aircraft->apply_location(m_location_name.c_str(), m_location.get_params());
}

//------------------------------------------------------------

void world::update(int dt)
{
    m_location.update(dt);

    if (m_player_aircraft.get_ref_count() == 2)
        m_player_aircraft.free();

    for (auto &o: m_objects) o->mdl.update(dt);

    m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), [](object_ptr &o)
                                    { return o.get_ref_count() <= 1; }), m_objects.end());

    m_aircrafts.erase(std::remove_if(m_aircrafts.begin(), m_aircrafts.end(), [](aircraft_ptr &a){ return a.get_ref_count() <= 1; }), m_aircrafts.end());
    for (auto &a: m_aircrafts) a->update(dt);

    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), [](missile_ptr &m)
                                    { if (m.get_ref_count() > 1)
                                          return false; m->release();
                                      return true;
                                    }), m_missiles.end());

    for (auto &m: m_missiles) m->update(dt);
}

//------------------------------------------------------------

void missile::update(int dt)
{
    if (engine_started)
        trail.update(mdl.get_pos(), dt);
}

//------------------------------------------------------------

void missile::release()
{
}

//------------------------------------------------------------
}
