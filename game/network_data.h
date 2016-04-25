//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "util/util.h"
#include "math/quaternion.h"

#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <sstream>

namespace game
{
//------------------------------------------------------------

const static unsigned int invalid_id = (unsigned int)(-1);

//------------------------------------------------------------

struct server_info
{
    int version = 0;
    std::string server_name;
    std::string game_mode;
    std::string location;
    int players = 0;
    int max_players = 0;
};

//------------------------------------------------------------

struct game_data
{
    template<typename t> t get(std::string name) const
    {
        for (auto &p: params)
        {
            if (p.first == name)
            {
                std::istringstream ss(p.second);
                t value;
                ss >> value;
                return value;
            }
        }

        return t();
    }

    void set(std::string name, std::string value)
    {
        for (auto &p: params)
        {
            if (p.first == name)
            {
                p.second = value;
                changed = true;
                return;
            }
        }

        params.push_back({name, value});
        changed = true;
    }

    template<typename t> void set(std::string name, t value) { set(name, std::to_string(value)); }

    std::vector<std::pair<std::string, std::string> > params;
    bool changed = false;
};

//------------------------------------------------------------

struct net_plane
{
    nya_math::vec3 pos;
    nya_math::vec3 vel;
    nya_math::quat rot;

    nya_math::vec3 ctrl_rot;
    float ctrl_throttle = 0.0f;
    float ctrl_brake = 0.0f;
    bool ctrl_mgun = false;
    bool ctrl_mgp = false;

    bool source = false;
};

typedef std::shared_ptr<net_plane> net_plane_ptr;

//------------------------------------------------------------

struct net_missile
{
    nya_math::vec3 pos;
    nya_math::vec3 vel;
    nya_math::quat rot;
    nya_math::vec3 target_dir;
    unsigned int target = invalid_id;
    bool engine_started = false;

    bool source = false;
};

typedef std::shared_ptr<net_missile> net_missile_ptr;

//------------------------------------------------------------

class network_interface
{
public:
    net_plane_ptr add_plane(std::string preset, std::string player_name, int color)
    {
        msg_add_plane r;
        r.client_id = m_id;
        r.id = new_obj_id();
        r.preset = preset;
        r.player_name = player_name;
        r.color = color;
        return m_planes.add(r, true);
    }

    struct msg_add_plane
    {
        unsigned int client_id = 0, id = 0;
        std::string preset, player_name;
        int color = 0;
    };

    net_plane_ptr add_plane(const msg_add_plane &m) { return m_planes.add(m, false); }
    bool get_add_plane_msg(msg_add_plane &m) { return get_msg(m, m_planes.add_msgs); }

    net_plane_ptr get_plane(unsigned int plane_id) { return m_planes.get_net(plane_id); }

    unsigned int get_plane_id(net_plane_ptr plane)
    {
        for (auto &p: m_planes.objects)
        {
            if (p.net == plane)
                return p.r.id;
        }

        return invalid_id;
    }

    unsigned int get_id() const { return m_id; }

public:
    net_missile_ptr add_missile(net_plane_ptr plane, bool special)
    {
        msg_add_missile m;
        m.client_id = m_id;
        m.plane_id = get_plane_id(plane);
        m.id = new_obj_id();
        m.special = special;
        return m_missiles.add(m, true);
    }

    struct msg_add_missile
    {
        unsigned int client_id = 0, plane_id = 0, id = 0;
        bool special = false;
    };

    net_missile_ptr add_missile(const msg_add_missile &m) { return m_missiles.add(m, false); }
    bool get_add_missile_msg(msg_add_missile &m) { return get_msg(m, m_missiles.add_msgs); }

public:
    void general_msg(std::string msg) { m_general_msg_requests.push_back(msg); }
    bool get_general_msg(std::string &m) { return get_msg(m, m_general_msg); }

public:
    struct msg_game_data
    {
        unsigned int client_id, plane_id;
        game_data data;
    };

    void send_game_data(net_plane_ptr plane, const game_data &data)
    {
        msg_game_data m;
        m.client_id = m_id;
        m.plane_id = get_plane_id(plane);
        m.data = data;

        m_game_data_msg_requests.push_back(m);
    }

    bool get_game_data_msg(msg_game_data &m) { return get_msg(m, m_game_data_msg); }

public:
    virtual bool is_server() const { return false; };

    virtual void update() {};
    virtual void update_post(int dt) {};

    unsigned int get_time() const { return m_time; }

private:
    template<typename t> bool get_msg(t &m, std::deque<t> &messages)
    {
        if (!messages.empty())
        {
            m = messages.front();
            messages.pop_front();
            return true;
        }

        return false;
    }

protected:
    template<typename request, typename type> struct net_objects
    {
        typedef std::shared_ptr<type> ptr;

        ptr add(const request &r, bool source)
        {
            objects.resize(objects.size() + 1);
            auto &n = objects.back().net;
            n = ptr(new type());
            n->source = source;
            objects.back().r = r;
            if (source)
                add_requests.push_back(r);
            return n;
        }

        ptr get_net(unsigned int id)
        {
            for (auto &o: objects)
            {
                if (o.r.id == id)
                    return o.net;
            }

            return ptr();
        }

        void remove(unsigned int id)
        {
            objects.erase(std::remove_if(objects.begin(), objects.end(), [id](const obj &o){ return o.r.id == id; }), objects.end());
        }

        void remove_by_client_id(unsigned int id)
        {
            objects.erase(std::remove_if(objects.begin(), objects.end(), [id](const obj &o){ return o.r.client_id == id; }), objects.end());
        }

        void remove_src_unique()
        {
            objects.erase(std::remove_if(objects.begin(), objects.end(), [](const obj &o){ return o.net.unique() && o.net->source; }), objects.end());
        }

        void clear() { objects.clear(), add_msgs.clear(), add_requests.clear(); }

    public:
        struct obj
        {
            request r;
            ptr net;
            unsigned int last_time = 0;
        };

        std::vector<obj> objects;

    public:
        std::deque<request> add_msgs;
        std::deque<request> add_requests;
    };

protected:
    unsigned int new_obj_id() { return m_last_obj_id++; }

protected:
    net_objects<msg_add_plane,net_plane> m_planes;
    net_objects<msg_add_missile,net_missile>  m_missiles;

protected:
    std::deque<std::string> m_general_msg;
    std::deque<std::string> m_general_msg_requests;

protected:
    std::deque<msg_game_data> m_game_data_msg;
    std::deque<msg_game_data> m_game_data_msg_requests;

protected:
    unsigned int m_id = 0;
    unsigned int m_time = 0;
    unsigned int m_last_obj_id = 0;
};

//------------------------------------------------------------
}
