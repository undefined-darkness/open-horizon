//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "util/util.h"
#include "math/quaternion.h"

#include <vector>
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
};

typedef std::shared_ptr<net_plane> net_plane_ptr;

//------------------------------------------------------------

class network_interface
{
public:
    net_plane_ptr add_plane(const char *name, int color)
    {
        net_plane_ptr p(new net_plane());
        p->source = true;

        plane_request r;
        r.name.assign(name);
        r.color = color;

        m_msg_mutex.lock();
        m_self_add_planes.push_back(p);
        m_add_plane_requests.push_back(r);
        m_planes.resize(m_planes.size() + 1);
        m_planes.back().r = r;
        m_msg_mutex.unlock();

        return p;
    }

    virtual bool is_server() const { return false; };

public:
    struct msg_add_plane
    {
        std::string name;
        int color = 0;
        net_plane_ptr data;
    };

    const std::vector<msg_add_plane> &get_add_plane_msg() const { return m_safe_plane_msgs; }

    void update()
    {
        m_msg_mutex.lock();

        m_safe_plane_msgs = m_add_plane_msgs;
        for (size_t i = 0; i < m_safe_planes.size(); ++i)
        {
            if (!m_safe_planes[i]->source)
                *m_safe_planes[i].get() = m_planes[i].net;
        }

        m_msg_mutex.unlock();
    }

    void update_post()
    {
        m_msg_mutex.lock();

        if (!m_self_add_planes.empty())
        {
            for (auto &sp: m_self_add_planes)
                m_safe_planes.push_back(sp);
            m_self_add_planes.clear();
        }

        assert(m_safe_planes.size() == m_planes.size());

        for (size_t i = 0; i < m_safe_planes.size(); ++i)
        {
            if (m_safe_planes[i]->source)
                m_planes[i].net = *m_safe_planes[i].get();
        }

        m_msg_mutex.unlock();
    }

protected:
    std::mutex m_msg_mutex;

    struct plane_request
    {
        std::string name;
        int color = 0;
    };

    std::vector<msg_add_plane> m_add_plane_msgs;
    std::vector<plane_request> m_add_plane_requests;

    struct plane
    {
        plane_request r;
        net_plane net;
    };

    std::vector<plane> m_planes;

private:
    std::vector<net_plane_ptr> m_self_add_planes;

private:
    std::vector<msg_add_plane> m_safe_plane_msgs;
    std::vector<net_plane_ptr> m_safe_planes;
};

//------------------------------------------------------------
}
