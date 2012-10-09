// Copyright (c) 2005-2010, Niels Martin Hansen
// Copyright (c) 2005-2010, Rodrigo Braz Monteiro
// Copyright (c) 2010, Amar Takhar
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file edit.cpp
/// @brief edit/ commands.
/// @ingroup command
///

#include "../config.h"

#ifndef AGI_PRE
#include <algorithm>

#include <wx/clipbrd.h>
#endif

#include "command.h"

#include "../ass_dialogue.h"
#include "../ass_file.h"
#include "../ass_karaoke.h"
#include "../ass_override.h"
#include "../ass_style.h"
#include "../dialog_colorpicker.h"
#include "../dialog_search_replace.h"
#include "../include/aegisub/context.h"
#include "../subs_edit_ctrl.h"
#include "../subs_grid.h"
#include "../text_selection_controller.h"
#include "../video_context.h"

namespace {
	using cmd::Command;
/// @defgroup cmd-edit Editing commands.
/// @{

struct validate_sel_nonempty : public Command {
	CMD_TYPE(COMMAND_VALIDATE)
	bool Validate(const agi::Context *c) {
		return c->selectionController->GetSelectedSet().size() > 0;
	}
};

struct validate_sel_multiple : public Command {
	CMD_TYPE(COMMAND_VALIDATE)
	bool Validate(const agi::Context *c) {
		return c->selectionController->GetSelectedSet().size() > 1;
	}
};

template<class T>
T get_value(AssDialogue const& line, int blockn, T initial, wxString const& tag, wxString alt = wxString()) {
	for (int i = blockn; i >= 0; i--) {
		AssDialogueBlockOverride *ovr = dynamic_cast<AssDialogueBlockOverride*>(line.Blocks[i]);
		if (!ovr) continue;

		for (int j = (int)ovr->Tags.size() - 1; j >= 0; j--) {
			if (ovr->Tags[j]->Name == tag || ovr->Tags[j]->Name == alt) {
				return ovr->Tags[j]->Params[0]->Get<T>(initial);
			}
		}
	}
	return initial;
}

/// Get the block index in the text of the position
int block_at_pos(wxString const& text, int pos) {
	int n = 0;
	int max = text.size() - 1;
	for (int i = 0; i <= pos && i <= max; ++i) {
		if (i > 0 && text[i] == '{')
			n++;
		if (text[i] == '}' && i != max && i != pos && i != pos -1 && (i+1 == max || text[i+1] != '{'))
			n++;
	}

	return n;
}

void set_tag(const agi::Context *c, wxString const& tag, wxString const& value, bool at_end = false) {
	AssDialogue * const line = c->selectionController->GetActiveLine();
	if (line->Blocks.empty())
		line->ParseASSTags();

	int sel_start = c->textSelectionController->GetSelectionStart();
	int sel_end = c->textSelectionController->GetSelectionEnd();
	int start = at_end ? sel_end : sel_start;
	int blockn = block_at_pos(line->Text, start);

	AssDialogueBlockPlain *plain = 0;
	AssDialogueBlockOverride *ovr = 0;
	while (blockn >= 0) {
		AssDialogueBlock *block = line->Blocks[blockn];
		if (dynamic_cast<AssDialogueBlockDrawing*>(block))
			--blockn;
		else if ((plain = dynamic_cast<AssDialogueBlockPlain*>(block))) {
			// Cursor is in a comment block, so try the previous block instead
			if (plain->GetText().StartsWith("{")) {
				--blockn;
				start = line->Text.rfind('{', start);
			}
			else
				break;
		}
		else {
			ovr = dynamic_cast<AssDialogueBlockOverride*>(block);
			assert(ovr);
			break;
		}
	}

	// If we didn't hit a suitable block for inserting the override just put
	// it at the beginning of the line
	if (blockn < 0)
		start = 0;

	wxString insert = tag + value;
	int shift = insert.size();
	if (plain || blockn < 0) {
		line->Text = line->Text.Left(start) + "{" + insert + "}" + line->Text.Mid(start);
		shift += 2;
		line->ParseASSTags();
	}
	else if(ovr) {
		wxString alt;
		if (tag == "\\c") alt = "\\1c";
		// Remove old of same
		bool found = false;
		for (size_t i = 0; i < ovr->Tags.size(); i++) {
			wxString name = ovr->Tags[i]->Name;
			if (tag == name || alt == name) {
				shift -= ((wxString)*ovr->Tags[i]).size();
				if (found) {
					delete ovr->Tags[i];
					ovr->Tags.erase(ovr->Tags.begin() + i);
					i--;
				}
				else {
					ovr->Tags[i]->Params[0]->Set(value);
					ovr->Tags[i]->Params[0]->omitted = false;
					found = true;
				}
			}
		}
		if (!found)
			ovr->AddTag(insert);

		line->UpdateText();
	}
	else
		assert(false);

	if (!at_end)
		c->textSelectionController->SetSelection(sel_start + shift, sel_end + shift);
}

void set_text(AssDialogue *line, wxString const& value) {
	line->Text = value;
}

void commit_text(agi::Context const * const c, wxString const& desc, int *commit_id = 0) {
	SubtitleSelection const& sel = c->selectionController->GetSelectedSet();
	for_each(sel.begin(), sel.end(),
		bind(set_text, std::tr1::placeholders::_1, c->selectionController->GetActiveLine()->Text));
	int new_commit_id = c->ass->Commit(desc, AssFile::COMMIT_DIAG_TEXT, commit_id ? *commit_id : -1, sel.size() == 1 ? *sel.begin() : 0);
	if (commit_id)
		*commit_id = new_commit_id;
}

void toggle_override_tag(const agi::Context *c, bool (AssStyle::*field), const char *tag, wxString const& undo_msg) {
	AssDialogue *const line = c->selectionController->GetActiveLine();
	AssStyle const* const style = c->ass->GetStyle(line->Style);
	bool state = style ? style->*field : AssStyle().*field;

	line->ParseASSTags();
	int sel_start = c->textSelectionController->GetSelectionStart();
	int sel_end = c->textSelectionController->GetSelectionEnd();
	int blockn = block_at_pos(line->Text, sel_start);

	state = get_value(*line, blockn, state, tag);

	set_tag(c, tag, state ? "0" : "1");
	if (sel_start != sel_end)
		set_tag(c, tag, state ? "1" : "0", true);

	line->ClearBlocks();
	commit_text(c, undo_msg);
}

void got_color(const agi::Context *c, const char *tag, int *commit_id, wxColour new_color) {
	if (new_color.Ok()) {
		set_tag(c, tag, AssColor(new_color).GetASSFormatted(false));
		commit_text(c, _("set color"), commit_id);
	}
}

void show_color_picker(const agi::Context *c, AssColor (AssStyle::*field), const char *tag, const char *alt) {
	AssDialogue *const line = c->selectionController->GetActiveLine();
	AssStyle const* const style = c->ass->GetStyle(line->Style);
	wxColor color = (style ? style->*field : AssStyle().*field).GetWXColor();

	line->ParseASSTags();

	int sel_start = c->textSelectionController->GetSelectionStart();
	int sel_end = c->textSelectionController->GetSelectionEnd();
	int blockn = block_at_pos(line->Text, sel_start);

	color = get_value(*line, blockn, color, tag, alt);
	int commit_id = -1;
	const wxColor newColor = GetColorFromUser(c->parent, color, bind(got_color, c, tag, &commit_id, std::tr1::placeholders::_1));
	line->ClearBlocks();
	commit_text(c, _("set color"), &commit_id);

	if (!newColor.IsOk()) {
		c->ass->Undo();
		c->textSelectionController->SetSelection(sel_start, sel_end);
	}
}

struct edit_color_primary : public Command {
	CMD_NAME("edit/color/primary")
	STR_MENU("Primary Color...")
	STR_DISP("Primary Color")
	STR_HELP("Primary Color")

