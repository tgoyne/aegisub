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

/// @file auto4_lua_options.h
/// @see auto4_lua_options.cpp
/// @ingroup scripting
/// @brief auto4lua access to Aegisub options

#ifndef AGI_PRE
#include <deque>
#include <map>
#include <string>
#endif

struct lua_State;
namespace agi { class OptionValue; }

namespace Automation4 {

/// @brief Lua bindings for Aegisub's options
///
/// This does not apply changes made immediately due to that changing options
/// from threads other than the GUI thread may not be safe, as option change
/// listeners may try to do GUI-related things. In addition, delaying the
/// application helps avoid any side effects from canceled/failed macros.
class LuaOptions {
	/// Options which will be set when Apply is called
	std::map<std::string, agi::OptionValue*> pending;

public:
	/// Create a new LuaOptions object and leave it on top of the stack
	/// @param L Lua state
	/// @return The new LuaOptions object
	static LuaOptions *Init(lua_State *L);

	/// Destructor
	~LuaOptions();

	/// Add an option to be changed when Apply is called
	void SetOption(agi::OptionValue *opt);

	/// Get an option value from the pending set if present, or from main set if not
	/// @param name Name of the option
	const agi::OptionValue *GetOption(std::string const& name) const;

	/// Apply all pending changes
	void Apply();

	/// Cancel all pending changes
	void Cancel();
};

}
