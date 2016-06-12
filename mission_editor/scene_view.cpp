//
// open horizon -- undefined_darkness@outlook.com
//

#include "scene_view.h"
#include "render/render.h"

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
}

//------------------------------------------------------------

void scene_view::paintGL()
{
    nya_render::clear(true, true);
}

//------------------------------------------------------------

void scene_view::mousePressEvent(QMouseEvent *event)
{

}

//------------------------------------------------------------

void scene_view::mouseMoveEvent(QMouseEvent *event)
{

}

//------------------------------------------------------------

void scene_view::wheelEvent(QWheelEvent *event)
{

}

//------------------------------------------------------------
