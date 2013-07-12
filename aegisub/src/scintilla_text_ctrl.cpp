// Copyright (c) 2007, Rodrigo Braz Monteiro
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

/// @file scintilla_text_ctrl.cpp
/// @brief Customised version of wxStyledTextControl used for main edit box
/// @ingroup custom_control
///

#include "config.h"

#include "scintilla_text_ctrl.h"
#include "utils.h"

ScintillaTextCtrl::ScintillaTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style)
: wxTextCtrl(parent, id, value, pos, size, style)
{
	SetWrapMode(wxSTC_WRAP_WORD);
	SetMarginWidth(1, 0);
	UsePopUp(false);

	// Disable Scintilla's default hotkeys
	CmdKeyClear(wxSTC_KEY_RETURN, wxSTC_SCMOD_CTRL);
	CmdKeyClear(wxSTC_KEY_RETURN, wxSTC_SCMOD_SHIFT);
	CmdKeyClear(wxSTC_KEY_RETURN, wxSTC_SCMOD_NORM);
	CmdKeyClear(wxSTC_KEY_TAB, wxSTC_SCMOD_NORM);
	CmdKeyClear(wxSTC_KEY_TAB, wxSTC_SCMOD_SHIFT);
	CmdKeyClear('D', wxSTC_SCMOD_CTRL);
	CmdKeyClear('L', wxSTC_SCMOD_CTRL);
	CmdKeyClear('L', wxSTC_SCMOD_CTRL | wxSTC_SCMOD_SHIFT);
	CmdKeyClear('T', wxSTC_SCMOD_CTRL);
	CmdKeyClear('T', wxSTC_SCMOD_CTRL | wxSTC_SCMOD_SHIFT);
	CmdKeyClear('U', wxSTC_SCMOD_CTRL);

	Bind(wxEVT_MOUSEWHEEL, &ScintillaTextCtrl::OnMouseWheel, this);
}

int ScintillaTextCtrl::GetUnicodePosition(int pos) {
	return pos;
}

int ScintillaTextCtrl::GetReverseUnicodePosition(int pos) {
	return pos;
}

void ScintillaTextCtrl::StartUnicodeStyling(int start,int mask) {
}

void ScintillaTextCtrl::SetUnicodeStyling(int start,int length,int style) {
}

void ScintillaTextCtrl::SetSelectionU(int start, int end) {
	SetSelection(start, end);
}

void ScintillaTextCtrl::SetText() {
	SetEvtHandlerEnabled(false);
	Freeze();

	int from = GetReverseUnicodePosition(GetSelectionStart());
	int to = GetReverseUnicodePosition(GetSelectionEnd());

	line_text.clear();
	SetText(text);

	SetSelectionU(from, to);

	SetEvtHandlerEnabled(true);
	Thaw();
}

void ScintillaTextCtrl::OnMouseWheel(wxMouseEvent& evt) {
	if (ForwardMouseWheelEvent(this, evt)) {
		// Skip the event so that wxSTC's default mouse wheel handler is hit
		evt.Skip();
	}
}
