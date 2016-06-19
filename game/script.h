//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "game.h"

struct lua_State;

namespace game
{
//------------------------------------------------------------

class script
{
public:
    bool load(std::string text);
    void unload();

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

private:
    void on_error();

private:
    lua_State *m_state = 0;
    std::string m_error;
};

//------------------------------------------------------------
}
