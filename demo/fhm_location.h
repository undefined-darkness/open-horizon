//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"

//------------------------------------------------------------

class memory_reader;
struct fhm_location_load_data;

//------------------------------------------------------------

class fhm_location
{
    friend class location; //ToDo

public:
    bool load(const char *fileName);
    void update(int dt);
    void draw_col(int col_idx);
    void draw_mptx();
    void draw_landscape();

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

    std::vector<mptx_mesh> mptx_meshes;

    struct col { nya_render::vbo vbo; };
    std::vector<col> cols;

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
    nya_scene::material m_map_parts_material;
    nya_scene::texture_proxy m_map_parts_color_texture;
    nya_scene::texture_proxy m_map_parts_diffuse_texture;
    nya_scene::texture_proxy m_map_parts_specular_texture;
    nya_scene::material::param_array_proxy m_map_parts_tr;

    nya_scene::material m_land_material;
};

//------------------------------------------------------------
