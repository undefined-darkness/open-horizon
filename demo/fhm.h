//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"

//------------------------------------------------------------

class memory_reader;
struct fhm_mesh_load_data;
struct fhm_location_load_data;

//------------------------------------------------------------

class fhm_mesh
{
public:
    bool load(const char *fileName);
    void draw_col(int col_idx);
    void draw_mptx();

public:
    void draw_landscape();

protected:
    bool read_chunks_info(memory_reader &reader, size_t base_offset);
    bool read_mptx(memory_reader &reader);
    bool read_colh(memory_reader &reader);
    bool read_mnt(memory_reader &reader, fhm_mesh_load_data &load_data);
    bool read_mop2(memory_reader &reader, fhm_mesh_load_data &load_data);
    bool read_ndxr(memory_reader &reader, fhm_mesh_load_data &load_data);
    bool read_ntxr(memory_reader &reader, fhm_location_load_data &load_data);
    bool read_location_tex_indices(memory_reader &reader, fhm_location_load_data &load_data);
    bool read_location_patches(memory_reader &reader, fhm_location_load_data &load_data);
    bool finish_load_location(fhm_location_load_data &load_data);

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

    struct mptx_mesh
    {
        struct instance
        {
            nya_math::aabb bbox;
            nya_math::vec3 pos;
            float yaw;
        };

        std::vector<instance> instances;
        nya_render::vbo vbo;
        float draw_dist;
        std::vector<uint> textures;

        mptx_mesh(): draw_dist(0.0f) {}
    };

    std::vector<mptx_mesh> mptx_meshes;

    struct col { nya_render::vbo vbo; };
    std::vector<col> cols;

    struct chunk_info
    {
        ushort unknown1;
        ushort unknown2;
        uint unknown_16;
        uint offset; //+header size(48)
        uint size;
    };

    std::vector<chunk_info> chunks;

    struct landscape
    {
        struct group
        {
            uint offset;
            uint count;
            uint tex_id;
        };

        struct patch
        {
            std::vector<group> groups;
        };

        patch *const operator [] (int i) { return &patches[width * i]; }

        landscape(): width(0), height(0) {}

        std::vector<patch> patches;
        uint width;
        uint height;
        nya_render::vbo vbo;
    } m_landscape;

protected:
    nya_scene::shader m_location_objects_shader;
    int m_location_objects_light_dir_idx;
};

class ndxr_mesh: public fhm_mesh
{
public:
    void draw(int lod_idx);
    void update(int dt);

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; }

    bool has_anim(int lod_idx, unsigned int anim_hash_id);
    float get_relative_anim_time(int lod_idx, unsigned int anim_hash_id);
    void set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time);
    void set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed);

    void set_ndxr_texture(int lod_idx, const char *semantics, const char *file_name);

protected:
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
};

//------------------------------------------------------------
