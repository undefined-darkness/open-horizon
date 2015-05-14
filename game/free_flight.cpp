//
// open horizon -- undefined_darkness@outlook.com
//

#include "free_flight.h"

namespace game
{
//------------------------------------------------------------

void free_flight::start(const char *plane, int color, const char *location)
{
    set_plane(plane, color);
    set_location(location);
}

//------------------------------------------------------------

void free_flight::process(int dt)
{
}

//------------------------------------------------------------

void free_flight::end()
{
    m_player.free();
}

//------------------------------------------------------------

void free_flight::set_location(const char *location)
{
    if (!location)
        return;

    m_world.set_location(location);
    if (!m_player.is_valid())
        return;

    m_player->set_pos(nya_math::vec3(-300, 50, 2000));
    m_player->set_rot(nya_math::quat());
}

//------------------------------------------------------------

void free_flight::set_plane(const char *plane, int color)
{
    if (!plane)
        return;

    nya_math::vec3 p;
    nya_math::quat r;

    if (m_player.is_valid())
    {
        p = m_player->get_pos();
        r = m_player->get_rot();
    }

    m_player = m_world.add_plane(plane, color, true);
    m_player->set_pos(p);
    m_player->set_rot(r);

    m_world.get_hud().load(plane);
}

//------------------------------------------------------------
}
