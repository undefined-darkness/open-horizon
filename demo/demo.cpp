//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "containers/qdf_provider.h"
#include "containers/dpl_provider.h"

#include "game/deathmatch.h"
#include "game/team_deathmatch.h"
#include "game/free_flight.h"
#include "gui/menu.h"
#include "util/util.h"

#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"
#include "render/render.h"
#include "system/system.h"

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
        return m_window ? glfwWindowShouldClose(m_window) : true;
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
    bool get_mouse_lbtn() { return m_window ? glfwGetMouseButton(m_window, 0) : false; }
    bool get_mouse_rbtn() { return m_window ? glfwGetMouseButton(m_window, 1) : false; }
    int get_mouse_x() { return m_mouse_x; }
    int get_mouse_y() { return m_mouse_y; }
    int get_width() { return m_screen_w; }
    int get_height() { return m_screen_h; }

    platform(): m_window(0) {}

private:
    static void key_func(GLFWwindow *, int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
            m_buttons[key] = true;
        else if (action == GLFW_RELEASE)
            m_buttons[key] = false;
    }

private:
    GLFWwindow *m_window;
    int m_mouse_x, m_mouse_y;
    int m_screen_w, m_screen_h;
    static std::map<int, bool> m_buttons;
    std::map<int, bool> m_last_buttons;
};

//------------------------------------------------------------

std::map<int, bool> platform::m_buttons;

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

    platform platform;
    if (!platform.init(1000, 562, "Open Horizon 3rd demo"))
        return -1;

    for (int i = 0; glfwJoystickPresent(i); ++i)
    {
        const char *name = glfwGetJoystickName(i);

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
    game::game_mode *active_game_mode = 0;
    game::plane_controls controls;

    gui::menu menu;
    gui::menu_controls menu_controls;

    int mx = platform.get_mouse_x(), my = platform.get_mouse_y();
    int screen_width = platform.get_width(), screen_height = platform.get_height();
    scene.resize(screen_width, screen_height);
    scene.loading(true);
    nya_render::clear(true, true);
    scene.draw();
    platform.end_frame();

    menu.init();

    gui::menu::on_action on_menu_action = [&menu, &scene, &platform,
                                           &active_game_mode,
                                           &game_mode_dm,
                                           &game_mode_tdm,
                                           &game_mode_ff](const std::string &event) mutable
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

        static bool speed10x = false, paused = false;

        if (platform.get_width() != screen_width || platform.get_height() != screen_height)
        {
            screen_width = platform.get_width(), screen_height = platform.get_height();
            scene.resize(screen_width, screen_height);
        }

        if (!active_game_mode)
            menu.update(dt, menu_controls);

        if (active_game_mode)
        {
            active_game_mode->update(paused ? 0 : (speed10x ? dt * 10 : dt), controls);
            scene.draw();
        }
        else
            menu.draw(scene.ui_render);

        const char *ui_ref = 0;
        //ui_ref = "ui_ref2.tga";
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

        int axis_count = 0, buttons_count = 0;
        const float *axis = glfwGetJoystickAxes(0, &axis_count);
        const unsigned char *buttons = glfwGetJoystickButtons(0, &buttons_count);
        const float joy_dead_zone = 0.1f;
        if (axis_count > 1)
        {
            if (fabsf(axis[0]) > joy_dead_zone) controls.rot.z = axis[0];
            if (fabsf(axis[1]) > joy_dead_zone) controls.rot.x = axis[1];
        }

        if (axis_count > 3)
        {
            if (fabsf(axis[2]) > joy_dead_zone) scene.camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
            if (fabsf(axis[3]) > joy_dead_zone) scene.camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
        }

        if (buttons_count > 11)
        {
            if (buttons[8]) controls.rot.y = -1.0f;
            if (buttons[9]) controls.rot.y = 1.0f;
            if (buttons[10]) controls.brake = 1.0f;
            if (buttons[11]) controls.throttle = 1.0f;

            if (buttons[2]) scene.camera.reset_delta_rot();
/*
            static bool last_btn3 = false;
            if (buttons[3] !=0 && !last_btn3)
                scene.pause();

            last_btn3 = buttons[3] != 0;
*/
        }

        if (buttons_count > 14)
        {
            if(buttons[13]) controls.missile = true;
            if(buttons[14]) controls.mgun = true;
            if(buttons[0]) controls.change_weapon = true;
        }

        if (platform.get_key(GLFW_KEY_W)) controls.throttle = 1.0f;
        if (platform.get_key(GLFW_KEY_S)) controls.brake = 1.0f;
        if (platform.get_key(GLFW_KEY_A)) controls.rot.y = -1.0f;
        if (platform.get_key(GLFW_KEY_D)) controls.rot.y = 1.0f;
        if (platform.get_key(GLFW_KEY_UP)) controls.rot.x = 1.0f, menu_controls.up = true;
        if (platform.get_key(GLFW_KEY_DOWN)) controls.rot.x = -1.0f, menu_controls.down = true;
        if (platform.get_key(GLFW_KEY_LEFT)) controls.rot.z = -1.0f;
        if (platform.get_key(GLFW_KEY_RIGHT)) controls.rot.z = 1.0f;

        if (platform.get_key(GLFW_KEY_LEFT_CONTROL)) controls.mgun = true;
        if (platform.get_key(GLFW_KEY_SPACE)) controls.missile = true, menu_controls.next = true;
        if (platform.get_key(GLFW_KEY_Q)) controls.change_weapon = true;
        if (platform.get_key(GLFW_KEY_E)) controls.change_target = true;
        if (platform.get_key(GLFW_KEY_V)) controls.change_camera = true;

        if (platform.was_pressed(GLFW_KEY_ESCAPE)) menu_controls.prev = true;
        if (platform.was_pressed(GLFW_KEY_ENTER)) menu_controls.next = true;

        if (platform.was_pressed(GLFW_KEY_P) || platform.was_pressed(GLFW_KEY_ESCAPE))
            scene.pause(paused = !paused);

        speed10x = platform.get_key(GLFW_KEY_RIGHT_SHIFT);

        if (controls.change_camera)
            scene.camera.reset_delta_rot();

        if (platform.was_pressed(GLFW_KEY_COMMA))
            debug_variable::set(debug_variable::get() - 1);
        if (platform.was_pressed(GLFW_KEY_PERIOD))
            debug_variable::set(debug_variable::get() + 1);
    }

    platform.terminate();

    return 0;
}

//------------------------------------------------------------
