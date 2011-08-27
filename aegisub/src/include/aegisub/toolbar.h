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

/// @file toolbar.h
/// @brief Dynamic toolbar generator.
/// @ingroup menu toolbar

#ifndef AGI_PRE
#include <string>
#endif

namespace agi { struct Context; }
class wxFrame;
class wxToolBar;
class wxWindow;

namespace toolbar {
	/// Add the named toolbar to a window
	/// @param frame Frame to attach the toolbar to
	/// @param name Name of the toolbar
	/// @param context Project context
	/// @param hotkey Hotkey context for the tooltip
	void AttachToolbar(wxFrame *frame, std::string const& name, agi::Context *context, std::string const& hotkey);
	wxToolBar *GetToolbar(wxWindow *parent, std::string const& name, agi::Context *context, std::string const& hotkey);
}
