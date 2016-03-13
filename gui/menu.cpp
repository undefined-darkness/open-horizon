//
// open horizon -- undefined_darkness@outlook.com
//

#include "menu.h"
#include "util/config.h"
#include "renderer/aircraft.h"
#include "game.h"

namespace gui
{
//------------------------------------------------------------

void menu::init_var(const std::string &name, const std::string &value)
{
    config::register_var(name, value);
    m_vars[name] = config::get_var(name);
}

//------------------------------------------------------------

void menu::init()
{
    *this = menu(); //release

    set_screen("main");

    m_fonts.load("UI/text/menuCommon.acf");
    m_bkg.load("UI/comp_simple_bg.lar");
    m_select.load("UI/comp_menu.lar");

    init_var("name", "PLAYER");
    init_var("address", "127.0.0.1");
    init_var("port", "8001");
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
    else if (m_screens.back() == "map_select" || m_screens.back() == "mp"
             || m_screens.back() == "mp_create" || m_screens.back() == "mp_connect")
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
        if (!e.input.empty())
            m_fonts.draw_text(r, e.input.c_str(), "ZurichBD_M", x + off, y, font_color);

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
        else if (!m_entries.empty())
            m_selected = (int)m_entries.size() - 1;
    }

    if (controls.down && controls.down != m_prev_controls.down)
    {
        if (!m_entries.empty())
        {
            if (m_selected + 1 < m_entries.size())
                ++m_selected;
            else
                m_selected = 0;
        }
    }

    if (controls.left && controls.left != m_prev_controls.left)
    {
        if (m_selected<m_entries.size())
        {
            auto &e = m_entries[m_selected];
            if (e.sub_selected > 0)
                --e.sub_selected;
            else if (!e.sub_select.empty())
                e.sub_selected = (int)e.sub_select.size() - 1;
            send_sub_events(e);

            if (e.input_numeric)
            {
                std::string s(e.input.begin(), e.input.end());
                int n = atoi(s.c_str());
                if (n > 0)
                {
                    s = std::to_string(--n);
                    if (!n && e.input_event == "max_players")
                        s = "No limit";

                    e.input = std::wstring(s.begin(), s.end());
                    send_event(e.input_event + "=" + std::string(e.input.begin(), e.input.end()));
                }
            }
        }
    }

