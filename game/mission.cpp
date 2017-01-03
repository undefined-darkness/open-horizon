//
// open horizon -- undefined_darkness@outlook.com
//

#include "mission.h"
#include "world.h"
#include "extensions/zip_resources_provider.h"
#include "util/xml.h"

namespace game
{
//------------------------------------------------------------

namespace { mission *current_mission = 0; }

void mission::start(const char *plane, int color, const char *mission)
{
    m_paths.clear();
    m_player_on_destroy.clear();

    nya_resources::zip_resources_provider zprov;
    if (!zprov.open_archive(mission))
        return;

    pugi::xml_document doc;
    if (!load_xml(zprov.access("objects.xml"), doc))
        return;

    auto root = doc.first_child();

    m_world.set_location(root.attribute("location").as_string());

    auto p = root.child("player");
    if (p)
    {
        m_player = m_world.add_plane(plane, m_world.get_player_name(), color, true);
        m_player->set_pos(read_vec3(p));
        m_player->set_rot(quat(0.0, angle_deg(p.attribute("yaw").as_float()), 0.0));
        m_player->reset_state();
        m_player_on_destroy = p.attribute("on_destroy").as_string();
    }

    for (auto p = root.child("path"); p; p = p.next_sibling("path"))
    {
        std::string name = p.attribute("name").as_string();
        if (name.empty())
            continue;

        auto &pth = m_paths[name];
        pth.second = p.attribute("loop").as_bool();
        for (auto p0 = p.child("point"); p0; p0 = p0.next_sibling("point"))
            pth.first.push_back(read_vec3(p0));
    }

    for (auto o = root.child("object"); o; o = o.next_sibling("object"))
    {
        auto u = m_world.add_unit(o.attribute("id").as_string());
        if (!u)
            continue;

        mission_unit mu;
        mu.name = o.attribute("name").as_string();
        mu.u = u;

        u->set_active(o.attribute("active").as_bool());
        u->set_pos(read_vec3(o));
        u->set_yaw(o.attribute("yaw").as_float());

        auto a = o.child("attribute");
        std::string align = a.attribute("align").as_string();

        mu.on_init = a.attribute("on_init").as_string();
        mu.on_destroy = a.attribute("on_destroy").as_string();

        if (align == "target")
            u->set_align(unit::align_target);
        else if (align == "enemy")
            u->set_align(unit::align_enemy);
        else if (align == "ally")
            u->set_align(unit::align_ally);
        else
            u->set_align(unit::align_neutral);

        auto p = m_paths.find(a.attribute("path").as_string());
        if (p != m_paths.end())
            u->set_path(p->second.first, p->second.second);

        auto target = get_object(a.attribute("target").as_string());
        if (target)
            u->set_target(target);

        auto follow = get_object(a.attribute("follow").as_string());
        if (follow)
            u->set_follow(follow);

        m_units.push_back(mu);
    }

    for (auto o = root.child("zone"); o; o = o.next_sibling("zone"))
    {
        zone z;

        z.name = o.attribute("name").as_string();
        z.active = o.attribute("active").as_bool();
        z.pos = read_vec3(o);
        z.radius = o.attribute("radius").as_float();
        z.radius_sq = z.radius * z.radius;

        auto a = o.child("attribute");
        z.on_enter = a.attribute("on_enter").as_string();
        z.on_leave = a.attribute("on_leave").as_string();

        std::string display = a.attribute("display").as_string();
        if (display == "point")
            z.display = zone::display_point;
        else if (display == "circle")
            z.display = zone::display_circle;
        else if (display == "cylinder")
            z.display = zone::display_cylinder;

        m_zones.push_back(z);
    }

    m_world.set_ally_handler(std::bind(&mission::is_ally, std::placeholders::_1, std::placeholders::_2));
    m_world.set_on_kill_handler(std::bind(&mission::on_kill, this, std::placeholders::_1, std::placeholders::_2));

    auto script_res = zprov.access("script.lua");
    if (script_res)
    {
        std::string script;
        script.resize(script_res->get_size());
        if (!script.empty())
            script_res->read_all(&script[0]);
        m_script.load(script);
        script_res->release();
    }

    m_script.add_callback("start_timer", start_timer);
    m_script.add_callback("setup_timer", setup_timer);
    m_script.add_callback("stop_timer", stop_timer);

    m_script.add_callback("set_active", set_active);
    m_script.add_callback("set_path", set_path);
    m_script.add_callback("set_follow", set_follow);
    m_script.add_callback("set_target", set_target);
    m_script.add_callback("set_target_search", set_target_search);
    m_script.add_callback("set_align", set_align);
    m_script.add_callback("set_speed", set_speed);
    m_script.add_callback("set_speed_limit", set_speed_limit);

    m_script.add_callback("get_height", get_height);

    m_script.add_callback("destroy", destroy);

    m_script.add_callback("setup_radio", setup_radio);
    m_script.add_callback("clear_radio", clear_radio);
    m_script.add_callback("add_radio", add_radio);

    m_script.add_callback("set_hud_visible", set_hud_visible);

    m_script.add_callback("mission_clear", mission_clear);
    m_script.add_callback("mission_update", mission_update);
    m_script.add_callback("mission_fail", mission_fail);

    m_finished = false;
    m_hide_hud = false;
    current_mission = this;
    m_script.call("init");
    update_zones();

    for (auto &u: m_units) if (u.u->is_active() && !u.on_init.empty()) m_script.call(u.on_init, {u.name}), u.on_init.clear();
}

//------------------------------------------------------------

inline float dist_xz_sq(const vec3 &a, const vec3 &b)
{
    const float x = a.x - b.x, z = a.z - b.z;
    return x * x + z * z;
}

//------------------------------------------------------------

bool mission::zone::is_inside(const object_ptr &p)
{
    for (auto &i: inside) if (p == i.lock()) return true;
    return false;
}

//------------------------------------------------------------

void mission::update(int dt, const plane_controls &player_controls)
{
    if (!m_player)
        return;

    if (dt > 100)
        dt = 100;

    m_player->controls = player_controls;
    current_mission = this;

    static std::vector<std::pair<std::string, std::string> > to_call;
    to_call.clear();
    for (auto &t: m_timers)
    {
        if (!t.active)
            continue;

        if ((t.time -= dt) > 0)
            continue;

        t.time = 0;
        t.active = false;
        to_call.push_back({t.func, t.id});
    }

    for (auto &z: m_zones)
    {
        if (!z.active)
            continue;

        const float treshold = 0.5f;

        //outside

        if (dist_xz_sq(z.pos, m_player->get_pos()) < z.radius_sq - treshold)
        {
            if (!z.is_inside(m_player) && !z.on_enter.empty())
            {
                to_call.push_back({z.on_enter, "player"});
                z.inside.push_back(m_player);
            }
        }

        for (auto &u: m_units)
        {
            if (dist_xz_sq(z.pos, u.u->get_pos()) < z.radius_sq - treshold)
            {
                if (!z.is_inside(u.u) && !z.on_enter.empty())
                {
                    to_call.push_back({z.on_enter, u.name});
                    z.inside.push_back(u.u);
                }
            }
        }

        //inside

        bool erase = false;

        for (auto &i: z.inside)
        {
            if (i.expired())
            {
                erase = true;
                continue;
            }

            auto il = i.lock();
            if (dist_xz_sq(z.pos, il->get_pos()) > z.radius_sq + treshold)
            {
                if (!z.on_leave.empty())
                {
                    for (auto &u: m_units)
                    {
                        if (u.u == il)
                        {
                            to_call.push_back({z.on_leave, u.name});
                            break;
                        }
                    }
                }

                i.reset();
                erase = true;
            }
        }

        if (erase)
            z.inside.erase(std::remove_if(z.inside.begin(), z.inside.end(), [](object_wptr &a){ return a.expired(); }),z.inside.end());
    }

    for (auto &t: to_call)
        m_script.call(t.first, {t.second});

    if (!m_radio_messages.empty())
    {
        auto &t = m_radio_messages.front().time;
        t -= dt;
        if (t < 0)
        {
            m_radio_messages.erase(m_radio_messages.begin());
            if(!m_radio_messages.empty())
                set_radio_message(m_radio_messages.front());
        }
    }

    m_world.update(dt);

    for (int i = 0; i < m_world.get_planes_count(); ++i)
    {
        auto p = m_world.get_plane(i);
        if (p->hp <= 0)
            mission_fail(0);
    }

    m_world.get_hud().clear_texts();
    int last_text_idx = 0;
    for (auto &t: m_timers)
    {
        if (!t.active || t.name.empty())
            continue;

        int s = t.time / 1000;
        wchar_t buf[255];
        swprintf(buf, sizeof(buf), L": %d:%02d", s/60, s % 60);

        m_world.get_hud().add_text(last_text_idx, t.name + buf, "Zurich20", 1000, 150 + last_text_idx * 30, gui::hud::green);
        ++last_text_idx;
    }

    if (m_hide_hud)
        m_world.get_hud().set_hide(true);
}

//------------------------------------------------------------

void mission::end()
{
    m_units.clear();
    m_player.reset();
    m_paths.clear();
    m_radio.clear();
    m_radio_messages.clear();
    m_timers.clear();
    m_zones.clear();
}

//------------------------------------------------------------

void mission::on_kill(const object_ptr &k, const object_ptr &v)
{
    if (!v)
        return;

    if (!m_player_on_destroy.empty() && v == m_player)
        m_script.call(m_player_on_destroy, {"player"});

    for (auto &u: m_units)
    {
        if (u.u != v)
            continue;

        if (!u.on_destroy.empty())
            m_script.call(u.on_destroy, {u.name});
        break;
    }
}

//------------------------------------------------------------

int mission::start_timer(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 2)
    {
        printf("invalid args count in function start_timer\n");
        return 0;
    }

    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t == current_mission->m_timers.end())
        t = current_mission->m_timers.insert(t, timer()), t->id = id;

    t->time = script::get_int(state, 1) * 1000;

    if (args_count > 2)
        t->func = script::get_string(state, 2);
    else
        t->func.clear();

    t->active = true;
    return 0;
}

