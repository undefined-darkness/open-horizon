//
// open horizon -- undefined_darkness@outlook.com
//

#include "world.h"
#include "network_helpers.h"
#include "weapon_information.h"
#include "util/config.h"
#include <algorithm>
#include <time.h>

namespace game
{
//------------------------------------------------------------

missile_ptr world::add_missile(const char *id, const renderer::model &mdl, bool add_to_phys_world)
{
    if (!id)
        return missile_ptr();

    missile_ptr m(new missile());
    m->phys = m_phys_world.add_missile(id, add_to_phys_world);
    m->render = m_render_world.add_missile(mdl);

    auto &param = get_arms_param();

    const std::string pref = "." + std::string(id) + ".action.";
    m->time = param.get_float(pref + "endTime") * 1000;
    m->homing_angle_cos = cosf(param.get_float(pref + "hormingAng"));
    m->dmg_radius = missile_dmg_radius;
    m->dmg = missile_damage;

    if (strcmp(id, "SAAM") == 0)
        m->mode = missile::mode_saam;
    else if (strcmp(id, "_4AGM") == 0)
        m->mode = missile::mode_4agm, m->dmg_radius *= 1.5, m->dmg *= 1.5;
    else if (strcmp(id, "LAGM") == 0)
        m->mode = missile::mode_lagm, m->dmg_radius *= 1.5, m->dmg *= 1.5;

    m_missiles.push_back(m);
    return m;
}

//------------------------------------------------------------

missile_ptr world::add_missile(const plane_ptr &p, net_missile_ptr ptr)
{
    if (!p)
        return missile_ptr();

    const bool add_to_world = !(ptr && !ptr->source);

    auto m = p->special_weapon_selected ? add_missile(p->special.id.c_str(), p->render->get_special_model(), add_to_world) :
                                          add_missile(p->missile.id.c_str(), p->render->get_missile_model(), add_to_world);
    if (!m)
        return missile_ptr();

    if (m_network)
        m->net = ptr ? ptr : m_network->add_missile(p->net, p->special_weapon_selected);

    return m;
}

//------------------------------------------------------------

missile_ptr world::add_missile(const char *id, const renderer::model &mdl, net_missile_ptr ptr)
{
    const bool add_to_world = !(ptr && !ptr->source);
    auto m = add_missile(id, mdl, add_to_world);
    if (!m)
        return missile_ptr();

    //if (m_network)
    //    m->net = ptr ? ptr : m_network->add_missile(m_player.lock()->net, false); //ToDo

    return m;
}

//------------------------------------------------------------

bomb_ptr world::add_bomb(const plane_ptr &p)
{
    if (!p)
        return bomb_ptr();

    return add_bomb(p->special.id.c_str(), p->render->get_special_model());
}

//------------------------------------------------------------

bomb_ptr world::add_bomb(const char *id, const renderer::model &m)
{
    if (!id)
        return bomb_ptr();

    bomb_ptr b(new bomb());
    b->phys = m_phys_world.add_bomb(id, true);
    b->render = m_render_world.add_object(m);

    b->dmg_radius = bomb_dmg_radius; //ToDo
    b->dmg = missile_damage * 2.0f;

    m_bombs.push_back(b);

    return b;
}

//------------------------------------------------------------

plane_ptr world::add_plane(const char *preset, const char *player_name, int color, bool player, net_plane_ptr ptr)
{
    if (!preset)
        return plane_ptr();

    plane_ptr p(new plane());
    const bool add_to_world = !(ptr && !ptr->source);
    p->phys = m_phys_world.add_plane(preset, add_to_world);

    p->render = m_render_world.add_aircraft(preset, color, player);
    p->phys->nose_offset = p->render->get_bone_pos("clv1");

    p->sounds.load(renderer::aircraft::get_sound_name(preset));

    p->max_hp = int(p->phys->params.misc.maxHp);

    p->hit_radius = preset[0] == 'b' ? 6.0f : 3.5f;

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    p->render->set_time(tm_now->tm_sec + tm_now->tm_min * 60 + tm_now->tm_hour * 60 * 60); //ToDo

    auto wi = weapon_information::get().get_aircraft_weapons(preset);
    if (wi)
    {
        p->name = wi->short_name;

        if (!wi->special.empty())
        {
            p->render->load_special(wi->special[0].model.c_str(), m_render_world.get_location_params());
            p->special = wpn_params(wi->special[0].id, wi->special[0].model);
            p->special_max_count = wi->special[0].count;
            p->special_internal = p->render->has_special_bay() && !wi->special[0].external;
        }

        p->render->load_missile(wi->missile.model.c_str(), m_render_world.get_location_params());
        p->missile = wpn_params(wi->missile.id, wi->missile.model);
        p->missile_max_count = wi->missile.count;
    }

    if (!m_network) //ToDo: get difficulty setting from server
    {
        p->missile_max_count *= m_difficulty.ammo_ammount;
        p->special_max_count *= m_difficulty.ammo_ammount;
    }

    if (player)
    {
        m_hud.load(preset, m_render_world.get_location_name());
        m_hud.set_missiles(p->missile.id.c_str(), 0);
        m_hud.set_missiles_count(p->missile_count);
        m_player = p;
    }

    if (m_network)
        p->net = ptr ? ptr : m_network->add_plane(preset, player_name ? player_name : "", color);

    if (player_name && player_name[0])
    {
        std::string name(player_name);
        std::transform(name.begin() + 1, name.end(), name.begin() + 1, ::tolower);
        p->player_name = std::wstring(name.begin(), name.end());
    }

    p->reset_state();

    m_planes.push_back(p);
    get_arms_param(); //cache
    return p;
}

//------------------------------------------------------------

unit_ptr world::add_unit(const char *id)
{
    if (!id)
        return unit_ptr();

    auto &objs = get_objects_list();
    for (auto &o: objs)
    {
        if (o.id != id)
            continue;

        m_units.resize(m_units.size() + 1);
        auto &u = m_units.back();

        if (o.params.speed_max > 0.01f)
            u = std::make_shared<unit_vehicle>(unit_vehicle(o.params, m_render_world.get_location_params()));
        else if (o.params.hp > 0)
            u = std::make_shared<unit_object>(unit_object());
        else
            u = std::make_shared<unit>(unit());

        u->load_model(o.model, o.dy, m_render_world);
        u->set_type_name(o.name);
        u->hp = o.params.hp;

        return u;
    }

    return unit_ptr();
}

//------------------------------------------------------------

void world::spawn_explosion(const nya_math::vec3 &pos, float radius, bool net_src)
{
    m_render_world.spawn_explosion(pos, radius);

    play_sound("MISL_HIT", random(2, 4), pos);

    if (m_network && net_src)
        m_network->general_msg("explosion " + to_string(pos) + " " + std::to_string(radius));
}

//------------------------------------------------------------

void world::spawn_bullet(const char *type, const vec3 &pos, const vec3 &dir, const plane_ptr &owner)
{
    vec3 r;
    const bool hit_world = m_phys_world.spawn_bullet(type, pos, dir, r);

    if (!owner->net || owner->net->source)
    {
        for (int i = 0; i < get_objects_count(); ++i)
        {
            auto t = get_object(i);
            if (t->hp <= 0 || t->is_ally(owner, *this))
                continue;

            const float spread_coeff = 2.0f;
            if (line_sphere_intersect(pos, r, t->get_pos(), t->get_hit_radius() * spread_coeff))
            {
                direct_damage(t, 60, owner);
                if (owner == get_player())
                    popup_hit(t->hp <= 0);
            }
        }
    }

    if (hit_world)
    {
        //ToDo: spark
    }
}

//------------------------------------------------------------

bool world::direct_damage(const object_ptr &target, int damage, const plane_ptr &owner)
{
    if (!target || target->hp <= 0 || target->is_ally(owner, *this))
        return false;

    if (!m_network) //ToDo: get difficulty setting from server
    {
        if (owner == get_player())
            damage *= m_difficulty.dmg_inflict;
        if (target == get_player())
            damage *= m_difficulty.dmg_take;
    }

    target->take_damage(damage, *this);
    if (target->hp <= 0)
        on_kill(owner, target);
    return true;
}

//------------------------------------------------------------

bool world::area_damage(const vec3 &pos, float radius, int damage, const plane_ptr &owner)
{
    bool hit = false;

    for (int i = 0; i < get_objects_count(); ++i)
    {
        auto o = get_object(i);
        if (!o || (o->get_pos() - pos).length() > radius)
            continue;

        if (direct_damage(o, damage, owner))
            hit = true;
    }

    return hit;
}

//------------------------------------------------------------

void world::respawn(const plane_ptr &p, const vec3 &pos, const quat &rot)
{
    if (!p)
        return;

    p->set_pos(pos);
    p->set_rot(rot);
    p->reset_state();

    if (is_host() && m_network)
        m_network->general_msg("respawn " + std::to_string(m_network->get_plane_id(p->net)) + " " + to_string(pos) + " " + to_string(rot));
}

//------------------------------------------------------------

const char *world::get_player_name() const
{
    static std::string tmp;
    tmp = config::get_var("name");
    return tmp.c_str();
}

//------------------------------------------------------------

plane_ptr world::get_plane(int idx)
{
    if (idx < 0 || idx >= (int)m_planes.size())
        return plane_ptr();

    return m_planes[idx];
}

//------------------------------------------------------------

plane_ptr world::get_plane(object_wptr o)
{
    if (o.expired())
        return plane_ptr();

    auto ol = o.lock();
    for (auto &p: m_planes)
    {
        if (p == ol)
            return p;
    }

    return plane_ptr();
}

//------------------------------------------------------------

missile_ptr world::get_missile(int idx)
{
    if (idx < 0 || idx >= (int)m_planes.size())
        return missile_ptr();

    return m_missiles[idx];
}

//------------------------------------------------------------

unit_ptr world::get_unit(int idx)
{
    if (idx < 0 || idx >= (int)m_units.size())
        return unit_ptr();

    return m_units[idx];
}

//------------------------------------------------------------

int world::get_objects_count() const
{
    return int(m_planes.size() + m_units.size());
}

//------------------------------------------------------------

object_ptr world::get_object(int idx)
{
    if (idx < 0)
        return object_ptr();

    if (idx < (int)m_planes.size())
        return m_planes[idx];

    idx -= (int)m_planes.size();

    if (idx < (int)m_units.size())
        return m_units[idx];

    return object_ptr();
}

//------------------------------------------------------------

void world::update_difficulty()
{
    auto selected = config::get_var("difficulty");
    auto list = get_difficulty_list();
    for (auto &d: list)
    {
        if (d.id == selected)
        {
            m_difficulty = d;
            return;
        }
    }

    if (!list.empty())
        m_difficulty = list.back();
}

//------------------------------------------------------------

void world::set_location(const char *name)
{
    update_difficulty();

    m_render_world.set_location(name);
    m_phys_world.set_location(name);
    m_hud.set_location(name);

    if (m_sounds.cues.empty())
        m_sounds.load("sound/game.acb");

    if (m_sounds_ui.cues.empty())
        m_sounds_ui.load("sound/game_common.acb");

    if (m_sounds_common.cues.empty())
        m_sounds_common.load("sound/common.acb");
}

//------------------------------------------------------------

void world::update(int dt)
{
    m_net_data_updated = false;

    if (m_network)
    {
        m_network->update();

        network_interface::msg_add_plane mp;
        while (m_network->get_add_plane_msg(mp))
            add_plane(mp.preset.c_str(), mp.player_name.c_str(), mp.color, false, m_network->add_plane(mp));

        network_interface::msg_add_missile mm;
        while (m_network->get_add_missile_msg(mm))
        {
            auto net = m_network->get_plane(mm.plane_id);
            if (!net)
                continue;

            for (auto &p: m_planes)
            {
                if(p->net != net)
                    continue;

                p->special_weapon_selected = mm.special;
                add_missile(p, m_network->add_missile(mm));
                break;
            }
        }

        network_interface::msg_game_data md;
        while (m_network->get_game_data_msg(md))
        {
            auto net = m_network->get_plane(md.plane_id);
            if (!net)
                continue;

            for (auto &p: m_planes)
            {
                if(p->net != net)
                    continue;

                p->net_game_data = md.data;
                break;
            }

            m_net_data_updated = true;
        }

        std::string str;
        while(m_network->get_general_msg(str))
        {
            std::istringstream is(str);
            std::string cmd;
            is >> cmd;

            if (cmd == "explosion")
            {
                vec3 pos; float r;
                read(is, pos), is >> r;
                spawn_explosion(pos, r, false);
            }

            if (is_host())
            {
                if (cmd == "plane_take_damage")
                {
                    unsigned int plane_id; int damage;
                    is >> plane_id, is >> damage;

                    auto net = m_network->get_plane(plane_id);
                    if (!net)
                        continue;

                    for (auto &p: m_planes)
                    {
                        if(p->net != net)
                            continue;

                        p->take_damage(damage, *this, false);
                        break;
                    }
                }
            }
            else
            {
                if (cmd == "respawn")
                {
                    unsigned int plane_id; vec3 pos; quat rot;
                    is >> plane_id, read(is, pos), read(is, rot);

                    auto net = m_network->get_plane(plane_id);
                    if (!net)
                        continue;

                    for (auto &p: m_planes)
                    {
                        if(p->net != net)
                            continue;

                        respawn(p, pos, rot);
                        break;
                    }
                }
                else if (cmd == "plane_set_hp")
                {
                    unsigned int plane_id; int hp;
                    is >> plane_id, is >> hp;

                    auto net = m_network->get_plane(plane_id);
                    if (!net)
                        continue;

                    for (auto &p: m_planes)
                    {
                        if(p->net != net)
                            continue;

                        p->hp = hp;
                        break;
                    }
                }
            }
        }

        for (auto &p: m_planes)
        {
            if (!p->net || p->net->source)
                continue;

            p->phys->pos = p->net->pos;
            p->phys->rot = p->net->rot;
            p->phys->vel = p->net->vel;
            p->controls.rot = p->net->ctrl_rot;
            p->controls.throttle = p->net->ctrl_throttle;
            p->controls.brake = p->net->ctrl_brake;
            p->controls.mgun = p->net->ctrl_mgun;
            p->phys->update(dt);

            if (p->net->ctrl_mgp)
            {
                p->special_weapon_selected = true;
                p->controls.missile = true;
                p->special_count = 9000;
            }
            else
                p->controls.missile = false;
        }

        for (auto &m: m_missiles)
        {
            if (!m->net || m->net->source)
                continue;

            m->phys->pos = m->net->pos;
            m->phys->rot = m->net->rot;
            m->phys->vel = m->net->vel;
            m->phys->target_dir = m->net->target_dir;
            m->phys->accel_started = m->net->engine_started;
            m->phys->update(dt);
            m->phys->accel_started = m->net->engine_started;

            m->target.reset();

            auto target_net = m_network->get_plane(m->net->target);
            if (!target_net)
                continue;

            for (auto &p: m_planes)
            {
                if(p->net == target_net)
                {
                    m->target = p;
                    break;
                }
            }
        }
    }

    auto removed_planes = std::remove_if(m_planes.begin(), m_planes.end(), [](const plane_ptr &p) { return p.unique() && (!p->net || p->net->source || p->net.unique()); });
    if (removed_planes != m_planes.end())
    {
        m_planes.erase(removed_planes, m_planes.end());
        for (auto &p: m_planes)
            p->targets.erase(remove_if(p->targets.begin(), p->targets.end(), [](const plane::target_lock &t){ return t.target.expired(); }), p->targets.end());
    }

    m_missiles.erase(std::remove_if(m_missiles.begin(), m_missiles.end(), [](const missile_ptr &m) { return (m->net && !m->net->source) ? m->net.unique() : m->time <= 0; }), m_missiles.end());

    m_bombs.erase(std::remove_if(m_bombs.begin(), m_bombs.end(), [](const bomb_ptr &b) { return b->dead; }), m_bombs.end());

    m_units.erase(std::remove_if(m_units.begin(), m_units.end(), [](const unit_ptr &u) { return u.unique(); }), m_units.end());

    for (auto &p: m_planes)
        p->phys->controls = p->controls;

    m_phys_world.update_planes(dt, [this](const phys::object_ptr &a, const phys::object_ptr &b)
    {
        auto p = this->get_plane(a);

        if (!b) //hit ground
        {
            if (p->hp > 0)
            {
                p->take_damage(9000, *this);
                on_kill(object_ptr(), p);
                p->render->set_hide(true);
                play_sound("PLAYER_CRASH_AIRPLANE", 0, p->get_pos());
            }
        }
    });

    for (auto &u: m_units) if (u->is_active()) u->update(dt, *this);

    for (auto &p: m_planes)
        p->alert_dirs.clear();

    for (auto &m: m_missiles)
        m->update_homing(dt, *this);

    m_phys_world.update_missiles(dt, [this](const phys::object_ptr &a, const phys::object_ptr &b)
    {
        auto m = this->get_missile(a);
        if (m && m->time > 0)
        {
            this->spawn_explosion(m->phys->pos, m->dmg_radius * 0.5f);
            m->time = 0;
            const bool target_alive = !m->target.expired() && m->target.lock()->hp > 0;
            const bool hit = area_damage(m->phys->pos, m->dmg_radius, m->dmg, m->owner.lock());
            if (m->owner.lock() == get_player() && target_alive)
            {
                if (hit)
                    this->popup_hit(m->target.lock()->hp <= 0);
                else
                    this->popup_miss();
            }
        }
    });

    m_phys_world.update_bombs(dt, [this](const phys::object_ptr &a, const phys::object_ptr &b)
    {
        auto m = this->get_bomb(a);
        if (m && !m->dead)
        {
            this->spawn_explosion(m->phys->pos, m->dmg_radius * 0.5f);
            m->dead = true;
            area_damage(m->phys->pos, m->dmg_radius, m->dmg, m->owner.lock());
        }
    });

    m_phys_world.update_bullets(dt);

    if (!m_player.expired())
    {
        auto p = m_player.lock();

        if (p->controls.change_radar && p->controls.change_radar != p->last_controls.change_radar)
            m_hud.change_radar();

        if (p->controls.change_camera && p->controls.change_camera != p->last_controls.change_camera)
        {
            switch (p->render->get_camera_mode())
            {
                case renderer::aircraft::camera_mode_third: p->render->set_camera_mode(renderer::aircraft::camera_mode_cockpit); break;
                case renderer::aircraft::camera_mode_cockpit: p->render->set_camera_mode(renderer::aircraft::camera_mode_first); break;
                case renderer::aircraft::camera_mode_first: p->render->set_camera_mode(renderer::aircraft::camera_mode_third); break;
            }
        }
    }

    if (!m_player.expired())
        m_player.lock()->update_hud(*this, m_hud);

    for (auto &p: m_planes)
        p->update(dt, *this);

    for (auto &m: m_missiles)
        m->update(dt, *this);

    for (auto &b: m_bombs)
        b->update(dt, *this);

    const auto &bullets_from = m_phys_world.get_bullets();
    auto &bullets_to = m_render_world.get_bullets();
    bullets_to.clear();
    for (auto &b: bullets_from)
        bullets_to.add_bullet(b.pos, b.vel);

    m_render_world.update(dt);
    m_sound_world.update(dt);

    if (m_network)
    {
        for (auto &p: m_planes)
        {
            if (!p->net)
                continue;

            if (p->net_game_data.changed)
                m_network->send_game_data(p->net, p->net_game_data);

            p->net->pos = p->phys->pos;
            p->net->rot = p->phys->rot;
            p->net->vel = p->phys->vel;

            if (!p->net->source)
                continue;

            p->net->ctrl_rot = p->controls.rot;
            p->net->ctrl_throttle = p->controls.throttle;
            p->net->ctrl_brake = p->controls.brake;

            p->net->ctrl_mgun = p->controls.mgun;
            p->net->ctrl_mgp = p->special_weapon_selected && p->controls.missile && p->special.id == "MGP" && p->special_count > 0;
        }

        for (auto &m: m_missiles)
        {
            if (!m->net)
                continue;

            m->net->pos = m->phys->pos;
            m->net->rot = m->phys->rot;
            m->net->vel = m->phys->vel;

            if (!m->net->source)
                continue;

            m->net->target_dir = m->phys->target_dir;

            auto p = get_plane(m->target);
            m->net->target = p ? m_network->get_plane_id(p->net) : invalid_id;

            m->net->engine_started = m->phys->accel_started;
        }

        m_network->update_post(dt);
    }

    for (auto &p: m_planes)
    {
        if (!p->net_game_data.changed)
            continue;

        m_net_data_updated = true;
        p->net_game_data.changed = false;
    }
}

//------------------------------------------------------------

bool world::is_ally(const plane_ptr &a, const plane_ptr &b)
{
    if (a == b)
        return true;

    if (!a || !b || !m_ally_handler)
        return false;

    return m_ally_handler(a, b);
}

//------------------------------------------------------------

void world::on_kill(const object_ptr &k, const object_ptr &v)
{
    if (m_on_kill_handler)
        m_on_kill_handler(k, v);
}

//------------------------------------------------------------

plane_ptr world::get_plane(const phys::object_ptr &o)
{
    if (!o)
        return plane_ptr();

    for (auto &p: m_planes)
    {
        if (p->phys == o)
            return p;
    }

    return plane_ptr();
}

//------------------------------------------------------------

missile_ptr world::get_missile(const phys::object_ptr &o)
{
    for (auto &m: m_missiles)
    {
        if (m->phys == o)
            return m;
    }

    return missile_ptr();
}

//------------------------------------------------------------

bomb_ptr world::get_bomb(const phys::object_ptr &o)
{
    for (auto &b: m_bombs)
    {
        if (b->phys == o)
            return b;
    }

    return bomb_ptr();
}

//------------------------------------------------------------

namespace
{
    enum
    {
        popup_priority_miss,
        popup_priority_hit,
        popup_priority_assist,
        popup_priority_destroyed,
        popup_priority_mission_update,
        popup_priority_mission_result = gui::hud::popup_priority_mission_result
    };
}

//------------------------------------------------------------

void world::popup_hit(bool destroyed)
{
    if (destroyed)
        m_hud.popup(L"DESTROYED", popup_priority_destroyed);
    else
        m_hud.popup(L"HIT", popup_priority_hit);
}

//------------------------------------------------------------

void world::popup_miss()
{
    m_hud.popup(L"MISS", popup_priority_miss, gui::hud::red);
}

//------------------------------------------------------------

void world::popup_mission_clear()
{
    m_hud.popup(L"MISSION ACCOMPLISHED", popup_priority_mission_result);
    m_sound_world.play_ui(m_sounds_common.get("MISSION_CLEAR"));
}

//------------------------------------------------------------

void world::popup_mission_update()
{
    m_hud.popup(L"MISSION STATUS UPDATE", popup_priority_mission_update, gui::hud::green, 5000);
    play_sound_ui("MISSION_UPDATE");
}

//------------------------------------------------------------

void world::popup_mission_fail()
{
    m_hud.popup(L"MISSION FAILED", popup_priority_mission_result, gui::hud::red);
    play_sound_ui("MISSION_FAILED");
}

//------------------------------------------------------------

sound::source_ptr world::add_sound(std::string name, int idx, bool loop)
{
    return m_sound_world.add(m_sounds.get(name, idx), loop);
}

//------------------------------------------------------------

unsigned int world::play_sound_ui(std::string name, bool loop)
{
    return m_sound_world.play_ui(m_sounds_ui.get(name), 1.0f, loop);
}

//------------------------------------------------------------

void world::stop_sound_ui(unsigned int id)
{
    m_sound_world.stop_ui(id);
}

//------------------------------------------------------------

void world::play_sound(std::string name, int idx, vec3 pos)
{
    m_sound_world.play(m_sounds.get(name, idx), pos);
}

//------------------------------------------------------------
}

