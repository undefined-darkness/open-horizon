//
// open horizon -- undefined_darkness@outlook.com
//

#include "menu.h"
#include "util/config.h"
#include "renderer/aircraft.h"
#include "game/game.h"
#include "game/locations_list.h"
#include "game/missions_list.h"

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
    set_screen("main");

    m_fonts.load("UI/text/menuCommon.acf");
    m_bkg.load("UI/comp_simple_bg.lar");
    m_select.load("UI/comp_menu.lar");

    m_sounds.load("sound/common.acb");

    init_var("name", "PLAYER");
    init_var("address", "127.0.0.1");
    init_var("port", "8001");

    init_var("master_volume", config::get_var("master_volumes"));
    init_var("music_volume", config::get_var("music_volume"));
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
    else if (m_screens.back() == "mission_select" ||  m_screens.back() == "map_select" || m_screens.back() == "mp"
             || m_screens.back() == "mp_create" || m_screens.back() == "mp_connect")
        m_bkg.draw_tx(r, 0, 2, sr, white);
    else if (m_screens.back() == "ac_select" || m_screens.back() == "color_select" || m_screens.back() == "settings" || m_screens.back() == "joystick")
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

    m_fonts.draw_text(r, m_error.c_str(), "ZurichBD_M", x, r.get_height() - 30, nya_math::vec4(1.0, 0.1, 0.1, 1.0));

    x = 700, y = 155;
    for (auto &d: m_desc)
    {
        m_fonts.draw_text(r, d.c_str(), "ZurichBD_M", x, y, font_color);
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
        play_sound("SYS_CURSOR");

        if (m_selected > 0)
            --m_selected;
        else if (!m_entries.empty())
            m_selected = (int)m_entries.size() - 1;
    }

    if (controls.down && controls.down != m_prev_controls.down)
    {
        play_sound("SYS_CURSOR");

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
            if (!e.sub_select.empty() || e.input_numeric)
            {
                play_sound("SYS_CURSOR");

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
    }

    if (controls.right && controls.right != m_prev_controls.right)
    {
        if (m_selected<m_entries.size())
        {
            auto &e = m_entries[m_selected];
            if (!e.sub_select.empty() || e.input_numeric)
            {
                play_sound("SYS_CURSOR");

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
    }

    if (controls.prev && controls.prev != m_prev_controls.prev)
    {
        if (!m_screens.empty())
        {
            play_sound("SYS_CANCEL");

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
            play_sound("SYS_DECIDE");

            auto events = m_entries[m_selected].events;
            for (auto &e: events)
                send_event(e);
        }
    }

    m_sound_world.update(dt);
    m_prev_controls = controls;
}

//------------------------------------------------------------

bool menu::joy_update(const float *axes, int axes_count, const unsigned char *btns, int btns_count)
{
    if (m_screens.empty() || m_screens.back() != "joystick")
        return false;

    std::string control;

    for (int i = 0; i < axes_count; ++i)
    {
        if (axes[i] > 0.8f)
        {
            control = "+axis" + std::to_string(i);
            break;
        }

        if (axes[i] < -0.8f)
        {
            control = "-axis" + std::to_string(i);
            break;
        }
    }

    for (int i = 0; i < btns_count; ++i)
    {
        if (btns[i])
        {
            control = "btn" + std::to_string(i);
            break;
        }
    }

    if (control.empty())
        return true;

    auto &e = m_entries[m_selected];
    auto wcontrol = std::wstring(control.begin(), control.end());
    if (e.input == wcontrol)
        return true;

    for (auto &e: m_entries)
    {
        if (e.input == wcontrol)
            e.input.clear();
    }

    e.input = wcontrol;
    auto var_name = "joy_" + e.input_event;
    config::register_var(var_name, control);
    config::set_var(var_name, control);
    send_event("update_joy_config");
    return true;
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

void menu::play_sound(std::string name)
{
    m_sound_world.play_ui(m_sounds.get(name));
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
    m_error.clear();
    m_entries.clear();
    m_back_events.clear();

    if (screen == "main")
    {
        m_title = L"MAIN MENU";
        if (!game::get_missions_list().empty())
            add_entry(L"Mission", {"mode=ms", "screen=mission_select", "multiplayer=no"});

        add_entry(L"Deathmatch", {"mode=dm", "screen=map_select", "multiplayer=no"});
        add_entry(L"Team deathmatch", {"mode=tdm", "screen=map_select", "multiplayer=no"});
        add_entry(L"Free flight", {"mode=ff", "screen=map_select", "multiplayer=no"});
        add_entry(L"Multiplayer", {"screen=mp"});
        add_entry(L"Settings", {"screen=settings"});

        //add_entry(L"Aircraft viewer", {"mode=none", "screen=ac_view"});

        add_entry(L"Exit", {"exit"});
    }
    else if (screen == "mp")
    {
        m_title = L"MULTIPLAYER";
        //add_entry(L"Internet servers", {"screen=mp_inet", "multiplayer=client"});
        //add_entry(L"Local network servers", {"screen=mp_local", "multiplayer=client"});
        add_entry(L"Connect to address", {"screen=mp_connect", "multiplayer=client"});
        add_entry(L"Start server", {"screen=mp_create", "multiplayer=server"});
    }
    else if (screen == "mp_connect")
    {
        m_screens.clear();
        m_screens.push_back("main");
        m_screens.push_back("mp");

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
        add_input("name");

        add_entry(L"Game mode: ", {}, "mode");
        add_sub_entry(L"Deathmatch", "dm");
        add_sub_entry(L"Team deathmatch", "tdm");
        add_sub_entry(L"Free flight", "ff");

        send_sub_events(m_entries.back());

        add_entry(L"Max players: ", {});
        send_event("max_players=8");
        add_input("max_players", true);
        m_entries.back().allow_input = false;
/*
        add_entry(L"Public: ", {}, "mp_public", {});
        add_sub_entry(L"Yes", "true");
        add_sub_entry(L"No", "false");
*/
        send_sub_events(m_entries.back());

        add_entry(L"Location: ", {}, "map", {});

        auto &locations = game::get_locations_list();
        for (auto &l: locations)
            add_sub_entry(l.second, l.first);

        send_sub_events(m_entries.back());

        add_entry(L"Aircraft: ", {""}, "ac", {"update_ac"});
        const auto planes = game::get_aircraft_ids({"fighter", "multirole", "attacker", "bomber"});
        for (auto &p: planes)
            add_sub_entry(game::get_aircraft_name(p), {p});

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

        for (auto &l: game::get_locations_list())
            add_entry(l.second, {"map=" + l.first});

        for (auto &e: m_entries)
            e.events.push_back("screen=ac_select");
    }
    else if (screen == "mission_select")
    {
        m_title = L"MISSION";

        m_desc.clear();

        for (auto &m: game::get_missions_list())
            add_entry(m.name, {"mission=" + m.id, "update_ms_desc"});

        for (auto &e: m_entries)
            e.events.push_back("screen=ac_select");
    }
    else if (screen == "ac_select")
    {
        std::vector<std::string> roles;
        if (get_var("mode") == "ff" || get_var("mode") == "ms")
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
    else if (screen == "settings")
    {
        m_title = L"SETTINGS";

        add_entry(L"Master volume: ", {});
        add_input("master_volume", true);
        m_entries.back().allow_input = false;

        add_entry(L"Music volume: ", {});
        add_input("music_volume", true);
        m_entries.back().allow_input = false;

        add_entry(L"Configure joystick", {"screen=joystick"});
    }
    else if (screen == "joystick")
    {
        m_title = L"CONFIGURE JOYSTICK";

        add_entry(L"Pitch up: ", {}), m_entries.back().input_event = "+pitch";
        add_entry(L"Pitch down: ", {}), m_entries.back().input_event = "-pitch";
        add_entry(L"Yaw left: ", {}), m_entries.back().input_event = "-yaw";
        add_entry(L"Yaw right: ", {}), m_entries.back().input_event = "+yaw";
        add_entry(L"Roll left: ", {}), m_entries.back().input_event = "-roll";
        add_entry(L"Roll right: ", {}), m_entries.back().input_event = "+roll";

        add_entry(L"Throttle: ", {}), m_entries.back().input_event = "throttle";
        add_entry(L"Brake: ", {}), m_entries.back().input_event = "brake";

        add_entry(L"Fire missile: ", {}), m_entries.back().input_event = "missile";
        add_entry(L"Fire gun: ", {}), m_entries.back().input_event = "mgun";

        add_entry(L"Change weapon: ", {}), m_entries.back().input_event = "change_weapon";
        add_entry(L"Change target: ", {}), m_entries.back().input_event = "change_target";
        add_entry(L"Change camera: ", {}), m_entries.back().input_event = "change_camera";
        add_entry(L"Change radar: ", {}), m_entries.back().input_event = "change_radar";

        add_entry(L"Camera pitch up: ", {}), m_entries.back().input_event = "+camera_pitch";
        add_entry(L"Camera pitch down: ", {}), m_entries.back().input_event = "-camera_pitch";
        add_entry(L"Camera yaw left: ", {}), m_entries.back().input_event = "+camera_yaw";
        add_entry(L"Camera yaw right: ", {}), m_entries.back().input_event = "-camera_yaw";

        add_entry(L"Pause: ", {}), m_entries.back().input_event = "pause";

        for (auto &e: m_entries)
        {
            auto c = config::get_var("joy_" + e.input_event);
            e.input = std::wstring(c.begin(), c.end());
        }
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

        if (var == "master_volume" || var == "music_volume")
        {
            if (get_var_int(var) > 10)
            {
                value = "10";
                config::set_var(var, value);
                m_vars[var] = value;
            }
            send_event("update_volume");
        }

        for (auto &e: m_entries)
        {
            if (e.input_event == var)
                e.input = to_wstring(value);
        }

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

    if (event == "update_ms_desc")
    {
        m_desc.clear();
        for (auto &m: game::get_missions_list())
        {
            if (m.id == get_var("mission"))
            {
                std::wstringstream wss(m.description);
                std::wstring l;
                while (std::getline(wss, l, L'\n'))
                    m_desc.push_back(l);
                break;
            }
        }
        return;
    }

    if (m_on_action)
        m_on_action(event);
}

//------------------------------------------------------------

void menu::set_error(const std::string &error)
{
    m_error = to_wstring(error);
}

//------------------------------------------------------------
}
