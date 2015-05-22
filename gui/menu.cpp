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

    set_screen("main");
    m_prev_screen = "exit";

    m_bkg.load("UI/comp_simple_bg.lar");
}

//------------------------------------------------------------

void menu::draw(const render &r)
{
    if (m_hide)
        return;

    nya_render::clear(true, true);

    rect sr(0, 0, r.get_width(), r.get_height());

    const nya_math::vec4 white(1.0, 1.0, 1.0, 1.0);

    if (m_screen == "main")
        m_bkg.draw_tx(r, 0, 0, sr, white);
    else if (m_screen == "ac_select")
        m_bkg.draw_tx(r, 0, 1, sr, white);
    else if (m_screen == "map_select")
        m_bkg.draw_tx(r, 0, 2, sr, white);

    //ToDo
}

//------------------------------------------------------------

void menu::update(int dt, const menu_controls &controls)
{
    if (controls.up && controls.up != m_prev_controls.up)
    {
        if (m_selected > 0)
            --m_selected;
    }

    if (controls.down && controls.down != m_prev_controls.down)
    {
        if (!m_entries.empty())
        {
            if (++m_selected >= m_entries.size())
                m_selected = (unsigned int)m_entries.size() - 1;
        }
    }

    if (controls.prev && controls.prev != m_prev_controls.prev)
    {
        set_screen(m_prev_screen);
    }

    if (controls.next && controls.next != m_prev_controls.next)
    {
        if (m_selected < m_entries.size())
            send_event(m_entries[m_selected].second);
    }

    m_prev_controls = controls;
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
    m_selected = 0;
    m_entries.clear();

    if (screen == "main")
    {
        m_entries.push_back(std::make_pair("DEATHMATCH", "mode_dm"));
        m_entries.push_back(std::make_pair("TEAM DEATHMATCH", "mode_tdm"));
        m_entries.push_back(std::make_pair("FREE FLIGHT", "mode_ff"));
        m_entries.push_back(std::make_pair("EXIT", "exit"));
    }
    else if (screen == "exit")
        send_event("exit");
    else
        printf("unknown screen %s\n", screen.c_str());

    m_prev_screen = screen;
    m_screen = screen;
}

//------------------------------------------------------------

void menu::send_event(const std::string &event)
{
    if (m_on_action)
        m_on_action(event);

    //ToDo
}

//------------------------------------------------------------
}
