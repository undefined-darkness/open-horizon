//
// open horizon -- undefined_darkness@outlook.com
//

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

struct fhm_mesh_load_data
{
    struct mnt { nya_render::skeleton skeleton; };
    std::vector<mnt> skeletons;

    struct mop2 { std::map<std::string, nya_scene::animation> animations; };
    std::vector<mop2> animations;

    size_t ndxr_count;

    fhm_mesh_load_data(): ndxr_count(0) {}
};

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

bool fhm_mesh::read_location_tex_indices(memory_reader &reader, fhm_location_load_data &load_data)
{
    load_data.tex_indices_data.resize(reader.get_remained());
    memcpy(&load_data.tex_indices_data[0], reader.get_data(), reader.get_remained());

    return true;
}

//------------------------------------------------------------

bool fhm_mesh::read_location_patches(memory_reader &reader, fhm_location_load_data &load_data)
{
    //ToDo
    return true;
}

//------------------------------------------------------------

bool fhm_mesh::finish_load_location(fhm_location_load_data &load_data)
{
    assert(!load_data.tex_indices_data.empty());

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

bool fhm_mesh::read_chunks_info(memory_reader &reader, size_t base_offset)
{
    reader.seek(base_offset);
    uint chunks_count = reader.read<uint>();
    //print_data(reader, base_offset, 1024);

    for (int i = 0; i < chunks_count; ++i)
    {
        //reader.seek(48+(i+1)*8);
        reader.seek(base_offset + 4 + i * 8);
        const uint nested = reader.read<uint>();
        const uint offset = reader.read<uint>();

        if (nested == 1)
        {
            read_chunks_info(reader, offset + base_offset);
            continue;
        }

        assert(nested == 0);

        //printf("%d %d\n", nested, offset);
        reader.seek(base_offset + offset);
        //print_data(reader, reader.get_offset(), sizeof(chunk_info));
        chunks.resize(chunks.size() + 1);
        chunks.back() = reader.read<chunk_info>();

        //assert((chunks[i].unknown1 == 1 && chunks[i].unknown2 == 2) || (chunks[i].unknown1 == 0 && chunks[i].unknown2 == 0));
        assert(chunks[i].unknown_16 == 16 || chunks[i].unknown_16 == 128 || chunks[i].unknown_16 == 4096);
    }

    return true;
}

//------------------------------------------------------------

void ndxr_mesh::set_ndxr_texture(int lod_idx, const char *semantics, const char *file_name)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return;

    if (!semantics || !file_name)
        return;

    nya_scene::texture tex;
    tex.load(file_name);
    for (int i = 0; i < lods[lod_idx].mesh.get_materials_count(); ++i)
        lods[lod_idx].mesh.modify_material(i).set_texture(semantics, tex);
}

//------------------------------------------------------------

