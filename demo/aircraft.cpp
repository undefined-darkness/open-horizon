//
// open horizon -- undefined_darkness@outlook.com
//

#include "aircraft.h"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "render/screen_quad.h"
#include "render/fbo.h"
#include "xml.h"

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

bool aircraft::load(const char *name, const char *color_name)
{
    if (!name)
        return false;

    pugi::xml_document doc;
    if (!load_xml((std::string("aircrafts/")+name+".xml").c_str(), doc))
        return false;

    auto root=doc.child("aircraft");
    if (!root)
        return false;

    m_mesh.load(root.child("model").attribute("file").as_string());
    m_mesh.set_ndxr_texture(0, "ambient", root.child("ambient").attribute("file").as_string());

    auto paint = root.child("paint");
    if (color_name)
    {
        for(auto p=paint;p;p=p.next_sibling())
        {
            if(strcmp(color_name,p.attribute("name").as_string(""))==0)
            {
                paint=p;
                break;
            }
        }
    }

    if(paint)
    {
        color_baker baker;
        baker.set_base(paint.child("base").attribute("file").as_string());
        baker.set_colx(paint.child("colx").attribute("file").as_string());
        baker.set_coly(paint.child("coly").attribute("file").as_string());

        int idx=0;
        for(auto c=paint.child("color");c;c=c.next_sibling(),++idx)
            baker.set_color(idx, nya_math::vec3(c.attribute("r").as_int()/255.0f,
                                                c.attribute("g").as_int()/255.0f,
                                                c.attribute("b").as_int()/255.0f));

        m_mesh.set_ndxr_texture(0, "diffuse", baker.bake() );
        m_mesh.set_ndxr_texture(0, "specular", paint.child("specular").attribute("file").as_string());
    }

    m_params.load(root.child("phys").attribute("file").as_string());

    m_speed = m_params.move.speed.speedCruising;

    m_mesh.set_relative_anim_time(0, 'cndl', 0.5); //fgo
    m_mesh.set_relative_anim_time(0, 'cndr', 0.5);
    m_mesh.set_relative_anim_time(0, 'cndn', 0.5);
    m_mesh.set_relative_anim_time(0, 'ldab', 0.0);
    m_mesh.set_relative_anim_time(0, 'rudl', 0.5); //tail vert
    m_mesh.set_relative_anim_time(0, 'rudr', 0.5);
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
    m_mesh.set_relative_anim_time(0, 'fdwg', 0.0); //carrier
    m_mesh.set_relative_anim_time(0, 'tail', 0.0); //carrier, tail only

    return true;
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

    //apply acceleration

    m_speed += m_params.move.accel.acceleR * throttle * kdt * 50.0f;
    m_speed -= m_params.move.accel.deceleR * brake * kdt * m_speed * 0.04f;
    if (m_speed < m_params.move.speed.speedMin)
        m_speed = m_params.move.speed.speedMin;
    if (m_speed > m_params.move.speed.speedMax)
        m_speed = m_params.move.speed.speedMax;

    //apply speed

    m_pos += forward * (m_speed / 3.6f) * kdt;
    if (m_pos.y < 7.0f)
    {
        m_pos.y = 7.0f;
        m_rot = nya_math::quat(-0.5, m_rot.get_euler().y, 0.0);
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

    const bool has_air_brake = m_mesh.has_anim(0,'abkn');
    if(has_air_brake)
        m_mesh.set_anim_speed(0, 'abkn', brake>0.7f ? 1.0: -1.0);

    float ideal_rl = clamp(-m_controls_rot.y + (!has_air_brake ? brake : 0.0), -1.0f, 1.0f) * speed_k;
    float ideal_rr = clamp(-m_controls_rot.y - (!has_air_brake ? brake : 0.0), -1.0f, 1.0f) * speed_k;

    m_mesh.set_anim_speed(0, 'rudl', (ideal_rl * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudl')) * ae_anim_speed_k);
    m_mesh.set_anim_speed(0, 'rudr', (ideal_rr * 0.5 + 0.5 - m_mesh.get_relative_anim_time(0, 'rudr')) * ae_anim_speed_k);

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

    m_mesh.set_pos(m_pos);
    m_mesh.set_rot(m_rot);
    m_mesh.update(dt);
}

//------------------------------------------------------------
