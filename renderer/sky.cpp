//
// open horizon -- undefined_darkness@outlook.com
//

#include "sky.h"
#include "scene/camera.h"
#include "shared.h"
#include "math/constants.h"
#include "util/location.h"

namespace renderer
{
//------------------------------------------------------------

class sph
{
public:
    bool load(const char *file_name)
    {
        auto r = load_resource(file_name);
        if (!r.get_data())
            return false;

        const bool result = load(r.get_data(), r.get_size());
        r.free();
        return result;
    }

    bool load(const void *buf, size_t size)
    {
        assert(size == sizeof(data));
        memcpy(&data, buf, size);

        if (data[0].radius < 1.0f || data[0].height < 1.0f) //endianness
        {
            for (size_t i = 0; i < sizeof(data)/sizeof(uint32_t); ++i)
                ((uint32_t *)&data)[i] = swap_bytes(((uint32_t *)&data)[i]);
        }

        return true;
    }

    struct color { unsigned char r, g, b, unused; };

    struct sph_data
    {
        float radius;
        float height;
        uint32_t zero[2];
        color colors[30];
        float k[10];

    };

    sph_data data[2];

    static color mix(const color &c0, const color &c1, float k)
    {
        color out;
        out.r = (unsigned char)nya_math::clamp(float(c0.r) + k * (float(c1.r) - float(c0.r)), 0.0f, 255.0f);
        out.g = (unsigned char)nya_math::clamp(float(c0.g) + k * (float(c1.g) - float(c0.g)), 0.0f, 255.0f);
        out.b = (unsigned char)nya_math::clamp(float(c0.b) + k * (float(c1.b) - float(c0.b)), 0.0f, 255.0f);
        return out;
    }

    struct color_table
    {
        color colors[58];
        color fog_color;

        color_table(const sph_data &d);
    };
};

//------------------------------------------------------------

sph::color_table::color_table(const sph_data &d)
{
    fog_color = mix(d.colors[10], d.colors[11], 0.87f);

    memset(colors, 0, sizeof(colors));

    colors[0] = colors[1] = d.colors[8];
    colors[16] = colors[17] = d.colors[9];
    int remap[] = { 20, 24, 28, 30, 21, 25, 29, 31 };
    for (size_t i = 0; i < sizeof(remap)/sizeof(remap[0]); ++i)
        colors[remap[i]] = d.colors[i];

    for (size_t i = 38; i < sizeof(colors)/sizeof(color); ++i)
        colors[i] = fog_color;

    for (int i = 1; i < 8; ++i)
    {
        static const float ks[] = { 0.980785f, 0.923880f, 0.831470f, 0.707107f, 0.555570f, 0.382683f, 0.195090f };
        const float k = ks[i-1];
        colors[i*2] = mix(colors[16], colors[0], k);
        colors[i*2+1] = mix(colors[17], colors[1], k);
    }

    colors[18] = mix(colors[16], colors[20], 0.5f);
    colors[19] = mix(colors[17], colors[21], 0.5f);

    colors[22] = mix(colors[20], colors[24], 0.5);
    colors[23] = mix(colors[21], colors[25], 0.5);

    for(int i = 1; i < 3; ++i)
    {
        const float k = i * 0.5f;
        colors[24 + i*2] = mix(colors[24], colors[28], k);
        colors[25 + i*2] = mix(colors[25], colors[29], k);
    }

    for (int i = 1; i < 5; ++i)
    {
        const float k = i * 0.25f;
        colors[30 + i*2] = mix(colors[30], colors[38], k);
        colors[31 + i*2] = mix(colors[31], colors[39], k);
    }
}

//------------------------------------------------------------

static const float hdr_k = 1.2995f / 255.0f;

//------------------------------------------------------------

struct solid_sphere
{
    struct vert { nya_math::vec3 pos, color; };
    std::vector<vert> vertices;
    std::vector<unsigned short> indices;

