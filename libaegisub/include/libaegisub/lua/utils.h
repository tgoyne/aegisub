// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <libaegisub/fs.h>
#include <libaegisub/log.h>

#include <lua.h>
#include <lauxlib.h>
#include <string>
#include <type_traits>

namespace agi { namespace lua {

inline void push_value(lua_State *L, bool value) { lua_pushboolean(L, value); }
inline void push_value(lua_State *L, const char *value) { lua_pushstring(L, value); }
inline void push_value(lua_State *L, double value) { lua_pushnumber(L, value); }
inline void push_value(lua_State *L, int value) { lua_pushinteger(L, value); }
inline void push_value(lua_State *L, void *p) { lua_pushlightuserdata(L, p); }

template<typename Integer>
typename std::enable_if<std::is_integral<Integer>::value>::type
push_value(lua_State *L, Integer value) {
	lua_pushinteger(L, static_cast<lua_Integer>(value));
}

inline void push_value(lua_State *L, fs::path const& value) {
	lua_pushstring(L, value.string().c_str());
}

inline void push_value(lua_State *L, std::string const& value) {
	lua_pushlstring(L, value.c_str(), value.size());
}

inline void push_value(lua_State *L, lua_CFunction value) {
	if (lua_type(L, -2) == LUA_TUSERDATA) {
		lua_pushvalue(L, -2);
		lua_pushcclosure(L, value, 1);
	}
	else
		lua_pushcclosure(L, value, 0);
}

template<typename T>
inline void set_field(lua_State *L, const char *name, T value) {
	push_value(L, value);
	lua_setfield(L, -2, name);
}

inline std::string get_string_or_default(lua_State *L, int idx) {
	size_t len = 0;
	const char *str = lua_tolstring(L, idx, &len);
	if (!str)
		return "<not a string>";
	return std::string(str, len);
}

inline std::string get_string(lua_State *L, int idx) {
	size_t len = 0;
	const char *str = lua_tolstring(L, idx, &len);
	return std::string(str ? str : "", len);
}

inline std::string check_string(lua_State *L, int idx) {
	size_t len = 0;
	const char *str = luaL_checklstring(L, idx, &len);
	return std::string(str ? str : "", len);
}

inline std::string get_global_string(lua_State *L, const char *name) {
	lua_getglobal(L, name);
	std::string ret;
	if (lua_isstring(L, -1))
		ret = lua_tostring(L, -1);
	lua_pop(L, 1);
	return ret;
}

template<typename T, typename... Args>
T *make(lua_State *L, const char *mt, Args&&... args) {
	auto obj = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
	new(obj) T(std::forward<Args>(args)...);
	luaL_getmetatable(L, mt);
	lua_setmetatable(L, -2);
	return obj;
}

template<typename T>
T& get(lua_State *L, int idx, const char *mt) {
	return *static_cast<T *>(luaL_checkudata(L, idx, mt));
}

struct LuaForEachBreak {};

template<typename Func>
void lua_for_each(lua_State *L, Func&& func) {
	lua_pushnil(L); // initial key
	while (lua_next(L, -2)) {
		try {
			func();
		}
		catch (LuaForEachBreak) {
			lua_pop(L, 1);
			break;
		}
		lua_pop(L, 1); // pop value, leave key
	}
	lua_pop(L, 1); // pop table
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
			std::string type(lua_typename(L, lua_type(L, -1)));
			if (lua_isstring(L, i))
				LOG_D("automation/lua/stackdump") << type << ": " << lua_tostring(L, -1);
			else
				LOG_D("automation/lua/stackdump") << type;
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

} }
