// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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
// $Id$

/// @file auto4_lua_scriptreader.h
/// @see auto4_lua_scriptreader.cpp
/// @ingroup scripting
///

#ifndef AGI_PRE
#include <istream>
#endif

#include <libaegisub/scoped_ptr.h>

struct lua_State;
namespace agi { namespace charset { class IconvWrapper; } }
class wxString;

namespace Automation4 {
	class LuaScriptReader {
		agi::scoped_ptr<agi::charset::IconvWrapper> conv;
		agi::scoped_ptr<std::istream> file;
		char buf[512];

		const char *Read(size_t *bytes_read);
	public:
		LuaScriptReader(wxString const& filename);
		~LuaScriptReader();

		static const char* reader_func(lua_State *, void *data, size_t *size);
	};
};
