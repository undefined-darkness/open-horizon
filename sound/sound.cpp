//
// open horizon -- undefined_darkness@outlook.com
//

#include "containers/cpk.h"
#include "scene/camera.h"
#include "sound.h"
#include "util/util.h"

#include "AL/al.h"
#include "AL/alc.h"

namespace sound
{
//------------------------------------------------------------

static float min_distance = 10.0f;
static float max_distance = 5000.0f;

//------------------------------------------------------------

class context
{
public:
    static ALCcontext *get() {return get_instance().m_context; }
    static bool valid() { return get() != 0; }

    static void release()
    {
        auto &c = get_instance();
        if (c.m_context)
        {
            alcMakeContextCurrent(NULL);
            alcDestroyContext(c.m_context);
            c.m_context = 0;
        }

        if (c.m_device)
        {
            alcCloseDevice(c.m_device);
            c.m_device = 0;
        }
    }

private:
    static context &get_instance() { static context c; return c; }

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
        {
            alcCloseDevice(m_device);
            return;
        }

        alcMakeContextCurrent(m_context);
        alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
        alDopplerFactor(0.1f);
    }

private:
    ALCdevice *m_device = 0;
    ALCcontext *m_context = 0;
};

//------------------------------------------------------------

void release_context() { context::release(); }

//------------------------------------------------------------

