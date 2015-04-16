//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "qdf_provider.h"
#include "dpl_provider.h"

#include "aircraft.h"
#include "location.h"
#include "clouds.h"
#include "shared.h"
#include "debug.h"

#include "render/render.h"
#include "math/scalar.h"
#include "system/system.h"
#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"
#include "scene/camera.h"
#include "scene/postprocess.h"

#include <math.h>
#include <vector>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

//------------------------------------------------------------

class plane_camera
{
public:
    void add_delta_rot(float dx, float dy);
    void reset_delta_rot() { m_drot.x = 0.0f; m_drot.y = 3.14f; }
    void add_delta_pos(float dx, float dy, float dz);
    void reset_delta_pos() { m_dpos = nya_math::vec3(); }//0.0f, 2.0f, -10.0f); }

    void set_aspect(float aspect);
    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; update(); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; update(); }

private:
    void update();

public:
    plane_camera() { m_drot.y = 3.14f; reset_delta_pos(); }

private:
    nya_math::vec3 m_drot;
    nya_math::vec3 m_dpos;

    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
};

//------------------------------------------------------------

void plane_camera::add_delta_rot(float dx, float dy)
{
    m_drot.x += dx;
    m_drot.y += dy;
    
    const float max_angle = 3.14f * 2.0f;
    
    if (m_drot.x > max_angle)
        m_drot.x -= max_angle;
    
    if (m_drot.x < -max_angle)
        m_drot.x += max_angle;
    
    if (m_drot.y > max_angle)
        m_drot.y -= max_angle;
    
    if (m_drot.y< -max_angle)
        m_drot.y+=max_angle;
    
    update();
}

//------------------------------------------------------------

void plane_camera::add_delta_pos(float dx, float dy, float dz)
{
    m_dpos.x -= dx;
    m_dpos.y -= dy;
    m_dpos.z -= dz;
    if (m_dpos.z < 5.0f)
        m_dpos.z = 5.0f;

    if (m_dpos.z > 20000.0f)
        m_dpos.z = 20000.0f;

    update();
}

//------------------------------------------------------------

void plane_camera::set_aspect(float aspect)
{
    nya_scene::get_camera().set_proj(55.0, aspect, 1.0, 21000.0);
                      //1300.0);
    update();
}

//------------------------------------------------------------

void plane_camera::update()
{
    nya_math::quat r = m_rot;
    r.v.x = -r.v.x, r.v.y = -r.v.y;

    r = r*nya_math::quat(m_drot.x, m_drot.y, 0.0f);
    //nya_math::vec3 r=m_drot+m_rot;

    //cam->set_rot(r.y*180.0f/3.14f,r.x*180.0f/3.14f,0.0);
    nya_scene::get_camera().set_rot(r);

    //nya_math::quat rot(-r.x,-r.y,0.0f);
    //rot=rot*m_rot;

    //nya_math::vec3 pos=m_pos+rot.rotate(m_dpos);
    r.v.x = -r.v.x, r.v.y = -r.v.y;
    nya_math::vec3 pos = m_pos + r.rotate(m_dpos);

    nya_scene::get_camera().set_pos(pos.x, pos.y, pos.z);
}

//------------------------------------------------------------

