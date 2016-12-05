//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm_location.h"
#include "containers/fhm.h"
#include "mesh_obj.h"

#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "render/render.h"
#include "scene/camera.h"
#include "scene/transform.h"
#include "scene/shader.h"
#include "math/scalar.h"
#include "util/half.h"
#include "util/location.h"
#include "util/xml.h"

#include "render/platform_specific_gl.h"

#include <math.h>
#include <stdint.h>

#include "shared.h"

#ifdef min
#undef min
#undef max
#endif

namespace renderer
{
//------------------------------------------------------------

static const int location_size = 16;

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

struct fhm_location_load_data
{
    unsigned char height_patches[location_size * location_size];
    std::vector<float> heights;

    unsigned char patches[location_size * location_size];
    std::vector<unsigned char> tex_indices_data;

    std::vector<nya_scene::texture> textures;

    unsigned int tree_types_count = 0;
    struct tree_info { unsigned int idx, offset, count = 0; };
    std::vector<tree_info> tree_patches[location_size * location_size];
    std::vector<nya_math::vec2> tree_positions;

    int quad_size = 1024;
    int quad_frags = 8;
    int subquads_per_quad = 8;
    int tile_size = 512;
    int tile_border = 8;
};

//------------------------------------------------------------

bool fhm_location::finish_load_location(fhm_location_load_data &load_data)
{
    assert(!load_data.tex_indices_data.empty());

    auto &p = m_land_material.get_default_pass();
    p.set_shader(nya_scene::shader("shaders/land.nsh"));
    p.get_state().set_cull_face(true);

    auto &p2 = m_trees_material.get_default_pass();
    p2.set_shader(nya_scene::shader("shaders/trees.nsh"));
    p2.get_state().set_cull_face(false);
    nya_scene::texture tree_tex;
    const uint tree_texture_resolution = 256;
    if (load_data.tree_types_count > 0)
        tree_tex.build(0, tree_texture_resolution * load_data.tree_types_count, tree_texture_resolution, nya_render::texture::color_bgra);
    m_trees_material.set_texture("diffuse", tree_tex);

    class vbo_data
    {
    public:
        void add_patch(float x, float y, int tc_idx, const float *h)
        {
            //ToDo: strips
            //ToDo: exclude patches with -9999.0 height
            //ToDo: lod seams

            const int line_count = int(int(m_tex_width) / int(m_tile_size + m_tile_border));

            int ty = tc_idx / line_count;
            int tx = tc_idx - ty * line_count;

            nya_math::vec4 tc(m_tile_border, m_tile_border, m_tile_size, m_tile_size);

            tc.x += (tc.z + tc.x * 2) * tx;
            tc.y += (tc.w + tc.y * 2) * ty;

            tc.z += tc.x;
            tc.w += tc.y;

            tc.x /= m_tex_width;
            tc.y /= m_tex_height;
            tc.z /= m_tex_width;
            tc.w /= m_tex_height;

            vert v;

            const uint hpw = m_quad_frags * m_subquads_per_quad + 1;
            const uint w = m_subquads_per_quad + 1;

            uint voff = (uint)m_verts.size();

            tc.zw() -= tc.xy();
            tc.zw() /= float(m_subquads_per_quad);
            const float hpatch_size = m_quad_size/m_subquads_per_quad;
            for (int hy=0; hy < w; ++hy)
            for (int hx=0; hx < w; ++hx)
            {
                v.pos.x = x + hx * hpatch_size;
                v.pos.y = h[hx + hy * hpw];
                v.pos.z = y + hy * hpatch_size;
                v.tc.x = tc.x + tc.z * hx;
                v.tc.y = tc.y + tc.w * hy;
                m_verts.push_back(v);

                m_min = nya_math::vec3::min(v.pos, m_min);
                m_max = nya_math::vec3::max(v.pos, m_max);
            }

            static std::vector<uint> low;
            low.clear();
            low.push_back(voff);
            low.push_back(voff + w * w - 1);
            low.push_back(voff + w - 1);
            low.push_back((uint)low[0]);
            low.push_back(voff + w * w - w);
            low.push_back((uint)low[1]);

            m_curr_indices_low.insert(m_curr_indices_low.end(), low.begin(), low.end());

            float same_height = m_verts[voff].pos.y;
            bool all_same_height = true;
            for (int i = 1; i < w * w; ++i)
            {
                if (fabsf(m_verts[voff + i].pos.y - same_height) > 0.01f)
                {
                    all_same_height = false;
                    break;
                }
            }

            if (all_same_height)
            {
                m_curr_indices_hi.insert(m_curr_indices_hi.end(), low.begin(), low.end());
                m_curr_indices_mid.insert(m_curr_indices_mid.end(), low.begin(), low.end());
                return;
            }

            for (int hy=0; hy < m_subquads_per_quad; ++hy)
            for (int hx=0; hx < m_subquads_per_quad; ++hx)
            {
                int i = voff + hx + hy * w;

                m_curr_indices_hi.push_back(i);
                m_curr_indices_hi.push_back(i + 1 + m_subquads_per_quad);
                m_curr_indices_hi.push_back(i + 1);
                m_curr_indices_hi.push_back(i + 1);
                m_curr_indices_hi.push_back(i + 1 + m_subquads_per_quad);
                m_curr_indices_hi.push_back(i + 1 + m_subquads_per_quad + 1);
            }

            if (m_subquads_per_quad <= 2)
                return;

            for (int hy=0; hy < m_subquads_per_quad; hy += 2)
            for (int hx=0; hx < m_subquads_per_quad; hx += 2)
            {
                int i = voff + hx + hy * w;

                m_curr_indices_mid.push_back(i);
                m_curr_indices_mid.push_back(i + 2 * (1 + m_subquads_per_quad));
                m_curr_indices_mid.push_back(i + 2);
                m_curr_indices_mid.push_back(i + 2);
                m_curr_indices_mid.push_back(i + 2 * (1 + m_subquads_per_quad));
                m_curr_indices_mid.push_back(i + 2 * (1 + m_subquads_per_quad + 1));
            }
        }

        typedef unsigned int uint;

        uint get_group_hi_offset() { return (uint)m_indices.size(); }
        uint get_group_hi_count() { return (uint)m_curr_indices_hi.size(); }
        uint get_group_mid_offset() { return (uint)(m_indices.size() + m_curr_indices_hi.size()); }
        uint get_group_mid_count() { return m_subquads_per_quad > 2 ? (uint)m_curr_indices_mid.size() : 0; }
        uint get_group_low_offset() { return (uint)(m_indices.size() + m_curr_indices_hi.size() + m_curr_indices_mid.size()); }
        uint get_group_low_count() { return m_subquads_per_quad > 2 ? (uint)m_curr_indices_low.size() : 0; }

        nya_math::aabb get_aabb() { return nya_math::aabb(m_min, m_max); }

        void end_group()
        {
            m_indices.insert(m_indices.end(), m_curr_indices_hi.begin(), m_curr_indices_hi.end());
            m_indices.insert(m_indices.end(), m_curr_indices_mid.begin(), m_curr_indices_mid.end());
            m_indices.insert(m_indices.end(), m_curr_indices_low.begin(), m_curr_indices_low.end());

            m_curr_indices_hi.clear();
            m_curr_indices_mid.clear();
            m_curr_indices_low.clear();

            m_min = nya_math::vec3(1.0, 1.0, 1.0) * 1000000.0f;
            m_max = -m_min;
        }

        void *get_vdata() { return m_verts.empty() ? 0 : &m_verts[0]; }
        uint get_vcount() { return (uint)m_verts.size(); }
        void *get_idata() { return m_indices.empty() ? 0 : &m_indices[0]; }
        uint get_icount() { return (uint)m_indices.size(); }

        void set_tex_size(int width, int height) { m_tex_width = width; m_tex_height = height; }
        void set_patch_size(int quad_size, int quad_frags, int subquads_per_quad)
        {
            m_quad_size = quad_size;
            m_quad_frags = quad_frags;
            m_subquads_per_quad = subquads_per_quad;
        }
        void set_tex_patch_size(int border, int size) { m_tile_border = border, m_tile_size = size; }

    private:
        float m_tex_width;
        float m_tex_height;
        float m_tile_border;
        float m_tile_size;
        int m_quad_size;
        int m_quad_frags;
        int m_subquads_per_quad;

        struct vert
        {
            nya_math::vec3 pos;
            nya_math::vec2 tc;
        };

        std::vector<vert> m_verts;
        std::vector<uint> m_indices;

        std::vector<uint> m_curr_indices_hi;
        std::vector<uint> m_curr_indices_mid;
        std::vector<uint> m_curr_indices_low;

        nya_math::vec3 m_min;
        nya_math::vec3 m_max;
    };

    m_landscape.patches.clear();
    m_landscape.patches.resize(location_size * location_size);

    assert(!load_data.textures.empty());

    vbo_data vdata; //ToDo
    vdata.end_group();
    vdata.set_patch_size(load_data.quad_size, load_data.quad_frags, load_data.subquads_per_quad);
    vdata.set_tex_patch_size(load_data.tile_border, load_data.tile_size);

    for (int py = 0; py < location_size; ++py)
    for (int px = 0; px < location_size; ++px)
    {
        const int idx = py * location_size + px;
        landscape::patch &p = m_landscape.patches[idx];

        int tc_idx = load_data.patches[idx] * load_data.quad_frags * load_data.quad_frags;
        if (tc_idx < 0)
            continue;

        const float base_x = load_data.quad_size * load_data.quad_frags * (px - location_size/2);
        const float base_y = load_data.quad_size * load_data.quad_frags * (py - location_size/2);

        const int hpw = load_data.quad_frags * load_data.subquads_per_quad + 1;
        const int h_idx = load_data.height_patches[idx] * hpw * hpw;

        assert(h_idx + hpw * hpw <= load_data.heights.size());

        int last_tex_idx = -1;
        for (int y = 0; y < load_data.quad_frags; ++y)
        for (int x = 0; x < load_data.quad_frags; ++x)
        {
            const int ind = tc_idx * 2 + 1;
            assert(ind < (int)load_data.tex_indices_data.size());
            const int tex_idx = load_data.tex_indices_data[ind];

            if (tex_idx != last_tex_idx)
            {
                if (last_tex_idx >= 0)
                {
                    auto &g = p.groups.back();
                    g.box = vdata.get_aabb();
                    g.hi_count = vdata.get_group_hi_count();
                    g.hi_offset = vdata.get_group_hi_offset();
                    g.mid_count = vdata.get_group_mid_count();
                    g.mid_offset = vdata.get_group_mid_offset();
                    g.low_count = vdata.get_group_low_count();
                    g.low_offset = vdata.get_group_low_offset();
                    vdata.end_group();
                }

                p.groups.resize(p.groups.size() + 1);
                auto &g = p.groups.back();
                g.tex = load_data.textures[tex_idx];
                vdata.set_tex_size(g.tex.get_width(), g.tex.get_height());

                last_tex_idx = tex_idx;
            }

            const float *h = &load_data.heights[h_idx + (x + y * hpw) * load_data.subquads_per_quad];

            const float pos_x = base_x + load_data.quad_size * x;
            const float pos_y = base_y + load_data.quad_size * y;

            vdata.add_patch(pos_x, pos_y, load_data.tex_indices_data[tc_idx * 2], h);

            ++tc_idx;
        }

        if (!p.groups.empty())
        {
            auto &g = p.groups.back();
            g.box = vdata.get_aabb();
            g.hi_count = vdata.get_group_hi_count();
            g.hi_offset = vdata.get_group_hi_offset();
            g.mid_count = vdata.get_group_mid_count();
            g.mid_offset = vdata.get_group_mid_offset();
            g.low_count = vdata.get_group_low_count();
            g.low_offset = vdata.get_group_low_offset();
            vdata.end_group();
        }

        if (!p.groups.empty())
        {
            nya_math::vec3 box_min = nya_math::vec3(1.0, 1.0, 1.0) * 1000000.0f;
            nya_math::vec3 box_max = -box_min;

            for (auto &g: p.groups)
            {
                box_min = nya_math::vec3::min(g.box.origin - g.box.delta, box_min);
                box_max = nya_math::vec3::max(g.box.origin + g.box.delta, box_max);
            }

            p.box = nya_math::aabb(box_min, box_max);
        }
    }

    if (!m_landscape.vbo.is_valid())
        m_landscape.vbo.create();

    m_landscape.vbo->set_vertex_data(vdata.get_vdata(), 5 * 4, vdata.get_vcount());
    m_landscape.vbo->set_index_data(vdata.get_idata(), nya_render::vbo::index4b, vdata.get_icount());
    m_landscape.vbo->set_tc(0, 3 * 4, 2);

    //------------------------------------

    if (!m_map_parts_color_texture.is_valid())
    {
        auto &p = m_map_parts_material.get_default_pass();
        p.set_shader("shaders/map_parts.nsh");
        p.get_state().set_cull_face(true);

        m_map_parts_color_texture.create();
        m_map_parts_material.set_texture("color", m_map_parts_color_texture);
        m_map_parts_diffuse_texture.create();
        m_map_parts_material.set_texture("diffuse", m_map_parts_diffuse_texture);
        m_map_parts_specular_texture.create();
        m_map_parts_material.set_texture("specular", m_map_parts_specular_texture);
        m_map_parts_tr.create();
        m_map_parts_material.set_param_array(m_map_parts_material.get_param_idx("transform"), m_map_parts_tr);
    }

    //------------------------------------

    uint trees_count = 0;

    for (int py = 0; py < location_size; ++py)
    for (int px = 0; px < location_size; ++px)
    {
        const int idx = py * location_size + px;
        auto &tp = load_data.tree_patches[idx];
        for (auto &p: tp)
            trees_count += p.count;
    }

    if (!trees_count)
        return true;

    struct tree_vert { nya_math::vec3 pos; uint16_t tc[2], delta[2]; };
    nya_memory::tmp_buffer_scoped tree_buf(trees_count * 4 * sizeof(tree_vert));
    tree_vert *curr_tree_vert = (tree_vert *)tree_buf.get_data();
    uint curr_tree_idx_off = 0;

    const float tree_tc_width = 1.0f / load_data.tree_types_count;

    for (int py = 0; py < location_size; ++py)
    for (int px = 0; px < location_size; ++px)
    {
        const int idx = py * location_size + px;
        landscape::patch &p = m_landscape.patches[idx];

        const int hpw = load_data.quad_frags * load_data.quad_frags + 1;
        const int h_idx = load_data.height_patches[idx] * hpw * hpw;

        const float base_x = load_data.quad_size * load_data.quad_frags * (px - location_size/2);
        const float base_y = load_data.quad_size * load_data.quad_frags * (py - location_size/2);

        p.tree_offset = curr_tree_idx_off;
        p.tree_count = 0;
        auto &tp = load_data.tree_patches[idx];
        for (auto &tpp: tp)
        {
            for (uint i = tpp.offset; i < tpp.offset + tpp.count; ++i)
            {
                auto &to = load_data.tree_positions[i];

                const float half_size = 32.0f * 0.5f; //ToDo

                nya_math::vec3 pos;
                pos.x = base_x + to.x;
                pos.z = base_y + to.y;

                //ToDo
                {
                    const int hpatch_size = load_data.quad_size / load_data.subquads_per_quad;

                    const int base = location_size/2 * load_data.quad_size * load_data.quad_frags;

                    const int idx_x = int(pos.x + base) / hpatch_size;
                    const int idx_z = int(pos.z + base) / hpatch_size;

                    const int tmp_idx_x = idx_x / load_data.subquads_per_quad;
                    const int tmp_idx_z = idx_z / load_data.subquads_per_quad;

                    const int qidx_x = tmp_idx_x - px * load_data.subquads_per_quad;
                    const int qidx_z = tmp_idx_z - py * load_data.subquads_per_quad;

                    const float *h = &load_data.heights[h_idx+(qidx_x + qidx_z * hpw)*load_data.quad_frags];

                    const uint hhpw = load_data.subquads_per_quad * load_data.subquads_per_quad + 1;

                    const int hidx_x = idx_x - tmp_idx_x * load_data.subquads_per_quad;
                    const int hidx_z = idx_z - tmp_idx_z * load_data.subquads_per_quad;

                    const float kx = (pos.x + base) / hpatch_size - idx_x;
                    const float kz = (pos.z + base) / hpatch_size - idx_z;

                    const float h00 = h[hidx_x + hidx_z * hhpw];
                    const float h10 = h[hidx_x + 1 + hidx_z * hhpw];
                    const float h01 = h[hidx_x + (hidx_z + 1) * hhpw];
                    const float h11 = h[hidx_x + 1 + (hidx_z + 1) * hhpw];

                    const float h00_h10 = nya_math::lerp(h00, h10, kx);
                    const float h01_h11 = nya_math::lerp(h01, h11, kx);

                    pos.y = nya_math::lerp(h00_h10, h01_h11, kz);
                }

                //pos.y = get_height(pos.x, pos.z); //ToDo
                pos.y += half_size;

                for (int j = 0; j < 4; ++j)
                {
                    curr_tree_vert->pos = pos;

                    static const float deltas[4][2] = { {-1.0f, -1.0f}, {-1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, -1.0f} };

                    curr_tree_vert->tc[0] = Float16Compressor::compress((0.5f * (deltas[j][0] + 1.0f) + tpp.idx) * tree_tc_width);
                    curr_tree_vert->tc[1] = Float16Compressor::compress(0.5f * (deltas[j][1] + 1.0f));

                    curr_tree_vert->delta[0] = Float16Compressor::compress(deltas[j][0] * half_size);
                    curr_tree_vert->delta[1] = Float16Compressor::compress(deltas[j][1] * half_size);

                    ++curr_tree_vert;
                }

                p.tree_count += 6;
            }
        }

        curr_tree_idx_off += p.tree_count;
    }

    if (!m_landscape.tree_vbo.is_valid())
        m_landscape.tree_vbo.create();

    m_landscape.tree_vbo->set_vertex_data(tree_buf.get_data(), (uint)sizeof(tree_vert), trees_count * 4);
    m_landscape.tree_vbo->set_tc(0, 3 * 4, 4, nya_render::vbo::float16);

    assert(sizeof(tree_vert) * 4 >= sizeof(uint) * 6);
    uint *tree_indices = (uint *)tree_buf.get_data();

    for (uint i = 0, v = 0; i < trees_count * 6; i += 6, v+=4)
    {
        tree_indices[i] = v;
        tree_indices[i + 1] = v + 1;
        tree_indices[i + 2] = v + 2;
        tree_indices[i + 3] = v;
        tree_indices[i + 4] = v + 2;
        tree_indices[i + 5] = v + 3;
    }

    m_landscape.tree_vbo->set_index_data(tree_buf.get_data(), nya_render::vbo::index4b, trees_count * 6);

    return true;
}

//------------------------------------------------------------

bool fhm_location::load(const char *fileName, const location_params &params, nya_math::vec3 fcolor)
{
    fhm_file fhm;
    if (!fhm.open(fileName))
        return false;

    const bool is_location = strncmp(fileName, "Map/ms", 6) == 0 && strstr(fileName, "_") == 0;

    fhm_location_load_data location_load_data;

    bool has_mptx = false;

    for (int j = 0; j < fhm.get_chunks_count(); ++j)
    {
        const uint sign = fhm.get_chunk_type(j);
        if (sign == 'HLOC')
            continue;

        nya_memory::tmp_buffer_scoped buf(fhm.get_chunk_size(j));
        fhm.read_chunk_data(j, buf.get_data());
        memory_reader reader(buf.get_data(), fhm.get_chunk_size(j));

        if (sign == 'RXTN') //NTXR texture
        {
            read_ntxr(reader, location_load_data);
        }
        else if (sign == 'xtpm') //mptx
        {
            read_mptx(reader);
            has_mptx = true;
        }
        else if (sign == 'cdpw')
        {
            read_wpdc(reader, location_load_data);
        }
        else if (sign == 'clde')//edlc
        {
            //test.add_line(nya_math::vec3(8192.0, 0.0, 0.0), nya_math::vec3(0.0, 0.0, 0.0), nya_math::vec4(1.0, 0.0, 0.0, 1.0));

/*
            reader.seek(4128);
            for (int i = 0; i < 11; ++i)
            {
                struct unknown
                {
                    nya_math::vec3 pos1;
                    uint hz3[5];
                };

                unknown u = reader.read<unknown>();
                get_debug_draw().add_point(u.pos1, nya_math::vec4(1.0, 0.0, 0.0, 1.0));
                get_debug_draw().add_line(u.pos1, u.pos1+nya_math::vec3(0.0, 1000.0, 0.0), nya_math::vec4(1.0, 0.0, 0.0, 1.0));
            }
*/
            //printf("chunk%d offset %d\n", j, ch.offset+48);

        }
        else if (sign == '2TBS')//SBT2
        {
            /*
            reader.seek(44);
            uint off = reader.read<uint>();
            reader.seek(off);
            for (int i = 0; i < 8747; ++i)
            {
                struct unknown
                {
                    uint hz0[4];
                    nya_math::vec3 pos1;
                    float hz1;
                    nya_math::vec3 pos2;
                    float hz2;
                    uint hz3[8];
                };

                unknown u = reader.read<unknown>();
                get_debug_draw().add_point(u.pos1, nya_math::vec4(1.0, 0.0, 0.0, 1.0));
                get_debug_draw().add_point(u.pos2, nya_math::vec4(0.0, 0.0, 1.0, 1.0));
                get_debug_draw().add_line(u.pos1, u.pos2);
            }
            */
        }
        else if ( is_location )
        {
            if (j == 4)
            {
                assert(reader.get_remained() == location_size*location_size);
                memcpy(location_load_data.height_patches, reader.get_data(), reader.get_remained());
            }
            else if (j == 5)
            {
                assert(reader.get_remained());
                location_load_data.heights.resize(reader.get_remained()/4);
                memcpy(&location_load_data.heights[0], reader.get_data(), reader.get_remained());
            }
            else if (j == 8)
            {
                assert(reader.get_remained() == location_size*location_size);
                memcpy(location_load_data.patches, reader.get_data(), reader.get_remained());
            }
            else if (j == 9)
            {
                assert(reader.get_remained());
                location_load_data.tex_indices_data.resize(reader.get_remained());
                memcpy(&location_load_data.tex_indices_data[0], reader.get_data(), reader.get_remained());
            }
            else if (reader.get_remained() > 0)
            {
                //char fname[255]; sprintf(fname, "out/chunk%d.txt", j); print_data(reader, 0, reader.get_remained(), 0, fname);
            }
        }
        else
        {
            //read_unknown(reader);

            //char fname[255]; sprintf(fname, "chunk%d.txt", j); print_data(reader, 0, 2000000, 0, fname);

            //read_unknown(reader);
            //int i = 5;

            //printf("chunk%d offset %ld\n", j, fhm.get_chunk_offset(j));
        }
    }

    if (is_location)
        finish_load_location(location_load_data);

    auto &s = params.sky.mapspecular;
    auto &d = params.detail;

    nya_scene::material::param light_dir(-params.sky.sun_dir, 0.0f);
    nya_scene::material::param fog_color(fcolor.x, fcolor.y, fcolor.z, -0.01*params.sky.fog_density);
    nya_scene::material::param fog_height(params.sky.fog_height_fresnel, params.sky.fog_height,
                                          -0.01 * params.sky.fog_height_fade_density, -0.01 * params.sky.fog_height_density);
    nya_scene::material::param map_param_vs(s.parts_power, 0, 0, 0);
    nya_scene::material::param map_param_ps(s.parts_scale, s.parts_fog_power, s.parts_fresnel_max, s.parts_fresnel);
    nya_scene::material::param map_param2_ps(d.mesh_range, d.mesh_power, d.mesh_repeat, s.parts_reflection_power);

    if (has_mptx)
    {
        auto &m = m_map_parts_material;

        m.set_param(m.get_param_idx("light dir"), light_dir);
        m.set_param(m.get_param_idx("fog color"), fog_color);
        m.set_param(m.get_param_idx("fog height"), fog_height);
        m.set_param(m.get_param_idx("map param vs"), map_param_vs);
        m.set_param(m.get_param_idx("map param ps"), map_param_ps);
        m.set_param(m.get_param_idx("map param2 ps"), map_param2_ps);
        m.set_param(m.get_param_idx("specular color"), s.parts_color.x, s.parts_color.y, s.parts_color.z, s.parts_contrast);
    }

    {
        auto &m = m_trees_material;
        m.set_param(m.get_param_idx("fog color"), fog_color);
        m.set_param(m.get_param_idx("fog height"), fog_height);
    }

    auto &m = m_land_material;
    m.set_param(m.get_param_idx("light dir"), light_dir);
    m.set_param(m.get_param_idx("fog color"), fog_color);
    m.set_param(m.get_param_idx("fog height"), fog_height);
/*
    if (m_cols.size() == m_mptx_meshes.size())
    {
        m_debug_draw.clear();
        for (size_t i = 0; i < m_mptx_meshes.size(); ++i)
        {
            for (auto inst: m_mptx_meshes[i].instances)
            {
                auto b = m_cols[i].box;
                auto q = nya_math::quat(0.0, inst.yaw, 0.0);

                nya_math::vec3 verts[4];
                verts[0] = b.origin + b.delta;
                b.delta.x = -b.delta.x;
                verts[1] = b.origin + b.delta;
                b.delta.z = -b.delta.z;
                verts[2] = b.origin + b.delta;
                b.delta.x = -b.delta.x;
                verts[3] = b.origin + b.delta;

                for (auto &v : verts)
                    v = inst.pos + q.rotate(v);

                for (int j = 0; j < 4; ++j)
                {
                    b.delta.x = b.delta.z = 0.0f;

                    m_debug_draw.add_line(verts[j], verts[(j+1) % 4], nya_math::vec4(0.0, 1.0, 0.0, 1.0));
                    m_debug_draw.add_line(verts[j], verts[j] - b.delta * 2.0, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
                }
            }
        }
    }
*/
    fhm.close();
    return true;
}

//------------------------------------------------------------

bool fhm_location::load_native(const char *name, const location_params &params, nya_math::vec3 fcolor)
{
    fhm_location_load_data location_load_data;

    auto &zip = get_native_location_provider(name);

    pugi::xml_document doc;
    if (!load_xml(zip.access("info.xml"), doc))
        return false;

    auto tiles = doc.first_child().child("tiles");
    location_load_data.quad_size = tiles.attribute("quad_size").as_int();
    location_load_data.quad_frags = tiles.attribute("quad_frags").as_int();
    location_load_data.subquads_per_quad = tiles.attribute("subfrags").as_int();
    location_load_data.tile_size = tiles.attribute("frag_size").as_int();
    location_load_data.tile_border = tiles.attribute("frag_border").as_int();

    const int tiles_tex_count = tiles.attribute("tex_count").as_int();
    location_load_data.textures.resize(tiles_tex_count);
    for (int i = 0; i < tiles_tex_count; ++i)
    {
        char name[255];
        sprintf(name, "land%03d.tga", i);
        auto data = load_resource(zip.access(name));

        nya_scene::shared_texture stex;
        if (nya_scene::texture::load_tga(stex, data, name))
            location_load_data.textures[i].create(stex);

        data.free();
    }

    auto tc_off = load_resource(zip.access("tex_offsets.bin"));
    tc_off.copy_to(location_load_data.patches, sizeof(location_load_data.patches));
    tc_off.free();

    auto tc_ind = load_resource(zip.access("tex_indices.bin"));
    location_load_data.tex_indices_data.resize(tc_ind.get_size());
    tc_ind.copy_to(location_load_data.tex_indices_data.data(), tc_ind.get_size());
    tc_ind.free();

    auto height_off = load_resource(zip.access("height_offsets.bin"));
    height_off.copy_to(location_load_data.height_patches, sizeof(location_load_data.height_patches));
    height_off.free();

    auto heights = load_resource(zip.access("heights.bin"));
    std::string format = doc.first_child().child("heightmap").attribute("format").as_string();
    if (format == "byte")
    {
        location_load_data.heights.resize(heights.get_size());
        auto hdata = (unsigned char *)heights.get_data();
        const float hscale = doc.first_child().child("heightmap").attribute("scale").as_float(1.0f);
        for (size_t i = 0; i < location_load_data.heights.size(); ++i)
            location_load_data.heights[i] = hdata[i] * hscale;
    }
    else if (format == "float")
    {
        location_load_data.heights.resize(heights.get_size()/4);
        heights.copy_to(location_load_data.heights.data(), heights.get_size());
    }
    heights.free();

    if (load_xml(zip.access("objects.xml"), doc))
    {
        std::map<std::string, nya_scene::texture> textures;
        std::vector<std::string> loaded;
        std::vector<nya_math::aabb> boxes;
        for (auto o = doc.first_child().child("object"); o; o = o.next_sibling())
        {
            nya_math::vec3 pos;
            pos.x = o.attribute("x").as_float();
            pos.y = o.attribute("y").as_float();
            pos.z = o.attribute("z").as_float();
            std::string file = o.attribute("file").as_string();

            const bool transparent = file.find("_transparent") != std::string::npos;
            auto &meshes = transparent ? m_mptx_transparent_meshes : m_mptx_meshes;

            auto l = std::find(loaded.begin(), loaded.end(), file);
            if (l != loaded.end())
            {
                auto idx = l - loaded.begin();

                meshes[idx].instances.resize( meshes[idx].instances.size() + 1);
                auto &i = meshes[idx].instances.back();

                i.bbox = boxes[idx];
                i.bbox.origin += pos;
                i.pos = pos;
                continue;
            }

            loaded.push_back(file);
            meshes.resize(meshes.size() + 1);
            boxes.resize(boxes.size()+1);

            auto &m = meshes.back();
            auto &b = boxes.back();

            mesh_obj obj;
            if (obj.load(file.c_str(), zip))
            {
                if (obj.groups.empty())
                    continue;

                auto fmax = std::numeric_limits<float>::max();
                auto fmin = std::numeric_limits<float>::lowest();
                nya_math::vec3 vmin(fmax,fmax,fmax), vmax(fmin,fmin,fmin);
                for (auto &v: obj.verts)
                {
                    vmin = nya_math::vec3::min(vmin, v.pos);
                    vmax = nya_math::vec3::max(vmax, v.pos);
                }

                b = nya_math::aabb(vmin, vmax);

                struct vert
                {
                    nya_math::vec3 pos;
                    nya_math::vec3 normal;
                    nya_math::vec2 tc;
                    float color_coord;
                };

                std::vector<vert> verts(obj.verts.size());
                std::vector<nya_math::vec4> colors(verts.size());
                for (size_t i = 0; i < verts.size(); ++i)
                {
                    verts[i].pos = obj.verts[i].pos;
                    verts[i].normal = obj.verts[i].normal;
                    verts[i].tc = obj.verts[i].tc;
                    verts[i].color_coord = (float(i) + 0.5f)/ colors.size();
                    colors[i] = obj.verts[i].color;
                }

                m.vbo.create();
                m.vbo->set_vertex_data(verts.data(), (int)sizeof(vert), (int)verts.size());
                m.vbo->set_normals(3*4);
                m.vbo->set_tc(0, (3+3)*4, 3);

                m.color.build(colors.data(), (int)colors.size(), 1, nya_render::texture::color_rgba32f);

                m.groups.resize(obj.groups.size());
                for (size_t i = 0; i < m.groups.size(); ++i)
                {
                    auto &f = obj.groups[i];
                    auto &t = m.groups[i];

                    t.offset = f.offset;
                    t.count = f.count;

                    std::string tex_name = obj.materials[f.mat_idx].tex_diffuse;
                    auto it = textures.find(tex_name);
                    if (it == textures.end())
                    {
                        nya_scene::resource_data data = load_resource(zip.access(tex_name.c_str()));
                        nya_scene::shared_texture res;
                        if (nya_scene::texture::load_tga(res, data, tex_name.c_str()))
                            textures[tex_name].create(res);
                        data.free();
                        it = textures.find(tex_name);
                    }

                    t.diff = (it == textures.end()) ? shared::get_white_texture() : it->second;
                    t.spec = shared::get_black_texture();
                }
            }

            m.instances.resize(1);
            auto &i = m.instances.back();
            i.bbox = b;
            i.bbox.origin += pos;
            i.pos = pos;

            m.draw_dist = b.delta.length() * 200.0f;
            m.draw_dist *= m.draw_dist;
        }
    }

    finish_load_location(location_load_data);

    nya_scene::material::param light_dir(-params.sky.sun_dir, 0.0f);
    nya_scene::material::param fog_color(fcolor.x, fcolor.y, fcolor.z, -0.01*params.sky.fog_density);
    nya_scene::material::param fog_height(params.sky.fog_height_fresnel, params.sky.fog_height,
                                          -0.01 * params.sky.fog_height_fade_density, -0.01 * params.sky.fog_height_density);
    auto &m = m_land_material;
    m.set_param(m.get_param_idx("light dir"), light_dir);
    m.set_param(m.get_param_idx("fog color"), fog_color);
    m.set_param(m.get_param_idx("fog height"), fog_height);

    return true;
}

//------------------------------------------------------------

void fhm_location::draw(const std::vector<mptx_mesh> &meshes)
{
    const nya_math::frustum &f = nya_scene::get_camera().get_frustum();
    const nya_math::vec3 &cp = nya_scene::get_camera().get_pos();

    nya_scene::transform::set(nya_scene::transform());

    bool mat_unset = false;
    for (auto &mesh: meshes)
    {
        m_map_parts_color_texture.set(mesh.color);
        auto &vbo = mesh.vbo.get();

        for (auto &g: mesh.groups)
        {
            m_map_parts_diffuse_texture.set(g.diff);
            m_map_parts_specular_texture.set(g.spec);

            m_map_parts_tr->set_count(0);
            for (size_t i = 0; i < mesh.instances.size(); ++i)
            {
                const auto &instance = mesh.instances[i];

                if ((instance.pos - cp).length_sq() > mesh.draw_dist)
                    continue;

                if (!f.test_intersect(instance.bbox))
                    continue;

                int idx = m_map_parts_tr->get_count();
                if (idx >= 127) //limited to 500 in shader uniforms, limited to 127 because of ati max instances per draw limitations
                {
                    mat_unset = true;
                    m_map_parts_material.internal().set();
                    vbo.bind();
                    vbo.draw(g.offset, g.count, vbo.get_element_type(), idx);
                    vbo.unbind();
                    idx = 0;
                }

                m_map_parts_tr->set_count(idx + 1);

                const float color_coord = (float(i) + 0.5f)/mesh.instances.size();

                const float pckd = floorf(instance.yaw*512.0f) + color_coord;
                m_map_parts_tr->set(idx, instance.pos.x, instance.pos.y, instance.pos.z, pckd);
            }

            int instances = m_map_parts_tr->get_count();
            if (instances > 0)
            {
                mat_unset = true;
                m_map_parts_material.internal().set(nya_scene::material::default_pass);
                vbo.bind();
                vbo.draw(g.offset, g.count, vbo.get_element_type(), instances);
                vbo.unbind();
            }
        }
    }

    if (mat_unset)
        m_map_parts_material.internal().unset();
}

//------------------------------------------------------------

void fhm_location::update(int dt)
{
    const int anim_time_interval[]={51731,73164,62695,84235};
    static int anim_time[]={124,6363,36262,47392};
    float t[4];
    for (int i=0;i<4;++i)
    {
        anim_time[i]+=dt;
        if (anim_time[i]>anim_time_interval[i])
            anim_time[i]=0;

        t[i]=float(anim_time[i])/anim_time_interval[i];
    }

    m_land_material.set_param(m_land_material.get_param_idx("anim"), t[0],t[1],-t[2],-t[3]);
}

//------------------------------------------------------------

void fhm_location::draw_mptx()
{
    auto &p = m_map_parts_material.get_default_pass();
    p.get_state().set_blend(false);
    p.get_state().zwrite = true;
    draw(m_mptx_meshes);
}

//------------------------------------------------------------

void fhm_location::draw_mptx_transparent()
{
    auto &p = m_map_parts_material.get_default_pass();
    p.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha); //ToDo: different blend modes
    p.get_state().zwrite = false; //ToDo: depend on material flags, apply to non-transparent meshes too
    draw(m_mptx_transparent_meshes);
}

