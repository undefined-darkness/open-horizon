//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "math/scalar.h"
#include "containers/fhm.h"
#include "containers/dpl.h"
#include "renderer/shared.h"
#include <stdint.h>

namespace renderer
{
//------------------------------------------------------------

class color_baker
{
public:
    void set_base(const char *name) { m_mat.set_texture("base", nya_scene::texture(name)); }
    void set_colx(const char *name) { m_mat.set_texture("colx", nya_scene::texture(name)); }
    void set_coly(const char *name) { m_mat.set_texture("coly", nya_scene::texture(name)); }

    void set_color(unsigned int idx, const nya_math::vec3 &color)
    {
        if (!m_colors.get_count())
        {
            m_colors.set_count(6);
            //for(int i=0;i<m_colors.get_count();++i) m_colors.set(i,nya_math::vec3(1.0,1.0,1.0));
        }

        m_colors.set(idx,color);
    }

    nya_scene::texture bake()
    {
        auto tp=m_mat.get_texture("base");
        if(!tp.is_valid())
            return nya_scene::texture();

        nya_render::fbo fbo;
        nya_render::screen_quad quad;
        quad.init();
        nya_scene::shared_texture res;
        res.tex.build_texture(0, tp->get_width(), tp->get_height(), nya_render::texture::color_rgba);
        fbo.set_color_target(res.tex);

        m_mat.get_pass(m_mat.add_pass(nya_scene::material::default_pass)).set_shader(nya_scene::shader("shaders/plane_camo.nsh"));
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
        int coledit_idx;
        nya_math::vec3 colors[6];
    };

    struct info
    {
        std::string name;
        nya_math::vec3 camera_offset;
        std::vector<color_info> colors;
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
                if(!info)
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

            if (buf[7+4] != '_' || buf[7+4+1] != 'T')
                continue;

            int idx = atoi(&buf[7+4+2]);

            std::string plane_name = buf.substr(7,4);
            std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);

            ints[plane_name].push_back(idx);
        }

        dp.close();

