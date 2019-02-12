//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/quaternion.h"

namespace renderer
{
//------------------------------------------------------------

class plane_camera
{
public:
    void add_delta_rot(float dx, float dy);
    const nya_math::vec2 &get_delta_rot() { return m_drot.xy(); }
    void reset_delta_rot() { m_drot.x = 0.0f; m_drot.y = 3.14f; update(); }
    void add_delta_pos(float dx, float dy, float dz);
    void reset_delta_pos() { if (m_ignore_dpos) return; m_dpos = nya_math::vec3(); update(); }//0.0f, 2.0f, -10.0f); }
    void set_ignore_delta_pos(bool ignore) { m_ignore_dpos = ignore; }
    void set_ignore_delta_rot(bool ignore) { m_ignore_drot = ignore; }
    void set_fixed_dist(float d) { m_fixed_dist = d; update(); }
    float get_fixed_dist() const { return m_fixed_dist; }

    void set_aspect(float aspect);
    void set_near_far(float znear, float zfar);
    void set_pos(const nya_math::vec3 &pos) { m_pos = pos; update(); }
    void set_rot(const nya_math::quat &rot) { m_rot = rot; update(); }

    const nya_math::vec3 &get_pos() const { return m_pos; }

private:
    void update();

public:
    plane_camera(): m_ignore_dpos(false), m_ignore_drot(false),
                    m_fixed_dist(0.0), m_aspect(1.0), m_near(1.0), m_far(1000.0) { m_drot.y = 3.14f; reset_delta_pos(); }

private:
    nya_math::vec3 m_drot;
    nya_math::vec3 m_dpos;

    nya_math::vec3 m_pos;
    nya_math::quat m_rot;
    bool m_ignore_dpos;
    bool m_ignore_drot;
    float m_fixed_dist;
    float m_aspect;
    float m_near, m_far;
};

//------------------------------------------------------------
}
