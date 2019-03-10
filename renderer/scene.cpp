//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene.h"
#include "shared.h"
#include "scene/camera.h"
#include "util/location.h"
#include "renderer/texture.h"
#include <algorithm>

namespace renderer
{

//------------------------------------------------------------

nya_scene::texture load_tonecurve(const char *file_name)
{
    nya_scene::shared_texture t;
    auto res = load_resource(file_name);
    assert(res.get_size() == 256 * 3);

    const unsigned char *data = (const unsigned char *)res.get_data();
    unsigned char color[256 * 3];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 256; ++j)
            color[j*3+i]=data[j+i*256];
    }

    t.tex.build_texture(color, 256, 1, nya_render::texture::color_rgb);
    t.tex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
    res.free();
    
    nya_scene::texture result;
    result.create(t);
    return result;
}

//------------------------------------------------------------

void scene::setup_shadow_camera(aircraft_ptr a)
{
    if (!a.is_valid())
        return;

    const float hsize = std::max(fabsf(a->get_wing_offset().x) * 1.2f, 7.0f);//ToDo: aabb.dir.length()
    nya_math::mat4 proj;
    proj.ortho(-hsize, hsize, -hsize, hsize, -hsize, hsize);
    m_shadow_camera->set_proj(proj);
    m_shadow_camera->set_rot(m_location.get_params().sky.sun_dir);
    a->setup_shadows(get_texture("shadows").get(), m_shadow_camera->get_view_matrix() * m_shadow_camera->get_proj_matrix());
}

//------------------------------------------------------------

aircraft_ptr scene::add_aircraft(const char *name, int color, bool player)
{
    auto a = world::add_aircraft(name, color, player);
    if (!player)
        return a;

    setup_shadow_camera(a);

    m_cam_fp_off = a->get_bone_pos("ckpp");

    //ToDo: research camera
    auto off = -a->get_camera_offset();
    camera.set_ignore_delta_pos(false);
    camera.reset_delta_pos();
    camera.reset_delta_rot();
    //camera.add_delta_pos(off.x, off.y * 1.5, off.z * 0.5);
    camera.add_delta_pos(off.x, -0.5 + off.y*1.5, off.z*0.7);
    if (strcmp(name, "f22a") == 0 || strcmp(name, "kwmr") == 0 || strcmp(name, "su47") == 0 || strcmp(name, "pkfa") == 0)
        camera.add_delta_pos(0.0, -1.0, 3.0);

    if (strcmp(name, "mr2k") == 0)
        camera.add_delta_pos(0.0, 0.0, 2.0);

    if (strcmp(name, "b01b") == 0)
        camera.add_delta_pos(0.0, -4.0, -2.0);

    if (strcmp(name, "su25") == 0)
        camera.add_delta_pos(0.0, -2.0, 0.0);

    if (strcmp(name, "su34") == 0 || strcmp(name, "f16c") == 0 || strcmp(name, "f15m") == 0
        || strcmp(name, "f02a") == 0 || strcmp(name, "su37") == 0 || strcmp(name, "f35b") == 0)
        camera.add_delta_pos(0.0, -1.0, 0.0);

    camera.reset_delta_rot();
    //shared::clear_textures(); //ToDo
    return a;
}

//------------------------------------------------------------

void scene::set_location(const char *name)
{
    if (!m_curve.is_valid())
    {
        m_curve.create();
        load("postprocess.txt");
        set_texture("color_curve", m_curve);
        set_shader_param("screen_radius", nya_math::vec4(1.185185, 0.5 * 4.0 / 3.0, 0.0, 0.0));
        set_shader_param("damage_frame", nya_math::vec4(0.35, 0.5, 1.0, 0.1));
        m_flare.init(get_texture("main_color"), get_texture("main_depth"));
        m_cockpit_black.load("shaders/cockpit_black.nsh");
        m_cockpit_black_quad.init();
        m_missile_trails_renderer.init();
        m_particles_render.init();
    }

    if (is_native_location(name))
    {
        auto &zip = get_native_location_provider(name);
        auto tex = load_texture(zip, "tonecurve.tga");
        if (tex.get_width() > 0)
        {
            auto rtex = tex.internal().get_shared_data()->tex;
            rtex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
            m_curve.set(tex);
        }
        else
            m_curve.set(load_tonecurve("Map/tonecurve_default.tcb"));
    }
    else
    {
        if (!name[0] || strcmp(name, "def") == 0)
            m_curve.set(load_tonecurve("Map/tonecurve_default.tcb"));
        else
            m_curve.set(load_tonecurve(("Map/tonecurve_" + std::string(name) + ".tcb").c_str()));
    }

    world::set_location(name);

    for (auto &a: m_aircrafts)
        a->apply_location(m_location.get_ibl(), m_location.get_env(), m_location.get_params());

    if (m_player_aircraft.is_valid())
        setup_shadow_camera(m_player_aircraft);

    m_flare.apply_location(m_location.get_params());

    auto &p = m_location.get_params();
    set_shader_param("bloom_param", nya_math::vec4(p.hdr.bloom_threshold, p.hdr.bloom_offset, p.hdr.bloom_scale, 1.0));
    set_shader_param("saturation", nya_math::vec4(p.tone_saturation * 0.01, 0.0, 0.0, 0.0));
    set_shader_param("light dir", nya_math::vec4(-p.sky.sun_dir, 0.0f));

    m_luminance_speed = p.hdr.luminance_speed;
    m_fade_time = m_fade_max_time = 2000;
}

