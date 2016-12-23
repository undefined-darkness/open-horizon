//
// open horizon -- undefined_darkness@outlook.com
//

#include "mesh_ndxr.h"
#include "util/util.h"
#include "util/half.h"

//------------------------------------------------------------

template<typename t> void change_endianness(t &o)
{
    auto *u = (uint32_t *)&o;
    for (int i = 0; i < sizeof(t)/4; ++i)
        u[i] = swap_bytes(u[i]);
}

//------------------------------------------------------------

inline void float3_to_half3(const float *from, unsigned short *to)
{
    for (int i = 0; i < 3; ++i)
        to[i] = Float16Compressor::compress(from[i]);
}

//------------------------------------------------------------

inline nya_math::vec3 half3_as_vec3(const unsigned short *from)
{
    nya_math::vec3 p;
    p.x = Float16Compressor::decompress(from[0]);
    p.y = Float16Compressor::decompress(from[1]);
    p.z = Float16Compressor::decompress(from[2]);
    return p;
}

//------------------------------------------------------------

inline bool is_normal(const unsigned short *from)
{
    const auto n = half3_as_vec3(from);
    const float l = n.length_sq();
    return l == 0 || fabsf(l - 1.0f) < 0.05f;
}

//------------------------------------------------------------

namespace
{

class memory_reader: public nya_memory::memory_reader
{
public:
    std::string read_string()
    {
        std::string out;
        for (char c = read<char>(); c; c = read<char>())
            out.push_back(c);

        return out;
    }

    memory_reader(const void *data, size_t size): nya_memory::memory_reader(data, size) {}
};

}

//------------------------------------------------------------

