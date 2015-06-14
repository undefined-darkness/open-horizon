//
// open horizon -- undefined_darkness@outlook.com
//

#include "particles.h"

#include "scene/camera.h"
#include "memory/memory_reader.h"
#include "shared.h"

namespace renderer
{
//------------------------------------------------------------

bool particles_bcf::load(const char *name)
{
    nya_memory::tmp_buffer_scoped res(load_resource(name));
    if (!res.get_data())
        return false;

    params::memory_reader reader(res.get_data(), res.get_size());

    struct bcf_header
    {
        char sign[4]; //"BCF\32"
        char unknown[4]; //"1000"
        uint32_t count;
        uint32_t unknown_zero;
        uint32_t unknown_zero2;

    } header;

    header = reader.read<bcf_header>();
    assert(memcmp(header.sign, "BCF ", 4) == 0);
    assume(header.unknown_zero == 0 && header.unknown_zero2 == 0);

    std::vector<uint32_t> offsets(header.count);
    for (auto &o: offsets)
        o = reader.read<uint32_t>();

    reader = params::memory_reader(reader.get_data(), reader.get_remained());

    for (auto &o: offsets)
    {
        reader.seek(o);

        struct unknown_header
        {
            uint32_t unknown; //hash id?
            uint32_t count;
            uint32_t sub_count;
            float unknown2; //usually 0.1, 0.01 or 0.05
            uint32_t zero_or_one;
            uint32_t zero[2];
        } uh;

        uh = reader.read<unknown_header>();
        assume(uh.zero_or_one == 0 || uh.zero_or_one == 1);
        assume(uh.zero[0] == 0 && uh.zero[1] == 0);

        std::vector<uint32_t> offsets(uh.count);
        for (auto &o: offsets)
            o = reader.read<uint32_t>();

        auto base_offset = reader.get_offset();

        for (auto &o: offsets)
        {
            reader.seek(base_offset + o);

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
            } ush;

            ush = reader.read<unknown_sub_header>();
            assume(ush.zero == 0 && ush.zero2 == 0);

            struct unknown_sub_data
            {
                float pos[3];
                uint8_t color[4];
            };

            for(int i = 0; i < uh.sub_count; ++i)
            {
                unknown_sub_data d = reader.read<unknown_sub_data>();
                d = d;
            }
        }
    }

    return true;
}

//------------------------------------------------------------

bool particles_bfx::load(const char *name)
{
    nya_memory::tmp_buffer_scoped res(load_resource(name));
    if (!res.get_data())
        return false;

    params::memory_reader reader(res.get_data(), res.get_size());

    struct bcf_header
    {
        char sign[4]; //"BFX\32"
        char unknown[4]; //"1300"
        uint32_t count;
        uint32_t count2;
        uint32_t count3;
        uint32_t unknown2;
        uint32_t unknown_zero;
        uint32_t unknown_zero2;

    } header;

    header = reader.read<bcf_header>();
    assert(memcmp(header.sign, "BFX ", 4) == 0);
    assume(header.unknown_zero == 0 && header.unknown_zero2 == 0);

    struct unknown_struct
    {
        uint32_t unknown;
        uint16_t unknwon2[2]; //offset and count?
        uint16_t unknwon3[2];
        uint32_t zero;
    };

    std::vector<unknown_struct> unknown_structs(header.count);
    assert(reader.get_remained() >= unknown_structs.size() * sizeof(unknown_struct));

    for (auto &u: unknown_structs)
    {
        u = reader.read<unknown_struct>();
        assume(u.zero == 0);
    }

    struct unknown_struct2
    {
        uint32_t unknown;
        uint32_t unknown2;
        uint32_t unknown3;
        float unknown4[5];
        uint16_t tc[4];
        uint16_t tc2[4];

        uint32_t unknown5[5]; //ToDo
        uint32_t unknown6[5]; //colors?

        uint32_t unknown7[13];

        uint32_t unknown8[33]; //ToDo

        uint32_t unknown9;
        float unknown10;
        float unknown11;
        uint32_t unknown12;
        float unknown13;
        uint32_t unknown14;

        uint32_t zero[14];
    };

    std::vector<unknown_struct2> unknown_structs2(header.count2);
    assert(reader.get_remained() >= unknown_structs2.size() * sizeof(unknown_struct2));

    for (auto &u: unknown_structs2)
    {
        u = reader.read<unknown_struct2>();
        for(auto &z: u.zero)
            assume(z == 0);
    }

    struct unknown_struct3
    {
        float unknown;
        uint16_t unknown2;
        uint8_t unknown3;
        uint8_t unknown4;
    };

    std::vector<unknown_struct3> unknown_structs3(header.count3);
    assert(reader.get_remained() >= unknown_structs3.size() * sizeof(unknown_struct3));

    for (auto &u: unknown_structs3)
    {
        u = reader.read<unknown_struct3>();
    }

    assert(reader.get_remained() == 0);

    return true;
}

//------------------------------------------------------------
}