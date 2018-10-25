//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "extensions/zip_resources_provider.h"
#include "memory/tmp_buffer.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "math/scalar.h"
#include "containers/fhm.h"
#include "containers/dpl.h"
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

    void set_color(unsigned int idx, const nya_math::vec3 &color)
    {
        if (!m_colors.get_count())
        {
            m_colors.set_count(6);
            //for (int i=0;i<m_colors.get_count();++i) m_colors.set(i,nya_math::vec3(1.0,1.0,1.0));
        }

        m_colors.set(idx,color);
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

class aircraft_information
{
public:
    struct color_info
    {
        std::string name;
        int coledit_idx;
        nya_math::vec3 colors[6];
    };

    struct mod_info: public color_info
    {
        std::string zip_name, tex_name, spec_name;
        bool override_colors[6] = { false };

        nya_scene::texture load_texture(std::string name)
        {
            nya_resources::zip_resources_provider z(zip_name.c_str());
            return renderer::load_texture(z, name.c_str());
        }
    };

    struct engine_info
    {
        float radius = 0.0f, dist = 0.0f;
        nya_math::vec2 tvc;
    };

    struct info
    {
        std::string name;
        nya_math::vec3 camera_offset;
        std::vector<color_info> colors;
        std::vector<mod_info> color_mods;
        std::string sound, voice;
        std::vector<engine_info> engines;
    };

    info *get_info(const char *name)
    {
        if (!name)
            return 0;

        std::string name_str(name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);

        for (auto &i: m_infos)
        {
            if (i.name == name_str)
                return &i;
        }

        return 0;
    }

    static aircraft_information &get()
    {
        static aircraft_information info("target/Information/AircraftInformation.AIN");
        static bool once = true;
        if (once)
        {
            aircraft_information info5("target/Information/AircraftInformationC05.AIN"); //ToDo
            *info.get_info("tnd4") = *info5.get_info("tnd4");

            auto &f14d = *info.get_info("f14d");
            if (f14d.colors.size() > 1)
                std::swap(f14d.colors.back().coledit_idx, f14d.colors[f14d.colors.size() - 2].coledit_idx);

            auto &av8b = *info.get_info("av8b");
            av8b.colors.resize(3);
            av8b.colors.back().coledit_idx = 3;

            pugi::xml_document doc;
            if (load_xml("aircrafts.xml", doc))
            {
                auto root = doc.first_child();
                for (pugi::xml_node ac = root.child("aircraft"); ac; ac = ac.next_sibling("aircraft"))
                {
                    auto inf = info.get_info(ac.attribute("id").as_string(""));
                    if (!inf)
                        continue;

                    for (int i = 0; i < (int)inf->colors.size(); ++i)
                    {
                        auto d = ac.child(("decal" + std::to_string(i)).c_str());
                        if (!d)
                            continue;

                        inf->colors[i].name = d.attribute("name").as_string();

                        if (!d.first_child())
                            continue;

                        for (int j = 0; j < 6; ++j)
                        {
                            auto c = d.child(("color" + std::to_string(j)).c_str());
                            if (c)
                                inf->colors[i].colors[j] = vec3(c.attribute("r").as_int(), c.attribute("g").as_int(), c.attribute("b").as_int()) / 255.0;
                        }
                    }

                    for (pugi::xml_node eng = ac.child("engine"); eng; eng = eng.next_sibling("engine"))
                    {
                        engine_info ei;
                        ei.radius = eng.attribute("radius").as_float();
                        ei.dist = eng.attribute("dist").as_float();
                        ei.tvc.x = ei.tvc.y = eng.attribute("tvc_xy").as_float();
                        ei.tvc.y = eng.attribute("tvc_y").as_float(ei.tvc.y);
                        inf->engines.push_back(ei);
                    }
                }
            }

            auto custom_decals = list_files("decals/");
            for (auto &d: custom_decals)
            {
                nya_resources::zip_resources_provider zprov;
                if (!zprov.open_archive(d.c_str()))
                    continue;

                pugi::xml_document doc;
                if (!load_xml(zprov.access("info.xml"), doc))
                    continue;

                auto root = doc.first_child();
                auto plane_id = root.attribute("plane").as_string();

                auto pi = info.get_info(plane_id);
                if (!pi)
                {
                    printf("plane '%s' not found for decal mod '%s'\n", plane_id, d.c_str());
                    continue;
                }

                mod_info m;
                m.name = root.attribute("name").as_string();
                m.zip_name = d;
                m.coledit_idx = root.attribute("base").as_int();

                if (m.coledit_idx < 0 || m.coledit_idx >= pi->colors.size())
                {
                    printf("invalid base idx %d for decal mod '%s', base count: %d\n", m.coledit_idx, d.c_str(), (int)pi->colors.size());
                    continue;
                }

                auto img = root.child("image");
                if (img)
                    m.tex_name = img.attribute("file").as_string();

                auto simg = root.child("specular");
                if (simg)
                    m.spec_name = simg.attribute("file").as_string();

                for (int i = 0; i < sizeof(m.colors) / sizeof(m.colors[0]); ++i)
                {
                    auto c = root.child(("color" + std::to_string(i)).c_str());
                    if (!c)
                        continue;

                    m.override_colors[i] = true;
                    m.colors[i].set(c.attribute("r").as_int() / 255.0f, c.attribute("g").as_int() / 255.0f, c.attribute("b").as_int() / 255.0f);
                }

                pi->color_mods.push_back(m);
            }

            once = false;
        }

        return info;
    }

    aircraft_information(const char *name)
    {
        //ToDo: don't determine aircraft order by camera offsets file

        auto r = load_resource("Camera/AdjustedCameraOffset.txt");

        std::vector<std::string> values;
        std::string tmp;
        for (size_t i = 0; i < r.get_size(); ++i)
        {
            char c=((char *)r.get_data())[i];
            if (c=='\n' || c=='\r')
            {
                if (tmp.empty())
                    continue;

                auto t = tmp.substr(0,7);

                if (t !="float\t.")
                {
                    tmp.clear();
                    continue;
                }

                tmp = tmp.substr(t.length());

                for (size_t j = 0; j < tmp.size(); ++j)
                {
                    std::string type = tmp.substr(0, j);
                    if (tmp[j] == '\t')
                    {
                        tmp = tmp.substr(j + 1);
                        break;
                    }
                }

                std::string plane_name = tmp.substr(0,4);
                std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);

                auto info = get_info(plane_name.c_str());
                if (!info)
                {
                    m_infos.resize(m_infos.size() + 1);
                    m_infos.back().name = plane_name;
                    info = &m_infos.back();
                    //printf("%s\n",plane_name.c_str());
                }

                auto d = tmp.find(':');
                auto vname = tmp.substr(d + 1);
                tmp = tmp.substr(5, d - 1 - 5);
                float v = atof(vname.c_str());

                if (tmp == "AroundLook.Offset.X") info->camera_offset.x = v;
                if (tmp == "AroundLook.Offset.Y") info->camera_offset.y = v;
                if (tmp == "AroundLook.Offset.Z") info->camera_offset.z = v;

                //printf("%s=%f\n",tmp.c_str(),v);

                tmp.clear();
                continue;
            }

            tmp.push_back(c);
        }

        r.free();

        r = load_resource(name);
        nya_memory::memory_reader reader(r.get_data(),r.get_size());

        struct ain_header
        {
            char sign[4];
            uint32_t unknown;
            uint32_t count;
            uint32_t zero;
        };

        auto header = reader.read<ain_header>();
        if (memcmp(header.sign, "AIN", 3) == 0)
        {
            struct ain_entry
            {
                uint16_t color_num;
                uint16_t plane_num;
                uint32_t unknown[39];
                uint8_t colors[6][4];
                uint32_t unknown2[6];
            };

            std::vector<ain_entry> entries(header.count);
            for (auto &e: entries)
            {
                e = reader.read<ain_entry>();
                auto plane_idx = int(e.plane_num)-1;
                assert(plane_idx >= 0);

                if (plane_idx >= m_infos.size())
                    continue;

                auto &info = m_infos[plane_idx];
                auto color_idx = int(e.color_num)-1;
                if (color_idx >= (int)info.colors.size())
                     info.colors.resize(color_idx + 1);

                auto &c = info.colors[color_idx];
                c.coledit_idx = -1;
                for (int i = 0; i < 6; ++i)
                {
                    auto ec = e.colors[i];
                    c.colors[i] = nya_math::vec3(ec[3], ec[2], ec[1]) / 255.0;
                }
            }
        }
        r.free();

        //ToDo

        std::map<std::string,std::vector<int> > ints;
        std::map<std::string, std::pair<std::string, std::string> > sounds;

        dpl_file dp;
        dp.open("datapack.bin");

        std::string buf;
        for (int i = 0; i < dp.get_files_count(); ++i)
        {
            buf.resize(dp.get_file_size(i));
            dp.read_file_data(i, &buf[0]);
            assert(buf.size()>16);
            if (strncmp(buf.c_str(), ";DPL_P_", 7) != 0)
                continue;

            std::string plane_name = buf.substr(7,4);
            std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);

            if (buf[7+4] == '\n')
            {
                auto s0 = buf.find("ound/");     if (s0 == std::string::npos) continue;
                auto e0 = buf.find('\x9', s0);    if (e0 == std::string::npos) continue;
                auto s1 = buf.find("ound/", e0); if (s1 == std::string::npos) continue;
                auto e1 = buf.find('\x9', s1);    if (e1 == std::string::npos) continue;

                auto test = sounds[plane_name] = std::make_pair('s' + buf.substr(s0, e0 - s0), 's' + buf.substr(s1, e1 - s1));
                test = test;
            }
            else if (buf[7+4] == '_' && buf[7+4+1] == 'T')
            {
                int idx = atoi(&buf[7+4+2]);
                ints[plane_name].push_back(idx);
            }
        }

        dp.close();

        for (auto &info: m_infos)
        {
            auto &in = ints[info.name];

            auto s = sounds.find(info.name);
            if (s != sounds.end())
                info.sound = s->second.first, info.voice = s->second.second;

            int last_idx = 0;
            for (size_t i = 0; i < info.colors.size(); ++i)
            {
                //assert(i < in.size());
                if (i < in.size())
                {
                    info.colors[i].coledit_idx = in[i];
                    last_idx = in[i];
                }
                else
                    info.colors[i].coledit_idx = ++last_idx;
            }
        }
    }

