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

#include <string>
#include <vector>
#include <wx/control.h>

class AssDialogue;
class AssStyle;
class StyleManager;

struct StyleListOptions {
	wxString style_already_exists_overwrite_prompt;
	wxString box_header;
	wxString copy_to_other_label;
	bool import_button_on_left;
};

class StyleList : public wxControl {
	StyleManager *manager;
	StyleManager *other_manager;
	std::vector<AssStyle *> styles;
	StyleListOptions options;

	agi::signal::Connection styles_changed_connection;
	agi::signal::Connection selection_changed_connection;

	void OnStylesChanged();
	void OnSetSelection(std::vector<std::string> new_sel);

	wxListBox *list_ctrl;
	wxButton *new_btn;
	wxButton *edit_btn;
	wxButton *copy_btn;
	wxButton *delete_btn;
	wxButton *copy_to_other_btn;
	wxButton *move_up_btn;
	wxButton *move_down_btn;
	wxButton *move_top_btn;
	wxButton *move_bottom_btn;
	wxButton *sort_btn;

	void UpdateButtons();
	void ImportFromFile();
	void CopyToOther();
	void CopyToClipboard();
	void Paste();
	void NewStyle();
	void EditStyle();
	void CopyStyle();
	void DeleteStyles();
	void OnKeyDown(wxKeyEvent &event);
	void MoveStyles(int type);

	void ShowEditor(AssStyle *style);

	void OnActiveLineChanged(AssDialogue *new_line);


public:
	StyleList(StyleManager *manager, StyleManager *other_manager, StyleListOptions const& options);
	void SetManager(StyleManager *manager);
	void SetOther(StyleManager *manager);
};
