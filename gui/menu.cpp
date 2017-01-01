//
// open horizon -- undefined_darkness@outlook.com
//

#include "menu.h"
#include "util/config.h"
#include "renderer/aircraft.h"
#include "game/game.h"
#include "game/locations_list.h"
#include "game/missions_list.h"
#include "util/resources.h"

namespace gui
{
//------------------------------------------------------------

namespace { menu *current_menu = 0; }

//------------------------------------------------------------

void menu::init_var(const std::string &name, const std::string &value)
{
    config::register_var(name, value);
    m_vars[name] = config::get_var(name);
}

//------------------------------------------------------------

void menu::init()
{
    m_fonts.load("UI/text/menuCommon.acf");
    m_bkg.load("UI/comp_simple_bg.lar");
    m_select.load("UI/comp_menu.lar");
    m_sounds.load("sound/common.acb");

    m_font_color = nya_math::vec4(89 / 255.0, 203 / 255.0, 136 / 255.0, 1.0);

    init_var("master_volume", config::get_var("master_volumes"));
    init_var("music_volume", config::get_var("music_volume"));

    load_script();
}

//------------------------------------------------------------

bool menu::load_script()
{
    current_menu = this;

    nya_memory::tmp_buffer_scoped menu_res(load_resource("menu.lua"));
    if (!menu_res.get_data())
    {
        printf("unable to load menu.lua\n");
        return false;
    }

    if (!m_script.load(std::string((char *)menu_res.get_data(),menu_res.get_size())))
    {
        printf("unable to load menu.lua: %s\n", m_script.get_error().c_str());
        return false;
    }

    m_script.add_callback("init_var", init_var);
    m_script.add_callback("get_var", get_var);
    m_script.add_callback("set_title", set_title);
    m_script.add_callback("set_bkg", set_bkg);
    m_script.add_callback("set_bkg_pic", set_bkg_pic);
    m_script.add_callback("set_sel_pic", set_sel_pic);
    m_script.add_callback("set_font_color", set_font_color);
    m_script.add_callback("add_entry", add_entry);
    m_script.add_callback("add_input", add_input);
    m_script.add_callback("send_event", send_event);
    m_script.add_callback("set_history", set_history);
    m_script.add_callback("get_aircrafts", get_aircrafts);
    m_script.add_callback("get_aircraft_colors", get_aircraft_colors);
    m_script.add_callback("get_locations", get_locations);
    m_script.add_callback("get_missions", get_missions);
    m_script.add_callback("get_campaigns", get_campaigns);

    m_script.call("init");

    set_screen("main");
    return true;
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

    if (m_current_bkg >= 0)
        m_bkg.draw_tx(r, 0, m_current_bkg, sr, white);
    else if (m_bkg_pic.get_width() > 0)
    {
        static std::vector<rect_pair> fsq(1);
        fsq[0].r.w = r.get_width();
        fsq[0].r.y = r.get_height();
        fsq[0].r.h = -r.get_height();
        fsq[0].tc.w = m_bkg_pic.get_width();
        fsq[0].tc.h = m_bkg_pic.get_height();
        r.draw(fsq, m_bkg_pic);
    }

    int x = 155, y = 155;
    m_fonts.draw_text(r, m_title.c_str(), "ZurichBD_M", x, 80, m_font_color);
    const int count = 15;
    for (int i = m_selected / (count - 1) * (count - 1), j = 0; i < (int)m_entries.size() && j < count; ++i, ++j)
    {
        auto &e = m_entries[i];
        if (m_selected == i)
        {
            if (m_select_pic.get_width() > 0)
            {
                static std::vector<rect_pair> q(1);
                q[0].r.x = x - 20;
                q[0].r.w = m_select_pic.get_width();
                q[0].r.y = y + (34 + m_select_pic.get_height())/2;
                q[0].r.h = -m_select_pic.get_height();
                q[0].tc.w = m_select_pic.get_width();
                q[0].tc.h = m_select_pic.get_height();
                r.draw(q, m_select_pic);
            }
            else
            {
                rect rct;
                rct.x = x - 20, rct.y = y;
                rct.w = 550, rct.h = 34;
                m_select.draw_tx(r, 0, 3, rct, white);
            }
        }

        auto off = m_fonts.draw_text(r, e.name.c_str(), "ZurichBD_M", x, y, m_font_color);
        if (!e.sub_select.empty())
            m_fonts.draw_text(r, e.sub_select[e.sub_selected].first.c_str(), "ZurichBD_M", x + off, y, m_font_color);
        if (!e.input.empty())
            m_fonts.draw_text(r, e.input.c_str(), "ZurichBD_M", x + off, y, m_font_color);

        y += 34;
    }

    m_fonts.draw_text(r, m_error.c_str(), "ZurichBD_M", x, r.get_height() - 30, nya_math::vec4(1.0, 0.1, 0.1, 1.0));

    x = 700, y = 155;
    for (auto &d: m_desc)
    {
        m_fonts.draw_text(r, d.c_str(), "ZurichBD_M", x, y, m_font_color);
        y += 34;
    }

    //m_fonts.draw_text(r, L"ASDFGHJKLasdfghjklQWERTYUIOPqwertyuiopZXCVBNMzxcvbnm\"\'*_-=.,0123456789", "NowGE24", 50, 200, white);
    //m_fonts.draw_text(r, L"This is a test. The quick brown fox jumps over the lazy dog's back 1234567890", "NowGE24", 50, 100, white);
    //m_fonts.draw_text(r, L"テストです。いろはにほへと ちりぬるを わかよたれそ つねならむ うゐのおくやま けふこえて あさきゆめみし えひもせす。", "ShinGo18outline", 50, 150, white);
}

//------------------------------------------------------------

void menu::update(int dt, const menu_controls &controls)
{
    if (m_need_load_campaign)
    {
        m_need_load_campaign = false;
        if (set_zip_mod(get_var("campaign").c_str()))
        {
            m_screens.clear();
            if (!load_script())
            {
                set_zip_mod(0);
                load_script();
            }
        }
    }

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

                auto it = m_choices.find(last);
                if (it != m_choices.end())
                {
                    for (int i = 0; i < (int)m_entries.size(); ++i)
                    {
                        if (m_entries[i].name == it->second)
                        {
                            m_selected = (uvalue)i;
                            break;
                        }
                    }
                }
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

void menu::add_input(const std::string &event, bool numeric_only, bool allow_input)
{
    if (m_entries.empty())
        return;

    auto &e = m_entries.back();

    e.allow_input = allow_input;
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
    current_menu = this;
    m_selected = 0;
    m_error.clear();
    m_entries.clear();
    m_back_events.clear();

    if (screen == "mp_create")
    {
        m_title = L"START SERVER";
        m_current_bkg = 2;

        add_entry(L"Name: ", {});
        add_input("name");

        add_entry(L"Game mode: ", {}, "mode");
        add_sub_entry(L"Deathmatch", "dm");
        add_sub_entry(L"Team deathmatch", "tdm");
        add_sub_entry(L"Free flight", "ff");

        send_sub_events(m_entries.back());

        add_entry(L"Max players: ", {});
        send_event("max_players=8");
        add_input("max_players", true, false);
/*
        add_entry(L"Public: ", {}, "mp_public", {});
        add_sub_entry(L"Yes", "true");
        add_sub_entry(L"No", "false");
*/
        send_sub_events(m_entries.back());

        add_entry(L"Location: ", {}, "map", {});

        auto &locations = game::get_locations_list();
        for (auto &l: locations)
            add_sub_entry(to_wstring(l.second), l.first);

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
    else if (screen == "ac_view")
    {
        m_title = L"AIRCRAFT";
        m_current_bkg = -1;

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
        m_current_bkg = 1;

        add_entry(L"Fullscreen: ", {}, "fullscreen");
        add_sub_entry(L"No", "false");
        add_sub_entry(L"Yes", "true");
        if (config::get_var_bool("fullscreen"))
            std::swap(m_entries.back().sub_select[0], m_entries.back().sub_select[1]);

        add_entry(L"Master volume: ", {});
        add_input("master_volume", true, false);

        add_entry(L"Music volume: ", {});
        add_input("music_volume", true, false);

        add_entry(L"Configure joystick", {"screen=joystick"});
    }
    else if (screen == "joystick")
    {
        m_title = L"CONFIGURE JOYSTICK";
        m_current_bkg = 1;

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
        m_script.call("on_set_screen", {screen});

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
        {
            if (!m_screens.empty() && m_selected < m_entries.size())
                m_choices[m_screens.back()] = m_entries[m_selected].name;

            set_screen(m_vars[var]);
        }

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

        if (var == "fullscreen")
            send_event("fullscreen_toggle");

        for (auto &e: m_entries)
        {
            if (e.input_event == var)
                e.input = to_wstring(value);
        }

        m_script.call("on_set_var", {var, value});
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

    if (event == "reset_mission_desc")
    {
        m_desc.clear();
        return;
    }

    if (event == "update_mission_desc")
    {
        m_desc.clear();
        for (auto &m: game::get_missions_list())
        {
            if (m.id == get_var("mission"))
            {
                std::wstringstream wss(to_wstring(m.description));
                std::wstring l;
                while (std::getline(wss, l, L'\n'))
                    m_desc.push_back(l);
                break;
            }
        }
        return;
    }

    if (event == "load_campaign")
    {
        m_need_load_campaign = true;
        return;
    }

    m_script.call("on_event", {event});

    if (m_on_action)
        m_on_action(event);
}

//------------------------------------------------------------

void menu::set_error(const std::string &error)
{
    m_error = to_wstring(error);
}

//------------------------------------------------------------

int menu::init_var(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 2)
    {
        printf("invalid args count in function init_var\n");
        return 0;
    }

    current_menu->init_var(script::get_string(state, 0), script::get_string(state, 1));
    return 0;
}

//------------------------------------------------------------

int menu::get_var(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function get_var\n");
        return 0;
    }

    script::push_string(state, current_menu->get_var(script::get_string(state, 0)));
    return 1;
}

//------------------------------------------------------------

int menu::set_title(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function set_title\n");
        return 0;
    }

    current_menu->m_title = to_wstring(script::get_string(state, 0));
    return 0;
}

//------------------------------------------------------------

int menu::set_bkg(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function set_bkg\n");
        return 0;
    }