bool fhm_mesh::load(const char *fileName)
{
    nya_resources::resource_data *file_data = nya_resources::get_resources_provider().access(fileName);
    if (!file_data)
    {
        nya_resources::get_log()<<"unable to open file\n";
        return false;
    }

    const bool is_location = strncmp(fileName, "Map/ms", 6) == 0 && strstr(fileName, "_") == 0;

    nya_memory::tmp_buffer_scoped fi_data(file_data->get_size());
    file_data->read_all(fi_data.get_data());
    memory_reader reader(fi_data.get_data(), file_data->get_size());

    typedef unsigned long ulong;
    typedef unsigned int uint;
    typedef unsigned short ushort;
    printf("total size: %ld header:\n", file_data->get_size());

    //print_data(reader, 0, 48);

    //print_data(reader, 0, 1024);

    chunks.clear();
    read_chunks_info(reader, 48);

    fhm_mesh_load_data load_data;

    fhm_location_load_data location_load_data;

    for (int j = 0; j < chunks.size(); ++j)
    {
        //int j = 0;
        chunk_info &ch = chunks[j];

        //if (!ch.size) print_data(reader, ch.offset + 48, 256);

        memory_reader reader((const char*)fi_data.get_data() + ch.offset + 48, ch.size);

        uint sign = reader.read<uint>(); //1381516366 = NDXR
        reader.seek(0);

        //const char *sign_text = (const char*)&sign; printf("\nchunk: %d type %u %c%c%c%c offset %d size: %d\n", j, sign, sign_text[0], sign_text[1], sign_text[2], sign_text[3], ch.offset, ch.size);

        //print_data(reader, 0, 20000);
        //char fname[255]; sprintf(fname, "chunk%d.txt", j); print_data(reader, 0, reader.get_remained(), 0, fname);

        if (sign == 'RXDN') //NDXR mesh, load it after anything else
        {
            ++load_data.ndxr_count;
        }
        else if (sign == '2POM') //MOP2 animation
        {
            read_mop2(reader, load_data);
        }
        else if (sign == '\0TNM') //MNT skeleton
        {
            read_mnt(reader, load_data);
        }
        else if (sign == 'ETAM') //MATE material?
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

    for (int j = 0; j < chunks.size(); ++j)
    {
        chunk_info &ch = chunks[j];
        memory_reader reader((const char*)fi_data.get_data() + ch.offset + 48, ch.size);
        uint sign = reader.read<uint>();
        reader.seek(0);

        if (sign == 1381516366) //NDXR mesh
        {
            //static int only = 0;
            //if (only++ == 0)
            read_ndxr(reader, load_data);
        }
    }

    if (is_location)
        finish_load_location(location_load_data);

    return true;
}

//------------------------------------------------------------

void fhm_mesh::draw_col(int col_idx)
{
    if (col_idx < 0 || col_idx >= cols.size())
        return;

    col &c = cols[col_idx];
    c.vbo.bind();
    c.vbo.draw();
    c.vbo.unbind();
}

//------------------------------------------------------------

void fhm_mesh::draw_mptx()
{
    const nya_math::frustum &f = nya_scene::get_camera()->get_frustum();
    const nya_math::vec3 &cp = nya_scene::get_camera()->get_pos();

    /*
    FILE *o = fopen("textures.bin", "wb");
    std::map<unsigned int, bool> texs;

    for (auto &mesh:mptx_meshes)
    {
        if (!mesh.textures.empty())
            texs[mesh.textures[0]] = true;
    }

    unsigned int size = (unsigned int)texs.size();
    fwrite(&size, 1, 4, o);
    for (auto &t:texs)
    {
        const nya_render::texture &tex = shared::get_texture(t.first).internal().get_shared_data()->tex;

        fwrite(&t.first, 1, 4, o);
        unsigned int width = tex.get_width();
        unsigned int height = tex.get_height();
        fwrite(&width, 1, 4, o);
        fwrite(&height, 1, 4, o);
        nya_memory::tmp_buffer_ref buf;
        tex.get_data(buf);
        assert(buf.get_size() == width*height*4);
        fwrite(buf.get_data(), 1, buf.get_size(), o);
    }
    fclose(o);
    */

    /*
    FILE *o = fopen("meshes.bin", "wb");
    unsigned int size = (unsigned int)mptx_meshes.size();
    fwrite(&size, 1, 4, o);
    for (auto &mesh:mptx_meshes)
    {
        nya_memory::tmp_buffer_ref buf;
        mesh.vbo.get_vertex_data(buf);
        assert(buf.get_data());
        fwrite(&mesh.draw_dist, 1, 4, o);
        unsigned int tex = mesh.textures[0];
        fwrite(&tex, 1, 4, o);
        unsigned int insts = (unsigned int)mesh.instances.size();
        fwrite(&insts, 1, 4, o);
        for (auto &i:mesh.instances)
        {
            fwrite(&i.bbox, 1, 3 * 4 * 2, o);
            fwrite(&i.pos, 1, 3 * 4, o);
            fwrite(&i.yaw, 1, 4, o);
        }
        unsigned int vcount = mesh.vbo.get_verts_count();
        fwrite(&vcount, 1, 4, o);
        unsigned int vdatasize = (unsigned int)buf.get_size();
        fwrite(&vdatasize, 1, 4, o);
        fwrite(buf.get_data(), 1, vdatasize, o);
        buf.free();
    }
    fclose(o);
    */

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

void fhm_mesh::draw_landscape()
{
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
}

//------------------------------------------------------------

bool fhm_mesh::read_ntxr(memory_reader &reader, fhm_location_load_data &load_data)
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

bool fhm_mesh::read_mptx(memory_reader &reader)
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

bool fhm_mesh::read_colh(memory_reader &reader)
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

bool fhm_mesh::read_mnt(memory_reader &reader, fhm_mesh_load_data &load_data)
{
    struct mnt_header
    {
        char sign[4];
        uint unknown_1; //version?
        uint chunk_size;
        uint bones_count;
        uint unknown2;
        ushort unknown_trash[2];
        uint offset_to_unknown1; //trash
        uint offset_to_unknown2; //all indices+1
        uint offset_to_parents;
        uint offset_to_bones;
        uint size_of_unknown;
        uint offset_to_bones_names;
    };

    const mnt_header header = reader.read<mnt_header>();
    assert(header.unknown_1 == 1);

    struct mnt_bone
    {
        ushort idx;
        ushort unknown;
        ushort unknown2;
        short parent;
        ushort unknown3[4];
        uint unknown_zero;
    };

    struct bone
    {
        std::string name;
        uint unknown1;
        ushort unknown2;
        short parent;

        mnt_bone header;
    };

    std::vector<bone> bones(header.bones_count);

    reader.seek(header.offset_to_unknown1);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].unknown1 = reader.read<uint>();

    reader.seek(header.offset_to_unknown2);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].unknown2 = reader.read<ushort>();

    reader.seek(header.offset_to_parents);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].parent = reader.read<short>();

    reader.seek(header.offset_to_bones);
    for (int i = 0; i < header.bones_count; ++i)
    {
        bones[i].header = reader.read<mnt_bone>();
        assert(bones[i].parent == bones[i].header.parent);
    }

    reader.seek(header.offset_to_bones_names);
    for (int i = 0; i < header.bones_count; ++i)
        bones[i].name = reader.read_string();

    fhm_mesh_load_data::mnt mnt;
    for (int i = 0; i < header.bones_count; ++i)
    {
        int b = mnt.skeleton.add_bone(bones[i].name.c_str(), nya_math::vec3(), bones[i].parent, true);
        assert(b == i);
    }

    load_data.skeletons.push_back(mnt);

    return true;
}

