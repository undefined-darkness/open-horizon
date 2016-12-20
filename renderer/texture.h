//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"
#include "util/util.h"

namespace renderer
{

static nya_scene::texture load_texture(nya_resources::resources_provider &prov, const char *name)
{
    nya_scene::texture tex;
    if (!name)
        return tex;

    auto data = load_resource(prov.access(name));
    if (!data.get_size())
    {
        if (nya_resources::check_extension(name, ".tga"))
        {
            std::string name_dds(name);
            name_dds = name_dds.substr(0, name_dds.size()-3) + "dds";
            data = load_resource(prov.access(name_dds.c_str()));
        }

        if (!data.get_size())
        {
            nya_log::log("unable to load texture: %s - not found\n", name);
            return tex;
        }
    }

    nya_scene::shared_texture stex;
    if (nya_scene::texture::load_dds(stex, data, name) || nya_scene::texture::load_tga(stex, data, name))
        tex.create(stex);
    else
    {
        nya_log::log("unable to load texture: %s - invalid format\n", name);
        return tex;
    }

    data.free();
    return tex;
}

}
