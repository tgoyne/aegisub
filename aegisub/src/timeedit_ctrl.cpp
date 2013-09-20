// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file timeedit_ctrl.cpp
/// @brief Edit-control for editing SSA-format timestamps
/// @ingroup custom_control
///

#include "config.h"

#include "timeedit_ctrl.h"

#include <functional>

#include "compat.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "utils.h"
#include "video_context.h"

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/menu.h>
#include <wx/valtext.h>

#define TimeEditWindowStyle

enum {
	Time_Edit_Copy = 1320,
	Time_Edit_Paste
};

using namespace agi::ass::time;

TimeEdit::TimeEdit(wxWindow* parent, wxWindowID id, agi::Context *c, const std::string& value, const wxSize& size, bool asEnd)
: wxTextCtrl(parent, id, to_wx(value), wxDefaultPosition, size, wxTE_CENTRE | wxTE_PROCESS_ENTER)
, c(c)
, byFrame(false)
, isEnd(asEnd)
, insert(!OPT_GET("Subtitle/Time Edit/Insert Mode")->GetBool())
, insert_opt(OPT_SUB("Subtitle/Time Edit/Insert Mode", &TimeEdit::OnInsertChanged, this))
{
	// Set validator
	wxTextValidator val(wxFILTER_INCLUDE_CHAR_LIST);
	wxString includes[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", ":" };
	val.SetIncludes(wxArrayString(11, includes));
	SetValidator(val);

	// Other stuff
	if (value.empty()) SetValue(to_wx(AssFormat(time)));

	Bind(wxEVT_COMMAND_MENU_SELECTED, std::bind(&TimeEdit::CopyTime, this), Time_Edit_Copy);
	Bind(wxEVT_COMMAND_MENU_SELECTED, std::bind(&TimeEdit::PasteTime, this), Time_Edit_Paste);
	Bind(wxEVT_COMMAND_TEXT_UPDATED, &TimeEdit::OnModified, this);
	Bind(wxEVT_CONTEXT_MENU, &TimeEdit::OnContextMenu, this);
	Bind(wxEVT_CHAR_HOOK, &TimeEdit::OnKeyDown, this);
	Bind(wxEVT_KILL_FOCUS, &TimeEdit::OnFocusLost, this);
}

void TimeEdit::SetTime(Time new_time) {
	if (time != new_time) {
		time = new_time;
		UpdateText();
	}
}

int TimeEdit::GetFrame() const {
	return c->videoController->FrameAtTime(time, isEnd ? agi::vfr::END : agi::vfr::START);
}

void TimeEdit::SetFrame(int fn) {
	SetTime(c->videoController->TimeAtFrame(fn, isEnd ? agi::vfr::END : agi::vfr::START));
}

void TimeEdit::SetByFrame(bool enableByFrame) {
	if (enableByFrame == byFrame) return;

	byFrame = enableByFrame && c->videoController->TimecodesLoaded();
	UpdateText();
}

void TimeEdit::OnModified(wxCommandEvent &event) {
	event.Skip();
	if (byFrame) {
		long temp = 0;
		GetValue().ToLong(&temp);
		time = c->videoController->TimeAtFrame(temp, isEnd ? agi::vfr::END : agi::vfr::START);
	}
	else if (insert)
		time = Time(from_wx(GetValue()));
}

void TimeEdit::UpdateText() {
	if (byFrame)
		ChangeValue(std::to_wstring(c->videoController->FrameAtTime(time, isEnd ? agi::vfr::END : agi::vfr::START)));
	else
		ChangeValue(to_wx(AssFormat(time)));
}

void TimeEdit::OnKeyDown(wxKeyEvent &event) {
	int key = event.GetKeyCode();
	if (event.CmdDown()) {
		if (key == 'C' || key == 'X')
			CopyTime();
		else if (key == 'V')
			PasteTime();
		else
			event.Skip();
		return;
	}

	// Shift-Insert would paste the stuff anyway
	// but no one updates the private "time" variable.
	if (event.ShiftDown() && key == WXK_INSERT) {
		PasteTime();
		return;
	}

	// Translate numpad presses to normal numbers
	if (key >= WXK_NUMPAD0 && key <= WXK_NUMPAD9)
		key += '0' - WXK_NUMPAD0;

	// If overwriting is disabled, we're in frame mode, or it's a key we
	// don't care about just let the standard processing happen
	event.Skip();
	if (byFrame) return;
	if (insert) return;
	if ((key < '0' || key > '9') && key != WXK_BACK && key != WXK_DELETE && key != ';' && key != '.') return;

	event.Skip(false);

	// Delete does nothing
	if (key == WXK_DELETE) return;

	long start = GetInsertionPoint();
	// Back just moves cursor back one without deleting
	if (key == WXK_BACK) {
		if (start > 0)
			SetInsertionPoint(start - 1);
		return;
	}

	std::string text = from_wx(GetValue());
	// Cursor is at the end so do nothing
	if (start >= (long)text.size()) return;

	// If the cursor is at punctuation, move it forward to the next digit
	if (text[start] == ':' || text[start] == '.')
		++start;

	// : and . hop over punctuation but never insert anything
	if (key == ';' || key == '.') {
		SetInsertionPoint(start);
		return;
	}

	// Overwrite the digit
	text[start] = (char)key;
	time = Time(text);
	SetValue(to_wx(AssFormat(time)));
	SetInsertionPoint(start + 1);
}

void TimeEdit::OnInsertChanged(agi::OptionValue const& opt) {
	insert = !opt.GetBool();
}

void TimeEdit::OnContextMenu(wxContextMenuEvent &evt) {
	if (byFrame || insert) {
		evt.Skip();
		return;
	}

	wxMenu menu;
	menu.Append(Time_Edit_Copy, _("&Copy"));
	menu.Append(Time_Edit_Paste, _("&Paste"));
	PopupMenu(&menu);
}

void TimeEdit::OnFocusLost(wxFocusEvent &evt) {
	if (insert || byFrame)
		UpdateText();
	evt.Skip();
}

void TimeEdit::CopyTime() {
	SetClipboard(from_wx(GetValue()));
}

void TimeEdit::PasteTime() {
	if (byFrame) {
		Paste();
		return;
	}

	std::string text(GetClipboard());
	if (text.empty()) return;

	Time tempTime(text);
	if (AssFormat(tempTime) == text) {
		SetTime(tempTime);
		SetSelection(0, GetValue().size());

		wxCommandEvent evt(wxEVT_COMMAND_TEXT_UPDATED, GetId());
		evt.SetEventObject(this);
		HandleWindowEvent(evt);
	}
}
