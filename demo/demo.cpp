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
#include "util/util.h"
#include "util/xml.h"

#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"
#include "render/render.h"
#include "scene/camera.h"
#include "system/system.h"

#include "util/config.h"

#include <thread>
#include <chrono>

#include <math.h>
#include <vector>
#include <stdio.h>
#ifndef _WIN32
    #include <unistd.h>
#endif

//------------------------------------------------------------

class platform
{
public:
    bool init(int width, int height, const char *title)
    {
        if (!glfwInit())
            return false;

        if (nya_render::get_render_api() == nya_render::render_api_opengl3)
        {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

		glfwSwapInterval(1);
/*
#ifdef _WIN32
        typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
        wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
        if(wglSwapIntervalEXT)
            wglSwapIntervalEXT(1);
#endif
*/
        m_window = glfwCreateWindow(width, height, title, NULL, NULL);
        if (!m_window)
        {
            glfwTerminate();
            return false;
        }

        glfwSetKeyCallback(m_window, key_func);
        glfwMakeContextCurrent(m_window);
        glfwGetFramebufferSize(m_window, &m_screen_w, &m_screen_h);

        return true;
    }

    void terminate()
    {
        if (!m_window)
            return;

        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = 0;
    }

    bool should_terminate()
    {
        return m_window ? glfwWindowShouldClose(m_window) > 0 : true;
    }

    void end_frame()
    {
        if (!m_window)
            return;

        glfwSwapBuffers(m_window);
        m_last_buttons = m_buttons;
        glfwPollEvents();

        double mx,my;
        glfwGetCursorPos(m_window, &mx, &my);
        m_mouse_x = int(mx), m_mouse_y = int(my);
        glfwGetFramebufferSize(m_window, &m_screen_w, &m_screen_h);
    }

    bool was_pressed(int key)
    {
        auto k = m_buttons.find(key);
        if (k == m_buttons.end() || !k->second)
            return false;

        auto l = m_last_buttons.find(key);
        return l == m_last_buttons.end() || !l->second;
    }

    bool get_key(int key) { auto k = m_buttons.find(key); return k == m_buttons.end() ? false : k->second; }
    bool get_mouse_lbtn() { return m_window ? glfwGetMouseButton(m_window, 0) > 0 : false; }
    bool get_mouse_rbtn() { return m_window ? glfwGetMouseButton(m_window, 1) > 0 : false; }
    int get_mouse_x() { return m_mouse_x; }
    int get_mouse_y() { return m_mouse_y; }
    int get_width() { return m_screen_w; }
    int get_height() { return m_screen_h; }

    typedef std::function<void(char c)> key_callback;
    void set_keyboard_callback(key_callback &k) { m_key_callback = k; }

    platform(): m_window(0) {}

private:
    static void key_func(GLFWwindow *, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            m_buttons[key] = true;
            if (m_key_callback)
            {
                if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
                    m_key_callback('0' + key - GLFW_KEY_0);
                else if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z)
                    m_key_callback('A' + key - GLFW_KEY_A);
                else if (key == GLFW_KEY_PERIOD)
                    m_key_callback('.');
                else if (key == GLFW_KEY_SEMICOLON)
                    m_key_callback(':');
                else if (key == GLFW_KEY_MINUS)
                    m_key_callback('-');
                else if (key == GLFW_KEY_SLASH)
                    m_key_callback('/');
                else if (key == GLFW_KEY_SPACE)
                    m_key_callback(' ');
                else if (key == GLFW_KEY_BACKSPACE)
                    m_key_callback(8);
            }
        }
        else if (action == GLFW_RELEASE)
            m_buttons[key] = false;
    }

private:
    GLFWwindow *m_window;
    int m_mouse_x, m_mouse_y;
    int m_screen_w, m_screen_h;
    static std::map<int, bool> m_buttons;
    std::map<int, bool> m_last_buttons;
    static key_callback m_key_callback;
};

