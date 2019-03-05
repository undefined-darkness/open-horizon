//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"
#include "util/util.h"

namespace shared
{
//------------------------------------------------------------

    unsigned int load_texture(const char *name);
    nya_scene::texture load_texture_nocache(const char *name);
    unsigned int load_texture(const void *data, size_t size);
    void clear_textures();
    const nya_scene::texture &get_texture(unsigned int hash_id);
    const nya_scene::texture &get_white_texture();
    const nya_scene::texture &get_black_texture();
    const nya_scene::texture &get_normal_texture();
    void set_loading_callback(std::function<void()> f);

//------------------------------------------------------------
}
