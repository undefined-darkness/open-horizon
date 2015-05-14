//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "containers/qdf_provider.h"
#include "containers/dpl_provider.h"

#include "game/deathmatch.h"
#include "game/team_deathmatch.h"
#include "game/free_flight.h"
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
        glfwTerminate();
    }

    bool should_terminate()
    {
        return glfwWindowShouldClose(m_window);
    }

    void end_frame()
    {
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
    bool get_mouse_lbtn() { return glfwGetMouseButton(m_window, 0); }
    bool get_mouse_rbtn() { return glfwGetMouseButton(m_window, 1); }
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
    game::game_mode *active_game_mode = &game_mode_ff;
    game::plane_controls controls;

    int mx = platform.get_mouse_x(), my = platform.get_mouse_y();
    int screen_width = platform.get_width(), screen_height = platform.get_height();
    scene.resize(screen_width, screen_height);
    scene.loading(true);
    nya_render::clear(true, true);
    scene.draw();
    platform.end_frame();

    const char *locations[] = {
        "ms06", //dubai
        "ms01", //miami
        "ms30", //paris
        "ms50", //tokyo
        //"ms11b", //moscow
        //"ms14" //washington
    };

    const char *planes[] = {
        "su33",
        "f22a",
        "su24",
        "f18f",
        "m29a",
        "f02a",
        "su25",
        "b01b",
        "f16c",
        "su37",
        "kwmr",
        "su34",
        "pkfa", //bay anim offset
        "av8b",
        "f35b",
        "f15m",
        "f15c",
        "f15e",
        "j39c",
        "mr2k",
        "rflm",
        "su47",
        "tnd4",
        "typn",
        "f04e", //some anims fail
        "su35",
        "b02a", //bay anim offset
        "f14d",
        "m21b", //no cockpit
        "f16f",
        "a10a",
        "f17a",
        //no anims
        //"yf23",
        //"fa44",
        //helicopters are not yet supported
        //"ah64",
        //"mi24",
        //"ka50",
        //"mh60",
        //not yet supported
        //"a130", //b.unknown2 = 1312
    };

    unsigned int current_location = 0;
    unsigned int current_plane = 0;
    unsigned int current_color = 0;

    if (active_game_mode == &game_mode_ff)
        game_mode_ff.start(planes[current_plane], current_color, locations[current_location]);

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

        active_game_mode->update(paused ? 0 : (speed10x ? dt * 10 : dt), controls);
        scene.draw();
        platform.end_frame();

        //controls

        controls = game::plane_controls();

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
        if (platform.get_key(GLFW_KEY_UP)) controls.rot.x = 1.0f;
        if (platform.get_key(GLFW_KEY_DOWN)) controls.rot.x = -1.0f;
        if (platform.get_key(GLFW_KEY_LEFT)) controls.rot.z = -1.0f;
        if (platform.get_key(GLFW_KEY_RIGHT)) controls.rot.z = 1.0f;

        if (platform.get_key(GLFW_KEY_LEFT_CONTROL)) controls.mgun = true;
        if (platform.get_key(GLFW_KEY_SPACE)) controls.missile = true;
        if (platform.get_key(GLFW_KEY_Q)) controls.change_weapon = true;
        if (platform.get_key(GLFW_KEY_V)) controls.change_camera = true;

        if (platform.was_pressed(GLFW_KEY_P) || platform.was_pressed(GLFW_KEY_ESCAPE))
            scene.pause(paused = !paused);

        speed10x = platform.get_key(GLFW_KEY_RIGHT_SHIFT);

        if (controls.change_camera)
            scene.camera.reset_delta_rot();

        if (platform.was_pressed(GLFW_KEY_COMMA))
            debug_variable::set(debug_variable::get() - 1);
        if (platform.was_pressed(GLFW_KEY_PERIOD))
            debug_variable::set(debug_variable::get() + 1);

        //demo purpose

        if (active_game_mode != &game_mode_ff)
            continue;

        const unsigned int locations_count = sizeof(locations) / sizeof(locations[0]);

        if (platform.was_pressed(GLFW_KEY_1))
        {
            scene.loading(true);
            scene.draw();
            platform.end_frame();

            game_mode_ff.set_location(locations[current_location = (current_location + 1) % locations_count]);
            scene.loading(false);
        }

        if (platform.was_pressed(GLFW_KEY_2))
        {
            scene.loading(true);
            scene.draw();
            platform.end_frame();

            game_mode_ff.set_location(locations[current_location = (current_location + locations_count - 1) % locations_count]);
            scene.loading(false);
        }

        const unsigned int planes_count = sizeof(planes) / sizeof(planes[0]);

        if (platform.was_pressed(GLFW_KEY_3))
        {
            scene.loading(true);
            scene.draw();
            platform.end_frame();

            current_color = 0;
            current_plane = (current_plane + 1) % planes_count;
            game_mode_ff.set_plane(planes[current_plane], current_color);
            scene.loading(false);
        }

        if (platform.was_pressed(GLFW_KEY_4))
        {
            scene.loading(true);
            scene.draw();
            platform.end_frame();

            current_color = 0;
            current_plane = (current_plane + planes_count - 1) % planes_count;
            game_mode_ff.set_plane(planes[current_plane], current_color);
            scene.loading(false);
        }

        if (platform.was_pressed(GLFW_KEY_6))
        {
            auto colors_count = renderer::aircraft::get_colors_count(planes[current_plane]);
            current_color = (current_color + 1) % colors_count;
            game_mode_ff.set_plane(planes[current_plane], current_color);
        }

        if (platform.was_pressed(GLFW_KEY_5))
        {
            auto colors_count = renderer::aircraft::get_colors_count(planes[current_plane]);
            current_color = (current_color + colors_count - 1) % colors_count;
            game_mode_ff.set_plane(planes[current_plane], current_color);
        }
    }

    platform.terminate();

    return 0;
}

//------------------------------------------------------------