//------------------------------------------------------------

void fhm_location::draw_trees()
{
    if (!m_landscape.tree_vbo.is_valid())
        return;

    const float tree_draw_distance = 7000;

    static nya_scene::transform t;
    nya_scene::transform::set(t);

    auto &c = nya_scene::get_camera();
    nya_render::set_modelview_matrix(c.get_view_matrix());
    m_trees_material.internal().set();

    m_landscape.tree_vbo->bind();
    for (const auto &p: m_landscape.patches)
    {
        if (!c.get_frustum().test_intersect(p.box))
            continue;

        const float sq = p.box.sq_dist(c.get_pos());
        if (sq > tree_draw_distance * tree_draw_distance)
            continue;

        m_landscape.tree_vbo->draw(p.tree_offset, p.tree_count);
    }
    nya_render::vbo::unbind();
    m_trees_material.internal().unset();
}

//------------------------------------------------------------

const nya_scene::texture_proxy &fhm_location::get_trees_texture() const
{
    return m_trees_material.get_texture("diffuse");
}

//------------------------------------------------------------

void fhm_location::draw_landscape()
{
    if (!m_landscape.vbo.is_valid())
        return;

    auto &c = nya_scene::get_camera();
    nya_render::set_modelview_matrix(c.get_view_matrix());
    m_land_material.internal().set();

    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    m_landscape.vbo->bind();
    for (const auto &p: m_landscape.patches)
    {
        if (!c.get_frustum().test_intersect(p.box))
            continue;

        for (const auto &g: p.groups)
        {
            if (!c.get_frustum().test_intersect(g.box))
                continue;

            const float sq = g.box.sq_dist(c.get_pos());
            g.tex.internal().set();

            if (g.mid_count > 0)
            {
                const float hi_dist = 10000.0f;
                const float mid_dist = 15000.0f;
                if (sq < hi_dist * hi_dist)
                    m_landscape.vbo->draw(g.hi_offset, g.hi_count);
                else if (sq < mid_dist * mid_dist)
                    m_landscape.vbo->draw(g.mid_offset, g.mid_count);
                else
                    m_landscape.vbo->draw(g.low_offset, g.low_count);
            }
            else
                m_landscape.vbo->draw(g.hi_offset, g.hi_count);

            g.tex.internal().unset();
        }
    }

    //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    nya_render::vbo::unbind();
    m_land_material.internal().unset();
}

