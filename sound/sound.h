//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "file.h"
#include "math/vector.h"
#include <memory>
#include <list>

namespace sound
{
//------------------------------------------------------------

typedef nya_math::vec3 vec3;

//------------------------------------------------------------

struct source
{
    vec3 pos;
    vec3 vel;
    float pitch = 1.0f;
    float volume = 1.0f;
};

typedef std::shared_ptr<source> source_ptr;

//------------------------------------------------------------

class world_2d
{
public:
    void set_music(const file &f);
    void set_music(const std::string &name);
    void stop_music();

    unsigned int play_ui(file &f, float volume = 1.0f, bool loop = false);
    void stop_ui(unsigned int id);

    void update(int dt);

protected:
    struct sound_src
    {
        bool init(const file &f, float volume, bool loop);
        void update(int dt);
        void release();
        void set_mute(bool mute);

        file f;
        unsigned int id = 0;

        sound_src() { for (auto &b: buf_ids) b = 0; }

    private:
        bool loop = false;
        bool muted = false;
        bool stream = false;
        int format = 0;
        unsigned int time = 0;
        unsigned int last_buf_idx = 0;
        static std::vector<char> cache_buf;

        static const int cache_count = 16;
        unsigned int buf_ids[cache_count];
    };

    sound_src m_music;
    std::list<sound_src> m_sounds;
};

//------------------------------------------------------------

class world: public world_2d
{
public:
    source_ptr add(file &f, bool loop);
    void play(file &f, vec3 pos, float volume = 1.0f);

    void update(int dt);

    void stop_sounds();

protected:
    struct sound_3d
    {
        vec3 pos;
        sound_src src;
    };

    std::list<sound_3d> m_sounds_3d;

    nya_math::vec3 m_prev_pos;
    std::list<std::pair<sound_src, source_ptr> > m_sound_sources;
};

//------------------------------------------------------------

bool cache(file &f);

//------------------------------------------------------------

void release_context();

//------------------------------------------------------------

struct pack
{
    std::vector<sound::file> waves;

    struct cue
    {
        std::string name;
        std::vector<uint16_t> wave_ids;
    };

    std::vector<cue> cues;

    bool has(const std::string &name);
    file &get(const std::string &name, int idx = 0);

    bool load(const std::string &name);
};

//------------------------------------------------------------
}
