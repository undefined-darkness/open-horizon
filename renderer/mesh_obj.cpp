//
// open horizon -- undefined_darkness@outlook.com
//

#include "mesh_obj.h"
#include "util/util.h"
#include <sstream>

namespace renderer
{
//------------------------------------------------------------

bool mesh_obj::load(const char *name, nya_resources::resources_provider &prov)
{
    std::vector<std::pair<nya_math::vec3, nya_math::vec4> > v;
    std::vector<nya_math::vec3> vn;
    std::vector<nya_math::vec2> vt;

    nya_memory::tmp_buffer_scoped data(load_resource(prov.access(name)));
    std::stringstream in(std::string((char *)data.get_data(), data.get_size()));
    std::string line;
    int last_mat_idx = -1;
    while (std::getline(in, line))
    {
        std::string id = line.substr(0,2);

        if (id == "v ")
        {
            std::istringstream s(line.substr(2));
            v.resize(v.size() + 1);
            auto &p = v.back().first;
            auto &c = v.back().second;

            if (s >> p.x && s >> p.y && s >> p.z)
            {
                if (s >> c.x && s >> c.y && s >> c.z)
                {
                    if (!(s >> c.w))
                        c.w = 1.0f;
                }
                else
                    c.set(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
        else if (id == "vn")
        {
            std::istringstream s(line.substr(2));
            vn.resize(vn.size() + 1);
            auto &n = vn.back();
            s >> n.x && s >> n.y && s >> n.z;
        }
        else if (id == "vt")
        {
            std::istringstream s(line.substr(2));
            vt.resize(vt.size() + 1);
            auto &t = vt.back();
            s >> t.x && s >> t.y;
        }
        else if (id == "f ")
        {
            static std::vector<vert> face_verts;
            face_verts.clear();
            unsigned int vi,vti,vni;
            const char *str = line.c_str() + 2;
            int off = 0;
            while (sscanf(str, "%u/%u/%u%n", &vi, &vti, &vni, &off) == 3)
            {
                str += off;

                face_verts.resize(face_verts.size() + 1);
                auto &vert = face_verts.back();

                if (--vi >= v.size() || --vti >= v.size() || --vni >= v.size())
                    return false;

                vert.pos = v[vi].first;
                vert.color = v[vi].second;
                vert.tc = vt[vti];
                vert.normal = vn[vni];
            }

            for (int i = 1; i < face_verts.size()-1; ++i)
            {
                verts.push_back(face_verts[0]);
                verts.push_back(face_verts[i]);
                verts.push_back(face_verts[i + 1]);
            }
        }
        else if (id == "us")
        {
            std::istringstream s(line);
            s >> id;
            if (id != "usemtl")
                continue;

            s >> id;

            if (last_mat_idx >= 0 && materials[last_mat_idx].name == id)
                continue;

            if (!groups.empty())
                groups.back().count = (int)verts.size() - groups.back().offset;

            groups.resize(groups.size() + 1);
            groups.back().offset = (int)verts.size();

            auto m = std::find_if(materials.begin(), materials.end(), [id](mat &m){ return m.name == id; });
            if (m == materials.end())
                return false;

            last_mat_idx = groups.back().mat_idx = int(m - materials.begin());
        }
        else if (id == "mt")
        {
            std::istringstream s(line);
            s >> id;
            if (id != "mtllib")
                continue;

            s >> id;
            auto path = get_path(name);
            nya_memory::tmp_buffer_scoped data(load_resource(prov.access((path + id).c_str())));
            std::stringstream in(std::string((char *)data.get_data(), data.get_size()));
            while (std::getline(in, line))
            {
                std::istringstream s(line);
                s >> id;
                if (id == "newmtl")
                {
                    materials.push_back({});
                    s >> materials.back().name;
                }
                else if (id == "map_Kd")
                {
                    if (materials.empty())
                        continue;

                    s >> id;
                    materials.back().tex_diffuse = path + id;
                }
            }
        }
    }

    if (!groups.empty())
        groups.back().count = (int)verts.size() - groups.back().offset;

    return true;
}

//------------------------------------------------------------
}