//------------------------------------------------------------

void scene::update(int dt)
{
    if (dt > 50)
        dt = 50;

    world::update(dt);

    for (auto &a: m_aircrafts)
        a->update_trail(dt, *this);

    for (auto &t: m_plane_trails)
        t.second += dt;

    m_plane_trails.erase(std::remove_if(m_plane_trails.begin(), m_plane_trails.end(), [](std::pair<plane_trail, unsigned int> &t)
                                      { return t.second > 5000; }), m_plane_trails.end());

    for (auto &t: m_missile_trails)
        t.update(dt);

    m_missile_trails.erase(std::remove_if(m_missile_trails.begin(), m_missile_trails.end(), [](missile_trail &t)
                                        { return t.is_dead(); }), m_missile_trails.end());

    if (m_player_aircraft.is_valid())
    {
        const bool is_dead = m_player_aircraft->is_dead();
        if (is_dead)
        {
            camera.set_ignore_delta_pos(true);
            camera.set_ignore_delta_rot(true);
            if(!m_was_dead)
                camera.set_fixed_dist(40.0f);
            else
                camera.set_fixed_dist(camera.get_fixed_dist() + dt * 0.01f);
            camera.set_pos(m_player_aircraft->get_pos());

            m_air.update(dt, m_player_aircraft->get_pos(), nya_math::vec3::zero());
        }
        else
        {
            const bool is_camera_third = m_player_aircraft->get_camera_mode() == aircraft::camera_mode_third;
            camera.set_ignore_delta_pos(!is_camera_third);
            camera.set_pos(m_player_aircraft->get_bone_pos(is_camera_third ? "camp" : "ckpp"));

            m_air.update(dt, m_player_aircraft->get_pos(), m_player_aircraft->get_vel());
        }

        camera.set_rot(m_player_aircraft->get_rot());

        set_shader_param("damage_frame_color", nya_math::vec4(1.0, 0.0, 0.0235, m_player_aircraft->get_damage()));
        if (m_was_dead && !is_dead)
        {
            camera.set_ignore_delta_rot(false);
            camera.set_fixed_dist(0.0f);
            m_fade_time = 1000, m_fade_max_time = m_fade_time * 1.5;
        }

        m_was_dead = is_dead;
    }

    set_shader_param("lum_adapt_speed", m_luminance_speed * dt / 1000.0f * nya_math::vec4(1.0, 1.0, 1.0, 1.0));

    if (m_fade_time > 0)
    {
        m_fade_time -= dt;
        if (m_fade_time < 0)
            m_fade_time = 0;

        set_shader_param("fade_color", nya_math::vec4(0.0, 0.0, 0.0, float(m_fade_time) / m_fade_max_time));
    }

    if (m_help_time > 0)
        m_help_time -= dt;

    hud.update(dt);

    m_frame_counter_time += dt;
    ++m_frame_counter;
    if (m_frame_counter_time > 1000)
    {
        m_fps = m_frame_counter;
        m_frame_counter = 0;
        m_frame_counter_time -= 1000;
    }
}

//------------------------------------------------------------

void scene::resize(unsigned int width,unsigned int height)
{
    nya_render::set_viewport(0, 0, width, height);

    if (!m_fonts_loaded)
    {
        ui_fonts.load("UI/text/menuCommon.acf");
        ui_render.init();
        m_fonts_loaded = true;
    }

    nya_scene::postprocess::resize(width, height);
    setup_shadow_camera(m_player_aircraft);

    if (height)
        camera.set_aspect(height > 0 ? float(width) / height : 1.0f);

    ui_render.resize(width, height);
}

//------------------------------------------------------------

