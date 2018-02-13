//
// open horizon -- undefined_darkness@outlook.com
//

#include "plane_camera.h"
#include "scene/camera.h"

namespace renderer
{
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
    if (m_ignore_dpos)
        return;

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
    m_aspect = aspect;
    nya_scene::get_camera().set_proj(50.0, m_aspect, m_near, m_far);
    update();
}

//------------------------------------------------------------

void plane_camera::set_near_far(float znear, float zfar)
{
    m_near = znear;
    m_far = zfar;
    nya_scene::get_camera().set_proj(50.0, m_aspect, m_near, m_far);
    update();
}

//------------------------------------------------------------

void plane_camera::update()
{
    nya_math::quat r = m_rot;
    r.v.x = -r.v.x, r.v.y = -r.v.y;

    r = r * (m_ignore_drot ? nya_math::quat(0.0f, nya_math::constants::pi, 0.0f) : nya_math::quat(m_drot.x, m_drot.y, 0.0f));
    //nya_math::vec3 r=m_drot+m_rot;

    //cam->set_rot(r.y*180.0f/3.14f,r.x*180.0f/3.14f,0.0);
    nya_scene::get_camera().set_rot(r);

    //nya_math::quat rot(-r.x,-r.y,0.0f);
    //rot=rot*m_rot;
    
    //nya_math::vec3 pos=m_pos+rot.rotate(m_dpos);
    r.v.x = -r.v.x, r.v.y = -r.v.y;
    nya_math::vec3 pos = m_pos + r.rotate(m_ignore_dpos? nya_math::vec3(0,-0.06,-0.06 + m_fixed_dist) : m_dpos);
    
    nya_scene::get_camera().set_pos(pos.x, pos.y, pos.z);
}

//------------------------------------------------------------
}
