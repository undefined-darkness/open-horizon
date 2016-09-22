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

    luaL_openlibs(m_state);

    const luaL_Reg libs[] =
    {
        {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math},
    };

    for (auto &l: libs)
    {
        lua_pushcfunction(m_state, l.func);
        lua_pushstring(m_state, l.name);
        lua_call(m_state, 1, 0);
    }

    add_callback("print", func_print);

    if (luaL_dostring(m_state, text.c_str()) != 0)
    {
        on_error();
        unload();
        return false;
    }

    lua_pcall(m_state, 0, 0, 0);

    return true;
}

//------------------------------------------------------------

void script::add_callback(std::string name, callback f)
{
    if (!m_state)
        return;

    lua_pushcfunction(m_state, f);
    lua_setglobal(m_state, name.c_str());
}

//------------------------------------------------------------

int script::get_args_count(lua_State *state)
{
    if (!state)
        return 0;

    return lua_gettop(state);
}

//------------------------------------------------------------

std::string script::get_string(lua_State *state, int arg_idx)
{
    if (!state)
        return "";

    const char *s = luaL_checkstring(state, arg_idx + 1);
    return s ? s : "";
}

//------------------------------------------------------------

int script::get_int(lua_State *state, int arg_idx)
{
    return state ? luaL_checkint(state, arg_idx + 1) : 0;
}

//------------------------------------------------------------

bool script::get_bool(lua_State *state, int arg_idx)
{
    return lua_isboolean(state, arg_idx + 1) ? lua_toboolean(state, arg_idx + 1) > 0 : false;
}

//------------------------------------------------------------

void script::push_float(lua_State *s, float value)
{
    lua_pushnumber(s, value);
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
    if (!m_state || function.empty())
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