//------------------------------------------------------------

bool fhm_location::read_ntxr(memory_reader &reader, fhm_location_load_data &load_data)
{
    uint r = shared::load_texture(reader.get_data(), reader.get_remained());
    if (r > 1000000000) //ToDo //probably there is another way
    {
        auto rt = shared::get_texture(r);
        if (!rt.internal().get_shared_data().is_valid())
            return false;

        //for (auto &t: load_data.textures) if (t.internal().get_shared_data().const_get() == rt.internal().get_shared_data().const_get()) return true;
        load_data.textures.push_back(rt);
    }

    return r>0;
}

//------------------------------------------------------------

bool fhm_location::read_wpdc(memory_reader &reader, fhm_location_load_data &load_data)
{
    struct wpdc_header
    {
        char sign[4];
        char version[4];
        uint unknown;
        uint unknown2;
        uint unknown3;
        float width;
        float height;
        uint zero;
    };

    auto header = reader.read<wpdc_header>();
    uint offsets[location_size * location_size];
    for (auto &o: offsets)
        o = reader.read<uint>();

    uint curr_patch = 0;
    std::map<uint, uint> cached;

    for (auto &o: offsets)
    {
        auto c = cached.find(o);
        if (c != cached.end())
        {
            load_data.tree_patches[curr_patch++] = load_data.tree_patches[c->second];
            continue;
        }

        reader.seek(o);
        for (uint i = 0; i < 16; ++i)
        {
            reader.seek(o + i * 8);
            const uint off = reader.read<uint>();
            const uint count = reader.read<uint>();

            if (!count)
                continue;

            if (i + 1 > load_data.tree_types_count)
                load_data.tree_types_count = i + 1;

            auto &ps = load_data.tree_patches[curr_patch];
            ps.resize(ps.size() + 1);
            auto &p = ps.back();
            p.idx = i;
            p.offset = (uint)load_data.tree_positions.size();
            p.count  = count;

            reader.seek(o + 16 * 8 + off);
            for (uint j = 0; j < count; ++j)
            {
                nya_math::vec2 pos;
                pos.x = (reader.read<short>() / 65535.0f + 0.5) * header.width;
                pos.y = (reader.read<short>() / 65535.0f + 0.5) * header.height;
                load_data.tree_positions.push_back(pos);
            }
        }

        cached[o] = curr_patch++;
    }

    return true;
}

