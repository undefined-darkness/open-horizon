//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm_mesh.h"
#include "containers/fhm.h"
#include "mesh_ndxr.h"

#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "render/render.h"
#include "scene/camera.h"
#include "scene/transform.h"
#include "scene/shader.h"

#include <math.h>
#include <stdint.h>

#include "shared.h"

#include "render/debug_draw.h"
extern nya_render::debug_draw test;

//------------------------------------------------------------

struct fhm_mnt { nya_render::skeleton skeleton; };
struct fhm_mop2 { std::map<std::string, nya_scene::animation> animations; };

//------------------------------------------------------------

static nya_math::vec3 light_dir = nya_math::vec3(0.5f, 1.0f, 0.5f).normalize(); //ToDo

//------------------------------------------------------------

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

//------------------------------------------------------------

void fhm_mesh::set_texture(int lod_idx, const char *semantics, const nya_scene::texture &tex)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return;

    if (!semantics)
        return;

    for (int i = 0; i < m_lods[lod_idx].mesh.get_groups_count(); ++i)
        m_lods[lod_idx].mesh.modify_material(i).set_texture(semantics, tex);
}

//------------------------------------------------------------

bool fhm_mesh::load(const char *file_name)
{
    fhm_file fhm;
    if (!fhm.open(file_name))
        return false;

    //mad heuristic

    int mnt_count = 0, mop2_count = 0, ndxr_offset = -1;
    for (int j = 0; j < fhm.get_chunks_count(); ++j)
    {
        const uint sign = fhm.get_chunk_type(j);
        if (sign == '\0TNM')
        {
            ++mnt_count;
        }
        else if (sign == '2POM')
        {
            ++mop2_count;
        }
        else if (sign == 'RXDN' && ndxr_offset == -1)
            ndxr_offset = j;
        else if (sign == 'RXTN')
        {
            const auto size = fhm.get_chunk_size(j);
            nya_memory::tmp_buffer_scoped buf(size);
            fhm.read_chunk_data(j, buf.get_data());
            shared::load_texture(buf.get_data(), buf.get_size());
        }
    }

    int lods_count = 0;
    for (int j = ndxr_offset; j < fhm.get_chunks_count(); ++j)
    {
        const uint sign = fhm.get_chunk_type(j);
        if (sign == 'RXDN' || sign == 0)
        {
            ++lods_count;
        }
        else
        {
            if (mnt_count > 2)
                assert(sign == '\0TNM');
            break;
        }
    }

    if (lods_count > 0)
    {
        std::vector<fhm_mnt> mnts(lods_count);
        std::vector<fhm_mop2> mop2s(lods_count);
        m_lods.resize(lods_count);

        const int signs[] = { '\0TNM', '2POM', 'RXDN' };

        const bool special_mnt = mnt_count <= 2;
        if (special_mnt && mnt_count > 0)
        {
            for (int j = 0; j < 2; ++j)
            {
                for (int i = 0, idx = 0; i < fhm.get_chunks_count(); ++i)
                {
                    if (fhm.get_chunk_type(i) == signs[j])
                    {
                        nya_memory::tmp_buffer_scoped buf(fhm.get_chunk_size(i));
                        fhm.read_chunk_data(i, buf.get_data());
                        memory_reader reader(buf.get_data(), buf.get_size());

                        const int from = (idx++) * lods_count/mnt_count, to = from + lods_count/mnt_count;
                        if (j == 0)
                        {
                            read_mnt(reader, mnts[from]);
                            for (int i = from + 1; i < to; ++i)
                                mnts[i] = mnts[from];
                        }
                        else
                        {
                            read_mop2(reader, mnts[from], mop2s[from]);
                            for (int i = from + 1; i < to; ++i)
                                mop2s[i] = mop2s[from];
                        }
                    }
                }
            }
        }

        const int offsets[] = { lods_count, lods_count * 2, 0 };
        for (int i = (special_mnt ? 2 : 0); i < 3; ++i)
        {
            for (int j = 0; j < lods_count; ++j)
            {
                const int idx = ndxr_offset + offsets[i] + j;
                const uint sign = fhm.get_chunk_type(idx);
                const auto size = fhm.get_chunk_size(idx);
                if (sign == signs[i])
                {
                    nya_memory::tmp_buffer_scoped buf(size);
                    fhm.read_chunk_data(idx, buf.get_data());
                    memory_reader reader(buf.get_data(), size);

                    switch(i)
                    {
                        case 0: read_mnt(reader, mnts[j]); break;
                        case 1: read_mop2(reader, mnts[j], mop2s[j]); break;
                        case 2: read_ndxr(reader, mnts[j], mop2s[j], m_lods[j]); break;
                    }
                }
                else
                    assert(sign == 0 && size == 0);
            }
        }
    }

    //ToDo: other chunks

    fhm.close();
    return true;
}

