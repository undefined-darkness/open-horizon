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

aircraft_ptr world::add_aircraft(const char *name, int color, bool player)
{
    aircraft_ptr a(true);
    a->load(name, color, m_location.get_params());
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
    m_location_name.assign(name);
    m_location = location();
    m_clouds = effect_clouds();

    shared::clear_textures();

    m_location.load(name);
    m_clouds.load(name, m_location.get_params());
    if (m_player_aircraft.is_valid())
        m_player_aircraft->apply_location(m_location_name.c_str(), m_location.get_params());
}

//------------------------------------------------------------

void world::update(int dt)
{
    m_location.update(dt);

    if (m_player_aircraft.get_ref_count() == 2)
        m_player_aircraft.free();

    m_aircrafts.erase(std::remove_if(m_aircrafts.begin(), m_aircrafts.end(), [](aircraft_ptr &a){ return a.get_ref_count() <= 1; }), m_aircrafts.end());
    for (auto &a: m_aircrafts) a->update(dt);

    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), [](missile_ptr &m){ return m.get_ref_count() <= 1; }), m_missiles.end());
}

//------------------------------------------------------------
}
