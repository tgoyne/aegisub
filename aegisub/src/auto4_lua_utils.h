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

#include <libaegisub/log.h>

#ifdef __WINDOWS__
#include "../../contrib/lua51/src/lualib.h"
#include "../../contrib/lua51/src/lauxlib.h"
#else
#include <lua.hpp>
#endif

#include <wx/string.h>

inline void push_value(lua_State *L, wxString const& value) {
	lua_pushstring(L, value.utf8_str());
}

inline void push_value(lua_State *L, const char *value) {
	lua_pushstring(L, value);
}

inline void push_value(lua_State *L, lua_CFunction value) {
	if (lua_type(L, -2) == LUA_TUSERDATA) {
		lua_pushvalue(L, -2);
		lua_pushcclosure(L, value, 1);
	}
	else
		lua_pushcclosure(L, value, 0);
}

inline void push_value(lua_State *L, bool value) {
	lua_pushboolean(L, value);
}

inline void push_value(lua_State *L, double value) {
	lua_pushnumber(L, value);
}

inline void push_value(lua_State *L, int value) {
	lua_pushinteger(L, value);
}

inline void push_value(lua_State *L, void *p) {
	lua_pushlightuserdata(L, p);
}

template<typename T>
inline void set_field(lua_State *L, const char *name, T value) {
	push_value(L, value);
	lua_setfield(L, -2, name);
}

inline wxString get_wxstring(lua_State *L, int idx) {
	return wxString(lua_tostring(L, idx), wxConvUTF8);
}

inline wxString check_wxstring(lua_State *L, int idx) {
	return wxString(luaL_checkstring(L, idx), wxConvUTF8);
}

inline wxString get_global_string(lua_State *L, const char *name) {
	lua_getglobal(L, name);
	wxString ret;
	if (lua_isstring(L, -1))
		ret = get_wxstring(L, -1);
	lua_pop(L, 1);
	return ret;
}

#ifdef _DEBUG
struct LuaStackcheck {
	lua_State *L;
	int startstack;

	void check_stack(int additional) {
		int top = lua_gettop(L);
		if (top - additional != startstack) {
			LOG_D("automation/lua") << "lua stack size mismatch.";
			dump();
			assert(top - additional == startstack);
		}
	}

	void dump() {
		int top = lua_gettop(L);
		LOG_D("automation/lua/stackdump") << "--- dumping lua stack...";
		for (int i = top; i > 0; i--) {
			lua_pushvalue(L, i);
			wxString type(lua_typename(L, lua_type(L, -1)), wxConvUTF8);
			if (lua_isstring(L, i)) {
				LOG_D("automation/lua/stackdump") << type << ": " << lua_tostring(L, -1);
			} else {
				LOG_D("automation/lua/stackdump") << type;
			}
			lua_pop(L, 1);
		}
		LOG_D("automation/lua") << "--- end dump";
	}
	LuaStackcheck(lua_State *L) : L(L), startstack(lua_gettop(L)) { }
	~LuaStackcheck() { check_stack(0); }
};
#else
struct LuaStackcheck {
	void check_stack(int) { }
	void dump() { }
	LuaStackcheck(lua_State*) { }
};
#endif
