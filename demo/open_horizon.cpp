//
// open horizon -- undefined_darkness@outlook.com
//

#include "GLFW/glfw3.h"

#include "qdf_provider.h"
#include "../dpl.h" //ToDo: dpl_provider.h

#include "aircraft.h"
#include "location.h"
#include "clouds.h"
#include "shared.h"
#include "debug.h"

#include "render/render.h"
#include "math/scalar.h"
#include "system/system.h"
#include "resources/file_resources_provider.h"
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

    void set_aspect(float aspect);
    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; update(); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; update(); }

private:
    void update();

public:
    plane_camera(): m_dpos(0.0f, 2.0f, -10.0f) { m_drot.y = 3.14f; }

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
    const char *plane_name = 0;
    int plane_color = 0;
    const char *location_name = "ms01";
    //ms06 - dubai
    //ms11b - moscow
    //ms14 - washinghton
    //ms50 - tokyo
    //ms30 - paris

    plane_name = "su35", plane_color=1;
    //plane_name = "f22a", plane_color=0;
    //plane_name = "su33", plane_color=0;
    //plane_name = "su34", plane_color=1;

    //f16c, av8b, su24, su25, f14d, m29a, m21b - no tail anim
    //b01b, su24, f14d - wings anim
    //b02a - anim offsets
    //su37 - weird colors
    //yf23 - weird textures
    //pkfa - missile bays anim
    //a10a, fa44, ac130, f17a, f15e, f15m - anim assert
    //m21b - no cockpit
    //helicopters - not yet supported

#ifndef _WIN32
    chdir(nya_system::get_app_path());
