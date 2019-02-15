//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"
#include "difficulty.h"
#include "plane.h"
#include "units.h"

namespace game
{

//------------------------------------------------------------

class world
{
public:
    void set_location(const char *name);
    void stop_sounds();

    plane_ptr add_plane(const char *preset, const char *player_name, int color, bool player, net_plane_ptr ptr = net_plane_ptr());
    missile_ptr add_missile(const plane_ptr &p, net_missile_ptr ptr = net_missile_ptr());
    missile_ptr add_missile(const char *id, const renderer::model &m, net_missile_ptr ptr = net_missile_ptr());
    bomb_ptr add_bomb(const plane_ptr &p);
    bomb_ptr add_bomb(const char *id, const renderer::model &m);
    unit_ptr add_unit(const char *id);

    void spawn_explosion(const vec3 &pos, float radius, bool net_src = true);
    void spawn_bullet(const char *type, const vec3 &pos, const vec3 &dir, const plane_ptr &owner);

    bool direct_damage(const object_ptr &target, int damage, const plane_ptr &owner);
    bool area_damage(const vec3 &pos, float radius, int damage, const plane_ptr &owner);

    void respawn(const plane_ptr &p, const vec3 &pos, const quat &rot);

    int get_planes_count() const { return (int)m_planes.size(); }
    plane_ptr get_plane(int idx);
    plane_ptr get_plane(object_wptr t);
    plane_ptr get_player() { return m_player.lock(); }
    const char *get_player_name() const;

    int get_missiles_count() const { return (int)m_missiles.size(); }
    missile_ptr get_missile(int idx);

    int get_units_count() const { return (int)m_units.size(); }
    unit_ptr get_unit(int idx);

    //planes, units
    int get_objects_count() const;
    object_ptr get_object(int idx);

    float get_height(float x, float z, bool include_objects = true) const { return m_phys_world.get_height(x, z, include_objects); }

    bool is_ally(const plane_ptr &a, const plane_ptr &b);
    typedef std::function<bool(const plane_ptr &a, const plane_ptr &b)> is_ally_handler;
    void set_ally_handler(const is_ally_handler &handler) { m_ally_handler = handler; }

    void on_kill(const object_ptr &a, const object_ptr &b);
    typedef std::function<void(const object_ptr &k, const object_ptr &v)> on_kill_handler;
    void set_on_kill_handler(const on_kill_handler &handler) { m_on_kill_handler = handler; }

    gui::hud &get_hud() { return m_hud; }
    void popup_hit(bool destroyed);
    void popup_miss();
    void popup_mission_clear();
    void popup_mission_update();
    void popup_mission_fail();

    void update(int dt);

    void set_network(network_interface *n) { m_network = n; }
    network_interface *get_network() { return m_network; }
    bool is_host() const { return !m_network || m_network->is_server(); }
    bool net_data_updated() const { return m_net_data_updated; }

    unsigned int get_net_time() const { return m_network ? m_network->get_time() : 0; }

    sound::source_ptr add_sound(sound::file &f, bool loop = false) { return m_sound_world.add(f, loop); }
    sound::source_ptr add_sound(std::string name, int idx = 0, bool loop = false);
    unsigned int play_sound_ui(std::string name, bool loop = false);
    void stop_sound_ui(unsigned int id);
    void play_sound(std::string name, int idx, vec3 pos);

    world(renderer::world &w, sound::world &s, gui::hud &h): m_render_world(w), m_sound_world(s), m_hud(h), m_network(0) {}

private:
    void update_difficulty();

private:
    missile_ptr add_missile(const char *id, const renderer::model &m, bool add_to_phys_world);

private:
    plane_ptr get_plane(const phys::object_ptr &o);
    missile_ptr get_missile(const phys::object_ptr &o);
    bomb_ptr get_bomb(const phys::object_ptr &o);

private:
    difficulty_settings m_difficulty;
    std::vector<plane_ptr> m_planes;
    w_ptr<plane> m_player;
    std::vector<missile_ptr> m_missiles;
    std::vector<bomb_ptr> m_bombs;
    std::vector<unit_ptr> m_units;
    renderer::world &m_render_world;
    gui::hud &m_hud;
    phys::world m_phys_world;
    sound::world &m_sound_world;
    sound::pack m_sounds;
    sound::pack m_sounds_ui;
    sound::pack m_sounds_common;

    is_ally_handler m_ally_handler;
    on_kill_handler m_on_kill_handler;

private:
    network_interface *m_network;
    bool m_net_data_updated = false;
};

//------------------------------------------------------------
}

