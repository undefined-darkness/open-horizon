//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "scene/texture.h"

//------------------------------------------------------------

inline void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0)
{
    FILE *file = 0;
    if (fileName)
        file = fopen(fileName, "wb");

#define prnt(...) do{ if (file) fprintf(file, __VA_ARGS__); else printf(__VA_ARGS__); }while(0)

    prnt("\ndata at offset: %ld size: %ld\n", offset, size);

    nya_memory::memory_reader reader = const_reader;
    reader.seek(offset);
    if (size > reader.get_remained())
        size = reader.get_remained();

    bool had_zero = false;
    for (int i = 0; reader.get_offset() < offset+size; ++i)
    {
        int off = int(reader.get_offset());
        unsigned int t = reader.read<unsigned int>();/*
                                                      if (t == 0 && !substruct_size)
                                                      {
                                                      if (!had_zero)
                                                      {
                                                      prnt("\n");
                                                      had_zero = true;
                                                      }

                                                      continue;
                                                      }

                                                      had_zero = false;
                                                      */

        if (i * 4 == off)
            prnt( "%7d ", i * 4 );
        else
            prnt( "%7d %7d ", i * 4, off );

        float f =* ((float*)&t);
        unsigned short s[2];
        memcpy(s, &t, 4);

        char c[4];
        memcpy(c, &t, 4);

        //if ((fabs(f) < 0.001 && t != 0) || (fabs(f) > 1000.0f))
        //prnt( "%10u ", t);

        if (fabs(f) > 50000.0f)
            prnt( "           " );
        else
            prnt( "%10f ", f);
        
        prnt( "%10u ", t);
        
        prnt( "%6d %6d   ", s[0], s[1] );
        for (int j = 0; j < 4; ++j)
        {
            char h = c[j];
            if (h > 32 && h < 127)
                prnt("%c", h);
            else
                prnt("¥");
        }
        
        prnt("    %08x    \n", t);
        
        if (substruct_size)
        {
            static int k = 0, count = 0;
            if (++k >= substruct_size) { k = 0; prnt("%d\n", count++); }
        }
    }
    
    prnt("\n");

#undef prnt
    
    if (file)
        fclose(file);
}

//------------------------------------------------------------
