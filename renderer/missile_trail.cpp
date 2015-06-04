//
// open horizon -- undefined_darkness@outlook.com
//

#include "missile_trail.h"

#include "scene/camera.h"
#include "shared.h"

namespace renderer
{
//------------------------------------------------------------

void missile_trail::init()
{
    m_first = true;
    m_debug.set_line_width(2.0);
}

//------------------------------------------------------------

void missile_trail::update(const nya_math::vec3 &pos, int dt)
{
    if (m_first)
    {
        m_first = false;
        m_prev_point = pos;
        return;
    }

    const float part_len = 30.0;
    if ((pos - m_prev_point).length_sq() < part_len*part_len)
        return;

    m_debug.add_line(pos, m_prev_point, nya_math::vec4(1.0, 1.0, 1.0, 0.3));
    //m_debug.add_line(pos, pos + nya_math::vec3(0.0,part_len,0.0), nya_math::vec4(1.0, 0.0, 0.0, 0.5));
    m_prev_point = pos;
}

//------------------------------------------------------------

void missile_trail::draw() const
{
    nya_render::set_modelview_matrix(nya_scene::get_camera().get_view_matrix());
    nya_render::set_state(nya_render::state());
    nya_render::blend::enable(nya_render::blend::src_alpha, nya_render::blend::one);
    nya_render::zwrite::disable();
    m_debug.draw();
}

//------------------------------------------------------------

void missile_trail::release()
{
    m_debug.release();
}

//------------------------------------------------------------
}
