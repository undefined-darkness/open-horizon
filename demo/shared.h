//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"

namespace shared
{
//------------------------------------------------------------

    unsigned int load_texture(const char *name);
    unsigned int load_texture(const void *data, size_t size);
    void clear_textures();
    const nya_scene::texture &get_texture(unsigned int hash_id);
    nya_memory::tmp_buffer_ref load_resource(const char *name);

//------------------------------------------------------------
}
