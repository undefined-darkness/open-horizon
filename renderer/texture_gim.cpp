//
// open horizon -- undefined_darkness@outlook.com
//

#include "texture_gim.h"
#include "util/util.h"

namespace renderer
{
//------------------------------------------------------------

gim_decoder::gim_decoder(const void *data, size_t size): m_indices4(0), m_indices8(0), m_colors(0), m_width(0), m_height(0)
{
    nya_memory::memory_reader r(data, size);

    struct gim_header
    {
        uint32_t sign;
        uint32_t unknown;
        uint32_t unknown2[2];
        uint8_t unknown3[3], format;
        uint32_t unknown4;
        uint32_t unknown5;
        uint16_t width, height;

    } const header = r.read<gim_header>();

    if (header.sign != '\0MIG')
        return;

    if (header.format == 19  && header.width * header.height == r.get_remained())
    {
        m_indices8 = (unsigned char *)r.get_data();
        m_greyscale = true;
    }
    else if (header.format == 19 || header.format == 27)
    {
        m_indices8 = (unsigned char *)r.get_data();

        if (!r.skip(header.width * header.height + 16))
            return;

        if (r.get_offset() % 16 > 0)
            r.skip(16 - r.get_offset() % 16);

        if (r.get_remained() < sizeof(m_palette))
            return;

        auto pal = (color *)r.get_data();
        for (int i = 0; i < 256; i+=32)
        {
            memcpy(&m_palette[i], &pal[i], 32);
            memcpy(&m_palette[i + 16], &pal[i + 8], 32);
            memcpy(&m_palette[i + 8], &pal[i + 16], 32);
            memcpy(&m_palette[i + 24], &pal[i + 24], 32);
        }

        for(auto &p: m_palette)
            p.a = p.a > 127 ? 255 : p.a * 2;
    }
    else if(header.format == 20)
    {
        m_indices4 = (unsigned char *)r.get_data();
        if (!r.skip(header.width * header.height / 2 + 16))
            return;

        long test = r.get_remained();

        if (r.get_offset() % 16 > 0)
            r.skip(16 - r.get_offset() % 16);

        if (r.get_remained() < sizeof(m_palette) / 16)
            return;

        memcpy(m_palette, r.get_data(), sizeof(m_palette) / 16);

        for(auto &p: m_palette)
            p.a = p.a > 127 ? 255 : p.a * 2;
    }
    else if(header.format == 0)
    {
        if (r.get_remained() < header.width * header.height)
            return;

        m_colors = (unsigned int *)r.get_data();
    }
    else
    {
        printf("Error: unknown gim texture format: %d\n", header.format);
        return;
    }

    m_width = header.width;
    m_height = header.height;
}

//------------------------------------------------------------

bool gim_decoder::decode(void *buf) const
{
    if (!buf || !get_required_size())
        return false;

    if (m_greyscale)
    {
        auto cbuf = (color *)buf;
        for (size_t i = 0; i < m_width * m_height; ++i)
            cbuf[i].r = cbuf[i].g = cbuf[i].b = m_indices8[i], cbuf[i].a = 255;

        return true;
    }

    if (m_indices8)
    {
        auto ibuf = (unsigned int *)buf;
        for (size_t i = 0; i < m_width * m_height; ++i)
            ibuf[i] = m_palette[m_indices8[i]].u;

        return true;
    }

    if (m_indices4)
    {
        for (size_t i = 0; i < m_width * m_height / 2; ++i)
        {
            auto pi = m_indices4[i];
            ((unsigned int *)buf)[i * 2] = m_palette[pi & 0x0F].u;
            ((unsigned int *)buf)[i * 2 + 1] = m_palette[pi >> 4].u;
        }

        return true;
    }

    memcpy(buf, m_colors, get_required_size());
    return true;
}

//------------------------------------------------------------
}
