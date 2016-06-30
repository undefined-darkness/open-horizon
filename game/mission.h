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

    static int start_timer(lua_State *state);
    static int setup_timer(lua_State *state);
    static int stop_timer(lua_State *state);

    static int set_active(lua_State *state);
    static int set_path(lua_State *state);
    static int set_follow(lua_State *state);
    static int set_target(lua_State *state);
    static int set_align(lua_State *state);

    static int get_height(lua_State *state);

    static int destroy(lua_State *state);

    static int setup_radio(lua_State *state);
    static int clear_radio(lua_State *state);
    static int add_radio(lua_State *state);

    static int set_hud_visible(lua_State *state);

    static int mission_clear(lua_State *state);
    static int mission_fail(lua_State *state);

private:
    struct timer
    {
        std::string id;
        std::wstring name;
        int time = 0;
        std::string func;
        bool active = false;
    };

    std::vector<timer> m_timers;

private:
    std::map<std::string, std::pair<std::wstring, gui::hud::color> > m_radio;

    struct radio_message { std::string id; std::wstring message; int time; };
    std::vector<radio_message> m_radio_messages;

private:
    std::map<std::string, std::pair<std::vector<vec3>, bool> > m_paths;

private:
    plane_ptr m_player;
    script m_script;
    bool m_finished = false;
    bool m_hide_hud = false;
};

//------------------------------------------------------------
}
