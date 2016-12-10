//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "render/skeleton.h"
#include <vector>
#include <stddef.h>

namespace renderer
{
//------------------------------------------------------------

struct mesh_ndxr
{
    struct vert
    {
        float pos[3];
        float tc[2];
        float param_tc;
        uint16_t normal[4]; //half float
        uint16_t tangent[4];
        uint16_t bitangent[4];

        float bones[4];
        float weights[4];
    };

    std::vector<vert> verts;
    std::vector<unsigned short> indices2b;
    std::vector<unsigned int> indices4b;

    struct render_group
    {
        bool blend = false;
        bool illum = false;
        bool alpha_clip = false;

        int param_idx = 0;

        unsigned int offset = 0, count = 0;
        std::vector<unsigned int> textures;
    };

    struct group
    {
        std::string name;
        std::vector<render_group> rgroups;
    };

    std::vector<group> groups;

public:
    bool load(const void *data, size_t size, const nya_render::skeleton &skeleton, bool endianness);
    void reduce_groups();
};

//------------------------------------------------------------
}
