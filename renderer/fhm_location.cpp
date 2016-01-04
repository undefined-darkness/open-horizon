//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm_location.h"
#include "containers/fhm.h"

#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "render/render.h"
#include "scene/camera.h"
#include "scene/transform.h"
#include "scene/shader.h"
#include "math/scalar.h"

#include "render/platform_specific_gl.h"

#include <math.h>
#include <stdint.h>

#include "shared.h"

namespace renderer
{
//------------------------------------------------------------

static const int location_size = 16;
static const float patch_size = 1024.0;
static const unsigned int quads_per_patch = 8, subquads_per_quad = 8;

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

    std::vector<unsigned int> textures;
};

//------------------------------------------------------------

bool fhm_location::finish_load_location(fhm_location_load_data &load_data)
{
    assert(!load_data.tex_indices_data.empty());

    auto &p=m_land_material.get_pass(m_land_material.add_pass(nya_scene::material::default_pass));
    p.set_shader(nya_scene::shader("shaders/land.nsh"));
    p.get_state().set_cull_face(true);

    class vbo_data
    {
    public:
        void add_patch(float x, float y, int tc_idx, float *h)
        {
            //ToDo: strips
            //ToDo: exclude patches with -9999.0 height
            //ToDo: lod seams

            int ty = tc_idx / (subquads_per_quad-1);
            int tx = tc_idx - ty * (subquads_per_quad-1);

            nya_math::vec4 tc(8, 8, 512, 512);

            tc.x += (tc.z + tc.x * 2) * tx;
            tc.y += (tc.w + tc.y * 2) * ty;

            tc.z += tc.x;
            tc.w += tc.y;

            tc.x /= m_tex_width;
            tc.y /= m_tex_height;
            tc.z /= m_tex_width;
            tc.w /= m_tex_height;

            vert v;

            const uint hpw = subquads_per_quad*subquads_per_quad+1;
            const uint w = subquads_per_quad + 1;

            uint voff = (uint)m_verts.size();

            tc.zw() -= tc.xy();
            tc.zw() /= float(subquads_per_quad);
            const float hpatch_size = patch_size/subquads_per_quad;
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

            for (int hy=0; hy < subquads_per_quad; ++hy)
            for (int hx=0; hx < subquads_per_quad; ++hx)
            {
                int i = voff + hx + hy * w;

                m_curr_indices_hi.push_back(i);
                m_curr_indices_hi.push_back(i + 1 + subquads_per_quad);
                m_curr_indices_hi.push_back(i + 1);
                m_curr_indices_hi.push_back(i + 1);
                m_curr_indices_hi.push_back(i + 1 + subquads_per_quad);
                m_curr_indices_hi.push_back(i + 1 + subquads_per_quad + 1);
            }

            for (int hy=0; hy < subquads_per_quad; hy += 2)
            for (int hx=0; hx < subquads_per_quad; hx += 2)
            {
                int i = voff + hx + hy * w;

                m_curr_indices_mid.push_back(i);
                m_curr_indices_mid.push_back(i + 2 * (1 + subquads_per_quad));
                m_curr_indices_mid.push_back(i + 2);
                m_curr_indices_mid.push_back(i + 2);
                m_curr_indices_mid.push_back(i + 2 * (1 + subquads_per_quad));
                m_curr_indices_mid.push_back(i + 2 * (1 + subquads_per_quad + 1));
            }
        }

        typedef unsigned int uint;

        uint get_group_hi_offset() { return (uint)m_indices.size(); }
        uint get_group_hi_count() { return (uint)m_curr_indices_hi.size(); }
        uint get_group_mid_offset() { return (uint)(m_indices.size() + m_curr_indices_hi.size()); }
        uint get_group_mid_count() { return (uint)m_curr_indices_mid.size(); }
        uint get_group_low_offset() { return (uint)(m_indices.size() + m_curr_indices_hi.size() + m_curr_indices_mid.size()); }
        uint get_group_low_count() { return (uint)m_curr_indices_low.size(); }

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

    private:
        float m_tex_width;
        float m_tex_height;

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

    //int py = 0;
    for (int py = 0; py < location_size; ++py)
    for (int px = 0; px < location_size; ++px)
    {
        const int idx = py * location_size + px;

        landscape::patch &p = m_landscape.patches[idx] ;

        int tc_idx = load_data.patches[idx] * quads_per_patch * quads_per_patch;
        if (tc_idx < 0)
            continue;

        const float base_x = patch_size * quads_per_patch * (px - location_size/2);
        const float base_y = patch_size * quads_per_patch * (py - location_size/2);

        const int hpw = quads_per_patch*quads_per_patch+1;
        int h_idx = load_data.height_patches[idx] * hpw * hpw;
        assert(h_idx>=0);

        assert(h_idx + hpw * hpw <= load_data.heights.size());

        int last_tex_idx = -1;
        for (int y = 0; y < quads_per_patch; ++y)
        for (int x = 0; x < quads_per_patch; ++x)
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
                g.tex_id = load_data.textures[tex_idx];
                auto &t = shared::get_texture(g.tex_id);
                vdata.set_tex_size(t.get_width(), t.get_height());

                last_tex_idx = tex_idx;
            }

            const int hpp = (hpw-1)/quads_per_patch;
            float *h = &load_data.heights[h_idx+(x + y * hpw)*hpp];

            const float pos_x = base_x + patch_size * x;
            const float pos_y = base_y + patch_size * y;

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

    m_landscape.vbo.set_vertex_data(vdata.get_vdata(), 5 * 4, vdata.get_vcount());
    m_landscape.vbo.set_index_data(vdata.get_idata(), nya_render::vbo::index4b, vdata.get_icount());
    m_landscape.vbo.set_tc(0, 3 * 4, 2);

    return true;
}

//------------------------------------------------------------

bool fhm_location::load(const char *fileName, const location_params &params)
{
    fhm_file fhm;
    if (!fhm.open(fileName))
        return false;

    const bool is_location = strncmp(fileName, "Map/ms", 6) == 0 && strstr(fileName, "_") == 0;

    fhm_location_load_data location_load_data;

    bool has_mptx = false;

    for (int j = 0; j < fhm.get_chunks_count(); ++j)
    {
        nya_memory::tmp_buffer_scoped buf(fhm.get_chunk_size(j));
        fhm.read_chunk_data(j, buf.get_data());
        memory_reader reader(buf.get_data(), fhm.get_chunk_size(j));
        const uint sign = fhm.get_chunk_type(j);

        if (sign == 'HLOC') //COLH collision mesh?
        {
            read_colh(reader);
            //static int counter = 0; printf("colh %d\n", ++counter);

            //read_unknown(reader);
            //int i = 5;
            //print_data(reader, 0, reader.get_remained());
        }
        else if (sign == 'RXTN') //NTXR texture
        {
            read_ntxr(reader, location_load_data);
        }
        else if (sign == 'xtpm') //mptx
        {
            read_mptx(reader);
            has_mptx = true;
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
            else
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

            //printf("chunk%d offset %d\n", j, ch.offset + 48);
        }
    }

    if (is_location)
        finish_load_location(location_load_data);

    auto &s = params.sky.mapspecular;
    auto &d = params.detail;

    nya_math::vec3 about_fog_color = params.sky.low.ambient * params.sky.low.skysphere_intensity; //ToDo
    //ms01 0.688, 0.749, 0.764

    nya_scene::material::param light_dir(-params.sky.sun_dir);
    nya_scene::material::param fog_color(about_fog_color.x, about_fog_color.y, about_fog_color.z, -0.01*params.sky.fog_density);
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
        m_map_parts_diffuse_texture.set(mesh.textures.size() > 0 ? shared::get_texture(mesh.textures[0]) : shared::get_black_texture());
        m_map_parts_specular_texture.set(mesh.textures.size() > 1 ? shared::get_texture(mesh.textures[1]) : shared::get_black_texture());

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
                m_map_parts_material.internal().set(nya_scene::material::default_pass);
                mesh.vbo.bind();
                mesh.vbo.draw(0, mesh.vbo.get_verts_count(), mesh.vbo.get_element_type(), idx);
                mesh.vbo.unbind();
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
            mesh.vbo.bind();
            mesh.vbo.draw(0, mesh.vbo.get_verts_count(), mesh.vbo.get_element_type(), instances);
            mesh.vbo.unbind();
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
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1,-1);

    auto &p = m_map_parts_material.get_default_pass();
    p.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha); //ToDo: different blend modes
    p.get_state().zwrite = false;
    draw(m_mptx_transparent_meshes);

    glDisable(GL_POLYGON_OFFSET_FILL);
}