    current_menu->m_current_bkg = script::get_int(state, 0);
    current_menu->m_bkg_pic.unload();
    return 0;
}

//------------------------------------------------------------

int menu::set_bkg_pic(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function set_bkg_pic\n");
        return 0;
    }

    current_menu->m_current_bkg = -1;
    current_menu->m_bkg_pic.load(script::get_string(state, 0).c_str());
    return 0;
}

//------------------------------------------------------------

int menu::set_sel_pic(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function set_sel_pic\n");
        return 0;
    }

    current_menu->m_select_pic.load(script::get_string(state, 0).c_str());
    return 0;
}

//------------------------------------------------------------

int menu::set_font_color(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 3)
    {
        printf("invalid args count in function set_font_color\n");
        return 0;
    }

    float alpha = args_count > 3 ? script::get_int(state, 3)/255.0f : 1.0f;
    current_menu->m_font_color.set(script::get_int(state, 0)/255.0f,
                                   script::get_int(state, 1)/255.0f,
                                   script::get_int(state, 2)/255.0f, alpha);
    return 0;
}

//------------------------------------------------------------

int menu::add_entry(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function add_entry\n");
        return 0;
    }

    const auto name = to_wstring(script::get_string(state, 0));

    std::vector<std::string> events;
    for (int i = 1; i < args_count; ++i)
        events.push_back(script::get_string(state, i));

    current_menu->add_entry(name, events);
    return 0;
}

