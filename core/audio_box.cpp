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


///////////
// Headers
#include <math.h>
#include <wx/tglbtn.h>
#include <wx/statline.h>
#include <wx/laywin.h>
#include <wx/recguard.h>
#include "audio_box.h"
#include "audio_display.h"
#include "audio_karaoke.h"
#include "frame_main.h"
#include "options.h"
#include "toggle_bitmap.h"
#include "hotkeys.h"


///////////////
// Constructor
AudioBox::AudioBox(wxWindow *parent,VideoDisplay *display) :
wxPanel(parent,-1,wxDefaultPosition,wxDefaultSize,wxTAB_TRAVERSAL|wxBORDER_RAISED)
{
	// Setup
	loaded = false;
	karaokeMode = false;

	// Controls
	audioScroll = new wxScrollBar(this,Audio_Scrollbar);
	audioScroll->SetToolTip(_("Seek bar"));
	Sash = new wxSashWindow(this,Audio_Sash,wxDefaultPosition,wxDefaultSize,wxCLIP_CHILDREN | wxSW_3DBORDER);
	//Sash = new wxSashLayoutWindow(this,Audio_Sash,wxDefaultPosition,wxDefaultSize);
	sashSizer = new wxBoxSizer(wxVERTICAL);
	audioDisplay = new AudioDisplay(Sash,display);
	sashSizer->Add(audioDisplay,1,wxEXPAND,0);
	//sashSizer->SetSizeHints(Sash);
	Sash->SetSizer(sashSizer);
	Sash->SetSashVisible(wxSASH_BOTTOM,true);
	Sash->SetSashBorder(wxSASH_BOTTOM,true);
	Sash->SetMinimumSizeY(50);
	audioDisplay->ScrollBar = audioScroll;
	audioDisplay->box = this;
	HorizontalZoom = new wxSlider(this,Audio_Horizontal_Zoom,50,0,100,wxDefaultPosition,wxSize(-1,20),wxSL_VERTICAL);
	HorizontalZoom->SetToolTip(_("Horizontal zoom"));
	VerticalZoom = new wxSlider(this,Audio_Vertical_Zoom,50,0,100,wxDefaultPosition,wxSize(-1,20),wxSL_VERTICAL|wxSL_INVERSE);
	VerticalZoom->SetToolTip(_("Vertical zoom"));

	// Display sizer
	DisplaySizer = new wxBoxSizer(wxVERTICAL);
	//DisplaySizer->Add(audioDisplay,1,wxEXPAND,0);
	DisplaySizer->Add(Sash,0,wxEXPAND,0);
	DisplaySizer->Add(audioScroll,0,wxEXPAND,0);

	// Top sizer
	TopSizer = new wxBoxSizer(wxHORIZONTAL);
	TopSizer->Add(DisplaySizer,1,wxEXPAND,0);
	TopSizer->Add(HorizontalZoom,0,wxEXPAND,0);
	TopSizer->Add(VerticalZoom,0,wxEXPAND,0);

	// Buttons sizer
	wxSizer *ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton *temp;
	temp = new wxBitmapButton(this,Audio_Button_Prev,wxBITMAP(button_prev),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Previous line/syllable (") + Hotkeys.GetText(_T("Audio Prev Line")) + _T("/") + Hotkeys.GetText(_T("Audio Prev Line Alt")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Next,wxBITMAP(button_next),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Next line/syllable (") + Hotkeys.GetText(_T("Audio Next Line")) + _T("/") + Hotkeys.GetText(_T("Audio Next Line Alt")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play,wxBITMAP(button_playsel),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play selection (") + Hotkeys.GetText(_T("Audio Play")) + _T("/") + Hotkeys.GetText(_T("Audio Play Alt")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play_Row,wxBITMAP(button_playline),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play current line (") + Hotkeys.GetText(_T("Audio Play Original Line")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Stop,wxBITMAP(button_stop),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Stop"));
	ButtonSizer->Add(temp,0,wxRIGHT,10);

	temp = new wxBitmapButton(this,Audio_Button_Play_500ms_Before,wxBITMAP(button_play500before),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play 500 ms before selection (") + Hotkeys.GetText(_T("Audio Play 500ms Before")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play_500ms_After,wxBITMAP(button_play500after),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play 500 ms after selection (") + Hotkeys.GetText(_T("Audio Play 500ms after")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play_500ms_First,wxBITMAP(button_playfirst500),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play first 500ms of selection (") + Hotkeys.GetText(_T("Audio Play First 500ms")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play_500ms_Last,wxBITMAP(button_playlast500),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play last 500ms of selection (") + Hotkeys.GetText(_T("Audio Play Last 500ms")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Play_To_End,wxBITMAP(button_playtoend),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Play from selection start to end of file"));
	ButtonSizer->Add(temp,0,wxRIGHT,10);

	temp = new wxBitmapButton(this,Audio_Button_Leadin,wxBITMAP(button_leadin),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Add lead in (") + Hotkeys.GetText(_T("Audio Add Lead In")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Leadout,wxBITMAP(button_leadout),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Add lead out (") + Hotkeys.GetText(_T("Audio Add Lead Out")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,10);

	temp = new wxBitmapButton(this,Audio_Button_Commit,wxBITMAP(button_audio_commit),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Commit changes (") + Hotkeys.GetText(_T("Audio Commit (Stay)")) + _T("/") + Hotkeys.GetText(_T("Audio Commit Alt")) + _T(")"));
	ButtonSizer->Add(temp,0,wxRIGHT,0);
	temp = new wxBitmapButton(this,Audio_Button_Goto,wxBITMAP(button_audio_goto),wxDefaultPosition,wxSize(30,-1));
	temp->SetToolTip(_("Go to selection"));
	ButtonSizer->Add(temp,0,wxRIGHT,10);

	AutoCommit = new ToggleBitmap(this,Audio_Check_AutoCommit,wxBITMAP(toggle_audio_autocommit),wxSize(30,-1));
	AutoCommit->SetToolTip(_("Automatically commit all changes"));
	AutoCommit->SetValue(Options.AsBool(_T("Audio Autocommit")));
	ButtonSizer->Add(AutoCommit,0,wxRIGHT | wxALIGN_CENTER | wxEXPAND,0);
	AutoScroll = new ToggleBitmap(this,Audio_Check_AutoGoto,wxBITMAP(toggle_audio_autoscroll),wxSize(30,-1));
	AutoScroll->SetToolTip(_("Auto scrolls audio display to selected line"));
	AutoScroll->SetValue(Options.AsBool(_T("Audio Autoscroll")));
	ButtonSizer->Add(AutoScroll,0,wxRIGHT | wxALIGN_CENTER | wxEXPAND,0);
	SSAMode = new ToggleBitmap(this,Audio_Check_SSA,wxBITMAP(toggle_audio_ssa),wxSize(30,-1));
	SSAMode->SetToolTip(_("Substation Alpha Mode - Left click sets start and right click sets end"));
	SSAMode->SetValue(Options.AsBool(_T("Audio SSA Mode")));
	ButtonSizer->Add(SSAMode,0,wxRIGHT | wxALIGN_CENTER | wxEXPAND,0);
	SpectrumMode = new ToggleBitmap(this,Audio_Check_Spectrum,wxBITMAP(toggle_audio_spectrum),wxSize(30,-1));
	SpectrumMode->SetToolTip(_("Spectrum analyzer mode"));
	SpectrumMode->SetValue(Options.AsBool(_T("Audio Spectrum")));
	ButtonSizer->Add(SpectrumMode,0,wxRIGHT | wxALIGN_CENTER | wxEXPAND,0);
	ButtonSizer->AddStretchSpacer(1);

	// Karaoke sizer
	wxSizer *karaokeSizer = new wxBoxSizer(wxHORIZONTAL);
	audioKaraoke = new AudioKaraoke(this);
	audioKaraoke->box = this;
	audioKaraoke->display = audioDisplay;
	audioDisplay->karaoke = audioKaraoke;
	KaraokeButton = new wxToggleButton(this,Audio_Button_Karaoke,_("Karaoke"),wxDefaultPosition,wxSize(-1,-1));
	KaraokeButton->SetToolTip(_("Toggle karaoke mode"));
	karaokeSizer->Add(KaraokeButton,0,wxRIGHT,0);
	JoinButton = new wxButton(this,Audio_Button_Join,_("Join"),wxDefaultPosition,wxSize(-1,-1));
	JoinButton->SetToolTip(_("Join selected syllables"));
	JoinButton->Enable(false);
	karaokeSizer->Add(JoinButton,0,wxRIGHT,0);
	SplitButton = new wxToggleButton(this,Audio_Button_Split,_("Split"),wxDefaultPosition,wxSize(-1,-1));
	SplitButton->SetToolTip(_("Toggle splitting-mode"));
	SplitButton->Enable(false);
	SplitButton->SetValue(false);
	karaokeSizer->Add(SplitButton,0,wxRIGHT,5);
	karaokeSizer->Add(audioKaraoke,1,wxEXPAND,0);

	// Main sizer
	MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(TopSizer,0,wxEXPAND,0);
	MainSizer->Add(ButtonSizer,0,wxEXPAND,0);
	MainSizer->Add(new wxStaticLine(this),0,wxEXPAND|wxTOP|wxBOTTOM,2);
	MainSizer->Add(karaokeSizer,0,wxEXPAND,0);
	//MainSizer->SetSizeHints(this);
	SetSizer(MainSizer);
}


//////////////
// Destructor
AudioBox::~AudioBox() {
}


////////////
// Set file
void AudioBox::SetFile(wxString file,bool FromVideo) {
	loaded = false;

	if (FromVideo) {
		audioDisplay->SetFromVideo();
		loaded = audioDisplay->loaded;
		audioName = _T("?video");
	}

	else {
		audioDisplay->SetFile(file);
		if (file != _T("")) loaded = audioDisplay->loaded;
		audioName = file;
	}
}


///////////////
// Event table
BEGIN_EVENT_TABLE(AudioBox,wxPanel)
	EVT_COMMAND_SCROLL(Audio_Scrollbar, AudioBox::OnScrollbar)
	EVT_COMMAND_SCROLL(Audio_Horizontal_Zoom, AudioBox::OnHorizontalZoom)
	EVT_COMMAND_SCROLL(Audio_Vertical_Zoom, AudioBox::OnVerticalZoom)
	EVT_SASH_DRAGGED(Audio_Sash,AudioBox::OnSash)

	EVT_BUTTON(Audio_Button_Play, AudioBox::OnPlaySelection)
	EVT_BUTTON(Audio_Button_Play_Row, AudioBox::OnPlayDialogue)
	EVT_BUTTON(Audio_Button_Stop, AudioBox::OnStop)
	EVT_BUTTON(Audio_Button_Next, AudioBox::OnNext)
	EVT_BUTTON(Audio_Button_Prev, AudioBox::OnPrev)
	EVT_BUTTON(Audio_Button_Play_500ms_Before, AudioBox::OnPlay500Before)
	EVT_BUTTON(Audio_Button_Play_500ms_After, AudioBox::OnPlay500After)
	EVT_BUTTON(Audio_Button_Play_500ms_First, AudioBox::OnPlay500First)
	EVT_BUTTON(Audio_Button_Play_500ms_Last, AudioBox::OnPlay500Last)
	EVT_BUTTON(Audio_Button_Play_To_End, AudioBox::OnPlayToEnd)
	EVT_BUTTON(Audio_Button_Commit, AudioBox::OnCommit)
	EVT_BUTTON(Audio_Button_Goto, AudioBox::OnGoto)
	EVT_BUTTON(Audio_Button_Join,AudioBox::OnJoin)
	EVT_BUTTON(Audio_Button_Leadin,AudioBox::OnLeadIn)
	EVT_BUTTON(Audio_Button_Leadout,AudioBox::OnLeadOut)

	EVT_TOGGLEBUTTON(Audio_Button_Karaoke, AudioBox::OnKaraoke)
	EVT_TOGGLEBUTTON(Audio_Check_AutoGoto,AudioBox::OnAutoGoto)
	EVT_TOGGLEBUTTON(Audio_Button_Split,AudioBox::OnSplit)
	EVT_TOGGLEBUTTON(Audio_Check_SSA,AudioBox::OnSSAMode)
	EVT_TOGGLEBUTTON(Audio_Check_Spectrum,AudioBox::OnSpectrumMode)
	EVT_TOGGLEBUTTON(Audio_Check_AutoCommit,AudioBox::OnAutoCommit)
END_EVENT_TABLE()


/////////////////////
// Scrollbar changed
void AudioBox::OnScrollbar(wxScrollEvent &event) {
	audioDisplay->SetPosition(event.GetPosition()*12);
}


///////////////////////////////
// Horizontal zoom bar changed
void AudioBox::OnHorizontalZoom(wxScrollEvent &event) {
	audioDisplay->SetSamplesPercent(event.GetPosition());
}


/////////////////////////////
// Vertical zoom bar changed
void AudioBox::OnVerticalZoom(wxScrollEvent &event) {
	int pos = event.GetPosition();
	if (pos < 1) pos = 1;
	if (pos > 100) pos = 100;
	audioDisplay->SetScale(pow(float(pos)/50.0f,3));
}


////////
// Sash
void AudioBox::OnSash(wxSashEvent& event) {
	// OK?
	if (event.GetDragStatus() == wxSASH_STATUS_OUT_OF_RANGE) return;

	// Recursion guard
	static wxRecursionGuardFlag inside;
	wxRecursionGuard guard(inside);
	if (guard.IsInside()) {
		return;
	}
	
	// Get size
	wxRect newSize = event.GetDragRect();
	int w = newSize.GetWidth();
	int h = newSize.GetHeight();
	if (h < 50) h = 50;
	int oldh = audioDisplay->GetSize().GetHeight();
	if (oldh == h) return;

	// Resize
	audioDisplay->SetSize(w,h);
	audioDisplay->SetSizeHints(w,h,w,h);

	// Store new size
	Options.SetInt(_T("Audio Display Height"),h);
	Options.Save();

	// Fix layout
	frameMain->Freeze();
	DisplaySizer->Layout();
	//TopSizer->Layout();
	//MainSizer->Layout();
	Layout();
	frameMain->ToolSizer->Layout();
	frameMain->MainSizer->Layout();
	frameMain->Layout();
	frameMain->Refresh();
	frameMain->Thaw();

	//event.Skip();
}


//////////////////
// Play selection
void AudioBox::OnPlaySelection(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	audioDisplay->Play(start,end);
}


/////////////////
// Play dialogue
void AudioBox::OnPlayDialogue(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesDialogue(start,end);
	audioDisplay->SetSelection(start, end);
	audioDisplay->Play(start,end);
}


////////////////
// Stop Playing
void AudioBox::OnStop(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->Stop();
}


////////
// Next
void AudioBox::OnNext(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->Next();
}


////////////
// Previous
void AudioBox::OnPrev(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->Prev();
}


/////////////////
// 500 ms before
void AudioBox::OnPlay500Before(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	audioDisplay->Play(start-500,start);
}


////////////////
// 500 ms after
void AudioBox::OnPlay500After(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	audioDisplay->Play(end,end+500);
}


////////////////
// First 500 ms
void AudioBox::OnPlay500First(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	int endp = start+500;
	if (endp > end) endp = end;
	audioDisplay->Play(start,endp);
}


///////////////
// Last 500 ms
void AudioBox::OnPlay500Last(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	int startp = end-500;
	if (startp < start) startp = start;
	audioDisplay->Play(startp,end);
}


////////////////////////
// Start to end of file
void AudioBox::OnPlayToEnd(wxCommandEvent &event) {
	int start=0,end=0;
	audioDisplay->SetFocus();
	audioDisplay->GetTimesSelection(start,end);
	audioDisplay->Play(start,-1);
}


//////////////////
// Commit changes
void AudioBox::OnCommit(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->CommitChanges();
}


//////////////////
// Toggle karaoke
void AudioBox::OnKaraoke(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	if (karaokeMode) {
		if (audioKaraoke->splitting) {
			audioKaraoke->EndSplit(false);
		}
		karaokeMode = false;
		audioKaraoke->enabled = false;
		SetKaraokeButtons(false,false);
		audioDisplay->SetDialogue();
		audioKaraoke->Refresh(false);
	}

	else {
		karaokeMode = true;
		audioKaraoke->enabled = true;
		SetKaraokeButtons(true,true);
		audioDisplay->SetDialogue();
	}
}


////////////////////////
// Sets karaoke buttons
void AudioBox::SetKaraokeButtons(bool join,bool split) {
	audioDisplay->SetFocus();
	JoinButton->Enable(join && !audioKaraoke->splitting);
	SplitButton->Enable(split);
	SplitButton->SetValue(audioKaraoke->splitting);
	if (audioKaraoke->splitting) {
		SplitButton->SetLabel(_("Cancel Split"));
	} else {
		SplitButton->SetLabel(_("Split"));
	}
}


///////////////
// Join button
void AudioBox::OnJoin(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioKaraoke->Join();
}


////////////////
// Split button
void AudioBox::OnSplit(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	if (!audioKaraoke->splitting) {
		audioKaraoke->BeginSplit();
	} else {
		audioKaraoke->EndSplit(false);
	}
}


///////////////
// Goto button
void AudioBox::OnGoto(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->MakeDialogueVisible(true);
}


/////////////
// Auto Goto
void AudioBox::OnAutoGoto(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	Options.SetBool(_T("Audio Autoscroll"),AutoScroll->GetValue());
	Options.Save();
}


///////////////
// Auto Commit
void AudioBox::OnAutoCommit(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	Options.SetBool(_T("Audio Autocommit"),AutoCommit->GetValue());
	Options.Save();
}


////////////
// SSA Mode
void AudioBox::OnSSAMode(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	Options.SetBool(_T("Audio SSA Mode"),SSAMode->GetValue());
	Options.Save();
}


//////////////////////////
// Spectrum Analyzer Mode
void AudioBox::OnSpectrumMode(wxCommandEvent &event) {
	Options.SetBool(_T("Audio Spectrum"),SpectrumMode->GetValue());
	Options.Save();
	audioDisplay->UpdateImage(false);
	audioDisplay->SetFocus();
	audioDisplay->Refresh(false);
}


///////////////
// Lead in/out
void AudioBox::OnLeadIn(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->AddLead(true,false);
}

void AudioBox::OnLeadOut(wxCommandEvent &event) {
	audioDisplay->SetFocus();
	audioDisplay->AddLead(false,true);
}