//------------------------------------------------------------

int mission::setup_timer(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function setup_timer\n");
        return 0;
    }

    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t == current_mission->m_timers.end())
        t = current_mission->m_timers.insert(t, timer()), t->id = id;

    if (args_count > 1)
        t->name = to_wstring(script::get_string(state, 1));
    else
        t->name.clear();

    return 0;
}

//------------------------------------------------------------

int mission::stop_timer(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function stop_timer\n");
        return 0;
    }

    std::string id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto t = std::find_if(current_mission->m_timers.begin(), current_mission->m_timers.end(), [id](const timer &t){ return t.id == id; });
    if (t != current_mission->m_timers.end())
        t->active = false;

    return 0;
}

//------------------------------------------------------------

int mission::set_active(lua_State *state)
{
    auto args_count = script::get_args_count(state);
    if (args_count < 1)
    {
        printf("invalid args count in function set_active\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    bool active = args_count < 2 ? true : script::get_bool(state, 1);
    for (auto &u: current_mission->m_units)
    {
        if (u.name != name || !u.u)
            continue;

        u.u->set_active(active);
        if (!u.on_init.empty())
            current_mission->m_script.call(u.on_init, {u.name}), u.on_init.clear();
    }

    bool need_update_zones = false;
    for (auto &z: current_mission->m_zones)
    {
        if (z.name != name || z.active == active)
            continue;

        need_update_zones = true;
        z.active = active;
    }

    if (need_update_zones)
        current_mission->update_zones();

    return 0;
}

//------------------------------------------------------------

int mission::set_path(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_path\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    auto p = current_mission->m_paths.find(script::get_string(state, 1));
    for (auto &u: current_mission->m_units)
    {
        if (u.name == name && u.u)
        {
            if (p == current_mission->m_paths.end())
                u.u->set_path({}, false);
            else
                u.u->set_path(p->second.first, p->second.second);
        }
    }

    return 0;
}

//------------------------------------------------------------

int mission::set_follow(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_follow\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    auto f = current_mission->get_object(script::get_string(state, 1));
    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_follow(f);

    return 0;
}

//------------------------------------------------------------

int mission::set_target(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_target\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    auto t = current_mission->get_object(script::get_string(state, 1));
    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_target(t);

    return 0;
}

//------------------------------------------------------------

int mission::set_target_search(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_target_search\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    unit::target_search_mode mode;
    std::string mode_str = script::get_string(state, 1);
    if (mode_str == "all")
        mode = unit::search_all;
    else if (mode_str == "player")
        mode = unit::search_player;
    else if (mode_str == "none")
        mode = unit::search_none;
    else
        return 0;

    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_target_search(mode);

    return 0;
}

//------------------------------------------------------------

int mission::set_align(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_align\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    unit::align a;
    auto align = script::get_string(state, 1);
    if (align == "target")
        a = unit::align_target;
    else if (align == "enemy")
        a = unit::align_enemy;
    else if (align == "ally")
        a = unit::align_ally;
    else if (align == "neutral")
        a = unit::align_neutral;
    else
        return 0;

    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_align(a);

    return 0;
}

//------------------------------------------------------------

int mission::set_speed(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_align\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    const float speed = script::get_int(state, 1);
    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_speed(speed);

    return 0;
}

//------------------------------------------------------------

int mission::set_speed_limit(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function set_align\n");
        return 0;
    }

    auto name = script::get_string(state, 0);
    if (name.empty())
        return 0;

    const float speed = script::get_int(state, 1);
    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->set_speed_limit(speed);

    return 0;
}

//------------------------------------------------------------

int mission::get_height(lua_State *state)
{
    if (script::get_args_count(state) < 1)
    {
        printf("invalid args count in function get_height\n");
        return 0;
    }

    auto id = script::get_string(state, 0);
    if (id == "player")
    {
        auto p = current_mission->m_world.get_player();
        script::push_float(state, p ? p->get_pos().y : 0.0f);
        return 1;
    }

    auto o = current_mission->get_object(id);
    script::push_float(state, o ? o->get_pos().y : 0.0f);
    return 1;
}

//------------------------------------------------------------

int mission::destroy(lua_State *state)
{
    if (script::get_args_count(state) < 1)
    {
        printf("invalid args count in function get_height\n");
        return 0;
    }

    std::string name = script::get_string(state, 0);
    if (name == "player")
    {
        current_mission->m_world.get_player()->take_damage(9000, current_mission->m_world);
        return 0;
    }

    if (name.empty())
        return 0;

    for (auto &u: current_mission->m_units) if (u.name == name && u.u) u.u->take_damage(9000, current_mission->m_world);

    return 0;
}

//------------------------------------------------------------

int mission::setup_radio(lua_State *state)
{
    if (script::get_args_count(state) < 3)
    {
        printf("invalid args count in function setup_radio\n");
        return 0;
    }

    auto id = script::get_string(state, 0);
    if (id.empty())
        return 0;

    auto &r = current_mission->m_radio[id];
    r.first = to_wstring(script::get_string(state, 1));
    r.second = gui::hud::white;
    auto cs = script::get_string(state, 2);
    if (cs == "green")
        r.second = gui::hud::green;
    else if (cs == "blue")
        r.second = gui::hud::blue;
    else if (cs == "red")
        r.second = gui::hud::red;

    return 0;
}

//------------------------------------------------------------

int mission::clear_radio(lua_State *state)
{
    current_mission->m_radio_messages.clear();
    current_mission->set_radio_message({});
    return 0;
}

//------------------------------------------------------------

int mission::add_radio(lua_State *state)
{
    if (script::get_args_count(state) < 2)
    {
        printf("invalid args count in function add_radio\n");
        return 0;
    }

    auto id = script::get_string(state, 0);
    auto r = current_mission->m_radio.find(id);
    if (r == current_mission->m_radio.end())
        return 0;

    const int time = script::get_args_count(state) > 2 ? script::get_int(state, 2) * 1000 : 2000;
    current_mission->m_radio_messages.push_back({id, to_wstring(script::get_string(state, 1)), time});

    if (current_mission->m_radio_messages.size() == 1)
        current_mission->set_radio_message(current_mission->m_radio_messages.back());
    return 0;
}

//------------------------------------------------------------

int mission::set_hud_visible(lua_State *state)
{
    if (script::get_args_count(state) < 1)
    {
        printf("invalid args count in function set_hud_visible\n");
        return 0;
    }

    if (current_mission->m_finished)
        return 0;

    current_mission->m_hide_hud = !script::get_bool(state, 0);
    return 0;
}

//------------------------------------------------------------

int mission::mission_clear(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_world.popup_mission_clear();
    current_mission->m_finished = true;
    return 0;
}

//------------------------------------------------------------

int mission::mission_update(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_world.popup_mission_update();
    return 0;
}

//------------------------------------------------------------

int mission::mission_fail(lua_State *state)
{
    if (current_mission->m_finished)
        return 0;

    current_mission->m_world.popup_mission_fail();
    current_mission->m_finished = true;
    return 0;
}

//------------------------------------------------------------

void mission::set_radio_message(const radio_message &m)
{
    auto r = m_radio.find(m.id);
    if (r == m_radio.end())
        return;

    m_world.get_hud().set_radio(r->second.first, m.message, m.time, r->second.second);
}

//------------------------------------------------------------

object_ptr mission::get_object(std::string id) const
{
    if (id.empty())
        return object_ptr();

    if (id == "player")
        return m_world.get_player();

    for (auto &u: m_units) if (u.name == id) return u.u;

    return object_ptr();
}

//------------------------------------------------------------

void mission::update_zones()
{
    m_world.get_hud().clear_zones();
    for (auto &z: m_zones)
    {
        if (!z.active)
            continue;

        if (z.display == zone::display_point)
        {
            m_world.get_hud().add_zone(z.pos);
        }
        else if(z.display == zone::display_circle || z.display == zone::display_cylinder)
        {
            auto hf = std::bind(&world::get_height, m_world, std::placeholders::_1, std::placeholders::_2);
            m_world.get_hud().add_zone(z.pos, z.radius, hf, z.display == zone::display_cylinder);
        }
    }
}

//------------------------------------------------------------
}
