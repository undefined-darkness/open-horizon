//
// open horizon -- undefined_darkness@outlook.com
//

//based on https://github.com/morkt/GARbro/blob/master/ArcFormats/Cri/AudioHCA.cs

#include "file.h"
#include "util/util.h"

namespace sound
{
//------------------------------------------------------------

static const int max_channels = 2; //ToDo?

//------------------------------------------------------------

class hsa_bit_stream
{
public:
    hsa_bit_stream(void *data, int size): m_data((unsigned char *)data), m_size(size) {}
    int get_bits(int count);
    void seek(int count);

private:
    unsigned char *m_data;
    int m_size, m_pos = 0;
    int m_bits = 0, m_cached_bits = 0;
};

//------------------------------------------------------------

int hsa_bit_stream::get_bits(int count)
{
    while (m_cached_bits < count)
    {
        if (m_pos >= m_size)
            return -1;

        const int b = *(m_data + m_pos++);
        m_bits = (m_bits << 8) | b;
        m_cached_bits += 8;
    }

    const int mask = (1 << count) - 1;
    m_cached_bits -= count;
    return (m_bits >> m_cached_bits) & mask;
}

//------------------------------------------------------------

void hsa_bit_stream::seek(int offset)
{
    if (offset > 0 && offset <= m_cached_bits)
    {
        m_cached_bits -= offset;
        return;
    }

    const int position = std::max(m_pos * 8 - m_cached_bits + offset, 0);
    m_cached_bits = 0;
    m_pos = position / 8;
    const int bit_pos = position & 7;
    if (bit_pos)
        get_bits(bit_pos);
}

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
    if (m_hca_data)
        return sizeof(hca_data::channel::samples) / sizeof(float) * m_hca_data->block_count / (m_hca_data->samples_per_second / 1000);

    if (m_cached)
        return m_cached->length;

    return 0;
}

//------------------------------------------------------------

unsigned int file::get_loop_start() const
{
    if (m_hca_data)
        return sizeof(hca_data::channel::samples) / sizeof(float) * m_hca_data->loop_start / (m_hca_data->samples_per_second / 1000);

    if (m_cached)
        return m_cached->loop_start;

    return 0;
}

//------------------------------------------------------------

unsigned int file::get_freq() const
{
    if (m_hca_data)
        return m_hca_data->samples_per_second;

    if (m_cached)
        return m_cached->freq;

    return 0;
}

//------------------------------------------------------------

bool file::is_stereo() const
{
    if (m_hca_data)
        return m_hca_data->channels.size() > 1;

    if (m_cached)
        return m_cached->channels > 1;

    return 0;
}

//------------------------------------------------------------

size_t file::get_buf_size() const
{
    if (m_hca_data)
        return std::min((int)m_hca_data->channels.size(), max_channels) * sizeof(hca_data::channel::samples) / sizeof(float) * sizeof(uint16_t);

    return 0;
}

//------------------------------------------------------------

inline short pack_sample16(float f)
{
    const int s = (int)(f * 0x7FFF);

    if (s > 0x7FFF)
        return 0x7FFF;

    if (s < -0x7FFF)
        return -0x7FFF;

    return (short)s;
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

        short *sdata = (short *)data;

        hsa_bit_stream bits(&d.raw_data[buf_idx * d.block_size], d.block_size);
        d.decode(bits);

        int count = 1024;
        if (buf_idx + 1 == d.block_count)
            count = d.loop_fine;

		for (int i = 0; i < count; ++i)
        {
            for (int k = 0; k < (int)d.channels.size() && k < max_channels; ++k)
                *sdata = pack_sample16(((float *)(d.channels[k].samples))[i]), ++sdata;
		}

        return (char *)sdata - (char *)data;
    }

    return 0;
}

//------------------------------------------------------------

