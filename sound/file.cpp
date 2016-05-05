//
// open horizon -- undefined_darkness@outlook.com
//

//based on https://github.com/morkt/GARbro/blob/master/ArcFormats/Cri/AudioHCA.cs

#include "file.h"
#include "util/util.h"

namespace sound
{
//------------------------------------------------------------

bool file::load(const void *data, size_t size)
{
    m_hca_data.reset();

    if (!data || !size)
        return false;

    if (load_hca(data, size))
        return true;

    //ToDo: other formats

    return false;
}

//------------------------------------------------------------

unsigned int file::get_length() const
{
    return 0; //ToDo
}

//------------------------------------------------------------

unsigned int file::get_freq() const
{
    if (m_hca_data)
        return m_hca_data->samples_per_second;

    return 0;
}

//------------------------------------------------------------

bool file::is_stereo() const
{
    if (m_hca_data)
        return m_hca_data->channels > 1;

    return 0;
}

//------------------------------------------------------------

size_t file::get_buf_size() const
{
    return 0; //ToDo
}

//------------------------------------------------------------

size_t file::cache_buf(void *data, unsigned int buf_idx, bool loop) const
{
    if (m_hca_data)
    {
        auto &d = *m_hca_data.get();

        if (buf_idx >= d.block_count)
        {
            if (loop && d.loop_end > d.loop_start)
                buf_idx = (buf_idx - d.loop_start) % (d.loop_end - d.loop_start) + d.loop_start;
            else
                return 0;
        }

        char *cdata = (char *)data;

        //ToDo: decode

        return cdata - (char *)data;
    }

    return 0;
}

//------------------------------------------------------------

inline uint32_t read_sign(nya_memory::memory_reader &r) { return swap_bytes(r.read<uint32_t>()) & 0x7F7F7F7F; }
inline uint32_t read_uint32(nya_memory::memory_reader &r) { return swap_bytes(r.read<uint32_t>()); }
inline uint16_t read_uint16(nya_memory::memory_reader &r) { return swap_bytes(r.read<uint16_t>()); }
inline uint8_t read_uint8(nya_memory::memory_reader &r) { return r.read<uint8_t>(); }
inline float read_float(nya_memory::memory_reader &r) { return swap_bytes(r.read<float>()); }

//------------------------------------------------------------

bool file::load_hca(const void *data, size_t size)
{
    nya_memory::memory_reader r(data, size);
    if (read_sign(r) != 'HCA\0')
        return false;

    m_hca_data = std::shared_ptr<hca_data>(new hca_data());
    auto &d = *m_hca_data.get();

    d.version = read_uint16(r);
    auto data_offset = read_uint16(r);
    while (r.get_offset() < data_offset)
    {
        auto signature = read_sign(r);
        if(!signature)
            break;

        //auto psign = swap_bytes(signature); printf("sign %.4s\n", (char*)&psign);

        switch (signature)
        {
            case 'fmt\0':
            {
                const uint32_t format = read_uint32(r);
                d.channels = format >> 24;
                d.samples_per_second = format & 0xFFFFFF;
                d.block_count = read_uint32(r);
                r.skip(4);
            }
            continue;

            case 'dec\0':
            {
                d.block_size = read_uint16(r);
                r.skip(6); //ToDo
            }
            continue;

            case 'rva\0':
            {
                d.volume = read_float(r);
            }
            continue;

            case 'ath\0':
            {
                auto ath_type = read_uint16(r);
                assert(ath_type == 0);
            }
            continue;

            case 'loop':
                d.loop_start = read_uint32(r);
                d.loop_end = read_uint32(r) + 1;
                r.skip(4);
                continue;

            case 'ciph':
            {
                auto ciph_type = read_uint16(r);
                assert(ciph_type == 0);
            }
            continue;

            case 'pad\0': continue;
        }

        signature = swap_bytes(signature);
        printf("unknown section %.4s in HCA file\n", (char*)&signature);
        break;
    }

    r.seek(data_offset);

    if (d.channels > 2)
        d.channels = 2;

    if (!d.loop_end)
        d.loop_end = d.block_count;

    auto data_size = d.block_count * d.block_size;

    assert(r.check_remained(data_size));
    assume(r.get_remained() == data_size);

    if (!data_size || !r.check_remained(data_size))
    {
        m_hca_data.reset();
        return false;
    }

    d.raw_data.resize(data_size);
    memcpy(d.raw_data.data(), r.get_data(), data_size);

    return true;
}

//------------------------------------------------------------
}
