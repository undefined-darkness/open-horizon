//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "aircraft_info.h"
#include "memory/tmp_buffer.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "math/scalar.h"
#include "containers/fhm.h"
#include "renderer/shared.h"
#include "renderer/scene.h"
#include "renderer/texture.h"
#include "util/xml.h"
#include <stdint.h>

namespace renderer
{
//------------------------------------------------------------

class color_baker
{
public:
    void set_base(const nya_scene::texture &tex) { m_mat.set_texture("base", tex); }
    void set_colx(const nya_scene::texture &tex) { m_mat.set_texture("colx", tex); }
    void set_coly(const nya_scene::texture &tex) { m_mat.set_texture("coly", tex); }
    void set_decal(const nya_scene::texture &tex) { m_mat.set_texture("decal", tex); }

    void set_colors(const unsigned char (&color)[6][3])
    {
        if (!m_colors.get_count())
            m_colors.set_count(6);

        for (int i = 0; i < 6; ++i)
            m_colors.set(i, vec3(color[i][0]/255.0f, color[i][1]/255.0f, color[i][2]/255.0f), 1.0f);
    }

    nya_scene::texture bake()
    {
        auto tp = m_mat.get_texture("base");
        if (!tp.is_valid())
            return nya_scene::texture();

        if (!m_mat.get_texture("decal").is_valid())
            m_mat.set_texture("decal", shared::get_black_texture());

        if (!m_mat.get_texture("colx").is_valid())
            m_mat.set_texture("colx", shared::get_black_texture());

        if (!m_mat.get_texture("coly").is_valid())
            m_mat.set_texture("coly", shared::get_black_texture());

        nya_render::fbo fbo;
        nya_render::screen_quad quad;
        quad.init();
        nya_scene::shared_texture res;
        res.tex.build_texture(0, tp->get_width(), tp->get_height(), nya_render::texture::color_rgba);
        fbo.set_color_target(res.tex);

        m_mat.get_default_pass().set_shader(nya_scene::shader("shaders/plane_camo.nsh"));
        m_mat.set_param_array(m_mat.get_param_idx("colors"), m_colors);

        fbo.bind();
        auto vp=nya_render::get_viewport();
        nya_render::set_viewport(0, 0, tp->get_width(), tp->get_height());
        m_mat.internal().set(nya_scene::material::default_pass);
        quad.draw();
        m_mat.internal().unset();
        nya_render::set_viewport(vp);
        fbo.unbind();

        quad.release();
        fbo.release();
        nya_scene::texture out;
        out.create(res);
        return out;
    }

private:
    nya_scene::material m_mat;
    nya_scene::material::param_array m_colors;
};

//------------------------------------------------------------

bool aircraft::load(const char *name, bool hd)
{
    auto info = aircraft_information::get().get_info(name);
    if (!info)
        return false;

    m_name = name;

    m_half_flaps_flag = m_name == "f22a" || m_name == "m21b" || m_name == "av8b";

    m_engine_lod_idx = 3; //ToDo: hd ? 3 : -1;
    if (m_name == "av8b")
        m_engine_lod_idx = -1;

    std::string name_str = (hd ? "p_" : "d_") + m_name;
    std::string name_tmp_str = "p_" + m_name; //ToDo: load d_ models
    m_mesh.load(name_tmp_str.c_str(), location_params());

    const std::string tex_pref = std::string("model_id/tdb/mech/") + (hd ? "plyr/" : "airp/");
    const std::string mesh_pref = std::string("model_id/mech/") + "plyr/"; //ToDo: + (player ? "plyr/" : "airp/");

    if (hd)
    {
        auto amb_name = tex_pref + name_str + "/00/" + name_str + "_00_amb.img";
        m_mesh.set_texture(0, "ambient", amb_name.c_str());
        m_mesh.set_texture(m_engine_lod_idx, "ambient", amb_name.c_str());
        auto norm_name = tex_pref + name_str + "/00/" + name_str + "_00_nor.img";
        m_mesh.set_texture(0, "normal", norm_name.c_str());
        m_mesh.set_texture(m_engine_lod_idx, "normal", norm_name.c_str());
    }
    else
    {
        m_mesh.set_texture(0, "ambient", shared::get_white_texture());
        m_mesh.set_texture(0, "normal", shared::get_normal_texture());
    }

    m_camera_offset = info->camera_offset;

    m_mesh.set_relative_anim_time(0, 'rudl', 0.5);
    m_mesh.set_relative_anim_time(0, 'rudr', 0.5);
    m_mesh.set_relative_anim_time(0, 'rudn', 0.5);

    m_mesh.set_relative_anim_time(0, 'vwgn', 1.0);
    m_mesh.update(0);
    m_wing_sw_off = get_bone_pos("clh2");
    m_mesh.set_relative_anim_time(0, 'vwgn', 0.0);
    m_mesh.update(0);
    m_wing_off = get_bone_pos("clh2");

    //ToDo: su24 spll splr

    //m_mesh.set_anim_speed(0, 'vctn', 0.5);
    //m_mesh.set_anim_speed(1, 'altr', 0.1);

    /*
    printf("%s bones\n", name);
    for (int i = 0; i < m_mesh.get_bones_count(0); ++i)
        printf("\t%s\n", m_mesh.get_bone_name(0, i));
    printf("\n");
    */

    //weapons

    m_msls_mount.clear();
    for (int i = 0; i < 2; ++i)
    {
        char name[] = "wms1";
        name[3] += i;

        wpn_mount m;
        m.bone_idx = m_mesh.find_bone_idx(0, name);
        m.visible = true;
        if (m.bone_idx >= 0)
            m_msls_mount.push_back(m);
    }

    m_special_mount.clear();
    for (int i = 0; i < 9; ++i)
    {
        char name[] = "ws01";
        name[3]+=i;
        wpn_mount m;
        for (int j = 0; j < 5; ++j)
        {
            m.bone_idx = m_mesh.find_bone_idx(0, name);
            if (m.bone_idx >= 0)
                break;
            ++name[2];
        }

        m.visible = true;
        if (m.bone_idx >= 0)
            m_special_mount.push_back(m);
    }

    m_mguns.clear();
    const int mgun_bone = m_mesh.get_bone_idx(0, "wgnn");
    if (mgun_bone >= 0)
    {
        mgun m;
        m.bone_idx = mgun_bone;
        m_mguns.push_back(m);
    }
    else
    {
        for (int i = 0; i < 9; ++i)
        {
            char name[] = "wgnn01";
            name[5]+=i;
            const int mgun_bone = m_mesh.find_bone_idx(0, name);
            if (mgun_bone < 0)
                break;

            mgun m;
            m.bone_idx = mgun_bone;
            m_mguns.push_back(m);
        }
    }

    //attitude
    m_adimx_bone_idx = m_mesh.get_bone_idx(1, "adimx1");
    if (m_adimx_bone_idx < 0) m_adimx_bone_idx = m_mesh.get_bone_idx(1, "adimx");
    m_adimx2_bone_idx = m_mesh.get_bone_idx(1, "adimx2");
    m_adimz_bone_idx = m_mesh.get_bone_idx(1, "adimz1");
    if (m_adimz_bone_idx < 0) m_adimz_bone_idx = m_mesh.get_bone_idx(1, "adimz");
    m_adimz2_bone_idx = m_mesh.get_bone_idx(1, "adimz2");
    m_adimxz_bone_idx = m_mesh.get_bone_idx(1, "adimxz1");
    if (m_adimxz_bone_idx < 0) m_adimxz_bone_idx = m_mesh.get_bone_idx(1, "adimxz");
    m_adimxz2_bone_idx = m_mesh.get_bone_idx(1, "adimxz2");

    m_trails[0].second = m_mesh.get_bone_idx(0, "clh2");
    m_trails[1].second = m_mesh.get_bone_idx(0, "clh3");

    for (auto &ei: info->engines)
    {
        std::string bname = "nzlp";
        if (info->engines.size() > 1)
            bname += std::to_string(m_engines.size() + 1);
        else
            m_tvc_param = ei.tvc / 180.0f * nya_math::constants::pi;
        m_engines.push_back({plane_engine(ei.radius, ei.dist, ei.yscale), m_mesh.get_bone_idx(0, bname.c_str())});
    }

    //ToDo: add single group in fhm_mesh instead
    if (hd)
    {
        int lods[] = {0, m_engine_lod_idx};
        for (auto &l: lods)
        {
            if (l < 0)
                continue;

            auto &mesh = m_mesh.get_mesh(l);
            for (int i = 0; i < mesh.get_groups_count(); ++i)
            {
                auto &mat = mesh.modify_material(i);
                if (mat.get_default_pass().get_state().blend)
                    continue;

                auto &p = mat.get_pass(mat.add_pass("caster"));
                p.set_shader("shaders/caster.nsh");
                p.get_state().set_cull_face(true, nya_render::cull_face::cw);
            }
        }
    }
    return true;
}

//------------------------------------------------------------

bool aircraft::set_decal(unsigned int color_idx, bool hd)
{
    auto info = aircraft_information::get().get_info(m_name.c_str());
    if (!info)
        return false;

    if (color_idx >= info->colors.size() + info->color_mods.size())
        return false;

    const int base_color_idx = color_idx < info->colors.size() ? color_idx : info->color_mods[color_idx - info->colors.size()].coledit_idx;

    assert(base_color_idx < info->colors.size());
    auto &color = info->colors[base_color_idx];

    const std::string tex_pref = std::string("model_id/tdb/mech/") + (hd ? "plyr/" : "airp/");
    const std::string mesh_pref = std::string("model_id/mech/") + "plyr/"; //ToDo: + (player ? "plyr/" : "airp/");

    char cfldr[256];
    sprintf(cfldr, "%02d", color.coledit_idx);
    std::string cfldr2 = std::string(cfldr) + (hd ? "" : "_l");

    std::string name_tmp_str = "p_" + m_name; //ToDo: load d_ models
    std::string name_str = (hd ? "p_" : "d_") + m_name;

    const std::string mat_name = mesh_pref + name_tmp_str + "/" + name_tmp_str + "_pcol" + cfldr + ".fhm";
    fhm_materials mat;
    mat.load(mat_name.c_str());
    assert(!mat.materials.empty());

    color_baker baker;

    nya_scene::texture spec_tex;

    if(!mat.textures.empty())
    {
        assert(mat.textures.size() == 2);

        baker.set_base(mat.textures[0]);
        spec_tex = mat.textures[1];

        // made up _pcoledit name to bind data.pac.xml entry somehow
        const std::string mat_name = mesh_pref + name_tmp_str + "/" + name_tmp_str + "_pcoledit" + cfldr + ".fhm";

        fhm_materials mat;
        mat.load(mat_name.c_str());
        assert(mat.textures.size() == 2);

        baker.set_colx(mat.textures[0]);
        baker.set_coly(mat.textures[1]);
    }
    else
    {
        baker.set_base((tex_pref + name_str + "/" + cfldr + "/" + name_str + "_" + cfldr2 + "_col.img").c_str());
        baker.set_colx((tex_pref + name_str + "/coledit" + cfldr + "/" + name_str + "_" + cfldr2 + "_mkx.img").c_str());
        baker.set_coly((tex_pref + name_str + "/coledit" + cfldr + "/" + name_str + "_" + cfldr2 + "_mky.img").c_str());

        spec_tex.load((tex_pref + name_str + "/" + cfldr + "/" + name_str + "_" + cfldr2 + "_spe.img").c_str());
    }

    baker.set_colors(color.colors);

    if (color_idx >= info->colors.size())
    {
        auto &cm = info->color_mods[color_idx - info->colors.size()];
        if (!cm.tex_name.empty())
            baker.set_decal(cm.load_texture(cm.tex_name));

        baker.set_colors(cm.colors);

        if (!cm.spec_name.empty())
        {
            color_baker spec_baker;
            spec_baker.set_base(spec_tex);
            spec_baker.set_decal(cm.load_texture(cm.spec_name));
            spec_tex = spec_baker.bake();
        }
    }

    m_mesh.set_material(0, mat.materials[0], "shaders/player_plane.nsh");
    if (m_engine_lod_idx >= 0 && mat.materials.size() > 2)
        m_mesh.set_material(m_engine_lod_idx, mat.materials[2], "shaders/player_plane.nsh");

    auto diff_tex = baker.bake();

    int lods[] = {0, m_engine_lod_idx};
    for (auto &l: lods)
    {
        if (l < 0)
            continue;

        m_mesh.set_texture(l, "diffuse", diff_tex );
        m_mesh.set_texture(l, "specular", spec_tex);
        m_mesh.set_texture(l, "shadows", shared::get_white_texture());
    }

    return true;
}

//------------------------------------------------------------

template<int size> nya_scene::texture tex_from_netimg(const netimg<size> &src)
{
    nya_scene::texture result;
    nya_memory::tmp_buffer_scoped buf(src.raw_size);
    src.decode(buf.get_data());
    result.build(buf.get_data(), src.get_width(), src.get_height(), nya_render::texture::color_rgba);
    return result;
}

//------------------------------------------------------------

bool aircraft::set_decal(const net_decal &decal)
{
    auto info = aircraft_information::get().get_info(m_name.c_str());
    if (!info)
        return false;

    const std::string tex_pref = std::string("model_id/tdb/mech/airp/");
    const std::string mesh_pref = std::string("model_id/mech/") + "plyr/"; //ToDo: + (player ? "plyr/" : "airp/");

    char cfldr[256];
    sprintf(cfldr, "%02d", decal.coledit_idx);
    std::string cfldr2 = std::string(cfldr) + "_l";

    std::string name_tmp_str = "p_" + m_name; //ToDo: load d_ models
    std::string name_str = "d_" + m_name;

    const std::string mat_name = mesh_pref + name_tmp_str + "/" + name_tmp_str + "_pcol" + cfldr + ".fhm";
    fhm_materials mat;
    mat.load(mat_name.c_str());
    assert(!mat.materials.empty());

    color_baker baker;

    nya_scene::texture spec_tex;

    if(!mat.textures.empty())
    {
        assert(mat.textures.size() == 2);

        baker.set_base(mat.textures[0]);
        spec_tex = mat.textures[1];

        // made up _pcoledit name to bind data.pac.xml entry somehow
        const std::string mat_name = mesh_pref + name_tmp_str + "/" + name_tmp_str + "_pcoledit" + cfldr + ".fhm";

        fhm_materials mat;
        mat.load(mat_name.c_str());
        assert(mat.textures.size() == 2);

        baker.set_colx(mat.textures[0]);
        baker.set_coly(mat.textures[1]);
    }
    else
    {
        baker.set_base((tex_pref + name_str + "/" + cfldr + "/" + name_str + "_" + cfldr2 + "_col.img").c_str());
        baker.set_colx((tex_pref + name_str + "/coledit" + cfldr + "/" + name_str + "_" + cfldr2 + "_mkx.img").c_str());
        baker.set_coly((tex_pref + name_str + "/coledit" + cfldr + "/" + name_str + "_" + cfldr2 + "_mky.img").c_str());

        spec_tex.load((tex_pref + name_str + "/" + cfldr + "/" + name_str + "_" + cfldr2 + "_spe.img").c_str());
    }

    baker.set_colors(decal.colors);
    baker.set_decal(tex_from_netimg(decal.diffuse));
    auto diff_tex = baker.bake();

    color_baker spec_baker;
    spec_baker.set_base(spec_tex);
    spec_baker.set_decal(tex_from_netimg(decal.specular));
    spec_tex = spec_baker.bake();

    m_mesh.set_material(0, mat.materials[0], "shaders/player_plane.nsh");
    if (m_engine_lod_idx >= 0 && mat.materials.size() > 2)
        m_mesh.set_material(m_engine_lod_idx, mat.materials[2], "shaders/player_plane.nsh");

    int lods[] = {0, m_engine_lod_idx};
    for (auto &l: lods)
    {
        if (l < 0)
            continue;

        m_mesh.set_texture(l, "diffuse", diff_tex);
        m_mesh.set_texture(l, "specular", spec_tex);
        m_mesh.set_texture(l, "shadows", shared::get_white_texture());
    }

    return true;
}

//------------------------------------------------------------

void aircraft::load_missile(const char *name, const location_params &params)
{
    m_missile.load((std::string("w_") + name).c_str(), params);
}

//------------------------------------------------------------

void aircraft::load_special(const char *name, const location_params &params)
{
    m_special.load((std::string("w_") + name).c_str(), params);

    m_mgps.clear();
    if (strncmp(name, "gpd", 3) == 0)
    {
        int time_off = 0;
        for (auto &s: m_special_mount)
        {
            mgun g;
            g.bone_idx = s.bone_idx;
            g.flash.update(nya_math::vec3(), nya_math::vec3(), (++time_off) * 100);
            m_mgps.push_back(g);
        }
    }
}

//------------------------------------------------------------

void aircraft::apply_location(const nya_scene::texture &ibl, const nya_scene::texture &env, const location_params &params)
{
    m_mesh.set_texture(0, "reflection", env);
    m_mesh.set_texture(m_engine_lod_idx, "reflection", env);
    m_mesh.set_texture(0, "ibl", ibl);
    m_mesh.set_texture(m_engine_lod_idx, "ibl", ibl);
}

//------------------------------------------------------------

void aircraft::set_dead(bool dead)
{
    m_dead = dead;
    if (m_dead)
    {
        m_fire_mgun = false;
        m_fire_mgp = false;
        m_fire_trail = fire_trail(5.0f);
    }
}

//------------------------------------------------------------

unsigned int aircraft::get_colors_count(const char *plane_name)
{
    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return 0;

    return (unsigned int)(info->colors.size() + info->color_mods.size());
}

//------------------------------------------------------------

std::string aircraft::get_color_name(const char *plane_name, int idx)
{
    if (idx < 0)
        return "";

    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return "";

    if (idx < info->colors.size())
        return info->colors[idx].name;

    idx -= info->colors.size();
    if (idx < info->color_mods.size())
        return info->color_mods[idx].name;

    return "";
}

//------------------------------------------------------------

std::string aircraft::get_sound_name(const char *plane_name)
{
    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return "";

    return info->sound;
}

//------------------------------------------------------------

std::string aircraft::get_voice_name(const char *plane_name)
{
    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return "";

    return info->voice;
}

//------------------------------------------------------------

aircraft::net_decal aircraft::get_net_decal(const char *plane_name, int color_idx)
{
    net_decal result;
    const auto &info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return result;

    if (color_idx < (int)info->colors.size())
    {
        memcpy(result.colors, info->colors[color_idx].colors, sizeof(result.colors));
        result.coledit_idx = info->colors[color_idx].coledit_idx;
        return result;
    }

    if (color_idx >= info->colors.size() + info->color_mods.size())
        return result;

    const auto &cm = info->color_mods[color_idx - (int)info->colors.size()];
    memcpy(result.colors, cm.colors, sizeof(result.colors));
    result.decal_crc32 = cm.decal_crc32;
    result.spec_crc32 = cm.specular_crc32;
    result.coledit_idx = info->colors[cm.coledit_idx].coledit_idx;

    if (!cm.tex_name.empty())
        cm.load_netimg(cm.tex_name, result.diffuse);

    if (!cm.spec_name.empty())
        cm.load_netimg(cm.spec_name, result.specular);

    return result;
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_bone_pos(const char *name)
{
    return m_mesh.get_bone_pos(0, m_mesh.get_bone_idx(0, name));
}

//------------------------------------------------------------

int aircraft::get_mguns_count() const
{
    return (int)m_mguns.size();
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_mgun_pos(int idx)
{
    if (idx<0 || idx>=get_mguns_count())
        return nya_math::vec3();

    return m_mesh.get_bone_pos(0, m_mguns[idx].bone_idx);
}

//------------------------------------------------------------

const renderer::model &aircraft::get_missile_model()
{
    return m_missile;
}

//------------------------------------------------------------

const renderer::model &aircraft::get_special_model()
{
    return m_special;
}

//------------------------------------------------------------

void aircraft::set_elev(float left, float right)
{
    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'elvl', (left * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'elvr', (right * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvr')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'elvn', (left * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvn')) * anim_speed_k);

    m_mesh.set_anim_speed(3, 'vctl', (left * 0.5f + 0.5f - m_mesh.get_relative_anim_time(3, 'vctl')) * anim_speed_k);
    m_mesh.set_anim_speed(3, 'vctr', (right * 0.5f + 0.5f - m_mesh.get_relative_anim_time(3, 'vctr')) * anim_speed_k);
    m_mesh.set_anim_speed(3, 'vctn', (((left + right) * 0.5) * 0.5f + 0.5f - m_mesh.get_relative_anim_time(3, 'vctn')) * anim_speed_k);
}

//------------------------------------------------------------

void aircraft::set_aileron(float left, float right)
{
    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'alrl', (left * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'alrr', (right * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrr')) * anim_speed_k);
}

//------------------------------------------------------------

void aircraft::set_rudder(float left, float right, float center)
{
    bool has_air_brake = m_mesh.has_anim(0,'abkn');
    if (!has_air_brake)
        has_air_brake = m_mesh.has_anim(0,'spll') || m_mesh.has_anim(0,'splr');

    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'rudl', ((has_air_brake ? center : left) * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudr', ((has_air_brake ? center : right) * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudr')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudn', (center * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudn')) * anim_speed_k);

    m_mesh.set_anim_speed(3, 'vcth', (center * 0.5 + 0.5 - m_mesh.get_relative_anim_time(3, 'vcth')) * anim_speed_k);
}

//------------------------------------------------------------

void aircraft::set_brake(float value)
{
    m_mesh.set_anim_speed(0, 'abkn', value>0.7f ? 1.0: -1.0);
    m_mesh.set_anim_speed(0, 'spll', value>0.7f ? 1.0: -1.0);
    m_mesh.set_anim_speed(0, 'splr', value>0.7f ? 1.0: -1.0);
}

//------------------------------------------------------------

void aircraft::set_flaperon(float value)
{
    float flap_anim_speed = 0.5f;
    m_mesh.set_anim_speed(0, 'lefn', -value * flap_anim_speed);
    m_mesh.set_anim_speed(0, 'tefn', value * flap_anim_speed);

    if (m_half_flaps_flag && m_mesh.get_relative_anim_time(0, 'tefn') > 0.5)
        m_mesh.set_relative_anim_time(0, 'tefn', 0.5);
}

//------------------------------------------------------------

void aircraft::set_canard(float value)
{
    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'cndl', (value * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'cndr', (value * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndr')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'cndn', (value * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndn')) * anim_speed_k);
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_wing_offset()
{
    return nya_math::vec3::lerp(m_wing_off, m_wing_sw_off, m_mesh.get_relative_anim_time(0, 'vwgn'));
}

//------------------------------------------------------------

void aircraft::set_wing_sweep(float value)
{
    float wing_anim_speed = 0.8f;
    m_mesh.set_anim_speed(0, 'vwgn', value * wing_anim_speed);
}

//------------------------------------------------------------

void aircraft::set_intake_ramp(float value)
{
    m_mesh.set_anim_speed(0, 'rmpn', value > 0 ? 0.7f : -0.5f);
}

//------------------------------------------------------------

void aircraft::set_thrust(float value)
{
    m_engine_rpm_n = value;

    const float thrust_start = 0.97f;
    m_engine_thrust = value > thrust_start ? (value - thrust_start) / (1.0f - thrust_start) : 0.0f;

    value *= 1.2;
    if (value > 1.0f)
        value = 2.0f - value;

    m_mesh.set_anim_weight(3, 'nzln', value);
}

//------------------------------------------------------------

void aircraft::set_special_bay(bool value)
{
    m_mesh.set_anim_speed(0, 'spwc', value ? 1.0f : -1.0f);
    m_mesh.set_anim_speed(0, 'swcc', value ? 1.0f : -1.0f);
    m_mesh.set_anim_speed(0, 'swc3', value ? 1.0f : -1.0f);
}

//------------------------------------------------------------

void aircraft::set_missile_bay(bool value)
{
    m_mesh.set_anim_speed(0, 'misc', value ? 1.0f : -1.0f);
}

//------------------------------------------------------------

void aircraft::set_mgun_bay(bool value)
{
    m_mesh.set_anim_speed(0, 'gunc', value ? 5.0f : -5.0f);
}

//------------------------------------------------------------

bool aircraft::has_special_bay()
{
    return m_mesh.has_anim(0, 'spwc') || m_mesh.has_anim(0, 'swcc') || m_mesh.has_anim(0, 'swc3');
}

//------------------------------------------------------------

bool aircraft::is_special_bay_opened()
{
    return m_mesh.get_relative_anim_time(0, 'spwc') > 0.999f
        || m_mesh.get_relative_anim_time(0, 'swcc') > 0.999f
        || m_mesh.get_relative_anim_time(0, 'swc3') > 0.999f;
}

//------------------------------------------------------------

bool aircraft::is_special_bay_closed()
{
    return m_mesh.get_relative_anim_time(0, 'spwc') < 0.01f
        && m_mesh.get_relative_anim_time(0, 'swcc') < 0.01f
        && m_mesh.get_relative_anim_time(0, 'swc3') < 0.01f;
}

//------------------------------------------------------------

bool aircraft::is_missile_ready()
{
    if (!m_mesh.has_anim(0, 'misc'))
        return true;

    return m_mesh.get_relative_anim_time(0, 'misc') >= 0.999f;
}

//------------------------------------------------------------

bool aircraft::is_mgun_ready()
{
    if (m_mguns.empty())
        return false;

    if (!m_mesh.has_anim(0, 'gunc'))
        return true;

    return m_mesh.get_relative_anim_time(0, 'gunc') >= 0.999f;
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_missile_mount_pos(int idx)
{
    if (idx < 0 || idx >= m_msls_mount.size())
        return nya_math::vec3();

    return m_mesh.get_bone_pos(0, m_msls_mount[idx].bone_idx);
}

//------------------------------------------------------------

nya_math::quat aircraft::get_missile_mount_rot(int idx)
{
    if (idx < 0 || idx >= m_msls_mount.size())
        return nya_math::quat();

    return m_mesh.get_bone_rot(0, m_msls_mount[idx].bone_idx);
}

//------------------------------------------------------------

void aircraft::set_missile_visible(int idx, bool visible)
{
    if (idx >= 0 && idx < m_msls_mount.size())
        m_msls_mount[idx].visible = visible;
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_special_mount_pos(int idx)
{
    if (idx < 0 || idx >= m_special_mount.size())
        return nya_math::vec3();

    return m_mesh.get_bone_pos(0, m_special_mount[idx].bone_idx);
}

//------------------------------------------------------------

nya_math::quat aircraft::get_special_mount_rot(int idx)
{
    if (idx < 0 || idx >= m_special_mount.size())
        return nya_math::quat();

    return m_mesh.get_bone_rot(0, m_special_mount[idx].bone_idx);
}

//------------------------------------------------------------

void aircraft::set_special_visible(int idx, bool visible)
{
    if (idx < 0)
    {
        for (auto &s: m_special_mount)
            s.visible = visible;
    }
    else if (idx < m_special_mount.size())
        m_special_mount[idx].visible = visible;
}

//------------------------------------------------------------

void aircraft::update(int dt)
{
    //cockpit animations
    auto pyr = m_mesh.get_rot().get_euler();

    //attitude indicator
    const nya_math::quat atti_x(pyr.x, 0.0, 0.0);
    const nya_math::quat atti_z(0.0, 0.0, -pyr.z);
    const nya_math::quat atti_xz = atti_z * atti_x;
    m_mesh.set_bone_rot(1, m_adimxz_bone_idx, atti_xz);
    m_mesh.set_bone_rot(1, m_adimxz2_bone_idx, atti_xz);
    m_mesh.set_bone_rot(1, m_adimx_bone_idx, atti_x);
    m_mesh.set_bone_rot(1, m_adimx2_bone_idx, atti_x);
    m_mesh.set_bone_rot(1, m_adimz_bone_idx, atti_z);
    m_mesh.set_bone_rot(1, m_adimz2_bone_idx, atti_z);

    pyr /= (2.0 * nya_math::constants::pi);

    if (pyr.x < 0.0) pyr.x += 1.0;
    if (pyr.y < 0.0) pyr.y += 1.0;
    if (pyr.z < 0.0) pyr.z += 1.0;

    //compass
    m_mesh.set_relative_anim_time(1, 'cmps', 1.0f-pyr.y);

    //speed
    m_mesh.set_relative_anim_time(1, 'aspk', m_speed / 2000.0); //ToDo: adjust max speed

    //clocks
    unsigned int seconds = m_time / 1000;
    m_mesh.set_relative_anim_time(1, 'acks', (seconds % 60 / 60.0));
    m_mesh.set_relative_anim_time(1, 'ackm', ((seconds / 60) % 60 / (60.0)));
    m_mesh.set_relative_anim_time(1, 'ackh', ((seconds / 60 / 60) % 12 / (12.0)));
    m_time += dt;

    auto h = m_mesh.get_pos().y;

    //barometric altimeter
    m_mesh.set_relative_anim_time(1, 'altk', h / 10000.0f); //ToDo: adjust max height
    m_mesh.set_relative_anim_time(1, 'altm', h / 10000.0f); //ToDo: adjust max height
    //radio altimeter
    m_mesh.set_relative_anim_time(1, 'altr', h / 10000.0f); //ToDo: adjust max height, trace
    m_mesh.set_relative_anim_time(1, 'alts', h / 10000.0f); //ToDo: adjust max height, trace

    //variometer
    m_mesh.set_relative_anim_time(1, 'vspm', m_vel.y / 1000.0f * 0.5f + 0.5f);

    //engine rpm
    m_mesh.set_relative_anim_time(1, 'erpm', m_engine_rpm_n * 0.9f);
    m_mesh.set_relative_anim_time(1, 'ecsp', m_engine_rpm_n * 0.9f);

    //ToDo: intt, mach, vspk, fufl, fasp, eopr

    //ToDo
    /*
    if (dt)
    {
        //accel
        //float accel = (m_speed - prev_speed) / float(dt);
        //m_mesh.set_relative_anim_time(1, 'accm', accel * 0.5 + 0.5);
    }
    */

    m_mesh.set_relative_anim_time(1, 'aoam', nya_math::clamp(0.2f + 0.8f * 10.0f * m_aoa / nya_math::constants::pi, 0.0f, 1.0f));

    m_mesh.update(dt);

    if (m_dead)
        m_fire_trail.update(get_pos(), dt);

    if (m_fire_mgun)
    {
        auto dir = get_rot().rotate(vec3(0.0f, 0.0f, 1.0f));
        for (auto &m: m_mguns)
            m.flash.update(m_mesh.get_bone_pos(0, m.bone_idx), dir, dt);
    }

    if (m_fire_mgp)
    {
        auto dir = get_rot().rotate(vec3(0.0f, 0.0f, 1.0f));
        for (auto &m: m_mgps)
            m.flash.update(m_mesh.get_bone_pos(0, m.bone_idx) + dir * 1.5f, dir, dt);
    }

    for (auto &e: m_engines)
    {
        e.first.update(m_mesh.get_bone_pos(0, e.second), m_mesh.get_bone_rot(0, e.second), m_engine_thrust, dt);
        if (m_tvc_param.y > 0.01f)
        {
            const float n = m_mesh.get_relative_anim_time(3, 'vctn');
            const float l = ((m_mesh.get_relative_anim_time(3, 'vctl') + n) * 2.0f - 1.0f) * m_tvc_param.y;
            const float r = ((m_mesh.get_relative_anim_time(3, 'vctr') + n) * 2.0f - 1.0f) * m_tvc_param.y;
            const float h = (m_mesh.get_relative_anim_time(3, 'vcth') * 2.0f - 1.0f) * m_tvc_param.x;
            e.first.update_tvc(0, nya_math::vec3(sinf(h), sinf(l), -cosf(h) * cosf(l)));
            e.first.update_tvc(1, nya_math::vec3(sinf(h), sinf(r), -cosf(h) * cosf(r)));
        }
    }
}

//------------------------------------------------------------

void aircraft::update_trail(int dt, scene &s)
{
    const bool has_trail = !m_dead && fabsf(m_aoa) > 0.1f;
    if (has_trail)
    {
        m_has_trail = true;
        for (auto &t: m_trails)
        {
            auto p = m_mesh.get_bone_pos(0, t.second);
            t.first.update(p, dt);

            if (t.first.is_full())
            {
                s.add_trail(t.first);
                t.first.clear();
                t.first.update(p, dt);
            }
        }
    }
    else if(m_has_trail)
    {
        m_has_trail = false;
        for (auto &t: m_trails)
            s.add_trail(t.first);
    }
}

//------------------------------------------------------------

void aircraft::setup_shadows(const nya_scene::texture &tex, const nya_math::mat4 &m)
{
    const int lods[] = { 0, m_engine_lod_idx };
    for (auto l: lods)
    {
        if (l < 0)
            continue;

        auto &mesh = m_mesh.get_mesh(l);
        for (int i = 0; i < mesh.get_groups_count(); ++i)
        {
            auto &mat = mesh.modify_material(i);
            mat.set_texture("shadows", tex);
            static nya_scene::material::param_array a;
            a.set_count(4);
            for (int j = 0; j < 4; ++j)
                a.set(j, nya_math::vec4(m[j]));
            mat.set_param_array(mat.get_param_idx("shadows tr"), a);
        }
    }
}

//------------------------------------------------------------

void aircraft::draw(int lod_idx)
{
    if (m_hide)
        return;

    m_mesh.draw(lod_idx);

    if (lod_idx == 0)
    {
        for (int i = 0; i < 2; ++i)
        {
            for (auto &m: i == 0 ? m_msls_mount : m_special_mount)
            {
                if (!m.visible || m.bone_idx < 0)
                    continue;

                auto &mesh = i == 0 ? m_missile: m_special;
                mesh.set_pos(m_mesh.get_bone_pos(0, m.bone_idx));
                mesh.set_rot(m_mesh.get_bone_rot(0, m.bone_idx));
                mesh.draw(0);
            }
        }
    }
}

//------------------------------------------------------------

void aircraft::draw_player()
{
    draw(0);

    if (m_engine_lod_idx >= 0)
        draw(m_engine_lod_idx);

    //draw(4); //landing gear
}

//------------------------------------------------------------

void aircraft::draw_cockpit()
{
    //ToDo: also draw 2
    auto &mesh = m_mesh.get_mesh(1);
    mesh.set_pos(vec3());
    mesh.set_rot(get_rot());
    mesh.draw();
}

//------------------------------------------------------------

void aircraft::draw_player_shadowcaster()
{
    if (m_hide)
        return;

    if (m_mesh.get_lods_count() < 1)
        return;

    const int lods[] = { 0, m_engine_lod_idx };
    for (auto l: lods)
    {
        if (l < 0)
            continue;

        auto &mesh = m_mesh.get_mesh(l);
        mesh.set_pos(vec3());
        mesh.set_rot(get_rot());
        mesh.draw("caster");
    }
}

//------------------------------------------------------------

void aircraft::draw_trails(const scene &s)
{
    if (m_has_trail)
    {
        for (auto &t: m_trails)
            s.get_part_renderer().draw(t.first);
    }
}

//------------------------------------------------------------

void aircraft::draw_fire_trail(const scene &s)
{
    if (m_dead)
        s.get_part_renderer().draw(m_fire_trail);
}

//------------------------------------------------------------

void aircraft::draw_mgun_flash(const scene &s)
{
    if (m_fire_mgun)
    {
        for (auto &m: m_mguns)
            s.get_part_renderer().draw(m.flash);
    }

    if (m_fire_mgp)
    {
        for (auto &m: m_mgps)
            s.get_part_renderer().draw(m.flash);
    }
}

//------------------------------------------------------------

void aircraft::draw_engine_effect(const scene &s)
{
    if (m_dead || m_hide)
        return;

    for (auto &e: m_engines)
        s.get_part_renderer().draw(e.first);
}

//------------------------------------------------------------

void aircraft::draw_engine_heat_effect(const scene &s)
{
    if (m_dead || m_hide)
        return;

    for (auto &e: m_engines)
        s.get_part_renderer().draw_heat(e.first);
}

//------------------------------------------------------------
}
