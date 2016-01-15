//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include "location_params.h"

#include "memory/memory_reader.h"
#include "shared.h"

namespace renderer
{

//------------------------------------------------------------

struct particles_bcf
{
    struct unknown_header
    {
        uint32_t unknown; //hash id?
        uint32_t count;
        uint32_t sub_count;
        float unknown2; //usually 0.1, 0.01 or 0.05
        uint32_t zero_or_one;
        uint32_t zero[2];
    };

    struct unknown_sub_header
    {
        uint32_t unknown; //usually 2 or 3
        uint32_t unknown2;
        float unknown3; //usually 1.0
        uint32_t zero;
        float unknown4;
        uint32_t zero2;
        uint32_t unknown5;
        uint32_t unknown6;
        uint32_t unknown7;
    };

    struct unknown_sub_data
    {
        float pos[3];
        uint8_t color[4];
    };

    struct unknown_sub_struct
    {
        unknown_sub_header header;
        std::vector<unknown_sub_data> sub_data;
    };

    struct unknown_struct
    {
        unknown_header header;
        std::vector<unknown_sub_struct> sub_structs;
    };

    std::vector<unknown_struct> unknown_structs;

public:
    bool load(const char *name);
};

//------------------------------------------------------------

struct particles_bfx
{
    struct unknown_struct
    {
        uint32_t unknown;
        uint16_t unknown_struct3_idx_offset;
        uint16_t unknown_struct3_count;
        uint16_t unknwon3[2];
        uint32_t zero;
    };

    std::vector<unknown_struct> unknown_structs;
//56
    struct unknown_struct2
    {
        uint16_t unknown[2];
        uint16_t unknown2[2];
        uint32_t unknown3;
        float edge_param;
        float unknown4[4];
        uint16_t color_tc[4];
        uint16_t alpha_tc[4];

        uint16_t unknown5[4]; //ToDo
        uint16_t detail_tc[4];
        uint16_t unknown6[2]; //ToDo

        uint8_t colors[6][4]; //ToDO

        uint32_t unknown7[12];

        uint32_t unknown8[33]; //ToDo

        uint32_t unknown9;
        float unknown10;
        float unknown11;
        uint32_t unknown12;
        float unknown13;
        uint32_t unknown14;

        uint32_t zero[14];
    };

    std::vector<unknown_struct2> unknown_structs2;

    struct unknown_struct3
    {
        float unknown; //0 - 10
        uint16_t unknown_struct2_idx;
        uint8_t unknown3; //0 - 7
        uint8_t unknown4; //0
    };

    std::vector<unknown_struct3> unknown_structs3;

public:
    bool load(const char *name);
};

//------------------------------------------------------------

class particle_effects
{
public:
    bool init();
    void spawn(int idx);
};

//------------------------------------------------------------
}
