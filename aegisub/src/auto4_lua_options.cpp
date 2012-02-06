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

/// @file auto4_lua_options.cpp
/// @see auto4_lua_options.h
/// @ingroup scripting
/// @brief auto4lua access to Aegisub options

#include "auto4_lua_options.h"

#include "auto4_lua_utils.h"
#include "main.h"

#include <libaegisub/exception.h>
#include <libaegisub/option.h>

namespace Automation4 {

LuaOptions::LuaOptions(lua_State *L)
{
	luaL_newmetatable(L, "LuaOptions");

	set_field(L, "__index", closure_wrapper<LuaOptions, &LuaOptions::GetOption>);
	set_field(L, "__newindex", closure_wrapper_v<LuaOptions, &LuaOptions::SetOption>);
	set_field(L, "__gc", closure_wrapper_v<LuaOptions, &LuaOptions::OnGC>);

	*static_cast<LuaOptions**>(lua_newuserdata(L, sizeof(LuaOptions**))) = this;
	luaL_getmetatable(L, "LuaOptions");
	lua_setmetatable(L, -2);
}

void LuaOptions::OnGC(lua_State *L) {
	delete this;
}

int LuaOptions::GetOption(lua_State *L) {
	try {
		const agi::OptionValue *opt = OPT_GET(luaL_checkstring(L, -1));
	}
	catch (agi::OptionErrorNotFound const& e) {
		luaL_error(L, "%s", e.GetMessage());
	}
	return 1;
}

void LuaOptions::SetOption(lua_State *L) {
	try {
		const agi::OptionValue *opt = OPT_GET(luaL_checkstring(L, -1));
	}
	catch (agi::OptionErrorNotFound const& e) {
		luaL_error(L, "%s", e.GetMessage());
	}
}


}
