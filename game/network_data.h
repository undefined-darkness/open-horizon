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
    unsigned int time = 0;

    nya_math::vec3 pos;
    nya_math::vec3 vel;
    nya_math::quat rot;

    bool source = false;

    int hp = 0;
};

typedef std::shared_ptr<net_plane> net_plane_ptr;

//------------------------------------------------------------

class network_interface
{
public:
    net_plane_ptr add_plane(const char *name, int color)
    {
        return add_plane(name, color, true, m_id, m_last_plane_id++);
    }

    virtual bool is_server() const { return false; };

public:
    struct msg_add_plane
    {
        unsigned short client_id = 0, plane_id = 0;
        std::string name;
        int color = 0;
    };

    net_plane_ptr add_plane(const msg_add_plane &m)
    {
        return add_plane(m.name.c_str(), m.color, false, m.client_id, m.plane_id);
    }

    bool get_add_plane_msg(msg_add_plane &m)
    {
        bool result = false;
        m_msg_mutex.lock();
        if (!m_add_plane_msgs.empty())
        {
            m = m_add_plane_msgs.front();
            m_add_plane_msgs.pop_front();
            result = true;
        }
        m_msg_mutex.unlock();
        return result;
    }

    void update()
    {
        m_msg_mutex.lock();

        for (size_t i = 0; i < m_safe_planes.size(); ++i)
        {
            if (!m_safe_planes[i]->source)
                *m_safe_planes[i].get() = m_planes[i].net;
        }

        m_msg_mutex.unlock();
    }

    void update_post(unsigned int dt)
    {
        m_msg_mutex.lock();

        m_time += dt;
        assert(m_safe_planes.size() == m_planes.size());

        for (size_t i = 0; i < m_safe_planes.size(); ++i)
        {
            if (m_safe_planes[i]->source)
            {
                m_planes[i].net = *m_safe_planes[i].get();
                m_planes[i].net.time = m_time;
            }
        }

        m_msg_mutex.unlock();
    }

private:
    net_plane_ptr add_plane(const char *name, int color, bool source, unsigned short client_id, unsigned short plane_id)
    {
        net_plane_ptr p(new net_plane());
        p->source = source;

        if (!source)
            assert(client_id != m_id);

        m_msg_mutex.lock();
        m_planes.resize(m_planes.size() + 1);
        m_safe_planes.push_back(p);
        auto &r = m_planes.back().r;
        r.name.assign(name);
        r.color = color;
        r.client_id = client_id;
        r.plane_id = plane_id;
        if (source)
            m_add_plane_requests.push_back(r);
        m_msg_mutex.unlock();

        return p;
    }

protected:
    std::mutex m_msg_mutex;

    std::deque<msg_add_plane> m_add_plane_msgs;
    std::deque<msg_add_plane> m_add_plane_requests;

    struct plane
    {
        msg_add_plane r;
        net_plane net;
        unsigned int last_time = 0;
    };

    std::vector<plane> m_planes;
    unsigned short m_id = 0;
    unsigned int m_time = 0;

private:
    std::vector<net_plane_ptr> m_safe_planes;
    unsigned short m_last_plane_id = 0;
};

//------------------------------------------------------------
}
