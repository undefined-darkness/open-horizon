//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include "location_params.h"
#include "render/debug_draw.h"

namespace renderer
{
//------------------------------------------------------------

class memory_reader;
struct fhm_location_load_data;

//------------------------------------------------------------

class fhm_location
{
    friend class location; //ToDo

public:
    bool load(const char *file_name, const location_params &params, nya_math::vec3 fog_color);
    bool load_native(const char *name, const location_params &params, nya_math::vec3 fog_color);
    void update(int dt);
    void draw_mptx();
    void draw_mptx_transparent();
    void draw_landscape();
    void draw_trees();
    void draw_cols() { m_debug_draw.draw(); }
    void release();

    const nya_scene::texture_proxy &get_trees_texture() const;

protected:
    bool read_mptx(memory_reader &reader);
    bool read_ntxr(memory_reader &reader, fhm_location_load_data &load_data);
    bool read_wpdc(memory_reader &reader, fhm_location_load_data &load_data);
    bool read_location_tex_indices(memory_reader &reader, fhm_location_load_data &load_data);
    bool read_location_patches(memory_reader &reader, fhm_location_load_data &load_data);
    bool finish_load_location(fhm_location_load_data &load_data);

protected:
    typedef unsigned int uint;
    typedef unsigned short ushort;

    struct mptx_mesh
    {
        struct instance
        {
            nya_math::aabb bbox;
            nya_math::vec3 pos;
            float yaw = 0;
            int group = -1;
        };

        std::vector<instance> instances;
        nya_scene::proxy<nya_render::vbo> vbo;
        float draw_dist;

        struct group
        {
            int offset = 0, count = 0;
            nya_scene::texture diff, spec;
        };

        std::vector<group> groups;

        nya_scene::texture color;

        mptx_mesh(): draw_dist(0.0f) {}
        ~mptx_mesh() { if (vbo.get_ref_count() == 1) vbo->release(); }
    };

    void draw(const std::vector<mptx_mesh> &meshes);

    std::vector<mptx_mesh> m_mptx_meshes;
    std::vector<mptx_mesh> m_mptx_transparent_meshes;

    struct landscape
    {
        struct group
        {
            nya_scene::texture tex;
            nya_math::aabb box;

            uint hi_offset;
            uint hi_count;
            uint mid_offset;
            uint mid_count;
            uint low_offset;
            uint low_count;
        };

        struct patch
        {
            nya_math::aabb box;
            std::vector<group> groups;

            uint tree_offset, tree_count;
        };

        std::vector<patch> patches;
        nya_render::vbo vbo;
        nya_render::vbo tree_vbo;

        void release()
        {
            vbo.release();
            tree_vbo.release();
            heights_width = heights_height = 0;
            heights.clear();
            patches.clear();
        }

    public:
        std::vector<float> heights;
        unsigned int heights_width;
        unsigned int heights_height;

    public:
        landscape(): heights_width(0),heights_height(0) {}

    } m_landscape;

protected:
    nya_scene::material m_map_parts_material;
    nya_scene::texture_proxy m_map_parts_color_texture;
    nya_scene::texture_proxy m_map_parts_diffuse_texture;
    nya_scene::texture_proxy m_map_parts_specular_texture;
    nya_scene::material::param_array_proxy m_map_parts_tr;
    nya_scene::material m_trees_material;
    nya_scene::material::param_proxy m_trees_up;
    nya_scene::material::param_proxy m_trees_right;
    nya_scene::material m_land_material;

    nya_render::debug_draw m_debug_draw;
};

//------------------------------------------------------------
}
