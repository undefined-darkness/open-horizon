//
// open horizon -- undefined_darkness@outlook.com
//

#include "ai.h"

namespace game
{
//------------------------------------------------------------

void ai::update(const world &w, int dt)
{
    if (m_plane.expired())
        return;

    auto p = m_plane.lock();

    //ToDo

    //p->controls.throttle = 1.0;

    if (!p->targets.empty() && p->targets.front().locked)
        p->controls.missile = !p->controls.missile;
    else
        p->controls.missile = false;
}

//------------------------------------------------------------
}
