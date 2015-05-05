//
// open horizon -- undefined_darkness@outlook.com
//

#include "model.h"

#include "memory/memory_reader.h"
#include "shared.h"

//------------------------------------------------------------

struct lst
{
    std::vector<std::string> strings;

    lst(const std::string &name)
    {
        nya_memory::tmp_buffer_scoped res(load_resource(name.c_str()));
        if(!res.get_data())
            return;

        std::string s;
        bool skip = false;
        auto buf = (const char *)res.get_data();
        for (size_t i = 0; i < res.get_size(); ++i)
        {
            auto c = buf[i];
            if (c == 9)
            {
                if(!skip)
                {
                    strings.push_back(s);
                    s.clear();
                    skip = true;
                }
            }
            else if (c == 10)
                skip = false;
            else if (!skip)
                s.push_back(c);
        }
    }
};

//------------------------------------------------------------

bool model::load(const char *name, const location_params &params)
{
    if(!name || strlen(name)<3)
        return false;

    std::string folder;

    if (name[1] == '_')
    {
        if (strchr("dor", name[0])) folder = "model_id/mech/airp/";
        if (strchr("cfs", name[0])) folder = "model_id/mech/arms/";
        if (strchr("p", name[0])) folder = "model_id/mech/plyr/";
        if (strchr("kw", name[0])) folder = "model_id/mech/weap/";
    }
    else if (name[2] == '_')
    {
        if (strncmp("cc", name, 2) == 0) folder = "model_id/demo/char/";
        if (strncmp("bg", name, 2) == 0) folder = "model_id/demo/evbg/";
        if (strncmp("ig", name, 2) == 0) folder = "model_id/demo/itec/";
        if (strncmp("ig", name, 2) == 0) folder = "model_id/demo/iteg/";
    }
    //else
      //  folder = "model_id/mapo/" + loc_name + "/mapobj_" + loc_name + "_" + name + "/"

    load_tdp(folder + name + "/" + name +"_t01.tdp");

    lst main_list(folder + name + "/" + name +"_com.lst");
    for (auto &s: main_list.strings)
    {
        if (nya_resources::check_extension(s.c_str(), "tdp"))
        {
            load_tdp(s.c_str());
        }
        else if (nya_resources::check_extension(s.c_str(), "lst"))
        {
            //ToDo: load effect
        }
        else if (nya_resources::check_extension(s.c_str(), "fhm"))
            m_mesh.load(s.c_str());
    }

    return true;
}

//------------------------------------------------------------

void model::load_tdp(const std::string &name)
{
    lst tdp(name);
    typedef std::vector<char> buf;
    buf header;
    std::vector<buf> headers;
    size_t idx = 0;
    for (auto &s: tdp.strings)
    {
        nya_memory::tmp_buffer_scoped res(load_resource(s.c_str()));
        assert(res.get_data());

        if (nya_resources::check_extension(s.c_str(), "nfh"))
        {
            assert(headers.empty());
            header.resize(res.get_size());
            res.copy_to(&header[0], res.get_size());
        }
        else if (nya_resources::check_extension(s.c_str(), "pih"))
        {
            buf h(res.get_size());
            res.copy_to(&h[0], res.get_size());
            headers.push_back(h);
        }
        else if (nya_resources::check_extension(s.c_str(), "img"))
        {
            nya_memory::tmp_buffer_scoped buf(header.size() + headers[idx].size() + res.get_size());
            buf.copy_from(&header[0], header.size());
            buf.copy_from(&(headers[idx])[0], headers[idx].size(), header.size());
            buf.copy_from(res.get_data(), res.get_size(), headers[idx].size() + header.size());
            ++idx;
            auto hash = shared::load_texture(buf.get_data(), buf.get_size());
            //printf("tex %d\n", hash);
        }
    }
    assert(idx == headers.size());
}

//------------------------------------------------------------

int model::get_bones_count(int lod_idx)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
        return -1;

    return m_mesh.get_mesh(lod_idx).get_bones_count();
}

//------------------------------------------------------------

const char *model::get_bone_name(int lod_idx, int bone_idx)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
        return 0;

    return m_mesh.get_mesh(lod_idx).get_bone_name(bone_idx);
}

//------------------------------------------------------------

int model::get_bone_idx(int lod_idx, const char *name)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
        return -1;

    return m_mesh.get_mesh(lod_idx).get_bone_idx(name);
}

//------------------------------------------------------------

int model::find_bone_idx(int lod_idx, const char *name_part)
{
    if (!name_part)
        return -1;

    size_t len = strlen(name_part);
    for (int j = 0; j < get_bones_count(lod_idx); ++j)
    {
        const char *bname = m_mesh.get_mesh(lod_idx).get_bone_name(j);
        if (!bname || !bname[0])
            continue;

        if (strncmp(bname, name_part, len) == 0)
            return j;
    }

    return -1;
}

//------------------------------------------------------------

nya_math::vec3 model::get_bone_pos(int lod_idx, int bone_idx)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
    {
        const static nya_math::vec3 pos;
        return pos;
    }

    return m_mesh.get_mesh(lod_idx).get_bone_pos(bone_idx);
}

//------------------------------------------------------------

nya_math::quat model::get_bone_rot(int lod_idx, int bone_idx)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
    {
        const static nya_math::quat rot;
        return rot;
    }

    return m_mesh.get_mesh(lod_idx).get_bone_rot(bone_idx);
}

//------------------------------------------------------------

void model::set_bone_rot(int lod_idx, int bone_idx, const nya_math::quat &rot)
{
    if (lod_idx < 0 || lod_idx >= m_mesh.get_lods_count())
        return;

    return m_mesh.get_mesh(lod_idx).set_bone_rot(bone_idx, rot, true);
}

//------------------------------------------------------------