//------------------------------------------------------------

bool fhm_materials::load(const char *file_name)
{
    materials.clear();
    textures.clear();

    fhm_file m;
    if (!m.open(file_name))
        return false;

    for (int i = 0; i < m.get_chunks_count(); ++i)
    {
        auto t = m.get_chunk_type(i);

        if (t == ' SDD')
        {
            const auto size = m.get_chunk_size(i);
            nya_scene::resource_data data(size);
            m.read_chunk_data(i, data.get_data());
            nya_scene::shared_texture st;
            nya_scene::texture::load_dds(st, data, "");
            data.free();
            nya_scene::texture t;
            t.create(st);
            textures.push_back(t);
            continue;
        }

        if (t != 'ETAM')
            continue;

        material mat;

        assert(m.get_chunk_size(i) > 0);

        nya_memory::tmp_buffer_scoped b(m.get_chunk_size(i));
        m.read_chunk_data(i, b.get_data());
        nya_memory::memory_reader r(b.get_data(),b.get_size());

        struct mate_header
        {
            char sign[4];
            uint16_t mat_count;
            uint16_t render_groups_count;
            uint32_t groups_count;
            uint32_t offset_to_mat_offsets;
            uint32_t offset_to_render_groups; //elements 32b
            uint32_t offset_to_groups; //elements 8b
        };

        auto header = r.read<mate_header>();

        struct mat_offset
        {
            uint32_t offset;
            uint32_t zero[2];
            uint32_t unknown;
        };

        r.seek(header.offset_to_mat_offsets);
        mat.params.resize(header.mat_count);
        std::vector<mat_offset> mat_offsets(header.mat_count);
        for (size_t j = 0; j < mat_offsets.size(); ++j)
        {
            mat_offsets[j] = r.read<mat_offset>();
            assume(mat_offsets[j].zero[0] == 0 && mat_offsets[j].zero[1] == 0);
        }

        for (size_t j = 0; j < mat_offsets.size(); ++j)
        {
            r.seek(mat_offsets[j].offset);

            struct mat_header
            {
                uint32_t unknown;
                uint32_t zero;
                uint16_t unknown2;
                uint16_t unknown_count;
                uint32_t unknown3;
                uint16_t unknown4;
                uint16_t unknown5;
                uint32_t zero3[3];
            };

            auto header = r.read<mat_header>();
            assume(header.zero == 0 && header.zero3[0] == 0 && header.zero3[1] == 0 && header.zero3[2] == 0);
            r.skip(header.unknown_count * 24);

            auto &params = mat.params[j];

            while (r.get_remained())
            {
                struct value
                {
                    uint32_t unknown_32_or_zero;
                    uint32_t offset_to_name;
                    uint32_t unknown;
                    uint32_t zero;
                    float val[4];
                };

                auto v = r.read<value>();
                assume(v.zero == 0);

                const char *name = (char *)r.get_data() + v.offset_to_name - r.get_offset();
                assert(name && name[0]);
                params.push_back(std::make_pair(name, nya_math::vec4(v.val)));

                //printf("%s\n", name);

                if (v.unknown_32_or_zero == 0)
                    break;

                assume(v.unknown_32_or_zero == 32);
            }
        }

        r.seek(header.offset_to_render_groups);
        mat.render_groups.resize(header.render_groups_count);
        for (uint16_t j = 0; j < header.render_groups_count; ++j)
        {
            struct bind
            {
                uint16_t idx;
                uint16_t mat_idx;
                uint32_t zero[3];
            };

            auto b = r.read<bind>();
            assert(b.idx == j);
            assume(b.zero[0] == 0 && b.zero[1] == 0 && b.zero[2] == 0);

            mat.render_groups[b.idx] = b.mat_idx;
        }

        r.seek(header.offset_to_groups);

        mat.groups_offset_count.resize(header.groups_count);
        for (auto &g: mat.groups_offset_count)
        {
            struct bind
            {
                uint16_t render_groups_count;
                uint16_t render_groups_offset;
                uint32_t zero;
            };

            auto b = r.read<bind>();
            assert(b.render_groups_count + b.render_groups_offset <= mat.render_groups.size());
            assume(b.zero == 0);

            g.first = b.render_groups_offset;
            g.second = b.render_groups_count;
        }

        materials.push_back(mat);
    }

    m.close();
    return true;
}

