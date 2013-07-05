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

#include "config.h"

#include "style_list.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "charset_detect.h"
#include "compat.h"
#include "dialog_selected_choices.h"
#include "libresrc/libresrc.h"
#include "style_manager.h"
#include "subtitle_format.h"
#include "utils.h"

#include <libaegisub/of_type_adaptor.h>
#include <libaegisub/util.h>

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/tokenizer.hpp>
#include <functional>

#include <wx/bmpbuttn.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

namespace {
	int get_single_sel(wxListBox *lb) {
		wxArrayInt selections;
		int n = lb->GetSelections(selections);
		return n == 1 ? selections[0] : -1;
	}

	template<typename F>
	void horizontal(wxSizer *sizer, wxSizerFlags flags, F&& func) {
		auto new_sizer = new wxBoxSizer(wxHORIZONTAL);
		sizer->Add(new_sizer, flags);
		func(new_sizer);
	}

	template<typename F>
	void vertical(wxSizer *sizer, wxSizerFlags flags, F&& func) {
		auto new_sizer = new wxBoxSizer(wxVERTICAL);
		sizer->Add(new_sizer, flags);
		func(new_sizer);
	}
}

StyleList::StyleList(StyleManager *manager, StyleManager *other_manager, StyleListOptions const& options)
: manager(manager)
, other_manager(other_manager)
, options(options)
, styles_changed_connection(manager->AddStylesChangedListener(&StyleList::OnStylesChanged, this))
, selection_changed_connection(manager->AddSelectionListener(&StyleList::OnSetSelection, this))
{
	auto main_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Current script"));

	horizontal(main_sizer, wxSizerFlags(1).Expand().Border(wxBOTTOM), [&](wxSizer *sizer) {
		sizer->Add(list_ctrl = new wxListBox(this, -1, wxDefaultPosition, wxSize(240,250), 0, nullptr, wxLB_EXTENDED), wxSizerFlags(1).Expand());
		vertical(sizer, wxSizerFlags().Expand(), [&](wxSizer *) {
			auto add_bitmap_button = [&](wxBitmap const& img, wxString const& tooltip) -> wxBitmapButton * {
				wxBitmapButton *btn = new wxBitmapButton(this, -1, img);
				btn->SetToolTip(tooltip);
				sizer->Add(btn, wxSizerFlags().Expand());
				return btn;
			};

			sizer->AddStretchSpacer(1);
			move_up_btn     = add_bitmap_button(GETIMAGE(arrow_up_24), _("Move style up"));
			move_down_btn   = add_bitmap_button(GETIMAGE(arrow_down_24), _("Move style down"));
			move_top_btn    = add_bitmap_button(GETIMAGE(arrow_up_stop_24), _("Move style to top"));
			move_bottom_btn = add_bitmap_button(GETIMAGE(arrow_down_stop_24), _("Move style to bottom"));
			sort_btn        = add_bitmap_button(GETIMAGE(arrow_sort_24), _("Sort styles alphabetically"));
			sizer->AddStretchSpacer(1);
		});
	});

	horizontal(main_sizer, wxSizerFlags(1).Expand().Border(wxBOTTOM), [&](wxSizer *sizer) {
		auto import_btn = new wxButton(this, -1, _("&Import from script..."));
		copy_to_other_btn = new wxButton(this, -1, options.copy_to_other_label);

		if (options.import_button_on_left)
			sizer->Add(import_btn, wxSizerFlags(1).Expand().Border(wxRIGHT));
		sizer->Add(copy_to_other_btn, wxSizerFlags(1).Expand());
		if (!options.import_button_on_left)
			sizer->Add(import_btn, wxSizerFlags(1).Expand().Border(wxLEFT));
	});

	horizontal(main_sizer, wxSizerFlags(1).Expand(), [&](wxSizer *sizer) {
		new_btn = new wxButton(this, -1, _("&New"));
		edit_btn = new wxButton(this, -1, _("&Edit"));
		copy_btn = new wxButton(this, -1, _("&Copy"));
		delete_btn = new wxButton(this, -1, _("&Delete"));

		sizer->Add(new_btn, wxSizerFlags(1).Expand().Border(wxRIGHT));
		sizer->Add(edit_btn, wxSizerFlags(1).Expand().Border(wxRIGHT));
		sizer->Add(copy_btn, wxSizerFlags(1).Expand().Border(wxRIGHT));
		sizer->Add(delete_btn, wxSizerFlags(1).Expand());
	});

	list_ctrl->Bind(wxEVT_COMMAND_LISTBOX_SELECTED, std::bind(&StyleList::UpdateButtons, this));
	list_ctrl->Bind(wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, [&](wxCommandEvent& evt) { EditStyle(); });
	Bind(wxEVT_KEY_DOWN, &StyleList::OnKeyDown, this);
}

