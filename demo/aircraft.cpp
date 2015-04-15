//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "fhm.h"
#include "debug.h"
#include <assert.h>
#include "dpl.h"

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
        std::vector<color_info> color_info;
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
        static aircraft_information info("target/Information/AircraftInformationC05.AIN");
        return info;
    }

    aircraft_information(const char *name)
    {
        //ToDo: don't determine aircraft order by camera offsets file

        auto r = shared::load_resource("Camera/AdjustedCameraOffset.txt");

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

        r = shared::load_resource(name);
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
                if (color_idx >= (int)info.color_info.size())
                     info.color_info.resize(color_idx + 1);

                auto &c = info.color_info[color_idx];
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

            for (size_t i = 0; i < info.color_info.size(); ++i)
            {
                assert(i < in.size());
                info.color_info[i].coledit_idx = in[i];
            }
        }
    }

private:
    std::vector<info> m_infos;
};

bool aircraft::load(const char *name, unsigned int color_idx)
{
    if (!name)
        return false;

    std::string name_str(name);

    auto info = aircraft_information::get().get_info(name);
    if (!info)
        return false;

    if (color_idx >= info->color_info.size())
        return false;

    auto &color = info->color_info[color_idx];

    m_camera_offset = info->camera_offset;

    m_mesh.load(("model_id/mech/plyr/p_" + name_str + "/p_" + name_str + "_pcom.fhm").c_str());
    m_mesh.set_ndxr_texture(0, "ambient", ("model_id/tdb/mech/plyr/p_" + name_str + "/00/p_" + name_str + "_00_amb.img").c_str());

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

    m_mesh.set_ndxr_texture(0, "diffuse", baker.bake() );
    sprintf(buf, "model_id/tdb/mech/plyr/p_%s/%02d/p_%s_%02d_spe.img", name, color.coledit_idx, name, color.coledit_idx);
    m_mesh.set_ndxr_texture(0, "specular", buf);

    m_params.load(("Player/Behavior/param_p_" + name_str + ".bin").c_str());

    m_speed = m_params.move.speed.speedCruising;

    m_hp = 1.0f;

    m_mesh.set_relative_anim_time(0, 'cndl', 0.5); //fgo
    m_mesh.set_relative_anim_time(0, 'cndr', 0.5);
    m_mesh.set_relative_anim_time(0, 'cndn', 0.5);
    m_mesh.set_relative_anim_time(0, 'ldab', 0.0);
    m_mesh.set_relative_anim_time(0, 'rudl', 0.5); //tail vert
    m_mesh.set_relative_anim_time(0, 'rudr', 0.5);
    m_mesh.set_relative_anim_time(0, 'rudn', 0.5);
    m_mesh.set_relative_anim_time(0, 'elvl', 0.5); //tail hor
    m_mesh.set_relative_anim_time(0, 'elvr', 0.5);
    m_mesh.set_relative_anim_time(0, 'tefn', 0.0); //elerons, used only with flaperons
    m_mesh.set_relative_anim_time(0, 'alrl', 0.5); //wing sides, used with tail hor
    m_mesh.set_relative_anim_time(0, 'alrr', 0.5);
    m_mesh.set_relative_anim_time(0, 'lefn', 0.0); //flaperons
    m_mesh.set_relative_anim_time(0, 'rmpn', 0.0); //engine air supl
    //m_mesh.set_relative_anim_time(0, 'vctn', 0.0); //engine
    m_mesh.set_relative_anim_time(0, 'abkn', 0.0); //air brake
    m_mesh.set_relative_anim_time(0, 'gunc', 0.0); //mgun
    m_mesh.set_relative_anim_time(0, 'misc', 0.0); //primary weapon
    m_mesh.set_relative_anim_time(0, 'spwc', 0.0); //special weapon
    m_mesh.set_relative_anim_time(0, 'swcc', 0.0); //special weapon
    m_mesh.set_relative_anim_time(0, 'swc3', 0.0); //special weapon
    m_mesh.set_relative_anim_time(0, 'fdwg', 0.0); //carrier
    m_mesh.set_relative_anim_time(0, 'tail', 0.0); //carrier, tail only

    //ToDo: su24 spll splr

    //m_mesh.set_anim_speed(0, 'vctn', 0.5);

    return true;
}

unsigned int aircraft::get_colors_count(const char *plane_name)
{
    auto info = aircraft_information::get().get_info(plane_name);
    if (!info)
        return 0;

    return (unsigned int)info->color_info.size();
}

