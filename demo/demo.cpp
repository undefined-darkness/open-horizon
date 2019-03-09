//
// open horizon -- undefined_darkness@outlook.com
//

#include "game/mission.h"
#include "game/deathmatch.h"
#include "game/team_deathmatch.h"
#include "game/free_flight.h"
#include "game/network_client.h"
#include "game/network_server.h"
#include "game/hangar.h"
#include "game/world.h"
#include "gui/menu.h"

#include "scene/camera.h"
#include "render/render_buffered.h"

#include "util/controls.h"
#include "util/resources.h"
#include "util/config.h"
#include "util/platform.h"
#include "renderer/shared.h"

#include <thread>
#include <chrono>
#include <atomic>

#include "GLFW/glfw3.h"

#ifdef _WIN32
    #undef APIENTRY
    #include "log/composite_log.h"
    #include "log/windbg_log.h"
    #include <windows.h>

    int main();
    int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) 
    { 
        auto log = new nya_log::composite_log();
        log->add_log(&nya_log::log());
        log->add_log(new nya_log::windbg_log());
        nya_log::set_log(log);
        return main(); 
    }
#endif

//------------------------------------------------------------

static const int target_fps = 60;

//------------------------------------------------------------

int main()
{
    setup_resources();

    config::register_var("screen_width", "1000");
    config::register_var("screen_height", "562");
    config::register_var("fullscreen", "false");
    config::register_var("master_volume", "10");
    config::register_var("music_volume", "5");
    config::register_var("difficulty", "hard");

    platform platform;
    if (!platform.init(config::get_var_int("screen_width"), config::get_var_int("screen_height"), "Open Horizon 7th demo"))
        return -1;

    platform.set_fullscreen(config::get_var_bool("fullscreen"), config::get_var_int("screen_width"), config::get_var_int("screen_height"));

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
        nya_log::log("joy%d: %s %d axis %d buttons\n", i, name, axis_count, buttons_count);
    }
    auto render = nya_render::render_buffered(nya_render::get_api_interface());
    nya_render::set_render_api(&render);

    struct game_thread
    {
        nya_render::render_buffered &render;
        std::thread thread;
        game_thread(nya_render::render_buffered &r,int w, int h): render(r)
        {
            platform_width = w, platform_height = h;
            platform_terminate = false;
            platform_fullscreen = false;
            frame_pending = false;
            paused = false, exit = false;
            thread = std::thread([this]()
            {
                renderer::scene scene;
                sound::world sound_world;
                game::world world(scene, sound_world, scene.hud);
                game::mission game_mode_ms(world);
                game::free_flight game_mode_ff(world);
                game::deathmatch game_mode_dm(world);
                game::team_deathmatch game_mode_tdm(world);
                game::hangar hangar(scene);
                game::game_mode *active_game_mode = 0;
                game::network_client client;
                game::network_server server;

                gui::menu menu(sound_world);
/*
                platform::key_callback kcb = std::bind(&gui::menu::on_input, &menu, std::placeholders::_1);
                platform.set_keyboard_callback(kcb);
 */
                int mx = mouse_x, my = mouse_y;

                int screen_width = platform_width, screen_height = platform_height;
                scene.resize(screen_width, screen_height);
                scene.loading(true);
                nya_render::clear(true, true);
                scene.draw();
                end_frame();

                sound_world.set_volume(config::get_var_int("master_volume") / 10.0f);
                sound_world.set_music_volume(config::get_var_int("music_volume") / 10.0f);

                menu.init();
                sound_world.set_music("BGM_menu");

                bool viewer_mode = false;
                bool is_client = false, is_server = false;

                gui::menu::on_action on_menu_action = [&](const std::string &event)
                {
                    if (event == "start")
                    {
                        exit = false;
                        bool first = true;
                        auto loading_callback=[this,&scene,&first]()
                        {
                            static bool init = true;
                            static gui::tiles loading_shape;
                            if (init)
                            {
                                loading_shape.load("UI/comp_loading.lar");
                                init = false;
                            }
                            const int anim_time = (int)nya_system::get_time();

                            if(!first && render.get_buffer_size()<16*1024*1024)
                                return;

                            first = false;

                            nya_render::clear(true, true);

                            const auto color = nya_math::vec4::one();
                            scene.ui_fonts.draw_text(scene.ui_render, L"LOADING", "ZurichBD20outline", scene.ui_render.get_width()/2 - 30, scene.ui_render.get_height()/2 - 10, color);
                            gui::rect r;
                            r. w = r.h = 32;
                            r.x = r.y = -r.w/2;
                            gui::render::transform t;
                            t.x=(scene.ui_render.get_width())/2 + 40;
                            t.y=(scene.ui_render.get_height())/2;
                            t.yaw = -(anim_time % 1000) * 0.002f * nya_math::constants::pi;
                            t.sx = 0.5f,t.sy = -0.5f;
                            loading_shape.draw_tx(scene.ui_render, 0, 1, r, color, t);

                            end_frame();
                        };

                        loading_callback();
                        shared::set_loading_callback(loading_callback);

                        controls = game::plane_controls(); //ToDo
                        sound_world.stop_music();
                        const char *music_names[] = {"BGM_ms10_08", "BGM_ms06", "BGM_ms08x", "BGM_ms11a", "BGM_ms11b", "BGM_ms12_02"};
                        sound_world.set_music(music_names[rand() % (sizeof(music_names) / sizeof(music_names[0]))]);

                        auto location = menu.get_var("map");
                        auto plane = menu.get_var("aircraft");
                        const int color = atoi(menu.get_var("color").c_str());

                        is_client = false, is_server = false;

                        auto mode = menu.get_var("mode");
                        auto mp_var = menu.get_var("multiplayer");
                        if (mp_var == "server")
                        {
                            world.set_network(&server);
                            auto port = menu.get_var_int("port");
                            server.open(port, config::get_var("name").c_str(), mode.c_str(), menu.get_var("map").c_str(), menu.get_var_int("max_players"));
                            if (menu.get_var("mp_public") == "true")
                                game::servers_list::register_server(port);
                            is_server = true;
                        }
                        else if (mp_var == "client")
                        {
                            world.set_network(&client);
                            client.start();
                            is_client = true;
                        }

                        if (mode == "ms")
                        {
                            active_game_mode = &game_mode_ms;
                            game_mode_ms.start(plane.c_str(), color, menu.get_var("mission").c_str());
                        }
                        else if (mode == "dm")
                        {
                            const int bots_count = (is_client || is_server) ? 0 : 11;

                            active_game_mode = &game_mode_dm;
                            game_mode_dm.start(plane.c_str(), color, 0, location.c_str(), bots_count);
                        }
                        else if (mode == "tdm")
                        {
                            const int bots_count = (is_client || is_server) ? 0 : 7;

                            active_game_mode = &game_mode_tdm;
                            game_mode_tdm.start(plane.c_str(), color, 0, location.c_str(), bots_count);
                        }
                        else if (mode == "ff")
                        {
                            active_game_mode = &game_mode_ff;
                            game_mode_ff.start(plane.c_str(), color, location.c_str());
                        }

                        shared::update_loading();
                        shared::set_loading_callback(0);
                    }
                    else if (event == "connect")
                    {
                        menu.set_error("");
                        client.disconnect();
                        auto port = menu.get_var_int("port");
                        if (client.connect(menu.get_var("address").c_str(), port))
                        {
                            menu.send_event("map=" + client.get_server_info().location);
                            menu.send_event("mode=" + client.get_server_info().game_mode);
                            menu.send_event("screen=ac_select");
                        }
                        else
                            menu.set_error(client.get_error());
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
                        hangar.set_plane(menu.get_var("aircraft").c_str());
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
                    else if (event == "update_volume")
                    {
                        sound_world.set_volume(config::get_var_int("master_volume") / 10.0f);
                        sound_world.set_music_volume(config::get_var_int("music_volume") / 10.0f);
                    }/*
                    else if (event == "update_joy_config") //ToDo
                    {
                        if (!joysticks.empty())
                            joysticks.front().update_config();
                    }*/
                    else if (event == "fullscreen_toggle")
                    {
                        platform_fullscreen = true;
                    }
                    else if (event == "exit")
                    {
                        server.close();
                        client.disconnect();
                        platform_terminate=true;
                    }
                    else
                        nya_log::log("unknown event: %s\n", event.c_str());
                };

                menu.set_callback(on_menu_action);

                bool reset_camera = false;

                unsigned long app_time = nya_system::get_time();
                while (!platform_terminate)
                {
                    unsigned long time = nya_system::get_time();
                    int dt = int(time - app_time);
                    if (dt > 1000 && !is_client && !is_server)
                        dt = 1000;

                    app_time = time;

                    static bool speed10x = false;

                    if (platform_width != screen_width || platform_height != screen_height)
                    {
                        screen_width = platform_width, screen_height = platform_height;
                        scene.resize(screen_width, screen_height);

                        if (!config::get_var_bool("fullscreen"))
                        {
                            config::set_var("screen_width", std::to_string(screen_width));
                            config::set_var("screen_height", std::to_string(screen_height));
                        }
                    }

                    if (active_game_mode)
                    {
                        if(is_server || is_client)
                            paused = false;

                        scene.pause(paused);

                        if (!paused)
                        {
                            active_game_mode->update(speed10x ? dt * 10 : dt, controls);

                            //ToDo: move to game code
                            //camera - tracking enemy
                            auto p = world.get_player();
                            if (p && p->change_target_hold_time >= game::plane::change_target_hold_max_time && !p->targets.empty() && !p->targets.front().target.expired())
                            {
                                static auto last_target = p->targets.front().target;
                                if (last_target.expired() || last_target.lock() != p->targets.front().target.lock())
                                {
                                    last_target = p->targets.front().target;
                                    p->change_target_hold_time = game::plane::change_target_hold_max_time;
                                }

                                auto t = p->targets.front().target.lock();
                                auto tdir = t->get_pos() - p->get_pos(); // + (t->get_vel() - p->get_vel()) * (dt * 0.001f)
                                nya_math::quat q(nya_math::vec3::forward(), tdir);
                                q = nya_math::quat::invert(p->get_rot()) * q;

                                auto da = q.get_euler();
                                scene.camera.reset_delta_rot();

                                const float k = nya_math::min((p->change_target_hold_time - game::plane::change_target_hold_max_time) / 500.0f, 1.0);

                                scene.camera.add_delta_rot(da.x * k, -da.y * k);
                                reset_camera = true;
                            }
                        }

                        scene.draw();
/*
                        //util debug draw
                        nya_render::clear(false, true);
                        nya_render::set_state(nya_render::state());
                        nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
                        get_debug_draw().set_line_width(2.0f);
                        get_debug_draw().set_point_size(3.0f);
                        get_debug_draw().draw();
*/
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
                        menu.update(dt, menu_controls);
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
                        scene.ui_render.draw(ui_ref_rects, ui_ref_texture, nya_math::vec4(1.0f, 1.0f, 1.0f, fabsf(alpha_anim / 2000.0f - 1.0f)));
                    }

                    end_frame();

                    //controls

                    if (mouse_l)
                        scene.camera.add_delta_rot((mouse_y - my) * 0.03, (mouse_x - mx) * 0.03);
                    if (mouse_r && viewer_mode)
                        scene.camera.add_delta_pos(0, 0, my - mouse_y);
                    mx = mouse_x, my = mouse_y;

                    if (active_game_mode)
                    {
                        bool should_stop = exit;
                        if (is_client && !client.is_up())
                            should_stop = true;
                        if (is_server && !server.is_up())
                            should_stop = true;

                        if (should_stop)
                        {
                            if (paused)
                                scene.pause(paused = !paused);
                            active_game_mode->end();
                            active_game_mode = 0;
                            server.close();
                            client.disconnect();
                            world.set_network(0);
                            sound_world.stop_sounds();
                            menu_controls.prev = false;
                            if (is_client)
                                menu.send_event("screen=mp_connect");

                            sound_world.set_music("BGM_menu");
                        }
                    }

                    if (controls.change_camera || (reset_camera && !controls.change_target))
                    {
                        scene.camera.reset_delta_rot();
                        reset_camera = false;
                    }
/*
                    if (!mouse_l && !controls.change_target && !viewer_mode)
                    {
                        scene.camera.reset_delta_rot();
                        scene.camera.add_delta_rot(-controls.cam_rot.x * nya_math::constants::pi_2, -controls.cam_rot.y * nya_math::constants::pi);
                    }
*/
                    int sleep_time = 1000/target_fps - int(nya_system::get_time() - time)-1;
                    if (sleep_time > 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
                }

                server.close();
                client.disconnect();
                sound::release_context();
            });
        }

        std::atomic<bool> platform_terminate, platform_fullscreen;
        std::atomic<int> platform_width, platform_height;
        std::atomic<bool> mouse_l, mouse_r;
        std::atomic<int> mouse_x, mouse_y;
        std::atomic<bool> paused, exit;

        game::plane_controls controls_update;
        gui::menu_controls menu_controls_update;

        std::atomic<bool> frame_pending;

    private:
        game::plane_controls controls;
        gui::menu_controls menu_controls;

        void end_frame()
        {
            while (!platform_terminate)
            {
                if(frame_pending)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000/target_fps/2));
                else
                {
                    render.commit();
                    controls = controls_update;
                    menu_controls = menu_controls_update;
                    frame_pending = true;
                    break;
                }
            }
        }

    } game_thread(render, platform.get_width(), platform.get_height());

    auto controls = game::plane_controls();
    auto menu_controls = gui::menu_controls();
    while (!game_thread.platform_terminate)
    {
        if(!game_thread.frame_pending)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000/target_fps/2));
            continue;
        }

        render.push();

        game_thread.controls_update = controls;
        game_thread.menu_controls_update = menu_controls;
        game_thread.frame_pending = false;
        render.execute();
        platform.end_frame();

        if(platform.should_terminate())
            game_thread.platform_terminate = true;
        game_thread.platform_width = platform.get_width();
        game_thread.platform_height = platform.get_height();
        if (game_thread.platform_fullscreen)
        {
            platform.set_fullscreen(config::get_var_bool("fullscreen"), config::get_var_int("screen_width"), config::get_var_int("screen_height"));
            game_thread.platform_fullscreen = false;
        }

        controls = game::plane_controls();
        menu_controls = gui::menu_controls();

        bool joy_pause = false;
        for (int i = 0; i < (int)joysticks.size(); ++i)
        {
            int axes_count = 0, buttons_count = 0;
            const float *axes = glfwGetJoystickAxes(i, &axes_count);
            const unsigned char *buttons = glfwGetJoystickButtons(i, &buttons_count);
            joysticks[i].update(axes, axes_count, buttons, buttons_count);
            joysticks[i].apply_controls(controls, joy_pause);

            //ToDo:
            joysticks[i].apply_controls(menu_controls);
            /*
            if (!active_game_mode)
            {
                //if (i == 0 && !menu.joy_update(axes, axes_count, buttons, buttons_count))
                    joysticks[i].apply_controls(menu_controls);
            }
            */
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

        if (platform.was_pressed(GLFW_KEY_ESCAPE)) menu_controls.prev = true, game_thread.exit = true;
        if (platform.was_pressed(GLFW_KEY_ENTER)) menu_controls.next = true;

        if (platform.was_pressed(GLFW_KEY_P)) game_thread.paused = !game_thread.paused;

        game_thread.mouse_l = platform.get_mouse_lbtn();
        game_thread.mouse_r = platform.get_mouse_rbtn();
        game_thread.mouse_x = platform.get_mouse_x();
        game_thread.mouse_y = platform.get_mouse_y();

        //speed10x = platform.get_key(GLFW_KEY_RIGHT_SHIFT) && !is_client && !is_server;

        if (platform.was_pressed(GLFW_KEY_COMMA))
            debug_variable::set(debug_variable::get() - 1);
        if (platform.was_pressed(GLFW_KEY_PERIOD))
            debug_variable::set(debug_variable::get() + 1);
    }

    nya_render::set_render_api(new nya_render::render_api_interface()); //ToDo
    game_thread.thread.join();
    platform.terminate();
    return 0;
}

//------------------------------------------------------------