#endif

    qdf_resources_provider qdfp;
    if (!qdfp.open_archive("datafile.qdf"))
    {
        printf("unable to open datafile.qdf");
        return 0;
    }

    nya_resources::set_resources_provider(&qdfp);

    class target_resource_provider: public nya_resources::resources_provider
    {
        nya_resources::resources_provider &m_provider;

    public:
        target_resource_provider(nya_resources::resources_provider &provider): m_provider(provider) {}

    private:
        nya_resources::resource_data *access(const char *resource_name)
        {
            if (!resource_name)
                return 0;

            const std::string str(resource_name);
            if (m_provider.has(("target/" + str).c_str()))
                return m_provider.access(("target/" + str).c_str());

            if (m_provider.has(("common/" + str).c_str()))
                return m_provider.access(("common/" + str).c_str());

            if (m_provider.has(str.c_str()))
                return m_provider.access(str.c_str());

            static nya_resources::file_resources_provider fprov;
            static bool dont_care = fprov.set_folder(nya_system::get_app_path()); dont_care = dont_care;

            return fprov.access(resource_name);
        }

        bool has(const char *resource_name)
        {
            if (!resource_name)
                return false;

            const std::string str(resource_name);
            return m_provider.has(("target/" + str).c_str()) || m_provider.has(("common/" + str).c_str()) || m_provider.has(resource_name);
        }
    } trp(nya_resources::get_resources_provider());

    nya_resources::set_resources_provider(&trp);

    //125 160 125   105 140 105
    //unsigned char c0[] = {125,160,125}; //from gpa
    //unsigned char c1[] = {105,140,105};
    //unsigned char c2[] = {113,159,123}; //from screenshot
    //unsigned char c3[] = {133,177,143};
    //unsigned char c4[] = {0x7d,0xa0,0x7d};
    //unsigned char c4[] = {200,200,200};
    //find_data(qdfp,c0,3);
    //find_data(qdfp,c1,3);
    //find_data(qdfp,c2,3);
    //find_data(qdfp,c4,3,1);

    //unsigned int color = 2107669760;
    //unsigned int color = 2594680320;
    //find_data(qdfp,&color,4);

    //float f0[] = {1.0f/125, 1.0f/160, 1.0f/125};
    //float f0[] = {0.4901961, 0.627451, 0.4901961};
    //float f2[] = {1.0/113, 1.0/159, 1.0/123};
    //find_data(qdfp,f2,3,0.001f,4);

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

    class : public nya_scene::postprocess
    {
    public:
        location loc;
        aircraft player_plane;
        effect_clouds clouds;

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
    } scene;

    scene.loc.load(location_name);
    scene.clouds.load(location_name);
    scene.player_plane.load(plane_name, plane_color);
    scene.player_plane.set_pos(nya_math::vec3(-300, 50, 2000));

    auto &p = scene.loc.get_params();

    scene.load("postprocess.txt");
    auto curve = load_tonecurve((std::string("Map/tonecurve_") + location_name + ".tcb").c_str());
    scene.set_texture("color_curve", nya_scene::texture_proxy(curve));
    scene.set_shader_param("bloom_param", nya_math::vec4(p.hdr.bloom_threshold, p.hdr.bloom_offset, p.hdr.bloom_scale, 1.0));
    scene.set_shader_param("saturation", nya_math::vec4(p.tone_saturation * 0.01, 0.0, 0.0, 0.0));
    scene.set_shader_param("screen_radius", nya_math::vec4(1.185185, 0.5 * 4.0 / 3.0, 0.0, 0.0));
    scene.set_shader_param("damage_frame", nya_math::vec4(0.35, 0.5, 1.0, 0.1));

    plane_camera camera;
    auto off = -scene.player_plane.get_camera_offset();
    camera.add_delta_pos(off.x, off.y, off.z);

    double mx,my;
    glfwGetCursorPos(window, &mx, &my);

    int frame_counter = 0;
    int frame_counter_time = 0;
    int fps = 0;

    int fade_time = 2500;

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

        if (fade_time > 0)
        {
            fade_time-=dt;
            if (fade_time < 0)
                fade_time = 0;

            scene.set_shader_param("fade_color", nya_math::vec4(0.0, 0.0, 0.0, fade_time / 2500.0f));
        }

        static bool paused = false;
        static bool speed10x = false;
        if (!paused)
            scene.player_plane.update(speed10x ? dt * 10 : dt);

        scene.set_shader_param("damage_frame_color", nya_math::vec4(1.0, 0.0, 0.0235, nya_math::max(1.0 - scene.player_plane.get_hp(), 0.0)));

        char title[255];
        sprintf(title,"speed: %7d alt: %7d \t fps: %3d  %s", int(scene.player_plane.get_speed()), int(scene.player_plane.get_alt()), fps, paused ? "paused" : "");
        glfwSetWindowTitle(window,title);

        int new_screen_width, new_screen_height;
        glfwGetFramebufferSize(window, &new_screen_width, &new_screen_height);
        if (new_screen_width != screen_width || new_screen_height != screen_height)
        {
            screen_width = new_screen_width, screen_height = new_screen_height;
            scene.resize(screen_width, screen_height);
            camera.set_aspect(screen_height > 0 ? float(screen_width) / screen_height : 1.0f);
        }

        camera.set_pos(scene.player_plane.get_pos());
        camera.set_rot(scene.player_plane.get_rot());

        scene.loc.update(dt);
        scene.draw(dt);

        glfwSwapBuffers(window);
        glfwPollEvents();

        //controls

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        if (glfwGetMouseButton(window, 0))
            camera.add_delta_rot((y - my) * 0.03, (x - mx) * 0.03);

        if (glfwGetMouseButton(window, 1))
            camera.add_delta_pos(0, 0, my - y);

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
            if (fabsf(axis[2]) > joy_dead_zone) camera.add_delta_rot(0.0f, -axis[2] * 0.05f);
            if (fabsf(axis[3]) > joy_dead_zone) camera.add_delta_rot(axis[3] * 0.05f, 0.0f);
        }

        if (buttons_count > 11)
        {
            if (buttons[8]) c_rot.y = -1.0f;
            if (buttons[9]) c_rot.y = 1.0f;
            if (buttons[10]) c_brake = 1.0f;
            if (buttons[11]) c_throttle = 1.0f;

            if (buttons[2]) camera.reset_delta_rot();

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
    }

    glfwTerminate();

    return 0;
}

//------------------------------------------------------------
