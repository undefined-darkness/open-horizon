//
// open horizon -- undefined_darkness@outlook.com
//

#include "shared.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include "memory/invalid_object.h"
#include <assert.h>

namespace shared
{

//------------------------------------------------------------

namespace
{
    std::map<unsigned int, nya_scene::texture> textures;
    std::map<std::string, unsigned int> texture_names;
    std::function<void()> loading_callback;
}

//------------------------------------------------------------

void add_texture(unsigned int hash_id, const nya_scene::texture &tex)
{
    textures[hash_id] = tex;
}

//------------------------------------------------------------

unsigned int load_texture(const char *name)
{
    if (!name)
        return 0;

    auto it = texture_names.find(name);
    if (it != texture_names.end())
        return it->second;

    nya_memory::tmp_buffer_scoped buf(load_resource(name));
    if (!buf.get_data())
        return 0;

    unsigned int hash_id = load_texture(buf.get_data(), buf.get_size());
    texture_names[name] = hash_id;

    if(loading_callback)
        loading_callback();
    return hash_id;
}

//------------------------------------------------------------

nya_scene::texture load_texture_nocache(const char *name)
{
    nya_memory::tmp_buffer_scoped buf(load_resource(name));
    if (!buf.get_size())
        return 0;

    nya_memory::memory_reader reader(buf.get_data(),buf.get_size());
    reader.seek(48);
    reader.seek(reader.read<int>() + 16);
    if (reader.get_remained() < 128)
        return nya_scene::texture();

    nya_scene::shared_texture st;
    nya_scene::resource_data data(reader.get_remained());
    data.copy_from(reader.get_data(), reader.get_remained());
    nya_scene::texture::load_dds(st, data, "");
    data.free();

    nya_scene::texture tex;
    tex.create(st);

    if(loading_callback)
        loading_callback();
    return tex;
}

//------------------------------------------------------------

unsigned int load_texture(const void *tex_data, size_t tex_size)
{
    assert(tex_data && tex_size);

    nya_memory::memory_reader reader(tex_data,tex_size);
    reader.seek(48);
    int offset = reader.read<int>();
    reader.seek(offset + 8);
    auto hash_id = reader.read<unsigned int>();
    //printf("%d\n", hash_id);
    reader.seek(offset + 16);

    if (reader.get_remained() < 128) //normal for ntxr
        return hash_id;

    nya_scene::shared_texture st;
    nya_scene::resource_data data(reader.get_remained());
    data.copy_from(reader.get_data(), reader.get_remained());
    nya_scene::texture::load_dds(st, data, "");
    data.free();

    if (hash_id > 1000000000) //ToDo
        st.tex.set_aniso(16);

    nya_scene::texture tex;
    tex.create(st);
    shared::add_texture(hash_id, tex);

    if(loading_callback)
        loading_callback();
    return hash_id;
}

//------------------------------------------------------------

void clear_textures()
{
    textures.clear();
    texture_names.clear();
}

//------------------------------------------------------------

const nya_scene::texture &get_texture(unsigned int hash_id)
{
    auto tex = textures.find(hash_id);
    //assert(tex != textures.end());
    if (tex == textures.end())
        return nya_memory::invalid_object<nya_scene::texture>();

    return tex->second;
}

//------------------------------------------------------------

const nya_scene::texture &get_white_texture()
{
    static nya_scene::texture white;
    if (!white.get_width())
    {
        const unsigned char data[4]={255,255,255,255};
        nya_scene::shared_texture res;
        res.tex.build_texture(data,1,1,nya_render::texture::color_rgba);
        white.create(res);
    }

    return white;
}

//------------------------------------------------------------

const nya_scene::texture &get_black_texture()
{
    static nya_scene::texture black;
    if (!black.get_width())
    {
        const unsigned char data[4]={0,0,0,0};
        nya_scene::shared_texture res;
        res.tex.build_texture(data,1,1,nya_render::texture::color_rgba);
        black.create(res);
    }

    return black;
}

//------------------------------------------------------------

const nya_scene::texture &get_normal_texture()
{
    static nya_scene::texture normal;
    if (!normal.get_width())
    {
        const unsigned char data[4]={0,128,0,128};
        nya_scene::shared_texture res;
        res.tex.build_texture(data,1,1,nya_render::texture::color_rgba);
        normal.create(res);
    }

    return normal;
}

//------------------------------------------------------------

void set_loading_callback(std::function<void()> f)
{
    loading_callback=f;
}

//------------------------------------------------------------

}
