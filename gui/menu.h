//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"
#include "game/network.h"
#include "sound/sound.h"
#include "util/script.h"
#include <functional>
#include <list>

namespace gui
{
//------------------------------------------------------------

struct menu_controls
{
    bvalue up;
    bvalue down;
    bvalue left;
    bvalue right;
    bvalue prev;
    bvalue next;
};

class menu
{
public:
    void init();
    void draw(const render &r);
    void update(int dt, const menu_controls &controls);
    bool joy_update(const float *axes, int axes_count, const unsigned char *btns, int btns_count); //for joystick config
    void on_input(char c);

    typedef std::function<void(const std::string &name)> on_action;
    void set_callback(on_action f) { m_on_action = f; }
    std::string get_var(const std::string &name) const;
    int get_var_int(const std::string &name) const;
    void send_event(const std::string &event);
    void set_error(const std::string &error);

    menu(sound::world_2d &s): m_sound_world(s) {}

private:
    void set_hide(bool value) { m_hide = value; }
    void set_screen(const std::string &screen);
    void init_var(const std::string &name, const std::string &value);
    void play_sound(std::string name);
    bool load_script();
    void script_call(std::string function, const std::vector<script::value> &values = {});

private:
    static int set_title(lua_State *state);
    static int set_bkg(lua_State *state);
    static int set_bkg_pic(lua_State *state);
    static int set_sel_pic(lua_State *state);
    static int set_font_color(lua_State *state);
    static int add_entry(lua_State *state);
    static int add_input(lua_State *state);
    static int send_event(lua_State *state);
    static int set_history(lua_State *state);

public:
    static int init_var(lua_State *state);
    static int get_var(lua_State *state);
    static int set_var(lua_State *state);

    static int get_aircrafts(lua_State *state);
    static int get_aircraft_colors(lua_State *state);
    static int get_locations(lua_State *state);
    static int get_missions(lua_State *state);
    static int get_campaigns(lua_State *state);

private:
    sound::world_2d &m_sound_world;
    sound::pack m_sounds;

    bvalue m_hide;
    fonts m_fonts;
    on_action m_on_action;
    std::map<std::string, std::string> m_vars;
    menu_controls m_prev_controls;
    std::wstring m_title;
    int m_current_bkg = -1;
    nya_scene::texture m_bkg_pic;
    nya_scene::texture m_select_pic;
    std::vector<std::wstring> m_desc;
    std::wstring m_error;

    struct entry
    {
        std::wstring name;
        std::vector<std::string> events;

        std::vector<std::pair<std::wstring, std::string> > sub_select;
        std::string sub_event;
        std::vector<std::string> sub_events;
        uvalue sub_selected;
        bool allow_input = false;
        bool input_numeric = false;
        std::wstring input;
        std::string input_event;
    };

    void send_sub_events(const entry &e);

    void add_entry(const std::wstring &name, const std::vector<std::string> &events,
                   const std::string &sub_event = "", const std::vector<std::string> &sub_events = {});
    void add_sub_entry(const std::wstring &name, const std::string &value);
    void add_input(const std::string &event, bool numeric_only = false, bool allow_input = true);

    std::vector<entry> m_entries;
    uvalue m_selected;
    std::list<std::string> m_screens;
    std::map<std::string, std::wstring> m_choices;
    std::vector<std::string> m_back_events;

    tiles m_bkg;
    tiles m_select;

    game::servers_list m_servers_list;

    script m_script;
    nya_math::vec4 m_font_color;
    bool m_need_load_campaign = false;
};

//------------------------------------------------------------
}