    solid_sphere(float radius, const sph::color_table &ct)
    {
        const unsigned int rings = 29, sectors = 32;

        const float ir = 1.0f/(float)(rings-1);
        const float is = 1.0f/(float)(sectors-1);

        vertices.resize(rings * sectors * 3);
        auto v = vertices.begin();
        for (int r = 0; r < rings; r++) for (int s = 0; s < sectors; s++)
        {
            const float sr = radius * sin( nya_math::constants::pi * r * ir );
            v->pos.x = sin(2*nya_math::constants::pi * s * is) * sr;
            v->pos.y = -radius * sin( -nya_math::constants::pi_2 + nya_math::constants::pi * r * ir );
            v->pos.z = cos(2*nya_math::constants::pi * s * is) * sr;

            const float k = (s < sectors / 2 ? s : sectors - s - 1) / float(sectors / 2 - 1);
            const auto c = sph::mix(ct.colors[r*2], ct.colors[r*2 + 1], k);

            v->color.x = c.r * hdr_k;
            v->color.y = c.g * hdr_k;
            v->color.z = c.b * hdr_k;

            ++v;
        }

        indices.resize(vertices.size()*2);
        auto i = indices.begin();
        for (int r = 0; r < rings-1; r++) for (int s = 0; s < sectors-1; s++)
        {
            *i++ = r * sectors + s;
            *i++ = r * sectors + (s+1);
            *i++ = (r+1) * sectors + (s+1);

            *i++ = r * sectors + s;
            *i++ = (r+1) * sectors + (s+1);
            *i++ = (r+1) * sectors + s;
        }
    }
};

//------------------------------------------------------------

bool sky_mesh::load(const char *name)
{
    if (!name)
        return false;

    renderer::sph t;

    if (is_native_location(name))
    {
        auto &p = get_native_location_provider(name);
        nya_memory::tmp_buffer_scoped res(load_resource(p.access("sky.sph")));
        t.load(res.get_data(), res.get_size());
    }
    else
        t.load((std::string("Map/sph_") + name + ".sph").c_str());

    renderer::sph::color_table ct(t.data[0]);

    solid_sphere s(20000.0, ct);

    m_mesh.set_vertex_data(&s.vertices[0], sizeof(solid_sphere::vert), (unsigned int)s.vertices.size());
    m_mesh.set_colors(sizeof(solid_sphere::vert::pos), 3);
    m_mesh.set_index_data(&s.indices[0], nya_render::vbo::index2b, (unsigned int)s.indices.size());

    auto dithering = shared::get_texture(shared::load_texture("PostProcess/dithering.nut"));

    m_material.get_default_pass().set_shader(nya_scene::shader("shaders/sky.nsh"));
    m_material.get_default_pass().get_state().zwrite = false;
    m_material.set_texture("dithering", dithering);

    m_fog_color.x = ct.fog_color.r * hdr_k;
    m_fog_color.y = ct.fog_color.g * hdr_k;
    m_fog_color.z = ct.fog_color.b * hdr_k;

    return true;
}

//------------------------------------------------------------

void sky_mesh::draw()
{
    nya_render::cull_face::disable();
    nya_scene::camera sky_cam=nya_scene::get_camera();
    sky_cam.set_pos(0, sky_cam.get_pos().y, 0);
    nya_render::set_modelview_matrix(sky_cam.get_view_matrix());

    m_material.internal().set();
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------

void sky_mesh::release()
{
    m_mesh.release();
    m_material.unload();
}

//------------------------------------------------------------

nya_math::vec3 sky_mesh::get_fog_color()
{
    return m_fog_color;
}

//------------------------------------------------------------

bool sun_mesh::init()
{
    auto &texture = shared::get_texture(shared::load_texture("Map/sun.nut"));
    auto &tex_data = texture.internal().get_shared_data();
    if (!tex_data.is_valid())
        return false;

    nya_render::texture tex = tex_data->tex;
    tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    auto &pass = m_material.get_default_pass();
    pass.set_shader(nya_scene::shader("shaders/sun.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    pass.get_state().zwrite=true;
    pass.get_state().depth_test=true;
    pass.get_state().depth_comparsion=nya_render::depth_test::not_greater;
    m_material.set_texture("diffuse", texture);

    struct vert
    {
        nya_math::vec2 pos;
    };

    vert verts[6];
    vert *v = &verts[0];

    v[0].pos = nya_math::vec2( -1.0f, -1.0f );
    v[1].pos = nya_math::vec2( -1.0f,  1.0f );
    v[2].pos = nya_math::vec2(  1.0f,  1.0f );
    v[3].pos = nya_math::vec2( -1.0f, -1.0f );
    v[4].pos = nya_math::vec2(  1.0f,  1.0f );
    v[5].pos = nya_math::vec2(  1.0f, -1.0f );

    m_mesh.set_vertex_data(verts, uint32_t(sizeof(vert)), uint32_t(sizeof(verts) / sizeof(verts[0])));
    m_mesh.set_vertices(0, 2);

    return true;
}

//------------------------------------------------------------

void sun_mesh::apply_location(const location_params &params)
{
    m_dir.xyz() = -params.sky.sun_dir;
    m_dir.w = params.sky.low.sun_flare_size; //params.sky.sun_size;
    m_material.set_param(m_material.get_param_idx("light dir"), m_dir);
    m_material.set_param(m_material.get_param_idx("color"), nya_math::vec3(1.0,1.0,1.0)); //params.sky.low.sun_flare_rgb
}

//------------------------------------------------------------

void sun_mesh::draw() const
{
    auto dir = nya_scene::get_camera().get_dir();
    dir.z = -dir.z;
    if (dir.dot(m_dir.xyz()) < 0.0)
        return;

    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_material.internal().set();
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------
}
