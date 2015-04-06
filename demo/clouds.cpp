//
// open horizon -- undefined_darkness@outlook.com
//

#include "clouds.h"

#include "scene/camera.h"
#include "memory/memory_reader.h"
#include "shared.h"
#include <assert.h>

//------------------------------------------------------------

bool effect_clouds::load(const char *location_name)
{
    if(!location_name)
        return false;

    const bool result = read_bdd((std::string("Effect/") + location_name + "/cloud_" + location_name + ".BDD").c_str(), m_clouds);
    if(!result)
        return false;

    std::vector<vert> verts;

    for (int i = 1; i <= 4; ++i)
    {
        if(i!=2) continue; //ToDo

        for (char j = 'A'; j <= 'E'; ++j)
        {
            m_obj_levels.resize(m_obj_levels.size() + 1);

            char buf[512];
            sprintf(buf, "Effect/%s/ObjCloud/Level%d_%c.BOC", location_name, i, j);

            nya_memory::tmp_buffer_ref res = shared::load_resource(buf);
            assert(res.get_size() > 0);
            nya_memory::memory_reader reader(res.get_data(), res.get_size());

            level_header header = reader.read<level_header>();
            auto &l = m_obj_levels.back();
            l.unknown = header.unknown;
            l.offset = (uint32_t)verts.size();
            l.count = header.count * 6;
            verts.resize(l.offset + l.count);
            for (int k = 0; k < header.count; ++k)
            {
                level_entry e = reader.read<level_entry>();
                m_obj_levels.back().entries.push_back(e);

                vert *v = &verts[l.offset + k * 6];

                v[0].dir = nya_math::vec2( -1.0f, -1.0f );
                v[1].dir = nya_math::vec2( -1.0f,  1.0f );
                v[2].dir = nya_math::vec2(  1.0f,  1.0f );
                v[3].dir = nya_math::vec2( -1.0f, -1.0f );
                v[4].dir = nya_math::vec2(  1.0f,  1.0f );
                v[5].dir = nya_math::vec2(  1.0f, -1.0f );

                for(int t = 0; t < 6; ++t)
                {
                    v[t].pos = e.pos;
                    v[t].size.x = e.size;// * 2.0f;
                    v[t].size.y = e.size;// * 2.0f;

                    auto tc=v[t].dir * 0.5f;
                    tc.x += 0.5f, tc.y += 0.5f;
                    tc.y = 1.0 - tc.y;

                    v[t].tc.x = tc.x * e.tc[2] + e.tc[0]; //main
                    v[t].tc.y = tc.y * e.tc[2] + e.tc[1];

                    v[t].tc.z = tc.x * e.tc[5] + e.tc[3]; //detail
                    v[t].tc.w = tc.y * e.tc[5] + e.tc[4];
                }
            }

            res.free();
        }

        break;
    }

    m_mesh.set_vertex_data(&verts[0], uint32_t(sizeof(verts[0])), uint32_t(verts.size()));
    m_mesh.set_vertices(0, 3);
    m_mesh.set_tc(0, 12, 4); //tc1, tc2
    m_mesh.set_tc(1, 12+16, 4); //dir, size

    m_shader.load("shaders/clouds.nsh");
    m_obj_tex = shared::get_texture(shared::load_texture((std::string("Effect/") + location_name + "/ObjCloud.nut").c_str()));
    m_flat_tex = shared::get_texture(shared::load_texture((std::string("Effect/") + location_name + "/FlatCloud.nut").c_str()));

    for (int i = 0; i < m_shader.internal().get_uniforms_count(); ++i)
    {
        auto &name = m_shader.internal().get_uniform(i).name;
        if (name == "pos")
            m_shader_pos = i;
        if (name == "up")
            m_shader_up = i;
        if (name == "right")
            m_shader_right = i;
    }

    m_dist_sort.resize(m_clouds.obj_clouds.size());

    return true;
}

void effect_clouds::draw_flat()
{
    //ToDo
}

void effect_clouds::draw_obj()
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    nya_render::depth_test::enable(nya_render::depth_test::less);
    nya_render::zwrite::disable();
    nya_render::cull_face::disable();
    nya_render::blend::enable(nya_render::blend::src_alpha, nya_render::blend::inv_src_alpha);

    m_mesh.bind();
    m_shader.internal().set();
    m_obj_tex.internal().set();

    //auto up=nya_scene::get_camera().get_rot().rotate(nya_math::vec3(0.0f,1.0f,0.0f));
    auto up=nya_math::vec3(0.0,1.0,0.0);
    m_shader.internal().set_uniform_value(m_shader_up, up.x, up.y, up.z, 0.0f);

    auto right = nya_scene::get_camera().get_dir();
    std::swap(right.x, right.z);
    right.y=0.0f;
    right.normalize();
    m_shader.internal().set_uniform_value(m_shader_right, right.x, right.y, right.z, 0.0f);

    for(uint32_t i = 0; i < m_dist_sort.size(); ++i)
    {
        auto cp = nya_scene::get_camera().get_pos();
        auto d = m_clouds.obj_clouds[i].second - nya_math::vec2(cp.x,cp.z);
        m_dist_sort[i].first = d * d;
        m_dist_sort[i].second = i;
    }

    std::sort(m_dist_sort.rbegin(), m_dist_sort.rend());

    for(const auto &d: m_dist_sort)
    {
        const auto &o = m_clouds.obj_clouds[d.second];
        auto &l = m_obj_levels[d.second % m_obj_levels.size()];
        m_shader.internal().set_uniform_value(m_shader_pos, o.second.x, 1500.0f, o.second.y, 0.0f);
        m_mesh.draw(l.offset,l.count);
    }

    nya_render::blend::disable();

    m_obj_tex.internal().unset();
    m_shader.internal().unset();
    m_mesh.unbind();
}

bool effect_clouds::read_bdd(const char *name, bdd &bdd_res)
{
    nya_memory::tmp_buffer_ref res = shared::load_resource(name);
    if(!res.get_size())
        return false;

    nya_memory::memory_reader reader(res.get_data(), res.get_size());

    bdd_header header = reader.read<bdd_header>();

    assert(header.unknown == '0001');
    assert(header.zero == 0);

    bdd_res.hiflat_clouds.resize(header.hiflat_clouds_count);
    for(auto &p: bdd_res.hiflat_clouds)
        p.x = reader.read<float>(), p.y = reader.read<float>();

    bdd_res.type2_pos.resize(header.type2_count);
    for(auto &p: bdd_res.type2_pos)
        p.x = reader.read<float>(), p.y = reader.read<float>();

    bdd_res.type3_pos.resize(header.type3_count);
    for(auto &p: bdd_res.type3_pos)
        p.x = reader.read<float>(), p.y = reader.read<float>();

    bdd_res.type4_pos.resize(header.type4_count);
    for(auto &p: bdd_res.type4_pos)
        p.x = reader.read<float>(), p.y = reader.read<float>();

    bdd_res.obj_clouds.resize(header.obj_clouds_count);
    for(auto &p: bdd_res.obj_clouds)
        p.first = reader.read<int>(), p.second.x = reader.read<float>(), p.second.y = reader.read<float>();

    assert(reader.get_remained()==0);

    bdd_res.header=header; //ToDo

    res.free();
    return true;
}

//------------------------------------------------------------