//------------------------------------------------------------

void fhm_mesh::set_material(int lod_idx, const fhm_materials::material &m, const char *shader)
{
    assert(!m_lods.empty());

    if (lod_idx < 0 || lod_idx >= get_lods_count())
        return;

    int idx = 0;
    auto &l = m_lods[lod_idx];
    if (!l.mesh.get_groups_count())
        return;

    assert(l.params_tex.is_valid());
    assert(l.params_tex->get_width() == m.render_groups.size());

    unsigned int rg_idx = 0;
    for (const auto &g: m.groups_offset_count)
    {
        for (int k = 0; k < g.second; ++k, ++rg_idx)
        {
            const auto &r = m.render_groups[g.first + k];
            const auto &ps = m.params;

            auto &m = l.mesh.modify_material(idx++);
            m.get_default_pass().set_shader(nya_scene::shader(shader));

            assert(r < ps.size());

            for (auto &p: ps[r])
            {
                if (p.first.find("HASH") != std::string::npos)
                    continue;

                if (p.first.find("FLAG") != std::string::npos) //ToDo
                    continue;

                if (p.first == "NU_ACE_vSpecularParam")
                    l.set_param(lod::specular_param, rg_idx, p.second);
                else if (p.first == "NU_ACE_vIBLParam")
                    l.set_param(lod::ibl_param, rg_idx, p.second);
                else if (p.first == "NU_ACE_vRimLightMtr")
                    l.set_param(lod::rim_light_mtr, rg_idx, p.second);
            }
        }
    }

    l.update_params_tex();
}

//------------------------------------------------------------

bool fhm_mesh::read_mnt(memory_reader &reader, fhm_mnt &mnt)
{
    struct mnt_header
    {
        char sign[4];
        uint unknown_1; //version?
        uint chunk_size;
        uint bones_count;
        uint unknown2;
        ushort unknown_trash[2];
        uint offset_to_unknown1; //trash
        uint offset_to_unknown2; //all indices+1
        uint offset_to_parents;
        uint offset_to_bones;
        uint size_of_unknown;
        uint offset_to_bones_names;
    };

    const mnt_header header = reader.read<mnt_header>();
    assume(header.unknown_1 == 1);

    struct mnt_bone
    {
        ushort idx;
        ushort unknown;
        ushort unknown2;
        short parent;
        ushort unknown3[4];
        uint unknown_zero;
    };

    struct bone
    {
        std::string name;
        uint unknown1;
        ushort unknown2;
        short parent;

        mnt_bone header;
    };

    std::vector<bone> bones(header.bones_count);

    reader.seek(header.offset_to_unknown1);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].unknown1 = reader.read<uint>();

    reader.seek(header.offset_to_unknown2);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].unknown2 = reader.read<ushort>();

    reader.seek(header.offset_to_parents);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].parent = reader.read<short>();

    reader.seek(header.offset_to_bones);
    for (int i = 0; i < header.bones_count; ++i)
    {
        bones[i].header = reader.read<mnt_bone>();
        assume(bones[i].parent == bones[i].header.parent);
    }

    reader.seek(header.offset_to_bones_names);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].name = reader.read_string();

    //printf("mnt\n");
    for (int i = 0; i < header.bones_count; ++i)
    {
        //printf("%d %s\n", i, bones[i].name.c_str());
        int b = mnt.skeleton.add_bone(bones[i].name.c_str(), nya_math::vec3(), nya_math::quat(), bones[i].parent, true);
        assert(b == i);
    }
    //printf("\n");

    return true;
}