//------------------------------------------------------------

bool fhm_mesh::read_mop2(memory_reader &reader, fhm_mesh_load_data &load_data)
{
    struct mop2_header
    {
        char sign[4];// "MOP2"
        ushort unknown;
        ushort unknown2;
        uint size;
        uint equals_to_size;
        uint kfm1_count;
        uint offset_to_kfm1_sizes;
        uint offset_to_kfm1_data_offsets;
        uint offset_to_kfm1_names_offsets;
    };

    mop2_header header = reader.read<mop2_header>();

    struct kfm1_info
    {
        uint size;
        uint offset_to_data; //from chunk begin
        uint offset_to_name;
    };

    std::vector<kfm1_info> kfm1_infos(header.kfm1_count);

    reader.seek(header.offset_to_kfm1_sizes);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].size = reader.read<uint>();

    reader.seek(header.offset_to_kfm1_data_offsets);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].offset_to_data = reader.read<uint>();

    reader.seek(header.offset_to_kfm1_names_offsets);
    for (int i = 0; i < header.kfm1_count; ++i)
        kfm1_infos[i].offset_to_name = reader.read<uint>();

    struct kfm1_sequence_header
    {
        ushort unknown;
        ushort offsets_size;
        ushort first_offset;
        ushort unknown2; //looks like the total size
    };

    struct kfm1_sequence_bone
    {
        ushort idx;
        ushort frames_count;
        ushort offset;
    };

    struct kfm1_sequence
    {
        ushort unknown;
        uint sequence_size;
        uint sequence_offset;

        kfm1_sequence_header header;

        std::vector<kfm1_sequence_bone> bones;
    };

    struct kfm1_bone_info
    {
        ushort unknown[3];
        ushort unknown1;
        ushort unknown2; // 773 or 1031
        ushort unknown3;
        ushort unknown4;
        ushort unknown_4;
        uint unknown6;
        uint unknown_zero[3];
    };

    struct kmf1_header
    {
        char sign[4];// "KMF1"
        uint unknown;
        uint unknown2;
        uint size; //size and offsets from header.offset_to_kmf1
        ushort unknown3;
        ushort bones_count;
        ushort unknown4;
        ushort sequences_count;
        uint offset_to_bones;
        uint offset_to_sequences;
        uint offset_to_sequences_offsets;
        uint unknown7;
        uint offset_to_name;
        uint unknown8;
        uint unknown9;
        uint unknown10;
        uint unknown_zero2[2];
    };

    struct kfm1_struct
    {
        std::string name;
        kmf1_header header;
        std::vector<kfm1_bone_info> bones;
        std::vector<kfm1_sequence> sequences;
    };

    size_t skeleton_idx = load_data.animations.size(); //ToDo?
    assert(skeleton_idx < load_data.skeletons.size());
    auto skeleton = load_data.skeletons[skeleton_idx].skeleton;
    load_data.animations.resize(load_data.animations.size() + 1);

    struct bone
    {
        int parent;
        nya_math::vec3 pos;
        nya_math::quat rot;
        nya_math::quat irot;

        nya_math::quat get_rot(const std::vector<bone> &bones) const
        {
            if (parent < 0)
                return rot;

            return bones[parent].get_rot(bones) * rot;
        }

        nya_math::vec3 get_pos(const std::vector<bone> &bones) const
        {
            if (parent < 0)
                return pos;

            return bones[parent].get_rot(bones).rotate(pos) + bones[parent].get_pos(bones);
        }

        nya_math::vec3 get_fake_pos(const std::vector<bone> &bones) const
        {
            if (parent < 0)
                return pos;

            return pos + bones[parent].get_pos(bones);
        }
    };

    std::vector<bone> base(skeleton.get_bones_count());
    bool first_anim = true;

    std::vector<kfm1_struct> kfm1_structs(header.kfm1_count);
    for (int i = 0; i < header.kfm1_count; ++i)
    {
        reader.seek(kfm1_infos[i].offset_to_name);
        kfm1_structs[i].name = reader.read_string();
        reader.seek(kfm1_infos[i].offset_to_data);
        memory_reader kfm1_reader(reader.get_data(), kfm1_infos[i].size);

        //print_data(kfm1_reader, 0, kfm1_reader.get_remained());

        kfm1_struct &kfm1 = kfm1_structs[i];

        kfm1.header = kfm1_reader.read<kmf1_header>();
        kfm1_reader.seek(kfm1.header.offset_to_bones);
        kfm1.bones.resize(kfm1.header.bones_count);
        for (int k = 0; k < kfm1.header.bones_count; ++k)
            kfm1.bones[k] = kfm1_reader.read<kfm1_bone_info>();

        kfm1.sequences.resize(kfm1.header.sequences_count);
        kfm1_reader.seek(kfm1.header.offset_to_sequences);
        //print_data(kfm1_reader, kfm1_reader.get_offset(), 3000);

        for (int k = 0; k < kfm1.header.sequences_count; ++k)
            kfm1.sequences[k].unknown = kfm1_reader.read<ushort>();

        kfm1_reader.seek(kfm1.header.offset_to_sequences_offsets);
        for (int k = 0; k < kfm1.header.sequences_count; ++k)
        {
            kfm1.sequences[k].sequence_size = kfm1_reader.read<uint>();
            kfm1.sequences[k].sequence_offset = kfm1_reader.read<uint>();
        }

        nya_scene::shared_animation anim;

        //printf("kfm1 name: %s sequences %d\n", kfm1.name.c_str(), kfm1.header.sequences_count);

        for (int k = 0; k < kfm1.header.sequences_count; ++k)
        {
            //int k = 0; //ToDO
            kfm1_sequence &f = kfm1.sequences[k];
            kfm1_reader.seek(kfm1.sequences[k].sequence_offset);
            memory_reader sequence_reader(kfm1_reader.get_data(), f.sequence_size);

            //print_data(sequence_reader, 0, f.sequence_size);

            f.header = sequence_reader.read<kfm1_sequence_header>();
            f.bones.resize(kfm1.bones.size());
            for (auto &b:f.bones)
            {
                b.frames_count = sequence_reader.read<int>();
                b.idx = sequence_reader.read<ushort>();
                b.offset = sequence_reader.read<ushort>();
            }

            for (int j = 0; j < f.bones.size(); ++j)
            {
                sequence_reader.seek(f.bones[j].offset);

                const int idx = kfm1.bones[j].unknown1;
                //assert(idx < skeleton.get_bones_count());
                const int aidx = anim.anim.add_bone(skeleton.get_bone_name(idx));
                assert(aidx >= 0);
                const int frame_time = 16;

                if (first_anim)
                    base[idx].parent = skeleton.get_bone_parent_idx(idx);

                for (int k = 0; k < f.bones[j].frames_count; ++k)
                {
                    nya_math::vec4 value;
                    value.x = sequence_reader.read<float>();
                    value.y = sequence_reader.read<float>();
                    value.z = sequence_reader.read<float>();
                    value.w = sequence_reader.read<float>();
                    const bool is_quat = fabsf(value * value - 1.0f) < 0.001f;
                    if (!is_quat)
                        sequence_reader.rewind(4);

                    //assert(kfm1.bones[j].unknown2 == 1031 || kfm1.bones[j].unknown2 == 773);
                    //if (kfm1.bones[j].unknown2 != 1031 && kfm1.bones[j].unknown2 != 773) printf("%d %d\n", kfm1.bones[j].unknown2, is_quat);

                    if (first_anim)
                    {
                        if (is_quat)
                        {
                            base[idx].rot.v = value.xyz();
                            base[idx].rot.w = value.w;

                            base[idx].irot.v = -base[idx].rot.v;
                            base[idx].irot.w = base[idx].rot.w;
                        }
                        else
                            base[idx].pos = value.xyz();

                        //if (is_quat) //if (kfm1.bones[j].unknown2 == 1031)
                        //    anim.anim.add_bone_rot_frame(aidx, k * frame_time, nya_math::quat(value.x, value.y, value.z, value.w));
                        //else //if (kfm1.bones[j].unknown2 == 773)
                        //    anim.anim.add_bone_pos_frame(aidx, k * frame_time, value.xyz());
                    }
                    else
                    {
                        if (is_quat) //if (kfm1.bones[j].unknown2 == 1031)
                            anim.anim.add_bone_rot_frame(aidx, k * frame_time, nya_math::quat(value.x, value.y, value.z, value.w)); //base[idx].irot*
                        else //if (kfm1.bones[j].unknown2 == 773)
                            anim.anim.add_bone_pos_frame(aidx, k * frame_time, value.xyz() - base[idx].pos);
                    }

                }
            }

            if (first_anim)
            {
                nya_render::skeleton s;
                for (int i = 0; i < skeleton.get_bones_count(); ++i)
                {
                    const int b = s.add_bone(skeleton.get_bone_name(i), base[i].get_fake_pos(base), skeleton.get_bone_parent_idx(i), true);
                    assert(i == b);
                }

                load_data.skeletons[skeleton_idx].skeleton = s;
            }

            //printf("    frame %d bones %d\n", k, int(f.bones.size()));
            first_anim = false;
        }

        load_data.animations.back().animations[kfm1.name].create(anim);
        //printf("duration %.2fs\n", anim.anim.get_duration()/1000.0f);
    }

    return true;
}

