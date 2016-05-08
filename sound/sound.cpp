//
// open horizon -- undefined_darkness@outlook.com
//

#include "containers/cpk.h"
#include "scene/camera.h"
#include "sound.h"
#include "util/util.h"

#ifdef __APPLE__
    #include <OpenAL/al.h>
    #include <OpenAL/alc.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>
#endif

namespace sound
{
//------------------------------------------------------------

class context
{
public:
    static ALCcontext *get()
    {
        static context c;
        return c.m_context;
    }

    static bool valid() { return get() != 0; }

private:
    context()
    {
        m_device = alcOpenDevice(NULL);
        if (!m_device)
        {
            printf("OpenAL error: default sound device not present");
            return;
        }

        m_context = alcCreateContext(m_device, NULL);
        if (!m_context)
            return;

        alcMakeContextCurrent(m_context);
    }
/*
    ~context()
    {
        if (m_context)
        {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(m_context);
            m_context = 0;
        }

        if (m_device)
        {
            alcCloseDevice(m_device);
            m_device = 0;
        }
    }
*/
private:
    ALCdevice *m_device = 0;
    ALCcontext *m_context = 0;
};

//------------------------------------------------------------

void world_2d::set_music(const file &f)
{
    m_music.init(f, 1.0f, true);
}

//------------------------------------------------------------

void world_2d::set_music(int idx)
{
    stop_music();
    if (idx < 0)
        return;

    cpk_file base;
    if (!base.open("DATA10.PAC"))
        return;

    auto *res = access(base, 0);
    if (!res)
    {
        base.close();
        return;
    }

    cpk_file files;
    if (!files.open(res) || idx >= files.get_files_count())
    {
        base.close();
        res->release();
        return;
    }

    nya_memory::tmp_buffer_scoped buf(files.get_file_size(idx));
    files.read_file_data(idx, buf.get_data());

    base.close();
    res->release();

    file f;
    if (!f.load(buf.get_data(), buf.get_size()))
        return;

    set_music(f);
}

//------------------------------------------------------------

void world_2d::stop_music()
{
    m_music.release();
}

//------------------------------------------------------------

void world_2d::play(const file &f, float volume)
{
    sound_src s;
    if (!s.init(f, volume, false))
        return;

    m_sounds.push_back(s);
}

//------------------------------------------------------------

std::vector<char> world_2d::sound_src::cache_buf;

//------------------------------------------------------------

bool world_2d::sound_src::init(const file &f, float volume, bool loop)
{
    if (!context::valid())
        return false;

    release();

    alGenSources(1, &id);
    if (!id)
        return false;

    alSourcef(id, AL_PITCH, 1.0f);
    alSourcef(id, AL_GAIN, volume);
    this->loop = loop;
    this->f = f;

    auto cached_id = f.get_cached_id();
    if (cached_id)
    {
        alSourcei(id, AL_BUFFER, cached_id);
        alSourcei(id, AL_LOOPING, loop);
        alSourcePlay(id);
        return true;
    }

    stream = true;
    format = f.is_stereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

    if (cache_buf.size() < f.get_buf_size())
        cache_buf.resize(f.get_buf_size(), 0);

    last_buf_idx = 0;
    while (last_buf_idx < cache_count)
    {
        const size_t size = f.cache_buf(cache_buf.data(), last_buf_idx, loop);
        if (!size)
            break;

        unsigned int buf_id = 0;
        alGenBuffers(1, &buf_id);
        alBufferData(buf_id, format, cache_buf.data(), (ALsizei)size, f.get_freq());
        alSourceQueueBuffers(id, 1, &buf_id);
        buf_ids[last_buf_idx] = buf_id;
        ++last_buf_idx;
    }

    if (last_buf_idx)
        alSourcePlay(id);

    return true;
}

//------------------------------------------------------------

void world_2d::sound_src::update(int dt)
{
    if (!id)
        return;

    if (!stream)
    {
        time += dt;
        if (time >= f.get_length())
        {
            if (loop && f.get_length() > 0)
                time %= f.get_length();
            else
                release();
        }
        return;
    }

    int processed_bufs = 0;
    alGetSourcei(id, AL_BUFFERS_PROCESSED, &processed_bufs);

    const auto prev_buf_idx = last_buf_idx;

    while (processed_bufs--)
    {
        unsigned int buf_id = 0;
        alSourceUnqueueBuffers(id, 1, &buf_id);

        const size_t size = f.cache_buf(cache_buf.data(), last_buf_idx, loop);
        if (!size)
            continue;

        alBufferData(buf_id, format, cache_buf.data(), (ALsizei)size, f.get_freq());
        alSourceQueueBuffers(id, 1, &buf_id);
        ++last_buf_idx;
    }

    ALint state;
    alGetSourcei(id, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING)
    {
        const bool has_bufs = last_buf_idx != prev_buf_idx;
        if (has_bufs)
            alSourcePlay(id);
        else
            release();
    }
}

//------------------------------------------------------------

void world_2d::sound_src::set_mute(bool mute)
{
    if (!id || stream)
        return;

    if (mute)
    {
        alSourceStop(id);
        return;
    }

    alSourcePlay(id);
    alSourcef(id, AL_SEC_OFFSET, time * 0.001f);
}

//------------------------------------------------------------

void world_2d::sound_src::release()
{
    if (!id)
        return;

    alSourceStop(id);
    alDeleteSources(1, &id);
    id = 0;
    alDeleteBuffers(cache_count, buf_ids);
    for (auto &b: buf_ids) b = 0;
}

//------------------------------------------------------------

void world::play(file &f, vec3 pos, float volume)
{
    cache(f);

    sound_src s;
    if (!s.init(f, volume, false))
        return;

    nya_math::vec3 vel;
    alSourcefv(s.id, AL_POSITION,  &pos.x);
    alSourcefv(s.id, AL_VELOCITY, &vel.x);
    m_sounds.push_back(s);
}

//------------------------------------------------------------

source_ptr world::add(file &f, bool loop)
{
    cache(f);

    sound_src s;
    s.init(f, 0.0, loop);
    m_sound_sources.push_back({s, source_ptr(new source())});
    return m_sound_sources.back().second;
}

//------------------------------------------------------------

void world_2d::update(int dt)
{
    if (!context::valid())
        return;

    m_music.update(dt);
    for (auto &s: m_sounds)
        s.update(dt);

    m_sounds.erase(std::remove_if(m_sounds.begin(), m_sounds.end(), [](const sound_src &s) { return !s.id; }), m_sounds.end());
}

//------------------------------------------------------------

void world::update(int dt)
{
    if (!context::valid())
        return;

    world_2d::update(dt);

    const auto c = nya_scene::get_camera();
    alListenerfv(AL_POSITION, &c.get_pos().x);

    const vec3 vel = (c.get_pos() - m_prev_pos) / (dt * 0.001f);
    m_prev_pos = c.get_pos();
    alListenerfv(AL_VELOCITY, &vel.x);

    const vec3 ori[] = { c.get_rot().rotate(vec3::forward()), c.get_rot().rotate(vec3::up())};
    alListenerfv(AL_ORIENTATION, &ori[0].x);

    //ToDo: mute far sounds

    for (auto &s: m_sound_sources)
    {
        s.first.update(dt);

        if (s.second.unique())
        {
            s.first.release();
            continue;
        }

        alSourcef (s.first.id, AL_PITCH, s.second->pitch);
        alSourcef (s.first.id, AL_GAIN, s.second->volume);
        alSourcefv(s.first.id, AL_POSITION,  &s.second->pos.x);
        alSourcefv(s.first.id, AL_VELOCITY, &s.second->vel.x);
    }

    m_sound_sources.erase(std::remove_if(m_sound_sources.begin(), m_sound_sources.end(),
                                         [](const std::pair<sound_src, source_ptr> &s) { return !s.first.id; }), m_sound_sources.end());
}

//------------------------------------------------------------

bool cache(file &f)
{
    if (!context::valid())
        return false;

    return f.cache([f](const void *data, size_t size)
                   {
                       unsigned int id;
                       alGenBuffers(1, &id);
                       alBufferData(id, f.is_stereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16, data, (ALsizei)size, f.get_freq());
                       return id;
                   },
                   [](unsigned int id){ if (context::valid()) alDeleteBuffers(1, &id); });
}

//------------------------------------------------------------

bool pack::load(const char *name)
{
    auto r = load_resource(name);
    cri_utf_table t(r.get_data(), r.get_size());
    r.free();

    for (auto &c: t.columns)
    {
        if (c.name == "AwbFile")
        {
            if (c.values.size() != 1)
                return false;

            struct data_adaptor: public nya_resources::resource_data
            {
                const std::vector<char> &data;
                data_adaptor(std::vector<char> & d): data(d) {}
                size_t get_size() override { return data.size(); }
                bool read_chunk(void *d,size_t size,size_t offset=0) override
                {
                    return d && (offset + size <= data.size()) && memcpy(d, &data[offset], size) != 0;
                }
            } da(c.values.front().d);

            cpk_file cpk;
            cpk.open(&da);

            files.reserve(cpk.get_files_count());
            for (int i = 0; i < cpk.get_files_count(); ++i)
            {
                nya_memory::tmp_buffer_scoped buf(cpk.get_file_size(i));
                cpk.read_file_data(i, buf.get_data());
                sound::file f;
                if (f.load(buf.get_data(), buf.get_size()))
                    files.push_back(f);
            }

            cpk.close();
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------
}
