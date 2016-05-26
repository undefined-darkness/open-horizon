//
// open horizon -- undefined_darkness@outlook.com
//

#include "location_params.h"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include <string.h>
#include <stdio.h>
#include "shared.h"

//------------------------------------------------------------

bool location_params::load(const char *file_name)
{
    *this = location_params(); //reset if was loaded already

    if (!file_name || !file_name[0])
        return false;

    auto text = params::text_params((std::string(file_name).substr(0, strlen(file_name) - 3) + "txt").c_str());

    nya_memory::tmp_buffer_scoped fi_data(load_resource(file_name));
    params::memory_reader reader(fi_data.get_data(), fi_data.get_size());

    clipping_plane.ground_zfar = reader.read<float>();
    clipping_plane.ground_znear = reader.read<float>();
    clipping_plane.zfar = reader.read<float>();
    clipping_plane.znear = reader.read<float>();

    reader.skip(54*4); //clouds

    cloud.far_fade_near = text.get_float(".Cloud.FarFade.Near");
    cloud.far_fade_far = text.get_float(".Cloud.FarFade.Far");
    cloud.ambient_obj_upper = text.get_float(".Cloud.A.Ambient.OBJ_Upper");
    cloud.ambient_obj_lower = text.get_float(".Cloud.A.Ambient.OBJ_Lower");

    cloud.ambient_power = text.get_float(".Cloud.Ambient.Power");
    cloud.ambient_lower_color.x = text.get_float(".Cloud.Ambient.LowerColor.R");
    cloud.ambient_lower_color.y = text.get_float(".Cloud.Ambient.LowerColor.G");
    cloud.ambient_lower_color.z = text.get_float(".Cloud.Ambient.LowerColor.B");
    cloud.ambient_upper_color.x = text.get_float(".Cloud.Ambient.UpperColor.R");
    cloud.ambient_upper_color.y = text.get_float(".Cloud.Ambient.UpperColor.G");
    cloud.ambient_upper_color.z = text.get_float(".Cloud.Ambient.UpperColor.B");

    cloud.diffuse_power = text.get_float(".Cloud.Diffuse.Power");
    cloud.diffuse_color.x = text.get_float(".Cloud.Diffuse.Color.R");
    cloud.diffuse_color.y = text.get_float(".Cloud.Diffuse.Color.G");
    cloud.diffuse_color.z = text.get_float(".Cloud.Diffuse.Color.B");
    cloud.diffuse_min = text.get_float(".Cloud.Diffuse.Min");

    cloud.intensity = text.get_float(".Cloud.SkySphere.intensity");
    cloud.highflat_alpha = text.get_float(".Cloud.HiFlatAlpha");

    hdr.bloom_offset = reader.read<float>();
    hdr.bloom_saturation = reader.read<float>();
    hdr.bloom_scale = reader.read<float>();
    hdr.bloom_threshold = reader.read<float>();
    hdr.bloom_kernel_brightness = reader.read<float>();
    hdr.bloom_kernel_sigma = reader.read<float>();
    reader.skip(2); //enabled
    reader.skip(2); //send
    hdr.luminance_measure_area = reader.read<float>();
    hdr.luminance_speed = reader.read<float>();
    hdr.middle_gray_range_max = reader.read<float>();
    hdr.middle_gray_range_min = reader.read<float>();
    reader.skip(2); //show debug

    reader.skip(2*4+5*2); //mlaa
    reader.skip(14*4); //ocean
    reader.skip(33*4+13*2); //render shadowmap
    reader.skip(60*4); //sand effect

    auto tone_control = reader.read<unsigned short>();
    assume(tone_control == 1);
    tone_saturation = reader.read<float>();

    reader.skip(11*4); //debug

    detail.mesh_power = reader.read<float>();
    detail.mesh_range = reader.read<float>();
    detail.mesh_repeat = reader.read<float>();

    reader.skip(8*4); //map parts

    reader.skip(18*4); //sky weather

    sky.fog_density = reader.read<float>();
    sky.fog_height = reader.read<float>();
    sky.fog_height_density = reader.read<float>();
    sky.fog_height_fade_density = reader.read<float>();
    sky.fog_height_fresnel = reader.read<float>();

    for (int i = 0; i < 2; ++i)
    {
        auto &s = i ? sky.low : sky.high;

        s.ambient = reader.read_color3();
        s.lens_brightness = reader.read<float>();

        s.light2_color = reader.read_color3();
        s.light2_dir = reader.read_dir_py();
        s.light2_power = reader.read<float>();

        s.light3_color = reader.read_color3();
        s.light3_dir = reader.read_dir_py();
        s.light3_power = reader.read<float>();

        s.player_shadow_brightness = reader.read<float>();
        s.player_shadow_color = reader.read_color3();

        s.skysphere_intensity = reader.read<float>();
        s.specular_color = reader.read_color3();
        s.specular_power = reader.read<float>();

        s.sun_color = reader.read_color3();
        s.sun_power = reader.read<float>();
        s.sun_rgb = reader.read_color3();

        s.sun_flare_rgb = reader.read_color3();
        s.sun_flare_size = reader.read<float>();
    }

    reader.skip(9*4); //sky mapspecular mesh
    reader.skip(6*4); //sky mapspecular obj

    sky.mapspecular.parts_color = reader.read_color3();
    sky.mapspecular.parts_contrast = reader.read<float>();
    sky.mapspecular.parts_fog_power = reader.read<float>();
    sky.mapspecular.parts_fresnel = reader.read<float>();
    sky.mapspecular.parts_fresnel_max = reader.read<float>();
    sky.mapspecular.parts_power = reader.read<float>();
    sky.mapspecular.parts_reflection_power = reader.read<float>();
    sky.mapspecular.parts_scale = reader.read<float>();

    sky.moon_dir = reader.read_dir_py();
    sky.moon_color = reader.read_color3();
    sky.moon_size = reader.read<float>();

    sky.sun_dir = reader.read_dir_py();
    sky.sun_size = reader.read<float>();

    reader.skip(93*4+5*2); //tree

    assume(!reader.get_remained());

    return true;
}

//------------------------------------------------------------