	void operator()(agi::Context *c) {
		show_color_picker(c, &AssStyle::primary, "\\c", "\\1c");
	}
};

struct edit_color_secondary : public Command {
	CMD_NAME("edit/color/secondary")
	STR_MENU("Secondary Color...")
	STR_DISP("Secondary Color")
	STR_HELP("Secondary Color")

	void operator()(agi::Context *c) {
		show_color_picker(c, &AssStyle::secondary, "\\c", "\\1c");
	}
};

struct edit_color_outline : public Command {
	CMD_NAME("edit/color/outline")
	STR_MENU("Outline Color...")
	STR_DISP("Outline Color")
	STR_HELP("Outline Color")

	void operator()(agi::Context *c) {
		show_color_picker(c, &AssStyle::outline, "\\c", "\\1c");
	}
};

struct edit_color_shadow : public Command {
	CMD_NAME("edit/color/shadow")
	STR_MENU("Shadow Color...")
	STR_DISP("Shadow Color")
	STR_HELP("Shadow Color")

	void operator()(agi::Context *c) {
		show_color_picker(c, &AssStyle::shadow, "\\c", "\\1c");
	}
};

struct edit_style_bold : public Command {
	CMD_NAME("edit/style/bold")
	STR_MENU("Bold")
	STR_DISP("Bold")
	STR_HELP("Bold")

