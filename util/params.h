//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"
#include "memory/memory_reader.h"
#include "math/constants.h"
#include "math/quaternion.h"
#include "util.h"
#include <sstream>
#include <map>
#include <stdint.h>

namespace params
{
//------------------------------------------------------------

//value with auto-initialisation
template<typename t> class value
{
public:
    inline value(): m_value(0.0f) {}
    inline value(const t& v): m_value(v) {}
    inline operator t&() { return m_value; }
    inline operator const t&() const { return m_value; }

private: t m_value;
};

//------------------------------------------------------------

typedef value<float> fvalue;
typedef value<unsigned int> uvalue;
typedef nya_math::vec3 color3;
typedef nya_math::vec4 color4;
typedef nya_math::vec3 vec3;

//------------------------------------------------------------

class memory_reader: public nya_memory::memory_reader
{
public:
    color3 read_color3_uint()
    {
        params::color3 c;
        c.z = read<uint8_t>()/255.0;
        c.y = read<uint8_t>()/255.0;
        c.x = read<uint8_t>()/255.0;
        skip(1);
        return c;
    }

    color3 read_color3()
    {
        color3 c;
        c.z = read<float>();
        c.y = read<float>();
        c.x = read<float>();
        return c / 255.0;
    }

    color4 read_color4()
    {
        color4 c;
        c.w = read<float>();
        c.z = read<float>();
        c.y = read<float>();
        c.x = read<float>();
        return c / 255.0;
    }

    vec3 read_dir_py()
    {
        const float pitch = read<float>() * nya_math::constants::pi / 180.0f;
        const float yaw = read<float>() * nya_math::constants::pi / 180.0f;
        return nya_math::quat(pitch, -yaw, 0.0f).rotate(nya_math::vec3(0.0, 0.0, 1.0));
    }

    memory_reader(const void *data, size_t size): nya_memory::memory_reader(data, size) {}
};

//------------------------------------------------------------

class text_params
{
public:
    text_params(const char *fname)
    {
        nya_memory::tmp_buffer_scoped r(load_resource(fname));
        if (!r.get_data())
            return;

        std::istringstream s((char *)r.get_data());
        std::string type, name, divider;

        while (s >> type)
        {
            if (type[0] == '#')
                continue;

            if (!(s >> name))
                continue;

            if (!(s >> divider) || divider != ":")
                continue;

            if (type == "float")
                s >> m_float_params[name];
            else if (type == "S16" || type == "s16" || type == "U16" || type == "u16")
                s >> m_int_params[name];
            else
                printf("invalid text param %s in %s\n", type.c_str(), fname);
        }
    }

    float get_float(const std::string &name, float default_value = 0.0f) const
    {
        auto p = m_float_params.find(name);
        return p == m_float_params.end() ? default_value : p->second;
    }

    int get_int(const std::string &name, int default_value = 0) const
    {
        auto p = m_int_params.find(name);
        return p == m_int_params.end() ? default_value : p->second;
    }

private:
    std::map<std::string,float> m_float_params;
    std::map<std::string,int> m_int_params;
};

//------------------------------------------------------------
};
