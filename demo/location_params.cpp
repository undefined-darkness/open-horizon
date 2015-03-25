//
// open horizon -- undefined_darkness@outlook.com
//

#include "location_params.h"
#include "resources/resources.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include "math/quaternion.h"
#include "math/constants.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

//------------------------------------------------------------

bool location_params::load(const char *file_name)
{
    *this = location_params(); //reset if was loaded already

    auto *file_data = nya_resources::get_resources_provider().access(file_name);
    if (!file_data)
    {
        printf("unable to open file %s\n", file_name ? file_name : "<invalid>");
        return false;
    }

    nya_memory::tmp_buffer_scoped fi_data(file_data->get_size());
    file_data->read_all(fi_data.get_data());

    class memory_reader: public nya_memory::memory_reader
    {
    public:
        location_params::color3 read_color3()
        {
            location_params::color3 c;
            c.z = read<float>();
            c.y = read<float>();
            c.x = read<float>();
            return c / 255.0;
        }

        location_params::color4 read_color4()
        {
            location_params::color4 c;
            c.w = read<float>();
            c.z = read<float>();
            c.y = read<float>();
            c.x = read<float>();
            return c / 255.0;
        }

        location_params::vec3 read_dir_py()
        {
            float pitch = read<float>() * nya_math::constants::pi / 180.0f;
            float yaw = read<float>() * nya_math::constants::pi / 180.0f;

            return nya_math::quat(pitch,yaw,0.0f).rotate(nya_math::vec3(0.0,1.0,0.0));
        }
        
        memory_reader(const void *data, size_t size): nya_memory::memory_reader(data, size) {}

    } reader(fi_data.get_data(), file_data->get_size());

    clipping_plane.ground_zfar = reader.read<float>();
    clipping_plane.ground_znear = reader.read<float>();
    clipping_plane.zfar = reader.read<float>();
    clipping_plane.znear = reader.read<float>();

    reader.skip(54*4); //clouds
    reader.skip(10*4+3*2); //hdr
    reader.skip(2*4+5*2); //mlaa
    reader.skip(14*4); //ocean
    reader.skip(33*4+13*2); //render shadowmap
    reader.skip(60*4); //sand effect
    reader.skip(2+4); //tone
    reader.skip(11*4); //debug
    reader.skip(3*4); //detail
    reader.skip(8*4); //map parts

    reader.skip(18*4); //sky weather

    sky.fog_density = reader.read<float>();
    sky.fog_height = reader.read<float>();
    sky.fog_height_density = reader.read<float>();
    sky.fog_height_fade_density = reader.read<float>();
    sky.fog_height_fresnel = reader.read<float>();

    reader.skip(36*4); //sky high
    reader.skip(36*4); //sky low
    reader.skip(25*4); //sky mapspecular

    sky.moon_dir = reader.read_dir_py();
    sky.moon_color = reader.read_color3();
    sky.moon_size = reader.read<float>();

    sky.sun_dir = reader.read_dir_py();
    sky.sun_size = reader.read<float>();

    reader.skip(93*4+5*2); //tree

    assert(!reader.get_remained());

    return true;
}

//------------------------------------------------------------
