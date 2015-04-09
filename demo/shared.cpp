//
// open horizon -- undefined_darkness@outlook.com
//

#include "shared.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include <assert.h>

namespace shared
{

//------------------------------------------------------------

namespace
{
    std::map<unsigned int,nya_scene::texture> textures;
}

//------------------------------------------------------------

void add_texture(unsigned int hash_id, const nya_scene::texture &tex)
{
    textures[hash_id] = tex;
}

//------------------------------------------------------------

unsigned int load_texture(const char *name)
{
    auto *res = nya_resources::get_resources_provider().access(name);
    if (!res)
        return 0;

    size_t size = res->get_size();
    nya_memory::tmp_buffer_scoped buf(size);
    res->read_all(buf.get_data());
    res->release();

    return load_texture(buf.get_data(), size);
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
/*
    if(hash_id > 1000000000) //ToDo
    {
        unsigned int *mip_count = (unsigned int *)reader.get_data()+7;
        assert(*mip_count > 10);
        *mip_count = 5;
    }
*/
    nya_scene::shared_texture st;
    nya_scene::resource_data data(reader.get_remained());
    data.copy_from(reader.get_data(), reader.get_remained());
    nya_scene::texture::load_dds(st, data, "");
    data.free();
    nya_scene::texture tex;
    tex.create(st);
    shared::add_texture(hash_id, tex);

    return hash_id;
}

//------------------------------------------------------------

void clear_textures()
{
    textures.clear();
}

//------------------------------------------------------------

const nya_scene::texture &get_texture(unsigned int hash_id)
{
    auto tex = textures.find(hash_id);
    //assert(tex != textures.end());

    return tex->second;
}

const nya_scene::texture &get_black_texture()
{
    static nya_scene::texture black;
    static bool initialised=false;
    if(!initialised)
    {
        const unsigned char data[4]={0,0,0,0};
        nya_scene::shared_texture res;
        res.tex.build_texture(data,1,1,nya_render::texture::color_rgba);
        black.create(res);
        initialised=true;
    }

    return black;
}

//------------------------------------------------------------

nya_memory::tmp_buffer_ref load_resource(const char *name)
{
    nya_resources::resource_data *res = nya_resources::get_resources_provider().access(name);
    if (!res)
        return nya_memory::tmp_buffer_ref();

    nya_memory::tmp_buffer_ref buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();
    
    return buf;
}

//------------------------------------------------------------

}
