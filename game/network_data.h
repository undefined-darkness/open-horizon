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
    virtual net_plane_ptr add_plane(const char *name, int color) = 0;

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
                *m_safe_planes[i].get() = m_planes[i];
        }
        m_msg_mutex.unlock();
    }

    void update_post()
    {
        m_msg_mutex.lock();
        for (size_t i = 0; i < m_safe_planes.size(); ++i)
        {
            if (m_safe_planes[i]->source)
                m_planes[i] = *m_safe_planes[i].get();
        }
        m_msg_mutex.unlock();
    }

private:
    std::mutex m_msg_mutex;
    std::vector<msg_add_plane> m_add_plane_msgs;
    std::vector<net_plane> m_planes;

private:
    std::vector<msg_add_plane> m_safe_plane_msgs;
    std::vector<net_plane_ptr> m_safe_planes;
};

//------------------------------------------------------------
}