	void operator()(agi::Context *c) {
		toggle_override_tag(c, &AssStyle::bold, "\\b", _("toggle bold"));
	}
};

struct edit_style_italic : public Command {
	CMD_NAME("edit/style/italic")
	STR_MENU("Italics")
	STR_DISP("Italics")
	STR_HELP("Italics")

	void operator()(agi::Context *c) {
		toggle_override_tag(c, &AssStyle::italic, "\\i", _("toggle italic"));
	}
};

struct edit_style_underline : public Command {
	CMD_NAME("edit/style/underline")
	STR_MENU("Underline")
	STR_DISP("Underline")
	STR_HELP("Underline")

	void operator()(agi::Context *c) {
		toggle_override_tag(c, &AssStyle::underline, "\\u", _("toggle underline"));
	}
};

struct edit_style_strikeout : public Command {
	CMD_NAME("edit/style/strikeout")
	STR_MENU("Strikeout")
	STR_DISP("Strikeout")
	STR_HELP("Strikeout")

	void operator()(agi::Context *c) {
		toggle_override_tag(c, &AssStyle::strikeout, "\\s", _("toggle strikeout"));
	}
};

struct edit_font : public Command {
	CMD_NAME("edit/font")
	STR_MENU("Font Face...")
	STR_DISP("Font Face")
	STR_HELP("Font Face")

	void operator()(agi::Context *c) {
		AssDialogue *const line = c->selectionController->GetActiveLine();
		line->ParseASSTags();
		const int blockn = block_at_pos(line->Text, c->textSelectionController->GetInsertionPoint());

		const AssStyle *style = c->ass->GetStyle(line->Style);
		const AssStyle default_style;
		if (!style)
			style = &default_style;

		const wxFont startfont(
			get_value(*line, blockn, (int)style->fontsize, "\\fs"),
			wxFONTFAMILY_DEFAULT,
			get_value(*line, blockn, style->italic, "\\i") ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
			get_value(*line, blockn, style->bold, "\\b") ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
			get_value(*line, blockn, style->underline, "\\u"),
			get_value(*line, blockn, style->font, "\\fn"));

		const wxFont font = wxGetFontFromUser(c->parent, startfont);
		if (!font.Ok() || font == startfont) {
			line->ClearBlocks();
			return;
		}

		if (font.GetFaceName() != startfont.GetFaceName())
			set_tag(c, "\\fn", font.GetFaceName());
		if (font.GetPointSize() != startfont.GetPointSize())
			set_tag(c, "\\fs", wxString::Format("%d", font.GetPointSize()));
		if (font.GetWeight() != startfont.GetWeight())
			set_tag(c, "\\b", wxString::Format("%d", font.GetWeight() == wxFONTWEIGHT_BOLD));
		if (font.GetStyle() != startfont.GetStyle())
			set_tag(c, "\\i", wxString::Format("%d", font.GetStyle() == wxFONTSTYLE_ITALIC));
		if (font.GetUnderlined() != startfont.GetUnderlined())
			set_tag(c, "\\i", wxString::Format("%d", font.GetUnderlined()));

		line->ClearBlocks();
		commit_text(c, _("set font"));
	}
};

/// Find and replace words in subtitles.
struct edit_find_replace : public Command {
	CMD_NAME("edit/find_replace")
	STR_MENU("Find and R&eplace...")
	STR_DISP("Find and Replace")
	STR_HELP("Find and replace words in subtitles")