bool file::cache(const as_create_buf &c, const as_free_buf &f)
{
    if (m_cached)
        return true;

    if (!c)
        return false;

    if (m_hca_data)
    {
        auto &d = *m_hca_data.get();

        nya_memory::tmp_buffer_scoped buf(d.block_count * get_buf_size());

        unsigned int offset = 0;
        for (unsigned int i = 0; i < d.block_count; ++i)
            offset += cache_buf(buf.get_data(offset), i, false);

        auto id = c(buf.get_data(), offset);
        if (!id)
            return false;

        m_cached = std::shared_ptr<cached>(new cached());
        m_cached->id = id;
        m_cached->channels = std::min((int)d.channels.size(), max_channels);
        m_cached->freq = get_freq();
        m_cached->length = get_length();
        m_cached->loop_start = get_loop_start();
        m_cached->on_free = f;
        m_hca_data.reset();
        return true;
    }

    return false;
}

//------------------------------------------------------------

unsigned int file::get_cached_id() const { return m_cached ? m_cached->id : 0; }

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

        switch (signature)
        {
            case 'fmt\0':
            {
                const uint32_t format = read_uint32(r);
                const auto channels = format >> 24;
                if (channels > 16)
                {
                    m_hca_data.reset();
                    return false;
                }

                d.channels.resize(channels);
                d.samples_per_second = format & 0xFFFFFF;
                d.block_count = read_uint32(r);
                r.skip(4);
            }
            continue;

            case 'dec\0':
            {
                d.block_size = read_uint16(r);
                const uint8_t unknown_1 = read_uint8(r);
                assume(unknown_1 == 1);
                const uint8_t unknown_15 = read_uint8(r);
                assume(unknown_15 == 15);
                const uint8_t count = read_uint8(r);
                const uint8_t count2 = read_uint8(r);
                assume(count2 == 0);
                const uint8_t step = read_uint8(r);
                assume(step == 0 || step == 1 || step == 3);
                const uint8_t u = read_uint8(r);
                assume(u == 0);

                if (d.channels.empty())
                {
                    m_hca_data.reset();
                    return false;
                }

                for (auto &c: d.channels)
                    c.count = count + 1;
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
                r.skip(2);
                d.loop_fine = read_uint16(r);
                d.loop_fine /= 2; //Dunno
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

    if (!d.loop_end)
    {
        d.loop_end = d.block_count;
        d.loop_fine = 1024;
    }

    r.seek(data_offset);
    const auto data_size = d.block_count * d.block_size;
    assert(r.check_remained(data_size));
    //assume(r.get_remained() == data_size);

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

void file::hca_data::decode(hsa_bit_stream &bits)
{
	if (bits.get_bits(16) != 0xFFFF)
        return;

    int t = bits.get_bits(9) << 8;
    t -= bits.get_bits(7);

    for (auto &c: channels)
        c.decode1(bits, t);

    for (int i = 0; i < 8; ++i)
    {
        for (auto &c: channels)
            c.decode2(bits);

        for (int j = 0; j < (int)channels.size() && j < max_channels; ++j)
            channels[j].decode5(i);
    }
}

//------------------------------------------------------------

void file::hca_data::channel::decode1(hsa_bit_stream &bits, int t)
{
    const int v = bits.get_bits(3);
    if (v >= 6)
    {
        for (int i = 0; i < count; ++i)
            value[i] = bits.get_bits(6);

    }
    else if (v)
    {
        int v1 = bits.get_bits(6);
        const int v2 = (1 << v) - 1;
        const int v3 = v2 >> 1;
        value[0] = v1;

        for (int i = 1; i < count; ++i)
        {
            const int v4 = bits.get_bits(v);
            if (v4 != v2)
                v1 += v4 - v3;
            else
                v1 = bits.get_bits(6);
            value[i] = (char)v1;
        }
    }
    else
        memset(value, 0, sizeof(value));

    static const unsigned char scale_index[] =
    {
        0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0D, 0x0D,
        0x0D, 0x0D, 0x0D, 0x0D, 0x0C, 0x0C, 0x0C, 0x0C,
        0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
        0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x09,
        0x09, 0x09, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x07, 0x06, 0x06, 0x05, 0x04,
        0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    for (int i = 0; i < count; ++i)
    {
        int v = value[i];
        if (v)
        {
            v = ((t + i) >> 8) - ((v * 5) >> 1) + 1;
            if (v < 0)
                v = 15;
            else if (v >= 0x39)
                v = 1;
            else
                v = scale_index[v];
        }
        scale[i] = v;
    }

    memset(scale + count, 0, sizeof(scale) - count);

    static const float value_table[] =
    {
        1.588383e-7f, 2.116414e-7f, 2.819978e-7f, 3.757431e-7f, 5.006523e-7f, 6.670855e-7f, 8.888464e-7f, 1.184328e-6f,
        1.578037e-6f, 2.102628e-6f, 2.80161e-6f, 3.732956e-6f, 4.973912e-6f, 6.627403e-6f, 8.830567e-6f, 1.176613e-5f,
        1.567758e-5f, 2.088932e-5f, 2.783361e-5f, 3.708641e-5f, 4.941514e-5f, 6.584233e-5f, 8.773047e-5f, 0.0001168949f,
        0.0001557546f, 0.0002075325f, 0.0002765231f, 0.0003684484f, 0.0004909326f, 0.0006541346f, 0.0008715902f, 0.001161335f,
        0.001547401f, 0.002061807f, 0.002747219f, 0.003660484f, 0.004877347f, 0.006498737f, 0.008659128f, 0.0115377f,
        0.01537321f, 0.02048377f, 0.02729324f, 0.0363664f, 0.04845578f, 0.06456406f, 0.08602725f, 0.1146255f,
        0.1527307f, 0.2035034f, 0.2711546f, 0.3612952f, 0.4814015f,  0.641435f, 0.8546689f,  1.138789f,
        1.517359f,  2.021779f,  2.693884f,  3.589418f,  4.782658f,  6.372569f,  8.491017f,  11.31371f
    };

    static const float scale_table[] =
    {
        0, 2.0f/3, 2.0f/5, 2.0f/7, 2.0f/9, 2.0f/11, 2.0f/13, 2.0f/15,
        2.0f/31, 2.0f/63, 2.0f/127, 2.0f/255, 2.0f/511, 2.0f/1023, 2.0f/2047, 2.0f/4095
    };

    for (int i = 0; i < count; ++i)
        base[i] = value_table[value[i]] * scale_table[scale[i]];
}

//------------------------------------------------------------

void file::hca_data::channel::decode2(hsa_bit_stream &bits)
{
    static const char table1[] = { 0, 2, 3, 3, 4, 4, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

    static const char table2[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 2, 2, 2, 2, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
    };

    static const float table3[] =
    {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  1, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  1,  1, -1, -1,  2, -2,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  1, -1,  2, -2,  3, -3,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  1,  1, -1, -1,  2,  2, -2, -2,  3,  3, -3, -3,  4, -4,
        0,  0,  1,  1, -1, -1,  2,  2, -2, -2,  3, -3,  4, -4,  5, -5,
        0,  0,  1,  1, -1, -1,  2, -2,  3, -3,  4, -4,  5, -5,  6, -6,
        0,  0,  1, -1,  2, -2,  3, -3,  4, -4,  5, -5,  6, -6,  7, -7
    };

    for (int i = 0; i < count; ++i)
    {
        float f;
        const int s = scale[i];
        const int bit_size = table1[s];
        int v = bits.get_bits(bit_size);
        if (s < 8)
        {
            v += s << 4;
            bits.seek(table2[v] - bit_size);
            f = table3[v];
        }
        else
        {
            v = (1 - ((v & 1) << 1)) * (v >> 1);
            if (!v)
                bits.seek(-1);
            f = (float)v;
        }

        block[i] = base[i] * f;
    }

    memset(block + count, 0, sizeof(block) - count * sizeof(float));
}

//------------------------------------------------------------

void file::hca_data::channel::decode5(int index)
{
    float *src = block;
    float *dst = sample1;
    for (int i = 0; i < 7; ++i)
    {
        int count1 = 1 << i;
        int count2 = 0x40 >> i;
        int d1 = 0;
        int d2 = count2;
        int s = 0;
        for (int j = 0; j < count1; ++j)
        {
            for (int k = 0; k < count2; ++k)
            {
                float a = src[s++];
                float b = src[s++];
                dst[d1++] = a + b;
                dst[d2++] = a - b;
            }
            d1 += count2;
            d2 += count2;
        }

        std::swap(src, dst);
    }

    static const float table[7][0x40] =
    {
        {
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f,
            0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f, 0.08166019f
        },
        {
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f,
            0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f, 0.9807853f, 0.8314696f
        },
        {
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f,
            0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f, 0.9951847f, 0.9569404f, 0.8819213f, 0.7730104f
        },
        {
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f,
            0.9987954f, 0.9891765f, 0.9700313f, 0.9415441f, 0.9039893f, 0.8577286f, 0.8032075f, 0.7409511f
        },
        {
            0.9996988f, 0.9972904f, 0.9924796f, 0.9852777f, 0.9757021f, 0.9637761f, 0.9495282f, 0.9329928f,
            0.9142098f, 0.8932243f,  0.870087f, 0.8448536f, 0.8175848f, 0.7883464f, 0.7572088f, 0.7242471f,
            0.9996988f, 0.9972904f, 0.9924796f, 0.9852777f, 0.9757021f, 0.9637761f, 0.9495282f, 0.9329928f,
            0.9142098f, 0.8932243f,  0.870087f, 0.8448536f, 0.8175848f, 0.7883464f, 0.7572088f, 0.7242471f,
            0.9996988f, 0.9972904f, 0.9924796f, 0.9852777f, 0.9757021f, 0.9637761f, 0.9495282f, 0.9329928f,
            0.9142098f, 0.8932243f,  0.870087f, 0.8448536f, 0.8175848f, 0.7883464f, 0.7572088f, 0.7242471f,
            0.9996988f, 0.9972904f, 0.9924796f, 0.9852777f, 0.9757021f, 0.9637761f, 0.9495282f, 0.9329928f,
            0.9142098f, 0.8932243f,  0.870087f, 0.8448536f, 0.8175848f, 0.7883464f, 0.7572088f, 0.7242471f
        },
        {
            0.9999247f, 0.9993224f, 0.9981181f, 0.9963126f,  0.993907f, 0.9909027f, 0.9873014f, 0.9831055f,
            0.9783174f,   0.97294f, 0.9669765f, 0.9604305f,  0.953306f, 0.9456073f,  0.937339f, 0.9285061f,
            0.9191139f,  0.909168f, 0.8986745f, 0.8876396f, 0.8760701f, 0.8639728f, 0.8513552f, 0.8382247f,
            0.8245893f, 0.8104572f, 0.7958369f, 0.7807372f, 0.7651672f, 0.7491364f, 0.7326543f, 0.7157308f,
            0.9999247f, 0.9993224f, 0.9981181f, 0.9963126f,  0.993907f, 0.9909027f, 0.9873014f, 0.9831055f,
            0.9783174f,   0.97294f, 0.9669765f, 0.9604305f,  0.953306f, 0.9456073f,  0.937339f, 0.9285061f,
            0.9191139f,  0.909168f, 0.8986745f, 0.8876396f, 0.8760701f, 0.8639728f, 0.8513552f, 0.8382247f,
            0.8245893f, 0.8104572f, 0.7958369f, 0.7807372f, 0.7651672f, 0.7491364f, 0.7326543f, 0.7157308f
        },
        {
            0.9999812f, 0.9998306f, 0.9995294f, 0.9990777f, 0.9984756f,  0.997723f, 0.9968203f, 0.9957674f,
            0.9945646f, 0.9932119f, 0.9917098f, 0.9900582f, 0.9882576f, 0.9863081f, 0.9842101f, 0.9819639f,
            0.9795698f, 0.9770281f, 0.9743394f, 0.9715039f, 0.9685221f, 0.9653944f, 0.9621214f, 0.9587035f,
            0.9551412f,  0.951435f, 0.9475856f, 0.9435934f, 0.9394592f, 0.9351835f, 0.9307669f, 0.9262102f,
            0.921514f, 0.9166791f,  0.911706f, 0.9065957f, 0.9013488f, 0.8959662f, 0.8904487f, 0.8847971f,
            0.8790122f,  0.873095f, 0.8670462f,  0.860867f,  0.854558f, 0.8481203f,  0.841555f, 0.8348629f,
            0.8280451f, 0.8211025f, 0.8140363f, 0.8068476f, 0.7995372f, 0.7921066f, 0.7845566f, 0.7768885f,
            0.7691033f, 0.7612024f, 0.7531868f, 0.7450578f, 0.7368166f, 0.7284644f, 0.7200025f, 0.7114322f
        }
    };

    static const float table2[7][0x40]=
    {
        {
            -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f,
            0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,
            0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,
            -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f,
            0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,
            -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f,
            -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f,
            0.03382476f, -0.03382476f, -0.03382476f,  0.03382476f, -0.03382476f,  0.03382476f,  0.03382476f, -0.03382476f
        },
        {
            -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,  0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f,
            0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f, -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,
            0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f, -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,
            -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,  0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f,
            0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f, -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,
            -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,  0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f,
            -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f,  0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f,
            0.1950903f,  0.5555702f, -0.1950903f, -0.5555702f, -0.1950903f, -0.5555702f,  0.1950903f,  0.5555702f
        },
        {
            -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,  0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f,
            0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f, -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,
            0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f, -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,
            -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,  0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f,
            0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f, -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,
            -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,  0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f,
            -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f,  0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f,
            0.09801714f,  0.2902847f,  0.4713967f,  0.6343933f, -0.09801714f, -0.2902847f, -0.4713967f, -0.6343933f
        },
        {
            -0.04906768f, -0.1467305f, -0.2429802f, -0.3368899f, -0.4275551f, -0.5141028f, -0.5956993f, -0.671559f,
            0.04906768f,  0.1467305f,  0.2429802f,  0.3368899f,  0.4275551f,  0.5141028f,  0.5956993f,  0.671559f,
            0.04906768f,  0.1467305f,  0.2429802f,  0.3368899f,  0.4275551f,  0.5141028f,  0.5956993f,  0.671559f,
            -0.04906768f, -0.1467305f, -0.2429802f, -0.3368899f, -0.4275551f, -0.5141028f, -0.5956993f, -0.671559f,
            0.04906768f,  0.1467305f,  0.2429802f,  0.3368899f,  0.4275551f,  0.5141028f,  0.5956993f,  0.671559f,
            -0.04906768f, -0.1467305f, -0.2429802f, -0.3368899f, -0.4275551f, -0.5141028f, -0.5956993f, -0.671559f,
            -0.04906768f, -0.1467305f, -0.2429802f, -0.3368899f, -0.4275551f, -0.5141028f, -0.5956993f, -0.671559f,
            0.04906768f,  0.1467305f,  0.2429802f,  0.3368899f,  0.4275551f,  0.5141028f,  0.5956993f,  0.671559f
        },
        {
            -0.02454123f, -0.07356457f, -0.1224107f, -0.1709619f, -0.2191012f, -0.2667128f, -0.3136818f, -0.3598951f,
            -0.4052413f,  -0.4496113f,  -0.4928982f, -0.5349976f, -0.5758082f, -0.6152316f, -0.6531729f, -0.6895406f,
            0.02454123f,  0.07356457f,  0.1224107f,  0.1709619f,  0.2191012f,  0.2667128f,  0.3136818f,  0.3598951f,
            0.4052413f,   0.4496113f,   0.4928982f,  0.5349976f,  0.5758082f,  0.6152316f,  0.6531729f,  0.6895406f,
            0.02454123f,  0.07356457f,  0.1224107f,  0.1709619f,  0.2191012f,  0.2667128f,  0.3136818f,  0.3598951f,
            0.4052413f,   0.4496113f,   0.4928982f,  0.5349976f,  0.5758082f,  0.6152316f,  0.6531729f,  0.6895406f,
            -0.02454123f, -0.07356457f, -0.1224107f, -0.1709619f, -0.2191012f, -0.2667128f, -0.3136818f, -0.3598951f,
            -0.4052413f,  -0.4496113f,  -0.4928982f, -0.5349976f, -0.5758082f, -0.6152316f, -0.6531729f, -0.6895406f
        },
        {
            -0.01227154f,-0.03680722f, -0.06132074f,-0.08579731f, -0.1102222f, -0.1345807f, -0.1588582f, -0.1830399f,
            -0.2071114f, -0.2310581f,  -0.2548656f, -0.2785197f,  -0.3020059f, -0.3253103f, -0.3484187f, -0.3713172f,
            -0.393992f,  -0.4164295f,  -0.4386162f, -0.4605387f,  -0.4821838f, -0.5035384f, -0.5245897f, -0.545325f,
            -0.5657318f, -0.5857978f,  -0.6055111f, -0.6248595f,  -0.6438316f, -0.6624158f, -0.680601f,  -0.6983762f,
            0.01227154f, 0.03680722f,  0.06132074f, 0.08579731f,  0.1102222f,  0.1345807f,  0.1588582f,  0.1830399f,
            0.2071114f,  0.2310581f,   0.2548656f,  0.2785197f,   0.3020059f,  0.3253103f,  0.3484187f,  0.3713172f,
            0.393992f,   0.4164295f,   0.4386162f,  0.4605387f,   0.4821838f,  0.5035384f,  0.5245897f,  0.545325f,
            0.5657318f,  0.5857978f,   0.6055111f,  0.6248595f,   0.6438316f,  0.6624158f,  0.680601f,   0.6983762f
        },
        {
            -0.006135885f, -0.01840673f, -0.0306748f, -0.04293826f, -0.05519525f, -0.06744392f, -0.07968244f, -0.09190895f,
            -0.1041216f, -0.1163186f, -0.1284981f, -0.1406582f, -0.1527972f, -0.1649131f, -0.1770042f, -0.1890687f,
            -0.2011046f, -0.2131103f, -0.2250839f, -0.2370236f, -0.2489276f, -0.2607941f, -0.2726214f, -0.2844075f,
            -0.2961509f, -0.3078496f, -0.319502f,  -0.3311063f, -0.3426607f, -0.3541635f, -0.365613f,  -0.3770074f,
            -0.388345f,  -0.3996242f, -0.4108432f, -0.4220003f, -0.4330938f, -0.4441221f, -0.4550836f, -0.4659765f,
            -0.4767992f, -0.4875502f, -0.4982277f, -0.5088301f, -0.519356f,  -0.5298036f, -0.5401714f, -0.550458f,
            -0.5606616f, -0.5707808f, -0.5808139f, -0.5907597f, -0.6006165f, -0.6103828f, -0.6200572f, -0.6296383f,
            -0.6391245f, -0.6485144f, -0.6578067f, -0.6669999f, -0.6760927f, -0.6850837f, -0.6939715f, -0.7027547f
        }
    };

    src = sample1;
    dst = block;
    for (int i = 0; i < 7; ++i)
    {
        int count1 = 0x40 >> i;
        int count2 = 1 << i;
        int l1 = 0;
        int l2 = 0;
        int s1 = 0;
        int s2 = count2;
        int d1 = 0;
        int d2 = count2 * 2 - 1;
        for (int j = 0; j < count1; ++j)
        {
            for (int k = 0; k < count2; ++k)
            {
                const float a = src[s1++];
                const float b = src[s2++];
                const float c = table[i][l1++];
                const float d = table2[i][l2++];
                dst[d1++] = a * c - b * d;
                dst[d2--] = a * d + b * c;
            }
            s1 += count2;
            s2 += count2;
            d1 += count2;
            d2 += count2 * 3;
        }

        std::swap(src, dst);
    }

    for (size_t i = 0; i < 0x80; ++i)
        sample2[i] = src[i];

    static const float table3[] =
    {
        0.0006905338f, 0.001976235f, 0.003673865f, 0.00572424f, 0.008096703f, 0.01077318f, 0.01374252f, 0.01699786f,
        0.02053526f, 0.0243529f, 0.02845052f, 0.03282909f, 0.03749062f, 0.0424379f, 0.04767443f, 0.0532043f,
        0.05903211f, 0.06516288f, 0.07160201f, 0.07835522f, 0.08542849f, 0.09282802f, 0.1005602f, 0.1086314f,
        0.1170481f,  0.125817f, 0.1349443f, 0.1444365f, 0.1542995f, 0.1645391f, 0.1751607f, 0.1861692f,
        0.1975687f,  0.209363f, 0.2215546f, 0.2341454f,  0.247136f, 0.2605258f, 0.2743127f, 0.2884932f,
        0.3030619f, 0.3180117f, 0.3333333f, 0.3490153f, 0.3650438f, 0.3814027f, 0.3980731f, 0.4150335f,
        0.4322598f,  0.449725f, 0.4673996f, 0.4852512f, 0.5032449f, 0.5213438f, 0.5395085f, 0.5576978f,
        0.5758689f,  0.593978f, 0.6119806f, 0.6298314f,  0.647486f, 0.6649002f, 0.6820312f, 0.6988376f,

        -0.7152804f, -0.7313231f, -0.7469321f, -0.7620773f, -0.7767318f, -0.7908728f, -0.8044813f, -0.817542f,
        -0.8300441f, -0.8419802f, -0.8533467f, -0.8641438f, -0.8743748f, -0.8840462f, -0.8931671f, -0.9017491f,
        -0.9098061f, -0.9173537f, -0.924409f,  -0.9309903f, -0.937117f,  -0.942809f,  -0.9480868f, -0.9529709f,
        -0.9574819f, -0.9616405f, -0.9654669f, -0.9689808f, -0.9722016f, -0.975148f,  -0.977838f,  -0.980289f,
        -0.9825177f, -0.9845399f, -0.9863706f, -0.9880241f, -0.9895141f, -0.9908532f, -0.9920534f, -0.9931263f,
        -0.9940821f, -0.994931f,  -0.9956822f, -0.9963443f, -0.9969255f, -0.9974333f, -0.9978746f, -0.9982561f,
        -0.9985837f, -0.9988629f, -0.9990991f, -0.999297f,  -0.999461f,  -0.9995952f, -0.9997034f, -0.9997891f,
        -0.9998555f, -0.9999056f, -0.9999419f, -0.9999672f, -0.9999836f, -0.9999933f, -0.999998f,  -0.9999998f
    };

    int s = 0;
    int w = 0;
    int s1 = 0x40;
    int s2 = 0;
    for (int i = 0; i < 0x40; ++i)
        samples[index][w++] = sample2[s1++] * table3[s++] + sample3[s2++];
    for (int i = 0; i < 0x40; ++i)
        samples[index][w++] = table3[s++] * sample2[--s1] - sample3[s2++];

    s1 = 0x3F;
    s2 = 0;
    for (int i = 0; i < 0x40; ++i)
        sample3[s2++] = sample2[s1--] * table3[--s];
    for (int i = 0; i < 0x40; ++i)
        sample3[s2++] = sample2[++s1] * table3[--s];
}

//------------------------------------------------------------
}
