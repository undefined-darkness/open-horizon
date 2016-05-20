//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include <assert.h>
#include <stdint.h>

//------------------------------------------------------------

class memory_reader;
struct fhm_mesh_load_data;
struct fhm_mnt;
struct fhm_mop2;

//------------------------------------------------------------

struct fhm_materials
{
public:
    bool load(const char *file_name);

public:
    typedef std::vector< std::pair<std::string, nya_math::vec4> > material_params;

    struct material
    {
        std::vector<material_params> params;
        std::vector<uint16_t> render_groups;
        std::vector<std::pair<uint16_t,uint16_t> > groups_offset_count;
    };

    std::vector<material> materials;
    std::vector<nya_scene::texture> textures;
};

//------------------------------------------------------------

class fhm_mesh
{
public:
    bool load(const char *file_name);

public:
    void draw(int lod_idx);
    void update(int dt);

    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; }
    const nya_math::vec3 &get_pos() const { return m_pos; }
    const nya_math::quat &get_rot() const { return m_rot; }

    bool has_anim(int lod_idx, unsigned int anim_hash_id);
    float get_relative_anim_time(int lod_idx, unsigned int anim_hash_id);
    void set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time);
    void set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed);
    void set_anim_weight(int lod_idx, unsigned int anim_hash_id, float weight);

    void set_material(int lod_idx, const fhm_materials::material &m, const char *shader);

    void set_texture(int lod_idx, const char *semantics, const nya_scene::texture &tex);
    void set_texture(int lod_idx, const char *semantics, const char *file_name)
    {
        set_texture(lod_idx,semantics,nya_scene::texture(file_name));
    }

    int get_lods_count() const { return (int)m_lods.size(); }
    nya_scene::mesh &get_mesh(int lod_idx) { assert(lod_idx>=0 && lod_idx < (int)m_lods.size()); return m_lods[lod_idx].mesh; }

protected:
    bool read_mnt(memory_reader &reader, fhm_mnt &mnt);
    bool read_mop2(memory_reader &reader, fhm_mnt &mnt, fhm_mop2 &mop2);
    struct lod;
    bool read_ndxr(memory_reader &reader, const fhm_mnt &mnt, const fhm_mop2 &mop2, lod &l);

protected:
    typedef unsigned int uint;
    typedef unsigned short ushort;

    struct lod
    {
        nya_scene::mesh mesh;

        nya_scene::texture_proxy params_tex;
        std::vector<nya_math::vec4> params_buf;

        enum param_type
        {
            specular_param,
            ibl_param,
            rim_light_mtr,
            alpha_clip,
            diff_k,
            params_count
        };

        void set_param(param_type t, int idx, const nya_math::vec4 &p)
        {
            assert(params_tex.is_valid());
            size_t pidx = params_tex->get_width() * t + idx;
            assert(pidx < params_buf.size());
            params_buf[pidx] = p;
        }

        void update_params_tex()
        {
            assert(params_tex.is_valid() && !params_buf.empty());
            nya_render::texture::filter f[3];
            nya_render::texture::get_default_filter(f[0],f[1],f[2]);
            nya_render::texture::set_default_filter(nya_render::texture::filter_nearest,nya_render::texture::filter_nearest,nya_render::texture::filter_nearest);
            params_tex->build(&params_buf[0], params_tex->get_width(), params_tex->get_height(), params_tex->get_format());
            nya_render::texture::set_default_filter(f[0],f[1],f[2]);
        }

        struct anim
        {
            float duration;
            float inv_duration;
            int layer;
        };

        std::map<uint, anim> anims;
    };

    std::vector<lod> m_lods;

protected:
    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
};

//------------------------------------------------------------
