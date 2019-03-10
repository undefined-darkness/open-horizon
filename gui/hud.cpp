//
// open horizon -- undefined_darkness@outlook.com
//

#include "hud.h"
#include "renderer/shared.h"
#include "scene/camera.h"

namespace gui
{

const hud::color hud::white = color(255,255,255,255)/255.0;
const hud::color hud::green = color(103,223,144,255)/255.0;
const hud::color hud::red = color(200,0,0,255)/255.0;
const hud::color hud::blue = color(100,200,200,255)/255.0;

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

void hud::load(const char *aircraft_name, const char *location_name)
{
    *this = hud(); //release

    if (!m_common_loaded)
    {
        m_fonts.load("Hud/hudCommon.fhm");
        m_common.load("Hud/hudCommon.fhm");
        m_bomb_target_mesh.init();
        m_common_loaded = true;
    }

    if (aircraft_name && aircraft_name[0])
        m_aircraft.load(("Hud/aircraft/hudAircraft_" + std::string(aircraft_name) + ".fhm").c_str());

    set_location(location_name);
}

//------------------------------------------------------------

void hud::set_location(const char *location_name)
{
    if (!location_name || !location_name[0])
    {
        m_location = tiles();
        m_map_offset.set(0.0f, 0.0f);
        m_map_scale = 1.0/150.0f;
        m_location_name.clear();
        return;
    }

    std::string name(location_name);
    if (m_location_name == name)
        return;
    m_location_name = name;

    if (name == "ms30")
    {
        name = "on_cap02";
        m_map_scale = 1.0/60.0f;
        m_map_offset.set(-5000.0f, -3000.0f);
    }

    if (name == "ms06")
        m_map_offset.y = 10000.0f;

    m_location.load(("Hud/mission/hudMission_" + name + ".fhm").c_str());
}

//------------------------------------------------------------

void hud::set_hide(bool value)
{
    if(!value && value !=m_hide)
    {
        //hud reset state

        for (auto &l: m_locks)
            l.active = true, l.locked = false;
    }

    m_hide = value;
}

//------------------------------------------------------------

void hud::update(int dt)
{
    m_anim_time += dt;
    m_anim_time = m_anim_time % 1000;

    if (m_popup_time > 0 && m_popup_priority < popup_priority_mission_result)
        m_popup_time -= dt;

    if (m_radio_time > 0)
        m_radio_time -= dt;
}

//------------------------------------------------------------

void hud::draw(const render &r)
{
    if (m_popup_time > 0)
    {
        if (m_hide && m_popup_priority < popup_priority_mission_result)
            return;

        const int popup_pos_y = r.get_height() / 2 - 71;
        const int twidth = m_fonts.get_text_width(m_popup_text.c_str(), "Zurich30");

        static std::vector<vec2> popup_quad(5);
        const int popup_width = twidth + 20;
        popup_quad[0].x = (r.get_width() - popup_width) / 2, popup_quad[0].y = popup_pos_y;
        popup_quad[1].x = (r.get_width() - popup_width) / 2, popup_quad[1].y = popup_pos_y + 30;
        popup_quad[2].x = (r.get_width() + popup_width) / 2, popup_quad[2].y = popup_pos_y + 30;
        popup_quad[3].x = (r.get_width() + popup_width) / 2, popup_quad[3].y = popup_pos_y;
        popup_quad[4] = popup_quad[0];
        r.draw(popup_quad, m_popup_color);

        m_fonts.draw_text(r, m_popup_text.c_str(), "Zurich30", (r.get_width() - twidth)/ 2, popup_pos_y - 2, m_popup_color);
    }

    if (m_radio_time > 0)
    {
        int radio_pos_y = 50;

        const int nwidth = m_fonts.get_text_width(m_radio_name.c_str(), "Zurich30");
        m_fonts.draw_text(r, m_radio_name.c_str(), "Zurich30", (r.get_width() - nwidth)/ 2, radio_pos_y, m_radio_color);
        radio_pos_y += 35;

        static std::vector<vec2> radio_line(2);
        radio_line[0].x = (r.get_width() - nwidth) / 2, radio_line[0].y = radio_pos_y;
        radio_line[1].x = (r.get_width() + nwidth) / 2, radio_line[1].y = radio_pos_y;
        r.draw(radio_line, m_radio_color);
        radio_pos_y += 10;

        size_t idx = 0;
        for (auto &t: m_radio_text)
        {
            int twidth = m_fonts.get_text_width(t.c_str(), "Zurich30");

            ++idx;
            if (idx == 1)
                twidth += m_fonts.get_text_width(L"<< ", "Zurich30");
            if (idx == m_radio_text.size())
                twidth += m_fonts.get_text_width(L" >>", "Zurich30");

            int x = (r.get_width() - twidth)/ 2;

            if (idx == 1)
                x += m_fonts.draw_text(r, L"<< ", "Zurich30", x, radio_pos_y, m_radio_color);

            x += m_fonts.draw_text(r, t.c_str(), "Zurich30", x, radio_pos_y, white);

            if (idx == m_radio_text.size())
                m_fonts.draw_text(r, L" >>", "Zurich30", x, radio_pos_y, m_radio_color);

            radio_pos_y += 35;
        }
    }

    if (m_hide)
        return;

    const float anim = fabsf(m_anim_time / 500.0f - 1.0f);

    vec2 jam_glitch;
    if (m_jammed)
        jam_glitch.set((rand() % 2000 - 1000) * 0.005f, (rand() % 2000 - 1000) * 0.005f);

    auto alert_color = green;
    if (!m_alerts.empty())
        alert_color = red;

    for (auto &z: m_zones)
    {
        if (z.mode_point)
        {
            vec2 p;
            if (!get_project_pos(r, z.pos, p))
                continue;

            m_common.draw(r, 173, p.x, p.y, green);
        }
        else
            z.mesh.draw();
    }

    //m_common.debug_draw_tx(r);
    //m_aircraft.debug_draw_tx(r);
    //m_common.debug_draw(r, debug_variable::get()); static int last_idx = 0; if (last_idx != debug_variable::get()) printf("idx %d id %d\n", debug_variable::get(), m_common.get_id(debug_variable::get())); last_idx = debug_variable::get();
    //m_aircraft.debug_draw(r, debug_variable::get()); static int last_idx = 0; if (last_idx != debug_variable::get()) printf("%d %d\n", debug_variable::get(), m_aircraft.get_id(debug_variable::get())); last_idx = debug_variable::get();

    //m_common.draw(r, 3, green);
    //m_common.draw(r, 158, green);
    //m_common.draw(r, 159, green);
    //m_common.draw(r, 214, green);

    wchar_t buf[255];
    swprintf(buf, sizeof(buf), L"pos   %ld   %ld   %ld", long(m_pos.x),long(m_pos.y),long(m_pos.z));
    m_fonts.draw_text(r, buf, "Zurich14", 20, r.get_height() - 20, green);

    vec2 proj_pos;
    if (get_project_pos(r, m_project_pos, proj_pos))
    {
        proj_pos.x -= 1.0f;

        if (m_jammed)
            proj_pos += jam_glitch;

        m_fonts.draw_text(r, L"SPEED", "Zurich14", proj_pos.x - 210, proj_pos.y - 33, alert_color);
        swprintf(buf, sizeof(buf), L"%4d", (int)m_speed);
        m_fonts.draw_text(r, buf, "OCRB15", proj_pos.x - 212, proj_pos.y - 8, alert_color);
        m_fonts.draw_text(r, L"ALT", "Zurich14", proj_pos.x + 170, proj_pos.y - 33, alert_color);
        swprintf(buf, sizeof(buf), L"%5d", (int)m_alt);
        m_fonts.draw_text(r, buf, "OCRB15", proj_pos.x + 158, proj_pos.y - 8, alert_color);

        m_common.draw(r, 10, proj_pos.x - 150, proj_pos.y, alert_color);
        m_common.draw(r, 16, proj_pos.x + 150, proj_pos.y, alert_color);
        if (m_ab)
            m_common.draw(r, 5, proj_pos.x - 245,  proj_pos.y + 2, alert_color);

        //m_common.draw(r, 215, proj_pos.x, proj_pos.y, green);
        m_common.draw(r, m_mgun ? 141 : 2, proj_pos.x, proj_pos.y, green);

        if (m_saam_visible)
        {
            render::transform t;
            t.x = proj_pos.x;
            t.y = proj_pos.y;
            r.draw(m_saam_mesh, m_saam_tracking ? red : green, t, m_saam_locked);
        }

        if (m_mgp)
            m_common.draw(r, 145, proj_pos.x, proj_pos.y, green);

        if (m_bomb_target_enabled)
        {
            static std::vector<vec2> line(2);
            line[0] = proj_pos;
            if (get_project_pos(r, m_bomb_target.p, line[1]))
            {
                m_bomb_target_mesh.set_pos(m_bomb_target.p);
                m_bomb_target_mesh.set_radius(m_bomb_target.r);
                m_bomb_target_mesh.set_color(m_bomb_target.c);
                m_bomb_target_mesh.draw();

                r.draw(line, m_bomb_target.c);
            }
        }

        if (m_pitch_ladder)
        {
            //pitch ladder

            static std::vector<vec2> lines;
            lines.clear();

            float offset = m_pitch * 180.0f / nya_math::constants::pi;
            int idx_offset = offset / 5;
            float doff = offset - idx_offset * 5;
            for (int i = 0; i < 2; ++i)
            {
                float right = i == 0 ? 1.0f : -1.0f;
                for (int j = -3; j <= 3; ++j)
                {
                    const float y = (j - doff / 5) * 56;
                    if (fabsf(y) > 140.0f)
                        continue;

                    int idx = idx_offset + j;
                    vec2 rp = vec2(right * 105, y);
                    if (idx <= 0)
                    {
                        lines.push_back(rp - vec2(right * 32, 0)), lines.push_back(rp);
                        if (idx < 0)
                        {
                            lines.push_back(rp), lines.push_back(rp + vec2(0, 8));
                            idx = -idx;
                        }
                    }
                    else
                    {
                        for (int i = 0; i < 3; ++i)
                            lines.push_back(rp - vec2(right * 11 * i, 0)), lines.push_back(rp - vec2(right * (10 * i + 8), 0));

                        lines.push_back(rp), lines.push_back(rp + vec2(0, -8));
                    }

                    auto fp = vec2(right * 120, rp.y).rotate(-m_roll) + proj_pos;
                    auto t = std::to_wstring(idx * 5);
                    m_fonts.draw_text(r, t.c_str(), "Zurich12", fp.x - m_fonts.get_text_width(t.c_str(), "Zurich12")/2, fp.y - 6, alert_color);
                }
            }

            render::transform t;
            t.x = proj_pos.x;
            t.y = proj_pos.y;
            t.yaw = -m_roll;
            r.draw(lines, alert_color, t, false);

            //compass

            const float yaw = m_yaw * 180.0f / nya_math::constants::pi;
            auto text = std::to_wstring(int(360 - yaw) % 360);
            m_fonts.draw_text(r, text.c_str(), "Zurich12", proj_pos.x - m_fonts.get_text_width(text.c_str(), "Zurich12")/2, proj_pos.y - 163, alert_color);

            m_common.draw(r, 1, proj_pos.x, proj_pos.y, alert_color);

            lines.clear();
            vec2 q(20, 7);
            lines.push_back(vec2(-q.x, -q.y));
            lines.push_back(vec2(q.x, -q.y));
            lines.push_back(vec2(q.x, q.y));
            lines.push_back(vec2(-q.x, q.y));
            lines.push_back(vec2(-q.x, -q.y));

            t.y = proj_pos.y - 156;
            t.yaw = 0;
            r.draw(lines, alert_color, t, true);

            lines.clear();

            int idx_yoff = yaw / 5;
            auto yoff = yaw - idx_yoff * 5.0f;
            for (int i = -5; i <= 5; ++i)
            {
                const float x = q.x - (i - yoff / 5) * 33;
                if (fabsf(x) < 25.0f || fabsf(x) > 110.0f)
                    continue;

                if ((idx_yoff + i) * 5 % 90)
                {
                    lines.push_back(vec2(x, -3)), lines.push_back(vec2(x, 3));
                    continue;
                }

                auto lidx = ((idx_yoff + i) * 5 / 90 + 4) % 4;
                wchar_t letters[][2] = {L"N", L"W", L"S", L"E"};
                m_fonts.draw_text(r, letters[lidx], "Zurich14", proj_pos.x + x - m_fonts.get_text_width(letters[lidx], "Zurich14") / 2, t.y - 9, green);
            }

            r.draw(lines, alert_color, t, false);
        }
    }

    if (m_bomb_target_enabled)
    {
        for (auto &t: m_bomb_marks)
        {
            m_bomb_target_mesh.set_pos(t.p);
            m_bomb_target_mesh.set_radius(t.r);
            m_bomb_target_mesh.set_color(t.c);
            m_bomb_target_mesh.draw();
        }
    }

    //small lock icons

    for (int i = 0; i < (int)m_locks.size(); ++i)
    {
        const auto &l = m_locks[i];
        const int pos_x = (i - (int)m_locks.size() / 2 + (m_locks.size() % 2 ? 0.0f : 0.5f)) * 16;
        m_aircraft.draw(r, l.active ? m_lock_icon_active : m_lock_icon, r.get_width()/2 + pos_x, r.get_height()/2 + 30, l.locked ? red : green);
    }

    //targets

    int min_enemy_range = 100000000;
    for (auto &t: m_targets)
    {
        const int range = int((m_pos - t.pos).length());
        if (t.t != target_air_ally && range < min_enemy_range)
            min_enemy_range = range;

        if (t.t == target_missile)
            continue;

        if (!get_project_pos(r, t.pos, proj_pos))
            continue;

        int icon = 102;
        auto color = green;

        const bool is_far = range > 10000;
        if (is_far)
            icon = 121;

        if (t.t == target_air_lock || t.t == target_ground_lock)
        {
            const bool is_special = !m_locks.empty();
            color = red;
            m_common.draw(r, is_special ? 101 : 100, proj_pos.x, proj_pos.y, red);
        }

        if (t.t == target_ground || t.t == target_ground_lock)
            icon = is_far ? 123 : 104;

        if (t.t == target_air_ally || t.t == target_ground_ally)
        {
            icon = 106;
            color = blue;
        }

        m_common.draw(r, icon, proj_pos.x, proj_pos.y, color);
        m_common.draw(r, icon + 1, proj_pos.x, proj_pos.y, color);

        if (t.s != select_not)
            m_common.draw(r, t.s == select_current ? 117 : 116, proj_pos.x, proj_pos.y, color);

        if (is_far)
            continue;

        if (t.t != target_air_ally)
        {
            swprintf(buf, sizeof(buf), L"%d", range);
            m_fonts.draw_text(r, buf, "Zurich14", proj_pos.x + 17, proj_pos.y - 25, color);
        }

        if (t.tgt)
            m_fonts.draw_text(r, L"TGT", "Zurich14", proj_pos.x - 40, proj_pos.y - 25, red);

        m_fonts.draw_text(r, t.name.c_str(), "Zurich14", proj_pos.x + 17, proj_pos.y - 10, color);
        m_fonts.draw_text(r, t.player_name.c_str(), "Zurich14", proj_pos.x + 17, proj_pos.y + 4, color);
    }

    if (m_show_map)
    {
        const int map_center_x = 250, map_center_y = r.get_height()-250;

        m_location.draw(r, 350, map_center_x, map_center_y, white);

        m_common.draw(r, 181, map_center_x + (m_pos.x - m_map_offset.x) * m_map_scale,
                      map_center_y + (m_pos.z - m_map_offset.y) * m_map_scale, green, nya_math::constants::pi - m_yaw);
    }
    else
    {
        //radar

        const int radar_range = min_enemy_range < 1000 ? 1000 : ( min_enemy_range < 5000 ? 5000 : 15000);
        const int radar_center_x = 185 + jam_glitch.x, radar_center_y = r.get_height()-140 + jam_glitch.y, radar_radius = 75;
        const int frame_half_size = radar_radius + 5;

        static std::vector<vec2> radar_frame(5);
        radar_frame[0].x = radar_frame[1].x = radar_center_x - frame_half_size;
        radar_frame[2].x = radar_frame[3].x = radar_center_x + frame_half_size;
        radar_frame[0].y = radar_frame[3].y = radar_center_y + frame_half_size;
        radar_frame[1].y = radar_frame[2].y = radar_center_y - frame_half_size;
        radar_frame[4] = radar_frame[0];
        static std::vector<rect_pair> bg(1);
        bg[0].r.x = radar_center_x - frame_half_size;
        bg[0].r.y = radar_center_y - frame_half_size;
        bg[0].r.w = frame_half_size * 2;
        bg[0].r.h = frame_half_size * 2;
        r.draw(bg, shared::get_white_texture(), color(0.0f, 0.02f, 0.0f, 0.5f));

        for (auto &e: m_ecms)
        {
            auto d = vec2::rotate(vec2(m_pos.x - e.x, m_pos.z - e.z), (float)m_yaw);
            const float len = d.length();
            if (len > radar_range)
                continue;

            if (len > 0.01f)
                d *= float(radar_radius) / radar_range;

            const float scale = radar_range > 1000 ? 0.6f : 1.6f;
            m_common.draw(r, radar_range > 5000 ? 207 : 208, radar_center_x + d.x, radar_center_y + d.y, green, 0.0f, scale);
        }

        r.draw(radar_frame, green);

        m_common.draw(r, 214, radar_center_x, radar_center_y, green); //circle
        if (radar_range>5000)
        {
            m_common.draw(r, 211, radar_center_x, radar_center_y, green);
            m_common.draw(r, 212, radar_center_x, radar_center_y, green);
        }
        else if (radar_range>1000)
            m_common.draw(r, 213, radar_center_x, radar_center_y, green);
        m_common.draw(r, 183, radar_center_x, radar_center_y, green); //my aircraft

        auto north_vec = -vec2::rotate(vec2(0.0f, radar_radius - 5.0f), (float)m_yaw);
        m_fonts.draw_text(r, L"N", "Zurich14", radar_center_x + north_vec.x - 4, radar_center_y + north_vec.y - 8, green);
        m_fonts.draw_text(r, L"S", "Zurich14", radar_center_x - north_vec.x - 3, radar_center_y - north_vec.y - 8, green);
        m_fonts.draw_text(r, L"E", "Zurich14", radar_center_x - north_vec.y - 3, radar_center_y + north_vec.x - 8, green);
        m_fonts.draw_text(r, L"W", "Zurich14", radar_center_x + north_vec.y - 5, radar_center_y - north_vec.x - 8, green);

        static std::vector<vec2> radar_ang(3);
        for (auto &r: radar_ang)
            r.x = radar_center_x, r.y = radar_center_y;
        auto radar_ang_vec = -vec2::rotate(vec2(0.0f, radar_radius + 15.0f), (60.0f*0.5f) * nya_math::constants::pi/180.0f);
        radar_ang[0].x -= radar_ang_vec.x, radar_ang[0].y += radar_ang_vec.y,
        radar_ang[2].x += radar_ang_vec.x, radar_ang[2].y += radar_ang_vec.y,
        r.draw(radar_ang, green);

        for (auto &t: m_targets)
        {
            auto d = vec2::rotate(vec2(m_pos.x - t.pos.x, m_pos.z - t.pos.z), (float)m_yaw);
            const float len = d.length();
            if (len > radar_range)
                continue;

            if (len > 0.01f)
                d *= float(radar_radius) / radar_range;

            int icon = 181;
            auto color = white;

            if (t.t == target_ground || t.t == target_ground_lock || t.t == target_ground_ally)
                icon = 187;

            if (t.t == target_air_ally || t.t == target_ground_ally)
                color = blue;
            else if (t.t == target_missile)
                color = white, icon = 209;
            else
                color = red;

            //if (t.t == target_air_lock)
            //    icon = 185;

            if (t.s == select_current)
                color.w = anim;

            m_common.draw(r, icon, radar_center_x + d.x, radar_center_y + d.y, color, m_yaw - t.yaw);
        }
    }

    //target arrow

    if (m_target_arrow_visible)
    {
        const float proj_scale = 100.0f;
        const float arrow_length = 3.0f * proj_scale;
        const float arrow_offset = 5.0f * proj_scale;
        const float arrow_width = 0.2f * proj_scale;

        const nya_math::vec3 arrow_origin3 = m_project_pos + m_target_arrow_dir * arrow_offset;

        vec2 arrow_origin, arrow_end;
        if(get_project_pos(r, arrow_origin3, arrow_origin) &&
           get_project_pos(r, arrow_origin3 + m_target_arrow_dir * arrow_length, arrow_end))
        {
            const nya_math::vec3 arrow_up3 = nya_math::vec3::normalize(fabsf(m_target_arrow_dir.dot(nya_math::vec3::forward())) < 0.5f ?
                                                                       nya_math::vec3::cross(m_target_arrow_dir, nya_math::vec3::forward()) :
                                                                       nya_math::vec3::cross(m_target_arrow_dir, nya_math::vec3::right()));

            const auto arrow_side3 = nya_math::vec3::cross(arrow_up3, m_target_arrow_dir);

            static const float sin60 = sin(nya_math::angle_deg(60.0f));
            static const float cos60 = cos(nya_math::angle_deg(60.0f));

            const auto arrow_rside3 = arrow_side3 * sin60 - arrow_up3 * cos60;
            const auto arrow_lside3 = -arrow_side3 * sin60 - arrow_up3 * cos60;

            const auto arrow_back3 = arrow_origin3 - m_target_arrow_dir * arrow_width * 2.0f;

            static std::vector<vec2> arrow(8);
            arrow[0] = arrow_origin;
            arrow[1] = arrow_end;
            get_project_pos(r, arrow_back3 + arrow_up3 * arrow_width, arrow[2]);
            arrow[3] = arrow_origin;
            get_project_pos(r, arrow_back3 + arrow_rside3 * arrow_width, arrow[4]);
            arrow[5] = arrow_end;
            get_project_pos(r, arrow_back3 + arrow_lside3 * arrow_width, arrow[6]);
            arrow[7] = arrow_origin;

            r.draw(arrow, green);
        }
    }

    //missiles

    m_aircraft.draw(r, m_missiles_icon + 401, r.get_width() - 110, r.get_height() - 60, green);
    m_aircraft.draw(r, m_missiles_icon + 406, r.get_width() - 110, r.get_height() - 60, green);

    m_fonts.draw_text(r, m_missiles_name.c_str(), "NowGE20", r.get_width() - 190, r.get_height() - 120, green);
    swprintf(buf, sizeof(buf), L"%d", (int)m_missiles_count);
    m_fonts.draw_text(r, buf, "ZurichXCnBT52", r.get_width() - 145 - m_fonts.get_text_width(buf, "ZurichXCnBT52"), r.get_height() - 103, green);

    if (!m_alerts.empty())
    {
        const int malert_pos_y = 235;

        alert_color.w = anim;
        static std::vector<vec2> malert_quad(5);
        const int malert_width = 150;
        malert_quad[0].x = (r.get_width() - malert_width) / 2, malert_quad[0].y = malert_pos_y;
        malert_quad[1].x = (r.get_width() - malert_width) / 2, malert_quad[1].y = malert_pos_y + 25;
        malert_quad[2].x = (r.get_width() + malert_width) / 2, malert_quad[2].y = malert_pos_y + 25;
        malert_quad[3].x = (r.get_width() + malert_width) / 2, malert_quad[3].y = malert_pos_y;
        malert_quad[4] = malert_quad[0];
        r.draw(malert_quad, alert_color);
        m_fonts.draw_text(r, L"MISSILE ALERT", "Zurich20", r.get_width()/2-60, malert_pos_y, alert_color);

        const float camera_rot = m_yaw + nya_scene::get_camera().get_rot().get_euler().y + nya_math::constants::pi;
        const float ar = 0.77f - camera_rot;
        for (auto &a: m_alerts)
            m_common.draw(r, 170, r.get_width()/2, r.get_height()/2, red, ar + a);
    }

    if (m_show_team_score)
    {
        const int h = 0; //r.get_height() - 70
        m_fonts.draw_text(r, m_team_score[0].c_str(), "ZurichXCnBT52", r.get_width()/2 - 30 - m_fonts.get_text_width(m_team_score[0].c_str(), "ZurichXCnBT52"), h, blue);
        m_fonts.draw_text(r, m_team_score[1].c_str(), "ZurichXCnBT52", r.get_width()/2 + 30, h, red);
    }

    int scores_y = 5;
    for (auto &s: m_scores)
    {
        if (s.place >= 0)
        {
            int pos = 10;
            wchar_t buf[16];
            swprintf(buf, sizeof(buf), L"%2d", s.place);
            m_fonts.draw_text(r, buf, "Zurich14", pos, scores_y, green);
            pos += 30;
            m_fonts.draw_text(r, s.name.c_str(), "Zurich14", pos, scores_y, green);
            pos += 200 - m_fonts.get_text_width(s.value.c_str(), "Zurich14");
            m_fonts.draw_text(r, s.value.c_str(), "Zurich14", pos, scores_y, green);
        }

        scores_y += 20;
    }

    for (auto &t: m_texts)
        m_fonts.draw_text(r, t.t.c_str(), t.f.c_str(), t.x, t.y, green);
}

//------------------------------------------------------------

void hud::add_zone(nya_math::vec3 pos)
{
    zone z;
    z.pos = pos;
    z.mode_point = true;
    m_zones.push_back(z);
}

//------------------------------------------------------------

void hud::add_zone(nya_math::vec3 pos, float radius, circle_mesh::height_function get_height, bool solid)
{
    zone z;
    z.pos = pos;
    z.mesh.init(pos, radius, 36, get_height, solid ? 1000.0 : 0.0f);
    auto color = green;
    if (solid)
        color.w *= 0.3f;
    z.mesh.set_color(color);
    m_zones.push_back(z);
}

//------------------------------------------------------------

void hud::clear_scores()
{
    m_show_team_score = false;
    m_scores.clear();
}

//------------------------------------------------------------

void hud::set_team_score(int allies, int enemies)
{
    m_show_team_score = true;
    m_team_score[0] = std::to_wstring(allies);
    m_team_score[1] = std::to_wstring(enemies);
}

//------------------------------------------------------------

void hud::set_score(int line, int place, const std::wstring &name, const std::wstring &value)
{
    if (line < 0)
        return;

    if (line >= (int)m_texts.size())
        m_scores.resize(line + 1);

    auto &s = m_scores[line];
    s.place = place;
    s.name = name;
    s.value = value;
}

//------------------------------------------------------------

void hud::remove_score(int line)
{
    if (line < 0 || line >= (int)m_texts.size())
        return;

    m_scores[line].place = -1;
}

//------------------------------------------------------------

void hud::add_text(int idx, const std::wstring &text, const std::string &font, int x, int y, const color &c)
{
    if (idx < 0)
        return;

    if (idx >= (int)m_texts.size())
        m_texts.resize(idx + 1);

    auto &t = m_texts[idx];
    t.t = text;
    t.f = font;
    t.x = x, t.y = y;
    t.c = c;
}

//------------------------------------------------------------

void hud::set_missiles(const char *id, int icon)
{
    if (id && id[0] == '_')
        ++id;

    if (id)
    {
        std::string id_str(id);
        if (id_str == "MISSILE")
            id_str = "MSL";

        if (id_str.length() < 4)
            id_str = " " + id_str;
        m_missiles_name = std::wstring(id_str.begin(), id_str.end());
    }

    m_missiles_icon = icon;
    m_bomb_target_enabled = false;
}

//------------------------------------------------------------

void hud::set_locks(int count, int icon)
{
    m_locks.resize(count);

    m_lock_icon_active = 417 + icon;
    m_lock_icon = 422 + icon;
}

//------------------------------------------------------------

void hud::set_lock(int idx, bool locked, bool active)
{
    if (idx < 0 || idx >= (int)m_locks.size())
        return;

    m_locks[idx].active = active;
    m_locks[idx].locked = locked;
}

//------------------------------------------------------------

void hud::set_target_arrow(bool visible, const nya_math::vec3 &dir)
{
    m_target_arrow_dir = dir;
    m_target_arrow_visible = visible;
}

//------------------------------------------------------------

void hud::set_saam_circle(bool visible, float angle)
{
    m_saam_visible = visible;
    if (!visible)
        return;

    const float radius = sinf(angle) * 800.0f;
    m_saam_mesh.clear();
    const int num_segments = int(radius * 0.75f);
    for(int i = 0; i < num_segments; ++i)
    {
        const float a = 2.0f * nya_math::constants::pi * float(i) / float(num_segments);
        const vec2 p(radius * cosf(a), radius * sinf(a));
        m_saam_mesh.push_back(p);
    }

    if (!m_saam_mesh.empty())
        m_saam_mesh.push_back(m_saam_mesh.front());
}

//------------------------------------------------------------

void hud::set_missile_reload(int idx, float value)
{
    m_aircraft.set_progress(m_missiles_icon + 406, idx, value);
}

//------------------------------------------------------------

void hud::add_target(const nya_math::vec3 &pos, float yaw, target_type target, select_type select)
{
    m_targets.push_back({L"", L"", pos, yaw, target, select, false});
}

//------------------------------------------------------------

void hud::add_target(const std::wstring &name, const std::wstring &player_name, const nya_math::vec3 &pos, float yaw, target_type target, select_type select, bool tgt)
{
    m_targets.push_back({name, player_name, pos, yaw, target, select, tgt});
}

//------------------------------------------------------------

void hud::set_bomb_target(const nya_math::vec3 &pos, float radius, const color &c)
{
    m_bomb_target_enabled = true;
    m_bomb_target.p = pos;
    m_bomb_target.r = radius;
    m_bomb_target.c = c;
}

//------------------------------------------------------------

void hud::add_bomb_mark(const nya_math::vec3 &pos, float radius, const color &c)
{
    bomb_target t;
    t.p = pos, t.r = radius, t.c = c;
    m_bomb_marks.push_back(t);
}

//------------------------------------------------------------

void hud::popup(const std::wstring &text, int priority, const color &c, int time)
{
    if (m_popup_time > 0 && m_popup_priority > priority)
        return;

    m_popup_text = text;
    m_popup_time = time;
    m_popup_color = c;
    m_popup_priority = priority;
}

//------------------------------------------------------------

void hud::set_radio(std::wstring name, std::wstring message, int time, const color &c)
{
    m_radio_name = name;
    if (name.empty())
    {
        m_radio_time = 0;
        return;
    }

    m_radio_color = c;
    m_radio_time = time;
    m_radio_text.clear();
    std::wstringstream wss(message);
    std::wstring l;
    while (std::getline(wss, l, L'\n'))
        m_radio_text.push_back(l);
}

//------------------------------------------------------------
}
