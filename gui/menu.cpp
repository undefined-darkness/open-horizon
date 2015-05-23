//
// open horizon -- undefined_darkness@outlook.com
//

#include "menu.h"
#include "renderer/aircraft.h"

namespace gui
{

//------------------------------------------------------------

void menu::init()
{
    *this = menu(); //release

    set_screen("main");

    m_fonts.load("UI/text/menuCommon.acf");
    m_bkg.load("UI/comp_simple_bg.lar");
    m_select.load("UI/comp_menu.lar");
}

//------------------------------------------------------------

void menu::draw(const render &r)
{
    if (m_hide)
        return;

    nya_render::clear(true, true);

    if (m_screens.empty())
        return;

    rect sr(0, 0, r.get_width(), r.get_height());
    const nya_math::vec4 white(1.0, 1.0, 1.0, 1.0);
    const nya_math::vec4 font_color(89 / 255.0, 203 / 255.0, 136 / 255.0, 1.0);

    if (m_screens.back() == "main")
        m_bkg.draw_tx(r, 0, 0, sr, white);
    else if (m_screens.back() == "map_select")
        m_bkg.draw_tx(r, 0, 2, sr, white);
    else if (m_screens.back() == "ac_select" || m_screens.back() == "color_select")
        m_bkg.draw_tx(r, 0, 1, sr, white);

    int x = 155, y = 162;
    m_fonts.draw_text(r, m_title.c_str(), "NowGE24", x, 80, font_color);
    const int count = 15;
    for (int i = ((m_selected + (m_selected >= (count - 1) ? 1 : 0)) / (count - 1)) * (count - 2), j = 0;
         i < (int)m_entries.size() && j < count; ++i, ++j)
    {
        auto &e = m_entries[i];
        if (m_selected == i)
        {
            rect rct;
            rct.x = x - 20, rct.y = y - 8;
            rct.w = 550, rct.h = 34;
            m_select.draw_tx(r, 0, 3, rct, white);
        }

        m_fonts.draw_text(r, e.first.c_str(), "NowGE24", x, y, font_color);
        y += 34;
    }

    //m_fonts.draw_text(r, L"ASDFGHJKLasdfghjklQWERTYUIOPqwertyuiopZXCVBNMzxcvbnm\"\'*_-=.,0123456789", "NowGE24", 50, 200, white);
    //m_fonts.draw_text(r, L"This is a test. The quick brown fox jumps over the lazy dog's back 1234567890", "NowGE24", 50, 100, white);
    //m_fonts.draw_text(r, L"テストです。いろはにほへと ちりぬるを わかよたれそ つねならむ うゐのおくやま けふこえて あさきゆめみし えひもせす。", "ShinGo18outline", 50, 150, white);
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
        if (!m_screens.empty())
        {
            m_screens.pop_back();
            if (m_screens.empty())
                send_event("exit");
            else
            {
                auto last = m_screens.back();
                m_screens.pop_back();
                set_screen(last);
            }
        }
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
        m_title = L"MAIN MENU";
        m_entries.push_back(std::make_pair(L"DEATHMATCH", "mode=dm"));
        m_entries.push_back(std::make_pair(L"TEAM DEATHMATCH", "mode=tdm"));
        m_entries.push_back(std::make_pair(L"FREE FLIGHT", "mode=ff"));
        m_entries.push_back(std::make_pair(L"EXIT", "exit"));
    }
    else if (screen == "map_select")
    {
        m_title = L"LOCATION";
        m_entries.push_back(std::make_pair(L"MIAMI", "map=ms01"));
        m_entries.push_back(std::make_pair(L"DUBAI", "map=ms06"));
        m_entries.push_back(std::make_pair(L"MOSCOW", "map=ms11b"));
        //m_entries.push_back(std::make_pair(L"WASHINGTON", "map=ms14"));
        m_entries.push_back(std::make_pair(L"PARIS", "map=ms30"));
        m_entries.push_back(std::make_pair(L"TOKYO", "map=ms50"));
    }
    else if (screen == "ac_select")
    {
        m_title = L"AIRCRAFT";
        m_entries.push_back(std::make_pair(L"F14D", "ac=f14d"));
        m_entries.push_back(std::make_pair(L"F15C", "ac=f15c"));
        m_entries.push_back(std::make_pair(L"F16C", "ac=f16c"));
        m_entries.push_back(std::make_pair(L"F18F", "ac=f18f"));
        m_entries.push_back(std::make_pair(L"F22A", "ac=f22a"));
        m_entries.push_back(std::make_pair(L"MIG29A", "ac=m29a"));
        m_entries.push_back(std::make_pair(L"PAK FA", "ac=pkfa"));
        m_entries.push_back(std::make_pair(L"SU33", "ac=su33"));
        m_entries.push_back(std::make_pair(L"SU35", "ac=su35"));
        m_entries.push_back(std::make_pair(L"SU37", "ac=su37"));
        m_entries.push_back(std::make_pair(L"SU47", "ac=su47"));
        m_entries.push_back(std::make_pair(L"TYPHOON", "ac=typn"));

        if (get_var("mode") == "ff")
        {
            m_entries.push_back(std::make_pair(L"A10A", "ac=a10a"));
            m_entries.push_back(std::make_pair(L"AV8B", "ac=ab8b"));
            m_entries.push_back(std::make_pair(L"ASF X", "ac=kwmr"));
            m_entries.push_back(std::make_pair(L"B01B", "ac=b01b"));
            m_entries.push_back(std::make_pair(L"B02A", "ac=b02a"));
            m_entries.push_back(std::make_pair(L"F2A", "ac=f02a"));
            m_entries.push_back(std::make_pair(L"F4E", "ac=f04e"));
            m_entries.push_back(std::make_pair(L"F15M", "ac=f15m"));
            m_entries.push_back(std::make_pair(L"F15E", "ac=f15e"));
            m_entries.push_back(std::make_pair(L"F16F", "ac=f16f"));
            m_entries.push_back(std::make_pair(L"F35B", "ac=f35b"));
            m_entries.push_back(std::make_pair(L"F117A", "ac=f17a"));
            m_entries.push_back(std::make_pair(L"JAS39C", "ac=j39c"));
            m_entries.push_back(std::make_pair(L"M21BIS", "ac=m21b"));
            m_entries.push_back(std::make_pair(L"MIRAGE2000", "ac=mr2k"));
            m_entries.push_back(std::make_pair(L"RAFALE M", "ac=rflm"));
            m_entries.push_back(std::make_pair(L"SU24MP", "ac=su24"));
            m_entries.push_back(std::make_pair(L"SU25", "ac=su25"));
            m_entries.push_back(std::make_pair(L"SU34", "ac=su34"));
            m_entries.push_back(std::make_pair(L"TORNADO GR4", "ac=tnd4"));
        }

        /*
        //no anims
        m_entries.push_back(std::make_pair(L"YF23", "ac=yf23"));
        m_entries.push_back(std::make_pair(L"FA44", "ac=fa44"));

        //helicopters are not yet supported
        m_entries.push_back(std::make_pair(L"AH64", "ac=ah64"));
        m_entries.push_back(std::make_pair(L"MI24", "ac=mi24"));
        m_entries.push_back(std::make_pair(L"KA50", "ac=ka50"));
        m_entries.push_back(std::make_pair(L"MH60", "ac=mh60"));

        //not yet supported
        m_entries.push_back(std::make_pair(L"A130", "ac=a130")); //b.unknown2 = 1312
        */
    }
    else if (screen == "color_select")
    {
        m_title = L"AIRCRAFT COLOR";
        const int colors_count = renderer::aircraft::get_colors_count(m_vars["ac"].c_str());
        for (int i = 0; i < colors_count; ++i)
        {
            wchar_t name[255];
            char action[255];
            swprintf(name, 255, L"COLOR%02d", i);
            sprintf(action, "color=%d", i);
            m_entries.push_back(std::make_pair(name, action));
        }
    }
    else
        printf("unknown screen %s\n", screen.c_str());

    m_screens.push_back(screen);
}

//------------------------------------------------------------

void menu::send_event(const std::string &event)
{
    auto eq = event.find("=");
    if (eq != std::string::npos)
    {
        auto var = event.substr(0, eq);
        m_vars[var] = event.substr(eq + 1);

        if (var == "mode")
            set_screen("map_select");
        else if (var == "map")
            set_screen("ac_select");
        else if (var == "ac")
            set_screen("color_select");
        else if (var == "color")
            send_event("start"); //ToDo: color and special select
        return;
    }

    if (m_on_action)
        m_on_action(event);
}

//------------------------------------------------------------
}
