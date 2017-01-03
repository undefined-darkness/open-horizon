//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"
#include "units.h"
#include "util/script.h"

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
    void on_kill(const object_ptr &k, const object_ptr &v);

    static int start_timer(lua_State *state);
    static int setup_timer(lua_State *state);
    static int stop_timer(lua_State *state);

    static int set_active(lua_State *state);
    static int set_path(lua_State *state);
    static int set_follow(lua_State *state);
    static int set_target(lua_State *state);
    static int set_target_search(lua_State *state);
    static int set_align(lua_State *state);
    static int set_speed(lua_State *state);
    static int set_speed_limit(lua_State *state);

    static int get_height(lua_State *state);

    static int destroy(lua_State *state);

    static int setup_radio(lua_State *state);
    static int clear_radio(lua_State *state);
    static int add_radio(lua_State *state);

    static int set_hud_visible(lua_State *state);

    static int mission_clear(lua_State *state);
    static int mission_update(lua_State *state);
    static int mission_fail(lua_State *state);

private:
    object_ptr get_object(std::string id) const;
    void update_zones();

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

    void set_radio_message(const radio_message &m);

private:
    std::map<std::string, std::pair<unit::path, bool> > m_paths;

private:
    struct mission_unit
    {
        std::string name;
        unit_ptr u;
        std::string on_init, on_destroy;
    };

    std::vector<mission_unit> m_units;
    std::string m_player_on_destroy;

private:
    struct zone
    {
        std::string name;
        std::string on_enter, on_leave;
        vec3 pos;
        float radius = 0.0f, radius_sq = 0.0f;
        bool active = true;

        enum display_type
        {
            display_none,
            display_point,
            display_circle,
            display_cylinder
        };

        display_type display;

        std::vector<object_wptr> inside;

        bool is_inside(const object_ptr &p);
    };

    std::vector<zone> m_zones;

private:
    plane_ptr m_player;
    script m_script;
    bool m_finished = false;
    bool m_hide_hud = false;
};

//------------------------------------------------------------
}
