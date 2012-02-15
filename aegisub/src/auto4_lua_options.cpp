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

#include "config.h"

#ifdef WITH_AUTO4_LUA

#include "auto4_lua_options.h"

#include "main.h"

#include <libaegisub/exception.h>
#include <libaegisub/option.h>

// This must be below the headers above.
#ifdef __WINDOWS__
#include "../../contrib/lua51/src/lualib.h"
#include "../../contrib/lua51/src/lauxlib.h"
#else
#include <lua.hpp>
#endif

namespace {
	inline void push_value(lua_State *L, int64_t value) { lua_pushinteger(L, value); }
	inline void push_value(lua_State *L, double value) { lua_pushnumber(L, value); }
	inline void push_value(lua_State *L, bool value) { lua_pushboolean(L, value); }
	inline void push_value(lua_State *L, std::string const& value) { lua_pushstring(L, value.c_str()); }

	template<class T>
	void push_table(lua_State *L, std::vector<T> const& vec) {
		lua_newtable(L);
		for (size_t i = 0; i < vec.size(); ++i) {
			push_value(L, vec[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	template<class T> inline T read_value(lua_State *L, int idx);
	template<> inline std::string read_value<>(lua_State *L, int idx) { return luaL_checkstring(L, idx); }
	template<> inline int64_t read_value<>(lua_State *L, int idx) { return luaL_checkinteger(L, idx); }
	template<> inline double read_value<>(lua_State *L, int idx) { return luaL_checknumber(L, idx); }
	template<> inline bool read_value<>(lua_State *L, int idx) { return !!lua_toboolean(L, idx); }

	template<class T>
	std::vector<T> read_table(lua_State *L, int idx) {
		std::vector<T> ret;
		luaL_argcheck(L, lua_istable(L, idx), idx, "");

		lua_pushvalue(L, idx);
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			ret.push_back(read_value<T>(L, -1));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		return ret;
	}

	void set_field(lua_State *L, const char *name, lua_CFunction fn) {
		lua_pushcfunction(L, fn);
		lua_setfield(L, -2, name);
	}

	int get_option(lua_State *L) {
		Automation4::LuaOptions *o = static_cast<Automation4::LuaOptions*>(luaL_checkudata(L, 1, "aegisub.options"));
		try {
			const agi::OptionValue *opt = o->GetOption(luaL_checkstring(L, 2));
			switch (opt->GetType()) {
				case agi::OptionValue::Type_String:
					push_value(L, opt->GetString());
					break;
				case agi::OptionValue::Type_Int:
					push_value(L, opt->GetInt());
					break;
				case agi::OptionValue::Type_Double:
					push_value(L, opt->GetDouble());
					break;
				case agi::OptionValue::Type_Colour:
					push_value(L, opt->GetColour());
					break;
				case agi::OptionValue::Type_Bool:
					push_value(L, opt->GetBool());
					break;
				case agi::OptionValue::Type_List_String:
					push_table(L, opt->GetListString());
					break;
				case agi::OptionValue::Type_List_Int:
					push_table(L, opt->GetListInt());
					break;
				case agi::OptionValue::Type_List_Double:
					push_table(L, opt->GetListDouble());
					break;
				case agi::OptionValue::Type_List_Colour:
					push_table(L, opt->GetListColour());
					break;
				case agi::OptionValue::Type_List_Bool:
					push_table(L, opt->GetListBool());
					break;
				default:
					return luaL_error(L, "Unknown option type");
			}
		}
		catch (agi::OptionErrorNotFound const& e) {
			return luaL_error(L, "%s", e.GetMessage().c_str());
		}
		return 1;
	}

	int set_option(lua_State *L) {
		Automation4::LuaOptions *o = static_cast<Automation4::LuaOptions*>(luaL_checkudata(L, 1, "aegisub.options"));
		std::string opt_name = luaL_checkstring(L, 2);
		agi::OptionValue *new_opt = 0;

		const agi::OptionValue *old_opt;
		try {
			old_opt = OPT_GET(opt_name);
		}
		catch (agi::OptionErrorNotFound const& e) {
			return luaL_error(L, "%s", e.GetMessage().c_str());
		}

		switch (old_opt->GetType()) {
			case agi::OptionValue::Type_String:
				new_opt = new agi::OptionValueString(opt_name, read_value<std::string>(L, 3));
				break;
			case agi::OptionValue::Type_Int:
				new_opt = new agi::OptionValueInt(opt_name, read_value<int64_t>(L, 3));
				break;
			case agi::OptionValue::Type_Double:
				new_opt = new agi::OptionValueDouble(opt_name, read_value<double>(L, 3));
				break;
			case agi::OptionValue::Type_Colour:
				new_opt = new agi::OptionValueColour(opt_name, read_value<agi::Colour>(L, 3));
				break;
			case agi::OptionValue::Type_Bool:
				new_opt = new agi::OptionValueBool(opt_name, read_value<bool>(L, 3));
				break;
			case agi::OptionValue::Type_List_String:
				new_opt = new agi::OptionValueListString(opt_name, read_table<std::string>(L, 3));
				break;
			case agi::OptionValue::Type_List_Int:
				new_opt = new agi::OptionValueListInt(opt_name, read_table<int64_t>(L, 3));
				break;
			case agi::OptionValue::Type_List_Double:
				new_opt = new agi::OptionValueListDouble(opt_name, read_table<double>(L, 3));
				break;
			case agi::OptionValue::Type_List_Colour:
				new_opt = new agi::OptionValueListColour(opt_name, read_table<agi::Colour>(L, 3));
				break;
			case agi::OptionValue::Type_List_Bool:
				new_opt = new agi::OptionValueListBool(opt_name, read_table<bool>(L, 3));
				break;
			default:
				return luaL_error(L, "Unknown option type");
		}

		o->SetOption(new_opt);
		return 0;
	}

	int on_gc(lua_State *L) {
		static_cast<Automation4::LuaOptions*>(luaL_checkudata(L, 1, "aegisub.options"))->~LuaOptions();
		return 0;
	}
}

namespace Automation4 {

LuaOptions *LuaOptions::Init(lua_State *L) {
	LuaOptions *o = (LuaOptions*)lua_newuserdata(L, sizeof(LuaOptions));
	new(o) LuaOptions();

	if (luaL_newmetatable(L, "aegisub.options")) {
		set_field(L, "__index", get_option);
		set_field(L, "__newindex", set_option);
		set_field(L, "__gc", on_gc);
	}

	lua_setmetatable(L, -2);
	return o;
}

LuaOptions::~LuaOptions() {
	Cancel();
}

void LuaOptions::SetOption(agi::OptionValue *opt) {
	std::string name = opt->GetName();
	if (pending.count(name))
		delete pending[name];
	pending[name] = opt;
}

const agi::OptionValue *LuaOptions::GetOption(std::string const& name) const {
	std::map<std::string, agi::OptionValue*>::const_iterator it = pending.find(name);
	if (it != pending.end())
		return it->second;
	return OPT_GET(name);
}

void LuaOptions::Apply() {
	for (std::map<std::string, agi::OptionValue*>::iterator it = pending.begin(); it != pending.end(); ++it) {
		OPT_SET(it->first)->Set(it->second);
		delete it->second;
	}
	pending.clear();
}

void LuaOptions::Cancel() {
	for (std::map<std::string, agi::OptionValue*>::iterator it = pending.begin(); it != pending.end(); ++it) {
		delete it->second;
	}
}

}

#endif
