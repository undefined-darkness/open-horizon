//
// open horizon -- undefined_darkness@outlook.com
//

#include "mesh.h"
#include "util/util.h"
#include "memory/memory_reader.h"

namespace phys
{
//------------------------------------------------------------

bool mesh::load(const void *data, size_t size)
{
    nya_memory::memory_reader reader(data, size);

    typedef unsigned int uint;
    typedef unsigned short ushort;

    struct colh_header
    {
        char sign[4];
        uint chunk_size;
        uint offset_to_info;
        uint unknown_zero;
        ushort count;
        ushort unknown2;
        uint offset_to_offsets;
        uint offset_to_unknown;
        ushort unknown3;
        ushort unknown4;
    };

    colh_header header = reader.read<colh_header>();

    assume(header.chunk_size == size);
    assume(header.unknown_zero == 0);

    struct colh_info
    {
        uint unknown_32;
        uint unknown_zero;
        uint offset_to_unknown;
        uint offset_to_unknown2;
        ushort unknown;
        short unknown2;
        ushort unknown_zero2;
        ushort shapes_count;
        ushort vcount;
        ushort unknown3;
        ushort unknown4;
        ushort unknown5;
    };

    struct colh_chunk
    {
        uint offset;
        uint size;

        colh_info header;
    };

    std::vector<colh_chunk> chunks;
    chunks.resize(header.count);
    reader.seek(header.offset_to_offsets);
    for (int i = 0; i < header.count; ++i)
    {
        chunks[i].offset = reader.read<uint>();
        chunks[i].size = reader.read<uint>();
    }

    assume(header.count == 1);

    for (int i = 0; i < header.count; ++i)
    {
        colh_chunk &c = chunks[i];
        reader.seek(c.offset);

        c.header = reader.read<colh_info>();

        assume(c.header.unknown_32 == 32);
        assume(c.header.unknown_zero == 0);
        //for (int j = 0; j < 1; ++j)
        {
            nya_math::vec3 p;
            p.x = reader.read<float>();
            p.y = reader.read<float>();
            p.z = reader.read<float>();
            float f = reader.read<float>();
            assume(f == 1.0f);
            nya_math::vec3 p2;
            p2.x = reader.read<float>();
            p2.y = reader.read<float>();
            p2.z = reader.read<float>();
            //float f = reader.read<float>();
            float f2 = reader.read<float>();
            assume(f2 == 0);
            //test.add_point(p, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            //test.add_line(p, p2, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            bbox.origin = p;
            //bbox.delta = nya_math::vec3(f, f, f);
            bbox.delta = p2;

            reader.skip(24*sizeof(float)); //crude shape
        }

        uint zero[4];
        for (auto &z: zero)
        {
            z = reader.read<uint>();
            assume(z == 0);
        }

        std::vector<uint> shape_offsets(c.header.shapes_count);
        for (auto &s: shape_offsets)
            s = reader.read<uint>();

        std::vector<uint> shape_sizes(c.header.shapes_count);
        for (auto &s: shape_sizes)
            s = reader.read<ushort>() * 16;

        auto last = c.offset + c.size - reader.get_offset();
        auto last_buf = (const char *)reader.get_data();
        for (int i = 0; i < last; ++i)
            assume(last_buf[i] == 0);

        m_shapes.resize(c.header.shapes_count);
        for(int i = 0; i < c.header.shapes_count; ++i)
        {
            auto &s = m_shapes[i];
            reader.seek(shape_offsets[i]);

            struct shape_header
            {
                ushort size_div_16;
                ushort count;
                uint count4;
                uint zero[2];
            };

            auto header = reader.read<shape_header>();
            assume(header.size_div_16 * 16 == shape_sizes[i]);
            assume(header.zero[0] == 0 && header.zero[1] == 0);
            assume(header.count4 == header.count * 4);

            s.pls.resize(header.count);
            for (auto &p: s.pls)
                p = reader.read<pl>();
        }
    }

    assume(reader.get_remained() == 0);

    //print_data(reader, reader.get_offset(), reader.get_remained(), 4);

    return true;
}

//------------------------------------------------------------

bool mesh::trace(const nya_math::vec3 &from, const nya_math::vec3 &to) const
{
    const vec3_float4 from4 = vec3_float4(from);
    const vec3_float4 dpu = vec3_float4(from - to);

    for (const auto &s: m_shapes)
    {
        for (const auto &pl: s.pls)
        {
            const vec3_float4 dp = from4 - pl.p;
            const float4 u = dpu.dot(pl.v), p = dp.dot(pl.v);
            const vec3_float4 c = dpu.cross(dp);
            const float4 l = -c.dot(pl.lv), r = c.dot(pl.rv);
            const static float4 zero;
            const float4 chk = u < zero | p < zero | r < zero | l < zero | u < p | u < r | u < l + r;
            if (chk.is_zero_or_nan())
                return true;
        }
    }

    return false;
}

//------------------------------------------------------------

bool mesh::trace(const nya_math::vec3 &from, const nya_math::vec3 &to, float &result) const
{
    align16 float lf[4];
    result = 1.0f;
    bool hit = false;

    const vec3_float4 from4 = vec3_float4(from);
    const vec3_float4 dpu = vec3_float4(from - to);

    for (const auto &s: m_shapes)
    {
        for (const auto &pl: s.pls)
        {
            const vec3_float4 dp = from4 - pl.p;
            const float4 u = dpu.dot(pl.v), p = dp.dot(pl.v);
            const vec3_float4 c = dpu.cross(dp);
            const float4 l = -c.dot(pl.lv), r = c.dot(pl.rv);
            const static float4 zero;
            const float4 chk = u <= zero | p < zero | r < zero | l < zero | u < p | u < r | u < l + r;
            if (chk.is_zero_or_nan())
            {
                hit = true;
                float4 l = p / u + chk;
                l.get(lf);
                for (auto l: lf) if (l > 0.0f && l < result) result = l;
            }
        }
    }

    return hit;
}

//------------------------------------------------------------
};
