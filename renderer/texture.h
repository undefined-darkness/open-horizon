//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"
#include "render/render_api.h"
#include "util/util.h"
#include <vector>

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

struct texture_data
{
    unsigned int w = 0, h = 0;
    std::vector<uint8_t> rgba;
};

static texture_data load_texture_data(nya_resources::resources_provider &prov, const char *name)
{
    auto curr_api = nya_render::get_api_interface();
    
    class grab_api: nya_render::render_api_interface
    {
        uint get_max_texture_dimention() override { return m_prev_interface.get_max_texture_dimention(); }
        bool is_texture_format_supported(nya_render::texture::color_format format) override { return format == nya_render::texture::color_rgba; }

        int create_texture(const void *data,uint w,uint h,nya_render::texture::color_format &format,int mip_count) override
        {
            texture.w = w, texture.h = h;
            texture.rgba.resize(w * h * 4);
            if(data && format == nya_render::texture::color_rgba)
                memcpy(texture.rgba.data(), data, texture.rgba.size());
            return 0;
        }

        nya_render::render_api_interface &m_prev_interface;

    public:
        grab_api(): m_prev_interface(nya_render::get_api_interface()) { nya_render::set_render_api(this); }
        ~grab_api() { nya_render::set_render_api(&m_prev_interface); }

        texture_data texture;
    } grab_api;

    load_texture(prov, name);
    return grab_api.texture;
}

}