//------------------------------------------------------------

bool fhm_mesh::read_ndxr(memory_reader &reader, fhm_mesh_load_data &load_data) //ToDo: add materials, reduce groups to materials count, load textures by hash
{
    struct ndxr_header
    {
        char sign[4];
        ushort unknown;
        ushort unknown2;
        ushort unknown3;
        ushort groups_count;
        ushort bone_min_idx; //if skeleton not presented, 
        ushort bone_max_idx; //this two may be strange

        uint offset_to_indices; //from header (+48)
        uint indices_buffer_size;
        uint vertices_buffer_size; //offset to vertices = offset_to_indices + 48 + indices_buffer_size

        uint unknown_often_zero;
        float bbox_origin[3];
        float bbox_size;
    };

    ndxr_header header = reader.read<ndxr_header>();

    //float *f;// = header.origin; test.add_point(nya_math::vec3(f[0], f[1], f[2]), nya_math::vec4(1, 1, 0, 1));
    //nya_math::aabb bb; bb.delta = nya_math::vec3(1, 1, 1)*header.bbox_size; f = header.bbox_origin; bb.origin = nya_math::vec3(f[0], f[1], f[2]); test.add_aabb(bb, nya_math::vec4(1, 1, 0, 1));

    //printf("offset %u indices_buf %u vertices_buf %u\n", header.offset_to_indices, header.indices_buffer_size, header.vertices_buffer_size);

    struct ndxr_group_header
    {
        float bbox_origin[3];
        float bbox_size;
        float origin[3];
        uint unknown_zero;
        uint offset_to_name; //from the end of vertices buf
        ushort unknown_zero2;
        ushort unknown_8;
        short bone_idx;
        ushort render_groups_count;
        uint offset_to_render_groups;
    };

    struct ndxr_render_group_header
    {
        uint ibuf_offset;
        uint vbuf_offset;
        uint unknown_zero;
        ushort vcount;
        ushort vertex_format;
        uint offset_to_sub_struct2;
        uint unknown_zero3[3];
        uint icount;
        uint unknown_zero5[3];
    };

    struct unknown_substruct2_info
    {
        ushort unknown;
        ushort unknown2;
        uint unknown_zero;

        ushort unknown3;
        ushort unknown_substruct2_count;
        ushort unknown4;
        ushort unknown5;
        ushort unknown6;
        ushort unknown_2;
        uint unknown_zero2[3];
    };

    struct unknown_substruct2
    {
        uint texture_hash_id;
        uint unknown_zero;
        uint unknown2_zero;

        uint unknown_hash_id;
        ushort unknown3;
        ushort unknown4;
        uint unknown5_zero;
    };

    struct ndxr_material_param
    {
        uint unknown_32_or_zero; //zero if last
        uint offset_to_name; //from the end of vertices buf
        ushort unknown_zero;
        ushort unknown_1024;
        uint unknown_zero2;
        float value[4];
    };

    struct material_param
    {
        std::string name;
        ndxr_material_param data;
    };

    struct render_group
    {
        ndxr_render_group_header header;
        unknown_substruct2_info substruct2_info;
        std::vector<unknown_substruct2> unknown_structs;
        std::vector<material_param> material_params;
    };

    struct group
    {
        std::string name;
        ndxr_group_header header;
        std::vector<render_group> render_groups;
    };

    std::vector<group> groups(header.groups_count);

    for (int i = 0; i < header.groups_count; ++i)
    {
        //print_data(reader, reader.get_offset(), sizeof(group_header));

        //unknown_struct &u = 
        groups[i].header = reader.read<ndxr_group_header>();

        //float *f = groups[i].header.origin;
        //test.add_point(nya_math::vec3(f[0], f[1], f[2]), nya_math::vec4(0, 0, 1, 1));
        //printf("origin: %f %f %f\n", f[0], f[1], f[2]);

        //nya_math::aabb bb; bb.delta = nya_math::vec3(1, 1, 1)*groups[i].header.bbox_size;
        //f = groups[i].header.bbox_origin; bb.origin = nya_math::vec3(f[0], f[1], f[2]); test.add_aabb(bb);
        //printf("bbox: %f %f %f | %f\n", f[0], f[1], f[2], groups[i].header.bbox_size);
    }

    //printf("subgroups\n");

    for (int i = 0; i < header.groups_count; ++i)
    {
        group &g = groups[i];
        g.render_groups.resize(g.header.render_groups_count);
        reader.seek(g.header.offset_to_render_groups);
        //printf("%d %ld %d\n", i, reader.get_offset(), g.header.offset_to_substruct+12);
        for (int k = 0; k < int(g.render_groups.size()); ++k)
            g.render_groups[k].header = reader.read<ndxr_render_group_header>();
    }

    //print_data(reader, reader.get_offset(), header.offset_to_indices + 48 - reader.get_offset());

    for (int i = 0; i < header.groups_count; ++i)
    {
        group &g = groups[i];
        for (int k = 0; k < int(g.render_groups.size()); ++k)
        {
            render_group &rg = g.render_groups[k];
            reader.seek(rg.header.offset_to_sub_struct2);
            //print_data(reader, reader.get_offset(), sizeof(unknown_substruct2_info));

            rg.substruct2_info = reader.read<unknown_substruct2_info>();
            rg.unknown_structs.resize(rg.substruct2_info.unknown_substruct2_count);
            for (int j = 0; j < rg.substruct2_info.unknown_substruct2_count; ++j)
                rg.unknown_structs[j] = reader.read<unknown_substruct2>();

            //print_data(reader, reader.get_offset(), 64);

            while(reader.check_remained(sizeof(ndxr_material_param)))
            {
                //print_data(reader, reader.get_offset(), sizeof(ndxr_material_param));
                material_param p;
                p.data = reader.read<ndxr_material_param>();
                rg.material_params.push_back(p);

                if (!p.data.unknown_32_or_zero)
                    break;
            }

            //printf("material params: %ld\n", rg.material_params.size());

            //print_data(reader, reader.get_offset(), 64);
        }
    }

    reader.seek(header.offset_to_indices + 48 + header.indices_buffer_size +header.vertices_buffer_size );
    //print_data(reader, reader.get_offset(), reader.get_remained());

    size_t strings_offset = header.offset_to_indices + 48 + header.indices_buffer_size +header.vertices_buffer_size;
    for (int i = 0; i < groups.size(); ++i)
    {
        group &g = groups[i];
        reader.seek(strings_offset+g.header.offset_to_name);
        g.name = reader.read_string();

        for (int k = 0; k < g.render_groups.size(); ++k)
        {
            render_group &rg = g.render_groups[k];
            for (int j = 0; j < rg.material_params.size(); ++j)
            {
                reader.seek(strings_offset + rg.material_params[j].data.offset_to_name);
                rg.material_params[j].name = reader.read_string();
            }
        }
    }

    lods.resize(lods.size() + 1);
    lod &l = lods.back();

    nya_scene::shared_mesh mesh;

    mesh.materials.resize(1);
    auto &mat = mesh.materials.back();
    nya_scene::shader plane_shader;
    plane_shader.load("shaders/player_plane.nsh");
    mat.set_shader(plane_shader);
    mat.set_param(mat.get_param_idx("light dir"), light_dir.x, light_dir.y, light_dir.z, 0.0f);
    mat.set_cull_face(true, nya_render::cull_face::ccw);
    //mat.set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha); //ToDo: transparency
    mesh.groups.resize(header.groups_count);
    l.groups.resize(header.groups_count);

    if (load_data.skeletons.size() == load_data.ndxr_count)
        mesh.skeleton = load_data.skeletons[lods.size() - 1].skeleton;

    struct vert
    {
        float pos[3];
        float tc[2];
        ushort normal[4]; //half float
    };

    std::vector<vert> verts;
    std::vector<ushort> indices;

    for (int i = 0; i < groups.size(); ++i)
    {
        group &gf = groups[i];
        auto &g = mesh.groups[i];
        g.material_idx = 0;
        l.groups[i].bone_idx = gf.header.bone_idx;
        g.offset = uint(indices.size());
        uint last_icount = 0;
        for (int j = 0; j < gf.render_groups.size(); ++j)
        {
            render_group &rgf = gf.render_groups[j];

            if (!rgf.header.vcount || !rgf.header.icount)
                continue;

            //if (gf.header.unknown3[2] != 15) //test
            //  continue;

            reader.seek(header.offset_to_indices + 48 + header.indices_buffer_size + rgf.header.vbuf_offset);

            const float *ndxr_verts = (float *)reader.get_data();

            const size_t first_index = verts.size();
            verts.resize(first_index+rgf.header.vcount);

            switch(rgf.header.vertex_format)
            {
                case 4358:
                    for (int i = 0; i < rgf.header.vcount; ++i)
                    {
                        memcpy(verts[i + first_index].pos, &ndxr_verts[i * 7], sizeof(verts[0].pos));
                        memcpy(verts[i + first_index].tc, &ndxr_verts[i * 7 + 5], sizeof(verts[0].tc));
                        memcpy(verts[i + first_index].normal, &ndxr_verts[i * 7 + 3], sizeof(verts[0].normal));
                    }
                    break;

                case 4359:
                    for (int i = 0; i < rgf.header.vcount; ++i)
                    {
                        memcpy(verts[i + first_index].pos, &ndxr_verts[i * 11], sizeof(verts[0].pos));
                        memcpy(verts[i + first_index].tc, &ndxr_verts[i * 11 + 9], sizeof(verts[0].tc));
                        memcpy(verts[i + first_index].normal, &ndxr_verts[i * 11 + 3], sizeof(verts[0].normal));
                    }
                    break;

               // case 4865: stride = 11 * 4; break;
                //case 4371:
                //  stride = 8*sizeof(float), rg.vbo.set_tc(0, 4 * sizeof(float), 3); break;
                //case 4096:
                //  stride = 3*sizeof(float), rg.vbo.set_tc(0, 4 * sizeof(float), 3); break;
                
                default:
                    //printf("ERROR: invalid stride. Vertex format: %d\n", rgf.header.vertex_format);
                    continue;
            }

            reader.seek(header.offset_to_indices + 48 + rgf.header.ibuf_offset); //rgf.header.icount
            const ushort *ndxr_indices = (ushort *)reader.get_data();

            if (j > 0)
            {
                indices.push_back(indices.back());
                indices.push_back(first_index + ndxr_indices[0]);
                if ( (indices.size() - g.offset) % 2 )
                    indices.push_back(indices.back());
            }

            uint ind_offset = uint(indices.size());
            uint ind_size = rgf.header.icount;
            indices.resize(ind_offset + ind_size);
            for (int i = 0; i < ind_size; ++i)
                indices[i + ind_offset] = first_index + ndxr_indices[i];
        }

        g.count = uint(indices.size()) - g.offset;
        g.elem_type = nya_render::vbo::triangle_strip;
        g.name = gf.name;
    }

    assert(verts.size() < 65535);

    if (indices.empty())
        return false;

    mesh.vbo.set_tc(0, sizeof(verts[0].pos), 2);
    mesh.vbo.set_normals(sizeof(verts[0].pos) + sizeof(verts[0].tc), nya_render::vbo::float16);
    mesh.vbo.set_vertex_data(&verts[0], sizeof(verts[0]), uint(verts.size()));

    mesh.vbo.set_index_data(&indices[0], nya_render::vbo::index2b, uint(indices.size()));

    l.mesh.create(mesh);

    if (load_data.animations.size() == load_data.ndxr_count)
    {
        auto anims = load_data.animations[lods.size() - 1].animations;
        assert(!anims.empty());

        int layer = 0;
        for (auto &a: anims)
        {
            assert(a.first == "basepose" || a.first.length() == 4);
            assert(a.first != "base");

            union { uint u; char c[4]; } hash_id;
            for (int i = 0; i < 4; ++i) hash_id.c[i] = a.first[3 - i];

            if (hash_id.u == 'swp1' || hash_id.u == 'swp2') continue; //ToDo

            auto &la = l.anims[hash_id.u];
            la.layer = layer;
            la.duration = a.second.get_duration();
            la.inv_duration = a.second.get_duration() > 0 ? 1.0f / a.second.get_duration() : 0.0f;
            a.second.set_speed(0.0f);
            a.second.set_loop(false);
            l.mesh.set_anim(a.second, layer++);

            //if (lods.size() == 1) printf("anim %s\n", a.first.c_str());
        }

        l.mesh.update(0);
    }

    return true;
}

