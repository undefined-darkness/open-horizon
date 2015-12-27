//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "renderer/world.h"

namespace game
{
//------------------------------------------------------------

class hangar
{
public:
    void update(int dt);
    void end();

public:
    void set_bkg(const char *name);
    void set_plane(const char *plane);
    void set_plane_color(int color);

public:
    hangar(renderer::world &w): m_render_world(w) {}

private:
    renderer::world &m_render_world;
    renderer::aircraft_ptr m_plane;
    std::string m_plane_name;
};

//------------------------------------------------------------
}
