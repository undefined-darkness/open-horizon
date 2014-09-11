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

#include <math.h>
#include <assert.h>

#include "shared.h"

#include "render/debug_draw.h"
extern nya_render::debug_draw test;

//------------------------------------------------------------

static nya_math::vec3 light_dir = nya_math::vec3(0.5f, 1.0f, 0.5f).normalize();

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

void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0)
{
    FILE *file = 0;
    if (fileName)
        file = fopen(fileName, "wb");

#define prnt(...) do{ if (file) fprintf(file, __VA_ARGS__); else printf(__VA_ARGS__); }while(0)

    prnt("\ndata at offset: %ld size: %ld\n", offset, size);

    nya_memory::memory_reader reader = const_reader;
    reader.seek(offset);
    if (size > reader.get_remained())
        size = reader.get_remained();

    bool had_zero = false;
    for (int i = 0; reader.get_offset() < offset+size; ++i)
    {
        int off = int(reader.get_offset());
        unsigned int t = reader.read<unsigned int>();/*
        if (t == 0 && !substruct_size)
        {
            if (!had_zero)
            {
                prnt("\n");
                had_zero = true;
            }

            continue;
        }

        had_zero = false;
        */

        if (i * 4 == off)
            prnt( "%7d ", i * 4 );
        else
            prnt( "%7d %7d ", i * 4, off );

        float f =* ((float*)&t);
        unsigned short s[2];
        memcpy(s, &t, 4);

        char c[4];
        memcpy(c, &t, 4);

        //if ((fabs(f) < 0.001 && t != 0) || (fabs(f) > 1000.0f))
        //prnt( "%10u ", t);

        if (fabs(f) > 50000.0f)
            prnt( "           " );
        else
            prnt( "%10f ", f);

        prnt( "%10u ", t);

        prnt( "%6d %6d   ", s[0], s[1] );
        for (int j = 0; j < 4; ++j)
        {
            char h = c[j];
            if (h > 32 && h < 127)
                prnt("%c", h);
            else
                prnt("¥");
        }

        prnt("    %08x    \n", t);

        if (substruct_size)
        {
            static int k = 0, count = 0;
            if (++k >= substruct_size) { k = 0; prnt("%d\n", count++); }
        }
    }
    
    prnt("\n");
    
    if (file)
        fclose(file);
}

//------------------------------------------------------------

struct fhm_location_load_data
{
    std::vector<unsigned char> tex_indices_data;
/*
    struct patch
    {
        ushort unknown[4];
    };
*/
    std::vector<unsigned int> textures;
};

//------------------------------------------------------------

bool fhm_location::read_location_tex_indices(memory_reader &reader, fhm_location_load_data &load_data)
{
    load_data.tex_indices_data.resize(reader.get_remained());
    memcpy(&load_data.tex_indices_data[0], reader.get_data(), reader.get_remained());

    return true;
}

//------------------------------------------------------------

bool fhm_location::read_location_patches(memory_reader &reader, fhm_location_load_data &load_data)
{
    //ToDo
    return true;
}

//------------------------------------------------------------