//------------------------------------------------------------

bool fhm_mesh::read_mop2(memory_reader &reader, fhm_mnt &mnt, fhm_mop2 &mop2)
{
    struct mop2_header
    {
        char sign[4];// "MOP2"
        ushort unknown;
        ushort unknown2;
        uint size;
        uint equals_to_size;
        uint kfm1_count;
        uint offset_to_kfm1_sizes;
        uint offset_to_kfm1_data_offsets;
        uint offset_to_kfm1_names_offsets;
    };

    mop2_header header = reader.read<mop2_header>();

    struct kfm1_info
    {
        uint size;
        uint offset_to_data; //from chunk begin
        uint offset_to_name;
    };

    std::vector<kfm1_info> kfm1_infos(header.kfm1_count);

    reader.seek(header.offset_to_kfm1_sizes);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].size = reader.read<uint>();

    reader.seek(header.offset_to_kfm1_data_offsets);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].offset_to_data = reader.read<uint>();

    reader.seek(header.offset_to_kfm1_names_offsets);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].offset_to_name = reader.read<uint>();

    struct kfm1_sequence_header
    {
        ushort unknown;
        ushort offsets_size;
        ushort first_offset;
        ushort unknown2; //looks like the total size
    };

    struct kfm1_sequence_bone
    {
        ushort frames_offset;
        ushort frames_count;
        ushort offset;
    };

    struct kfm1_sequence
    {
        ushort unknown;
        uint sequence_size;
        uint sequence_offset;

        kfm1_sequence_header header;

        std::vector<kfm1_sequence_bone> bones;
    };

    struct kfm1_bone_info
    {
        ushort unknown[3];
        ushort bone_idx;
        ushort unknown2; // 773 or 1031
        ushort unknown3;
        ushort unknown4;
        ushort unknown_4;
        uint unknown6;
        uint unknown_zero[3];
    };

    struct kmf1_header
    {
        char sign[4];// "KMF1"
        uint unknown;
        uint unknown2;
        uint size; //size and offsets from header.offset_to_kmf1
        ushort unknown3;
        ushort bones_count;
        ushort bones2_count;
        ushort sequences_count;
        uint offset_to_bones;
        uint offset_to_sequences;
        uint offset_to_sequences_offsets;
        uint unknown7;
        uint offset_to_name;
        uint unknown8;
        uint unknown9;
        uint unknown10;
        uint unknown_zero2[2];
    };

    struct kfm1_struct
    {
        std::string name;
        kmf1_header header;
        std::vector<kfm1_bone_info> bones;
        std::vector<kfm1_bone_info> bones2;
        std::vector<kfm1_sequence> sequences;
    };

    struct bone
    {
        int parent;
        nya_math::vec3 pos;
        nya_math::quat rot;
        nya_math::quat irot;

        nya_math::quat get_rot(const std::vector<bone> &bones) const
        {
            if (parent < 0)
                return rot;

            return bones[parent].get_rot(bones) * rot;
        }

        nya_math::vec3 get_pos(const std::vector<bone> &bones) const
        {
            if (parent < 0)
                return pos;

            return bones[parent].get_rot(bones).rotate(pos) + bones[parent].get_pos(bones);
        }
    };

    int basepos_idx = -1;
    std::vector<kfm1_struct> kfm1_structs(header.kfm1_count);
    for (int i = 0; i < header.kfm1_count; ++i)
    {
        reader.seek(kfm1_infos[i].offset_to_name);
        kfm1_structs[i].name = reader.read_string();
        if (kfm1_structs[i].name == "basepose")
            basepos_idx = i;
    }

    assert(basepos_idx>=0);
    assume(basepos_idx == 0);

    std::vector<bone> base(mnt.skeleton.get_bones_count());
    for (int fi = 0; fi < header.kfm1_count; ++fi)
    {
        const int i = (fi == 0 ? basepos_idx : (fi == basepos_idx ? 0 : fi));
        const bool first_anim = i == 0;

        reader.seek(kfm1_infos[i].offset_to_data);
        memory_reader kfm1_reader(reader.get_data(), kfm1_infos[i].size);

        if (kfm1_structs[i].name == "swp1" || kfm1_structs[i].name == "swp2" ) //ToDo
            continue;

        //print_data(kfm1_reader, 0, kfm1_reader.get_remained());

        kfm1_struct &kfm1 = kfm1_structs[i];

        kfm1.header = kfm1_reader.read<kmf1_header>();

        //assert(!kfm1.header.bones2_count || kfm1.header.bones2_count >= kfm1.header.bones_count);

        kfm1_reader.seek(kfm1.header.offset_to_bones);
        kfm1.bones.resize(kfm1.header.bones_count);
        for (auto &b : kfm1.bones)
        {
            b = kfm1_reader.read<kfm1_bone_info>();
            //assert(b.unknown_4 == 4);
            assume(b.unknown4 % 4 == 0);
            if (b.unknown2 != 1031 && b.unknown2 != 773) printf("%d\n", b.unknown2);
            assert(b.unknown2 == 1031 || b.unknown2 == 773);
        }

        kfm1.bones2.resize(kfm1.header.bones2_count);
        for (auto &b : kfm1.bones2)
        {
            b = kfm1_reader.read<kfm1_bone_info>();
            assume(b.unknown_4 == 4);
            assume(b.unknown4 % 4 == 0);
            if (b.unknown2 != 1031 && b.unknown2 != 773) printf("%d\n", b.unknown2);
            assert(b.unknown2 == 1031 || b.unknown2 == 773);
        }

        kfm1.sequences.resize(kfm1.header.sequences_count);
        kfm1_reader.seek(kfm1.header.offset_to_sequences);
        //print_data(kfm1_reader, kfm1_reader.get_offset(), 3000);

        for (int k = 0; k < kfm1.header.sequences_count; ++k)
            kfm1.sequences[k].unknown = kfm1_reader.read<ushort>();

        kfm1_reader.seek(kfm1.header.offset_to_sequences_offsets);
        for (int k = 0; k < kfm1.header.sequences_count; ++k)
        {
            kfm1.sequences[k].sequence_size = kfm1_reader.read<uint>();
            kfm1.sequences[k].sequence_offset = kfm1_reader.read<uint>();
        }

        nya_scene::shared_animation anim;

        //printf("kfm1 name: %s sequences %d\n", kfm1.name.c_str(), kfm1.header.sequences_count);

        for (int k = 0; k < kfm1.header.sequences_count; ++k)
        {
            bool is_bones2 = (!kfm1.bones2.empty() && k > 0);

            kfm1_sequence &f = kfm1.sequences[k];
            kfm1_reader.seek(kfm1.sequences[k].sequence_offset);
            memory_reader sequence_reader(kfm1_reader.get_data(), f.sequence_size);

            assert((f.unknown > 0 && k > 0) || (!f.unknown && !k) );

            //print_data(sequence_reader, 0, f.sequence_size);

            f.header = sequence_reader.read<kfm1_sequence_header>();
            f.bones.resize(is_bones2 ? kfm1.header.bones2_count : kfm1.header.bones_count);

            size_t first_offset = -1;

            for (auto &b:f.bones)
            {
                b.frames_count = sequence_reader.read<int>();
                b.frames_offset = sequence_reader.read<ushort>();
                b.offset = sequence_reader.read<ushort>();

                if (b.offset < first_offset)
                    first_offset = b.offset;
            }

            if (!f.bones.empty())
            {
                auto header_remained = f.bones.front().offset - first_offset;
                assume(header_remained == 0);
            }

            size_t last_offset = sequence_reader.get_offset();
            for (int j = 0; j < f.bones.size(); ++j)
            {
                sequence_reader.seek(f.bones[j].offset);

                const auto &b = is_bones2 ? kfm1.bones2[j] : kfm1.bones[j];

                const int idx = b.bone_idx;

                if (idx >= mnt.skeleton.get_bones_count())
                    continue;

                assert(idx < mnt.skeleton.get_bones_count());
                const int aidx = anim.anim.add_bone(mnt.skeleton.get_bone_name(idx));
                assert(aidx >= 0);
                const int frame_time = 16;

                if (first_anim)
                    base[idx].parent = mnt.skeleton.get_bone_parent_idx(idx);

                bool is_quat = b.unknown2 == 1031;

                for (int l = 0; l < f.bones[j].frames_count; ++l)
                {
                    nya_math::vec4 value;
                    value.x = sequence_reader.read<float>();
                    value.y = sequence_reader.read<float>();
                    value.z = sequence_reader.read<float>();
                    if (is_quat)
                        value.w = sequence_reader.read<float>();

                    if (first_anim)
                    {
                        if (is_quat)
                        {
                            base[idx].rot.v = value.xyz();
                            base[idx].rot.w = value.w;
                            base[idx].irot = nya_math::quat::invert(base[idx].rot);
                        }
                        else
                            base[idx].pos = value.xyz();
                    }
                    else
                    {
                        if (is_quat)
                        {
                            auto q = base[idx].irot * nya_math::quat(value.x, value.y, value.z, value.w);
                            if (q.w < 0)
                                q = -q;
                            anim.anim.add_bone_rot_frame(aidx, l * frame_time, q);
                        }
                        else
                            anim.anim.add_bone_pos_frame(aidx, l * frame_time, value.xyz() - base[idx].pos);
                    }
                }

                if (sequence_reader.get_offset() > last_offset)
                    last_offset = sequence_reader.get_offset();
            }

            sequence_reader.seek(last_offset);
/*
            if (sequence_reader.get_remained())
                print_data(sequence_reader);
            //printf("sequence remained %ld\n", sequence_reader.get_remained());
*/
            if (first_anim && k == 0)
            {
                nya_render::skeleton s;
                for (int i = 0; i < mnt.skeleton.get_bones_count(); ++i)
                {
                    const int b = s.add_bone(mnt.skeleton.get_bone_name(i), base[i].get_pos(base), base[i].get_rot(base),
                                             mnt.skeleton.get_bone_parent_idx(i), true);
                    assert(i == b);
                }

                mnt.skeleton = s;
            }
            
            //printf("    frame %d bones %d\n", k, int(f.bones.size()));
        }

        if (!first_anim)
            mop2.animations[kfm1.name].create(anim);

        //printf("duration %.2fs\n", anim.anim.get_duration()/1000.0f);
    }

    for (auto &a: mop2.animations)
    {
        a.second.set_speed(0.0f);
        a.second.set_loop(false);
    }

    return true;
}

