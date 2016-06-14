//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene_view.h"
#include "render/render.h"
#include "scene/camera.h"
#include "renderer/shared.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>

//------------------------------------------------------------

static const float location_size = 256 * 256.0f;

//------------------------------------------------------------

void scene_view::load_location(std::string name)
{
    m_location = renderer::location();
    shared::clear_textures();
    m_location.load(name.c_str());
    m_location_phys.set_location(name.c_str());
    m_camera_pos.set(0, 1000, 0);
    m_camera_yaw = 0;
    m_camera_pitch = 30;
}

//------------------------------------------------------------

scene_view::scene_view(QWidget *parent): QGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

//------------------------------------------------------------

void scene_view::initializeGL()
{
    nya_render::set_clear_color(0.2f,0.4f,0.5f,0.0f);
    nya_render::apply_state(true);
    m_dd.set_point_size(3.0f);
}

//------------------------------------------------------------

void scene_view::resizeGL(int w, int h)
{
    nya_render::set_viewport(0, 0, w, h);
    nya_scene::get_camera_proxy()->set_proj(60.0, w/float(h), 1.0f, 100000.0f);
}

//------------------------------------------------------------

void scene_view::paintGL()
{
    const float height = m_location_phys.get_height(m_camera_pos.x, m_camera_pos.z);

    nya_scene::get_camera_proxy()->set_rot(m_camera_yaw, m_camera_pitch, 0.0f);
    nya_scene::get_camera_proxy()->set_pos(m_camera_pos + nya_math::vec3(0, height, 0));

    nya_render::clear(true, true);
    m_location.update_tree_texture();
    m_location.draw();

    nya_render::set_state(nya_render::state());
    nya_render::depth_test::disable();
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    m_dd.draw();
}

//------------------------------------------------------------

nya_math::vec3 scene_view::world_cursor_pos() const
{
    auto r = nya_render::get_viewport();
    nya_math::vec4 pos(m_mouse_x, r.height - m_mouse_y, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glReadPixels(pos.x, pos.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &pos.z);
    nya_render::apply_state(true);

    pos.x = 2.0 * pos.x / r.width - 1.0f;
    pos.y = 2.0 * pos.y / r.height - 1.0f;
    pos.z = 2.0 * pos.z - 1.0;

    auto mvp = nya_render::get_modelview_matrix() * nya_render::get_projection_matrix();
    mvp.invert();

    pos = mvp * pos;
    pos /= pos.w;

    return pos.xyz();
}

//------------------------------------------------------------

void scene_view::mousePressEvent(QMouseEvent *event)
{
    m_mouse_x = event->localPos().x();
    m_mouse_y = event->localPos().y();

    //test
    if (event->buttons().testFlag(Qt::LeftButton))
    {
        m_dd.add_point(world_cursor_pos(), nya_math::vec4(1.0, 0.3, 0.3, 1.0));
        update();
    }
}

//------------------------------------------------------------

void scene_view::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->localPos().x();
    int y = event->localPos().y();

    auto btns = event->buttons();
    if (btns.testFlag(Qt::RightButton))
    {
        if (event->modifiers().testFlag(Qt::ShiftModifier))
        {
            m_camera_yaw += x - m_mouse_x;
            m_camera_pitch += y - m_mouse_y;
            m_camera_yaw.normalize();
            m_camera_pitch.clamp(-90, 90);
        }
        else if (event->modifiers().testFlag(Qt::AltModifier))
        {
            m_camera_pos.y -= (y - m_mouse_y) * 10.0f;
            m_camera_pos.y = nya_math::clamp(m_camera_pos.y, 20.0f, 5000.0f);
        }
        else
        {
            nya_math::vec2 dpos(x - m_mouse_x, y - m_mouse_y);
            dpos.rotate(m_camera_yaw);
            dpos *= m_camera_pos.y / 30.0f;
            m_camera_pos.x += dpos.x, m_camera_pos.z += dpos.y;
            m_camera_pos.x = nya_math::clamp(m_camera_pos.x, -location_size, location_size);
            m_camera_pos.z = nya_math::clamp(m_camera_pos.z, -location_size, location_size);
        }
    }

    m_mouse_x = x, m_mouse_y = y;
    update();
}

//------------------------------------------------------------

void scene_view::wheelEvent(QWheelEvent *event)
{
    m_camera_pos.y += event->delta();
    m_camera_pos.y = nya_math::clamp(m_camera_pos.y, 20.0f, 5000.0f);
    update();
}

//------------------------------------------------------------
