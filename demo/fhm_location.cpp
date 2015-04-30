//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm_location.h"
#include "fhm.h"

#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "render/render.h"
#include "scene/camera.h"
#include "scene/transform.h"
#include "scene/shader.h"
#include "math/scalar.h"

#include <math.h>
#include <assert.h>
#include <stdint.h>

#include "shared.h"
#include "debug.h"

#include "render/debug_draw.h"
extern nya_render::debug_draw test;

//------------------------------------------------------------

static const int location_size = 16;
static const float patch_size = 1024.0;
static const unsigned int quads_per_patch = 8;

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
    std::vector<unsigned char> tex_indices_data;

    unsigned char patches[location_size * location_size];

    std::vector<unsigned int> textures;
};

//------------------------------------------------------------

bool fhm_location::finish_load_location(fhm_location_load_data &load_data)
{
    assert(!load_data.tex_indices_data.empty());

    auto &p=m_land_material.get_pass(m_land_material.add_pass(nya_scene::material::default_pass));
    p.set_shader(nya_scene::shader("shaders/land.nsh"));

    class vbo_data
    {
    public:
        void add_patch(float x, float y, int tc_idx)
        {
            int ty = tc_idx / 7;
            int tx = tc_idx - ty * 7;

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
            v.pos.y=-0.2f;

            v.pos.x = x;
            v.pos.z = y;
            v.tc.x = tc.x;
            v.tc.y = tc.y;
            m_verts.push_back(v);

            v.pos.x = x;
            v.pos.z = y + m_patch_size;
            v.tc.x = tc.x;
            v.tc.y = tc.w;
            m_verts.push_back(v);

            v.pos.x = x + m_patch_size;
            v.pos.z = y + m_patch_size;
            v.tc.x = tc.z;
            v.tc.y = tc.w;
            m_verts.push_back(v);

            m_verts.push_back(m_verts[m_verts.size() - 3]);
            m_verts.push_back(m_verts[m_verts.size() - 2]);

            v.pos.x = x + m_patch_size;
            v.pos.z = y;
            v.tc.x = tc.z;
            v.tc.y = tc.y;
            m_verts.push_back(v);
        }

        void update_heights(landscape &l)
        {
            for(auto &v: m_verts)
                v.pos.y = l.get_height(v.pos.x, v.pos.z) - 0.05f;
        }

        void *get_data() { return m_verts.empty() ? 0 : &m_verts[0]; }
        unsigned int get_count() { return (unsigned int)m_verts.size(); }

        void set_tex_size(int width, int height) { m_tex_width = width; m_tex_height = height; }

        vbo_data(int patch_size):m_patch_size(patch_size) {}

    private:
        float m_tex_width;
        float m_tex_height;
        float m_patch_size;

        struct vert
        {
            nya_math::vec3 pos;
            nya_math::vec2 tc;
        };

        std::vector<vert> m_verts;
    };

    m_landscape.patches.clear();
    m_landscape.patches.resize(location_size * location_size);

    assert(!load_data.textures.empty());

    vbo_data vdata(patch_size); //ToDo

    int tc_idx = 64 * 4;

    //int py = 0;
    for (int py = 0; py < location_size; ++py)
    for (int px = 0; px < location_size; ++px)
    {
        int idx = py * location_size + px;

        landscape::patch &p = m_landscape.patches[idx] ;

        tc_idx = load_data.patches[idx] * quads_per_patch * quads_per_patch;
        if (tc_idx < 0)
            continue;

        float base_x = patch_size * quads_per_patch * (px - location_size/2);
        float base_y = patch_size * quads_per_patch * (py - location_size/2);

        int last_tex_idx = -1;
        for (int y = 0; y < quads_per_patch; ++y)
        for (int x = 0; x < quads_per_patch; ++x)
        {
            int tex_idx = load_data.tex_indices_data[tc_idx * 2 + 1];

            if (tex_idx != last_tex_idx)
            {
                if (last_tex_idx >= 0)
                    p.groups.back().count = vdata.get_count() - p.groups.back().offset;

                p.groups.resize(p.groups.size() + 1);
                auto &g = p.groups.back();
                g.offset = vdata.get_count();
                g.tex_id = load_data.textures[tex_idx];
                auto &t = shared::get_texture(g.tex_id);
                vdata.set_tex_size(t.get_width(), t.get_height());

                last_tex_idx = tex_idx;
            }

            vdata.add_patch(base_x + patch_size * x, base_y + patch_size * y, load_data.tex_indices_data[tc_idx * 2]); //15 //25
            ++tc_idx;
        }

        p.groups.back().count = vdata.get_count() - p.groups.back().offset;
    }

    vdata.update_heights(m_landscape);

    m_landscape.vbo.set_vertex_data(vdata.get_data(), 5 * 4, vdata.get_count());
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

        if (sign == 'ETAM') //MATE material?
        {
            //read_mate(reader);
            //read_unknown(reader);
        }
        else if (sign == 'HLOC') //COLH collision mesh?
        {
            read_colh(reader);
            //static int counter = 0; printf("colh %d\n", ++counter);

            //read_unknown(reader);
            //int i = 5;
            //print_data(reader, 0, 200);
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
                test.add_point(u.pos1, nya_math::vec4(1.0, 0.0, 0.0, 1.0));
                test.add_line(u.pos1, u.pos1+nya_math::vec3(0.0, 1000.0, 0.0), nya_math::vec4(1.0, 0.0, 0.0, 1.0));
            }
          */
            //printf("chunk%d offset %d\n", j, ch.offset+48);

        }
        else if (sign == '2TBS')//SBT2
        {/*
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
                test.add_point(u.pos1, nya_math::vec4(1.0, 0.0, 0.0, 1.0));
                test.add_point(u.pos2, nya_math::vec4(0.0, 0.0, 1.0, 1.0));
            }
          */
        }
        else if( is_location )
        {
            if (j == 5)
            {
                /*
                int count = int(reader.get_remained()/4);
                int h = (int)sqrtf(count);
                assert(h);
                int w = count / h;

                //std::swap(w,h);

                assert(reader.get_remained());
                m_landscape.heights.resize(w*h);
                m_landscape.heights_width=w;
                m_landscape.heights_height=h;

                memcpy(&m_landscape.heights[0], reader.get_data(), reader.get_remained());
                */
            }
            else if (j == 8)
            {
                assert(reader.get_remained() == location_size*location_size);
                memcpy(&location_load_data.patches[0], reader.get_data(), reader.get_remained());
            }
            else if (j == 9)
            {
                assert(reader.get_remained());
                location_load_data.tex_indices_data.resize(reader.get_remained());
                memcpy(&location_load_data.tex_indices_data[0], reader.get_data(), reader.get_remained());
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

    if(is_location)
        finish_load_location(location_load_data);

    auto &s = params.sky.mapspecular;
    auto &d = params.detail;

    nya_math::vec3 about_fog_color = params.sky.low.ambient * params.sky.low.skysphere_intensity; //ToDo
    //ms01 0.688, 0.749, 0.764

    nya_scene::material::param light_dir(-params.sky.sun_dir);
    nya_scene::material::param fog_color(about_fog_color.x, about_fog_color.y, about_fog_color.z, -0.01*params.sky.fog_density);
    nya_scene::material::param fog_height(params.sky.fog_height_fresnel, 0.0, 0.0, 0.0); //ToDo: height fade density
    nya_scene::material::param map_param_vs(s.parts_power, 0, 0, 0);
    nya_scene::material::param map_param_ps(s.parts_scale, s.parts_fog_power, s.parts_fresnel_max, s.parts_fresnel);
    nya_scene::material::param map_param2_ps(d.mesh_range, d.mesh_power, d.mesh_repeat, s.parts_reflection_power);

    if(has_mptx)
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
    if(m_cols.size() == m_mptx_meshes.size())
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

void fhm_location::draw_mptx()
{
    const nya_math::frustum &f = nya_scene::get_camera().get_frustum();
    const nya_math::vec3 &cp = nya_scene::get_camera().get_pos();

    nya_scene::transform::set(nya_scene::transform());


    for (auto &mesh: m_mptx_meshes)
    {
        bool was_set = false;

        m_map_parts_color_texture.set(mesh.color);
        m_map_parts_diffuse_texture.set(mesh.textures.size() > 0 ? shared::get_texture(mesh.textures[0]) : shared::get_black_texture());
        m_map_parts_specular_texture.set(mesh.textures.size() > 1 ? shared::get_texture(mesh.textures[1]) : shared::get_black_texture());

        m_map_parts_tr->set_count(0);
        for (size_t i = 0; i < mesh.instances.size(); ++i)
        {
            const auto &instance = mesh.instances[i];

            const nya_math::vec3 delta = instance.pos - cp;
            const float dist_sq = delta * delta;
            if (dist_sq>mesh.draw_dist)
                continue;

            if (!f.test_intersect(instance.bbox))
                continue;

            int idx = m_map_parts_tr->get_count();
            if (idx >= 127) //limited to 500 in shader uniforms, limited to 127 because of ati max instances per draw limitations
            {
                m_map_parts_material.internal().set(nya_scene::material::default_pass);
                if(!was_set)
                {
                    mesh.vbo.bind();
                    was_set = true;
                }
                mesh.vbo.draw(0, mesh.vbo.get_verts_count(), mesh.vbo.get_element_type(), idx);
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
            m_map_parts_material.internal().set(nya_scene::material::default_pass);
            if (!was_set)
            {
                mesh.vbo.bind();
                was_set = true;
            }
            mesh.vbo.draw(0, mesh.vbo.get_verts_count(), mesh.vbo.get_element_type(), instances);
        }

        if( was_set )
        {
            mesh.vbo.unbind();
            m_map_parts_material.internal().unset();
        }
    }

}

//------------------------------------------------------------

void fhm_location::update(int dt)
{
    const int anim_time_interval[]={51731,73164,62695,84235};
    static int anim_time[]={124,6363,36262,47392};
    float t[4];
    for(int i=0;i<4;++i)
    {
        anim_time[i]+=dt;
        if(anim_time[i]>anim_time_interval[i])
            anim_time[i]=0;

        t[i]=float(anim_time[i])/anim_time_interval[i];
    }

    m_land_material.set_param(m_land_material.get_param_idx("anim"), t[0],t[1],-t[2],-t[3]);
}

//------------------------------------------------------------

void fhm_location::draw_landscape()
{
    //ToDo: draw visible only

    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_land_material.internal().set(nya_scene::material::default_pass);

    m_landscape.vbo.bind();
    for (const auto &p: m_landscape.patches)
    {
        for (const auto &g: p.groups)
        {
            shared::get_texture(g.tex_id).internal().set();
            m_landscape.vbo.draw(g.offset, g.count);
            shared::get_texture(g.tex_id).internal().unset();
        }
    }

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
    auto &p = m_map_parts_material.get_pass(m_map_parts_material.add_pass(nya_scene::material::default_pass));
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

    m_mptx_meshes.resize(m_mptx_meshes.size() + 1);
    mptx_mesh &mesh = m_mptx_meshes.back();

    reader.seek(18);
    uint tex_count = reader.read<ushort>();
    mesh.textures.resize(tex_count);
    if (tex_count>0)
    {
        reader.seek(40);
        mesh.textures[0] = reader.read<uint>();
    }

    if (tex_count>1)
    {
        reader.seek(64);
        mesh.textures[1] = reader.read<uint>();
    }

    assert(tex_count < 3);

    reader.seek(120);
    unsigned int vcount = reader.read<unsigned int>();

    reader.seek(128);
    unsigned int instances_count = reader.read<unsigned int>();

    reader.seek(132);
    nya_math::vec3 bbox_origin = reader.read<nya_math::vec3>();
    float bbox_size = reader.read<float>();
    mesh.draw_dist = bbox_size * 200;
    mesh.draw_dist *= mesh.draw_dist;
    reader.seek(148);
    mesh.instances.resize(instances_count);
    for (auto &instance:mesh.instances)
    {
        instance.pos = reader.read<nya_math::vec3>();
        instance.yaw = reader.read<float>();
        instance.bbox.origin = instance.pos + bbox_origin;
        instance.bbox.delta = nya_math::vec3(bbox_size, bbox_size, bbox_size);
    }

    struct mptx_vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        float color_coord;
    };

    std::vector<mptx_vert> verts(vcount);

    for (auto &v: verts)
        v.pos = reader.read<nya_math::vec3>();

    for (auto &v: verts)
        v.tc = reader.read<nya_math::vec2>();

    for (auto &v: verts)
        v.normal = reader.read<nya_math::vec3>();

    for (int i = 0; i < vcount; ++i)
        verts[i].color_coord = (float(i) + 0.5f)/ vcount;

    const int bpp = int(reader.get_remained() / (vcount * instances_count));
    assert(bpp == 4 || bpp == 8 || bpp == 12 || bpp == 0);

    nya_scene::shared_texture res;
    if (bpp == 4 || bpp == 12)
    {
        res.tex.build_texture(reader.get_data(), vcount, instances_count, nya_render::texture::color_rgba);
    }
    else //ToDo
    {
        uint8_t white[]={255,255,255,255};
        res.tex.build_texture(white, 1, 1, nya_render::texture::color_rgba);
    }

    res.tex.set_filter(nya_render::texture::filter_nearest, nya_render::texture::filter_nearest, nya_render::texture::filter_nearest);
    res.tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    mesh.color.create(res);

    //if(reader.get_remained() > 0)
    //    printf("%ld\n", reader.get_remained());

    mesh.vbo.set_vertex_data(&verts[0], sizeof(mptx_vert), vcount);
    mesh.vbo.set_normals(12);
    mesh.vbo.set_tc(0, 24, 3);

    return true;
}

//------------------------------------------------------------

bool fhm_location::read_colh(memory_reader &reader)
{
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

    assert(header.chunk_size == reader.get_remained());
    assert(header.unknown_zero == 0);

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

    assert(header.count == 1);

    col_mesh col;

    for (int i = 0; i < header.count; ++i)
    {
        colh_chunk &c = chunks[i];
        reader.seek(c.offset);

        //print_data(reader,reader.get_offset(),sizeof(colh_info));

        c.header = reader.read<colh_info>();

        //print_data(reader,reader.get_offset(),c.size-sizeof(colh_info));

        assert(c.header.unknown_32 == 32);
        assert(c.header.unknown_zero == 0);
        //for (int j = 0; j < 1; ++j)
        {
            nya_math::vec3 p;
            p.x = reader.read<float>();
            p.y = reader.read<float>();
            p.z = reader.read<float>();
            float f = reader.read<float>();
            assert(f == 1.0f);
            nya_math::vec3 p2;
            p2.x = reader.read<float>();
            p2.y = reader.read<float>();
            p2.z = reader.read<float>();
            //float f = reader.read<float>();
            float f2 = reader.read<float>();
            assert(f2 == 0);
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

float fhm_location::landscape::get_height(float x,float y) const
{
    if (heights.empty())
        return 0.0f;

    float scale = location_size * patch_size * quads_per_patch;

    float tc_x = x / scale + 0.5;
    float tc_y = y / scale + 0.5;

    float fx_idx = tc_x * heights_width;
    float fy_idx = tc_y * heights_height;

    int x_idx = int(fx_idx);
    int y_idx = int(fy_idx);

    if (x_idx < 0 || y_idx < 0)
        return 0.0f;

    if (x_idx + 1 >= heights_width || y_idx + 1>= heights_height)
        return 0.0f;

    float h00 = heights[y_idx * heights_width + x_idx];
    float h10 = heights[y_idx * heights_width + x_idx + 1];
    float h01 = heights[(y_idx + 1) * heights_width + x_idx];
    float h11 = heights[(y_idx + 1) * heights_width + x_idx + 1];

    float kx = fx_idx - x_idx;
    float ky = fy_idx - y_idx;

    float h00_10 = nya_math::lerp(h00, h10, kx);
    float h01_11 = nya_math::lerp(h01, h11, kx);

    return nya_math::lerp(h00_10, h01_11, ky);
}

//------------------------------------------------------------