        for(auto &info: m_infos)
        {
            auto &in = ints[info.name];

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

bool aircraft::load(const char *name, unsigned int color_idx, const location_params &params)
{
    if (!name)
        return false;

    std::string name_str(name);

    auto info = aircraft_information::get().get_info(name);
    if (!info)
        return false;

    if (color_idx >= info->colors.size())
        return false;

    auto &color = info->colors[color_idx];

    m_camera_offset = info->camera_offset;

    m_mesh.load(("p_" + name_str).c_str(), params);
    m_mesh.set_texture(0, "ambient", ("model_id/tdb/mech/plyr/p_" + name_str + "/00/p_" + name_str + "_00_amb.img").c_str());
    m_mesh.set_texture(0, "normal", ("model_id/tdb/mech/plyr/p_" + name_str + "/00/p_" + name_str + "_00_nor.img").c_str());

    char buf[1024];

    color_baker baker;
    sprintf(buf, "model_id/tdb/mech/plyr/p_%s/%02d/p_%s_%02d_col.img", name, color.coledit_idx, name, color.coledit_idx);
    baker.set_base(buf);
    sprintf(buf, "model_id/tdb/mech/plyr/p_%s/coledit%02d/p_%s_%02d_mkx.img", name, color.coledit_idx, name, color.coledit_idx);
    baker.set_colx(buf);
    sprintf(buf, "model_id/tdb/mech/plyr/p_%s/coledit%02d/p_%s_%02d_mky.img", name, color.coledit_idx, name, color.coledit_idx);
    baker.set_coly(buf);

    for (int i = 0; i < 6; ++i)
        baker.set_color(i, color.colors[i]);

    m_mesh.set_texture(0, "diffuse", baker.bake() );
    sprintf(buf, "model_id/tdb/mech/plyr/p_%s/%02d/p_%s_%02d_spe.img", name, color.coledit_idx, name, color.coledit_idx);
    m_mesh.set_texture(0, "specular", buf);

    sprintf(buf, "model_id/mech/plyr/p_%s/p_%s_pcol%02d.fhm", name, name, color.coledit_idx);
    m_mesh.load_material(buf, "shaders/player_plane.nsh");

    m_mesh.set_relative_anim_time(0, 'rudl', 0.5);
    m_mesh.set_relative_anim_time(0, 'rudr', 0.5);
    m_mesh.set_relative_anim_time(0, 'rudn', 0.5);

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

    //ToDo: sane way to get weapon information
    std::string special_wpn_name;
    auto r = load_resource(("model_id/mech/airp/d_" + name_str + "/d_" + name_str + "_t00.tdp").c_str());
    std::string wpn_info((char *)r.get_data(), r.get_size());
    for (size_t f = 0;;)
    {
        const static std::string fs("/mech/weap/");
        f = wpn_info.find(fs, f);
        if (f == std::string::npos)
            break;

        f += fs.length();

        auto t = wpn_info.find("/00/", f);
        if (t == std::string::npos)
            break;

        std::string name = wpn_info.substr(f, t - f);
        if (name == "w_msl_")
            continue;

        special_wpn_name = name;
        break;
    }

    const bool is_plane_russian = strncmp(name, "su", 2) == 0 || strncmp(name, "m2", 2) == 0  || strcmp(name, "pkfa") == 0; //ToDo

    std::string wpn_info_suff = is_plane_russian ? "r0" : "a0";
    if (name_str == "f02a")
        wpn_info_suff = "j0";

    //if (has_missiles)
        m_missile.load(("w_msl_" + wpn_info_suff).c_str(), params);

    if (special_wpn_name == "w_ew1_")
        wpn_info_suff = "x0";
    if (special_wpn_name == "w_sod_")
        wpn_info_suff = "e0";

    m_special.load((special_wpn_name + wpn_info_suff).c_str(), params);

    m_adimx_bone_idx = m_mesh.get_bone_idx(1, "adimx1");
    if (m_adimx_bone_idx < 0) m_adimx_bone_idx = m_mesh.get_bone_idx(1, "adimx");
    m_adimx2_bone_idx = m_mesh.get_bone_idx(1, "adimx2");
    m_adimz_bone_idx = m_mesh.get_bone_idx(1, "adimz1");
    if (m_adimz_bone_idx < 0) m_adimz_bone_idx = m_mesh.get_bone_idx(1, "adimz");
    m_adimz2_bone_idx = m_mesh.get_bone_idx(1, "adimz2");
    m_adimxz_bone_idx = m_mesh.get_bone_idx(1, "adimxz1");
    if (m_adimxz_bone_idx < 0) m_adimxz_bone_idx = m_mesh.get_bone_idx(1, "adimxz");
    m_adimxz2_bone_idx = m_mesh.get_bone_idx(1, "adimxz2");

    return true;
}

//------------------------------------------------------------

void aircraft::apply_location(const char *location_name, const location_params &params)
{
    m_mesh.set_texture(0, "reflection", shared::get_texture(shared::load_texture((std::string("Map/envmap_") + location_name + ".nut").c_str())));
    m_mesh.set_texture(0, "ibl", shared::get_texture(shared::load_texture((std::string("Map/ibl_") + location_name + ".nut").c_str())));
}

//------------------------------------------------------------

unsigned int aircraft::get_colors_count(const char *plane_name)
{
    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return 0;

    return (unsigned int)info->colors.size();
}

//------------------------------------------------------------

nya_math::vec3 aircraft::get_bone_pos(const char *name)
{
    return m_mesh.get_bone_pos(0, m_mesh.get_bone_idx(0, name));
}

//------------------------------------------------------------

void aircraft::set_elev(float left, float right)
{
    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'elvl', (left * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'elvr', (right * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvr')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'elvn', (left * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvn')) * anim_speed_k);
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
    if(!has_air_brake)
        has_air_brake = m_mesh.has_anim(0,'spll') || m_mesh.has_anim(0,'splr');

    const float anim_speed_k = 5.0f;
    m_mesh.set_anim_speed(0, 'rudl', ((has_air_brake ? center : left) * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudl')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudr', ((has_air_brake ? center : right) * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudr')) * anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudn', (center * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudn')) * anim_speed_k);
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

void aircraft::update(int dt)
{
    //ToDo
/*
    //weapon animations

    m_mesh.set_anim_speed(0, 'gunc', 5.0 * (m_controls_mgun ? 1.0f : -1.0f));
    m_controls_mgun = false;

    if (m_controls_rocket)
    {
        //if (m_mesh.get_relative_anim_time(0, 'spwc') < 0.01f)
        if (!m_special_selected)
            m_rocket_bay_time = 3.0f;
        m_controls_rocket = false;
    }

    m_rocket_bay_time -= kdt;
    m_mesh.set_anim_speed(0, 'misc', m_rocket_bay_time > 0.01f ? 1.0f : -1.0f);

    if (m_controls_special)
    {
        m_special_selected = !m_special_selected;
        if (m_special_selected)
            m_rocket_bay_time = 0.0f;
        m_controls_special = false;
    }

    m_mesh.set_anim_speed(0, 'spwc', m_special_selected ? 1.0f : -1.0f);
    m_mesh.set_anim_speed(0, 'swcc', m_special_selected ? 1.0f : -1.0f);
    m_mesh.set_anim_speed(0, 'swc3', m_special_selected ? 1.0f : -1.0f);
*/

/*
    //cockpit
    auto pyr = m_rot.get_euler();

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
    m_mesh.set_relative_anim_time(1, 'aspk', speed / m_params.move.speed.speedMax);

    //barometric altimeter
    m_mesh.set_relative_anim_time(1, 'altk', m_pos.y / 10000.0f); //ToDo: adjust max height
    m_mesh.set_relative_anim_time(1, 'altm', m_pos.y / 10000.0f); //ToDo: adjust max height
    //radio altimeter
    m_mesh.set_relative_anim_time(1, 'altr', m_pos.y / 10000.0f); //ToDo: adjust max height, trace
    m_mesh.set_relative_anim_time(1, 'alts', m_pos.y / 10000.0f); //ToDo: adjust max height, trace

    //clocks
    unsigned int seconds = m_time / 1000;
    m_mesh.set_relative_anim_time(1, 'acks', (seconds % 60 / 60.0));
    m_mesh.set_relative_anim_time(1, 'ackm', ((seconds / 60) % 60 / (60.0)));
    m_mesh.set_relative_anim_time(1, 'ackh', ((seconds / 60 / 60) % 12 / (12.0)));
    m_time += dt;

    if (dt)
    {
        //variometer
        float vert_speed = ((m_pos - prev_pos) / float(dt)).y;
        m_mesh.set_relative_anim_time(1, 'vspm', vert_speed * 0.5 + 0.5);

        //accel
        //float accel = (m_speed - prev_speed) / float(dt);
        //m_mesh.set_relative_anim_time(1, 'accm', accel * 0.5 + 0.5);
    }

    m_mesh.set_relative_anim_time(1, 'aoam', nya_math::min(10.0 * nya_math::max(1.0 - nya_math::vec3::normalize(m_vel)*forward, 0.0), 1.0));
    m_mesh.set_relative_anim_time(1, 'ecsp', 0.5 + 0.5 * m_thrust_time/m_params.move.accel.thrustMinWait);

    //erpm - engine rpm
*/

    m_mesh.update(dt);
}

//------------------------------------------------------------

void aircraft::draw(int lod_idx)
{
    m_mesh.draw(lod_idx);

    if (lod_idx == 0)
    {
        for(int i = 0; i < 2; ++i)
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
}