//
// open horizon -- undefined_darkness@outlook.com
//

#include "physics.h"

namespace phys
{
//------------------------------------------------------------

plane_ptr world::add_plane(const char *name)
{
    plane_ptr p;
    p.create();

    //ToDo

    return p;
}

//------------------------------------------------------------

void world::update(int dt, std::function<void(object_ptr &a, object_ptr &b)> on_hit)
{
    m_planes.erase(std::remove_if(m_planes.begin(), m_planes.end(), [](plane_ptr &p){ return p.get_ref_count() <= 1; }), m_planes.end());
    for (auto &p: m_planes) update_plane(p);
}

//------------------------------------------------------------

void world::update_plane(plane_ptr &p)
{
    //ToDo
}

//------------------------------------------------------------
}
