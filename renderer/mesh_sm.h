//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"
#include <vector>
#include <stddef.h>

namespace renderer
{
//------------------------------------------------------------

struct mesh_sm
{
    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        unsigned char color[4];

        vert() { color[0] = color[1] = color[2] = color[3] = 255; }
    };

    struct tri_strip
    {
        std::vector<vert> verts;
    };

    struct group
    {
        int tex_idx = 0;
        std::vector<tri_strip> geometry;
    };

    std::vector<group> groups;

public:
    bool load(const void *data, size_t size);
};

//------------------------------------------------------------
}