namespace renderer
{
//------------------------------------------------------------

bool mesh_ndxr::load(const void *data, size_t size, const nya_render::skeleton &skeleton, bool endianness)
{
    memory_reader reader(data, size);

    typedef uint32_t uint;
    typedef uint16_t ushort;

    struct ndxr_header
    {
        char sign[4];
        uint size;
        ushort version;
        ushort groups_count;
        ushort bone_min_idx; //if skeleton not presented,
        ushort bone_max_idx; //this two may be strange

        uint offset_to_indices; //from header (+48)
        uint indices_buffer_size;
        uint vertices_buffer_size; //offset to vertices = offset_to_indices + 48 + indices_buffer_size

        uint unknown_often_zero;
        float bbox_origin[3];
        float bbox_size;

        void change_endianness()
        {
            ::change_endianness(*this);
            std::swap(version, groups_count);
            version = swap_bytes(version);
            std::swap(bone_min_idx, bone_max_idx);
        }
    };

    ndxr_header header = reader.read<ndxr_header>();
    if (memcmp(header.sign, "NDXR", 4) != 0)
        return false;

    if (endianness)
        header.change_endianness();

    assume(header.size <= size);
    assume(header.version == 2);

    struct ndxr_group_header
    {
        float bbox_origin[3];
        float bbox_size;
        float origin[3];
        uint unknown_zero;
        uint offset_to_name; //from the end of vertices buf
        uint unknown;
        short bone_idx;
        short render_groups_count;
        uint offset_to_render_groups;

        void change_endianness()
        {
            ::change_endianness(*this);
            std::swap(bone_idx, render_groups_count);
        }
    };

    struct ndxr_render_group_header
    {
        uint ibuf_offset;
        uint vbuf_offset;
        uint unknown;
        ushort vcount;
        ushort vertex_format;
        uint offset_to_tex_info;
        uint unknown_zero3[3];
        ushort icount;
        ushort unknown3; //skining-related
        uint unknown_zero5[3];

        void change_endianness()
        {
            ::change_endianness(*this);
            std::swap(vcount, vertex_format);
            std::swap(icount, unknown3);
            vertex_format = swap_bytes(vertex_format);
        }
    };

    struct tex_info_header
    {
        ushort unknown;
        ushort unknown2;
        uint unknown_zero;

        ushort unknown3;
        ushort tex_info_count;
        ushort unknown4;
        ushort unknown5;
        ushort unknown6;
        ushort unknown_2;
        uint unknown_zero2[3];

        void change_endianness()
        {
            ::change_endianness(*this);
            std::swap(unknown, unknown2);
            std::swap(unknown3, tex_info_count);
            std::swap(unknown4, unknown5);
            std::swap(unknown6, unknown_2);
        }
    };

    struct tex_info
    {
        uint texture_hash_id;
        uint unknown_zero;
        uint unknown2_zero;

        uint unknown3;
        ushort unknown4;
        ushort unknown5;
        uint unknown6_zero;

        void change_endianness()
        {
            ::change_endianness(*this);
            std::swap(unknown4, unknown5);
        }
    };

    struct ndxr_material_param
    {
        uint unknown_32_or_zero; //zero if last
        uint offset_to_name; //from the end of vertices buf
        uint unknown;
        uint unknown_zero2;

        union
        {
            float value[4];
            uint32_t uvalue;
        };

        void change_endianness()
        {
            ::change_endianness(*this);
        }
    };

    struct material_param
    {
        std::string name;
        ndxr_material_param data;
    };

    struct render_group
    {
        ndxr_render_group_header header;
        tex_info_header tex_header;
        std::vector<tex_info> tex_infos;
        std::vector<material_param> material_params;
    };

    struct group
    {
        std::string name;
        ndxr_group_header header;
        std::vector<render_group> render_groups;
    };

    std::vector<group> groups(header.groups_count);

    for (auto &g: groups)
    {
        g.header = reader.read<ndxr_group_header>();
        assume(g.header.unknown_zero==0);
        if (endianness)
            g.header.change_endianness();
    }

    size_t strings_offset = header.offset_to_indices + 48 + header.indices_buffer_size + header.vertices_buffer_size;
    reader.seek(strings_offset);
    while (reader.get_remained() > 4 && reader.read<uint>() == 0) { strings_offset += 4; }

    for (auto &g: groups)
    {
        g.render_groups.resize(g.header.render_groups_count);
        reader.seek(g.header.offset_to_render_groups);
        for (auto &rg: g.render_groups)
        {
            rg.header = reader.read<ndxr_render_group_header>();
            if (endianness)
                rg.header.change_endianness();
        }

        reader.seek(strings_offset+g.header.offset_to_name);
        g.name = reader.read_string();
    }

    for (auto &g: groups)
    {
        for (auto &rg: g.render_groups)
        {
            reader.seek(rg.header.offset_to_tex_info);
            rg.tex_header = reader.read<tex_info_header>();
            if (endianness)
                rg.tex_header.change_endianness();

            rg.tex_infos.resize(rg.tex_header.tex_info_count);
            for (auto &ti: rg.tex_infos)
            {
                ti = reader.read<tex_info>();
                if (endianness)
                    ti.change_endianness();
            }

            while(reader.check_remained(sizeof(ndxr_material_param)))
            {
                material_param p;
                p.data = reader.read<ndxr_material_param>();
                if (endianness)
                    p.data.change_endianness();

                rg.material_params.push_back(p);

                if (!p.data.unknown_32_or_zero)
                    break;
            }

            for (auto &p: rg.material_params)
            {
                reader.seek(strings_offset + p.data.offset_to_name);
                p.name = reader.read_string();
            }
        }
    }

    this->groups.resize(groups.size());
    for (size_t i = 0; i < groups.size(); ++i)
    {
        const auto &from = groups[i];
        auto &to = this->groups[i];

        to.name = from.name;
        to.rgroups.resize(from.render_groups.size());
        for (size_t k = 0; k < from.render_groups.size(); ++k)
        {
            const auto &rfrom = from.render_groups[k];
            auto &rto = to.rgroups[k];

            rto.textures.resize(rfrom.tex_infos.size());
            for (size_t l = 0; l < rfrom.tex_infos.size(); ++l)
                rto.textures[l] = rfrom.tex_infos[l].texture_hash_id;
        }
    }

    //vertices and indices data

    bool use_indices4b = false;
    uint total_rgf_count = 0;
    for (auto &gf: groups)
        total_rgf_count += (uint)gf.render_groups.size();

    uint add_vertex_offset = 0, total_rgf_idx = 0;
    for (int i = 0; i < header.groups_count; ++i)
    {
        const auto &gf = groups[i];
        auto &gt = this->groups[i];

        for (int j = 0; j < gf.render_groups.size(); ++j, ++total_rgf_idx)
        {
            const auto &rgf = gf.render_groups[j];
            auto &rgt = gt.rgroups[j];

            if (!rgf.header.vcount || !rgf.header.icount)
                continue;

            //printf("%s\n", gf.name.c_str());

            //ToDo
            const bool day = gf.name.find("dayt_") != std::string::npos;
            const bool night = gf.name.find("nigt_") != std::string::npos;

            rgt.offset = uint(use_indices4b ? indices4b.size() : indices2b.size());
            rgt.count = rgf.header.icount;

            bool opaque = gf.name.find("OBJ_O") != std::string::npos; //really?

            //ToDo: correct apaque/alpha instead of guessing
            if (gf.name.find("mrot0") != std::string::npos || gf.name.find("_prop") != std::string::npos ||
                gf.name.find("auta_") != std::string::npos || gf.name.find("auto_") != std::string::npos)
                opaque = false;
            if (gf.name.find("plth_") != std::string::npos)
                opaque = true;

            if (gf.name.find("_SHR") == std::string::npos && gf.name.find("_shl") == std::string::npos && gf.name.find("_l") == std::string::npos)
                rgt.illum = true;

            const bool as_opaque = gf.name.find("_AS_OPAQUE") != std::string::npos;
            if (as_opaque || day || night)
                opaque = true;

            if (gf.name.find("alpha") != std::string::npos)
                rgt.alpha_clip = true;

            if (!opaque || as_opaque)
                rgt.blend = true;

            rgt.param_idx = total_rgf_idx;

            reader.seek(header.offset_to_indices + 48 + header.indices_buffer_size + rgf.header.vbuf_offset);

            const float *ndxr_verts = (float *)reader.get_data();

            const uint first_index = uint(verts.size());
            verts.resize(first_index+rgf.header.vcount);

            const float ptc = (total_rgf_idx + 0.5f) / total_rgf_count;
            const float bone_fidx = (skeleton.get_bones_count() <= 0 || gf.header.bone_idx < 0) ? 0.0f : float(gf.header.bone_idx);
            const float bone_weight = (skeleton.get_bones_count() <= 0 || gf.header.bone_idx < 0) ? 0.0f : 1.0f;

            for (int i = first_index; i < first_index + rgf.header.vcount; ++i)
            {
                verts[i].param_tc = ptc;
                verts[i].bones[0] = bone_fidx;
                verts[i].weights[0] = bone_weight;
                for (int j = 1; j < 4; ++j)
                {
                    verts[i].bones[j] = 0;
                    verts[i].weights[j] = 0.0f;
                }

                for (int j = 0; j < 4; ++j)
                {
                    verts[i].normal[j] = verts[i].tangent[j] = verts[i].bitangent[j] = 0;
                    verts[i].color[j] = 255;
                }
            }

            const ushort format = rgf.header.vertex_format;
            const ushort uv_count = format >> 12;
            const bool has_tbn = format & (1<<0);
            const bool has_skining = (format & (1<<4)) > 0;
            const bool has_normal = (format & (1<<8)) > 0;
            const bool has_color = (format & (1<<9)) > 0;

            assume(uv_count == 1 || uv_count == 2);
            if(has_skining)
                assert(uv_count==1);

            const void *prev_skining_off = 0;
            if (has_skining)
            {
                //ToDo: endianness

                for (int i = 0; i < rgf.header.vcount; ++i)
                    memcpy(&verts[i + first_index].tc, &ndxr_verts[i * 2], sizeof(verts[0].tc));

                reader.seek(header.offset_to_indices + 48 + header.indices_buffer_size + header.vertices_buffer_size + add_vertex_offset);
                ndxr_verts = (float *)reader.get_data();
                prev_skining_off = ndxr_verts = (float *)reader.get_data();
            }

            const std::vector<ushort> known_formats = {4096,4102,4112,4358,4359,4369,4371,4608,4614,4865,4870,4871,8454,8455,8710,8966,8967};
            if(std::find(known_formats.begin(), known_formats.end(), format) == known_formats.end())
            {
                nya_log::log()<<"unknown vertex format: "<<format<<"\t"<<to_bits(format)<<"\n";
                //print_data(reader,reader.get_offset(),512,0,0,endianness);
            }
            else if (has_skining)
            {
                for (int i = first_index; i < (rgf.header.vcount + first_index); ++i)
                {
                    //ToDo: endianness

                    memcpy(&verts[i].pos, ndxr_verts, sizeof(verts[0].pos));
                    ndxr_verts += 3;

                    ndxr_verts++; //ToDo

                    if (format == 4369) //ToDo: n
                        ndxr_verts += 4;
                    else if (format == 4371) //ToDo: nbt
                        ndxr_verts += 12;

                    for (int j = 0; j < 4; ++j)
                        verts[i].bones[j] = *(int *)ndxr_verts++;

                    for (int j = 0; j < 4; ++j)
                        verts[i].weights[j] = *ndxr_verts++;
                }

                add_vertex_offset += size_t((char *)ndxr_verts - (char *)prev_skining_off);
            }
            else for (int i = first_index; i < (rgf.header.vcount + first_index); ++i)
            {
                memcpy(&verts[i].pos, ndxr_verts, sizeof(verts[0].pos));
                if (endianness)
                    change_endianness(verts[i].pos);
                ndxr_verts += 3;

                if (format == 4102) ++ndxr_verts; //ToDo?

                if (format == 4865)
                {
                    ++ndxr_verts;
                    if (endianness)
                    {
                        auto n = *(nya_math::vec3 *)ndxr_verts;
                        change_endianness(n);
                        float3_to_half3(&n.x, verts[i].normal);
                    }
                    else
                        float3_to_half3(ndxr_verts, verts[i].normal);
                    ndxr_verts += 3;
                    ++ndxr_verts;

                    assume(is_normal(verts[i].normal));
                }
                else
                {
                    if (has_normal)
                    {
                        memcpy(verts[i].normal, ndxr_verts, sizeof(verts[0].normal));
                        if (endianness)
                            for (auto &n: verts[i].normal)
                                n = swap_bytes(n);
                        assume(is_normal(verts[i].normal));
                        ndxr_verts += 2;
                    }

                    if (has_tbn)
                    {
                        memcpy(verts[i].bitangent, ndxr_verts, sizeof(verts[0].bitangent));
                        if (endianness)
                            for (auto &n: verts[i].bitangent)
                                n = swap_bytes(n);
                        assume(is_normal(verts[i].bitangent));
                        ndxr_verts += 2;

                        memcpy(verts[i].tangent, ndxr_verts, sizeof(verts[0].tangent));
                        if (endianness)
                            for (auto &n: verts[i].tangent)
                                n = swap_bytes(n);
                        assume(is_normal(verts[i].tangent));
                        ndxr_verts += 2;
                    }
                }

                if (has_normal && has_color)
                {
                    memcpy(&verts[i].color, ndxr_verts, sizeof(verts[0].color));
                    if (endianness)
                        change_endianness(verts[i].color);
                    ++ndxr_verts;
                }

                if (uv_count)
                {
                    memcpy(&verts[i].tc, ndxr_verts, sizeof(verts[0].tc));
                    if (endianness)
                        change_endianness(verts[i].tc);
                    ndxr_verts += 2;
                }

                if (has_color && !has_normal) ++ndxr_verts; //ToDo: color

                if (uv_count > 1) ndxr_verts += (uv_count - 1) * 2; //ToDo: uv2

                if (format == 4614)
                    ++ndxr_verts;
            }

            //ToDo
            if (night)
            {
                rgt.offset = rgt.count = 0;
                continue;
            }

            if (gf.header.bone_idx > 0)
            {
                for (int i = first_index; i < first_index + rgf.header.vcount; ++i)
                    *(nya_math::vec3 *)&verts[i].pos = skeleton.get_bone_rot(gf.header.bone_idx).rotate(*(nya_math::vec3 *)&verts[i].pos)
                    + skeleton.get_bone_pos(gf.header.bone_idx);
            }

            reader.seek(header.offset_to_indices + 48 + rgf.header.ibuf_offset); //rgf.header.icount
            const ushort *ndxr_indices = (ushort *)reader.get_data();

            if (!use_indices4b && verts.size() > 65535)
            {
                indices4b.resize(indices2b.size());
                for (size_t i = 0; i < indices2b.size(); ++i)
                    indices4b[i] = indices2b[i];

                use_indices4b = true;
                indices2b.clear();
            }

            if (use_indices4b)
            {
                indices4b.resize(rgt.offset + rgt.count);

                assert(!endianness); //ToDo

                for (uint i = 0; i < rgt.count; ++i)
                    indices4b[i + rgt.offset] = first_index + ndxr_indices[i];
            }
            else
            {
                indices2b.resize(rgt.offset + rgt.count);

                if (endianness)
                {
                    const uint16_t primitive_restart = uint16_t(-1);
                    for (uint i = 0; i < rgt.count; ++i)
                    {
                        const auto idx = swap_bytes(ndxr_indices[i]);
                        indices2b[i + rgt.offset] = idx == primitive_restart ? primitive_restart : first_index + idx;
                    }
                }
                else
                    for (uint i = 0; i < rgt.count; ++i)
                        indices2b[i + rgt.offset] = first_index + ndxr_indices[i];
            }
        }
    }

    return true;
}

//------------------------------------------------------------

void mesh_ndxr::reduce_groups()
{
    std::vector<unsigned short> regroup_indices2b;
    std::vector<unsigned int> regroup_indices4b;
    const bool use_indices4b = !indices4b.empty();

    std::vector<group> new_groups(2);
    new_groups[0].name = "opaque";
    new_groups[1].name = "transparent";

    for (int i = 0; i < 2; ++i)
    {
        auto &ng = new_groups[i];
        for (auto &g: groups)
        {
            for (auto &rg: g.rgroups)
            {
                if (!rg.count) //used
                    continue;

                if ((i == 1) != rg.blend)
                    continue;

                ng.rgroups.push_back(rg);
                auto &nrg = ng.rgroups.back();
                if (use_indices4b)
                {
                    const auto off = nrg.offset = (unsigned int)regroup_indices4b.size();
                    regroup_indices4b.resize(off + rg.count);
                    memcpy(&regroup_indices4b[off], &indices4b[rg.offset], rg.count * 4);
                }
                else
                {
                    const auto off = nrg.offset = (unsigned int)regroup_indices2b.size();
                    regroup_indices2b.resize(off + rg.count);
                    memcpy(&regroup_indices2b[off], &indices2b[rg.offset], rg.count * 2);
                }

                if (i != 0)
                    continue;

                rg.count = 0; //used

                for (auto &ag: groups)
                {
                    for (auto &arg: ag.rgroups)
                    {
                        if (!arg.count)
                            continue;

                        if (rg.blend != arg.blend || rg.textures != arg.textures)
                            continue;

                        if (use_indices4b)
                        {
                            const auto ioff1 = (unsigned int)regroup_indices4b.size();

                            //degenerate triangle
                            if (!regroup_indices4b.empty())
                                regroup_indices4b.push_back(regroup_indices4b.back());
                            regroup_indices4b.push_back(indices4b[arg.offset]);
                            if ( (ioff1 - nrg.offset) % 2 )
                                regroup_indices4b.push_back(regroup_indices4b.back());

                            const auto ioff2 = (unsigned int)regroup_indices4b.size();
                            regroup_indices4b.resize(ioff2 + arg.count);
                            memcpy(&regroup_indices4b[ioff2], &indices4b[arg.offset], arg.count * 2);
                            nrg.count += (unsigned int)regroup_indices4b.size() - ioff1;
                        }
                        else
                        {
                            const auto ioff1 = (unsigned int)regroup_indices2b.size();

                            //degenerate triangle
                            if (!regroup_indices2b.empty())
                                regroup_indices2b.push_back(regroup_indices2b.back());
                            regroup_indices2b.push_back(indices2b[arg.offset]);
                            if ( (ioff1 - nrg.offset) % 2 )
                                regroup_indices2b.push_back(regroup_indices2b.back());

                            const auto ioff2 = (unsigned int)regroup_indices2b.size();
                            regroup_indices2b.resize(ioff2 + arg.count);
                            memcpy(&regroup_indices2b[ioff2], &indices2b[arg.offset], arg.count * 2);
                            nrg.count += (unsigned int)regroup_indices2b.size() - ioff1;
                        }

                        arg.count = 0; //used
                    }
                }
            }
        }
    }

    groups.swap(new_groups);
    indices2b.swap(regroup_indices2b);
    indices4b.swap(regroup_indices4b);
}

//------------------------------------------------------------

nya_math::vec3 mesh_ndxr::vert::get_normal() const
{
    return half3_as_vec3(normal);
}

//------------------------------------------------------------
}