//------------------------------------------------------------

bool fhm_mesh::read_ndxr(memory_reader &reader, const fhm_mnt &mnt, const fhm_mop2 &mop2, lod &l)
{
    renderer::mesh_ndxr nmesh;
    if (!nmesh.load(reader.get_data(), reader.get_remained(), mnt.skeleton, false))
        return false;

    nya_scene::shared_mesh mesh;
    mesh.skeleton = mnt.skeleton;

    nya_scene::material mat;
    auto &p=mat.get_pass(mat.add_pass(nya_scene::material::default_pass));
    p.set_shader(nya_scene::shader("shaders/object.nsh"));
    mat.set_param(mat.get_param_idx("light dir"), light_dir.x, light_dir.y, light_dir.z, 0.0f);
    p.get_state().set_cull_face(true, nya_render::cull_face::ccw);
    p.get_state().depth_comparsion = nya_render::depth_test::not_greater;

    unsigned int total_rgf_count = 0;
    for (auto &gf: nmesh.groups)
        total_rgf_count += (unsigned int)gf.rgroups.size();

    l.params_buf.resize(total_rgf_count * lod::params_count);
    l.params_tex.create();
    l.params_tex->build(&l.params_buf[0], total_rgf_count, lod::params_count, nya_render::texture::color_rgba32f);
    mat.set_texture("params", l.params_tex);

    if (mesh.skeleton.get_bones_count()>=250)
    {
        printf("bones %d\n", mesh.skeleton.get_bones_count());
        assert(mesh.skeleton.get_bones_count()<250); //shader uniforms count restriction
    }

    for (auto &ng: nmesh.groups)
    {
        for (auto &rg: ng.rgroups)
        {
            if(rg.illum)
                l.set_param(lod::diff_k, rg.param_idx, nya_math::vec4(1.0, 0.0, 0.0, 0.0));
            else
                l.set_param(lod::diff_k, rg.param_idx, nya_math::vec4(0.6, 0.4, 0.0, 0.0));

            if (rg.alpha_clip)
                l.set_param(lod::alpha_clip, rg.param_idx, nya_math::vec4(8/255.0, 0.0, 0.0, 0.0));
            else
                l.set_param(lod::alpha_clip, rg.param_idx, nya_math::vec4(-1.0, 0.0, 0.0, 0.0));
        }
    }

    l.update_params_tex();

    nmesh.reduce_groups();

    for (auto &ng: nmesh.groups)
    {
        for (auto &rg: ng.rgroups)
        {
            mesh.groups.resize(mesh.groups.size() + 1);
            auto &g = mesh.groups.back();
            g.offset = rg.offset;
            g.count = rg.count;
            g.elem_type = nya_render::vbo::triangle_strip;
            g.material_idx = (int)mesh.materials.size();
            mesh.materials.push_back(mat);

            //ToDo: specular, ambient, etc
            if (rg.textures.size()>0)
                mesh.materials.back().set_texture("diffuse", shared::get_texture(rg.textures[0]));

            if (rg.blend)
                mesh.materials.back().get_default_pass().get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
        }
    }

#define off(st, m) uint((size_t)(&((st *)0)->m))

    typedef renderer::mesh_ndxr::vert vert;
    mesh.vbo.set_tc(0, off(vert, tc), 3);
    mesh.vbo.set_normals(off(vert, normal), nya_render::vbo::float16);
    mesh.vbo.set_tc(1, off(vert, tangent), 3, nya_render::vbo::float16);
    mesh.vbo.set_tc(2, off(vert, bitangent), 3, nya_render::vbo::float16);
    mesh.vbo.set_tc(3, off(vert, bones), 4, nya_render::vbo::float32);
    mesh.vbo.set_tc(4, off(vert, weights), 4, nya_render::vbo::float32);

    mesh.vbo.set_vertex_data(nmesh.verts.data(), sizeof(vert), uint(nmesh.verts.size()));

    if (!nmesh.indices4b.empty())
        mesh.vbo.set_index_data(nmesh.indices4b.data(), nya_render::vbo::index4b, uint(nmesh.indices4b.size()));
    else
        mesh.vbo.set_index_data(nmesh.indices2b.data(), nya_render::vbo::index2b, uint(nmesh.indices2b.size()));

    l.mesh.create(mesh);

    //assert(!anims.empty());

    int layer = 0;
    for (auto &a: mop2.animations)
    {
        //assume(a.first == "basepose" || a.first.length() == 4);
        assert(a.first != "base");

        union { uint u; char c[4]; } hash_id;
        for (int i = 0; i < 4; ++i) hash_id.c[i] = a.first[3 - i];

        //if (hash_id.u == 'swp1' || hash_id.u == 'swp2') continue; //ToDo

        //printf("anim %d %s\n", layer, a.first.c_str());

        auto &la = l.anims[hash_id.u];
        la.layer = layer;
        la.duration = a.second.get_duration();
        la.inv_duration = a.second.get_duration() > 0 ? 1.0f / a.second.get_duration() : 0.0f;
        l.mesh.set_anim(a.second, layer++);
    }

    l.mesh.update(0);

    return true;
}

