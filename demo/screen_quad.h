//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "render/vbo.h"

//------------------------------------------------------------

class screen_quad
{
public:
    void init()
    {
        struct { float x,y; } verts[4];
        verts[0].x = verts[1].x = verts[1].y = verts[3].y = 1.0f;
        verts[3].x = verts[2].x = verts[0].y = verts[2].y = 0.0f;

        m_mesh.set_vertex_data(verts, sizeof(verts[0]), 4);
        m_mesh.set_vertices(0,2);
        m_mesh.set_element_type(nya_render::vbo::triangle_strip);
    }

    void draw()
    {
        m_mesh.bind();
        m_mesh.draw();
        m_mesh.unbind();
    }

    void release() { m_mesh.release(); }

private:
    nya_render::vbo m_mesh;
};

//------------------------------------------------------------
