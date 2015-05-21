//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"

namespace gui
{
//------------------------------------------------------------

struct menu_controls
{
    bvalue up;
    bvalue down;
    bvalue prev;
    bvalue next;
};

class menu
{
public:
    void init();
    void draw(const render &r);
    void update(int dt, const menu_controls &controls);

    typedef std::function<void(const std::string &name)> on_action;
    void set_callback(on_action f) { m_on_action = f; }
    std::string get_var(const std::string &name) const;

private:
    void set_hide(bool value) { m_hide = value; }
    void set_screen(const std::string &screen);
    void send_event(const std::string &event);

private:
    bvalue m_hide;
    fonts m_fonts;
    on_action m_on_action;
    std::map<std::string, std::string> m_vars;
    menu_controls m_prev_controls;
    std::vector<std::pair<std::string, std::string> > m_entries;
    uvalue m_selected;
    std::string m_screen;
    std::string m_prev_screen;
};

//------------------------------------------------------------
}
