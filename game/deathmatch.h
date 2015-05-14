//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class deathmatch: public game_mode
{
public:
    virtual void update(int dt, const plane_controls &player_controls) override {}

    deathmatch(world &w): game_mode(w) {}
};

//------------------------------------------------------------
}