//------------------------------------------------------------

bool fhm_location::read_mptx(memory_reader &reader)
{
    struct tex_header
    {
        uint id;
        uint unknown_zero[2];
        uint unknown;
        uint unknown2;
        uint unknown_zero2;
    };

    struct mptx_header
    {
        char sign[4]; //"mptx"
        char version[4];
        ushort unknown[2];
        uint zero;

        ushort zero2;
        ushort tex_count;

        ushort zero3;
        ushort unknown2[3];
        uint zero4[3];

        tex_header tex0;
        tex_header tex1;

        uint unknown3[2];
        uint zero5;
        uint unknown4[2];
        uint zero6;

        char material_params[8];

        uint vert_count;
        uint vert_format;
        uint instances_count;

        nya_math::vec3 bbox_origin;
        float bbox_size;

    } header = reader.read<mptx_header>();

    //print_data(reader, 0, sizeof(header));

    assume(header.zero == 0);
    assume(header.zero2 == 0);
    assume(header.zero3 == 0);
    assume(header.zero4[0] == 0 && header.zero4[1] == 0 && header.zero4[2] == 0);
    assume(header.zero5 == 0);
    assume(header.zero6 == 0);

    if (!header.vert_count || !header.instances_count)
        return true;

    const bool transparent = header.material_params[1] > 0;

    if (transparent)
        m_mptx_transparent_meshes.resize(m_mptx_transparent_meshes.size() + 1);
    else
        m_mptx_meshes.resize(m_mptx_meshes.size() + 1);

    mptx_mesh &mesh = transparent ? m_mptx_transparent_meshes.back() : m_mptx_meshes.back();

    mesh.groups.resize(1);
    auto &g = mesh.groups.back();
    g.offset = 0, g.count = header.vert_count;
    g.diff = header.tex_count > 0 && header.tex0.id ? shared::get_texture(header.tex0.id) : shared::get_black_texture();
    g.spec = header.tex_count > 1 && header.tex1.id ? shared::get_texture(header.tex1.id) : shared::get_black_texture();
    assume(header.tex_count < 3);

    mesh.draw_dist = header.bbox_size * 200;
    mesh.draw_dist *= mesh.draw_dist;

    mesh.instances.resize(header.instances_count);
    for (auto &instance:mesh.instances)
    {
        instance.pos = reader.read<nya_math::vec3>();
        instance.yaw = reader.read<float>();
        instance.bbox.origin = instance.pos + header.bbox_origin;
        instance.bbox.delta = nya_math::vec3(header.bbox_size, header.bbox_size, header.bbox_size);
    }

    struct mptx_vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        float color_coord;
    };

    std::vector<mptx_vert> verts(header.vert_count);

    for (auto &v: verts)
        v.pos = reader.read<nya_math::vec3>();

    for (auto &v: verts)
        v.tc = reader.read<nya_math::vec2>();

    for (auto &v: verts)
        v.normal = reader.read<nya_math::vec3>();

    for (uint i = 0; i < header.vert_count; ++i)
        verts[i].color_coord = (float(i) + 0.5f)/ header.vert_count;

    const int bpp = int(reader.get_remained() / (header.vert_count * header.instances_count));
    assume(bpp == 4 || bpp == 8 || bpp == 12 || bpp == 0);

    assume(header.vert_format == 4353 || header.vert_format == 4865
           || header.vert_format == 8449 || header.vert_format == 8961);

    assert((header.vert_format == 4353 && bpp == 0) || (header.vert_format != 4353 && bpp != 0));
    assert((header.vert_format == 4865 && bpp == 4) || (header.vert_format != 4865 && bpp != 4));
    assert((header.vert_format == 8449 && bpp == 8) || (header.vert_format != 8449 && bpp != 8));
    assert((header.vert_format == 8961 && bpp == 12) || (header.vert_format != 8961 && bpp != 12));

    //ToDo: vert format instead of bpp assumption

    nya_scene::shared_texture res;
    if (bpp == 4 || bpp == 12)
    {
        res.tex.build_texture(reader.get_data(), header.vert_count, header.instances_count, nya_render::texture::color_rgba);
    }
    else //ToDo
    {
        uint8_t white[]={255,255,255,255};
        res.tex.build_texture(white, 1, 1, nya_render::texture::color_rgba);
    }

    res.tex.set_filter(nya_render::texture::filter_nearest, nya_render::texture::filter_nearest, nya_render::texture::filter_nearest);
    res.tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    mesh.color.create(res);

    //if (reader.get_remained() > 0)
    //    printf("%ld\n", reader.get_remained());

    if (!mesh.vbo.is_valid())
        mesh.vbo.create();

    mesh.vbo->set_vertex_data(&verts[0], sizeof(mptx_vert), header.vert_count);
    mesh.vbo->set_normals(12);
    mesh.vbo->set_tc(0, 24, 3);

    return true;
}

//------------------------------------------------------------
}