void StyleList::OnStylesChanged() {
	list_ctrl->Clear();
	list_ctrl->Append(to_wx(manager->GetStyleNames()));
	UpdateButtons();
}

void StyleList::OnSetSelection(std::vector<std::string> new_sel) {
	list_ctrl->DeselectAll();
	for (auto const& style_name : new_sel)
		list_ctrl->SetStringSelection(to_wx(style_name));
	UpdateButtons();
}

void StyleList::CopyToOther() {
	wxArrayInt selections;
	int n = list_ctrl->GetSelections(selections);

	std::vector<AssStyle *> to_copy;
	to_copy.reserve(n);

	for (auto sel : selections) {
		wxString wx_name = list_ctrl->GetString(sel);
		std::string name = from_wx(wx_name);

		if (other_manager->GetStyle(name)) {
			if (wxYES == wxMessageBox(wxString::Format(options.style_already_exists_overwrite_prompt, wx_name), _("Style name collision"), wxYES_NO))
				to_copy.push_back(manager->GetStyle(name));
		}
		else
			to_copy.push_back(manager->GetStyle(name));
	}

	other_manager->AddStyles(to_copy);
}

void StyleList::CopyToClipboard() {
	wxString data;
	wxArrayInt selections;
	list_ctrl->GetSelections(selections);

	for(size_t i = 0; i < selections.size(); ++i) {
		if (i) data += "\r\n";
		AssStyle *s = manager->GetStyle(from_wx(list_ctrl->GetString(selections[i])));
		s->UpdateData();
		data += to_wx(s->GetEntryData());
	}

	SetClipboard(data);
}

void StyleList::Paste() {
	std::vector<wxString> names;

	boost::char_separator<char> sep("\n");
	for (auto tok : boost::tokenizer<boost::char_separator<char>>(GetClipboard(), sep)) {
		boost::trim(tok);
		try {
			AssStyle s(tok);
			auto style = manager->Create(manager->GetStyle(s.name));
			s.name = style->name;
			*style = s;
			names.emplace_back(to_wx(s.name));
		}
		catch (...) {
			wxMessageBox(_("Could not parse style"), _("Could not parse style"), wxOK | wxICON_EXCLAMATION);
		}
	}

	manager->Save(_("style paste"));

	list_ctrl->DeselectAll();
	for (auto const& style_name : names)
		list_ctrl->SetStringSelection(style_name, true);
}

void StyleList::NewStyle() {
	ShowEditor(manager->Create());
}

void StyleList::EditStyle() {
	int sel = get_single_sel(list_ctrl);
	if (sel == -1) return;
	ShowEditor(styles[sel]);
}

void StyleList::CopyStyle() {
	int sel = get_single_sel(list_ctrl);
	if (sel == -1) return;

	ShowEditor(manager->Create(styles[sel]));
}

void StyleList::DeleteStyles() {
	wxArrayInt selections;
	list_ctrl->GetSelections(selections);

	for (auto sel : selections)
		manager->Delete(from_wx(list_ctrl->GetString(sel)));
	manager->Save(_("style delete"));
}

