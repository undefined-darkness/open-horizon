//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "resources/resources.h"
#include <assert.h>
#include <math.h>
#include <vector>
#include <algorithm>

#define assume(expr) assert(expr) //like assert, but not critical

//------------------------------------------------------------

inline nya_memory::tmp_buffer_ref load_resource(const char *name)
{
    nya_resources::resource_data *res = nya_resources::get_resources_provider().access(name);
    if (!res)
        return nya_memory::tmp_buffer_ref();

    nya_memory::tmp_buffer_ref buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();

    return buf;
}

//------------------------------------------------------------

inline void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0)
{
    FILE *file = 0;
    if (fileName)
        file = fopen(fileName, "wb");

#define prnt(...) do{ if (file) fprintf(file, __VA_ARGS__); else printf(__VA_ARGS__); }while(0)

    prnt("\ndata at offset: %ld size: %ld\n", (long int)offset, (long int)size);

    nya_memory::memory_reader reader = const_reader;
    reader.seek(offset);
    if (size > reader.get_remained())
        size = reader.get_remained();

    //bool had_zero = false;
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
            prnt("%3d ", ((unsigned char *)c)[j]);
        prnt( "   ");

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

inline void print_data(const nya_memory::memory_reader &reader)
{
    print_data(reader, reader.get_offset(), reader.get_remained());
}

//------------------------------------------------------------

inline void print_data(const char *name)
{
    nya_memory::tmp_buffer_scoped r(load_resource(name));
    nya_memory::memory_reader reader(r.get_data(),r.get_size());
    print_data(reader, reader.get_offset(), reader.get_remained());
}

//------------------------------------------------------------

inline void print_params(const char *name)
{
    nya_memory::tmp_buffer_scoped r(load_resource(name));
    if (!r.get_size())
        return;

    std::vector<std::string> values;
    std::string tmp;
    for (size_t i = 0; i < r.get_size(); ++i)
    {
        char c=((char *)r.get_data())[i];
        if (c=='\n' || c=='\r')
        {
            if (tmp.empty())
                continue;

            if (tmp[0]=='#')
            {
                tmp.clear();
                continue;
            }

            for (size_t j = 0; j < tmp.size(); ++j)
            {
                std::string type = tmp.substr(0, j);
                if (tmp[j] == '\t')
                {
                    tmp = tmp.substr(j + 1);
                    tmp += " ";
                    tmp += type;
                    break;
                }
            }

            values.push_back(tmp);
            tmp.clear();
            continue;
        }

        tmp.push_back(c);
    }

    std::sort(values.begin(), values.end());

    for (size_t i = 0; i < values.size(); ++i)
        printf("%03zu %s\n", i, values[i].c_str());
}

//------------------------------------------------------------

inline void find_data(nya_resources::resources_provider &rp, const void *data, size_t size, size_t align=4)
{
    for (int i = 0; i < rp.get_resources_count(); ++i)
    {
        auto n=rp.get_resource_name(i);
        if (!n)
            continue;

        auto r = rp.access(n);
        if (!r)
            continue;

        unsigned int found_count = 0;

        nya_memory::tmp_buffer_scoped buf(r->get_size());
        r->read_all(buf.get_data());
        auto d = (char *)buf.get_data();
        for (size_t j = 0; j + size < r->get_size(); j+=align)
        {
            if (memcmp(data, d + j, size) == 0)
                ++found_count;
        }

        if (found_count > 0)
            printf("found %u times in %s\n", found_count, n);
    }

    printf("\n");
}

//------------------------------------------------------------

inline void find_data(nya_resources::resources_provider &rp, float *f, size_t count, float eps = 0.01f, size_t align=4)
{
    for (int i = 0; i < rp.get_resources_count(); ++i)
    {
        auto n=rp.get_resource_name(i);
        if (!n)
            continue;

        auto r = rp.access(n);
        if (!r)
            continue;

        unsigned int found_count = 0;

        nya_memory::tmp_buffer_scoped buf(r->get_size());
        r->read_all(buf.get_data());
        for (size_t j = 0; j + count * sizeof(float) < r->get_size(); j+=align)
        {
            bool found = true;
            float *t = (float *)((char *)buf.get_data() + j);
            for (size_t k = 0; k < count; ++k)
            {
                if (t[k] != t[k]) //nan
                {
                    found = false;
                    continue;
                }

                if (fabsf(t[k] - f[k]) < eps)
                    continue;

                found = false;

            }
            if(found)
                ++found_count;
        }

        if (found_count > 0)
            printf("found %u times in %s\n", found_count, n);
    }

    printf("\n");
}

//------------------------------------------------------------

struct debug_variable
{
    static int get() { return *value(); }
    static void set(int v) { *value() = v; printf("debug variable %d\n", v); }

private:
    static int *value() { static int *value = new int(0); return value; }
};

//------------------------------------------------------------
