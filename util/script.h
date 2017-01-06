//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "math/vector.h"
#include <string>
#include <vector>

struct lua_State;

//------------------------------------------------------------

class script
{
public:
    bool load(std::string text);
    void unload();

    typedef int(*callback)(lua_State *);
    void add_callback(std::string name, callback f);
    static int get_args_count(lua_State *s);
    static std::string get_string(lua_State *s, int arg_idx);
    static int get_int(lua_State *s, int arg_idx);
    static float get_float(lua_State *s, int arg_idx);
    static bool get_bool(lua_State *s, int arg_idx);
    static nya_math::vec3 get_vec3(lua_State *s, int arg_idx);
    static void push_float(lua_State *s, float value);
    static void push_vec3(lua_State *s, const nya_math::vec3 &value);
    static void push_string(lua_State *s, std::string value);
    static void push_array(lua_State *s, const std::vector<std::string> &array);
    static void push_array(lua_State *s, const std::vector<std::pair<std::string, std::string> > &array);
    static void push_array(lua_State *s, const std::vector<std::pair<std::string, std::string> > &array, std::string key0, std::string key1);

    const std::string &get_error() const { return m_error; }

    struct value
    {
        enum value_type { type_int = 'i', type_float = 'f', type_string = 's' } type;

        int i = 0;
        float f = 0.0f;
        std::string s;

        value() {}
        value(int i): type(type_int), i(i) {}
        value(float f): type(type_float), f(f) {}
        value(std::string s): type(type_string), s(s) {}
        value(const char *s): type(type_string), s(s?s:"") {}
    };

    bool call(std::string function, const std::vector<value> &values = {});
    bool has_function(std::string function);

private:
    void on_error();

private:
    lua_State *m_state = 0;
    std::string m_error;
};

//------------------------------------------------------------
