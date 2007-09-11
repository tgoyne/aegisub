// Copyright (c) 2005, Rodrigo Braz Monteiro, Niels Martin Hansen
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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


#pragma once


///////////
// Headers
#include <wx/wxprec.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/checklst.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <map>


////////////
// Typedefs
typedef std::map<wxString,wxSizer*> SizerMap;


//////////////
// Prototypes
class AssExporter;


///////////////////////
// Export dialog Class
class DialogExport : public wxDialog {
private:
	AssExporter *Export;
	SizerMap SetupMap;
	wxTextCtrl *Description;
	wxCheckListBox *FilterList;
	wxChoice *CharsetList;
	wxSizer *MainSizer;
	wxSizer *HorizSizer;
	wxSizer *OptionsSizer;

	void OnProcess(wxCommandEvent &event);
	void OnMoveUp(wxCommandEvent &event);
	void OnMoveDown(wxCommandEvent &event);
	void OnSelectAll(wxCommandEvent &event);
	void OnSelectNone(wxCommandEvent &event);
	void OnCheck(wxCommandEvent &event);
	void OnChange(wxCommandEvent &event);
	void RefreshOptions();

public:
	DialogExport(wxWindow *parent);
	~DialogExport();

	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {
	Button_Process = 1400,
	Button_Move_Up,
	Button_Move_Down,
	Button_Select_All,
	Button_Select_None,
	Filter_List_Box,
	Charset_List_Box
};