//------------------------------------------------------------

int menu::add_input(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function add_input\n");
        return 0;
    }

    const bool numeric = args_count > 1 ? script::get_bool(state, 1) : false;
    const bool allow_input = args_count > 2 ? script::get_bool(state, 2) : false;
    current_menu->add_input(script::get_string(state, 0), numeric, allow_input);
    return 0;
}

//------------------------------------------------------------

int menu::send_event(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function send_event\n");
        return 0;
    }

    current_menu->send_event(script::get_string(state, 0));
    return 0;
}

//------------------------------------------------------------

int menu::set_history(lua_State *state)
{
    current_menu->m_screens.clear();
    auto args_count = script::get_args_count(state);
    for (int i = 0; i < args_count; ++i)
        current_menu->m_screens.push_back(script::get_string(state, i));
    return 0;
}

//------------------------------------------------------------

int menu::get_aircrafts(lua_State *state)
{
    int args_count = script::get_args_count(state);
    std::vector<std::string> roles_list(args_count);
    for (int i = 0; i < args_count; ++i)
        roles_list[i] = script::get_string(state, i);

    auto ac_list = game::get_aircraft_ids(roles_list);

    std::vector<std::pair<std::string, std::string> > list(ac_list.size());
    for (int i = 0; i < int(list.size()); ++i)
    {
        list[i].first = ac_list[i];
        list[i].second = from_wstring(game::get_aircraft_name(ac_list[i]));
    }

    script::push_array(state, list, "id", "name");
    return 1;
}

//------------------------------------------------------------

int menu::get_aircraft_colors(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function get_aircraft_colors\n");
        return 0;
    }

    auto ac = script::get_string(state, 0);
    const int colors_count = renderer::aircraft::get_colors_count(ac.c_str());

    std::vector<std::string> list(colors_count);
    for (int i = 0; i < colors_count; ++i)
    {
        char buf[255];
        sprintf(buf, "Color%02d ", i);
        list[i] = buf + renderer::aircraft::get_color_name(ac.c_str(), i);
    }

    script::push_array(state, list);
    return 1;
}

//------------------------------------------------------------

int menu::get_locations(lua_State *state)
{
    script::push_array(state, game::get_locations_list(), "id", "name");
    return 1;
}

//------------------------------------------------------------

int menu::get_missions(lua_State *state)
{
    const auto &ms_list = game::get_missions_list();
    std::vector<std::pair<std::string, std::string> > list(ms_list.size());

    for (int i = 0; i < int(list.size()); ++i)
    {
        list[i].first = ms_list[i].id;
        list[i].second = ms_list[i].name;
    }

    script::push_array(state, list, "id", "name");
    return 1;
}

//------------------------------------------------------------

int menu::get_campaigns(lua_State *state)
{
    static std::vector<std::pair<std::string, std::string> > list;
    if (list.empty())
    {
        auto campaigns = list_files("campaigns/");
        for (auto &c: campaigns)
        {
            nya_resources::zip_resources_provider zprov;
            if (!zprov.open_archive(c.c_str()))
                continue;

            pugi::xml_document doc;
            if (!load_xml(zprov.access("info.xml"), doc))
                continue;

            auto root = doc.first_child();
            std::string name = root.attribute("name").as_string();
            if (name.empty())
                continue;

            list.push_back({c, name});
        }
    }

    script::push_array(state, list, "id", "name");
    return 1;
}

//------------------------------------------------------------
}
