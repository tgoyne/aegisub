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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


///////////
// Headers
#include "dialog_properties.h"
#include "options.h"
#include "ass_file.h"
#include "video_display.h"
#include "validators.h"
#include "video_provider.h"


///////////////
// Constructor
DialogProperties::DialogProperties (wxWindow *parent, VideoDisplay *_vid)
: wxDialog(parent, -1, _("Script Properties"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	// Setup
	vid = _vid;
	AssFile *subs = AssFile::top;

	// Script details crap
	wxStaticText *TitleLabel = new wxStaticText(this,-1,_("Title:"));
	TitleEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Title")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *OrigScriptLabel = new wxStaticText(this,-1,_("Original script:"));
	OrigScriptEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Original Script")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *TranslationLabel = new wxStaticText(this,-1,_("Translation:"));
	TranslationEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Original Translation")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *EditingLabel = new wxStaticText(this,-1,_("Editing:"));
	EditingEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Original Editing")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *TimingLabel = new wxStaticText(this,-1,_("Timing:"));
	TimingEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Original Timing")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *SyncLabel = new wxStaticText(this,-1,_("Synch point:"));
	SyncEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Synch Point")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *UpdatedLabel = new wxStaticText(this,-1,_("Updated by:"));
	UpdatedEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Script Updated By")),wxDefaultPosition,wxSize(200,20));
	wxStaticText *UpdateDetailsLabel = new wxStaticText(this,-1,_("Update details:"));
	UpdateDetailsEdit = new wxTextCtrl(this,-1,subs->GetScriptInfo(_T("Update Details")),wxDefaultPosition,wxSize(200,20));
	wxSizer *TopSizer = new wxStaticBoxSizer(wxHORIZONTAL,this,_("Script"));
	wxSizer *TopSizerGrid = new wxFlexGridSizer(0,2,5,5);
	TopSizerGrid->Add(TitleLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(TitleEdit,1,0,0);
	TopSizerGrid->Add(OrigScriptLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(OrigScriptEdit,1,0,0);
	TopSizerGrid->Add(TranslationLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(TranslationEdit,1,0,0);
	TopSizerGrid->Add(EditingLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(EditingEdit,1,0,0);
	TopSizerGrid->Add(TimingLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(TimingEdit,1,0,0);
	TopSizerGrid->Add(SyncLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(SyncEdit,1,0,0);
	TopSizerGrid->Add(UpdatedLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(UpdatedEdit,1,0,0);
	TopSizerGrid->Add(UpdateDetailsLabel,0,wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,0);
	TopSizerGrid->Add(UpdateDetailsEdit,1,0,0);
	TopSizer->Add(TopSizerGrid,0,wxALL,0);

	// Resolution box
	wxSizer *ResSizer = new wxStaticBoxSizer(wxHORIZONTAL,this,_("Resolution"));
	ResXValue = subs->GetScriptInfo(_T("PlayResX"));
	ResYValue = subs->GetScriptInfo(_T("PlayResY"));
	ResX = new wxTextCtrl(this,-1,_T(""),wxDefaultPosition,wxSize(50,20),0,NumValidator(&ResXValue));
	ResY = new wxTextCtrl(this,-1,_T(""),wxDefaultPosition,wxSize(50,20),0,NumValidator(&ResYValue));
	wxStaticText *ResText = new wxStaticText(this,-1,_T("x"));
	wxButton *FromVideo = new wxButton(this,BUTTON_FROM_VIDEO,_("From video"));
	if (!vid->loaded) FromVideo->Enable(false);
	ResSizer->Add(ResX,1,wxRIGHT,5);
	ResSizer->Add(ResText,0,wxALIGN_CENTER | wxRIGHT,5);
	ResSizer->Add(ResY,1,wxRIGHT,5);
	ResSizer->Add(FromVideo,1,0,0);

	// Wrap box
	wxSizer *WrapBox = new wxStaticBoxSizer(wxHORIZONTAL,this,_("Wrap style"));
	wxArrayString options;
	options.Add(_("0: Smart wrapping, top line is wider"));
	options.Add(_("1: End-of-line word wrapping, only \\N breaks"));
	options.Add(_("2: No word wrapping, both \\n and \\N break"));
	options.Add(_("3: Smart wrapping, bottom line is wider"));
	WrapStyle = new wxComboBox(this,-1,_T(""),wxDefaultPosition,wxDefaultSize,options,wxCB_READONLY);
	long n;
	subs->GetScriptInfo(_T("WrapStyle")).ToLong(&n);
	WrapStyle->SetSelection(n);
	WrapBox->Add(WrapStyle,1,0,0);

	// Button sizer
	wxButton *ButtonOK = new wxButton(this,wxID_OK);
	ButtonOK->SetDefault();
	wxSizer *ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	ButtonSizer->AddStretchSpacer(1);
	ButtonSizer->Add(ButtonOK,0,0,0);

	// MainSizer
	wxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(TopSizer,0,wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND,5);
	MainSizer->Add(ResSizer,0,wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND,5);
	MainSizer->Add(WrapBox,0,wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND,5);
	MainSizer->Add(ButtonSizer,0,wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND,5);

	// Set sizer
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	CenterOnParent();
}


//////////////
// Destructor
DialogProperties::~DialogProperties () {
}


///////////////
// Event table
BEGIN_EVENT_TABLE(DialogProperties,wxDialog)
	EVT_BUTTON(wxID_OK,DialogProperties::OnOK)
	EVT_BUTTON(BUTTON_FROM_VIDEO,DialogProperties::OnSetFromVideo)
END_EVENT_TABLE()


/////////////////
// Apply changes
void DialogProperties::OnOK(wxCommandEvent &event) {
	// Update details
	int count = 0;
	count += SetInfoIfDifferent(_T("Title"),TitleEdit->GetValue());
	count += SetInfoIfDifferent(_T("Original Script"),OrigScriptEdit->GetValue());
	count += SetInfoIfDifferent(_T("Original Translation"),TranslationEdit->GetValue());
	count += SetInfoIfDifferent(_T("Original Editing"),EditingEdit->GetValue());
	count += SetInfoIfDifferent(_T("Original Timing"),TimingEdit->GetValue());
	count += SetInfoIfDifferent(_T("Synch Point"),SyncEdit->GetValue());
	count += SetInfoIfDifferent(_T("Script Updated By"),UpdatedEdit->GetValue());
	count += SetInfoIfDifferent(_T("Update Details"),UpdateDetailsEdit->GetValue());
	count += SetInfoIfDifferent(_T("PlayResX"),ResX->GetValue());
	count += SetInfoIfDifferent(_T("PlayResY"),ResY->GetValue());
	count += SetInfoIfDifferent(_T("WrapStyle"),wxString::Format(_T("%i"),WrapStyle->GetSelection()));
	
	if (count) AssFile::top->FlagAsModified();

	EndModal(count?1:0);
}


//////////////////////////////////////
// Only set script info if it changed
int DialogProperties::SetInfoIfDifferent(wxString key,wxString value) {
	// Get script
	AssFile *subs = AssFile::top;

	// Compare
	if (subs->GetScriptInfo(key) != value) {
		subs->SetScriptInfo(key,value);
		return 1;
	}
	else return 0;
}


//////////////////////////
// Set res to match video
void DialogProperties::OnSetFromVideo(wxCommandEvent &event) {
	ResX->SetValue(wxString::Format(_T("%i"),vid->provider->GetSourceWidth()));
	ResY->SetValue(wxString::Format(_T("%i"),vid->provider->GetSourceHeight()));
	event.Skip();
}
