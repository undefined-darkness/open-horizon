//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"
#include "script.h"

namespace game
{
//------------------------------------------------------------

class mission: public game_mode
{
public:
    virtual void start(const char *plane, int color, const char *mission);
    virtual void update(int dt, const plane_controls &player_controls) override;
    virtual void end() override;

    mission(world &w): game_mode(w) {}

protected:
    static bool is_ally(const plane_ptr &a, const plane_ptr &b) { return true; }

    static int mission_clear(lua_State *state);
    static int mission_fail(lua_State *state);

    static int start_timer(lua_State *state);
    static int setup_timer(lua_State *state);
    static int stop_timer(lua_State *state);

    plane_ptr m_player;
    script m_script;
    bool m_finished = false;
};

//------------------------------------------------------------
}
