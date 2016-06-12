//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "fhm_mesh.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class model
{
public:
    bool load(const char *name, const location_params &params);
    void draw(int lod_idx) { m_mesh.draw(lod_idx); }
    int get_lods_count() const { return m_mesh.get_lods_count(); }
    void update(int dt) { m_mesh.update(dt); }

    void set_pos(const nya_math::vec3 &pos) { m_mesh.set_pos(pos); }
    void set_rot(const nya_math::quat &rot) { m_mesh.set_rot(rot); }
    const nya_math::vec3 &get_pos() const { return m_mesh.get_pos(); }
    const nya_math::quat &get_rot() const { return m_mesh.get_rot(); }

public:
    void set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time)
    {
        m_mesh.set_relative_anim_time(lod_idx, anim_hash_id, time);
    }

    void set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed)
    {
        m_mesh.set_anim_speed(lod_idx, anim_hash_id, speed);
    }

    float get_relative_anim_time(int lod_idx, unsigned int anim_hash_id)
    {
        return m_mesh.get_relative_anim_time(lod_idx, anim_hash_id);
    }

    void set_anim_weight(int lod_idx, unsigned int anim_hash_id, float weight)
    {
        m_mesh.set_anim_weight(lod_idx, anim_hash_id, weight);
    }

    bool has_anim(int lod_idx, unsigned int anim_hash_id)
    {
        return m_mesh.has_anim(lod_idx, anim_hash_id);
    }

    void set_texture(int lod_idx, const char *semantics, const nya_scene::texture &tex)
    {
        return m_mesh.set_texture(lod_idx, semantics, tex);
    }

    void set_texture(int lod_idx, const char *semantics, const char *file_name)
    {
        return m_mesh.set_texture(lod_idx, semantics, file_name);
    }

    void set_material(int lod_idx, const fhm_materials::material &m, const char *shader)
    {
        return m_mesh.set_material(lod_idx, m, shader);
    }

    int get_bones_count(int lod_idx);
    const char *get_bone_name(int lod_idx, int bone_idx);
    int get_bone_idx(int lod_idx, const char *name);
    int find_bone_idx(int lod_idx, const char *name_part);
    nya_math::vec3 get_bone_pos(int lod_idx, int bone_idx);
    nya_math::quat get_bone_rot(int lod_idx, int bone_idx);
    void set_bone_rot(int lod_idx, int bone_idx, const nya_math::quat &rot);

    nya_scene::mesh &get_mesh(int lod_idx);

public:
    model() {}
    model(const char *name, const location_params &params) { load(name, params); }

private:
    void load_tdp(const std::string &name);

private:
    fhm_mesh m_mesh;
};

//------------------------------------------------------------
}