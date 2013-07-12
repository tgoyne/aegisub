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

/// @file scintilla_text_ctrl.h
/// @see scintilla_text_ctrl.cpp
/// @ingroup custom_control
///

#include <wx/stc/stc.h>

#include <string>

class ScintillaTextCtrl : public wxControl {
	wxString text;

	void OnMouseWheel(wxMouseEvent& evt);
public:
	// All in bytes
	std::pair<int, int> GetSelection() const;
	int GetSelectionStart() const;
	int GetSelectionEnd() const;
	int GetInsertionPoint() const;

	void SetSelection(int from, int to);
	void SetInsertionPoint(int pos);

	std::string GetText() const;
	void Append(std::string const& text);
	void Insert(std::string const& text); 
	void SetText(std::string const& text, bool preserve_selection=false);

	void Cut();
	void Copy() const;
	void Paste();

	void HideCallTip();
	void ShowCallTip(int pos, int highlight_start, int highlight_end, std::string const& text);

	ScintillaTextCtrl(wxWindow* parent, const wxString& value = wxString(), const wxSize& size = wxDefaultSize, long style = 0, bool read_only = false);
};
