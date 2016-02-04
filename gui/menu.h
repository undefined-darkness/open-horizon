//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"
#include "game/network.h"
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
    void on_input(char c);

    typedef std::function<void(const std::string &name)> on_action;
    void set_callback(on_action f) { m_on_action = f; }
    std::string get_var(const std::string &name) const;
    int get_var_int(const std::string &name) const;

private:
    void set_hide(bool value) { m_hide = value; }
    void set_screen(const std::string &screen);
    void send_event(const std::string &event);
    void init_var(const std::string &name, const std::string &value);

private:
    bvalue m_hide;
    fonts m_fonts;
    on_action m_on_action;
    std::map<std::string, std::string> m_vars;
    menu_controls m_prev_controls;
    std::wstring m_title;

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
    void add_input(const std::string &event, bool numeric_only = false);

    std::vector<entry> m_entries;
    uvalue m_selected;
    std::list<std::string> m_screens;
    std::vector<std::string> m_back_events;

    tiles m_bkg;
    tiles m_select;

    game::servers_list m_servers_list;
};

//------------------------------------------------------------
}
