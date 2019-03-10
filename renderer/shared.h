//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"
#include "util/util.h"
#include <functional>

namespace shared
{
//------------------------------------------------------------

    unsigned int load_texture(const char *name,unsigned int aniso = 0);
    nya_scene::texture load_texture_nocache(const char *name,unsigned int aniso = 0);
    unsigned int load_texture(const void *data, size_t size,unsigned int aniso = 0);
    void clear_textures();
    const nya_scene::texture &get_texture(unsigned int hash_id);
    const nya_scene::texture &get_white_texture();
    const nya_scene::texture &get_black_texture();
    const nya_scene::texture &get_normal_texture();
    void set_loading_callback(std::function<void()> f);
    void update_loading();

//------------------------------------------------------------
}
