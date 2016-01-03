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

    int x = 155, y = 155;
    m_fonts.draw_text(r, m_title.c_str(), "ZurichBD_M", x, 80, font_color);
    const int count = 15;
    for (int i = ((m_selected + (m_selected >= (count - 1) ? 1 : 0)) / (count - 1)) * (count - 2), j = 0;
         i < (int)m_entries.size() && j < count; ++i, ++j)
    {
        auto &e = m_entries[i];
        if (m_selected == i)
        {
            rect rct;
            rct.x = x - 20, rct.y = y;
            rct.w = 550, rct.h = 34;
            m_select.draw_tx(r, 0, 3, rct, white);
        }

        auto off = m_fonts.draw_text(r, e.name.c_str(), "ZurichBD_M", x, y, font_color);
        if (!e.sub_select.empty())
            m_fonts.draw_text(r, e.sub_select[e.sub_selected].first.c_str(), "ZurichBD_M", x + off, y, font_color);

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
            if (m_selected + 1 < m_entries.size())
                ++m_selected;
        }
    }

    if (controls.left && controls.left != m_prev_controls.left)
    {
        if (m_selected<m_entries.size())
        {
            auto &e = m_entries[m_selected];
            if (e.sub_selected > 0)
            {
                --e.sub_selected;
                send_sub_events(e);
            }
        }
    }

    if (controls.right && controls.right != m_prev_controls.right)
    {
        if (m_selected<m_entries.size())
        {
            auto &e = m_entries[m_selected];
            if (e.sub_selected + 1 < e.sub_select.size())
            {
                ++e.sub_selected;
                send_sub_events(e);
            }
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

                auto events = m_back_events;
                for (auto &e: events)
                    send_event(e);

                set_screen(last);
            }
        }
    }

    if (controls.next && controls.next != m_prev_controls.next)
    {
        if (m_selected < m_entries.size())
        {
            auto events = m_entries[m_selected].events;
            for (auto &e: events)
                send_event(e);
        }
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

void menu::add_entry(const std::wstring &name, const std::vector<std::string> &events,
                     const std::string &sub_event, const std::vector<std::string> &sub_events)
{
    m_entries.resize(m_entries.size() + 1);
    m_entries.back().name = name;
    m_entries.back().sub_event = sub_event;
    m_entries.back().events = events;
    m_entries.back().sub_events = sub_events;
}

//------------------------------------------------------------

void menu::add_sub_entry(const std::wstring &name, const std::string &value)
{
    if (m_entries.empty())
        return;

    m_entries.back().sub_select.push_back({name, value});
}

//------------------------------------------------------------

void menu::send_sub_events(const entry &e)
{
    if (!e.sub_event.empty() && e.sub_selected < e.sub_select.size())
        send_event(e.sub_event + '=' + e.sub_select[e.sub_selected].second);

    for (auto &e: e.sub_events)
        send_event(e);
}

//------------------------------------------------------------

void menu::set_screen(const std::string &screen)
{
    m_selected = 0;
    m_entries.clear();
    m_back_events.clear();

    if (screen == "main")
    {
        m_title = L"MAIN MENU";
        add_entry(L"Deathmatch", {"mode=dm", "screen=map_select"});
        add_entry(L"Team deathmatch", {"mode=tdm", "screen=map_select"});
        add_entry(L"Free flight", {"mode=ff", "screen=map_select"});
        add_entry(L"Aircraft viewer", {"mode=none", "screen=ac_view"});
        add_entry(L"Exit", {"exit"});
    }
    else if (screen == "map_select")
    {
        m_title = L"LOCATION";
        add_entry(L"Miami", {"map=ms01"});
        add_entry(L"Dubai", {"map=ms06"});
        add_entry(L"Moscow", {"map=ms11b"});
        add_entry(L"Paris", {"map=ms30"});
        add_entry(L"Tokyo", {"map=ms50"});
        add_entry(L"Honolulu", {"map=ms51"});
/*
        add_entry(L"02", {"map=ms02"}); //inferno //oil day
        //add_entry(L"03 Eastern Africa", {"map=ms03"}); //red moon //tex indices idx < size assert
        //add_entry(L"04 Mogadiyu", {"map=ms04"}); //spooky //tex indices idx < size assert
        add_entry(L"05", {"map=ms05"}); //oil night - blue on blue
        //add_entry(L"07 Suez Canal", {"map=ms07"}); //lock n load //tex indices idx < size assert
        add_entry(L"08 Derbent", {"map=ms08"}); //pipeline
        add_entry(L"08X Belyi Base", {"map=ms08x"}); //siege
        add_entry(L"09 Black Sea", {"map=ms09"}); //hostile fleet
        add_entry(L"10 Caucasus Region", {"map=ms10"}); //launch
        //add_entry(L"11A Moscow", {"map=ms11a"}); //motherland //tex indices idx < size assert
        //add_entry(L"12 Miami", {"map=ms12"}); //homefront //type 8 chunk assert, kinda small
        add_entry(L"12T", {"map=ms12t"});
        add_entry(L"13 Florida Coast", {"map=ms13"}); //hurricane
        add_entry(L"Washington", {"map=ms14"});
        //add_entry(L"OP", {"map=msop"}); //tex indices idx < size assert
*/
        for (auto &e: m_entries)
            e.events.push_back("screen=ac_select");
    }
    else if (screen == "ac_select")
    {
        m_title = L"AIRCRAFT";
        add_entry(L"F-14D", {"ac=f14d"});
        add_entry(L"F-15C", {"ac=f15c"});
        add_entry(L"F-16C", {"ac=f16c"});
        add_entry(L"F-18F", {"ac=f18f"});
        add_entry(L"F-22A", {"ac=f22a"});
        add_entry(L"Mig-29A", {"ac=m29a"});
        add_entry(L"PAK FA", {"ac=pkfa"});
        add_entry(L"Su-33", {"ac=su33"});
        add_entry(L"Su-35", {"ac=su35"});
        add_entry(L"Su-37", {"ac=su37"});
        add_entry(L"Su-47", {"ac=su47"});
        add_entry(L"Typhoon", {"ac=typn"});

        //ToDo: aircraft lists

        if (get_var("mode") == "ff")
        {
            add_entry(L"A-10A", {"ac=a10a"});
            add_entry(L"AV-8B", {"ac=av8b"});
            add_entry(L"ASF X", {"ac=kwmr"});
            add_entry(L"B-01B", {"ac=b01b"});
            add_entry(L"B-02A", {"ac=b02a"});
            add_entry(L"F-2A", {"ac=f02a"});
            add_entry(L"F-4E", {"ac=f04e"});
            add_entry(L"F-15M", {"ac=f15m"});
            add_entry(L"F-15E", {"ac=f15e"});
            add_entry(L"F-16F", {"ac=f16f"});
            add_entry(L"F-35B", {"ac=f35b"});
            add_entry(L"F-117A", {"ac=f17a"});
            add_entry(L"JAS39C", {"ac=j39c"});
            add_entry(L"Mig-21Bis", {"ac=m21b"});
            add_entry(L"Mirage 2000", {"ac=mr2k"});
            add_entry(L"Rafale M", {"ac=rflm"});
            add_entry(L"Su-24MP", {"ac=su24"});
            add_entry(L"Su-25", {"ac=su25"});
            add_entry(L"Su-34", {"ac=su34"});
            add_entry(L"Tornado GR4", {"ac=tnd4"});
        }

        /*
        //no anims
        add_entry(L"YF23", {"ac=yf23"});
        add_entry(L"FA44", {"ac=fa44"});

        //helicopters are not yet supported
        add_entry(L"AH64", {"ac=ah64"});
        add_entry(L"MI24", {"ac=mi24"});
        add_entry(L"KA50", {"ac=ka50"});
        add_entry(L"MH60", {"ac=mh60"});

        //not yet supported
        add_entry(L"A130", {"ac=a130"}); //b.unknown2 = 1312
        */

        for (auto &e: m_entries)
            e.events.push_back("screen=color_select");
    }
    else if (screen == "color_select")
    {
        m_title = L"AIRCRAFT COLOR";
        const int colors_count = renderer::aircraft::get_colors_count(m_vars["ac"].c_str());
        for (int i = 0; i < colors_count; ++i)
        {
            wchar_t name[255];
            char action[255];
            swprintf(name, 255, L"Color%02d", i);
            sprintf(action, "color=%d", i);
            add_entry(name, {action, "start"});
        }
    }
    else if (screen == "ac_view")
    {
        m_title = L"AIRCRAFT";
        send_event("viewer_start");
        m_back_events = {"viewer_end"};

        add_entry(L"Background: ", {""}, "bkg", {"viewer_update_bg"});
        //add_sub_entry(L"Hangar01", "hg01");
        add_sub_entry(L"Hangar02", "hg02");
        add_sub_entry(L"Hangar03", "hg03");
        add_sub_entry(L"Hangar04", "hg04");
        add_sub_entry(L"Hangar05", "hg05");

        send_sub_events(m_entries.back());

        add_entry(L"Aircraft: ", {""}, "ac", {"viewer_update_ac"});
        add_sub_entry(L"F-14D", "f14d");
        add_sub_entry(L"Su-33", "su33");
        add_sub_entry(L"B-01B", "b01b");
        add_sub_entry(L"AH-64", "ah64");
        add_sub_entry(L"Mi-24", "mi24");
        //add_sub_entry(L"MH-60", "mh60");
        //add_sub_entry(L"AC-130", "a130");
        //ToDo

        add_entry(L"Color: ", {""}, "color", {"viewer_update_color"});

        send_sub_events(*(m_entries.end() - 2));
        send_sub_events(m_entries.back());

        if (get_var("mode") != "none")
            add_entry(L"Next", {"start"});
    }
    else
        printf("unknown screen: %s\n", screen.c_str());

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
        if (var == "screen")
            set_screen(m_vars[var]);

        return;
    }

    if (event == "viewer_update_ac")
    {
        for (auto &e: m_entries)
        {
            if (e.sub_event != "color")
                continue;

            e.sub_select.clear();
            e.sub_selected = 0;

            const int colors_count = renderer::aircraft::get_colors_count(m_vars["ac"].c_str());
            for (int i = 0; i < colors_count; ++i)
            {
                wchar_t name[255];
                char action[255];
                swprintf(name, 255, L"Color%02d", i);
                sprintf(action, "%d", i);
                e.sub_select.push_back({name, action});
            }
            break;
        }
    }

    if (m_on_action)
        m_on_action(event);
}

//------------------------------------------------------------
}
