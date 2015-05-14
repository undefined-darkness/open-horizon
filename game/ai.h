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
    void update(const world &w, const plane_ptr &plane, int dt, plane_controls &controls);
};

//------------------------------------------------------------
}