//------------------------------------------------------------

void ndxr_mesh::draw(int lod_idx)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return;

    lod &l = lods[lod_idx];

    for (int i = 0; i < l.groups.size(); ++i)
    {
        lod::group &g = l.groups[i];

        if (g.bone_idx >= 0)
        {
            l.mesh.set_pos(m_pos + m_rot.rotate(l.mesh.get_bone_pos(g.bone_idx, true)));
            l.mesh.set_rot(m_rot * l.mesh.get_bone_rot(g.bone_idx, true));
            l.mesh.draw(i);
            continue;
        }

        l.mesh.set_pos(m_pos);
        l.mesh.set_rot(m_rot);
        l.mesh.draw(i);
    }
}

//------------------------------------------------------------

void ndxr_mesh::update(int dt)
{
    for (auto &l: lods)
        l.mesh.update(dt);
}

//------------------------------------------------------------

void ndxr_mesh::set_anim_speed(int lod_idx, unsigned int anim_hash_id, float speed)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return;

    auto &l = lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return;

    auto ma = l.mesh.get_anim(a->second.layer);
    if (!ma.is_valid())
        return;

    ma->set_speed(speed);
}

//------------------------------------------------------------

float ndxr_mesh::get_relative_anim_time(int lod_idx, unsigned int anim_hash_id)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return 0.0f;

    auto &l = lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return 0.0f;

    return l.mesh.get_anim_time(a->second.layer) * a->second.inv_duration;
}

//------------------------------------------------------------

void ndxr_mesh::set_relative_anim_time(int lod_idx, unsigned int anim_hash_id, float time)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return;

    auto &l = lods[lod_idx];
    auto a = l.anims.find(anim_hash_id);
    if (a == l.anims.end())
        return;

    l.mesh.set_anim_time(time * a->second.duration, a->second.layer);
}

//------------------------------------------------------------

bool ndxr_mesh::has_anim(int lod_idx, unsigned int anim_hash_id)
{
    if (lod_idx < 0 || lod_idx >= lods.size())
        return false;

    return lods[lod_idx].anims.find(anim_hash_id) != lods[lod_idx].anims.end();
}

//------------------------------------------------------------
