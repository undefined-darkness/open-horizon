//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"

//------------------------------------------------------------

class memory_reader;
struct fhm_mesh_load_data;

//------------------------------------------------------------

class fhm_mesh
{
public:
    bool load(const char *fileName);

public:
    void draw(int lod_idx);
    void update(int dt);

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; }

    bool has_anim(int lod_idx, unsigned int anim_hash_id);
    float get_relative_anim_time(int lod_idx, unsigned int anim_hash_id);
    void set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time);
    void set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed);

    void set_ndxr_texture(int lod_idx, const char *semantics, const nya_scene::texture &tex);
    void set_ndxr_texture(int lod_idx, const char *semantics, const char *file_name)
    {
        set_ndxr_texture(lod_idx,semantics,nya_scene::texture(file_name));
    }

protected:
    bool read_mnt(memory_reader &reader, fhm_mesh_load_data &load_data);
    bool read_mop2(memory_reader &reader, fhm_mesh_load_data &load_data);
    bool read_ndxr(memory_reader &reader, fhm_mesh_load_data &load_data);

protected:
    typedef unsigned int uint;
    typedef unsigned short ushort;

    struct lod
    {
        struct group
        {
            short bone_idx;
        };

        nya_scene::mesh mesh;
        std::vector<group> groups;

        struct anim
        {
            float duration;
            float inv_duration;
            int layer;
        };

        std::map<uint, anim> anims;
    };

    std::vector<lod> lods;

protected:
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
};

//------------------------------------------------------------