void aircraft::set_controls(const nya_math::vec3 &rot, float throttle, float brake)
{
    m_controls_rot = rot;
    m_controls_throttle = throttle;
    m_controls_brake = brake;
}

void aircraft::update(int dt)
{
    float kdt = dt * 0.001f;

    if (m_hp < 1.0)
    {
        m_hp += kdt * 0.1f;
        if (m_hp > 1.0f)
            m_hp = 1.0f;
    }

    //simulation

    const float d2r = 3.1416 / 180.0f;

    const float speed_arg = m_params.rotgraph.speed.get(m_speed);

    const nya_math::vec3 rot_speed = m_params.rotgraph.speedRot.get(speed_arg);

    const float eps = 0.001f;
    if (fabsf(m_controls_rot.z) > eps)
        m_rot_speed.z = tend(m_rot_speed.z, m_controls_rot.z * rot_speed.z, m_params.rot.addRotR.z * fabsf(m_controls_rot.z) * kdt * 100.0);
    else
        m_rot_speed.z = tend(m_rot_speed.z, 0.0f, m_params.rot.addRotR.z * kdt * 40.0);

    if (fabsf(m_controls_rot.y) > eps)
        m_rot_speed.y = tend(m_rot_speed.y, -m_controls_rot.y * rot_speed.y, m_params.rot.addRotR.y * fabsf(m_controls_rot.y) * kdt * 50.0);
    else
        m_rot_speed.y = tend(m_rot_speed.y, 0.0f, m_params.rot.addRotR.y * kdt * 10.0);

    if (fabsf(m_controls_rot.x) > eps)
        m_rot_speed.x = tend(m_rot_speed.x, m_controls_rot.x * rot_speed.x, m_params.rot.addRotR.x * fabsf(m_controls_rot.x) * kdt * 100.0);
    else
        m_rot_speed.x = tend(m_rot_speed.x, 0.0f, m_params.rot.addRotR.x * kdt * 40.0);

    //get axis

    nya_math::vec3 up(0.0, 1.0, 0.0);
    nya_math::vec3 forward(0.0, 0.0, 1.0);
    nya_math::vec3 right(1.0, 0.0, 0.0);
    forward = m_rot.rotate(forward);
    up = m_rot.rotate(up);
    right = m_rot.rotate(right);

    //rotation

    float high_g_turn = 1.0f + m_controls_brake * 0.5;
    m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 0.0, 1.0), m_rot_speed.z * kdt * d2r * 0.7);
    m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), m_rot_speed.y * kdt * d2r * 0.12);
    m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), m_rot_speed.x * kdt * d2r * (0.13 + 0.05 * (1.0f - fabsf(up.y))) * high_g_turn
                                   * (m_rot_speed.x < 0 ? 1.0f : 1.0f * m_params.rot.pitchUpDownR));
    //nose goes down while upside-down

    const nya_math::vec3 rot_grav = m_params.rotgraph.rotGravR.get(speed_arg);
    m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), -(1.0 - up.y) * kdt * d2r * rot_grav.x * 0.5f);
    m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * kdt * d2r * rot_grav.y * 0.5f);

    //nose goes down during stall

    if (m_speed < m_params.move.speed.speedStall)
    {
        float stallRotSpeed = m_params.rot.rotStallR * kdt * d2r * 10.0f;
        m_rot = m_rot * nya_math::quat(nya_math::vec3(1.0, 0.0, 0.0), up.y * stallRotSpeed);
        m_rot = m_rot * nya_math::quat(nya_math::vec3(0.0, 1.0, 0.0), -right.y * stallRotSpeed);
    }

    //adjust throttle to fit cruising speed

    float throttle = m_controls_throttle;
    float brake = m_controls_brake;
    if (brake < 0.01f && throttle < 0.01f)
    {
        if (m_speed < m_params.move.speed.speedCruising)
            throttle = std::min((m_params.move.speed.speedCruising - m_speed) * 0.1f, 0.1f);
        else if (m_speed > m_params.move.speed.speedCruising)
            brake = std::min((m_speed - m_params.move.speed.speedCruising) * 0.1f, 0.1f);
    }

    //afterburner

    if (m_controls_throttle > 0.3)
    {
        m_thrust_time += kdt;
        if (m_thrust_time >= m_params.move.accel.thrustMinWait)
        {
            m_thrust_time = m_params.move.accel.thrustMinWait;
            throttle *= m_params.move.accel.powerAfterBurnerR;
            m_mesh.set_anim_speed(0, 'rmpn', 0.7f);
        }
    }
    else if (m_thrust_time > 0.0f)
    {
        m_thrust_time -= kdt;
        if (m_thrust_time < 0.0f)
            m_thrust_time = 0.0f;

        m_mesh.set_anim_speed(0, 'rmpn', -0.5f);
    }

    m_mesh.set_anim_speed(0, 'vwgn', m_speed >  m_params.move.speed.speedCruising + 250 ? 0.8: -0.8);

    //apply acceleration

    m_speed += m_params.move.accel.acceleR * throttle * kdt * 50.0f;
    m_speed -= m_params.move.accel.deceleR * brake * kdt * m_speed * 0.04f;
    if (m_speed < m_params.move.speed.speedMin)
        m_speed = m_params.move.speed.speedMin;
    if (m_speed > m_params.move.speed.speedMax)
        m_speed = m_params.move.speed.speedMax;

    //apply speed

    m_pos += forward * (m_speed / 3.6f) * kdt;
    if (m_pos.y < 5.0f)
    {
        m_pos.y = 5.0f;
        m_rot = nya_math::quat(-0.5, m_rot.get_euler().y, 0.0);
        m_hp = 0.0f;
    }

    //animations

    float flap_anim_speed = 0.5f;
    if (m_speed < m_params.move.speed.speedCruising - 100)
    {
        m_mesh.set_anim_speed(0, 'lefn', flap_anim_speed);
        m_mesh.set_anim_speed(0, 'tefn', -flap_anim_speed);
    }
    else
    {
        m_mesh.set_anim_speed(0, 'lefn', -flap_anim_speed);
        if (m_mesh.get_relative_anim_time(0, 'tefn') < 0.5f)
            m_mesh.set_anim_speed(0, 'tefn', flap_anim_speed);
        else
        {
            m_mesh.set_anim_speed(0, 'tefn', 0.0f);
            m_mesh.set_relative_anim_time(0, 'tefn', 0.5f);
        }
    }

    const float speed_k = std::max((m_params.move.speed.speedMax - m_speed) / m_params.move.speed.speedMax, 0.1f);
    const float ae_anim_speed_k = 5.0f;
    float ideal_el = clamp(-m_controls_rot.z - m_controls_rot.x, -1.0f, 1.0f) * speed_k;
    float ideal_er = clamp(m_controls_rot.z - m_controls_rot.x, -1.0f, 1.0f) * speed_k;

    m_mesh.set_anim_speed(0, 'elvl', (ideal_el * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvl')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'elvr', (ideal_er * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'elvr')) * ae_anim_speed_k);

    const float ideal_al = -m_controls_rot.z * speed_k;
    const float ideal_ar = m_controls_rot.z * speed_k;
    m_mesh.set_anim_speed(0, 'alrl', (ideal_al * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrl')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'alrr', (ideal_ar * 0.5f+0.5f - m_mesh.get_relative_anim_time(0, 'alrr')) * ae_anim_speed_k);

    const float ideal_cn = -m_controls_rot.x * speed_k;
    m_mesh.set_anim_speed(0, 'cndl', (ideal_cn * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndl')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'cndr', (ideal_cn * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndr')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'cndn', (ideal_cn * 0.5f + 0.5f - m_mesh.get_relative_anim_time(0, 'cndn')) * ae_anim_speed_k);

    bool has_air_brake = m_mesh.has_anim(0,'abkn');
    if(has_air_brake)
        m_mesh.set_anim_speed(0, 'abkn', brake>0.7f ? 1.0: -1.0);
    else
        has_air_brake = m_mesh.has_anim(0,'spll') || m_mesh.has_anim(0,'splr');

    m_mesh.set_anim_speed(0, 'spll', brake>0.7f ? 1.0: -1.0);
    m_mesh.set_anim_speed(0, 'splr', brake>0.7f ? 1.0: -1.0);

    float ideal_rl = clamp(-m_controls_rot.y + (!has_air_brake ? brake : 0.0), -1.0f, 1.0f) * speed_k;
    float ideal_rr = clamp(-m_controls_rot.y - (!has_air_brake ? brake : 0.0), -1.0f, 1.0f) * speed_k;
    float ideal_rn = clamp(-m_controls_rot.y, -1.0f, 1.0f) * speed_k;

    m_mesh.set_anim_speed(0, 'rudl', (ideal_rl * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudl')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudr', (ideal_rr * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudr')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudn', (ideal_rn * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudn')) * ae_anim_speed_k);

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

    m_mesh.set_anim_speed(0, 'ldab', m_special_selected ? 1.0f : -1.0f);

    m_mesh.set_pos(m_pos);
    m_mesh.set_rot(m_rot);
    m_mesh.update(dt);
}

//------------------------------------------------------------
