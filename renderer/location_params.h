//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"
#include "util/params.h"

//------------------------------------------------------------

struct location_params
{
    struct
    {
        params::fvalue znear;
        params::fvalue zfar;
        params::fvalue ground_znear;
        params::fvalue ground_zfar;

    } clipping_plane;

    params::fvalue tone_saturation;

    struct
    {
        params::fvalue bloom_offset;
        params::fvalue bloom_saturation;
        params::fvalue bloom_scale;
        params::fvalue bloom_threshold;

        params::fvalue bloom_kernel_brightness;
        params::fvalue bloom_kernel_sigma;

        params::fvalue luminance_measure_area;
        params::fvalue luminance_speed;

        params::fvalue middle_gray_range_max;
        params::fvalue middle_gray_range_min;

    } hdr;

    struct
    {
        params::fvalue far_fade_near;
        params::fvalue far_fade_far;

        params::fvalue ambient_obj_upper;
        params::fvalue ambient_obj_lower;

        params::color3 ambient_lower_color;
        params::color3 ambient_upper_color;
        params::fvalue ambient_power;

        params::color3 diffuse_color;
        params::fvalue diffuse_power;

        params::fvalue diffuse_min;

        params::fvalue intensity;
        params::fvalue highflat_alpha;

    } cloud;

    struct skysphere
    {
        params::color3 ambient;
        params::fvalue lens_brightness;

        params::color3 light2_color;
        params::vec3 light2_dir;
        params::fvalue light2_power;

        params::color3 light3_color;
        params::vec3 light3_dir;
        params::fvalue light3_power;

        params::fvalue player_shadow_brightness;
        params::color3 player_shadow_color;

        params::fvalue skysphere_intensity;
        params::color3 specular_color;
        params::fvalue specular_power;

        params::color3 sun_color;
        params::fvalue sun_power;
        params::color3 sun_rgb;

        params::color3 sun_flare_rgb;
        params::fvalue sun_flare_size;
    };

    struct
    {
        struct
        {
            params::fvalue gravity_speed;
            params::vec3 gravity_dir;

            params::color4 air_color;
            params::fvalue air_ratio;

            params::color4 rain_color;
            params::fvalue rain_ratio;

            params::color4 snow_color;
            params::fvalue snow_ratio;

        } weather;

        params::fvalue fog_density;
        params::fvalue fog_height;
        params::fvalue fog_height_density;
        params::fvalue fog_height_fade_density;
        params::fvalue fog_height_fresnel;

        skysphere low;
        skysphere high;

        params::vec3 moon_dir;
        params::color3 moon_color;
        params::fvalue moon_size;

        params::vec3 sun_dir;
        params::fvalue sun_size;

        struct
        {
            params::color3 mesh_color;
            params::fvalue mesh_contrast;
            params::fvalue mesh_fog_power;
            params::fvalue mesh_fog_fresnel;
            params::fvalue mesh_fog_fresnel_max;
            params::fvalue mesh_power;
            params::fvalue mesh_scale;

            params::color3 obj_ambient;
            params::color3 obj_sun_color;

            params::color3 parts_color;
            params::fvalue parts_contrast;
            params::fvalue parts_fog_power;
            params::fvalue parts_fresnel;
            params::fvalue parts_fresnel_max;
            params::fvalue parts_power;
            params::fvalue parts_reflection_power;
            params::fvalue parts_scale;

        } mapspecular;

    } sky;

    struct
    {
        params::fvalue mesh_power;
        params::fvalue mesh_range;
        params::fvalue mesh_repeat;

    } detail;

    //ToDo

public:
    location_params() { sky.sun_dir.y = -1.0f; }

public:
    bool load(const char *file_name);
};

//------------------------------------------------------------