//------------------------------------------------------------

void fhm_mesh::draw(int lod_idx)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return;

    lod &l = m_lods[lod_idx];

    l.mesh.set_pos(m_pos);
    l.mesh.set_rot(m_rot);
    l.mesh.draw();
/*
if (lod_idx == 0)
{
    nya_render::depth_test::disable();
    static nya_render::debug_draw d;
    d.set_point_size(5.0);
    d.clear();
    d.add_skeleton(l.mesh.get_skeleton());
    //d.add_line(l.mesh.get_skeleton().get_bone_pos(16), l.mesh.get_skeleton().get_bone_pos(18), nya_math::vec4(1.0,0.0,0.0,1.0));
    //d.add_line(l.mesh.get_skeleton().get_bone_pos(65), l.mesh.get_skeleton().get_bone_pos(65) + nya_math::vec3(0,1,0), nya_math::vec4(0.0,1.0,0.0,1.0));
    //d.add_line(l.mesh.get_skeleton().get_bone_pos(66), l.mesh.get_skeleton().get_bone_pos(66) + nya_math::vec3(0,1,0), nya_math::vec4(0.0,0.0,1.0,1.0));
    //d.add_line(l.mesh.get_skeleton().get_bone_pos(33), l.mesh.get_skeleton().get_bone_pos(33) + nya_math::vec3(0,1,0), nya_math::vec4(1.0,0.0,1.0,1.0));
    //d.add_line(l.mesh.get_skeleton().get_bone_pos(60), l.mesh.get_skeleton().get_bone_pos(60) + nya_math::vec3(0,1,0), nya_math::vec4(1.0,1.0,0.0,1.0));

    static auto last_dvar = debug_variable::get();
    if (last_dvar != debug_variable::get())
        printf("bone %d %s\n", debug_variable::get(), l.mesh.get_skeleton().get_bone_name(last_dvar = debug_variable::get()));

    d.add_point(l.mesh.get_skeleton().get_bone_pos(debug_variable::get()), nya_math::vec4(0.0,1.0,0.0,0.5));
    d.draw();
}
*/
}

