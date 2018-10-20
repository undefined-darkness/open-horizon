//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"
#include "objects.h"

namespace game
{
//------------------------------------------------------------

struct unit;
typedef w_ptr<unit> unit_wptr;
typedef ptr<unit> unit_ptr;

struct unit: public object
{
    virtual void set_active(bool a) { m_active = a; if (m_render.is_valid()) m_render->visible = a; }

    typedef std::list<vec3> path;
    virtual void set_path(const path &p, bool loop) {}
    virtual void set_follow(object_wptr target) {}
    virtual void set_target(object_wptr target) {}

    enum target_search_mode { search_all, search_player, search_none };
    virtual void set_target_search(target_search_mode mode) {}

    enum align { align_target, align_enemy, align_ally, align_neutral };
    virtual void set_align(align a) { m_align = a; }
    virtual void set_type_name(std::wstring name) { m_type_name = name; }

    virtual void set_pos(const vec3 &p) { if (m_render.is_valid()) m_render->mdl.set_pos(p + m_dpos); }
    virtual void set_yaw(angle_deg yaw) { if (m_render.is_valid()) m_render->mdl.set_rot(quat(0.0f, yaw, 0.0f)); }

    virtual void set_speed(float speed) {}
    virtual void set_speed_limit(float speed) {}

    virtual bool is_active() const { return m_active; }
    virtual align get_align() const { return m_align; }

    virtual bool is_ally(const unit_ptr &u) const;

    //object
public:
    virtual vec3 get_pos() override { return m_render.is_valid() ? m_render->mdl.get_pos() - m_dpos : vec3(); }
    virtual quat get_rot() override { return m_render.is_valid() ? m_render->mdl.get_rot() : quat(); }
    virtual std::wstring get_type_name() override { return m_type_name; }
    virtual bool get_tgt() override { return m_align == align_target; }
    virtual bool is_ally(const plane_ptr &p, world &w) override { return m_align > align_enemy; }
    virtual void take_damage(int damage, world &w, bool net_src = true) override;

public:
    virtual void load_model(std::string model, float dy, renderer::world &w)
    {
        m_render = w.add_object(model.c_str());
        m_dpos.y = dy;

        if (m_render.is_valid())
            m_render->visible = m_active;
    }

    virtual void update(int dt, world &w) {}

protected:
    bool m_active = true;
    renderer::object_ptr m_render;
    vec3 m_dpos;
    align m_align;
    std::wstring m_type_name;
};

//------------------------------------------------------------

struct unit_vehicle: public unit
{
public:
    virtual void set_path(const path &p, bool loop) override { m_path = p; m_path_loop = loop; }
    virtual void set_follow(object_wptr f) override { m_follow = f; }
    virtual void set_target(object_wptr t) override { m_target = t; }
    virtual void set_target_search(target_search_mode mode) override { m_target_search = mode; }
    virtual void set_speed(float speed) override;
    virtual void set_speed_limit(float speed) override;
    virtual void update(int dt, world &w) override;
    unit_vehicle(const object_params &p, const location_params &lp);

    //object
public:
    vec3 get_vel() override { return m_vel; }
    virtual bool is_targetable(bool air, bool ground) override { return hp > 0 && m_active && (ground == m_ground || air != m_ground); }
    virtual float get_hit_radius() override { return m_params.hit_radius; }

protected:
    unit::path m_path;
    bool m_path_loop = false;
    bool m_ground = true;
    object_wptr m_target;
    target_search_mode m_target_search = search_all;
    object_wptr m_follow;
    bool m_first_update = true;
    vec3 m_vel;
    vec3 m_formation_offset;
    object_params m_params;
    float m_speed_limit = 9000.0f;

    enum ai_type { ai_default, ai_air_to_air, ai_air_to_ground, ai_air_multirole };
    ai_type m_ai = ai_default;

    struct wpn
    {
        wpn_params params;
        renderer::model mdl;
        int cooldown = 0;
    };

    std::vector<wpn> m_weapons;
};

//------------------------------------------------------------

struct unit_object: public unit
{
    virtual bool is_targetable(bool air, bool ground) override { return hp > 0 && ground; }
};

//------------------------------------------------------------
}

