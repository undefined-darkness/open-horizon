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
    void stop_music();

    void play(const file &f, float volume);

    virtual void update(int dt);

protected:
    struct sound_src
    {
        bool init(const file &f, float volume, bool loop);
        void update();
        void release();

        file f;
        unsigned int id = 0;

        sound_src() { for (auto &b: buf_ids) b = 0; }

    private:
        bool loop = false;
        int format = 0;
        unsigned int last_buf_idx = 0;
        static std::vector<char> cache_buf;

        static const int cache_count = 2;
        unsigned int buf_ids[cache_count];
    };

    sound_src m_music;
    std::list<sound_src> m_sounds;
};

//------------------------------------------------------------

class world: public world_2d
{
public:
    source_ptr add(const file &f, bool loop);
    void play(const file &f, vec3 pos, float volume);

    void update(int dt) override;

protected:
    nya_math::vec3 m_prev_pos;
    std::list<std::pair<sound_src, source_ptr> > m_sound_sources;
};

//------------------------------------------------------------
}
