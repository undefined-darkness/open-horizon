//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/mesh.h"
#include "location_params.h"

namespace renderer
{
//------------------------------------------------------------

class effect_clouds
{
public:
    bool load(const char *location_name, const location_params &params);
    void draw_flat();
    void draw_obj();
    void release();

private:
    typedef unsigned int uint;
    struct bdd_header
    {
        char sign[4];
        uint unknown;

        float unknown2[2];
        uint hiflat_clouds_count;
        float unknown3[2];
        uint type2_count;
        float unknown4[2];
        uint type3_count;
        float unknown5[3];
        uint type4_count;
        float unknown6[2];
        uint obj_clouds_count;
        float params[20];

        uint zero;
    };

    struct bdd
    {
        bdd_header header; //ToDo

        std::vector<nya_math::vec2> hiflat_clouds;
        std::vector<nya_math::vec2> type2_pos;
        std::vector<nya_math::vec2> type3_pos;
        std::vector<nya_math::vec2> type4_pos;
        std::vector<std::pair<int,nya_math::vec2> > obj_clouds;
    };

private:
    bool read_bdd(const char *name, bdd &bdd_res);

    bdd m_clouds;
    
    struct level_header
    {
        uint count;
        float height;
        uint zero;
    };

private:
    nya_render::vbo m_mesh;

    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec4 tc;
        nya_math::vec2 dir;
        nya_math::vec2 size;
    };

private:
    nya_scene::shader m_shader_obj;
    int m_shader_pos;
    nya_scene::texture m_obj_tex;

private:
    struct level_entry
    {
        nya_math::vec3 pos;
        float size;
        float tc[6];
        uint zero;
    };
    
    struct obj_level
    {
        float height;
        std::vector<level_entry> entries;
        
        uint32_t offset;
        uint32_t count;
    };
    
    std::vector<obj_level> m_obj_levels;
    std::vector<std::pair<uint32_t,uint32_t> > m_dist_sort;

private:
    nya_scene::shader m_shader_hi_flat;
    nya_scene::texture m_flat_tex;
    unsigned int m_hi_flat_offset;
    unsigned int m_hi_flat_count;
};

//------------------------------------------------------------
}
