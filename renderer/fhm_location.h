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
    bool load(const char *file_name, const location_params &params);
    void update(int dt);
    void draw_mptx();
    void draw_landscape();
    void draw_cols() { m_debug_draw.draw(); }

protected:
    bool read_mptx(memory_reader &reader);
    bool read_colh(memory_reader &reader);
    bool read_ntxr(memory_reader &reader, fhm_location_load_data &load_data);
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
            float yaw;
        };

        std::vector<instance> instances;
        nya_render::vbo vbo;
        float draw_dist;
        std::vector<uint> textures;

        nya_scene::texture color;

        mptx_mesh(): draw_dist(0.0f) {}
    };

    std::vector<mptx_mesh> m_mptx_meshes;

    struct col_mesh { nya_math::aabb box; };
    std::vector<col_mesh> m_cols;

    struct landscape
    {
        struct group
        {
            uint tex_id;
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
        };

        std::vector<patch> patches;
        nya_render::vbo vbo;

    public:
        float get_height(float x,float y) const;

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

    nya_scene::material m_land_material;

    nya_render::debug_draw m_debug_draw;
};

//------------------------------------------------------------
}