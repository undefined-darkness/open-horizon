//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include <vector>
#include <string>
#include <map>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

class config
{
public:
    static void register_var(const std::string &name, const std::string &value)
    {
        auto p = cfg().m_params.find(name);
        if (p == cfg().m_params.end())
        {
            cfg().m_params[name] = value;
            cfg().m_changed = true;
        }
    }

    static void set_var(const std::string &name, const std::string &value)
    {
        auto p = cfg().m_params.find(name);
        if (p != cfg().m_params.end() && p->second != value)
        {
            p->second = value;
            cfg().m_changed = true;
        }
    }

    static std::string get_var(const std::string &name)
    {
        auto p = cfg().m_params.find(name);
        if (p == cfg().m_params.end())
            return "";
        return p->second;
    }

    static int get_var_int(const std::string &name)
    {
        auto v = get_var(name);
        return atoi(v.c_str());
    }

    const static std::map<std::string, std::string> &get_vars()
    {
        return cfg().m_params;
    }

private:
    static config &cfg() { static config c; return c; }
    config() { read(); }
    ~config() { if (m_changed) write(); }

private:
    bool read()
    {
        FILE *f = fopen(get_fname(), "rb");
        if (!f)
            return false;

        fseek(f, 0, SEEK_END);
        auto size = ftell(f);
        if (size)
        {
            fseek(f, 0, SEEK_SET);

            std::vector<char> data(size);
            fread(data.data(), 1, size, f);

            std::string name, value;
            bool parse_value = false;
            for (auto c: data)
            {
                if (c == '\n')
                {
                    parse_value = false;
                    m_params[name] = value;
                    name.clear();
                    value.clear();
                }
                else if (c == '=')
                    parse_value = true;
                else if (parse_value)
                    value.push_back(c);
                else
                    name.push_back(c);
            }
        }

        fclose(f);
        return true;
    }

    bool write()
    {
        FILE *f = fopen(get_fname(), "wb");
        if (!f)
            return false;

        std::string data;
        for (auto &p: m_params)
            data.append(p.first + "=" + p.second + "\n");

        fwrite(data.data(), 1, data.size(), f);
        fclose(f);
        return true;
    }

private:
    const char *get_fname()
    {
        static std::string fname;
        if (fname.empty())
        {
            const char *path =
#ifdef _WIN32
            getenv("APPDATA");
#else
            getenv("HOME");
            if (!path)
                path = getpwuid(getuid())->pw_dir;
#endif
            if (path)
                fname.append(path).append("/");

            fname.append(".open-horizon.cfg");
        }

        return fname.c_str();
    }

private:
    std::map<std::string, std::string> m_params;
    bool m_changed = false;
};
