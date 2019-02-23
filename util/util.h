//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "resources/file_resources_provider.h"
#include "render/debug_draw.h"
#include "system/system.h"
#include <assert.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <atomic>

#if !defined __APPLE__ && defined __GNUC__ && __GNUC__ < 5
    #define NO_CODECVT
#else
    #include <locale>
    #include <codecvt>
#endif

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

//------------------------------------------------------------

#define assume(expr) assert(expr) //like assert, but not critical

//------------------------------------------------------------

inline uint16_t swap_bytes(uint16_t v) { return ((v & 0xff) << 8) | ((v & 0xff00) >> 8); }
inline uint32_t swap_bytes(uint32_t v) { return (v >> 24) | ((v<<8) & 0x00FF0000) | ((v>>8) & 0x0000FF00) | (v << 24); }
inline uint64_t swap_bytes(uint64_t v)
{
    v = (v & 0x00000000FFFFFFFF) << 32 | (v & 0xFFFFFFFF00000000) >> 32;
    v = (v & 0x0000FFFF0000FFFF) << 16 | (v & 0xFFFF0000FFFF0000) >> 16;
    return (v & 0x00FF00FF00FF00FF) << 8  | (v & 0xFF00FF00FF00FF00) >> 8;
}

inline int16_t swap_bytes(int16_t v) { const uint16_t t = swap_bytes(*(uint16_t *)&v); return *(int16_t *)&t; }
inline int32_t swap_bytes(int32_t v) { const uint32_t t = swap_bytes(*(uint32_t *)&v); return *(int32_t *)&t; }
inline float swap_bytes(float v) { const uint32_t t = swap_bytes(*(uint32_t *)&v); return *(float *)&t; }

//------------------------------------------------------------

template<typename t> t roundup(t v, int m) { return ((v + m - 1) / m) * m; }

//------------------------------------------------------------

class noncopyable
{
protected:
    noncopyable() {}

private:
    noncopyable(const noncopyable& ) = delete; // non construction-copyable
    noncopyable& operator=(const noncopyable& ) = delete; // non copyable
};

//------------------------------------------------------------

inline nya_memory::tmp_buffer_ref load_resource(nya_resources::resource_data *res)
{
    if (!res)
        return nya_memory::tmp_buffer_ref();

    nya_memory::tmp_buffer_ref buf(res->get_size());
    res->read_all(buf.get_data());
    res->release();

    return buf;
}

//------------------------------------------------------------

inline nya_memory::tmp_buffer_ref load_resource(const char *name)
{
    return load_resource(nya_resources::get_resources_provider().access(name));
}

//------------------------------------------------------------

inline std::string get_path(std::string file_path)
{
    auto path=file_path.substr(0, file_path.find_last_of("\\/"));
    return path.empty() ? path : path + "/";
}

//------------------------------------------------------------

inline std::wstring to_wstring(const std::string &s)
{
#ifdef NO_CODECVT
    return std::wstring(s.begin(), s.end()); //ToDo
#else
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > converter;
    return converter.from_bytes(s);
#endif
}

//------------------------------------------------------------

inline std::string from_wstring(const std::wstring &s)
{
#ifdef NO_CODECVT
    return std::string(s.begin(), s.end()); //ToDo
#else
    static std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(s);
#endif
}

//------------------------------------------------------------

template<typename t> std::string to_bits(t v)
{
    std::string str;

    const int shift = 8 * sizeof(t) - 1;
    const t mask = 1 << shift;

    for (int i = 0; i <= shift; ++i)
    {
        str.push_back(v & mask ? '1' : '0');
        v <<= 1;
        if ((i + 1) % 8 == 0 )
            str.push_back(' ');
    }

    return str;
}

//------------------------------------------------------------

inline void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0, bool be = false)
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

    for (int i = 0; reader.get_offset() < offset+size; ++i)
    {
        int off = int(reader.get_offset());
        unsigned int t = reader.read<unsigned int>();

        if (i * 4 == off)
            prnt( "%7d ", i * 4 );
        else
            prnt( "%7d %7d ", i * 4, off );

        unsigned int u = t;
        unsigned short s[2];
        memcpy(s, &t, 4);
        char c[4];
        memcpy(c, &t, 4);
        if (be)
        {
            u = swap_bytes(u);
            s[0] = swap_bytes(s[0]);
            s[1] = swap_bytes(s[1]);
        }
        float f =* ((float*)&u);

        //if ((fabs(f) < 0.001 && t != 0) || (fabs(f) > 1000.0f))
        //prnt( "%10u ", t);

        if (fabs(f) > 50000.0f)
            prnt( "           " );
        else
            prnt( "%10f ", f);

        prnt( "%10u ", u);

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

        prnt("    %08x    \n", u);

        if (substruct_size)
        {
            static size_t k = 0, count = 0;
            if (++k >= substruct_size) { k = 0; prnt("%ld\n", count++); }
        }
    }

    prnt("\n");

