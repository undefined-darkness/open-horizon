//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <QGLWidget>
#include "renderer/location.h"

//------------------------------------------------------------

class scene_view : public QGLWidget
{
    Q_OBJECT

public:
    scene_view(QWidget *parent = NULL);

public:
    void load_location(std::string name);

private:
    virtual void initializeGL() override;
    virtual void resizeGL(int w, int h) override;
    virtual void paintGL() override;

private:
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

private:
    renderer::location m_location;
    nya_math::angle_deg m_camera_yaw, m_camera_pitch;
    int m_mouse_x = 0, m_mouse_y = 0;
};

//------------------------------------------------------------
