//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "ui.h"

namespace gui
{
//------------------------------------------------------------

class menu
{
public:
    void init();
    void draw(const render &r);

    void set_hide(bool value) { m_hide = value; }

    typedef std::function<void(const std::string &name)> on_action;
    void set_callback(on_action f) { m_on_action = f; }
    std::string get_var(const std::string &name) const;

    menu(): m_hide(false) {}

private:
    bool m_hide;
    fonts m_fonts;
    on_action m_on_action;
    std::map<std::string, std::string> m_vars;
};

//------------------------------------------------------------
}
