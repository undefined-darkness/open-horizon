//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/frustum.h"
#include <vector>

namespace phys
{
//------------------------------------------------------------

struct mesh
{
    nya_math::aabb bbox;

public:
    bool load(const void *data, size_t size);
};

//------------------------------------------------------------
};
