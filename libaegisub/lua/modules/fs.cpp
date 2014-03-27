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

#include "config.h"

#include "libaegisub/lua/utils.h"

#include <boost/filesystem.hpp>

#include <lualib.h>

#if LUA_VERSION_NUM < 502
#define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))
#endif

namespace {
using namespace agi::lua;
using namespace boost::filesystem;

template<typename Func>
int call(lua_State *L, Func&& f) {
	try {
		return f();
	}
	catch (filesystem_error const& e) {
		lua_pushnil(L);
		lua_pushstring(L, e.what());
		return 2;
	}
	catch (agi::Exception const& e) {
		lua_pushnil(L);
		lua_pushstring(L, e.GetChainedMessage().c_str());
		return 2;
	}
}

int create_path(lua_State *L) {
	make<path>(L, "aegisub.path", luaL_checkstring(L, 1));
	return 1;
}

void push(lua_State *L, bool value) { lua_pushboolean(L, value); }
void push(lua_State *L, path&& p) { make<path>(L, "aegisub.path", std::move(p)); }

#define PATH_FN(name) { \
	#name, \
		[](lua_State *L) -> int { \
		push(L, get<path>(L, 1, "aegisub.path").name()); \
		return 1; \
	} \
}

const luaL_Reg path_table[] = {
	PATH_FN(root_name),
	PATH_FN(root_directory),
	PATH_FN(root_path),
	PATH_FN(relative_path),
	PATH_FN(parent_path),
	PATH_FN(filename),
	PATH_FN(stem),
	PATH_FN(extension),

	PATH_FN(empty),
	PATH_FN(has_root_path),
	PATH_FN(has_root_name),
	PATH_FN(has_root_directory),
	PATH_FN(has_relative_path),
	PATH_FN(has_parent_path),
	PATH_FN(has_filename),
	PATH_FN(has_stem),
	PATH_FN(has_extension),
	PATH_FN(is_absolute),
	PATH_FN(is_relative),

	{"__gc", [](lua_State *L) -> int { get<path>(L, 1, "aegisub.path").~path(); return 0; }}
};

path path_arg(lua_State *L, int idx) {
	if (lua_type(L, idx) == LUA_TSTRING)
		return lua_tostring(L, idx);
	if (lua_type(L, idx) == LUA_TLIGHTUSERDATA)
		return get<path>(L, idx, "aegisub.path");
	throw luaL_argerror(L, idx, "expected string or path");
}

template<typename Ret>
int wrap(lua_State *L, Ret (*func)(path const&)) {
	path p = path_arg(L, 1);
	return call(L, [&] {
		push_value(L, func(p));
		return 1;
	});
}

int wrap(lua_State *L, void (*func)(path const&)) {
	path p = path_arg(L, 1);
	return call(L, [&] {
		func(p);
		lua_pushboolean(L, 1);
		return 1;
	});
}

int wrap(lua_State *L, void (*func)(path const&, path const&)) {
	path p1 = path_arg(L, 1);
	path p2 = path_arg(L, 1);
	return call(L, [&] {
		func(p1, p2);
		lua_pushboolean(L, 1);
		return 1;
	});
}

#define WRAP(name) [](lua_State *L) { \
	return wrap(L, agi::fs::name);    \
}

const luaL_Reg lib[] = {
	{"free_space", WRAP(FreeSpace)},
	{"size", WRAP(Size)},
	{"modified_time", WRAP(ModifiedTime)},
	{"create_directory", WRAP(CreateDirectory)},
	{"touch", WRAP(Touch)},
	{"remove", WRAP(Remove)},
	{"canonicalize", WRAP(Canonicalize)},
	{"rename", WRAP(Rename)},
	{"copy", WRAP(Copy)},
	{"path", create_path},
	{nullptr, nullptr},
};
}

extern "C" int luaopen_fs(lua_State *L) {
	if (luaL_newmetatable(L, "aegisub.path")) {
		luaL_register(L, nullptr, path_table);
		lua_pop(L, 1);
	}

	lua_newtable(L);
	luaL_register(L, nullptr, lib);
	return 1;
}