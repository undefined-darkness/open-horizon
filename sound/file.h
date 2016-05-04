//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "file.h"
#include "clHCA.h"
#include <vector>
#include <memory>

namespace sound
{
//------------------------------------------------------------

class file
{
public:
    bool load(const void *data, size_t size);

    unsigned int get_length() const;
    unsigned int get_freq() const;
    bool is_stereo() const;

    size_t get_buf_size() const;
    size_t cache_buf(void *data, unsigned int buf_idx, bool loop) const; //returns actually cached size

private:
    bool load_hca(const void *data, size_t size);

    struct hca_data
    {
        uint16_t version = 0;
        uint8_t channels = 0;
        uint32_t samples_per_second = 0;
        uint32_t block_count = 0;
        uint16_t block_size = 0;
        std::vector<char> raw_data;
    };

    std::shared_ptr<hca_data> m_hca_data;
};

//------------------------------------------------------------
}
