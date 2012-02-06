// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file auto4_lua_utils.h
/// @see auto4_lua_utils.cpp
/// @ingroup scripting
/// @brief Assorted lua helper functions

#ifdef __WINDOWS__
#include "../../contrib/lua51/src/lualib.h"
#include "../../contrib/lua51/src/lauxlib.h"
#else
#include <lua.hpp>
#endif

namespace Automation4 {

namespace detail {
	// helpers for set_field
	inline void push_value(lua_State *L, bool value) { lua_pushboolean(L, value); }
	inline void push_value(lua_State *L, double value) { lua_pushnumber(L, value); }
	inline void push_value(lua_State *L, int value) { lua_pushinteger(L, value); }
	inline void push_value(lua_State *L, wxString const& value) { lua_pushstring(L, value.utf8_str()); }
	inline void push_value(lua_State *L, const char *value) { lua_pushstring(L, value); }
	inline void push_value(lua_State *L, std::string const& value) { lua_pushstring(L, value.c_str()); }
	inline void push_value(lua_State *L, lua_CFunction fn) { lua_pushcfunction(L, fn); }
}

/// Set a field in the table or userdata at the top of the stack
/// @param L     Lua state
/// @param name  Name of the field to set
/// @param value New value of the field
template<class T>
inline void set_field(lua_State *L, const char *name, T value) {
	detail::push_value(L, value);
	lua_setfield(L, -2, name);
}

inline void get_if_right_type(lua_State *L, wxString &def) {
	if (lua_isstring(L, -1))
		def = wxString(lua_tostring(L, -1), wxConvUTF8);
}

inline void get_if_right_type(lua_State *L, double &def) {
	if (lua_isnumber(L, -1))
		def = std::max(lua_tonumber(L, -1), def);
}

inline void get_if_right_type(lua_State *L, int &def) {
	if (lua_isnumber(L, -1))
		def = std::max<int>(lua_tointeger(L, -1), def);
}

inline void get_if_right_type(lua_State *L, bool &def) {
	if (lua_isboolean(L, -1))
		def = !!lua_toboolean(L, -1);
}

template<class T>
T get_field(lua_State *L, const char *name, T def) {
	lua_getfield(L, -1, name);
	get_if_right_type(L, def);
	lua_pop(L, 1);
	return def;
}

inline wxString get_field(lua_State *L, const char *name) {
	return get_field(L, name, wxString());
}

template<class T, int (T::*closure)(lua_State *)>
int closure_wrapper(lua_State *L) {
	int idx = lua_upvalueindex(1);
	assert(lua_type(L, idx) == LUA_TUSERDATA);
	return (*((T**)lua_touserdata(L, idx))->*closure)(L);
}

template<class T, void (T::*closure)(lua_State *)>
int closure_wrapper_v(lua_State *L) {
	int idx = lua_upvalueindex(1);
	assert(lua_type(L, idx) == LUA_TUSERDATA);
	(*((T**)lua_touserdata(L, idx))->*closure)(L);
	return 0;
}

inline wxString get_wxstring(lua_State *L, int idx) {
	return wxString(lua_tostring(L, idx), wxConvUTF8);
}

inline wxString check_wxstring(lua_State *L, int idx) {
	return wxString(luaL_checkstring(L, idx), wxConvUTF8);
}

wxString get_global_string(lua_State *L, const char *name) {
	lua_getglobal(L, name);
	wxString ret;
	if (lua_isstring(L, -1))
		ret = get_wxstring(L, -1);
	lua_pop(L, 1);
	return ret;
}

// LuaStackcheck
#if 0
struct LuaStackcheck {
	lua_State *L;
	int startstack;
	void check_stack(int additional)
	{
		int top = lua_gettop(L);
		if (top - additional != startstack) {
			LOG_D("automation/lua") << "lua stack size mismatch.";
			dump();
			assert(top - additional == startstack);
		}
	}
	void dump()
	{
		int top = lua_gettop(L);
		LOG_D("automation/lua/stackdump") << "--- dumping lua stack...";
		for (int i = top; i > 0; i--) {
			lua_pushvalue(L, i);
			wxString type(lua_typename(L, lua_type(L, -1)), wxConvUTF8);
			if (lua_isstring(L, i)) {
				LOG_D("automation/lua/stackdump") << type << ": " << luatostring(L, -1);
			} else {
				LOG_D("automation/lua/stackdump") << type;
			}
			lua_pop(L, 1);
		}
		LOG_D("automation/lua") << "--- end dump";
	}
	LuaStackcheck(lua_State *L) : L(L) { startstack = lua_gettop(L); }
	~LuaStackcheck() { check_stack(0); }
};
#else
struct LuaStackcheck {
	void check_stack(int) { }
	void dump() { }
	LuaStackcheck(lua_State*) { }
};
#endif


template<class T>
void read_string_array(lua_State *L, T &cont) {
	lua_pushnil(L);
	while (lua_next(L, -2)) {
		if (lua_isstring(L, -1))
			cont.push_back(wxString(lua_tostring(L, -1), wxConvUTF8));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

}
