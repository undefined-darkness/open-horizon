//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

namespace game
{
//------------------------------------------------------------

class team_deathmatch: public game_mode
{
public:
    virtual void update(int dt, const plane_controls &player_controls) override {}

    team_deathmatch(world &w): game_mode(w) {}
};

//------------------------------------------------------------
}
