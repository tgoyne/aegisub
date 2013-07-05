// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

#include <libaegisub/signal.h>

#include <memory>
#include <wx/dialog.h>

namespace agi { struct Context; }
class AssStyle;
class PersistLocation;
class StyleList;
class StyleManager;

class DialogStyleManager : public wxDialog {
	agi::Context *c;
	std::unique_ptr<PersistLocation> persist;

	std::unique_ptr<StyleManager> current_manager;
	std::unique_ptr<StyleManager> storage_manager;

	StyleList *current_list;
	StyleList *storage_list;

	void ShowEditor(AssStyle *style, StyleManager *manager, wxListBox *list);
	void OnChangeCatalog(wxThreadEvent &evt);

public:
	DialogStyleManager(agi::Context *context);
	~DialogStyleManager();
};