	void operator()(agi::Context *c) {
		c->videoController->Stop();
		Search.OpenDialog(true);
	}
};

/// Copy subtitles.
struct edit_line_copy : public validate_sel_nonempty {
	CMD_NAME("edit/line/copy")
	STR_MENU("&Copy Lines")
	STR_DISP("Copy Lines")
	STR_HELP("Copy subtitles")

	void operator()(agi::Context *c) {
		// Ideally we'd let the control's keydown handler run and only deal
		// with the events not processed by it, but that doesn't seem to be
		// possible with how wx implements key event handling - the native
		// platform processing is evoked only if the wx event is unprocessed,
		// and there's no way to do something if the native platform code leaves
		// it unprocessed

		if (wxTextEntryBase *ctrl = dynamic_cast<wxTextEntryBase*>(c->parent->FindFocus()))
			ctrl->Copy();
		else
			c->subsGrid->CopyLines(c->subsGrid->GetSelection());
	}
};


/// Cut subtitles.
struct edit_line_cut: public validate_sel_nonempty {
	CMD_NAME("edit/line/cut")
	STR_MENU("Cu&t Lines")
	STR_DISP("Cut Lines")
	STR_HELP("Cut subtitles")

	void operator()(agi::Context *c) {
		if (wxTextEntryBase *ctrl = dynamic_cast<wxTextEntryBase*>(c->parent->FindFocus()))
			ctrl->Cut();
		else
			c->subsGrid->CutLines(c->subsGrid->GetSelection());
	}
};


/// Delete currently selected lines.
struct edit_line_delete : public validate_sel_nonempty {
	CMD_NAME("edit/line/delete")
	STR_MENU("De&lete Lines")
	STR_DISP("Delete Lines")
	STR_HELP("Delete currently selected lines")

	void operator()(agi::Context *c) {
		c->subsGrid->DeleteLines(c->subsGrid->GetSelection());
	}
};

struct in_selection : public std::unary_function<AssEntry*, bool> {
	SubtitleSelectionController::Selection const& sel;
	in_selection(SubtitleSelectionController::Selection const& sel) : sel(sel) { }
	bool operator()(AssEntry *e) const {
		AssDialogue *d = dynamic_cast<AssDialogue*>(e);
		return d && sel.count(d);
	}
};

static void duplicate_lines(agi::Context *c, bool shift) {
	in_selection sel(c->selectionController->GetSelectedSet());
	SubtitleSelectionController::Selection new_sel;
	AssDialogue *new_active = 0;

	std::list<AssEntry*>::iterator start = c->ass->Line.begin();
	std::list<AssEntry*>::iterator end = c->ass->Line.end();
	while (start != end) {
		// Find the first line in the selection
		start = find_if(start, end, sel);
		if (start == end) break;

		// And the last line in this contiguous selection
		std::list<AssEntry*>::iterator insert_pos = find_if(start, end, std::not1(sel));
		std::list<AssEntry*>::iterator last = insert_pos;
		--last;

		// Duplicate each of the selected lines, inserting them in a block
		// after the selected block
		do {
			AssDialogue *new_diag = static_cast<AssDialogue*>((*start)->Clone());

			c->ass->Line.insert(insert_pos, new_diag);
			new_sel.insert(new_diag);
			if (!new_active)
				new_active = new_diag;

			if (shift) {
				int pos = c->videoController->FrameAtTime(new_diag->End, agi::vfr::END) + 1;
				new_diag->Start = c->videoController->TimeAtFrame(pos, agi::vfr::START);
				new_diag->End = c->videoController->TimeAtFrame(pos, agi::vfr::END);
			}
		} while (start++ != last);

		// Skip over the lines we just made
		start = insert_pos;
	}

	if (new_sel.empty()) return;

	c->ass->Commit(_("duplicate lines"), AssFile::COMMIT_DIAG_ADDREM);

	c->selectionController->SetSelectionAndActive(new_sel, new_active);
}

/// Duplicate the selected lines.
struct edit_line_duplicate : public validate_sel_nonempty {
	CMD_NAME("edit/line/duplicate")
	STR_MENU("&Duplicate Lines")
	STR_DISP("Duplicate Lines")
	STR_HELP("Duplicate the selected lines")