private:
    std::vector<info> m_infos;
};

//------------------------------------------------------------

bool aircraft::load(const char *name, unsigned int color_idx, const location_params &params, bool player)
{
    if (!name)
        return false;

    std::string name_str(name);

    m_half_flaps_flag = name_str == "f22a" || name_str == "m21b" || name_str == "av8b";

    m_engine_lod_idx = 3; //ToDo: player ? 3 : -1;
    if (name_str == "av8b")
        m_engine_lod_idx = -1;

    std::string name_tmp_str = "p_" + name_str; //ToDo: load d_ models
    name_str = (player ? "p_" : "d_") + name_str;

    const std::string tex_pref = std::string("model_id/tdb/mech/") + (player ? "plyr/" : "airp/");
    const std::string mesh_pref = std::string("model_id/mech/") + "plyr/"; //ToDo: + (player ? "plyr/" : "airp/");

    auto info = aircraft_information::get().get_info(name);
    if (!info)
        return false;

    if (color_idx >= info->colors.size() + info->color_mods.size())
        return false;

    const auto base_color_idx = color_idx < info->colors.size() ? color_idx : info->color_mods[color_idx - info->colors.size()].coledit_idx;

    assert(base_color_idx < info->colors.size());
    auto &color = info->colors[base_color_idx];

    m_camera_offset = info->camera_offset;

    m_mesh.load(name_tmp_str.c_str(), params);

    if (player)
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

    char cfldr[256];
    sprintf(cfldr, "%02d", color.coledit_idx);
    std::string cfldr2 = std::string(cfldr) + (player ? "" : "_l");

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

        m_mesh.set_texture(m_engine_lod_idx, "specular", spec_tex);

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

    for (int i = 0; i < 6; ++i)
        baker.set_color(i, color.colors[i]);

    if (color_idx >= info->colors.size())
    {
        auto &cm = info->color_mods[color_idx - info->colors.size()];
        if (!cm.tex_name.empty())
            baker.set_decal(cm.load_texture(cm.tex_name));

        for (int i = 0; i < 6; ++i)
        {
            if (cm.override_colors[i])
                baker.set_color(i, cm.colors[i]);
        }

        if (!cm.spec_name.empty())
        {
            color_baker spec_baker;
            spec_baker.set_base(spec_tex);
            spec_baker.set_decal(cm.load_texture(cm.spec_name));
            spec_tex = spec_baker.bake();
        }
    }

    m_mesh.set_material(0, mat.materials[0], "shaders/player_plane.nsh");
    if (player && m_engine_lod_idx >= 0 && mat.materials.size() > 2)
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

    //ToDo: add single group in fhm_mesh instead
    if (player)
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
        m_engines.push_back({plane_engine(ei.radius, ei.dist), m_mesh.get_bone_idx(0, bname.c_str())});
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

    //ToDo
    /*
    if (dt)
    {
        //variometer
        static nya_math::vec3 prev_pos = m_pos;
        float vert_speed = ((m_pos - prev_pos) / float(dt)).y;
        m_mesh.set_relative_anim_time(1, 'vspm', vert_speed * 0.5 + 0.5);
        prev_pos = m_pos;

        //accel
        //float accel = (m_speed - prev_speed) / float(dt);
        //m_mesh.set_relative_anim_time(1, 'accm', accel * 0.5 + 0.5);
    }

    m_mesh.set_relative_anim_time(1, 'ecsp', 0.5 + 0.5 * m_thrust_time/m_params.move.accel.thrustMinWait);
     //erpm - engine rpm
    */

    m_mesh.set_relative_anim_time(1, 'aoam', nya_math::min(10.0 * nya_math::max(m_aoa / nya_math::constants::pi, 0.0), 1.0));

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
            const float l = (m_mesh.get_relative_anim_time(3, 'vctl') * 2.0f - 1.0f) * m_tvc_param.y;
            const float r = (m_mesh.get_relative_anim_time(3, 'vctr') * 2.0f - 1.0f) * m_tvc_param.y;
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
}
