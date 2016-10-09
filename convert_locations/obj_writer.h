//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <sstream>

//------------------------------------------------------------

class obj_writer
{
public:
    std::string get_string(std::string matlib_name) const { return "mtllib " + matlib_name + "\n" + m_data.str(); }
    std::string get_mat_string() const { return m_mat_data.str(); }

public:
    void add_pos(const nya_math::vec3 &v) { m_data<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<"\n"; }
    void add_pos(const nya_math::vec3 &v, const nya_math::vec4 &c) { m_data<<"v "<<v.x<<" "<<v.y<<" "<<v.z<<" "
                                                                                 <<c.x<<" "<<c.y<<" "<<c.z<<" "<<c.w<<"\n"; }
    void add_normal(const nya_math::vec3 &v) { m_data<<"vn "<<v.x<<" "<<v.y<<" "<<v.z<<"\n"; }
    void add_tc(const nya_math::vec2 &v) { m_data<<"vt "<<v.x<<" "<<v.y<<"\n"; }

    void add_face(int vcount)
    {
        m_data<<"f ";
        for (int i = 0; i < vcount; ++i)
        {
            ++m_last_face;
            m_data<<" "<<m_last_face<<"/"<<m_last_face<<"/"<<m_last_face;
        }
        m_data<<"\n";
    }

    void add_group(std::string group_name, std::string mat_name) { m_data<<"\ng "<<group_name<<"\nusemtl "<<mat_name<<"\n"; }

    void add_material(std::string mat_name, std::string tex_name)
    {
        m_mat_data<<"newmtl "<<mat_name<<"\nillum 2\n"
                                "Kd 0.800000 0.800000 0.800000\n"
                                "Ka 0.200000 0.200000 0.200000\n"
                                "Ks 0.000000 0.000000 0.000000\n"
                                "Ke 0.000000 0.000000 0.000000\n"
                                "Ns 0.000000\n";
        m_mat_data<<"map_Kd "<<tex_name<<"\n\n";
    }

private:
    std::ostringstream m_data, m_mat_data;
    int m_last_face = 0;
};

//------------------------------------------------------------
