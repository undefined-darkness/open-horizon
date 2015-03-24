//
// open horizon -- undefined_darkness@outlook.com
//

#include "location_params.h"
#include "resources/resources.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
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
    nya_memory::memory_reader reader(fi_data.get_data(), file_data->get_size());

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
    reader.skip(129*4); //sky
    reader.skip(93*4+5*2); //tree

    assert(!reader.get_remained());

    return true;
}

//------------------------------------------------------------
