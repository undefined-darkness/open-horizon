//
// open horizon -- undefined_darkness@outlook.com
//

#include "sky.h"
#include "scene/camera.h"
#include "shared.h"
#include "math/constants.h"

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

        struct sph_data
        {
            float radius;
            float height;
            uint zero[2];

            color colors[24];
            uint zero2[2];
            color colors2[3];
            uint zero3;

            //color colors[30];

            float k[10];

        } data[2];

        //colors count is 40 ? and coeffs is char and 40 too?

        assert(r.get_size() == sizeof(data));

        memcpy(&data, r.get_data(), r.get_size());

        for (uint i = 0; i < sizeof(data)/sizeof(uint32_t); ++i)
            ((uint32_t *)&data)[i] = swap_bytes(((uint32_t *)&data)[i]);

        for (auto &d: data)
        {
            assume(d.zero[0] == 0 && d.zero[1] == 0);
            //assume(d.zero2[0] == 0 && d.zero2[1] == 0);
            //assume(d.zero3 == 0);

            for (auto &c: d.colors) assume(c.unused == 0);
            //for (auto &c: d.colors2) assume(c.a == 0);
        }

        r.free();
        return true;
    }

private:
    struct color { unsigned char r, g, b, unused; };

    color mix(const color &c0, const color &c1, float k)
    {
        color out;
        out.r = c0.r + k * (c1.r - c0.r);
        out.g = c0.g + k * (c1.g - c0.g);
        out.b = c0.b + k * (c1.b - c0.b);
        return out;
    }
};

//------------------------------------------------------------

struct solid_sphere
{
    std::vector<nya_math::vec3> vertices;
    std::vector<unsigned short> indices;

    solid_sphere(float radius, unsigned int rings, unsigned int sectors)
    {
        const float ir = 1.0f/(float)(rings-1);
        const float is = 1.0f/(float)(sectors-1);

        vertices.resize(rings * sectors * 3);
        auto v = vertices.begin();
        for (int r = 0; r < rings; r++) for (int s = 0; s < sectors; s++)
        {
            float sr = radius * sin( nya_math::constants::pi * r * ir );
            v->x = cos(2*nya_math::constants::pi * s * is) * sr;
            v->y = radius * sin( -nya_math::constants::pi_2 + nya_math::constants::pi * r * ir );
            v->z = sin(2*nya_math::constants::pi * s * is) * sr;
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

bool sky_mesh::load(const char *name, const location_params &params)
{
    solid_sphere s(20000.0,32,24);

    m_mesh.set_vertex_data(&s.vertices[0], sizeof(float)*3, (unsigned int)s.vertices.size());
    m_mesh.set_index_data(&s.indices[0], nya_render::vbo::index2b, (unsigned int)s.indices.size());

    //ToDo: load from Map/sph_*.sph
    auto env = shared::get_texture(shared::load_texture((std::string("Map/envmap_") + name + ".nut").c_str()));

    auto dithering = shared::get_texture(shared::load_texture("PostProcess/dithering.nut"));

    m_material.get_default_pass().set_shader(nya_scene::shader("shaders/sky.nsh"));
    m_material.get_default_pass().get_state().zwrite = false;
    m_material.set_texture("diffuse", env);
    m_material.set_texture("dithering", dithering);

    nya_math::vec3 about_fog_color = params.sky.low.ambient * params.sky.low.skysphere_intensity; //ToDo
    m_material.set_param(m_material.get_param_idx("fog color"), about_fog_color);

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

bool sun_mesh::init()
{
    auto texture = shared::get_texture(shared::load_texture("Map/sun.nut"));
    auto tex_data = texture.internal().get_shared_data();
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