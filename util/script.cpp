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

    //add_callback("print", func_print);

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

float script::get_float(lua_State *state, int arg_idx)
{
    return state ? (float)luaL_checknumber(state, arg_idx + 1) : 0.0f;
}

//------------------------------------------------------------

bool script::get_bool(lua_State *state, int arg_idx)
{
    return lua_isboolean(state, arg_idx + 1) ? lua_toboolean(state, arg_idx + 1) > 0 : false;
}

//------------------------------------------------------------

nya_math::vec3 script::get_vec3(lua_State *s, int arg_idx)
{
    nya_math::vec3 p;

    if (!lua_istable(s, arg_idx + 1))
        return p;

    lua_getfield(s, -1, "x");
    p.x = (float)luaL_checknumber(s, -1);
    lua_pop(s, 1);

    lua_getfield(s, -1, "y");
    p.y = (float)luaL_checknumber(s, -1);
    lua_pop(s, 1);

    lua_getfield(s, -1, "z");
    p.z = (float)luaL_checknumber(s, -1);
    lua_pop(s, 1);

    return p;
}

//------------------------------------------------------------

void script::push_float(lua_State *s, float value)
{
    lua_pushnumber(s, value);
}

//------------------------------------------------------------

void script::push_vec3(lua_State *s, const nya_math::vec3 &value)
{
    lua_createtable(s, 0, 3);

    lua_pushstring(s, "x");
    lua_pushnumber(s, value.x);
    lua_settable(s, -3);

    lua_pushstring(s, "y");
    lua_pushnumber(s, value.y);
    lua_settable(s, -3);

    lua_pushstring(s, "z");
    lua_pushnumber(s, value.z);
    lua_settable(s, -3);
}

//------------------------------------------------------------

void script::push_string(lua_State *s, std::string value)
{
    lua_pushstring(s, value.c_str());
}

//------------------------------------------------------------

void script::push_array(lua_State *s, const std::vector<std::string> &array)
{
    lua_createtable(s, 0, (int)array.size());

    int i = 0;
    for (auto &v: array)
    {
        lua_pushnumber(s, ++i);
        lua_pushstring(s, v.c_str());
        lua_settable(s, -3);
    }
}

//------------------------------------------------------------

void script::push_array(lua_State *s, const std::vector<std::pair<std::string, std::string> > &array)
{
    lua_createtable(s, 0, (int)array.size());

    for (auto &v: array)
    {
        lua_pushstring(s, v.first.c_str());
        lua_pushstring(s, v.second.c_str());
        lua_settable(s, -3);
    }
}

//------------------------------------------------------------

void script::push_array(lua_State *s, const std::vector<std::pair<std::string, std::string> > &array, std::string key0, std::string key1)
{
    lua_createtable(s, 0, (int)array.size());

    int i = 0;
    for (auto &v: array)
    {
        lua_pushnumber(s, ++i);
        lua_createtable(s, 0, 2);

        lua_pushstring(s, key0.c_str());
        lua_pushstring(s, v.first.c_str());
        lua_settable(s, -3);

        lua_pushstring(s, key1.c_str());
        lua_pushstring(s, v.second.c_str());
        lua_settable(s, -3);

        lua_settable(s, -3);
    }
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
