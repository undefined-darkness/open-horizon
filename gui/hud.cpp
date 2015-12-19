//
// open horizon -- undefined_darkness@outlook.com
//

#include "hud.h"
#include "renderer/shared.h"
#include "scene/camera.h"

namespace gui
{

//------------------------------------------------------------

inline bool get_project_pos(const render &r, const nya_math::vec3 &pos, nya_math::vec2 &result)
{
    nya_math::vec4 p(pos, 1.0);
    p = nya_scene::get_camera().get_view_matrix() * nya_render::get_projection_matrix() * p;
    if (p.z < 0)
        return false;

    const float aspect = float(r.get_width(true)) / r.get_height(true) / (float(r.get_width()) / r.get_height() );
    if (aspect > 1.0)
        p.x *= aspect;
    else
        p.y /= aspect;

    result.x = (p.x / p.w * 0.5 + 0.5) * r.get_width();
    result.y = (p.y / p.w * -0.5 + 0.5) * r.get_height();
    return true;
}

//------------------------------------------------------------

void hud::load(const char *aircraft_name)
{
    *this = hud(); //release

    if (!aircraft_name)
        return;

    if (!m_common_loaded)
    {
        m_fonts.load("Hud/hudCommon.fhm");
        m_common.load("Hud/hudCommon.fhm");
        m_common_loaded = true;
    }

    m_aircraft.load(("Hud/aircraft/hudAircraft_" + std::string(aircraft_name) + ".fhm").c_str());
}

//------------------------------------------------------------

void hud::update(int dt)
{
    m_anim_time += dt;
    m_anim_time = m_anim_time % 1000;
}

//------------------------------------------------------------

void hud::draw(const render &r)
{
    if (m_hide)
        return;

    const float anim = fabsf(m_anim_time / 500.0f - 1.0);

    const auto white = nya_math::vec4(255,255,255,255)/255.0;
    const auto green = nya_math::vec4(103,223,144,255)/255.0;
    const auto red = nya_math::vec4(200,0,0,255)/255.0;
    const auto blue = nya_math::vec4(100,200,200,255)/255.0;

    wchar_t buf[255];
    swprintf(buf, sizeof(buf), L"%d", m_speed);
    m_fonts.draw_text(r, L"SPEED", "Zurich14", r.get_width()/2 - 210, r.get_height()/2 - 33, green);
    m_fonts.draw_text(r, buf, "OCRB15", r.get_width()/2 - 210, r.get_height()/2 - 8, green);
    swprintf(buf, sizeof(buf), L"%d", m_alt);
    m_fonts.draw_text(r, L"ALT", "Zurich14", r.get_width()/2 + 170, r.get_height()/2 - 33, green);
    m_fonts.draw_text(r, buf, "OCRB15", r.get_width()/2 + 170, r.get_height()/2 - 8, green);

    swprintf(buf, sizeof(buf), L"pos   %ld   %ld   %ld", long(m_pos.x),long(m_pos.y),long(m_pos.z));
    m_fonts.draw_text(r, buf, "Zurich14", 20, 20, green);

    m_common.draw(r, 10, r.get_width()/2 - 150, r.get_height()/2, green);
    m_common.draw(r, 16, r.get_width()/2 + 150, r.get_height()/2, green);

    //m_common.debug_draw_tx(r);
    //m_aircraft.debug_draw_tx(r);
    //m_common.debug_draw(r, debug_variable::get()); static int last_idx = 0; if (last_idx != debug_variable::get()) printf("%d %d\n", debug_variable::get(), m_common.get_id(debug_variable::get())); last_idx = debug_variable::get();
    //m_aircraft.debug_draw(r, debug_variable::get()); static int last_idx = 0; if (last_idx != debug_variable::get()) printf("%d %d\n", debug_variable::get(), m_aircraft.get_id(debug_variable::get())); last_idx = debug_variable::get();

    //m_common.draw(r, 3, green);
    //m_common.draw(r, 158, green);
    //m_common.draw(r, 159, green);
    //m_common.draw(r, 214, green);

    nya_math::vec2 proj_pos;
    if (get_project_pos(r, m_project_pos, proj_pos))
    {
        //m_common.draw(r, 215, proj_pos.x, proj_pos.y, green);
        m_common.draw(r, 2, proj_pos.x, proj_pos.y, green);
        //ToDo
    }

    //targets

    for (auto &t: m_targets)
    {
        if (!get_project_pos(r, t.pos, proj_pos))
            continue;

        int icon = 102;
        auto color = green;

        const bool is_far = (nya_scene::get_camera().get_pos() - t.pos).length() > 10000.0;
        if (is_far)
            icon = 121;

        if (t.t == target_air_lock)
        {
            color = red;
            m_common.draw(r, 100, proj_pos.x, proj_pos.y, red);
        }

        if (t.t == target_air_ally)
        {
            icon = 106;
            color = blue;
        }

        m_common.draw(r, icon, proj_pos.x, proj_pos.y, color);
        m_common.draw(r, icon + 1, proj_pos.x, proj_pos.y, color);

        if (t.s != select_not)
            m_common.draw(r, t.s == select_current ? 117 : 116, proj_pos.x, proj_pos.y, color);
    }

    //radar

    m_common.draw(r, 214, 185, r.get_height()-140, green); //circle
    m_common.draw(r, 183, 185, r.get_height()-140, green); //my aircraft

    //missiles

    m_aircraft.draw(r, m_missiles_icon + 401, r.get_width() - 110, r.get_height() - 60, green);
    m_aircraft.draw(r, m_missiles_icon + 406, r.get_width() - 110, r.get_height() - 60, green);

    if(m_missile_alert)
    {
        auto color = red;
        color.w = anim;
        static std::vector<nya_math::vec2> malert_quad(5);
        malert_quad[0].x = r.get_width()/2 - 75, malert_quad[0].y = 235;
        malert_quad[1].x = r.get_width()/2 - 75, malert_quad[1].y = 260;
        malert_quad[2].x = r.get_width()/2 + 75, malert_quad[2].y = 260;
        malert_quad[3].x = r.get_width()/2 + 75, malert_quad[3].y = 235;
        malert_quad[4] = malert_quad[0];
        r.draw(malert_quad, color);
        m_fonts.draw_text(r, L"MISSILE ALERT", "Zurich20", r.get_width()/2-60, 235, color);
    }

    //Zurich12 -23 14 -111 20 -105 30 -96 BD30Outline -113 BD20Outline -26 BD22Outline -18 OCRB15 -33
}

//------------------------------------------------------------

void hud::set_missiles(const char *id, int icon)
{
    //ToDo
    m_missiles_icon = icon;
}

//------------------------------------------------------------

void hud::set_missile_reload(int idx, float value)
{
    m_aircraft.set_progress(m_missiles_icon + 406, idx, value);
}

//------------------------------------------------------------

void hud::add_target(const nya_math::vec3 &pos, target_type target, select_type select)
{
    m_targets.push_back({pos, target, select});
}

//------------------------------------------------------------
}
