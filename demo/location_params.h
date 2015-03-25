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
    typedef nya_math::vec3 color3;
    typedef nya_math::vec4 color4;
    typedef nya_math::vec3 vec3;

    struct
    {
        fvalue znear;
        fvalue zfar;
        fvalue ground_znear;
        fvalue ground_zfar;

    } clipping_plane;

    struct skysphere
    {
        color3 ambient;
        fvalue lens_brightness;

        color3 light2_color;
        vec3 light2_dir;
        fvalue light2_power;

        color3 light3_color;
        vec3 light3_dir;
        fvalue light3_power;

        fvalue player_shadow_brightness;
        color3 player_shadow_color;

        fvalue skysphere_intensity;
        color3 specular_color;
        fvalue specular_power;

        color3 sun_color;
        fvalue sun_power;
        color3 sun_rgb;

        color3 sun_flare_rgb;
        fvalue sun_flare_size;
    };

    struct
    {
        struct
        {
            fvalue gravity_speed;
            vec3 gravity_dir;

            color4 air_color;
            fvalue air_ratio;

            color4 rain_color;
            fvalue rain_ratio;

            color4 snow_color;
            fvalue snow_ratio;

        } weather;

        fvalue fog_density;
        fvalue fog_height;
        fvalue fog_height_density;
        fvalue fog_height_fade_density;
        fvalue fog_height_fresnel;

        skysphere low;
        skysphere high;

        vec3 moon_dir;
        color3 moon_color;
        fvalue moon_size;

        vec3 sun_dir;
        fvalue sun_size;

        struct
        {
            color3 mesh_color;
            fvalue mesh_contrast;
            fvalue mesh_fog_power;
            fvalue mesh_fog_fresnel;
            fvalue mesh_fog_fresnel_max;
            fvalue mesh_power;
            fvalue mesh_scale;

            color3 obj_ambient;
            color3 obj_sun_color;

            color3 parts_color;
            fvalue parts_contrast;
            fvalue parts_fog_power;
            fvalue parts_fresnel;
            fvalue parts_fresnel_max;
            fvalue parts_power;
            fvalue parts_reflection_power;
            fvalue parts_scale;

        } mapspecular;

    } sky;

    //ToDo

public:
    bool load(const char *file_name);
};

//------------------------------------------------------------
