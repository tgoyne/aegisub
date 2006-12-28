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
#include "dialog_jumpto.h"
#include "vfr.h"
#include "video_display.h"


///////////////
// Constructor
DialogJumpTo::DialogJumpTo (wxWindow *parent,VideoDisplay *_vid)
: wxDialog(parent, -1, _("Jump to"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("JumpTo"))
{
	// Set initial values
	ready = false;
	vid = _vid;
	jumpframe = vid->frame_n;
	jumptime.SetMS(VFR_Output.GetTimeAtFrame(jumpframe));

	// Times
	wxStaticText *LabelTime = new wxStaticText(this,-1,_("Time: "),wxDefaultPosition,wxSize(60,20));
	wxStaticText *LabelFrame = new wxStaticText(this,-1,_("Frame: "),wxDefaultPosition,wxSize(60,20));
	JumpTime = new wxTextCtrl(this,TEXT_JUMP_TIME,jumptime.GetASSFormated(),wxDefaultPosition,wxSize(60,20));
	JumpFrame = new wxTextCtrl(this,TEXT_JUMP_FRAME,wxString::Format(_T("%i"),jumpframe),wxDefaultPosition,wxSize(60,20));
	JumpTime->SetEventHandler(new DialogJumpToEvent(this));
	JumpFrame->SetEventHandler(new DialogJumpToEvent(this));
	wxSizer *TimeSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer *FrameSizer = new wxBoxSizer(wxHORIZONTAL);
	TimeSizer->Add(LabelTime,0,wxALIGN_CENTER_VERTICAL,0);
	TimeSizer->Add(JumpTime,1,wxLEFT,5);
	FrameSizer->Add(LabelFrame,0,wxALIGN_CENTER_VERTICAL,0);
	FrameSizer->Add(JumpFrame,1,wxLEFT,5);
	wxSizer *TimesSizer = new wxStaticBoxSizer(wxVERTICAL, this, _T(""));
	TimesSizer->Add(TimeSizer,0,wxEXPAND,0);
	TimesSizer->Add(FrameSizer,0,wxEXPAND | wxTOP,5);

	// Buttons
	wxButton *CancelButton = new wxButton(this,wxID_CANCEL);
	wxButton *OKButton = new wxButton(this,wxID_OK);
	CancelButton->SetEventHandler(new DialogJumpToEvent(this));
	OKButton->SetEventHandler(new DialogJumpToEvent(this));
	OKButton->SetDefault();
	wxSizer *ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	ButtonSizer->Add(OKButton,1,wxRIGHT,5);
	ButtonSizer->Add(CancelButton,0,0,0);

	// General layout
	wxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(TimesSizer,0,wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,5);
	MainSizer->Add(ButtonSizer,0,wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT,5);

	// Set sizer
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	CenterOnParent();
	ready = true;
}


/////////////////
// Event handler

// Constructor
DialogJumpToEvent::DialogJumpToEvent (DialogJumpTo *ctrl) {
	control = ctrl;
}

// Table
BEGIN_EVENT_TABLE(DialogJumpToEvent, wxEvtHandler)
	EVT_KEY_DOWN(DialogJumpToEvent::OnKey)
	EVT_BUTTON(wxID_CANCEL,DialogJumpToEvent::OnClose)
	EVT_BUTTON(wxID_OK,DialogJumpToEvent::OnOK)
	EVT_TEXT(TEXT_JUMP_TIME, DialogJumpToEvent::OnEditTime)
	EVT_TEXT(TEXT_JUMP_FRAME, DialogJumpToEvent::OnEditFrame)
END_EVENT_TABLE()

// Redirects
void DialogJumpToEvent::OnKey (wxKeyEvent &event) { control->OnKey(event); }
void DialogJumpToEvent::OnClose (wxCommandEvent &event) { control->OnClose(false); }
void DialogJumpToEvent::OnOK (wxCommandEvent &event) { control->OnClose(true); }
void DialogJumpToEvent::OnEditTime (wxCommandEvent &event) { control->OnEditTime(event); }
void DialogJumpToEvent::OnEditFrame (wxCommandEvent &event) { control->OnEditFrame(event); }


//////////////////
// On Key pressed
void DialogJumpTo::OnKey(wxKeyEvent &event) {
	int key = event.GetKeyCode();
	if (key == WXK_RETURN) {
		vid->JumpToFrame(jumpframe);
		EndModal(0);
		return;
	}
	event.Skip();
}


////////////////////////
// On OK button pressed
void DialogJumpTo::OnClose(bool ok) {
	if (ok)	vid->JumpToFrame(jumpframe);
	EndModal(0);
}


////////////////////////
// Time editbox changed
void DialogJumpTo::OnEditTime (wxCommandEvent &event) {
	if (ready) {
		ready = false;
		jumptime.UpdateFromTextCtrl(JumpTime);

		// Update frame
		long newframe = VFR_Output.GetFrameAtTime(jumptime.GetMS());
		if (jumpframe != newframe) {
			jumpframe = newframe;
			JumpFrame->SetValue(wxString::Format(_T("%i"),jumpframe));
		}

		ready = true;
	}
	else event.Skip();
}


/////////////////////////
// Frame editbox changed
void DialogJumpTo::OnEditFrame (wxCommandEvent &event) {
	if (ready) {
		ready = false;

		// Update frame
		JumpFrame->GetValue().ToLong(&jumpframe);
		JumpFrame->SetValue(wxString::Format(_T("%i"),jumpframe));

		// Update time
		int newtime = VFR_Output.GetTimeAtFrame(jumpframe);
		if (jumptime.GetMS() != newtime) {
			jumptime.SetMS(newtime);
			JumpTime->SetValue(jumptime.GetASSFormated());
		}

		ready = true;
	}
	else event.Skip();
}
