//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/frustum.h"
#include <vector>

namespace phys
{
//------------------------------------------------------------

class mesh
{
public:
    bool load(const void *data, size_t size);
    const nya_math::aabb &get_box() const { return m_box; }

private:
    nya_math::aabb m_box;
};

//------------------------------------------------------------
};
