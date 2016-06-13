//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene_view.h"
#include "render/render.h"
#include "scene/camera.h"
#include <QMouseEvent>

//------------------------------------------------------------

void scene_view::load_location(std::string name)
{
    m_location = renderer::location();
    m_location.load(name.c_str());

    nya_scene::get_camera_proxy()->set_pos(0, 1000, 0);
    m_camera_yaw = m_camera_pitch = 0;
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
    }

    m_mouse_x = x, m_mouse_y = y;
    update();
}

//------------------------------------------------------------

void scene_view::wheelEvent(QWheelEvent *event)
{

}

//------------------------------------------------------------