	void operator()(agi::Context *c) {
		duplicate_lines(c, false);
	}
};


/// Duplicate lines and shift by one frame.
struct edit_line_duplicate_shift : public Command {
	CMD_NAME("edit/line/duplicate/shift")
	STR_MENU("D&uplicate and Shift by 1 Frame")
	STR_DISP("Duplicate and Shift by 1 Frame")
	STR_HELP("Duplicate lines and shift by one frame")
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) {
		return !c->selectionController->GetSelectedSet().empty() && c->videoController->IsLoaded();
	}

	void operator()(agi::Context *c) {
		duplicate_lines(c, true);
	}
};

static void combine_lines(agi::Context *c, void (*combiner)(AssDialogue *, AssDialogue *), wxString const& message) {
	SubtitleSelection sel = c->selectionController->GetSelectedSet();

	AssDialogue *first = 0;
	entryIter out = c->ass->Line.begin();
	for (entryIter it = c->ass->Line.begin(); it != c->ass->Line.end(); ++it) {
		AssDialogue *diag = dynamic_cast<AssDialogue*>(*it);
		if (!diag || !sel.count(diag)) {
			*out++ = *it;
			continue;
		}
		if (!first) {
			first = diag;
			*out++ = *it;
			continue;
		}

		combiner(first, diag);

		first->End = std::max(first->End, diag->End);
		delete diag;
	}

	c->ass->Line.erase(out, c->ass->Line.end());
	sel.clear();
	sel.insert(first);
	c->selectionController->SetSelectionAndActive(sel, first);
	c->ass->Commit(message, AssFile::COMMIT_DIAG_ADDREM | AssFile::COMMIT_DIAG_FULL);
}

static void combine_karaoke(AssDialogue *first, AssDialogue *second) {
	first->Text += wxString::Format("{\\k%d}%s", (second->Start - first->End) / 10, second->Text);
}

static void combine_concat(AssDialogue *first, AssDialogue *second) {
	first->Text += " " + second->Text;
}

static void combine_drop(AssDialogue *, AssDialogue *) { }

/// Joins selected lines in a single one, as karaoke.
struct edit_line_join_as_karaoke : public validate_sel_multiple {
	CMD_NAME("edit/line/join/as_karaoke")
	STR_MENU("As &Karaoke")
	STR_DISP("As Karaoke")
	STR_HELP("Joins selected lines in a single one, as karaoke")

	void operator()(agi::Context *c) {
		combine_lines(c, combine_karaoke, _("join as karaoke"));
	}
};


/// Joins selected lines in a single one, concatenating text together.
struct edit_line_join_concatenate : public validate_sel_multiple {
	CMD_NAME("edit/line/join/concatenate")
	STR_MENU("&Concatenate")
	STR_DISP("Concatenate")
	STR_HELP("Joins selected lines in a single one, concatenating text together")

	void operator()(agi::Context *c) {
		combine_lines(c, combine_concat, _("join lines"));
	}
};


/// Joins selected lines in a single one, keeping text of first and discarding remaining.
struct edit_line_join_keep_first : public validate_sel_multiple {
	CMD_NAME("edit/line/join/keep_first")
	STR_MENU("Keep &First")
	STR_DISP("Keep First")
	STR_HELP("Joins selected lines in a single one, keeping text of first and discarding remaining")

	void operator()(agi::Context *c) {
		combine_lines(c, combine_drop, _("join lines"));
	}
};


/// Paste subtitles.
struct edit_line_paste : public Command {
	CMD_NAME("edit/line/paste")
	STR_MENU("&Paste Lines")
	STR_DISP("Paste Lines")
	STR_HELP("Paste subtitles")
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *) {
		if (wxTheClipboard->Open()) {
			bool can_paste = wxTheClipboard->IsSupported(wxDF_TEXT);
			wxTheClipboard->Close();
			return can_paste;
		}
		return false;
	}

	void operator()(agi::Context *c) {
		if (wxTextEntryBase *ctrl = dynamic_cast<wxTextEntryBase*>(c->parent->FindFocus()))
			ctrl->Paste();
		else
			c->subsGrid->PasteLines(c->subsGrid->GetFirstSelRow());
	}
};