void world_2d::set_music(const file &f)
{
    if (!m_music.init(f, m_music_volume, true))
        return;

    alSourcei(m_music.id, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(m_music.id, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(m_music.id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------

static void load_refs(int idx, const cri_utf_table::column &c, std::vector<uint16_t> &ids)
{
    assert(idx<(int)c.values.size());
    auto inds = (uint16_t *)c.values[idx].d.data();
    int count = (int)c.values[idx].d.size() / (sizeof(uint16_t) * 2);
    for (int i = 0; i < count; ++i)
    {
        auto type = swap_bytes(*inds++);
        auto ind = swap_bytes(*inds++);
        if (type == 2)
            load_refs((int)ind, c, ids);
        else if (type == 1)
            ids.push_back(ind);
    }
}

//------------------------------------------------------------

static void load_cues(const cri_utf_table &t, std::vector<pack::cue> &cues)
{
    cri_utf_table cnt(t.get_value("CueNameTable").d);
    auto &ct_cn = cnt.get_column("CueName");
    auto &ct_ci = cnt.get_column("CueIndex");

    cri_utf_table ct(t.get_value("CueTable").d);
    auto &ct_id = ct.get_column("CueId");
    //auto &ct_rt = ct.get_column("ReferenceType");
    auto &ct_ri = ct.get_column("ReferenceIndex");

    cri_utf_table st(t.get_value("SynthTable").d);
    auto &st_r = st.get_column("ReferenceItems");

    assert(ct_ci.values.size() == ct_id.values.size());

    cues.resize(ct_id.values.size());

    for (int i = 0; i < (int)cues.size(); ++i)
    {
        assert(ct_ci.values[i].u < cues.size());
        cues[ct_ci.values[i].u].name = ct_cn.values[i].s;

        //assert(ct_id.values[i].u < cues.size());
        //auto &c = cues[ct_id.values[i].u];
        auto &c = cues[i];

        load_refs((int)ct_ri.values[i].u, st_r, c.wave_ids);
    }
}

//------------------------------------------------------------

void world_2d::set_music(const std::string &name)
{
    stop_music();
    if (name.empty())
        return;

    cpk_file base;
    if (!base.open("target/DATA10.PAC"))
        return;

    auto *res = access(base, 0);
    if (!res)
    {
        base.close();
        return;
    }

    static std::vector<pack::cue> cues;
    static std::vector<int> waveform_remap;
    if (cues.empty())
    {
        auto *res1 = access(base, 1);
        nya_memory::tmp_buffer_scoped buf1(res1->get_size());
        res1->read_all(buf1.get_data());
        cri_utf_table t(buf1.get_data(), buf1.get_size());
        load_cues(t, cues);

        cri_utf_table wt(t.get_value("WaveformTable").d);
        auto &wt_id = wt.get_column("Id");
        waveform_remap.resize(wt_id.values.size());
        for (int i = 0; i < (int)waveform_remap.size(); ++i)
            waveform_remap[i] = (int)wt_id.values[i].u;
    }

    int idx = -1;

    for (auto &c: cues)
    {
        if (c.name == name)
        {
            if (!c.wave_ids.empty())
                idx = c.wave_ids[0];
            break;
        }
    }

    if (idx < 0 || idx >= (int)waveform_remap.size())
        return;

    idx = waveform_remap[idx];

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

void world_2d::set_music_volume(float volume)
{
    m_music_volume = volume;
    if (m_music.id > 0)
        alSourcef(m_music.id, AL_GAIN, volume);
}

//------------------------------------------------------------

void world_2d::stop_sounds()
{
    for (auto &s: m_sounds)
        s.release();

    m_sounds.clear();
}

//------------------------------------------------------------

unsigned int world_2d::play_ui(file &f, float volume, bool loop)
{
    if (!loop)
        cache(f);

    sound_src s;
    if (!s.init(f, volume, loop))
        return 0;

    alSourcei(s.id, AL_SOURCE_RELATIVE, AL_TRUE);
    alSource3f(s.id, AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(s.id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);

    m_sounds.push_back(s);
    return s.id;
}

//------------------------------------------------------------

void world_2d::stop_ui(unsigned int id)
{
    if (!id)
        return;

    for (auto &s: m_sounds)
    {
        if (s.id == id)
            s.release();
    }
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
    if (!id)
        return;

    if (muted == mute)
        return;

    muted = mute;

    if (mute)
    {
        alSourceStop(id);
        return;
    }

    alSourcePlay(id);
    if (!stream)
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

void world::stop_sounds()
{
    world_2d::stop_sounds();

    for (auto &s: m_sounds_3d)
        s.src.set_mute(true);

    for (auto &s: m_sound_sources)
        s.first.set_mute(true);
}

//------------------------------------------------------------

void world::play(file &f, vec3 pos, float volume)
{
    f.limit_channels(1);
    cache(f);

    sound_src s;
    if (!s.init(f, volume, false))
        return;

    nya_math::vec3 vel;
    alSourcefv(s.id, AL_POSITION,  &pos.x);
    alSourcefv(s.id, AL_VELOCITY, &vel.x);
    alSourcei(s.id, AL_REFERENCE_DISTANCE, min_distance);
    alSourcei(s.id, AL_MAX_DISTANCE, max_distance);
    m_sounds_3d.push_back({pos, s});
}

//------------------------------------------------------------

source_ptr world::add(file &f, bool loop)
{
    f.limit_channels(1);

    if (!loop) //ToDo: OpenAL doesn't support loop points
        cache(f);

    sound_src s;
    if (s.init(f, 0.0f, loop))
    {
        alSourcei(s.id, AL_REFERENCE_DISTANCE, min_distance);
        alSourcei(s.id, AL_MAX_DISTANCE, max_distance);
    }

    m_sound_sources.push_back({s, source_ptr(new source())});
    return m_sound_sources.back().second;
}

//------------------------------------------------------------

void world::set_volume(float volume)
{
    alListenerf(AL_GAIN, volume);
}

//------------------------------------------------------------

void world::update(int dt)
{
    if (!context::valid())
        return;

    world_2d::update(dt);

    const auto c = nya_scene::get_camera();

    const vec3 &listener_pos = c.get_pos();

    alListenerfv(AL_POSITION, &listener_pos.x);

    const vec3 listener_vel = (c.get_pos() - m_prev_pos) / (dt * 0.001f);
    m_prev_pos = c.get_pos();
    alListenerfv(AL_VELOCITY, &listener_vel.x);

    const vec3 ori[] = { c.get_rot().rotate_inv(-vec3::forward()), c.get_rot().rotate_inv(vec3::up())};
    alListenerfv(AL_ORIENTATION, &ori[0].x);

    for (auto &s: m_sounds_3d)
    {
        s.src.update(dt);
        s.src.set_mute((s.pos-listener_pos).length_sq() > max_distance * max_distance);
    }

    for (auto &s: m_sound_sources)
    {
        s.first.update(dt);

        if (s.second.unique())
        {
            s.first.release();
            continue;
        }

        alSourcef(s.first.id, AL_PITCH, s.second->pitch);
        alSourcef(s.first.id, AL_GAIN, s.second->volume);
        alSourcefv(s.first.id, AL_POSITION,  &s.second->pos.x);
        alSourcefv(s.first.id, AL_VELOCITY, &s.second->vel.x);

        s.first.set_mute((s.second->pos-listener_pos).length_sq() > max_distance * max_distance);
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

bool pack::has(const std::string &name)
{
    for (auto &c: cues) if (c.name == name) return true;
    return false;
}

//------------------------------------------------------------

file &pack::get(const std::string &name, int idx)
{
    for (auto &c: cues)
    {
        if (c.name != name)
            continue;

        if (idx < 0 || idx >= c.wave_ids.size())
            break;

        auto id = c.wave_ids[idx];
        if (id >= waves.size())
            break;

        return waves[id];
    }

    return nya_memory::invalid_object<file>();
}

//------------------------------------------------------------

bool pack::load(const std::string &name)
{
    auto r = load_resource(name.c_str());
    cri_utf_table t(r.get_data(), r.get_size());
    r.free();

    struct data_adaptor: public nya_resources::resource_data
    {
        const std::vector<char> &data;
        data_adaptor(const std::vector<char> & d): data(d) {}
        size_t get_size() override { return data.size(); }
        bool read_chunk(void *d,size_t size,size_t offset=0) override
        {
            return d && (offset + size <= data.size()) && memcpy(d, &data[offset], size) != 0;
        }
    } da(t.get_value("AwbFile").d);

    cpk_file cpk;
    if (!cpk.open(&da))
        return false;

    cri_utf_table wt(t.get_value("WaveformTable").d);
    auto &wt_id = wt.get_column("Id");
    //assume((int)wt_id.values.size() == cpk.get_files_count());

    for (auto &v: wt_id.values)
    {
        nya_memory::tmp_buffer_scoped buf(cpk.get_file_size((int)v.u));
        cpk.read_file_data((int)v.u, buf.get_data());
        sound::file f;
        if (f.load(buf.get_data(), buf.get_size()))
            waves.push_back(f);
    }

    cpk.close();

    load_cues(t, cues);

    return true;
}

//------------------------------------------------------------
}
