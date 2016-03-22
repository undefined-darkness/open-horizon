//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <zlib.h>

//------------------------------------------------------------

static bool unzip(const void *from, size_t from_size, void *to, size_t to_size)
{
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)from_size;
    infstream.next_in = (Bytef *)from;
    infstream.avail_out = (uInt)to_size;
    infstream.next_out = (Bytef *)to;

    inflateInit2(&infstream, -MAX_WBITS);

    const int result = inflate(&infstream, Z_NO_FLUSH);
    if (result != Z_OK && result != Z_STREAM_END)
        return false;

    if (infstream.total_in != from_size || infstream.total_out != to_size)
        return false;

    inflateEnd(&infstream);
    return true;
}

//------------------------------------------------------------
