//
// ported from https://github.com/Force67/rpftool with XBOX360 define
//

#pragma once

//------------------------------------------------------------

inline void DecodeDXT1(const uint8_t *data, uint8_t *pixData, int width, int height)
{
    typedef uint8_t byte;
    typedef uint16_t ushort;
    typedef uint32_t uint;

    int xBlocks = width / 4;
    int yBlocks = height / 4;
    for (int y = 0; y < yBlocks; y++)
    {
        for (int x = 0; x < xBlocks; x++)
        {
            int blockDataStart = ((y * xBlocks) + x) * 8;

            uint color0 = ((uint)data[blockDataStart + 0] << 8) + data[blockDataStart + 1];
            uint color1 = ((uint)data[blockDataStart + 2] << 8) + data[blockDataStart + 3];

            uint code = *(uint *)(data + blockDataStart + 4);

            ushort r0 = 0, g0 = 0, b0 = 0, r1 = 0, g1 = 0, b1 = 0;
            r0 = (ushort)(8 * (color0 & 31));
            g0 = (ushort)(4 * ((color0 >> 5) & 63));
            b0 = (ushort)(8 * ((color0 >> 11) & 31));

            r1 = (ushort)(8 * (color1 & 31));
            g1 = (ushort)(4 * ((color1 >> 5) & 63));
            b1 = (ushort)(8 * ((color1 >> 11) & 31));

            for (int k = 0; k < 4; k++)
            {
                int j = k ^ 1;

                for (int i = 0; i < 4; i++)
                {
                    int pixDataStart = (width * (y * 4 + j) * 4) + ((x * 4 + i) * 4);
                    uint codeDec = code & 0x3;

                    switch (codeDec)
                    {
                        case 0:
                            pixData[pixDataStart + 0] = (byte)r0;
                            pixData[pixDataStart + 1] = (byte)g0;
                            pixData[pixDataStart + 2] = (byte)b0;
                            pixData[pixDataStart + 3] = 255;
                            break;
                        case 1:
                            pixData[pixDataStart + 0] = (byte)r1;
                            pixData[pixDataStart + 1] = (byte)g1;
                            pixData[pixDataStart + 2] = (byte)b1;
                            pixData[pixDataStart + 3] = 255;
                            break;
                        case 2:
                            pixData[pixDataStart + 3] = 255;
                            if (color0 > color1)
                            {
                                pixData[pixDataStart + 0] = (byte)((2 * r0 + r1) / 3);
                                pixData[pixDataStart + 1] = (byte)((2 * g0 + g1) / 3);
                                pixData[pixDataStart + 2] = (byte)((2 * b0 + b1) / 3);
                            }
                            else
                            {
                                pixData[pixDataStart + 0] = (byte)((r0 + r1) / 2);
                                pixData[pixDataStart + 1] = (byte)((g0 + g1) / 2);
                                pixData[pixDataStart + 2] = (byte)((b0 + b1) / 2);
                            }
                            break;
                        case 3:
                            if (color0 > color1)
                            {
                                pixData[pixDataStart + 0] = (byte)((r0 + 2 * r1) / 3);
                                pixData[pixDataStart + 1] = (byte)((g0 + 2 * g1) / 3);
                                pixData[pixDataStart + 2] = (byte)((b0 + 2 * b1) / 3);
                                pixData[pixDataStart + 3] = 255;
                            }
                            else
                            {
                                pixData[pixDataStart + 0] = 0;
                                pixData[pixDataStart + 1] = 0;
                                pixData[pixDataStart + 2] = 0;
                                pixData[pixDataStart + 3] = 0;
                            }
                            break;
                    }

                    code >>= 2;
                }
            }


        }
    }
}

//------------------------------------------------------------

