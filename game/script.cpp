//
// open horizon -- undefined_darkness@outlook.com
//

#include "script.h"

extern "C"
{
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

namespace game
{
//------------------------------------------------------------

static int func_print(lua_State *state)
{
    const char *s = luaL_checkstring(state, 1);
    if (s)
        printf("script print: %s\n", s);
    return 0;
}

//------------------------------------------------------------

bool script::load(std::string text)
{
    m_error.clear();
    unload();

    m_state = luaL_newstate();

    lua_pushcfunction(m_state, func_print);
    lua_setglobal(m_state, "print");

    if (luaL_dostring(m_state, text.c_str()) != 0)
    {
        on_error();
        unload();
        return false;
    }

    luaopen_string(m_state);
    luaopen_math(m_state);
    lua_pcall(m_state, 0, 0, 0);

    return true;
}

//------------------------------------------------------------

void script::on_error()
{
    if (!m_state)
        return;

    m_error = lua_tostring(m_state, -1);
    lua_pop(m_state, 1);
}

//------------------------------------------------------------

bool script::call(std::string function, const std::vector<value> &values)
{
    m_error.clear();
    if (!m_state)
        return false;

    lua_getglobal(m_state, function.c_str());
    if (lua_type(m_state, -1) != LUA_TFUNCTION)
    {
        m_error = "function not found: " + function;
        return false;
    }

    for (auto &v: values)
    {
        switch(v.type)
        {
            case value::type_float: lua_pushnumber(m_state, v.f); break;
            case value::type_int: lua_pushinteger(m_state, v.i); break;
            case value::type_string: lua_pushstring(m_state, v.s.c_str()); break;
        }
    }

    if (lua_pcall(m_state, (int)values.size(), 0, 0) != 0)
    {
        on_error();
        return false;
    }

    return true;
}

//------------------------------------------------------------

bool script::has_function(std::string function)
{
    if (!m_state)
        return false;

    lua_getglobal(m_state, function.c_str());
    if (lua_type(m_state, -1) != LUA_TFUNCTION)
        return false;

    lua_pop(m_state, 1);
    return true;
}

//------------------------------------------------------------

void script::unload()
{
    if (m_state)
        lua_close(m_state);
    m_state = 0;
}

//------------------------------------------------------------
}