//------------------------------------------------------------

void fhm_location::draw_landscape()
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_land_material.internal().set(nya_scene::material::default_pass);

    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    auto &c = nya_scene::get_camera();

    m_landscape.vbo.bind();
    for (const auto &p: m_landscape.patches)
    {
        if (!c.get_frustum().test_intersect(p.box))
            continue;

        for (const auto &g: p.groups)
        {
            if (!c.get_frustum().test_intersect(g.box))
                continue;

            const float sq = g.box.sq_dist(c.get_pos());
            shared::get_texture(g.tex_id).internal().set();

            const float hi_dist = 10000.0f;
            const float mid_dist = 15000.0f;
            if (sq < hi_dist * hi_dist)
                m_landscape.vbo.draw(g.hi_offset, g.hi_count);
            else if (sq < mid_dist * mid_dist)
                m_landscape.vbo.draw(g.mid_offset, g.mid_count);
            else
                m_landscape.vbo.draw(g.low_offset, g.low_count);

            shared::get_texture(g.tex_id).internal().unset();
        }
    }

    //glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );

    m_landscape.vbo.unbind();
    m_land_material.internal().unset();
}

//------------------------------------------------------------

bool fhm_location::read_ntxr(memory_reader &reader, fhm_location_load_data &load_data)
{
    uint r = shared::load_texture(reader.get_data(), reader.get_remained());
    if (r > 1000000000) //ToDo //probably there is another way
    {
        for (auto &t: load_data.textures) if (t == r) return true;
        load_data.textures.push_back(r);
    }

    return r>0;
}

