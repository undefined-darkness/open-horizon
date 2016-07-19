//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <vector>

namespace renderer
{
//------------------------------------------------------------

class gim_decoder
{
public:
    gim_decoder(const void *data, size_t size); //inplace

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

    size_t get_required_size() const { return m_width * m_height * 4; }
    bool decode(void *buf) const;

private:
    unsigned short m_width, m_height;
    const unsigned char *m_indices4;
    const unsigned char *m_indices8;
    const unsigned int *m_colors;
    bool m_greyscale = false;

    union color
    {
        unsigned int u;
        struct { unsigned char b, g, r, a; };
    };

    color m_palette[256];
};

//------------------------------------------------------------
}
