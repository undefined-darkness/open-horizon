//
// open horizon -- undefined_darkness@outlook.com
//

#include "lens_flare.h"

#include "scene/camera.h"
#include "shared.h"

namespace renderer
{
//------------------------------------------------------------

bool lens_flare::init(const nya_scene::texture_proxy &color, const nya_scene::texture_proxy &depth)
{
    assert(depth.is_valid());

    nya_memory::tmp_buffer_scoped res(load_resource("PostProcess/LensParam.bin"));
    if (!res.get_data())
        return false;

    params::memory_reader reader(res.get_data(), res.get_size());

    for (int i = 0; i < 16; ++i)
    {
        lens[i].position = reader.read<float>();
        lens[i].radius = reader.read<float>();
        lens[i].color = reader.read_color3_uint();
    }

    star_radius = reader.read<float>();
    star_color = reader.read_color3_uint();

    glow_radius = reader.read<float>();
    glow_color = reader.read_color3_uint();

    ring_specular = reader.read<float>();
    ring_shiness = reader.read<float>();
    ring_radius = reader.read<float>();

    f_min = reader.read<float>();
    f_max = reader.read<float>();
    aberration = reader.read<float>();

    auto texture = shared::get_texture(shared::load_texture("PostProcess/lens.nut"));
    auto tex_data = texture.internal().get_shared_data();
    if(!tex_data.is_valid())
        return false;

    nya_render::texture tex = tex_data->tex;
    tex.set_wrap(nya_render::texture::wrap_repeat_mirror, nya_render::texture::wrap_repeat_mirror);
    auto &pass = m_material.get_pass(m_material.add_pass(nya_scene::material::default_pass));
    pass.set_shader(nya_scene::shader("shaders/lens_flare.nsh"));
    pass.get_state().set_cull_face(false);
    pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);
    //pass.get_state().set_blend(true, nya_render::blend::src_alpha, nya_render::blend::one);
    pass.get_state().zwrite=false;
    pass.get_state().depth_test=false;
    m_material.set_texture("diffuse", texture);
    m_material.set_texture("color", color);
    m_material.set_texture("depth", depth);

    struct vert
    {
        nya_math::vec2 pos;
        float dist;
        float radius;
        nya_math::vec3 color;
        float special;
    };

    vert verts[(16+1)*6];

    for (int i = 0; i < 16; ++i)
    {
        vert *v = &verts[i * 6];

        v[0].pos = nya_math::vec2( -1.0f, -1.0f );
        v[1].pos = nya_math::vec2( -1.0f,  1.0f );
        v[2].pos = nya_math::vec2(  1.0f,  1.0f );
        v[3].pos = nya_math::vec2( -1.0f, -1.0f );
        v[4].pos = nya_math::vec2(  1.0f,  1.0f );
        v[5].pos = nya_math::vec2(  1.0f, -1.0f );

        for(int t = 0; t < 6; ++t)
        {
            if(i>0)
            {
                auto &l = lens[i-1];
                v[t].dist = l.position;
                v[t].radius = l.radius;
                v[t].color = l.color;
                v[t].special = 0.0;
            }
            else
            {
                v[t].dist = 1.0;
                v[t].radius = 0.2;
                v[t].color = nya_math::vec3(1.0, 1.0, 1.0);
                v[t].special = 1.0;
            }
        }
    }

    m_mesh.set_vertex_data(verts, uint32_t(sizeof(vert)), uint32_t(sizeof(verts) / sizeof(verts[0])));
    m_mesh.set_vertices(0, 4);
    m_mesh.set_tc(0, 16, 4);

    if (!m_dir_alpha.is_valid())
        m_dir_alpha.create();
    m_material.set_param(m_material.get_param_idx("light dir"), m_dir_alpha);

    return true;
}

//------------------------------------------------------------

void lens_flare::apply_location(const location_params &params)
{
    if (!m_dir_alpha.is_valid())
        m_dir_alpha.create();

    m_dir_alpha.set(-params.sky.sun_dir);
    m_brightness = params.sky.low.lens_brightness;
}

//------------------------------------------------------------

void lens_flare::draw() const
{
    if (!m_dir_alpha.is_valid())
        return;

    auto da = m_dir_alpha->get();

    auto dir = nya_scene::get_camera().get_dir();
    dir.z = -dir.z;
    float c = dir.dot(da.xyz());
    /*
     float a = acosf(c) * 180.0 / 3.1415;
     if(a>f_max)
     return;

     a - f_min;
     da.w = (1.0 - a /(f_max - f_min)) * m_brightness;
     */
    if (c < 0.0)
        return;

    c -= 0.9;
    //if(c < 0.0)
    //    return;

    da.w = c * m_brightness;

    m_dir_alpha.set(da);

    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_material.internal().set(nya_scene::material::default_pass);
    m_mesh.bind();
    m_mesh.draw();
    m_mesh.unbind();
    m_material.internal().unset();
}

//------------------------------------------------------------
}