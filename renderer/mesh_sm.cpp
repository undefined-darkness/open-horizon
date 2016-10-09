//
// open horizon -- undefined_darkness@outlook.com
//

#include "mesh_sm.h"
#include "util/util.h"

namespace renderer
{
//------------------------------------------------------------

bool mesh_sm::load(const void *data, size_t size)
{
    nya_memory::memory_reader r(data, size);
    //print_data(r);

    struct sm_header
    {
        uint32_t sign;
        uint32_t unknown_5;
        float unknown2;
        float unknown3;
        uint32_t node_count;
        uint32_t node_offset;
        uint32_t groups_count;
        uint32_t groups_offset;
        uint32_t total_tex_count;
        uint32_t zero[7];
        uint32_t unknown_count;
        uint32_t unknown_offset;
        uint32_t unknown_1;
        uint32_t size;

    } const header = r.read<sm_header>();

    if (header.sign != '\0MCA')
        return false;

    for (auto &z: header.zero) assume(z == 0);
    assume(header.size + 32 == size);
    assume(header.unknown_5 == 5);
    assume(header.unknown_1 == 1);
    assume(header.node_count == header.groups_count + 1);

    if (!r.seek(header.groups_offset))
        return false;

    std::vector<uint32_t> offsets(header.groups_count);

    for (auto &o: offsets)
        o = r.read<uint32_t>();

    for (auto &o: offsets)
        o = roundup(o + (uint32_t)r.get_offset(), 16);

    groups.resize(header.groups_count);

    for (int i = 0; i < header.groups_count; ++i)
    {
        r.seek(offsets[i]);

        auto &g = groups[i];

        struct group_header
        {
            uint8_t unknown; //about face count x 10
            uint8_t unknown_zero;
            uint8_t unknown_zero2;
            uint8_t unknown_96;
            uint32_t unknown_zero3;

        } const gh = r.read<group_header>();

        assume(gh.unknown_zero == 0 && gh.unknown_zero2 == 0);
        assume(gh.unknown_zero3 == 0);
        assume(gh.unknown_96 == 96);

        while (r.get_remained() > 0)
        {
            tri_strip f;

            struct strip_header
            {
                uint8_t elem_count;
                uint8_t unknown_1;
                uint8_t unknown_0;
                uint8_t unknown2_1;

            } fh = r.read<strip_header>();

            if (!(fh.unknown_1 == 1 && fh.unknown_0 == 0 && fh.unknown2_1 == 1))
            {
                r.rewind(sizeof(strip_header));
                break;
            }

            //assume(fh.unknown_1 == 1);
            //assume(fh.unknown_0 == 0);
            //assume(fh.unknown2_1 == 1);

            for (int j = 0; j < fh.elem_count; ++j)
            {
                struct elem_header
                {
                    uint8_t idx;
                    uint8_t unknown_128_or_192;
                    uint8_t count;
                    uint8_t type;

                } eh = r.read<elem_header>();

                assume(eh.idx == j);
                assume(eh.unknown_128_or_192 == 128 || eh.unknown_128_or_192 == 192);

                if (eh.idx == 0)
                {
                    assume(eh.count == 1);
                    const uint32_t vcount = r.read<uint32_t>();
                    const uint32_t unknown_1 = r.read<uint32_t>();
                    assume(unknown_1 == 1);
                    assume(eh.count == 1);

                    f.verts.resize(vcount);
                    continue;
                }

                assume(eh.count == f.verts.size());

                switch (eh.type)
                {
                    case 100:
                        for (auto &v: f.verts)
                            v.tc = r.read<nya_math::vec2>();
                        break;

                    case 104:
                        for (auto &v: f.verts)
                            v.pos = r.read<nya_math::vec3>();
                        break;

                    case 105:
                        {
                            short *normals = (short *)r.get_data();
                            for (auto &v: f.verts)
                            {
                                v.normal = nya_math::vec3(normals[0], normals[1], normals[2]) / 4096.0f;
                                normals += 3;
                            }

                            r.skip(roundup(6 * eh.count, 4));
                        }
                        break;

                    case 110:
                        for (auto &v: f.verts)
                            *(uint32_t *)&v.color = r.read<uint32_t>();
                        break;
                }
            }

            struct face_footer
            {
                uint8_t zero[3];
                uint8_t unknown_17;
                uint8_t zero2[3];
                uint8_t unknown2_23;
                uint8_t unknown3_4;
                uint8_t unknown4_4;
                uint8_t unknown_0;
                uint8_t unknown_1;

            } ff = r.read<face_footer>();

            assume(ff.unknown_17 == 17);
            assume(ff.unknown2_23 == 23);
            for (auto &z: ff.zero) assume(z == 0);
            for (auto &z: ff.zero2) assume(z == 0);
            assume(ff.unknown3_4 == 4);
            assume(ff.unknown4_4 == 4);
            assume(ff.unknown_0 == 0);
            assume(ff.unknown_1 == 1);

            g.geometry.push_back(f);
        }
    }

    if (!r.seek(header.node_offset))
        return false;

    struct node
    {
        uint8_t unknown;
        uint8_t unknown2;
        uint16_t zero;
        uint32_t unknown3;
        int32_t group_idx;
        uint32_t tex_idx;
        uint32_t zero2;
        int32_t unknown_minus_one[3];
        float unknown_float[8];
    };

    std::vector<node> nodes(header.node_count);
    for (auto &n: nodes)
    {
        n = r.read<node>();

        assume(n.zero == 0);
        assume(n.zero2 == 0);

        if (n.group_idx >= 0)
        {
            assume(n.group_idx < groups.size());
            groups[n.group_idx].tex_idx = n.tex_idx;
        }

        //printf("\tnode %d %d %d %d %d\n", n.unknown, n.unknown2, n.unknown3, n.group_idx, n.tex_idx);
    }

    return true;
}

//------------------------------------------------------------
}