//------------------------------------------------------------

std::map<int, bool> platform::m_buttons;
platform::key_callback platform::m_key_callback;

//------------------------------------------------------------

class joystick_config
{
public:
    void init(const char *name)
    {
        pugi::xml_document doc;
        if (!name || !load_xml("joystick.xml", doc))
            return;

        std::string name_str(name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);

#ifdef _WIN32
        const std::string os("windows");
#elif defined __APPLE__
        const std::string os("osx");
#else
        const std::string os("linux");
#endif

        pugi::xml_node root = doc.first_child();
        for (pugi::xml_node j = root.child("joystick"); j; j = j.next_sibling("joystick"))
        {
            std::string name_part = j.attribute("name").as_string("");
            std::transform(name_part.begin(), name_part.end(), name_part.begin(), ::tolower);

            if (name_str.find(name_part) == std::string::npos)
                continue;

            for (pugi::xml_node c = j.first_child(); c; c = c.next_sibling())
            {
                auto aos = c.attribute("os");
                if (aos && os != aos.as_string())
                    continue;

                std::string name(c.name());
                std::string type; int idx = 0;
                for (auto &n: name)
                {
                    if (!isalpha(n))
                    {
                        idx = atoi(&n);
                        break;
                    }

                    type.push_back(n);
                }

                auto cmd = convert_cmd(c.attribute("control").as_string(""));

                if (type == "axis")
                    m_axes.push_back({idx, c.attribute("dead").as_float(), c.attribute("invert").as_bool(), cmd});
                else if (type == "btn")
                    m_buttons.push_back({idx,cmd});
            }
        }
    }

    void update(const float *axes, int axes_count, const unsigned char *btns, int btns_count)
    {
        m_joy_axes.resize(axes_count);
        for (int i = 0; i < axes_count; ++i)
            m_joy_axes[i] = axes[i];

        m_joy_btns.resize(btns_count);
        for (int i = 0; i < btns_count; ++i)
            m_joy_btns[i] = btns[i] > 0;
    }

    void apply_controls(game::plane_controls &controls, bool &pause)
    {
        for (int i = 0; i < (int)m_joy_axes.size(); ++i)
        {
            for (auto &a: m_axes)
            {
                if (a.idx != i)
                    continue;

                float v = m_joy_axes[i];
                if (fabsf(v) < a.deadzone)
                    continue;

                if (a.inverted)
                    v = -v;

                switch (a.cmd)
                {
                    case 'pyaw': controls.rot.y = v; break;
                    case 'ppch': controls.rot.x = v; break;
                    case 'prll': controls.rot.z = v; break;
                    case 'thrt': controls.throttle = v; break;
                    default: break;
                }
            }
        }

        for (int i = 0; i < (int)m_joy_btns.size(); ++i)
        {
            if (!m_joy_btns[i])
                continue;

            for (auto &b: m_buttons)
            {
                if (b.idx != i)
                    continue;

                switch (b.cmd)
                {
                    case '+pyw': controls.rot.y = 1.0; break;
                    case '-pyw': controls.rot.y = -1.0; break;
                    case 'cchg': controls.change_camera = true; break;
                    case 'wchg': controls.change_weapon = true; break;
                    case 'tchn': controls.change_target = true; break;
                    case 'rchn': controls.change_radar = true; break;
                    case 'msle': controls.missile = true; break;
                    case 'flrs': controls.flares = true; break;
                    case 'mgun': controls.mgun = true; break;
                    case 'brke': controls.brake = true; break;
                    case 'thrt': controls.throttle = true; break;
                    case 'paus': pause = true; break;
                    default: break;
                }
            }
        }

        //if (fabsf(axis[2]) > joy_dead_zone) scene.camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
        //if (fabsf(axis[3]) > joy_dead_zone) scene.camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
    }