bool fhm_location::finish_load_location(fhm_location_load_data &load_data)
{
    assert(!load_data.tex_indices_data.empty());

    auto &p=m_land_material.get_pass(m_land_material.add_pass(nya_scene::material::default_pass));
    p.set_shader(nya_scene::shader("shaders/land.nsh"));

    m_land_material.set_param(m_land_material.get_param_idx("light dir"), light_dir.x, light_dir.y, light_dir.z, 0.0f );

    class vbo_data
    {
    public:
        void add_patch(float x, float y, int tc_idx)
        {
            int ty = tc_idx / 7;
            int tx = tc_idx - ty * 7;

            nya_math::vec4 tc(7, 7, 514, 514);

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

    m_landscape.width = 4;
    m_landscape.height = 6;
    float patch_size = 1024.0;
    m_landscape.patches.clear();
    m_landscape.patches.resize(m_landscape.width * m_landscape.height);

    //static int test = 0;

    //ToDo: offsets for all maps, or find out how to load it (possible harder)
    const int tci_offsets[] = 
    { 0,  0,  0,  1, 
      3,  4,  5,  6, 
      7,  8,  9, 10, 
     11, 12, 13, 14, 
     3,  15, 16, 17, 
     15, 18, -1, -1 };

    //printf("test %d\n", test);
    //++test;

    assert(!load_data.textures.empty());

    vbo_data vdata(patch_size); //ToDo

    unsigned int quads_per_patch = 8;
    int tc_idx = 64 * 4;

    //int py = 0;
    for (int px = 0; px < m_landscape.width; ++px)
    for (int py = 0; py < m_landscape.height; ++py)
    {
        landscape::patch &p = m_landscape[px][py];

        //if (px != 0 || py != 0)
        //    continue;

        //if (px == 1 && py == 0) tc_idx = 0;

        //tc_idx = 64;

        tc_idx = tci_offsets[py * m_landscape.width + px] * quads_per_patch * quads_per_patch;
        if (tc_idx < 0)
            continue;

        //ToDo

        float base_x = patch_size * quads_per_patch * (px - 3);
        float base_y = patch_size * quads_per_patch * (py - 3);

        int last_tex_idx = -1; //int test = 0;
        for (int y = 0; y < quads_per_patch; ++y)
        for (int x = 0; x < quads_per_patch; ++x)
        {
            //if (test++ >= 10)
            //    break;

            int tex_idx = load_data.tex_indices_data[tc_idx * 2 + 1];

            //tex_idx = 2;//test

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

        //px = py = 9000;
    }

    m_landscape.vbo.set_vertex_data(vdata.get_data(), 5 * 4, vdata.get_count());
    m_landscape.vbo.set_tc(0, 3 * 4, 2);

    return true;
}

//------------------------------------------------------------

bool fhm_location::load(const char *fileName)
{
    fhm_file fhm;
    if (!fhm.open(fileName))
        return false;

    const bool is_location = strncmp(fileName, "Map/ms", 6) == 0 && strstr(fileName, "_") == 0;

    fhm_location_load_data location_load_data;

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
        else if (sign == 'RXTN') //NTXR texture?
        {
            read_ntxr(reader, location_load_data);
        }
        else if (sign == 'xtpm') //mptx
        {
            read_mptx(reader);
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
        else
        {
            if (is_location && j == 8)
                read_location_patches(reader, location_load_data);
            else if (is_location && j == 9)
                read_location_tex_indices(reader, location_load_data);

            //read_unknown(reader);

            //char fname[255]; sprintf(fname, "chunk%d.txt", j); print_data(reader, 0, 2000000, 0, fname);

            //read_unknown(reader);
            //int i = 5;

            //printf("chunk%d offset %d\n", j, ch.offset + 48);
        }
    }

    if(is_location)
        finish_load_location(location_load_data);

    return true;
}

//------------------------------------------------------------

void fhm_location::draw_col(int col_idx)
{
    if (col_idx < 0 || col_idx >= cols.size())
        return;

    col &c = cols[col_idx];
    c.vbo.bind();
    c.vbo.draw();
    c.vbo.unbind();
}

//------------------------------------------------------------

void fhm_location::draw_mptx()
{
    const nya_math::frustum &f = nya_scene::get_camera().get_frustum();
    const nya_math::vec3 &cp = nya_scene::get_camera().get_pos();

    for (auto &mesh:mptx_meshes)
    {
        mesh.vbo.bind();

        for (int i = 0; i < mesh.textures.size(); ++i)
        {
            shared::get_texture(mesh.textures[i]).internal().set(i);
            break; //ToDo
        }

        for ( auto &instance:mesh.instances)
        {
            if (!f.test_intersect(instance.bbox))
                continue;

            const nya_math::vec3 delta = instance.pos - cp;
            const float dist_sq = delta * delta;
            if (dist_sq>mesh.draw_dist)
                continue;

            nya_scene::transform tr;
            tr.set_pos(instance.pos);
            tr.set_rot(instance.yaw, 0.0f, 0.0f);
            nya_scene::transform::set(tr);
            m_location_objects_shader.internal().set();

            m_location_objects_shader.internal().set_uniform_value(m_location_objects_light_dir_idx, light_dir.x, light_dir.y, light_dir.z, 0.0f);

            mesh.vbo.draw();
            m_location_objects_shader.internal().unset();
        }

        mesh.vbo.unbind();
    }
}

//------------------------------------------------------------

void fhm_location::draw_landscape(int dt)
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_land_material.internal().set(nya_scene::material::default_pass);

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
    if (r > 1000000000) //probably there is another way
    {
        for (auto &t: load_data.textures) if (t == r) return true;
        load_data.textures.push_back(r);
    }

    return r>0;
}

//------------------------------------------------------------

bool fhm_location::read_mptx(memory_reader &reader)
{
    //print_data(reader, 0, reader.get_remained());
    //print_data(reader, 120, 48);

    m_location_objects_shader.load("shaders/location_objects.nsh");

    auto sh = m_location_objects_shader.internal();
    for (int i = 0; i < sh.get_uniforms_count(); ++i)
    {
        if (sh.get_uniform(i).name == "light dir")
        {
            m_location_objects_light_dir_idx = i;
            break;
        }
    }

    mptx_meshes.resize(mptx_meshes.size() + 1);
    mptx_mesh &mesh = mptx_meshes.back();

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
        instance.yaw = reader.read<float>() * 180.0f / 3.14f;
        instance.bbox.origin = instance.pos + bbox_origin;
        instance.bbox.delta = nya_math::vec3(bbox_size, bbox_size, bbox_size);
    }

    struct mptx_vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
    };

    std::vector<mptx_vert> verts(vcount);

    for (auto &v: verts)
        v.pos = reader.read<nya_math::vec3>();

    for (auto &v: verts)
        v.tc = reader.read<nya_math::vec2>();

    for (auto &v: verts)
        v.normal = reader.read<nya_math::vec3>();

    mesh.vbo.set_vertex_data(&verts[0], sizeof(mptx_vert), vcount);
    mesh.vbo.set_normals(12);
    mesh.vbo.set_tc(0, 24, 2);

    return true;
}

//------------------------------------------------------------

bool fhm_location::read_colh(memory_reader &reader)
{
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

    for (int i = 0; i < header.count; ++i)
    {
        colh_chunk &c = chunks[i];
        reader.seek(c.offset);

        c.header = reader.read<colh_info>();
        assert(c.header.unknown_32 == 32);
        assert(c.header.unknown_zero == 0);
        //for (int j = 0; j < 1; ++j)
        {
            nya_math::vec3 p;
            p.x = reader.read<float>();
            p.y = reader.read<float>();
            p.z = reader.read<float>();
            float f = reader.read<float>();
            nya_math::vec3 p2;
            p2.x = reader.read<float>();
            p2.y = reader.read<float>();
            p2.z = reader.read<float>();
            //float f = reader.read<float>();
            float f2 = reader.read<float>();
            //test.add_point(p, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            //test.add_line(p, p2, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            nya_math::aabb b;
            b.origin = p;
            //b.delta = nya_math::vec3(f, f, f);
            b.delta = p2;
            //test.add_aabb(b, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
        }

        break;
    }

    return true;
}

//------------------------------------------------------------
