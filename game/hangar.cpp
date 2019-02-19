//
// open horizon -- undefined_darkness@outlook.com
//

#include "hangar.h"

namespace game
{
//------------------------------------------------------------

void hangar::update(int dt)
{
    m_render_world.update(dt);
}

//------------------------------------------------------------

void hangar::end()
{
    m_plane.free();
    m_object.free();
    //m_render_world.set_location("");
    m_render_world.update(0);
}

//------------------------------------------------------------

void hangar::set_bkg(const char *name)
{
    m_render_world.set_location("def");

    m_object = m_render_world.add_object((std::string("bg_")+name).c_str());
}

//------------------------------------------------------------

void hangar::set_plane(const char *plane)
{
    m_plane_name.clear();
    if (!plane)
        return;

    m_plane_name = plane;
    m_plane = m_render_world.add_aircraft(m_plane_name.c_str(), 0, true);
    m_plane->set_pos(nya_math::vec3(0.0f, 3.0f, 0.0f));
}

//------------------------------------------------------------

void hangar::set_plane_color(int color)
{
    if (m_plane_name.empty())
        return;

    m_plane = m_render_world.add_aircraft(m_plane_name.c_str(), color, true);
    m_plane->set_pos(nya_math::vec3(0.0f, 3.0f, 0.0f));
}

//------------------------------------------------------------
}