    void apply_controls(gui::menu_controls &controls)
    {
        for (int i = 0; i < (int)m_joy_axes.size(); ++i)
        {
            for (auto &a: m_axes)
            {
                if (a.idx != i)
                    continue;

                if (fabsf(m_joy_axes[i]) < a.deadzone)
                    continue;

                float v = m_joy_axes[i];
                if (a.inverted)
                    v = -v;

                if (a.cmd == 'm_ud')
                {
                    if (v > 0.0f)
                        controls.up = true;
                    else
                        controls.down = true;
                }
                else if (a.cmd == 'm_lr')
                {
                    if (v > 0.0f)
                        controls.right = true;
                    else
                        controls.left = true;
                }
            }
        }

        for (int i = 0; i < (int)m_joy_btns.size(); ++i)
        {
            if (!m_joy_btns[i])
                continue;

            //printf("btn %2d\n",i);

            for (auto &b: m_buttons)
            {
                if (b.idx != i)
                    continue;

                switch (b.cmd)
                {
                    case 'm_up': controls.up = true; break;
                    case 'm_dn': controls.down = true; break;
                    case 'm_lf': controls.left = true; break;
                    case 'm_rt': controls.right = true; break;
                    case 'm_pr': controls.prev = true; break;
                    case 'm_nx': controls.next = true; break;
                    default: break;
                }
            }
        }
    }

private:
    static unsigned int convert_cmd(const std::string &cmd)
    {
        if (cmd == "yaw") return 'pyaw';
        if (cmd == "+yaw") return '+pyw';
        if (cmd == "-yaw") return '-pyw';
        if (cmd == "pitch") return 'ppch';
        if (cmd == "roll") return 'prll';
        if (cmd == "camera_yaw") return 'cyaw';
        if (cmd == "camera_pitch") return 'cpch';
        if (cmd == "change_camera") return 'cchg';
        if (cmd == "change_weapon") return 'wchg';
        if (cmd == "change_radar") return 'rchg';
        if (cmd == "pause") return 'paus';
        if (cmd == "brake") return 'brke';
        if (cmd == "throttle") return 'thrt';
        if (cmd == "change_target") return 'tchn';
        if (cmd == "missile") return 'msle';
        if (cmd == "flares") return 'flrs';
        if (cmd == "mgun") return 'mgun';

        if (cmd == "menu_up_down") return 'm_ud';
        if (cmd == "menu_up") return 'm_up';
        if (cmd == "menu_down") return 'm_dn';
        if (cmd == "menu_left") return 'm_lf';
        if (cmd == "menu_right") return 'm_rt';
        if (cmd == "menu_prev") return 'm_pr';
        if (cmd == "menu_next") return 'm_nx';

        return 0;
    }

private:
    struct axis
    {
        int idx;
        float deadzone;
        bool inverted;
        unsigned int cmd;
    };

    std::vector<axis> m_axes;

    struct btn
    {
        int idx;
        unsigned int cmd;
    };

    std::vector<btn> m_buttons;

private:
    std::vector<float> m_joy_axes;
    std::vector<bool> m_joy_btns;
};

//------------------------------------------------------------

int main(void)
{
#ifndef _WIN32
    chdir(nya_system::get_app_path());
#endif

    qdf_resources_provider qdfp;
    if (!qdfp.open_archive("datafile.qdf"))
    {
        printf("unable to open datafile.qdf");
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

    gui::menu::on_action on_menu_action = [&](const std::string &event)
    {
        if (event == "start")
        {
            auto location = menu.get_var("map");
            auto plane = menu.get_var("ac");
            const int color = atoi(menu.get_var("color").c_str());

            scene.loading(true);
            nya_render::clear(true, true);
            scene.draw();
            platform.end_frame();
            scene.loading(false);

            auto mode = menu.get_var("mode");
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
            platform.terminate();
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

            if (platform.was_pressed(GLFW_KEY_ESCAPE))
            {
                active_game_mode->end();
                active_game_mode = 0;
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

    platform.terminate();

    return 0;
}

//------------------------------------------------------------
