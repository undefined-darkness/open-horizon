//
// open horizon -- undefined_darkness@outlook.com
//

// Unused

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

        unknown_structs.push_back({});
        auto &uh = unknown_structs.back().header;
        uh = reader.read<unknown_header>();
        assume(uh.zero_or_one == 0 || uh.zero_or_one == 1);
        assume(uh.zero[0] == 0 && uh.zero[1] == 0);

        std::vector<uint32_t> sub_offsets(uh.count);
        for (auto &o: sub_offsets)
            o = reader.read<uint32_t>();

        auto base_offset = reader.get_offset();

        for (auto &so: sub_offsets)
        {
            reader.seek(base_offset + so);

            unknown_structs.back().sub_structs.push_back({});
            auto &us = unknown_structs.back().sub_structs.back();
            auto &ush = us.header;
            ush = reader.read<unknown_sub_header>();
            assume(ush.zero == 0 && ush.zero2 == 0);

            for (int i = 0; i < uh.sub_count; ++i)
                us.sub_data.push_back(reader.read<unknown_sub_data>());
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

    unknown_structs.resize(header.count);
    assert(reader.get_remained() >= unknown_structs.size() * sizeof(unknown_struct));

    for (auto &u: unknown_structs)
    {
        u = reader.read<unknown_struct>();
        assume(u.zero == 0);
    }

    unknown_structs2.resize(header.count2);
    assert(reader.get_remained() >= unknown_structs2.size() * sizeof(unknown_struct2));

    for (auto &u: unknown_structs2)
    {
        u = reader.read<unknown_struct2>();
        for (auto &z: u.zero)
            assume(z == 0);
    }

    unknown_structs3.resize(header.count3);
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