//------------------------------------------------------------

void fhm_mesh::update(int dt)
{
    for (auto &l: m_lods)
    {
        l.mesh.set_pos(m_pos);
        l.mesh.set_rot(m_rot);
        l.mesh.update(dt);
    }
}

//------------------------------------------------------------

void fhm_mesh::set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return;

    auto &l = m_lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return;

    auto ma = l.mesh.get_anim(a->second.layer);
    if (!ma.is_valid())
        return;

    ma->set_speed(speed);
}

//------------------------------------------------------------

void fhm_mesh::set_anim_weight(int lod_idx, unsigned int anim_hash_id, float weight)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return;

    auto &l = m_lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return;

    auto ma = l.mesh.get_anim(a->second.layer);
    if (!ma.is_valid())
        return;

    ma->set_weight(weight);
}

//------------------------------------------------------------

float fhm_mesh::get_relative_anim_time(int lod_idx, unsigned int anim_hash_id)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return 0.0f;

    auto &l = m_lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return 0.0f;

    return l.mesh.get_anim_time(a->second.layer) * a->second.inv_duration;
}

//------------------------------------------------------------

void fhm_mesh::set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return;

    auto &l = m_lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return;

    l.mesh.set_anim_time(time * a->second.duration, a->second.layer);
}

//------------------------------------------------------------

bool fhm_mesh::has_anim(int lod_idx, unsigned int anim_hash_id)
{
    if (lod_idx < 0 || lod_idx >= m_lods.size())
        return false;

    return m_lods[lod_idx].anims.find(anim_hash_id) != m_lods[lod_idx].anims.end();
}

//------------------------------------------------------------
