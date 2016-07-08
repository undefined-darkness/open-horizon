//
// open horizon -- undefined_darkness@outlook.com
//

#include "game.h"
#include "objects.h"
#include "network_helpers.h"
#include "math/scalar.h"
#include "util/util.h"
#include "util/xml.h"
#include "util/config.h"
#include <algorithm>
#include <time.h>

namespace game
{
//------------------------------------------------------------

namespace
{
     //ToDo
    const int missile_damage = 80;
    const float missile_dmg_radius = 20;
}

//------------------------------------------------------------

namespace { const static params::text_params &get_arms_param() { static params::text_params ap("Arms/ArmsParam.txt"); return ap; } }

//------------------------------------------------------------

class weapon_information
{
public:
    struct weapon
    {
        std::string id;
        std::string model;
        int count;
    };

    struct aircraft
    {
        std::wstring name;
        std::wstring short_name;
        std::string role;
        weapon missile;
        std::vector<weapon> special;
    };

public:
    static weapon_information &get()
    {
        static weapon_information info("aircrafts.xml");
        return info;
    }

    aircraft *get_aircraft_weapons(const char *name)
    {
        if (!name)
            return 0;

        std::string name_str(name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);
        auto a = m_aircrafts.find(name);
        if (a == m_aircrafts.end())
            return 0;

        return &a->second;
    }

    const std::vector<std::string> &get_aircraft_ids(const std::string &role)
    {
        for (auto &l: m_lists)
        {
            if (l.first == role)
                return l.second;
        }

        const static std::vector<std::string> empty;
        return empty;
    }

private:
    weapon_information(const char *xml_name)
    {
        pugi::xml_document doc;
        if (!load_xml(xml_name, doc))
            return;

        pugi::xml_node root = doc.child("aircrafts");
        if (!root)
            return;

        for (pugi::xml_node ac = root.child("aircraft"); ac; ac = ac.next_sibling("aircraft"))
        {
            auto id = ac.attribute("id").as_string("");
            aircraft &a = m_aircrafts[id];
            a.role = ac.attribute("role").as_string("");
            if (!a.role.empty())
                aircraft_ids(a.role).push_back(id);
            const std::string name = ac.attribute("name").as_string("");
            a.name = std::wstring(name.begin(), name.end());
            const std::string short_name = ac.attribute("short_name").as_string("");
            if (short_name.empty())
            {
                auto sp = a.name.find(' ');
                if (sp == std::wstring::npos)
                    a.short_name = a.name;
                else
                    a.short_name = a.name.substr(0, sp);
            }
            else
                a.short_name = std::wstring(short_name.begin(), short_name.end());

            for (pugi::xml_node wpn = ac.first_child(); wpn; wpn = wpn.next_sibling())
            {
                weapon w;
                w.id = wpn.attribute("id").as_string("");
                w.model = wpn.attribute("model").as_string("");
                w.count = wpn.attribute("count").as_int(0);

                std::string name(wpn.name() ? wpn.name() : "");
                if (name == "msl")
                    a.missile = w;
                else if (name == "spc")
                    a.special.push_back(w);
            }
        }
    }

    std::vector<std::string> &aircraft_ids(const std::string &role)
    {
        for (auto &l: m_lists)
        {
            if (l.first == role)
                return l.second;
        }

        m_lists.push_back({role, {}});
        return m_lists.back().second;
    }


    std::map<std::string, aircraft> m_aircrafts;
    std::vector<std::pair<std::string, std::vector<std::string> > > m_lists;
};

//------------------------------------------------------------

wpn_params::wpn_params(std::string id, std::string model)
{
    const std::string action = "." + id + ".action.";
    const std::string lockon = "." + id + ".lockon.";
    const std::string lockon_dfm = "." + id + ".lockon_dfm.";

    this->id = id;
    this->model = model;

    auto &param = get_arms_param();

    lockon_range = param.get_float(lockon + "range");
    lockon_time = param.get_float(lockon_dfm + "timeLockon") * 1000;
    lockon_count = param.get_float(lockon + "lockonNum");
    lockon_air = param.get_int(lockon + "target_air") > 0;
    lockon_ground = param.get_int(lockon + "target_grd") > 0;
    action_range = param.get_float(action + "range");
    reload_time = param.get_float(lockon + "reload") * 1000;

    float lon_angle = param.get_float(lockon + "angle") * 0.5f * nya_math::constants::pi / 180.0f;
    //I dunno
    if (id == "SAAM")
        lon_angle *= 0.3f;
    lockon_angle_cos = cosf(lon_angle);
}

//------------------------------------------------------------

std::vector<std::string> get_aircraft_ids(const std::vector<std::string> &roles)
{
    std::vector<std::string> planes;
    for (auto &r: roles)
    {
        auto &l = weapon_information::get().get_aircraft_ids(r);
        planes.insert(planes.end(), l.begin(), l.end());
    }

    return planes;
}

//------------------------------------------------------------

const std::wstring &get_aircraft_name(const std::string &id)
{
    auto *a = weapon_information::get().get_aircraft_weapons(id.c_str());
    if(!a)
    {
        static std::wstring invalid;
        return invalid;
    }

    return a->name;
}

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
    m->homing_angle_cos = cosf(param.get_float(".MISSILE.action.hormingAng"));
    if (strcmp(id, "SAAM") == 0)
        m->is_saam = true;

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

