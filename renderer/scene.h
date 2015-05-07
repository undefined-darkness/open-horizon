//
// open horizon -- undefined_darkness@outlook.com
//

#include "world.h"
#include "lens_flare.h"
#include "plane_camera.h"
#include "scene/postprocess.h"
#include "ui.h"

namespace renderer
{
//------------------------------------------------------------

class scene: private nya_scene::postprocess, public world
{
public:
    plane_camera camera;

private: //ui temporary here, needs refactoring
    ui m_ui;
    bool m_paused;
    bool m_loading;
    bool m_fonts_loaded;

private:
    lens_flare m_flare;
    nya_scene::texture_proxy m_curve;
    params::fvalue m_luminance_speed;
    int m_fade_time;
    int m_help_time;
    nya_math::vec3 m_cam_fp_off;
    nya_scene::shader m_cockpit_black;
    nya_render::screen_quad m_cockpit_black_quad;

private:
    int m_frame_counter, m_frame_counter_time, m_fps;

public:
    scene(): m_fade_time(0), m_help_time(3000), m_frame_counter(0), m_frame_counter_time(0), m_fps(0), m_paused(false), m_loading(false),
    m_fonts_loaded(false) {}

public:
    void switch_camera();
    void resize(unsigned int width,unsigned int height);
    void update(int dt);
    void draw();
    void pause(bool paused) { m_paused = paused; }
    void loading(bool loading) { m_loading = loading; }

private:
    aircraft_ptr add_aircraft(const char *name, int color, bool player);
    void set_location(const char *name);
    void draw_scene(const char *pass, const char *tags);
};

//------------------------------------------------------------
}
