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
#include <mutex>

namespace game
{
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

class network_interface
{
public:
    net_plane_ptr add_plane(const char *name, int color)
    {
        return add_plane(name, color, true, m_id, new_plane_id());
    }

    unsigned int new_plane_id() { return m_last_plane_id++; }

    virtual bool is_server() const { return false; };

public:
    struct msg_add_plane
    {
        unsigned int client_id = 0, plane_id = 0;
        std::string name;
        int color = 0;
    };

    net_plane_ptr add_plane(const msg_add_plane &m)
    {
        return add_plane(m.name.c_str(), m.color, false, m.client_id, m.plane_id);
    }

    bool get_add_plane_msg(msg_add_plane &m)
    {
        if (!m_add_plane_msgs.empty())
        {
            m = m_add_plane_msgs.front();
            m_add_plane_msgs.pop_front();
            return true;
        }

        return false;
    }

public:
    struct msg_explosion
    {
        nya_math::vec3 pos;
        float radius = 0.0f;
    };

    void spawn_explosion(const nya_math::vec3 &pos, float radius)
    {
        msg_explosion me;
        me.pos = pos, me.radius = radius;
        m_explosion_requests.push_back(me);
    }

    bool get_explosion_msg(msg_explosion &m)
    {
        if (!m_explosions.empty())
        {
            m = m_explosions.front();
            m_explosions.pop_front();
            return true;
        }

        return false;
    }

public:
    virtual void update() {};
    virtual void update_post(int dt) {};

    unsigned int get_time() const { return m_time; }

private:
    net_plane_ptr add_plane(const char *name, int color, bool source, unsigned int client_id, unsigned int plane_id)
    {
        if (!source)
            assert(client_id != m_id);

        m_planes.resize(m_planes.size() + 1);
        auto &n = m_planes.back().net;
        n = net_plane_ptr(new net_plane());
        n->source = source;

        auto &r = m_planes.back().r;
        r.name.assign(name);
        r.color = color;
        r.client_id = client_id;
        r.plane_id = plane_id;
        if (source)
            m_add_plane_requests.push_back(r);

        printf("netdata add_plane %s %s\n", name, source ? "source" : "ref");

        return n;
    }

protected:
    std::deque<msg_add_plane> m_add_plane_msgs;
    std::deque<msg_add_plane> m_add_plane_requests;

protected:
    std::deque<msg_explosion> m_explosions;
    std::deque<msg_explosion> m_explosion_requests;

protected:
    struct plane
    {
        msg_add_plane r;
        net_plane_ptr net;
        unsigned int last_time = 0;
    };

    std::vector<plane> m_planes;
    unsigned int m_id = 0;
    unsigned int m_time = 0;

private:
    unsigned short m_last_plane_id = 0;
};

//------------------------------------------------------------
}
