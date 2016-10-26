//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"
#include "resources/resources.h"
#include <vector>
#include <string>
#include <stddef.h>

namespace renderer
{
//------------------------------------------------------------

struct mesh_obj
{
    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        nya_math::vec4 color;
    };

    std::vector<vert> verts;

    struct group
    {
        int offset = 0;
        int count = 0;
        int mat_idx = -1;
    };

    std::vector<group> groups;

    struct mat
    {
        std::string name;
        std::string tex_diffuse;
    };

    std::vector<mat> materials;

public:
    bool load(const char *name, nya_resources::resources_provider &prov);
};

//------------------------------------------------------------
}