void scene::draw()
{
    m_location.update_tree_texture();

    nya_scene::postprocess::draw(0);

    const auto white = nya_math::vec4(1.0, 1.0, 1.0, 1.0);

    //if (m_help_time > 0)
    //    m_ui.draw_text(L"Press 1-2 to change location, 3-4 to change plane, 5-6 to change paint", "NowGE24", 50, 100, white);

    wchar_t buf[255];
    swprintf(buf, sizeof(buf), L"FPS: %d", m_fps);
    ui_fonts.draw_text(ui_render, buf, "NowGE20", ui_render.get_width() - 90, 0, white);

    if (m_loading)
    {
        m_loading = false;
        ui_fonts.draw_text(ui_render, L"LOADING", "ZurichBD20outline", ui_render.get_width() * 0.5 - 50, ui_render.get_height() * 0.5, white);
    }
    else
    {
        hud.draw(ui_render);
        if (m_paused)
            ui_fonts.draw_text(ui_render, L"PAUSED", "ZurichBD20outline", ui_render.get_width() * 0.5 - 45, ui_render.get_height() * 0.5, white);
    }
}

//------------------------------------------------------------

void scene::draw_scene(const char *pass,const nya_scene::tags &t)
{
    camera.set_near_far(1.0, 21000.0);

    if (t.has("location"))
        m_location.draw();

    if (t.has("objects"))
    {
        for (auto &o:m_objects)
        {
            if (o->visible)
                o->mdl.draw(0);
        }
    }
    if (t.has("aircrafts"))
    {
        for (auto &a:m_aircrafts)
        {
            if (a == m_player_aircraft)
                continue;

            a->draw(0);
        }

        for (auto &m:m_missiles)
            m->mdl.draw(0);
    }
    if (t.has("player"))
    {
        if (m_player_aircraft.is_valid())
        {
            if (m_player_aircraft->get_camera_mode() == aircraft::camera_mode_third)
            {
                if(strcmp(pass, "shadows") == 0)
                {
                    auto pc = nya_scene::get_camera_proxy();
                    nya_scene::set_camera(m_shadow_camera);
                    m_player_aircraft->draw_player_shadowcaster();
                    nya_scene::set_camera(pc);
                }
                else
                    m_player_aircraft->draw_player();
            }
        }
    }
    if (t.has("particles"))
    {
        for (auto &a: m_aircrafts)
            a->draw_mgun_flash(*this);

        for (auto &t: m_plane_trails)
            m_particles_render.draw(t.first);

        for (auto &a: m_aircrafts)
            a->draw_trails(*this);

        for (auto &t: m_missile_trails)
            m_missile_trails_renderer.draw(t);

        for (auto &m: m_missiles)
            m_missile_trails_renderer.draw(m->trail);

        for (auto &a: m_aircrafts)
        {
            a->draw_engine_effect(*this);
            a->draw_fire_trail(*this);
        }

        m_particles_render.draw(m_bullets);

        auto cam_pos = nya_scene::get_camera().get_pos();
        static std::vector<std::pair<unsigned int, size_t> > sorted;
        sorted.resize(m_explosions.size());
        for (size_t i = 0; i < sorted.size(); ++i)
            sorted[i].first = (m_explosions[i].get_pos() - cam_pos).length_sq(), sorted[i].second = i;

        std::sort(sorted.rbegin(), sorted.rend());

        for (size_t i = 0; i < sorted.size(); ++i)
            m_particles_render.draw(m_explosions[sorted[i].second]);

        m_particles_render.draw(m_air);
    }
    if (t.has("heat"))
    {
        for (auto &e: m_explosions)
            m_particles_render.draw_heat(e);

        if (m_player_aircraft.is_valid())
            m_player_aircraft->draw_engine_heat_effect(*this);
    }
    if (t.has("cockpit"))
    {
        if (m_player_aircraft.is_valid() && m_player_aircraft->is_visible() && m_player_aircraft->get_camera_mode() == aircraft::camera_mode_cockpit)
        {
             nya_render::clear(false, true);
             auto cam_pos = camera.get_pos();
             camera.set_pos(m_player_aircraft->get_rot().rotate(m_cam_fp_off));
             camera.set_near_far(0.01,10.0);
             m_player_aircraft->draw_cockpit();

             //fill holes
             nya_render::set_state(nya_render::state());
             nya_render::depth_test::enable(nya_render::depth_test::not_greater);
             m_cockpit_black.internal().set();
             m_cockpit_black_quad.draw();
             m_cockpit_black.internal().unset();

             //restore
             camera.set_pos(cam_pos);
        }
    }
    if (t.has("clouds_flat"))
        m_clouds.draw_flat();
    if (t.has("clouds_obj"))
        m_clouds.draw_obj();
    if (t.has("flare"))
        m_flare.draw();
}

//------------------------------------------------------------
}