    if (controls.right && controls.right != m_prev_controls.right)
    {
        if (m_selected<m_entries.size())
        {
            auto &e = m_entries[m_selected];
            if (e.sub_selected + 1 < e.sub_select.size())
                ++e.sub_selected;
            else
                e.sub_selected = 0;
            send_sub_events(e);

            if (e.input_numeric)
            {
                std::string s(e.input.begin(), e.input.end());
                int n = atoi(s.c_str());
                if (n < 65535)
                {
                    s = std::to_string(++n);
                    e.input = std::wstring(s.begin(), s.end());
                    send_event(e.input_event + "=" + std::string(e.input.begin(), e.input.end()));
                }
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

void menu::on_input(char c)
{
    if (m_selected >= m_entries.size())
        return;

    auto &e = m_entries[m_selected];
    if (!e.allow_input)
        return;

    if (c == 8 && !e.input.empty())
        e.input.pop_back();

    if (e.input_numeric && (c < '0' || c > '9'))
        return;

    if (c < 32 || c >= 127)
        return;

    e.input.push_back(c);
    send_event(e.input_event + "=" + std::string(e.input.begin(), e.input.end()));
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

int menu::get_var_int(const std::string &name) const
{
    return atoi(get_var(name).c_str());
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

void menu::add_input(const std::string &event, bool numeric_only)
{
    if (m_entries.empty())
        return;

    auto &e = m_entries.back();

    e.allow_input = true;
    e.input_numeric = numeric_only;
    e.input_event = event;

    if (event.empty())
        return;

    auto v = get_var(event);
    e.input = std::wstring(v.begin(), v.end());
}

//------------------------------------------------------------

void menu::send_sub_events(const entry &e)
{
    if (!e.sub_event.empty() && e.sub_selected < e.sub_select.size())
        send_event(e.sub_event + '=' + e.sub_select[e.sub_selected].second);

    for (auto &se: e.sub_events)
        send_event(se);
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
        add_entry(L"Deathmatch", {"mode=dm", "screen=map_select", "multiplayer=no"});
        add_entry(L"Team deathmatch", {"mode=tdm", "screen=map_select", "multiplayer=no"});
        add_entry(L"Free flight", {"mode=ff", "screen=map_select", "multiplayer=no"});
/*
        add_entry(L"Multiplayer", {"screen=mp"});
        add_entry(L"Aircraft viewer", {"mode=none", "screen=ac_view"});
*/
        add_entry(L"Exit", {"exit"});
    }
    else if (screen == "mp")
    {
        m_title = L"MULTIPLAYER";
        add_entry(L"Internet servers", {"screen=mp_inet", "multiplayer=client"});
        //add_entry(L"Local network servers", {"screen=mp_local", "multiplayer=client"});
        add_entry(L"Connect to address", {"screen=mp_connect", "multiplayer=client"});
        add_entry(L"Start server", {"screen=mp_create", "multiplayer=server"});
    }
    else if (screen == "mp_connect")
    {
        m_title = L"CONNECT TO SERVER";

        add_entry(L"Name: ", {});
        add_input("name");

        add_entry(L"Address: ", {});
        add_input("address");
        add_entry(L"Port: ", {});
        add_input("port", true);
        add_entry(L"Connect", {"connect"});
    }
    else if (screen == "mp_create")
    {
        m_title = L"START SERVER";

        add_entry(L"Name: ", {});
        add_input("name", "PLAYER");

        add_entry(L"Game mode: ", {}, "mode");
        add_sub_entry(L"Free flight", "ff");

        send_sub_events(m_entries.back());

        add_entry(L"Max players: ", {});
        send_event("max_players=8");
        add_input("max_players", true);
        m_entries.back().allow_input = false;

        add_entry(L"Public: ", {}, "mp_public", {});
        add_sub_entry(L"Yes", "true");
        add_sub_entry(L"No", "false");

        send_sub_events(m_entries.back());

        add_entry(L"Location: ", {}, "map", {});
#if _DEBUG || DEBUG
        add_sub_entry(L"Test", "def");
#endif
        add_sub_entry(L"Miami", "ms01");
        add_sub_entry(L"Dubai", "ms06");
        add_sub_entry(L"Honolulu", "ms51");
        add_sub_entry(L"Beliy Base", "ms08x");

        send_sub_events(m_entries.back());

        add_entry(L"Aircraft: ", {""}, "ac", {"update_ac"});
        add_sub_entry(L"F-14D", "f14d");
        add_sub_entry(L"Su-33", "su33");
        add_sub_entry(L"B-01B", "b01b");

        add_entry(L"Color: ", {""}, "color", {});

        send_sub_events(*(m_entries.end() - 2));
        send_sub_events(m_entries.back());

        add_entry(L"Port: ", {});
        add_input("port", true);

        add_entry(L"Start", {"start"});
    }
    else if (screen == "map_select")
    {
        m_title = L"LOCATION";
        add_entry(L"Miami", {"map=ms01"});
        add_entry(L"Dubai", {"map=ms06"});
        add_entry(L"Paris", {"map=ms30"});
        add_entry(L"Tokyo", {"map=ms50"});
        add_entry(L"Honolulu", {"map=ms51"});
        add_entry(L"Beliy Base", {"map=ms08x"}); //siege
        add_entry(L"Black Sea", {"map=ms09"});
/*
        add_entry(L"Moscow", {"map=ms11b"});
        add_entry(L"02", {"map=ms02"}); //inferno //oil day
        //add_entry(L"03 Eastern Africa", {"map=ms03"}); //red moon //tex indices idx < size assert
        //add_entry(L"04 Mogadiyu", {"map=ms04"}); //spooky //tex indices idx < size assert
        add_entry(L"05", {"map=ms05"}); //oil night - blue on blue
        //add_entry(L"07 Suez Canal", {"map=ms07"}); //lock n load //tex indices idx < size assert
        add_entry(L"08 Derbent", {"map=ms08"}); //pipeline
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
        std::vector<std::string> roles;
        if (get_var("mode") == "ff")
            roles = {"fighter", "multirole", "attacker", "bomber"};
        else
            roles = {"fighter", "multirole"};

        m_title = L"AIRCRAFT";
        const auto planes = game::get_aircraft_ids(roles);
        for (auto &p: planes)
            add_entry(game::get_aircraft_name(p), {"ac="+p});

        /*
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
        add_sub_entry(L"Hangar01", "hg01");
        add_sub_entry(L"Hangar02", "hg02");
        add_sub_entry(L"Hangar03", "hg03");
        add_sub_entry(L"Hangar04", "hg04");
        add_sub_entry(L"Hangar05", "hg05");

        send_sub_events(m_entries.back());

        add_entry(L"Aircraft: ", {""}, "ac", {"update_ac","viewer_update_ac"});
        const auto planes = game::get_aircraft_ids({"fighter", "multirole", "attacker", "bomber"});
        for (auto &p: planes)
            add_sub_entry(game::get_aircraft_name(p), p);

        //add_sub_entry(L"MH-60", "mh60");
        //add_sub_entry(L"AC-130", "a130");

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
        auto value = event.substr(eq + 1);
        m_vars[var] = value;
        config::set_var(var, value);
        if (var == "screen")
            set_screen(m_vars[var]);

        return;
    }

    if (event == "update_ac")
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
        return;
    }

    if (m_on_action)
        m_on_action(event);
}

//------------------------------------------------------------
}
