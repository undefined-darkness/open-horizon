//
// GetTiledOffset from https://github.com/gildor2/UModel
//

#pragma once

//------------------------------------------------------------

inline unsigned GetTiledOffset(int x, int y, int alignedWidth, int logBpb)
{
    // top bits of coordinates
    int macro  = ((x >> 5) + (y >> 5) * (alignedWidth >> 5)) << (logBpb + 7);
    // lower bits of coordinates (result is 6-bit value)
    int micro  = ((x & 7) + ((y & 0xE) << 2)) << logBpb;
    // mix micro/macro + add few remaining x/y bits
    int offset = macro + ((micro & ~0xF) << 1) + (micro & 0xF) + ((y & 1) << 4);
    // mix bits again
    return (((offset & ~0x1FF) << 3) +                    // upper bits (offset bits [*-9])
            ((y & 16) << 7) +                            // next 1 bit
            ((offset & 0x1C0) << 2) +                    // next 3 bits (offset bits [8-6])
            (((((y & 8) >> 2) + (x >> 3)) & 3) << 6) +    // next 2 bits
            (offset & 0x3F)                                // lower 6 bits (offset bits [5-0])
            ) >> logBpb;
}

//------------------------------------------------------------

inline void UntileDXT(const char *from, char *to, uint32_t width, uint32_t height, bool dxt35)
{
    const uint32_t blockSize = 4;
    const uint32_t texelPitch = dxt35 ? 16 : 8;
    const uint32_t logBpp = (texelPitch >> 2) + ((texelPitch >> 1) >> (texelPitch >> 2));

    const uint32_t blockWidth = width / blockSize;
    const uint32_t blockHeight = height / blockSize;
    const uint32_t alignedWidth = (blockWidth + 31) & ~31;

    for (uint32_t j = 0; j < blockHeight; j++)
    {
        for (uint32_t i = 0; i < blockWidth; i++, to += texelPitch)
            memcpy(to, from + GetTiledOffset(i, j, alignedWidth, logBpp) * texelPitch, texelPitch);
    }
}

//------------------------------------------------------------