#undef prnt

    if (file)
        fclose(file);
}

//------------------------------------------------------------

inline void print_data(const nya_memory::memory_reader &reader, const char *file_name = 0)
{
    print_data(reader, reader.get_offset(), reader.get_remained(), 0, file_name);
}

//------------------------------------------------------------

inline void print_data(const char *name, const char *file_name = 0)
{
    nya_memory::tmp_buffer_scoped r(load_resource(name));
    nya_memory::memory_reader reader(r.get_data(),r.get_size());
    print_data(reader, file_name);
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

        std::vector<size_t> found_offsets;

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
            if (found)
                found_offsets.push_back(j);
        }

        if (!found_offsets.empty())
        {
            printf("found %lu times in %s", found_offsets.size(), n);
            if (found_offsets.size()<=10)
            {
                printf("at offset%s: ", found_offsets.size()==1?"":"s");
                for (auto &o: found_offsets)
                    printf("%lu ", o);
            }
            printf("\n");
        }
    }

    printf("\n");
}

//------------------------------------------------------------

inline std::vector<std::string> list_files(std::string folder)
{
    std::vector<std::string> result;
    auto app_path = nya_system::get_app_path();
    if (!app_path)
        return result;

    nya_resources::file_resources_provider fprov;
    if (!fprov.set_folder((app_path + folder).c_str()))
       return result;

    for (int i = 0; i < fprov.get_resources_count(); ++i)
    {
        auto name = fprov.get_resource_name(i);
        if (!name)
            continue;

        result.push_back(folder + name);
    }

    std::sort(result.begin(), result.end());
    return result;
}

//------------------------------------------------------------

inline void create_path(const char *dir)
{
    if (!dir)
        return;

    std::string tmp(dir);
    for (char *p = &tmp[1]; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
#ifdef _WIN32
            _mkdir(tmp.c_str());
#else
            mkdir(tmp.c_str(), S_IRWXU);
#endif
            *p = '/';
        }
    }
}

//------------------------------------------------------------

inline bool write_file(const char *name, const void *buf, size_t size)
{
    if (!name || !buf)
        return false;

    FILE *f = fopen(name, "wb");
    if (!f)
    {
        printf("unable to write file %s\n", name);
        return false;
    }

    const bool r = fwrite(buf, size, 1, f) == size;
    fclose(f);
    return r;
}

//------------------------------------------------------------

inline bool file_exists(const char *name)
{
    struct stat sb;
    return name && name[0] && stat(name, &sb)==0;
}

//------------------------------------------------------------

template<typename t> nya_resources::resource_data *access(t &provider, int idx)
{
    if (idx < 0 || idx >= provider.get_files_count())
        return 0;

    struct res: public nya_resources::resource_data
    {
        t &p;
        int idx;

        res(t &p, int idx): p(p), idx(idx) {}

    private:
        size_t get_size() override { return p.get_file_size(idx); }
        bool read_all(void*data) override { return p.read_file_data(idx, data); }
        bool read_chunk(void *data, size_t size, size_t offset=0) override { return p.read_file_data(idx, data, size, offset); }
        void release() override { delete this; }
    };

    return new res(provider, idx);
}

//------------------------------------------------------------

struct debug_variable
{
    static int get() { return *value(); }
    static void set(int v) { *value() = v; printf("debug variable %d\n", v); }

private:
    static std::atomic<int> *value() { static std::atomic<int> *value = new std::atomic<int>(0); return value; }
};

//------------------------------------------------------------

inline nya_render::debug_draw &get_debug_draw() { static nya_render::debug_draw dd; return dd; }

//------------------------------------------------------------

inline float random(float from, float to) { return from + (to - from) * float(rand()/(RAND_MAX + 1.0f)); }
inline int random(int from, int to) { return from +  rand() % (to - from + 1); }

//------------------------------------------------------------