nya_scene::texture load_tonecurve(const char *file_name)
{
    nya_scene::shared_texture t;
    auto res = shared::load_resource(file_name);
    assert(res.get_size() == 256 * 3);

    const unsigned char *data = (const unsigned char *)res.get_data();
    unsigned char color[256 * 3];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 256; ++j)
            color[j*3+i]=data[j+i*256];
    }

    t.tex.build_texture(color, 256, 1, nya_render::texture::color_rgb);
    t.tex.set_wrap(false, false);
    res.free();

    nya_scene::texture result;
    result.create(t);
    return result;
}

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

    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(1000, 562, "open horizon", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    for (int i = 0; glfwJoystickPresent(i); ++i)
    {
        const char *name = glfwGetJoystickName(i);

        int axis_count = 0, buttons_count = 0;
        glfwGetJoystickAxes(i, &axis_count);
        glfwGetJoystickButtons(i, &buttons_count);
        printf("joy%d: %s %d axis %d buttons\n", i, name, axis_count, buttons_count);
    }

    glfwMakeContextCurrent(window);
    nya_render::texture::set_default_aniso(2);

    class : private nya_scene::postprocess
    {
    public:
        aircraft player_plane;
        plane_camera camera;

    private:
        location loc;
        effect_clouds clouds;

    public:
        void load_location(const char *location_name)
        {
            loc = location();
            clouds = effect_clouds();

            shared::clear_textures();

            unload();
            load("postprocess.txt");

            loc.load(location_name);
            clouds.load(location_name);

            auto &p = loc.get_params();
            auto curve = load_tonecurve((std::string("Map/tonecurve_") + location_name + ".tcb").c_str());
            set_texture("color_curve", nya_scene::texture_proxy(curve));
            set_shader_param("bloom_param", nya_math::vec4(p.hdr.bloom_threshold, p.hdr.bloom_offset, p.hdr.bloom_scale, 1.0));
            set_shader_param("saturation", nya_math::vec4(p.tone_saturation * 0.01, 0.0, 0.0, 0.0));
            set_shader_param("screen_radius", nya_math::vec4(1.185185, 0.5 * 4.0 / 3.0, 0.0, 0.0));
            set_shader_param("damage_frame", nya_math::vec4(0.35, 0.5, 1.0, 0.1));

            player_plane.set_pos(nya_math::vec3(-300, 50, 2000));
            player_plane.set_rot(nya_math::quat());
            m_fade_time = 2500;

            //shared::clear_textures(); //ToDo
        }

        void load_player_plane(const char *name,int color)
        {
            auto pos = player_plane.get_pos();

            player_plane = aircraft();

            player_plane.load(name, color);

            player_plane.set_pos(pos);

            //ToDo: research camera
            auto off = -player_plane.get_camera_offset();
            camera.reset_delta_pos();
            camera.add_delta_pos(off.x, -0.5 + off.y*1.5, off.z*0.55);
            if (strcmp(name, "f22a") == 0 || strcmp(name, "kwmr") == 0 || strcmp(name, "su47") == 0)
                camera.add_delta_pos(0.0, -1.0, 3.0);

            if (strcmp(name, "b01b") == 0)
                camera.add_delta_pos(0.0, -4.0, -2.0);

            if (strcmp(name, "su25") == 0 || strcmp(name, "su34") == 0 || strcmp(name, "f16c") == 0
                || strcmp(name, "f02a") == 0 || strcmp(name, "su37") == 0)
                camera.add_delta_pos(0.0, -1.0, 0.0);

            //shared::clear_textures(); //ToDo
        }

        void resize(unsigned int width,unsigned int height)
        {
            nya_scene::postprocess::resize(width, height);
            if (height)
                camera.set_aspect(height > 0 ? float(width) / height : 1.0f);
        }

        void update(int dt)
        {
            if (dt > 50)
                dt = 50;

            loc.update(dt);
            player_plane.update(dt);

            camera.set_pos(player_plane.get_pos());
            camera.set_rot(player_plane.get_rot());

            set_shader_param("damage_frame_color", nya_math::vec4(1.0, 0.0, 0.0235, nya_math::max(1.0 - player_plane.get_hp(), 0.0)));

            if (m_fade_time > 0)
            {
                m_fade_time-=dt;
                if (m_fade_time < 0)
                    m_fade_time = 0;

                set_shader_param("fade_color", nya_math::vec4(0.0, 0.0, 0.0, m_fade_time / 2500.0f));
            }
        }

        void draw() { nya_scene::postprocess::draw(0); }

    private:
        void draw_scene(const char *pass, const char *tags) override
        {
            if (strcmp(tags, "location") == 0)
                loc.draw();
            else if (strcmp(tags, "player") == 0)
                player_plane.draw();
            else if (strcmp(tags, "clouds_flat") == 0)
                clouds.draw_flat();
            else if (strcmp(tags, "clouds_obj") == 0)
                clouds.draw_obj();
        }
    private:
        int m_fade_time;

    } scene;

    const char *locations[] = {
        "ms01", //miami
        "ms06", //dubai
        "ms50", //tokyo
        "ms11b", //moscow
        "ms30", //paris
        //"ms14" //washington
    };

    unsigned int current_location=0;

    const char *planes[] = {
        "f22a",
        "su24",
        "f18f",
        "m29a",
        "f02a",
        "su25",
        "b01b",
        "f16c",
        "su33",
        "kwmr",
        "su34",
        "pkfa", //bay anim offset
        "av8b",
        "f35b",
        "j39c",
        "mr2k",
        "rflm",
        "su47",
        "tnd4",
        "typn",
        "f04e",
        "su35",
        "b02a", //bay anim offset
        "f14d",
        "m21b", //no cockpit
        "f16f",
        "su37",

        //"yf23", //weird anims

                  //anim assert
        //"a10a",
        //"a130",
        //"fa44",
        //"f17a",
        //"f15c",
        //"f15e",
        //"f15m",

                  //helicopters are not yet supported
        //"ah64",
        //"mi24",
        //"ka50",
        //"mh60",
    };

    unsigned int current_plane=0;
    unsigned int current_color=0;

    scene.load_location(locations[current_location]);
    scene.load_player_plane(planes[current_plane], current_color);

    double mx,my;
    glfwGetCursorPos(window, &mx, &my);

    int frame_counter = 0;
    int frame_counter_time = 0;
    int fps = 0;

    int screen_width = 0, screen_height = 0;
    unsigned long app_time = nya_system::get_time();
    while (!glfwWindowShouldClose(window))
    {
        unsigned long time = nya_system::get_time();
        int dt = int(time - app_time);
        if (dt > 1000)
            dt = 1000;

        frame_counter_time += dt;
        ++frame_counter;
        if (frame_counter_time > 1000)
        {
            fps = frame_counter;
            frame_counter = 0;
            frame_counter_time -= 1000;
        }
        app_time = time;

        static bool paused = false;
        static bool speed10x = false;

        char title[255];
        sprintf(title,"speed: %7d alt: %7d \t fps: %3d  %s", int(scene.player_plane.get_speed()), int(scene.player_plane.get_alt()), fps, paused ? "paused" : "");
        glfwSetWindowTitle(window,title);

        int new_screen_width, new_screen_height;
        glfwGetFramebufferSize(window, &new_screen_width, &new_screen_height);
        if (new_screen_width != screen_width || new_screen_height != screen_height)
        {
            screen_width = new_screen_width, screen_height = new_screen_height;
            scene.resize(screen_width, screen_height);
        }

        scene.update(paused ? 0 : (speed10x ? dt * 10 : dt));
        scene.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();

        //controls

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        if (glfwGetMouseButton(window, 0))
            scene.camera.add_delta_rot((y - my) * 0.03, (x - mx) * 0.03);

        if (glfwGetMouseButton(window, 1))
            scene.camera.add_delta_pos(0, 0, my - y);

        mx = x; my = y;

        nya_math::vec3 c_rot;
        float c_throttle = 0.0f, c_brake = 0.0f;

        int axis_count = 0, buttons_count = 0;
        const float *axis = glfwGetJoystickAxes(0, &axis_count);
        const unsigned char *buttons = glfwGetJoystickButtons(0, &buttons_count);
        const float joy_dead_zone = 0.1f;
        if (axis_count > 1)
        {
            if (fabsf(axis[0]) > joy_dead_zone) c_rot.z = axis[0];
            if (fabsf(axis[1]) > joy_dead_zone) c_rot.x = axis[1];
        }

        if (axis_count > 3)
        {
            if (fabsf(axis[2]) > joy_dead_zone) scene.camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
            if (fabsf(axis[3]) > joy_dead_zone) scene.camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
        }

        if (buttons_count > 11)
        {
            if (buttons[8]) c_rot.y = -1.0f;
            if (buttons[9]) c_rot.y = 1.0f;
            if (buttons[10]) c_brake = 1.0f;
            if (buttons[11]) c_throttle = 1.0f;

            if (buttons[2]) scene.camera.reset_delta_rot();

            static bool last_btn3 = false;
            if (buttons[3] !=0 && !last_btn3)
                paused = !paused;

            last_btn3 = buttons[3] != 0;
        }

        bool key_mgun = false, key_rocket = false, key_spec = false;

        if (buttons_count > 14)
        {
            if(buttons[13]) key_rocket = true;
            if(buttons[14]) key_mgun = true;
            if(buttons[0]) key_spec = true;
        }

        if (glfwGetKey(window, GLFW_KEY_W)) c_throttle = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S)) c_brake = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A)) c_rot.y = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_D)) c_rot.y = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_UP)) c_rot.x = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_DOWN)) c_rot.x = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT)) c_rot.z = -1.0f;
        if (glfwGetKey(window, GLFW_KEY_RIGHT)) c_rot.z = 1.0f;

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) key_mgun = true;
        if (glfwGetKey(window, GLFW_KEY_SPACE)) key_rocket = true;
        if (glfwGetKey(window, GLFW_KEY_Q)) key_spec = true;

        if (key_mgun) scene.player_plane.fire_mgun();

        static bool last_control_rocket = false, last_control_special = false;

        if (key_rocket)
        {
            if (!last_control_rocket)
                scene.player_plane.fire_rocket();
            last_control_rocket = true;
        }
        else
            last_control_rocket = false;

        if (key_spec)
        {
            if (!last_control_special)
                scene.player_plane.change_weapon();
            last_control_special = true;
        }
        else
            last_control_special = false;

        //if (glfwGetKey(window, GLFW_KEY_L)) loc.load(location_name);

        static bool last_btn_p = false;
        if (glfwGetKey(window, GLFW_KEY_P) && !last_btn_p)
            paused = !paused;

        last_btn_p = glfwGetKey(window, GLFW_KEY_P);

        speed10x = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);

        scene.player_plane.set_controls(c_rot, c_throttle, c_brake);

        //demo purpose

        std::vector<bool> fkeys(6, false);
        static std::vector<bool> fkeys_last(6, false);

        for (int i = 0; i < fkeys.size(); ++i)
            fkeys[i] = glfwGetKey(window, GLFW_KEY_1 + i);

        const unsigned int locations_count = sizeof(locations) / sizeof(locations[0]);

        if (fkeys[0] && fkeys[0] != fkeys_last[0])
        {
            current_location = (current_location + 1) % locations_count;
            scene.load_location(locations[current_location]);
        }

        if (fkeys[1] && fkeys[1] != fkeys_last[1])
        {
            current_location = (current_location + locations_count - 1) % locations_count;
            scene.load_location(locations[current_location]);
        }

        const unsigned int planes_count = sizeof(planes) / sizeof(planes[0]);

        if (fkeys[2] && fkeys[2] != fkeys_last[2])
        {
            current_color = 0;
            current_plane = (current_plane + 1) % planes_count;
            scene.load_player_plane(planes[current_plane], current_color);
        }

        if (fkeys[3] && fkeys[3] != fkeys_last[3])
        {
            current_color = 0;
            current_plane = (current_plane + planes_count - 1) % planes_count;
            scene.load_player_plane(planes[current_plane], current_color);
        }

        if (fkeys[5] && fkeys[5] != fkeys_last[5])
        {
            auto colors_count = aircraft::get_colors_count(planes[current_plane]);
            current_color = (current_color + 1) % colors_count;
            scene.load_player_plane(planes[current_plane], current_color);
        }

        if (fkeys[4] && fkeys[4] != fkeys_last[4])
        {
            auto colors_count = aircraft::get_colors_count(planes[current_plane]);
            current_color = (current_color + colors_count - 1) % colors_count;
            scene.load_player_plane(planes[current_plane], current_color);
        }

        fkeys_last = fkeys;

    }

    glfwTerminate();

    return 0;
}

//------------------------------------------------------------
