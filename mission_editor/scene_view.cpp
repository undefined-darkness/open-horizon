//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene_view.h"
#include "render/render.h"
#include "scene/camera.h"
#include "renderer/shared.h"
#include <QMouseEvent>
#include <QWheelEvent>

//------------------------------------------------------------

void scene_view::load_location(std::string name)
{
    m_location = renderer::location();
    shared::clear_textures();
    m_location.load(name.c_str());
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
    nya_scene::get_camera_proxy()->set_rot(m_camera_yaw, m_camera_pitch, 0.0f);
    nya_scene::get_camera_proxy()->set_pos(m_camera_pos);

    nya_render::clear(true, true);
    m_location.update_tree_texture();
    m_location.draw();
}

//------------------------------------------------------------

void scene_view::mousePressEvent(QMouseEvent *event)
{
    m_mouse_x = event->localPos().x();
    m_mouse_y = event->localPos().y();
}

//------------------------------------------------------------

void scene_view::mouseMoveEvent(QMouseEvent *event)
{
    int x = event->localPos().x();
    int y = event->localPos().y();

    auto btns = event->buttons();
    if (btns.testFlag(Qt::LeftButton))
    {
        m_camera_yaw += x - m_mouse_x;
        m_camera_pitch += y - m_mouse_y;
        m_camera_yaw.normalize();
        m_camera_pitch.clamp(-90, 90);
    }

    if (btns.testFlag(Qt::RightButton))
    {
        nya_math::vec2 dpos(x - m_mouse_x, y - m_mouse_y);
        dpos.rotate(m_camera_yaw);
        dpos *= m_camera_pos.y / 30.0f;
        m_camera_pos.x += dpos.x, m_camera_pos.z += dpos.y;
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
