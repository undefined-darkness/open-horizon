//
// open horizon -- undefined_darkness@outlook.com
//

#include "menu.h"

namespace gui
{

//------------------------------------------------------------

void menu::init()
{
    *this = menu(); //release

    //ToDo

    set_screen("main");
}

//------------------------------------------------------------

void menu::draw(const render &r)
{
    if (m_hide)
        return;

    //ToDo
}

//------------------------------------------------------------

void update(int dt, const menu_controls &controls)
{
    //ToDo
}

//------------------------------------------------------------

std::string menu::get_var(const std::string &name) const
{
    auto v = m_vars.find(name);
    if (v == m_vars.end())
        return std::string();

    return v->second;
}

//------------------------------------------------------------

void menu::set_screen(const std::string &screen)
{
    //ToDo
}

//------------------------------------------------------------
}