    p->hp = p->max_hp = int(p->phys->params.misc.maxHp);

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
        }

        p->render->load_missile(wi->missile.model.c_str(), m_render_world.get_location_params());
        p->missile = wpn_params(wi->missile.id, wi->missile.model);
        p->missile_max_count = wi->missile.count;
    }

    p->missile_count = p->missile_max_count;
    p->special_count = p->special_max_count;

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

    m_planes.push_back(p);
    get_arms_param(); //cache
    return p;
}

//------------------------------------------------------------

struct unit_vehicle: public unit
{
public:
    virtual void set_path(const path &p, bool loop) override { m_path = p; m_path_loop = loop; }
    virtual void set_follow(object_wptr f) override { m_follow = f; m_target.reset(); }
    virtual void set_target(object_wptr t) override { m_target = t; m_follow.reset(); }

    virtual void update(int dt, world &w) override
    {
        if (m_first_update)
        {
            m_first_update = false;

            if (!m_follow.expired())
            {
                auto f = m_follow.lock();
                vec3 diff = get_pos() - f->get_pos();
                if (diff.length() < 100.0f)
                    m_formation_offset = f->get_rot().rotate_inv(diff);
            }

            m_ground = (get_pos().y - w.get_height(get_pos().x, get_pos().z)) < 5.0f;
            if (m_ground)
            {
                //ToDo: gear anim
            }
            else
            {
                if (m_params.speed_min > 0.01f)
                    m_vel = get_rot().rotate(vec3::forward() * m_params.speed_cruise);
                m_dpos = vec3();
            }
        }

        float kdt = dt * 0.001f;

        vec3 target_dir;
        bool has_target = false;

        if (!m_path.empty())
        {
            has_target = true;
            target_dir = m_path.front() - get_pos();
            if (target_dir.length() < m_vel.length() * kdt * 2.0)
            {
                if (m_path_loop)
                    m_path.push_back(m_path.front());
                m_path.pop_front();
                if (m_path.empty())
                    has_target = false;
                else
                    target_dir = m_path.front() - get_pos();
            }
        }
        else if (!m_follow.expired())
        {
            has_target = true;
            auto f = m_follow.lock();
            target_dir = f->get_pos() + f->get_rot().rotate(m_formation_offset) - (get_pos() + get_vel() * kdt);
            //+ f->get_vel() * kdt //player's plane is already updated, ToDo
        }

        if (has_target)
        {
            const float eps=1.0e-6f;
            const float target_dist = target_dir.length();
            const vec3 v=target_dir / target_dist;
            const float xz_sqdist=v.x*v.x+v.z*v.z;

            auto rot = get_rot();
            auto pyr = rot.get_euler();

            const nya_math::angle_rad new_yaw=(xz_sqdist>eps*eps)? (atan2(v.x,v.z)) : pyr.y;
            const nya_math::angle_rad new_pitch=(fabsf(v.y)>eps)? (-atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;
            nya_math::angle_rad new_roll;
            if (!m_follow.expired())
                new_roll = m_follow.lock()->get_rot().get_euler().z;

            auto ideal_rot = quat(new_pitch, new_yaw, new_roll);

            const float angle_diff = rot.rotate(vec3::forward()).angle(target_dir).get_deg();
            if (fabsf(angle_diff) > 0.001f)
            {
                const float turn_k = nya_math::clamp((180.0f / angle_diff) * m_params.turn_speed * kdt, 0.0, 1.0);
                m_render->mdl.set_rot(quat::slerp(rot, ideal_rot, turn_k));
            }

            float speed = m_vel.length();
            float want_speed = target_dist / kdt;
            if (!m_follow.expired())
            {
                const float target_speed = m_follow.lock()->get_vel().length();
                float decel_time = (speed - target_speed) / m_params.decel;
                if (target_dist / (speed + m_params.accel * kdt) < decel_time)
                    want_speed = target_speed;
            }

            want_speed = nya_math::clamp(want_speed, m_params.speed_min, m_params.speed_max);
            speed = tend(speed, want_speed, m_params.accel * kdt, m_params.decel * kdt);
            m_vel = get_rot().rotate(vec3(0.0, 0.0, speed));
        }
        else if (m_params.speed_min < 0.01f)
        {
            float speed = m_vel.length();
            if (speed > 0.0f)
            {
                speed = tend(speed, 0.0f, m_params.accel * kdt, m_params.decel * kdt);
                m_vel = get_rot().rotate(vec3(0.0, 0.0, speed));
            }
        }

        set_pos(get_pos() + m_vel * kdt);
    }

    unit_vehicle(const object_params &p): m_params(p) {}

    //object
public:
    vec3 get_vel() override { return m_vel; }
    virtual bool is_targetable(bool air, bool ground) override { return hp > 0 && (ground == m_ground || air != m_ground); }
    virtual float get_hit_radius() override { return m_params.hit_radius; }

protected:
    float tend(float value, float target, float accel, float deccel)
    {
        const float diff = target - value;
        if (diff > accel) return value + accel;
        if (-diff > deccel) return value - deccel;
        return target;
    }

protected:
    unit::path m_path;
    bool m_path_loop = false;
    bool m_ground = true;
    object_wptr m_target;
    object_wptr m_follow;
    bool m_first_update = true;
    vec3 m_vel;
    vec3 m_formation_offset;
    object_params m_params;
};

//------------------------------------------------------------

struct unit_object: public unit
{
    virtual bool is_targetable(bool air, bool ground) override { return hp > 0 && ground; }
};

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
            u = std::make_shared<unit_vehicle>(unit_vehicle(o.params));
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

inline bool line_sphere_intersect(const vec3 &start, const vec3 &end, const vec3 &sp_center, const float sp_radius)
{
    const vec3 diff = end - start;
    const float len = nya_math::clamp(diff.dot(sp_center - start) / diff.length_sq(), 0.0f, 1.0f);
    const vec3 inter_pt = start + len  * diff;
    return (inter_pt - sp_center).length_sq() <= sp_radius * sp_radius;
}

//------------------------------------------------------------

void world::spawn_bullet(const char *type, const vec3 &pos, const vec3 &dir, const plane_ptr &owner)
{
    vec3 r;
    const bool hit_world = m_phys_world.spawn_bullet(type, pos, dir, r);

    if (!owner->net || owner->net->source)
    {
        for (int i = 0; i < get_planes_count() + get_units_count(); ++i)
        {
            auto t = i < get_planes_count() ? (object_ptr)m_planes[i] : m_units[i - get_planes_count()];
            if (t->hp <= 0 || t->is_ally(owner, *this))
                continue;

            const float spread_coeff = 2.0f;
            if (line_sphere_intersect(pos, r, t->get_pos(), t->get_hit_radius() * spread_coeff))
            {
                t->take_damage(60, *this);
                const bool destroyed = t->hp <= 0;

                if(destroyed && i < get_planes_count())
                    on_kill(owner, m_planes[i]);

                if (owner == get_player())
                    popup_hit(destroyed);
            }
        }
    }

    if (hit_world)
    {
        //ToDo: spark
    }
}

//------------------------------------------------------------

bool world::area_damage(const vec3 &pos, float radius, int damage, const plane_ptr &owner)
{
    bool hit = false;

    for (auto &p: m_planes)
    {
        if (!p || p->hp <= 0 || is_ally(owner, p) || (p->get_pos() - pos).length() > radius)
            continue;

        p->take_damage(damage, *this);
        hit = true;

        if (p->hp <= 0)
            on_kill(owner, p);
    }

    for (auto &u: m_units)
    {
        if (!u || u->hp <= 0 || u->is_ally(owner, *this) || (u->get_pos() - pos).length() > radius)
            continue;

        u->take_damage(damage, *this);
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

void world::set_location(const char *name)
{
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
                on_kill(plane_ptr(), p);
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
            this->spawn_explosion(m->phys->pos, missile_dmg_radius / 2);
            m->time = 0;
            const bool target_alive = !m->target.expired() && m->target.lock()->hp > 0;
            const bool hit = area_damage(m->phys->pos, missile_dmg_radius, missile_damage, m->owner.lock());
            if (m->owner.lock() == get_player() && target_alive)
            {
                if (hit)
                    this->popup_hit(m->target.lock()->hp <= 0);
                else
                    this->popup_miss();
            }
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

    for (auto &p: m_planes)
        p->update(dt, *this);

    if (!m_player.expired())
        m_player.lock()->update_hud(*this, m_hud);

    for (auto &m: m_missiles)
        m->update(dt, *this);

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

    if (!m_ally_handler)
        return false;

    return m_ally_handler(a, b);
}

//------------------------------------------------------------

void world::on_kill(const plane_ptr &k, const plane_ptr &v)
{
    if (m_on_kill_handler)
        m_on_kill_handler(k, v);
}

//------------------------------------------------------------

plane_ptr world::get_plane(const phys::object_ptr &o)
{
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

namespace
{
    enum
    {
        popup_priority_miss,
        popup_priority_hit,
        popup_priority_assist,
        popup_priority_destroyed,
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

void world::popup_mission_fail()
{
    m_hud.popup(L"MISSION FAILED", popup_priority_mission_result, gui::hud::red);
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

bool plane::is_ally(const plane_ptr &p, world &w) { return w.is_ally(p, shared_from_this()); }

//------------------------------------------------------------

void plane::reset_state()
{
    hp = max_hp;
    targets.clear();

    phys->reset_state();

    render->set_dead(false);

    //render->reset_state(); //ToDo

    render->set_special_visible(-1, true);

    need_fire_missile = false;
    missile_bay_time = 0;

    for (auto &c: missile_cooldown) c = 0;
    missile_mount_cooldown.clear();
    missile_mount_idx = 0;
    missile_count = missile_max_count;

    for (auto &c: special_cooldown) c = 0;
    special_mount_cooldown.clear();
    special_mount_idx = 0;
    special_count = special_max_count;

    jammed = false;
    ecm_time = 0;
}

//------------------------------------------------------------

void plane::select_target(const object_wptr &o)
{
    for (auto &t: targets)
    {
        if (t.target.lock() != o.lock())
            continue;

        std::swap(t, targets.front());
        return;
    }
}

//------------------------------------------------------------

void plane::update_targets(world &w)
{
    const plane_ptr &me = shared_from_this();
    const vec3 dir = get_dir();
    jammed = false;

    bool lock_air = special_weapon_selected ? (bool)special.lockon_air : true;
    bool lock_ground = special_weapon_selected ? (bool)special.lockon_ground : true;

    bool first_target = targets.empty();

    for (int i = 0; i < w.get_planes_count() + w.get_units_count(); ++i)
    {
        const object_ptr &o = i < w.get_planes_count() ? (object_ptr)w.get_plane(i) : w.get_unit(i - w.get_planes_count());
        if (o->is_ally(me, w))
            continue;

        auto t = std::find_if(targets.begin(), targets.end(), [o](target_lock &t) { return o == t.target.lock(); });
        auto target_dir = o->get_pos() - me->get_pos();
        const float dist = target_dir.length();

        if (dist > 12000.0f || !o->is_targetable(lock_air, lock_ground))
        {
            if (t != targets.end())
                targets.erase(t);
            continue;
        }

        if (i < w.get_planes_count())
        {
            auto p = w.get_plane(i);
            if (p->is_ecm_active() && dist < p->special.action_range)
            {
                jammed = true;
                targets.clear();
                break;
            }
        }

        if (t == targets.end())
        {
            target_lock l;
            l.target = o;
            t = targets.insert(targets.end(), l);
        }

        t->dist = dist;
        t->cos = target_dir.dot(dir) / dist;
    }

    if (targets.size() > 1)
        std::sort(first_target ? targets.begin() : std::next(targets.begin()), targets.end(), [](const target_lock &a, const target_lock &b) { return a.dist < b.dist; });
}

//------------------------------------------------------------

void plane::update_render(world &w)
{
    render->set_hide(hp <= 0);
    if (hp <= 0)
    {
        sound_srcs.clear();
        return;
    }

    render->set_pos(phys->pos);
    render->set_rot(phys->rot);

    render->set_damage(max_hp ? float(max_hp-hp) / max_hp : 0.0);

    const float speed = phys->get_speed_kmh();
    const float speed_k = nya_math::max((phys->params.move.speed.speedMax - speed) / phys->params.move.speed.speedMax, 0.1f);

    render->set_speed(speed);

    const float el = nya_math::clamp(-controls.rot.z - controls.rot.x, -1.0f, 1.0f) * speed_k;
    const float er = nya_math::clamp(controls.rot.z - controls.rot.x, -1.0f, 1.0f) * speed_k;
    render->set_elev(el, er);

    const float rl = nya_math::clamp(-controls.rot.y + controls.brake, -1.0f, 1.0f) * speed_k;
    const float rr = nya_math::clamp(-controls.rot.y - controls.brake, -1.0f, 1.0f) * speed_k;
    render->set_rudder(rl, rr, -controls.rot.y);

    render->set_aileron(-controls.rot.z * speed_k, controls.rot.z * speed_k);
    render->set_canard(controls.rot.x * speed_k);
    render->set_brake(controls.brake);
    render->set_flaperon(speed < phys->params.move.speed.speedCruising - 100 ? -1.0 : 1.0);
    render->set_wing_sweep(speed >  phys->params.move.speed.speedCruising + 250 ? 1.0 : -1.0);

    render->set_intake_ramp(phys->thrust_time >= phys->params.move.accel.thrustMinWait ? 1.0 : -1.0);
    render->set_thrust(phys->get_thrust());

    const float aoa = acosf(nya_math::vec3::dot(nya_math::vec3::normalize(phys->vel), get_dir()));
    render->set_aoa(aoa);

    render->set_missile_bay(missile_bay_time > 0);
    render->set_special_bay(special_weapon_selected);
    render->set_mgun_bay(controls.mgun);

    const bool mg_fire = is_mg_bay_ready() && controls.mgun;
    const bool mgp_fire = special_weapon_selected && controls.missile && special.id == "MGP" && special_count > 0;

    render->set_mgun_fire(mg_fire);
    render->set_mgp_fire(mgp_fire);

    update_sound(w, "VULCAN_REAR", mg_fire);
    update_sound(w, "MGP", mgp_fire);
    update_sound(w, "75p", true, phys->get_thrust());
    update_sound(w, "JET_REAR_AB", phys->get_ab());

    sound_rel_srcs.erase(std::remove_if(sound_rel_srcs.begin(), sound_rel_srcs.end(), [](const sound_rel_src &s){ return s.second.unique(); }), sound_rel_srcs.end());
    for (auto &s: sound_rel_srcs)
    {
        s.second->pos = phys->pos + phys->rot.rotate(s.first);
        s.second->vel = phys->vel;
    }
}

//------------------------------------------------------------

bool plane::is_mg_bay_ready()
{
    return render->is_mgun_ready();
}

//------------------------------------------------------------

bool plane::is_special_bay_ready()
{
    if (!render->has_special_bay())
        return true;

    return special_weapon_selected ? render->is_special_bay_opened() : render->is_special_bay_closed();
}

//------------------------------------------------------------

void plane::play_relative(world &w, std::string name, int idx, vec3 offset)
{
    sound_rel_srcs.push_back({offset, w.add_sound(name, idx)});
}

//------------------------------------------------------------

void plane::update_sound(world &w, std::string name, bool enabled, float volume)
{
    auto &snd = sound_srcs[name];

    if (!enabled)
    {
        if (snd)
            snd.reset();
        return;
    }

    if (snd)
    {
        snd->pos = phys->pos;
        snd->vel = phys->vel;
        snd->volume = volume;
    }
    else
    {
        if (sounds.has(name))
            snd = w.add_sound(sounds.get(name), true);
        else
            snd = w.add_sound(name, 0, true);
    }
}

//------------------------------------------------------------

void plane::update_ui_sound(world &w, std::string name, bool enabled)
{
    auto &snd = sounds_ui[name];

    if (enabled)
    {
        if (!snd)
            snd = w.play_sound_ui(name, true);
    }
    else
    {
        if (snd)
            w.stop_sound_ui(snd), snd = 0;
    }
}

//------------------------------------------------------------

void plane::update(int dt, world &w)
{
    const plane_ptr &me = shared_from_this();
    const vec3 dir = get_dir();
    const vec3 pos_fix = phys->pos - render->get_pos(); //skeleton is not updated yet

    update_render(w);

    if (hp <= 0)
        return;

    phys->wing_offset = render->get_wing_offset();

    update_targets(w);

    //switch weapons

    if (controls.change_weapon && controls.change_weapon != last_controls.change_weapon && is_special_bay_ready())
    {
        special_weapon_selected = !special_weapon_selected;

        for (auto &t: targets)
            t.locked = 0;

        if (!saam_missile.expired())
            saam_missile.lock()->target.reset();

        missile_bay_time = 0;
        need_fire_missile = false;
    }

    //mgun

    if (controls.mgun && render->is_mgun_ready())
    {
        mgun_fire_update += dt;
        const int mgun_update_time = 150;
        if (mgun_fire_update > mgun_update_time)
        {
            mgun_fire_update %= mgun_update_time;

            for (int i = 0; i < render->get_mguns_count(); ++i)
                w.spawn_bullet("MG", render->get_mgun_pos(i) + pos_fix, dir, me);
        }
    }

    //reload

    for(int i = 0; i < 2; ++i)
    {
        auto &count = i == 0 ? missile_count : special_count;
        auto *cd = i == 0 ? missile_cooldown : special_cooldown;

        if (count == 1)
        {
            if (cd[0] > 0 && cd[1] > 0)
            {
                auto &m = cd[1] < cd[0] ?  cd[1] :  cd[0];
                if (m > 0)
                    m -= dt;
            }
        }
        else if (count > 1)
        {
            if (cd[0] > 0) cd[0] -= dt;
            if (cd[1] > 0) cd[1] -= dt;
        }
    }

    for (int i = 0; i < (int)missile_mount_cooldown.size(); ++i)
    {
        if (missile_mount_cooldown[i] < 0)
            continue;

        missile_mount_cooldown[i] -= dt;
        if (missile_mount_cooldown[i] >= 0)
            continue;

        if (missile_count <= 0)
            continue;

        if (missile_count == 1 && missile_mount_cooldown[i == 0 ? 1 : 0] <= 0)
            continue;

        render->set_missile_visible(i, true);
    }

    for (int i = 0; i < (int)special_mount_cooldown.size(); ++i)
    {
        if (special_mount_cooldown[i] < 0)
            continue;

        special_mount_cooldown[i] -= dt;
        if (special_mount_cooldown[i] >= 0)
            continue;

        //ToDo: don't set visible on low ammo
        render->set_special_visible(i, true);
    }

    //ecm

    if (is_ecm_active())
    {
        for (int i = 0; i < w.get_missiles_count(); ++i)
        {
            auto m = w.get_missile(i);
            if (!m)
                continue;

            if (!m->owner.expired() && w.is_ally(me, m->owner.lock()))
                continue;

            if ((get_pos() - m->phys->pos).length_sq() > special.action_range * special.action_range)
                continue;

            m->target.reset();
        }
    }

    //weapons

    if (missile_bay_time > 0)
        missile_bay_time -= dt;

    if (ecm_time > 0)
        ecm_time -= dt;

    const bool want_fire = controls.missile && controls.missile != last_controls.missile;

    if (special_weapon_selected)
    {
        const bool can_fire = (special_cooldown[0] <=0 || special_cooldown[1] <= 0) && is_special_bay_ready() && special_count > 0;
        const bool need_fire = want_fire && can_fire;

        if (controls.missile && special.id == "MGP" && special_count > 0)
        {
            mgp_fire_update += dt;
            const int mgp_update_time = 150;
            if (mgp_fire_update > mgp_update_time)
            {
                --special_count;
                mgp_fire_update %= mgp_update_time;

                for (int i = 0; i < render->get_special_mount_count(); ++i)
                    w.spawn_bullet("MGP", render->get_special_mount_pos(i) + pos_fix, dir, me);
            }
        }

        if (need_fire && special.id == "ECM")
        {
            ecm_time = special_cooldown[0] = special_cooldown[1] = special.reload_time;
            --special_count;

            play_relative(w, "ECM", 0, vec3());
        }

        if (special.id == "ADMM")
        {
            //ToDo
        }

        if (need_fire && special.id == "EML")
        {
            //ToDo
        }

        if (special.id == "QAAM")
        {
            if (!targets.empty())
            {
                auto t = targets.begin();

                if (can_fire && t->can_lock(special))
                {
                    if (lock_timer > special.lockon_time)
                    {
                        lock_timer %= special.lockon_time;
                        t->locked = 1;
                    }

                    lock_timer += dt;
                }
                else
                    t->locked = 0, lock_timer = 0;
            }
        }

        if (special.id == "SAAM")
        {
            if (!targets.empty())
                targets.front().locked = targets.front().can_lock(special) ? 1 : 0;

            if (!saam_missile.expired())
            {
                auto m = saam_missile.lock();
                if (!targets.empty() && targets.front().locked > 0)
                    m->target = targets.begin()->target;
                else
                    m->target.reset();
            }
        }

        if (special.lockon_count > 1)
        {
            lock_timer += dt;
            if (can_fire)
            {
                for (auto &t: targets) if (!t.can_lock(special)) t.locked = 0;

                if (lock_timer > special.lockon_time)
                {
                    if (special.lockon_time)
                        lock_timer %= special.lockon_time;

                    int lockon_count = 0;
                    for (auto &t: targets)
                    {
                        if (!t.can_lock(special))
                            t.locked = 0;

                        lockon_count += t.locked;
                    }

                    if (lockon_count < special.lockon_count && lockon_count < special_count)
                    {
                        for (int min_lock = 0; min_lock < special.lockon_count; ++min_lock)
                        {
                            bool found = false;
                            for (auto &t: targets)
                            {
                                if (t.locked > min_lock)
                                    continue;

                                if (!t.can_lock(special))
                                    continue;

                                ++t.locked;
                                found = true;
                                break;
                            }

                            if (found)
                                break;
                        }
                    }
                }
            }
            else
                lock_timer = 0;
        }

        if (need_fire && special.lockon_count > 0 && render->get_special_mount_count() > 0)
        {
            if (special.lockon_count > 1)
                special_cooldown[0] = special_cooldown[1] = special.reload_time;
            else
                for (auto &s: special_cooldown) if (s <= 0) { s = special.reload_time; break; }

            special_mount_cooldown.resize(render->get_special_mount_count());

            std::vector<object_wptr> locked_targets;
            for (auto &t: targets)
            {
                for (int j = 0; j < t.locked; ++j)
                    locked_targets.push_back(t.target);
                t.locked = 0;
            }

            const int shot_cout = special_count > special.lockon_count ? special.lockon_count : special_count;
            for (int i = 0; i < shot_cout; ++i)
            {
                auto m = w.add_missile(shared_from_this());
                m->owner = shared_from_this();

                special_mount_idx = ++special_mount_idx % (int)special_mount_cooldown.size();
                special_mount_cooldown[special_mount_idx] = special.reload_time;
                render->set_special_visible(special_mount_idx, false);
                m->phys->pos = render->get_special_mount_pos(special_mount_idx) + pos_fix;
                m->phys->rot = render->get_special_mount_rot(special_mount_idx);
                m->phys->vel = phys->vel;
                m->phys->target_dir = m->phys->rot.rotate(vec3(0.0, 0.0, 1.0)); //ToDo

                if (i < (int)locked_targets.size())
                    m->target = locked_targets[i];

                if (special.id == "SAAM")
                {
                    if (!saam_missile.expired())
                        saam_missile.lock()->target.reset(); //one at a time
                    saam_missile = m;
                }

                if (i == 0)
                {
                    if (special.id == "QAAM")
                        play_relative(w, "SHOT_MSL", random(0, 2), m->phys->rot.rotate_inv(m->phys->pos - get_pos()));
                    else if (shot_cout > 1)
                        play_relative(w, "SAAM", 0, vec3());
                    else
                        play_relative(w, "SAAM", 0, m->phys->rot.rotate_inv(m->phys->pos - get_pos()));
                }
            }

            special_count -= shot_cout;
        }
    }
    else
    {
        if (!targets.empty())
            targets.front().locked = targets.front().can_lock(missile);

        if (want_fire && missile_count > 0)
            need_fire_missile = true, missile_bay_time = 3000;

        if (need_fire_missile && render->is_missile_ready())
        {
            need_fire_missile = false;
            missile_mount_cooldown.resize(render->get_missile_mount_count());
            if ((missile_cooldown[0] <=0 || missile_cooldown[1] <= 0) && render->get_missile_mount_count() > 0)
            {
                if (missile_cooldown[0] <= 0)
                    missile_cooldown[0] = missile.reload_time;
                else if (missile_cooldown[1] <= 0)
                    missile_cooldown[1] = missile.reload_time;

                auto m = w.add_missile(shared_from_this());
                m->owner = shared_from_this();
                missile_mount_idx = ++missile_mount_idx % render->get_missile_mount_count();
                missile_mount_cooldown[missile_mount_idx] = missile.reload_time;
                render->set_missile_visible(missile_mount_idx, false);
                m->phys->pos = render->get_missile_mount_pos(missile_mount_idx) + pos_fix;
                m->phys->rot = render->get_missile_mount_rot(missile_mount_idx);
                m->phys->vel = phys->vel;

                m->phys->target_dir = m->phys->rot.rotate(vec3(0.0f, 0.0f, 1.0f));
                if (!targets.empty() && targets.front().locked > 0)
                    m->target = targets.front().target;

                --missile_count;

                play_relative(w, "SHOT_MSL", random(0, 2), m->phys->rot.rotate_inv(m->phys->pos - get_pos()));
            }
        }
    }

    //target selection

    if (controls.change_target != last_controls.change_target && !controls.change_target )
    {
        if (change_target_hold_time < change_target_hold_max_time && targets.size() > 1)
        {
            if (!special_weapon_selected || special.lockon_count == 1)
                targets.front().locked = 0;

            std::swap(targets.front(), targets[1]);
        }

        change_target_hold_time = 0;
    }

    if (controls.change_target)
        change_target_hold_time += dt;

    last_controls = controls;
}

//------------------------------------------------------------

void plane::update_hud(world &w, gui::hud &h)
{
    bool missile_alert = !alert_dirs.empty(), missile_alert_near = false;
    for (auto &a: alert_dirs)
    {
        if (a.length() < 1000.0f)
            missile_alert_near = true;
    }

    update_ui_sound(w, "HUD_MSL_ALERT", missile_alert && !missile_alert_near);
    update_ui_sound(w, "HUD_MSL_ALERT_NEAR", missile_alert_near);

    update_ui_sound(w, "HUD_MSL_LOCK_ON", !special_weapon_selected && !targets.empty() && targets.front().locked > 0);
    update_ui_sound(w, "HUD_SP_LOCK_ON", special_weapon_selected && special.lockon_count == 1 && !targets.empty() && targets.front().locked > 0);

    h.set_hide(hp <= 0);
    if (hp <= 0)
        return;

    const auto dir = get_dir();
    const auto proj_dir = dir * 1000.0f;
    h.set_project_pos(phys->pos + proj_dir);
    h.set_pos(phys->pos);
    auto angles = phys->rot.get_euler();
    h.set_angles(angles.x, angles.y, angles.z);
    h.set_ab(phys->get_ab());
    h.set_speed(phys->get_speed_kmh());
    h.set_alt(phys->pos.y);
    h.set_jammed(jammed);
    h.set_pitch_ladder(render->get_camera_mode() != renderer::aircraft::camera_mode_third);

    //missile alerts

    h.clear_alerts();
    for(auto &a: alert_dirs)
        h.add_alert(-nya_math::vec2(dir.x, dir.z).angle(nya_math::vec2(a.x, a.z)).get_rad());

    //update weapon reticle and lock icons

    if (special.id == "SAAM")
    {
        const bool locked = !targets.empty() && targets.front().locked > 0;
        h.set_saam(locked, locked && !saam_missile.expired());
    }
    else if (special.id == "MGP")
        h.set_mgp(special_weapon_selected && controls.missile);

    if (special.lockon_count == 1)
    {
        const bool locked = !targets.empty() && targets.front().locked > 0;
        h.set_lock(0, locked && special_cooldown[0] <= 0, special_cooldown[0] <= 0);
        h.set_lock(1, locked && special_cooldown[0] > 0 && special_cooldown[1] <= 0, special_cooldown[1] <= 0);
    }
    else if (special.lockon_count > 1)
    {
        int count = 0;
        for (auto &t: targets)
            count += t.locked;

        for (int i = 0; i < special.lockon_count; ++i)
            h.set_lock(i, i < count && special_cooldown[0] <= 0, special_cooldown[0] <= 0 && i < special_count);
    }

    h.set_missiles_count(special_weapon_selected ? special_count : missile_count);

    const float gun_range = 1500.0f;
    const bool hud_mgun = controls.mgun || (!targets.empty() && targets.front().dist < gun_range);
    h.set_mgun(hud_mgun);

    //update weapon icons

    if (special_weapon_selected != h.is_special_selected())
    {
        w.play_sound_ui(special_weapon_selected ? "HUD_WEP_CHANGE" : "HUD_WEP_CHANGE_NORMAL");

        if (special_weapon_selected)
        {
            const int special_weapon_idx = 0; //ToDo

            h.set_missiles(special.id.c_str(), special_weapon_idx + 1);
            int lock_count = special.lockon_count > 2 ? (int)special.lockon_count : 2;
            if (special.id == "MGP" || special.id == "ECM")
                lock_count = 1;
            h.set_locks(lock_count, special_weapon_idx);
            if (special.id == "SAAM")
                h.set_saam_circle(true, acosf(special.lockon_angle_cos));
        }
        else
        {
            h.set_missiles(missile.id.c_str(), 0);
            h.set_locks(0, 0);
            h.set_saam_circle(false, 0.0f);
            h.set_mgp(false);
        }
    }

    //update reload icons

    if (special_weapon_selected)
    {
        if (special.lockon_count > 1)
        {
            float value = 1.0f - float(special_cooldown[0]) / special.reload_time;
            for (int i = 0; i < special.lockon_count; ++i)
                h.set_missile_reload(i, i < special_count ? value : 0.0f);
        }
        else
        {
            h.set_missile_reload(0, 1.0f - float(special_cooldown[0]) / special.reload_time);
            h.set_missile_reload(1, 1.0f - float(special_cooldown[1]) / special.reload_time);
        }
    }
    else
    {
        h.set_missile_reload(0, 1.0f - float(missile_cooldown[0]) / missile.reload_time);
        h.set_missile_reload(1, 1.0f - float(missile_cooldown[1]) / missile.reload_time);
    }

    //radar and hud targets

    h.clear_targets();

    for (int i = 0; i < w.get_missiles_count(); ++i)
    {
        auto m = w.get_missile(i);
        if (m)
            h.add_target(m->phys->pos, m->phys->rot.get_euler().y, gui::hud::target_missile, gui::hud::select_not);
    }

    const plane_ptr &me = shared_from_this();

    int t_idx = 0;
    for (auto &t: targets)
    {
        auto p = t.target.lock();
        bool ground = p->is_targetable(false, true);
        auto type = t.locked > 0 ? (ground ? gui::hud::target_ground_lock : gui::hud::target_air_lock) :
                                   (ground ? gui::hud::target_ground : gui::hud::target_air);
        auto select = ++t_idx == 1 ? gui::hud::select_current : (t_idx == 2 ? gui::hud::select_next : gui::hud::select_not);
        h.add_target(p->get_type_name(), p->get_name(), p->get_pos(), p->get_rot().get_euler().y, type, select, p->get_tgt());
    }

    if (!targets.empty())
        h.set_target_arrow(true, vec3::normalize(targets.front().target.lock()->get_pos() - get_pos()));
    else
        h.set_target_arrow(false);

    h.clear_ecm();

    for (int i = 0; i < w.get_planes_count(); ++i)
    {
        auto p = w.get_plane(i);

        if (p->is_ecm_active())
            h.add_ecm(p->get_pos());

        if (!w.is_ally(me, p) || me == p)
            continue;

        if (p->hp <= 0)
            continue;

        h.add_target(p->get_type_name(), p->get_name(), p->get_pos(), p->get_rot().get_euler().y, gui::hud::target_air_ally, gui::hud::select_not, p->get_tgt());
    }

    for (int i = 0; i < w.get_units_count(); ++i)
    {
        auto u = w.get_unit(i);
        if (u->hp <= 0)
            continue;

        if (!u->is_ally(me, w))
            continue;

        bool ground = u->is_targetable(false, true);
        auto type = ground ? gui::hud::target_ground_ally : gui::hud::target_air_ally;
        h.add_target(u->get_type_name(), u->get_name(), u->get_pos(), u->get_rot().get_euler().y, type, gui::hud::select_not, u->get_tgt());
    }
}

//------------------------------------------------------------

void plane::take_damage(int damage, world &w, bool net_src)
{
    if (hp <= 0)
        return;

    object::take_damage(damage, w);

    if (!w.is_host() && net_src)
    {
        w.get_network()->general_msg("plane_take_damage " + std::to_string(w.get_network()->get_plane_id(net)) + " " + std::to_string(damage));
        return;
    }

    if (w.is_host() && w.get_network())
        w.get_network()->general_msg("plane_set_hp " + std::to_string(w.get_network()->get_plane_id(net)) + " " + std::to_string(hp));

    if (hp <= 0)
    {
        render->set_dead(true);
        w.spawn_explosion(get_pos(), 30.0f);
        if (!saam_missile.expired())
            saam_missile.lock()->target.reset();
    }
}

//------------------------------------------------------------

void missile::update_homing(int dt, world &w)
{
    if (net && !net->source)
    {
        auto p = w.get_plane(target);
        if (p)
            p->alert_dirs.push_back(p->get_pos() - phys->pos);
        return;
    }

    if (is_saam)
    {
        if (owner.expired() || owner.lock()->hp <= 0)
            target.reset();
    }

    if (target.expired())
        return;

    const vec3 dir = phys->rot.rotate(vec3::forward());
    auto t = target.lock();
    auto diff = t->get_pos() - phys->pos;
    const vec3 target_dir = (diff + (t->get_vel() - phys->vel) * dt * 0.001f).normalize();
    if (dir.dot(target_dir) > homing_angle_cos || !t->is_targetable(true, true))
    {
        phys->target_dir = target_dir;
        auto p = w.get_plane(target);
        if (p)
            p->alert_dirs.push_back(diff);
    }
    else
    {
        target.reset();
        if (!is_saam && owner.lock() == w.get_player())
            w.popup_miss();
    }
}

//------------------------------------------------------------

void missile::update(int dt, world &w)
{
    render->mdl.set_pos(phys->pos);
    render->mdl.set_rot(phys->rot);

    render->engine_started = phys->accel_started;

    if (time > 0)
        time -= dt;

    if (net && !net->source)
        return;

    if (!target.expired())
    {
        auto t = target.lock();
        bool hit = line_sphere_intersect(phys->pos, phys->pos + phys->vel * (dt * 0.001f), t->get_pos(), t->get_hit_radius());

        if (!hit)
        {
            auto dir = t->get_pos() - phys->pos;
            hit = dir.length() < 5.0; //proximity detonation
        }

        if(hit)
        {
            time = 0;
            //if (vec3::normalize(target.lock()->phys->vel) * dir.normalize() < -0.5)  //direct shoot
            //    missile_damage *= 3;

            w.spawn_explosion(phys->pos, missile_dmg_radius / 2);

            const bool target_alive = t->hp > 0;
            const bool hit = w.area_damage(phys->pos, missile_dmg_radius, missile_damage, owner.lock());
            if (!hit && target_alive)
            {
                t->take_damage(missile_damage, w);
                auto p = w.get_plane(t);
                if (p && p->hp <= 0 )
                    w.on_kill(owner.lock(), p);
            }

            const bool destroyed = t->hp <= 0;
            if (owner.lock() == w.get_player() && target_alive)
                w.popup_hit(destroyed);
        }

        if (t->hp < 0)
            target.reset();
    }
}

//------------------------------------------------------------
}