inline void DecodeDXT5(const uint8_t *data, uint8_t *pixData, int width, int height)
{
    typedef uint8_t byte;
    typedef uint16_t ushort;
    typedef uint32_t uint;

    int xBlocks = width / 4;
    int yBlocks = height / 4;
    for (int y = 0; y < yBlocks; y++)
    {
        for (int x = 0; x < xBlocks; x++)
        {
            int blockDataStart = ((y * xBlocks) + x) * 16;
            uint alphas[8];
            uint64_t alphaMask = 0;

            alphas[0] = data[blockDataStart + 1];
            alphas[1] = data[blockDataStart + 0];

            alphaMask |= data[blockDataStart + 6];
            alphaMask <<= 8;
            alphaMask |= data[blockDataStart + 7];
            alphaMask <<= 8;
            alphaMask |= data[blockDataStart + 4];
            alphaMask <<= 8;
            alphaMask |= data[blockDataStart + 5];
            alphaMask <<= 8;
            alphaMask |= data[blockDataStart + 2];
            alphaMask <<= 8;
            alphaMask |= data[blockDataStart + 3];

            // 8-alpha or 6-alpha block
            if (alphas[0] > alphas[1])
            {
                // 8-alpha block: derive the other 6
                // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                alphas[2] = (byte)((6 * alphas[0] + 1 * alphas[1] + 3) / 7);    // bit code 010
                alphas[3] = (byte)((5 * alphas[0] + 2 * alphas[1] + 3) / 7);    // bit code 011
                alphas[4] = (byte)((4 * alphas[0] + 3 * alphas[1] + 3) / 7);    // bit code 100
                alphas[5] = (byte)((3 * alphas[0] + 4 * alphas[1] + 3) / 7);    // bit code 101
                alphas[6] = (byte)((2 * alphas[0] + 5 * alphas[1] + 3) / 7);    // bit code 110
                alphas[7] = (byte)((1 * alphas[0] + 6 * alphas[1] + 3) / 7);    // bit code 111
            }
            else
            {
                // 6-alpha block.
                // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
                alphas[2] = (byte)((4 * alphas[0] + 1 * alphas[1] + 2) / 5);    // Bit code 010
                alphas[3] = (byte)((3 * alphas[0] + 2 * alphas[1] + 2) / 5);    // Bit code 011
                alphas[4] = (byte)((2 * alphas[0] + 3 * alphas[1] + 2) / 5);    // Bit code 100
                alphas[5] = (byte)((1 * alphas[0] + 4 * alphas[1] + 2) / 5);    // Bit code 101
                alphas[6] = 0x00;                                               // Bit code 110
                alphas[7] = 0xFF;                                               // Bit code 111
            }

            byte alpha[4][4];

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    alpha[j][i] = (byte)alphas[alphaMask & 7];
                    alphaMask >>= 3;
                }
            }

            ushort color0 = (ushort)((data[blockDataStart + 8] << 8) + data[blockDataStart + 9]);
            ushort color1 = (ushort)((data[blockDataStart + 10] << 8) + data[blockDataStart + 11]);

            uint code = *(uint *)(data + blockDataStart + 8 + 4);

            ushort r0 = 0, g0 = 0, b0 = 0, r1 = 0, g1 = 0, b1 = 0;
            r0 = (ushort)(8 * (color0 & 31));
            g0 = (ushort)(4 * ((color0 >> 5) & 63));
            b0 = (ushort)(8 * ((color0 >> 11) & 31));

            r1 = (ushort)(8 * (color1 & 31));
            g1 = (ushort)(4 * ((color1 >> 5) & 63));
            b1 = (ushort)(8 * ((color1 >> 11) & 31));

            for (int k = 0; k < 4; k++)
            {
                int j = k ^ 1;

                for (int i = 0; i < 4; i++)
                {
                    int pixDataStart = (width * (y * 4 + j) * 4) + ((x * 4 + i) * 4);
                    uint codeDec = code & 0x3;

                    pixData[pixDataStart + 3] = alpha[i][j];

                    switch (codeDec)
                    {
                        case 0:
                            pixData[pixDataStart + 0] = (byte)r0;
                            pixData[pixDataStart + 1] = (byte)g0;
                            pixData[pixDataStart + 2] = (byte)b0;
                            break;
                        case 1:
                            pixData[pixDataStart + 0] = (byte)r1;
                            pixData[pixDataStart + 1] = (byte)g1;
                            pixData[pixDataStart + 2] = (byte)b1;
                            break;
                        case 2:
                            if (color0 > color1)
                            {
                                pixData[pixDataStart + 0] = (byte)((2 * r0 + r1) / 3);
                                pixData[pixDataStart + 1] = (byte)((2 * g0 + g1) / 3);
                                pixData[pixDataStart + 2] = (byte)((2 * b0 + b1) / 3);
                            }
                            else
                            {
                                pixData[pixDataStart + 0] = (byte)((r0 + r1) / 2);
                                pixData[pixDataStart + 1] = (byte)((g0 + g1) / 2);
                                pixData[pixDataStart + 2] = (byte)((b0 + b1) / 2);
                            }
                            break;
                        case 3:
                            if (color0 > color1)
                            {
                                pixData[pixDataStart + 0] = (byte)((r0 + 2 * r1) / 3);
                                pixData[pixDataStart + 1] = (byte)((g0 + 2 * g1) / 3);
                                pixData[pixDataStart + 2] = (byte)((b0 + 2 * b1) / 3);
                            }
                            else
                            {
                                pixData[pixDataStart + 0] = 0;
                                pixData[pixDataStart + 1] = 0;
                                pixData[pixDataStart + 2] = 0;
                            }
                            break;
                    }

                    code >>= 2;
                }
            }
        }
    }
}