/// Paste subtitles over others.
struct edit_line_paste_over : public Command {
	CMD_NAME("edit/line/paste/over")
	STR_MENU("Paste Lines &Over...")
	STR_DISP("Paste Lines Over")
	STR_HELP("Paste subtitles over others")
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) {
		if (wxTheClipboard->Open()) {
			bool can_paste = wxTheClipboard->IsSupported(wxDF_TEXT);
			wxTheClipboard->Close();
			return can_paste && c->selectionController->GetSelectedSet().size();
		}
		return false;
	}

	void operator()(agi::Context *c) {
		c->subsGrid->PasteLines(c->subsGrid->GetFirstSelRow(),true);
	}
};


/// Recombine subtitles when they have been split and merged.
struct edit_line_recombine : public validate_sel_multiple {
	CMD_NAME("edit/line/recombine")
	STR_MENU("Recom&bine Lines")
	STR_DISP("Recombine Lines")
	STR_HELP("Recombine subtitles when they have been split and merged")

	void operator()(agi::Context *c) {
		c->subsGrid->RecombineLines();
	}
};


/// Uses karaoke timing to split line into multiple smaller lines.
struct edit_line_split_by_karaoke : public validate_sel_nonempty {
	CMD_NAME("edit/line/split/by_karaoke")
	STR_MENU("Split Lines (by karaoke)")
	STR_DISP("Split Lines (by karaoke)")
	STR_HELP("Uses karaoke timing to split line into multiple smaller lines")

	void operator()(agi::Context *c) {
		AssKaraoke::SplitLines(c->selectionController->GetSelectedSet(), c);
	}
};

/// Redoes last action.
struct edit_redo : public Command {
	CMD_NAME("edit/redo")
	STR_HELP("Redoes last action")
	CMD_TYPE(COMMAND_VALIDATE | COMMAND_DYNAMIC_NAME)

	wxString StrMenu(const agi::Context *c) const {
		return c->ass->IsRedoStackEmpty() ?
			_("Nothing to &redo") :
		wxString::Format(_("&Redo %s"), c->ass->GetRedoDescription());
	}
	wxString StrDisplay(const agi::Context *c) const {
		return c->ass->IsRedoStackEmpty() ?
			_("Nothing to redo") :
		wxString::Format(_("Redo %s"), c->ass->GetRedoDescription());
	}

	bool Validate(const agi::Context *c) {
		return !c->ass->IsRedoStackEmpty();
	}

	void operator()(agi::Context *c) {
		c->ass->Redo();
	}
};

/// Undoes last action.
struct edit_undo : public Command {
	CMD_NAME("edit/undo")
	STR_HELP("Undoes last action")
	CMD_TYPE(COMMAND_VALIDATE | COMMAND_DYNAMIC_NAME)

	wxString StrMenu(const agi::Context *c) const {
		return c->ass->IsUndoStackEmpty() ?
			_("Nothing to &undo") :
			wxString::Format(_("&Undo %s"), c->ass->GetUndoDescription());
	}
	wxString StrDisplay(const agi::Context *c) const {
		return c->ass->IsUndoStackEmpty() ?
			_("Nothing to undo") :
			wxString::Format(_("Undo %s"), c->ass->GetUndoDescription());
	}

	bool Validate(const agi::Context *c) {
		return !c->ass->IsUndoStackEmpty();
	}

	void operator()(agi::Context *c) {
		c->ass->Undo();
	}
};

}
/// @}

namespace cmd {
	void init_edit() {
		reg(new edit_color_primary);
		reg(new edit_color_secondary);
		reg(new edit_color_outline);
		reg(new edit_color_shadow);
		reg(new edit_font);
		reg(new edit_find_replace);
		reg(new edit_line_copy);
		reg(new edit_line_cut);
		reg(new edit_line_delete);
		reg(new edit_line_duplicate);
		reg(new edit_line_duplicate_shift);
		reg(new edit_line_join_as_karaoke);
		reg(new edit_line_join_concatenate);
		reg(new edit_line_join_keep_first);
		reg(new edit_line_paste);
		reg(new edit_line_paste_over);
		reg(new edit_line_recombine);
		reg(new edit_line_split_by_karaoke);
		reg(new edit_style_bold);
		reg(new edit_style_italic);
		reg(new edit_style_underline);
		reg(new edit_style_strikeout);
		reg(new edit_redo);
		reg(new edit_undo);
	}
}
