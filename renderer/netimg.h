//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "memory/tmp_buffer.h"
#include "render/bitmap.h"

namespace renderer
{

struct netimg
{
    static const int width = 128, height = width;
    static const int color_size = width * height * 6 / 4;
    static const int alpha_size = width * height / 2;
    static const int raw_size = width * height * 4;

    unsigned char buf[color_size + alpha_size];

    netimg() { *this = netimg(0, 0, 0, 0); }

    netimg(int src_width, int src_height, int src_channels, const void *src_data)
    {
        if (!src_data)
        {
            memset(buf, 0, color_size + alpha_size);
            return;
        }

        typedef unsigned char uchar;

        nya_memory::tmp_buffer_scoped tmp(src_width * src_height);
        bool first = true;
        while (src_width >= width * 2 && src_height >= height * 2)
        {
            if (first)
            {
                nya_render::bitmap_downsample2x((uchar *)src_data, src_width, src_height, src_channels, (uchar *)tmp.get_data());
                src_data = tmp.get_data();
                first = false;
            }
            else
                nya_render::bitmap_downsample2x((uchar *)src_data, src_width, src_height, src_channels);
            src_width /= 2, src_height /= 2;
        }

        nya_memory::tmp_buffer_ref tmp2;

        if(src_width != width || src_height != height)
        {
            tmp2.allocate(width * height * src_channels);
            const uchar *src = (uchar *)src_data;
            uchar *dst = (uchar *)tmp2.get_data();
            const int x_ratio = ((src_width - 1) << 16) / width;
            const int y_ratio = ((src_height - 1) << 16) / height;
            long int y = 0;
            for (int i = 0; i < height; ++i)
            {
                const int yr = int(y >> 16);
                const long int y_diff = y - (yr << 16);
                const long int one_min_y_diff = 0x10000 - y_diff;
                const int y_index = yr * src_width;
                long int x = 0;
                for (int j = 0; j < width; ++j)
                {
                    const int xr = int(x >> 16);
                    const long int x_diff = x - (xr << 16);
                    const long int one_min_x_diff = 0x10000 - x_diff;
                    int idx = (y_index + xr) * src_channels;
                    for (int k = 0; k < src_channels; ++k, ++idx)
                    {
                        *dst++ = (uchar)((src[idx] * one_min_x_diff * one_min_y_diff +
                                          src[idx + src_channels] * x_diff * one_min_y_diff +
                                          src[idx + src_width * src_channels] * y_diff * one_min_x_diff +
                                          src[idx + (src_width + 1) * src_channels] * x_diff * y_diff) >> 32);
                    }
                    x += x_ratio;
                }
                y += y_ratio;
            }
            src_data = tmp2.get_data();
        }

        nya_render::bitmap_rgb_to_yuv420((uchar *)src_data, width, height, src_channels, buf);
        if (src_channels == 4)
        {
            uchar *src_a = (uchar *)src_data + 3;
            uchar *buf_a = buf + color_size;
            for (size_t i = 0; i < width * height * 4; i += 8, ++buf_a)
                *buf_a = ((src_a[i] / 0x11) << 4) | (src_a[i + 4] / 0x11);
        }
        else
        {
            for (size_t i = color_size; i < color_size + alpha_size; ++i)
                buf[i] = 0xff;
        }

        tmp2.free();
    }

    void decode(void *data) const
    {
        auto udata = (unsigned char *)data;
        nya_render::bitmap_yuv420_to_rgb(buf, width, height, udata);
        nya_render::bitmap_rgb_to_rgba(udata, width, height, 0xff, udata);

        udata += 3;
        for (size_t i = color_size; i < color_size + alpha_size; ++i)
        {
            *udata = ((buf[i] & 0xf0) >> 4) * 0x11, udata += 4;
            *udata = (buf[i] & 0x0f) * 0x11, udata += 4;
        }
    }
};

}
