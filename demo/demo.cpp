//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "containers/qdf_provider.h"
#include "containers/dpl_provider.h"

#include "game/deathmatch.h"
#include "game/team_deathmatch.h"
#include "game/free_flight.h"
#include "game/hangar.h"
#include "gui/menu.h"

#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"
#include "scene/camera.h"
#include "system/system.h"

#include "util/config.h"
#include "util/controls.h"
#include "util/platform.h"

#include <thread>
#include <chrono>

#ifndef _WIN32
    #include <unistd.h>
#endif

//------------------------------------------------------------

int main(void)
{
#ifndef _WIN32
    chdir(nya_system::get_app_path());
#endif

    config::register_var("acah_path", "");

    qdf_resources_provider qdfp;
    if (!qdfp.open_archive((config::get_var("acah_path") + "datafile.qdf").c_str()))
    {
        if (platform::show_msgbox("Open Horizon", "You are running Open Horizon outside of the Assault Horizon folder. "
                                                  "You are ok to do so, but please specify the path to the Assault Horizon folder. "
                                                  "This will be saved automatically."))
        {
            config::set_var("acah_path", platform::open_folder_dialog());
            if (!qdfp.open_archive((config::get_var("acah_path") + "datafile.qdf").c_str()))
                return 0;
        }
        else
            return 0;
    }

    class target_resource_provider: public nya_resources::resources_provider
    {
        nya_resources::resources_provider &m_provider;
        dpl_resources_provider m_dlc_provider;
        nya_resources::file_resources_provider m_fprov;

    public:
        target_resource_provider(nya_resources::resources_provider &provider): m_provider(provider)
        {
            m_fprov.set_folder(nya_system::get_app_path());

            nya_resources::composite_resources_provider cprov;
            cprov.add_provider(&m_fprov);
            cprov.add_provider(&provider);
            nya_resources::set_resources_provider(&cprov);

            m_dlc_provider.open_archive("target/DATA.PAC", "DATA.PAC.xml");
        }

    private:
        nya_resources::resource_data *access(const char *resource_name)
        {
            if (!resource_name)
                return 0;

            if (m_dlc_provider.has(resource_name))
                return m_dlc_provider.access(resource_name);

            const std::string str(resource_name);
            if (m_provider.has(("target/" + str).c_str()))
                return m_provider.access(("target/" + str).c_str());

            if (m_provider.has(("common/" + str).c_str()))
                return m_provider.access(("common/" + str).c_str());

            if (m_provider.has(resource_name))
                return m_provider.access(resource_name);

            return m_fprov.access(resource_name);
        }

        bool has(const char *resource_name)
        {
            if (!resource_name)
                return false;

            const std::string str(resource_name);
            return m_provider.has(("target/" + str).c_str()) || m_provider.has(("common/" + str).c_str()) || m_provider.has(resource_name);
        }

    } trp(qdfp);

    nya_resources::set_resources_provider(&trp);

    config::register_var("screen_width", "1000");
    config::register_var("screen_height", "562");

    platform platform;
    if (!platform.init(config::get_var_int("screen_width"), config::get_var_int("screen_height"), "Open Horizon 4th demo"))
        return -1;

    std::vector<joystick_config> joysticks;

    for (int i = 0; glfwJoystickPresent(i); ++i)
    {
        const char *name = glfwGetJoystickName(i);

        joystick_config j;
        j.init(name);
        joysticks.push_back(j);

        int axis_count = 0, buttons_count = 0;
        glfwGetJoystickAxes(i, &axis_count);
        glfwGetJoystickButtons(i, &buttons_count);
        printf("joy%d: %s %d axis %d buttons\n", i, name, axis_count, buttons_count);
    }

    renderer::scene scene;
    game::world world(scene, scene.hud);
    game::free_flight game_mode_ff(world);
    game::deathmatch game_mode_dm(world);
    game::team_deathmatch game_mode_tdm(world);
    game::hangar hangar(scene);
    game::game_mode *active_game_mode = 0;
    game::plane_controls controls;
    game::network_client client;
    game::network_server server;

    gui::menu menu;
    gui::menu_controls menu_controls;

    platform::key_callback kcb = std::bind(&gui::menu::on_input, &menu, std::placeholders::_1);
    platform.set_keyboard_callback(kcb);

    int mx = platform.get_mouse_x(), my = platform.get_mouse_y();
    int screen_width = platform.get_width(), screen_height = platform.get_height();
    scene.resize(screen_width, screen_height);
    scene.loading(true);
    nya_render::clear(true, true);
    scene.draw();
    platform.end_frame();

    menu.init();
    bool viewer_mode = false;
    bool is_client = false;

    gui::menu::on_action on_menu_action = [&](const std::string &event)
    {
        if (event == "start")
        {
            auto location = menu.get_var("map");
            auto plane = menu.get_var("ac");
            const int color = atoi(menu.get_var("color").c_str());

            is_client = false;

            scene.loading(true);
            nya_render::clear(true, true);
            scene.draw();
            platform.end_frame();
            scene.loading(false);

            auto mode = menu.get_var("mode");
            auto mp_var = menu.get_var("multiplayer");
            if (mp_var == "server")
            {
                world.set_network(&server);
                auto port = menu.get_var_int("port");
                server.open(port, mode.c_str(), menu.get_var("map").c_str(), menu.get_var_int("max_players"));
                if (menu.get_var("mp_public") == "true")
                    game::servers_list::register_server(port);
            }
            else if (mp_var == "client")
            {
                world.set_network(&client);
                client.start();
                is_client = true;
            }

            if (mode == "dm")
            {
                active_game_mode = &game_mode_dm;
                game_mode_dm.start(plane.c_str(), color, 0, location.c_str(), 12);
            }
            else if (mode == "tdm")
            {
                active_game_mode = &game_mode_tdm;
                game_mode_tdm.start(plane.c_str(), color, 0, location.c_str(), 8);
            }
            else if (mode == "ff")
            {
                active_game_mode = &game_mode_ff;
                game_mode_ff.start(plane.c_str(), color, location.c_str());
            }
        }
        else if (event == "connect")
        {
            client.disconnect();
            auto port = menu.get_var_int("port");
            if (client.connect(menu.get_var("address").c_str(), port))
            {
                menu.send_event("map=" + client.get_server_info().location);
                menu.send_event("mode=" + client.get_server_info().game_mode);
                menu.send_event("screen=ac_select");
            }
            else
                printf("unable to connect to server\n"); //ToDo: show error
        }
        else if (event == "viewer_start")
        {
            viewer_mode = true;
            scene.camera.add_delta_rot(0.2f, 2.5f);
        }
        else if (event == "viewer_update_bg")
        {
            hangar.set_bkg(menu.get_var("bkg").c_str());
        }
        else if (event == "viewer_update_ac")
        {
            const auto dr = scene.camera.get_delta_rot();
            hangar.set_plane(menu.get_var("ac").c_str());
            scene.camera.add_delta_rot(dr.x, dr.y - 3.14f);
        }
        else if (event == "viewer_update_color")
        {
            const auto dr = scene.camera.get_delta_rot();
            const int color = atoi(menu.get_var("color").c_str());
            hangar.set_plane_color(color);
            scene.camera.add_delta_rot(dr.x, dr.y - 3.14f);
        }
        else if (event == "viewer_end")
        {
            viewer_mode = false;
            hangar.end();
        }
        else if (event == "exit")
        {
            server.close();
            client.disconnect();
            platform.terminate();
        }
        else
            printf("unknown event: %s\n", event.c_str());
    };

    menu.set_callback(on_menu_action);

    unsigned long app_time = nya_system::get_time();
    while (!platform.should_terminate())
    {
        unsigned long time = nya_system::get_time();
        int dt = int(time - app_time);
        if (dt > 1000)
            dt = 1000;

        app_time = time;

        static bool speed10x = false, last_pause = false, paused = false;

        if (platform.get_width() != screen_width || platform.get_height() != screen_height)
        {
            screen_width = platform.get_width(), screen_height = platform.get_height();
            scene.resize(screen_width, screen_height);

            config::set_var("screen_width", std::to_string(screen_width));
            config::set_var("screen_height", std::to_string(screen_height));
        }

        if (!active_game_mode)
            menu.update(dt, menu_controls);

        if (active_game_mode)
        {
            if (!paused)
                active_game_mode->update(speed10x ? dt * 10 : dt, controls);
            scene.draw();

            //util debug draw
            nya_render::clear(false, true);
            nya_render::set_state(nya_render::state());
            nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
            get_debug_draw().set_line_width(2.0f);
            get_debug_draw().set_point_size(3.0f);
            get_debug_draw().draw();
        }
        else
        {
            nya_render::clear(true, true);

            if (viewer_mode)
            {
                hangar.update(dt);
                scene.draw();
            }

            menu.draw(scene.ui_render);
        }

        const char *ui_ref = 0;
        //ui_ref = "ui_ref4.tga";
        if (ui_ref)
        {
            static nya_scene::texture ui_ref_texture(ui_ref);
            static std::vector<gui::rect_pair> ui_ref_rects(1);
            ui_ref_rects[0].r.w = scene.ui_render.get_width();
            ui_ref_rects[0].r.y = scene.ui_render.get_height();
            ui_ref_rects[0].r.h = -scene.ui_render.get_height();
            ui_ref_rects[0].tc.w = ui_ref_texture.get_width();
            ui_ref_rects[0].tc.h = ui_ref_texture.get_height();
            static int alpha_anim = 0;
            alpha_anim += dt;
            alpha_anim = alpha_anim % 4000;
            //alpha_anim = 1000;
            scene.ui_render.draw(ui_ref_rects, ui_ref_texture, nya_math::vec4(1.0, 1.0, 1.0, fabsf(alpha_anim / 2000.0f - 1.0)));
        }

        platform.end_frame();

        //controls

        controls = game::plane_controls();
        menu_controls = gui::menu_controls();

        if (platform.get_mouse_lbtn())
            scene.camera.add_delta_rot((platform.get_mouse_y() - my) * 0.03, (platform.get_mouse_x() - mx) * 0.03);

        if (platform.get_mouse_rbtn())
            scene.camera.add_delta_pos(0, 0, my - platform.get_mouse_y());

        mx = platform.get_mouse_x(), my = platform.get_mouse_y();

        bool pause = false;
        for (int i = 0; i < (int)joysticks.size(); ++i)
        {
            int axes_count = 0, buttons_count = 0;
            const float *axes = glfwGetJoystickAxes(i, &axes_count);
            const unsigned char *buttons = glfwGetJoystickButtons(i, &buttons_count);
            joysticks[i].update(axes, axes_count, buttons, buttons_count);
            joysticks[i].apply_controls(controls, pause);
            joysticks[i].apply_controls(menu_controls);
        }

        if (platform.get_key(GLFW_KEY_W)) controls.throttle = 1.0f;
        if (platform.get_key(GLFW_KEY_S)) controls.brake = 1.0f;
        if (platform.get_key(GLFW_KEY_A)) controls.rot.y = -1.0f;
        if (platform.get_key(GLFW_KEY_D)) controls.rot.y = 1.0f;
        if (platform.get_key(GLFW_KEY_UP)) controls.rot.x = 1.0f, menu_controls.up = true;
        if (platform.get_key(GLFW_KEY_DOWN)) controls.rot.x = -1.0f, menu_controls.down = true;
        if (platform.get_key(GLFW_KEY_LEFT)) controls.rot.z = -1.0f, menu_controls.left = true;
        if (platform.get_key(GLFW_KEY_RIGHT)) controls.rot.z = 1.0f, menu_controls.right = true;

        if (platform.get_key(GLFW_KEY_LEFT_CONTROL)) controls.mgun = true;
        if (platform.get_key(GLFW_KEY_LEFT_SHIFT)) controls.mgun = true;
        if (platform.get_key(GLFW_KEY_SPACE)) controls.missile = true, menu_controls.next = true;
        if (platform.get_key(GLFW_KEY_F)) controls.flares = true;
        if (platform.get_key(GLFW_KEY_Q)) controls.change_weapon = true;
        if (platform.get_key(GLFW_KEY_E)) controls.change_target = true;
        if (platform.get_key(GLFW_KEY_R)) controls.change_radar = true;
        if (platform.get_key(GLFW_KEY_V)) controls.change_camera = true;

        if (platform.was_pressed(GLFW_KEY_ESCAPE)) menu_controls.prev = true;
        if (platform.was_pressed(GLFW_KEY_ENTER)) menu_controls.next = true;

        if (active_game_mode)
        {
            if ((pause && pause != last_pause) || platform.was_pressed(GLFW_KEY_P))
                scene.pause(paused = !paused);
            last_pause = pause;

            bool should_stop = platform.was_pressed(GLFW_KEY_ESCAPE);
            if (is_client && !client.is_started())
                should_stop = true;

            if (should_stop)
            {
                active_game_mode->end();
                active_game_mode = 0;
                server.close();
                client.disconnect();
                world.set_network(0);
                menu_controls.prev = false;
            }
        }

        speed10x = platform.get_key(GLFW_KEY_RIGHT_SHIFT);

        if (controls.change_camera)
            scene.camera.reset_delta_rot();

        if (platform.was_pressed(GLFW_KEY_COMMA))
            debug_variable::set(debug_variable::get() - 1);
        if (platform.was_pressed(GLFW_KEY_PERIOD))
            debug_variable::set(debug_variable::get() + 1);

        if (!active_game_mode) //force limit 60 fps in menu
        {
            int sleep_time = 1000/60 - int(nya_system::get_time() - time);
            if (sleep_time > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
    }

    server.close();
    client.disconnect();
    platform.terminate();

    return 0;
}

//------------------------------------------------------------
