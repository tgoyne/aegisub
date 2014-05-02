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

#include <libaegisub/lua/utils.h>

#include "utils.h"

namespace {
	struct string {
		const char *data;
		size_t len;
	};
	struct library {
		std::string str;

		string return_string(std::string ret) {
			str = std::move(ret);
			return {str.data(), str.size()};
		}
	};
}

extern "C" library *auto5_init() {
	return new library;
}

extern "C" void auto5_deinit(library *lib) {
	delete lib;
}

extern "C" string auto5_clipboard_get(library *lib) {
	return lib->return_string(GetClipboard());
}

extern "C" bool auto5_clipboard_set(library *lib, const char *str, size_t len) {
	bool succeeded = false;

#if wxUSE_OLE
	// OLE needs to be initialized on each thread that wants to write to
	// the clipboard, which wx does not handle automatically
	wxClipboard cb;
#else
	wxClipboard &cb = *wxTheClipboard;
#endif
	if (cb.Open()) {
		succeeded = cb.SetData(new wxTextDataObject(wxString::FromUTF8(str, len)));
		cb.Close();
		cb.Flush();
	}
	return succeeded;
}