//------------------------------------------------------------

bool fhm_location::read_mptx(memory_reader &reader)
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

    const bool transparent = header.material_params[1] > 0;

    if (transparent)
    {
        m_mptx_transparent_meshes.resize(m_mptx_transparent_meshes.size() + 1);
    }
    else
        m_mptx_meshes.resize(m_mptx_meshes.size() + 1);

    mptx_mesh &mesh = transparent ? m_mptx_transparent_meshes.back() : m_mptx_meshes.back();

    mesh.textures.resize(header.tex_count);
    if (header.tex_count>0)
        mesh.textures[0] = header.tex0.id;
    if (header.tex_count>1)
        mesh.textures[1] = header.tex1.id;

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

    assume(header.vert_format == 4865 || header.vert_format == 8449 || header.vert_format == 8961);

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

    mesh.vbo.set_vertex_data(&verts[0], sizeof(mptx_vert), header.vert_count);
    mesh.vbo.set_normals(12);
    mesh.vbo.set_tc(0, 24, 3);

    return true;
}

//------------------------------------------------------------

bool fhm_location::read_colh(memory_reader &reader)
{
    return true;
    //printf("\n\nCOLH\n\n");
    //print_data(reader);

    struct colh_header
    {
        char sign[4];
        uint chunk_size;
        uint offset_to_info;
        uint unknown_zero;
        ushort count;
        ushort unknown2;
        uint offset_to_offsets;
        uint offset_to_unknown;
        ushort unknown3;
        ushort unknown4;
    };

    colh_header header = reader.read<colh_header>();
    reader.seek(0);
    //print_data(reader, 0, reader.get_remained());

    assume(header.chunk_size == reader.get_remained());
    assume(header.unknown_zero == 0);

    struct colh_info
    {
        uint unknown_32;
        uint unknown_zero;
        uint offset_to_unknown;
        uint offset_to_unknown2;
        ushort unknown;
        short unknown2;
        uint unknown_zero2;
        ushort vcount;
        ushort unknown3;
        ushort unknown4;
        ushort unknown5;
    };

    struct colh_chunk
    {
        uint offset;
        uint size;

        colh_info header;
    };

    std::vector<colh_chunk> chunks;
    chunks.resize(header.count);
    reader.seek(header.offset_to_offsets);
    for (int i = 0; i < header.count; ++i)
    {
        chunks[i].offset = reader.read<uint>();
        chunks[i].size = reader.read<uint>();
    }

    assume(header.count == 1);

    col_mesh col;

    for (int i = 0; i < header.count; ++i)
    {
        colh_chunk &c = chunks[i];
        reader.seek(c.offset);

        //print_data(reader,reader.get_offset(),sizeof(colh_info));

        c.header = reader.read<colh_info>();

        //print_data(reader,reader.get_offset(),c.size-sizeof(colh_info));

        assume(c.header.unknown_32 == 32);
        assume(c.header.unknown_zero == 0);
        //for (int j = 0; j < 1; ++j)
        {
            nya_math::vec3 p;
            p.x = reader.read<float>();
            p.y = reader.read<float>();
            p.z = reader.read<float>();
            float f = reader.read<float>();
            assume(f == 1.0f);
            nya_math::vec3 p2;
            p2.x = reader.read<float>();
            p2.y = reader.read<float>();
            p2.z = reader.read<float>();
            //float f = reader.read<float>();
            float f2 = reader.read<float>();
            assume(f2 == 0);
            //test.add_point(p, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            //test.add_line(p, p2, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            nya_math::aabb b;
            b.origin = p;
            //b.delta = nya_math::vec3(f, f, f);
            b.delta = p2;

            col.box = b;
        }
    }

    //print_data(reader);

    m_cols.push_back(col);

    return true;
}

//------------------------------------------------------------
}