void StyleList::ImportFromFile() {
	auto filename = OpenFileSelector(_("Open subtitles file"), "Path/Last/Subtitles", "", "", SubtitleFormat::GetWildcards(0), this);
	if (filename.empty()) return;

	std::string charset;
	try {
		charset = CharSetDetect::GetEncoding(filename);
	}
	catch (agi::UserCancelException const&) {
		return;
	}

	auto reader = SubtitleFormat::GetReader(filename);
	if (!reader) {
		wxMessageBox("Unsupported subtitle format", "Error", wxOK | wxICON_ERROR | wxCENTER, this);
		return;
	}

	AssFile temp;
	try {
		reader->ReadFile(&temp, filename, charset);
	}
	catch (agi::Exception const& err) {
		wxMessageBox(to_wx(err.GetChainedMessage()), "Error", wxOK | wxICON_ERROR | wxCENTER, this);
		return;
	}
	catch (...) {
		wxMessageBox("Unknown error", "Error", wxOK | wxICON_ERROR | wxCENTER, this);
		return;
	}

	auto styles = temp.GetStyles();
	if (styles.empty()) {
		wxMessageBox(_("The selected file has no available styles."), _("Error Importing Styles"));
		return;
	}

	wxArrayInt selections;
	int res = GetSelectedChoices(GetParent(), selections, _("Choose styles to import:"), _("Import Styles"), to_wx(styles));
	if (res == -1 || selections.empty()) return;
	bool modified = false;

	for (auto const& sel : selections) {
		if (AssStyle *existing = manager->GetStyle(styles[sel])) {
			int answer = wxMessageBox(
				wxString::Format(_("There is already a style with the name \"%s\" in the current script. Overwrite?"), styles[sel]),
				_("Style name collision"),
				wxYES_NO);
			if (answer == wxYES) {
				modified = true;
				*existing = *temp.GetStyle(styles[sel]);
			}
			continue;
		}

		modified = true;
		manager->Create(temp.GetStyle(styles[sel]));
	}

	if (modified)
		manager->Save(_("style import"));
}

bool is_contiguous(wxArrayInt const& arr) {
	for (size_t i = 1; i < arr.size(); ++i) {
		if (arr[i] != arr[i-1]+1)
			return false;
	}
	return true;
}

void StyleList::UpdateButtons() {
	wxArrayInt sels;
	int n = list_ctrl->GetSelections(sels);

	edit_btn->Enable(n == 1);
	copy_btn->Enable(n == 1);
	delete_btn->Enable(n > 0);
	copy_to_other_btn->Enable(n > 0);

	int firstStor = -1;
	int lastStor = -1;
	if (n) {
		firstStor = sels[0];
		lastStor = sels[n-1];
	}

	bool contStor = is_contiguous(sels);
	int itemsStor = list_ctrl->GetCount();
	move_up_btn->Enable(contStor && firstStor > 0);
	move_top_btn->Enable(contStor && firstStor > 0);
	move_down_btn->Enable(contStor && lastStor != -1 && lastStor < itemsStor-1);
	move_bottom_btn->Enable(contStor && lastStor != -1 && lastStor < itemsStor-1);
	sort_btn->Enable(itemsStor > 1);
}

void StyleList::MoveStyles(int type) {
	wxArrayInt sels;
	int n = list_ctrl->GetSelections(sels);
	if (n == 0 && type != 4) return;

	std::vector<wxString> sel_string;
	for (auto sel : sels)
		sel_string.emplace_back(list_ctrl->GetString(sel));

	int first = 0, last = 0;
	if (n) {
		first = sels.front();
		last = sels.back();
	}

	manager->Move(type, first, last);

	list_ctrl->DeselectAll();
	for (auto const& sel : sel_string)
		list_ctrl->Select(list_ctrl->FindString(sel));
}

void StyleList::OnKeyDown(wxKeyEvent &event) {
	switch(event.GetKeyCode()) {
	case WXK_DELETE:
		DeleteStyles();
		break;

	case 'C': case 'c':
		if (event.CmdDown())
			CopyToClipboard();
		break;

	case 'V': case 'v':
		if (event.CmdDown())
			Paste();
		break;
	default:
		event.Skip();
		break;
	}
}

void StyleList::ShowEditor(AssStyle *style) {

}
