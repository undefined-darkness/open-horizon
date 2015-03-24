//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"

//------------------------------------------------------------

struct location_params
{
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

    typedef value<float> fvalue;
    typedef value<unsigned int> uvalue;

    struct
    {
        fvalue znear;
        fvalue zfar;
        fvalue ground_znear;
        fvalue ground_zfar;

    } clipping_plane;

    //ToDo

public:
    bool load(const char *file_name);
};

//------------------------------------------------------------
