//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class ai
{
public:
    void set_plane(plane_ptr &p) { m_plane = p; }
    void update(const world &w, int dt);

private:
    w_ptr<plane> m_plane;
};

//------------------------------------------------------------
}
