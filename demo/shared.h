//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"
#include <assert.h>

#define assume(expr) assert(expr) //like assert, but not critical

namespace shared
{
//------------------------------------------------------------

    unsigned int load_texture(const char *name);
    unsigned int load_texture(const void *data, size_t size);
    void clear_textures();
    const nya_scene::texture &get_texture(unsigned int hash_id);
    const nya_scene::texture &get_black_texture();
    nya_memory::tmp_buffer_ref load_resource(const char *name);

//------------------------------------------------------------
}
