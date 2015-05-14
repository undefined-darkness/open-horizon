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
    virtual void process(int dt) override {}

    team_deathmatch(world &w): game_mode(w) {}
};

//------------------------------------------------------------
}