//------------------------------------------------------------

inline uint32_t XGAddress2DTiledX(uint32_t Offset, uint32_t Width, uint32_t TexelPitch)
{
    uint32_t AlignedWidth = (Width + 31) & ~31;

    uint32_t LogBpp = (TexelPitch >> 2) + ((TexelPitch >> 1) >> (TexelPitch >> 2));
    uint32_t OffsetB = Offset << LogBpp;
    uint32_t OffsetT = ((OffsetB & ~4095) >> 3) + ((OffsetB & 1792) >> 2) + (OffsetB & 63);
    uint32_t OffsetM = OffsetT >> (7 + LogBpp);

    uint32_t MacroX = ((OffsetM % (AlignedWidth >> 5)) << 2);
    uint32_t Tile = ((((OffsetT >> (5 + LogBpp)) & 2) + (OffsetB >> 6)) & 3);
    uint32_t Macro = (MacroX + Tile) << 3;
    uint32_t Micro = ((((OffsetT >> 1) & ~15) + (OffsetT & 15)) & ((TexelPitch << 3) - 1)) >> LogBpp;

    return Macro + Micro;
}

//------------------------------------------------------------

inline uint32_t XGAddress2DTiledY(uint32_t Offset, uint32_t Width, uint32_t TexelPitch)
{
    uint32_t AlignedWidth = (Width + 31) & ~31;

    uint32_t LogBpp = (TexelPitch >> 2) + ((TexelPitch >> 1) >> (TexelPitch >> 2));
    uint32_t OffsetB = Offset << LogBpp;
    uint32_t OffsetT = ((OffsetB & ~4095) >> 3) + ((OffsetB & 1792) >> 2) + (OffsetB & 63);
    uint32_t OffsetM = OffsetT >> (7 + LogBpp);

    uint32_t MacroY = ((OffsetM / (AlignedWidth >> 5)) << 2);
    uint32_t Tile = ((OffsetT >> (6 + LogBpp)) & 1) + (((OffsetB & 2048) >> 10));
    uint32_t Macro = (MacroY + Tile) << 3;
    uint32_t Micro = ((((OffsetT & (((TexelPitch << 6) - 1) & ~31)) + ((OffsetT & 15) << 1)) >> (3 + LogBpp)) & ~1);

    return Macro + Micro + ((OffsetT & 16) >> 4);
}

//------------------------------------------------------------

inline void ConvertFromLinearTexture(const char *from, char *to, uint32_t width, uint32_t height, bool dxt35)
{
    const uint32_t blockSize = 4;
    const uint32_t texelPitch = dxt35 ? 16 : 8;

    const uint32_t blockWidth = width / blockSize;
    const uint32_t blockHeight = height / blockSize;

    for (uint32_t j = 0; j < blockHeight; j++)
    {
        for (uint32_t i = 0; i < blockWidth; i++)
        {
            const uint32_t blockOffset = j * blockWidth + i;

            const uint32_t x = XGAddress2DTiledX(blockOffset, blockWidth, texelPitch);
            const uint32_t y = XGAddress2DTiledY(blockOffset, blockWidth, texelPitch);

            const uint32_t srcOffset = j * blockWidth * texelPitch + i * texelPitch;
            const uint32_t destOffset = y * blockWidth * texelPitch + x * texelPitch;

            memcpy(to + destOffset, from + srcOffset, texelPitch);
        }
    }
}

//------------------------------------------------------------
