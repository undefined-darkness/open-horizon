//
// open horizon -- undefined_darkness@outlook.com
//

#include "plane.h"
#include "world.h"

namespace game
{
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

    for (int i = 0; i < w.get_objects_count(); ++i)
    {
        const auto o = w.get_object(i);
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

    auto *wp = &w;
    targets.erase(std::remove_if(targets.begin(), targets.end(),
                                 [me, wp](const target_lock &t)
                                 {
                                     if (t.target.expired())
                                         return true;

                                     auto tt = t.target.lock();
                                     return tt->hp <= 0 || tt->is_ally(me, *wp);
                                 }), targets.end());
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
    render->set_special_bay(special_weapon_selected && special_internal);
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
    if (!special_internal)
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

inline float get_fall_time(float h, float v, float g)
{
    // y"*t^2/2 + y'*t + y = 0

    const float d = v * v + 2.0f * g * h;
    return (v + sqrt(d)) / g;
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

        if (special.id == "LAGM")
        {
            if (!targets.empty())
                targets.front().locked = targets.front().can_lock(special);
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

        if (special.id == "UGB" || special.id == "GPB")
        {
            if (special.id == "GPB" && !targets.empty())
                targets.front().locked = targets.front().can_lock(special);

            if (need_fire)
            {
                for (auto &s: special_cooldown) if (s <= 0) { s = special.reload_time; break; }

                auto b = w.add_bomb(shared_from_this());
                b->owner = shared_from_this();

                special_mount_cooldown.resize(render->get_special_mount_count());

                special_mount_idx = ++special_mount_idx % (int)special_mount_cooldown.size();
                special_mount_cooldown[special_mount_idx] = special.reload_time;
                render->set_special_visible(special_mount_idx, false);
                b->phys->pos = render->get_special_mount_pos(special_mount_idx) + pos_fix;
                b->phys->rot = render->get_special_mount_rot(special_mount_idx);
                b->phys->vel = phys->vel;

                if (special.id == "GPB" && !targets.empty() && targets.front().locked > 0 && !targets.front().target.expired())
                {
                    const vec3 p = get_pos();
                    const float height = p.y - w.get_height(p.x, p.z);
                    const float t = get_fall_time(height, 0.0f, special.gravity);

                    auto tl = targets.front().target.lock();
                    const vec3 tdiff = tl->get_pos() + tl->get_vel() * t - p;
                    const float hspeed = tdiff.length() / t;

                    b->phys->vel = vec3(tdiff.x, 0.0f, tdiff.z).normalize() * hspeed;
                }
                else
                    b->phys->vel += get_rot().rotate(vec3::forward() * special.speed_init);

                play_relative(w, "UGB", 0, get_rot().rotate_inv(b->phys->pos - get_pos()));

                bomb_mark m;
                m.b = b, m.need_update = true;
                bomb_marks.push_back(m);

                --special_count;
            }

            bomb_marks.erase(std::remove_if(bomb_marks.begin(), bomb_marks.end(), [](const bomb_mark &m) { return m.b.expired(); }), bomb_marks.end());
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
                    if (special.id == "QAAM")  play_relative(w, "SHOT_MSL", random(0, 2), get_rot().rotate_inv(m->phys->pos - get_pos()));
                    else if (special.id == "_4AGM") play_relative(w, "4AGM", 0, vec3());
                    else if (special.id == "LAGM")  play_relative(w, "LAGM", 0, get_rot().rotate_inv(m->phys->pos - get_pos()));
                    else if (shot_cout > 1)         play_relative(w, "SAAM", 0, vec3());
                    else                            play_relative(w, "SAAM", 0, get_rot().rotate_inv(m->phys->pos - get_pos()));
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
    {
        h.set_mgp(special_weapon_selected && controls.missile);
    }
    else if (special_weapon_selected && (special.id == "UGB" || special.id == "GPB"))
    {
        vec3 tpos;

        if (special.id == "GPB" && !targets.empty() && targets.front().locked > 0)
        {
            auto &t = targets.front().target;
            if (!t.expired())
            {
                tpos = t.lock()->get_pos();
                h.set_bomb_target(tpos, bomb_dmg_radius, gui::hud::red);
            }
        }
        else
        {
            const vec3 p = get_pos();
            const vec3 v = phys->vel + dir * special.speed_init;
            const float ground_height = w.get_height(p.x, p.z);
            const float height = p.y - ground_height;
            const float t = get_fall_time(height, v.y, special.gravity);

            tpos = p + v * t;
            tpos.y = ground_height;

            h.set_bomb_target(tpos, bomb_dmg_radius, gui::hud::green);
        }

        if (!bomb_marks.empty() && bomb_marks.back().need_update)
            bomb_marks.back().p = tpos, bomb_marks.back().need_update = false;
    }

    h.clear_bomb_marks();
    for (auto &m: bomb_marks)
        h.add_bomb_mark(m.p, bomb_dmg_radius, gui::hud::green);

    if (special.lockon_count == 1 || special.id == "GPB")
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
}

