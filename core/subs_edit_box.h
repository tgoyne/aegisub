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


#pragma once


////////////
// Includes
#include <wx/wxprec.h>


//////////////
// Prototypes
class SubtitlesGrid;
class TimeEdit;
class SubsEditBox;
class AudioDisplay;
class HiliModTextCtrl;
class VideoDisplay;


////////////////////
// SubsTextEditCtrl
class SubsTextEditCtrl : public wxTextCtrl {
private:
	void OnMouseEvent(wxMouseEvent &event);

public:
	SubsEditBox *control;
	SubsTextEditCtrl(wxWindow* parent, wxWindowID id, const wxString& value = _T(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr);

	DECLARE_EVENT_TABLE()
};


//////////////////
// Edit box class
class SubsEditBox : public wxPanel {
private:
	bool enabled;
	bool textEditReady;
	bool controlState;
	wxColour origBgColour;
	wxColour disabledBgColour;

	SubtitlesGrid *grid;
	wxCheckBox *CommentBox;
	wxComboBox *StyleBox;
	wxComboBox *ActorBox;
	TimeEdit *StartTime;
	TimeEdit *EndTime;
	TimeEdit *Duration;
	HiliModTextCtrl *Layer;
	HiliModTextCtrl *MarginL;
	HiliModTextCtrl *MarginR;
	HiliModTextCtrl *MarginV;
	wxRadioButton *ByTime;
	wxRadioButton *ByFrame;
	wxCheckBox *SyntaxHighlight;

	wxButton *Bold;
	wxButton *Italics;
	wxButton *Underline;
	wxButton *Strikeout;
	wxButton *FontName;
	wxButton *Color1;
	wxButton *Color2;
	wxButton *Color3;
	wxButton *Color4;

	void SetControlsState(bool state);
	void CommitTimes(bool start,bool end);

	int BlockAtPos(int pos);

	void OnButtonColor1(wxCommandEvent &event);
	void OnButtonColor2(wxCommandEvent &event);
	void OnButtonColor3(wxCommandEvent &event);
	void OnButtonColor4(wxCommandEvent &event);
	void OnButtonFontFace(wxCommandEvent &event);
	void OnButtonBold(wxCommandEvent &event);
	void OnButtonItalics(wxCommandEvent &event);
	void OnButtonUnderline(wxCommandEvent &event);
	void OnButtonStrikeout(wxCommandEvent &event);

	void OnEditText(wxCommandEvent &event);
	void OnSyntaxBox(wxCommandEvent &event);
	void OnFrameRadio(wxCommandEvent &event);
	void OnTimeRadio(wxCommandEvent &event);
	void OnKeyDown(wxKeyEvent &event);
	void OnStyleChange(wxCommandEvent &event);
	void OnActorChange(wxCommandEvent &event);
	void OnLayerChange(wxCommandEvent &event);
	void OnStartTimeChange(wxCommandEvent &event);
	void OnEndTimeChange(wxCommandEvent &event);
	void OnDurationChange(wxCommandEvent &event);
	void OnMarginLChange(wxCommandEvent &event);
	void OnMarginRChange(wxCommandEvent &event);
	void OnMarginVChange(wxCommandEvent &event);
	void OnCommentChange(wxCommandEvent &event);

	void OnSplitLinePreserve(wxCommandEvent &event);
	void OnSplitLineEstimate(wxCommandEvent &event);
	void OnCut(wxCommandEvent &event);
	void OnCopy(wxCommandEvent &event);
	void OnPaste(wxCommandEvent &event);
	void OnUndo(wxCommandEvent &event);
	void OnSelectAll(wxCommandEvent &event);

public:
	int linen;
	AudioDisplay *audio;
	VideoDisplay *video;
	SubsTextEditCtrl *TextEdit;

	SubsEditBox(wxWindow *parent,SubtitlesGrid *gridp);

	void SetOverride (wxString tag,wxString preValue=_T(""),int pos=-1);
	void SetStyleFlag (wxString tag,bool hasEnd=false);

	void CommitText();
	void SetText(const wxString text);
	void Update(bool timeOnly=false);
	void UpdateGlobals();
	void SetToLine(int n);
	void UpdateFrameTiming();
	void KeyPress(wxKeyEvent &event);

	DECLARE_EVENT_TABLE()
};


/////////////////
// Event handler
class SubsEditBoxEvent : public wxEvtHandler {
private:
	SubsEditBox *control;
	void OnKeyPress(wxKeyEvent &event);

public:
	SubsEditBoxEvent(SubsEditBox *control);
	DECLARE_EVENT_TABLE()
};


///////
// IDs
enum {
	EDIT_BOX = 1300,
	SYNTAX_BOX,
	RADIO_TIME_BY_FRAME,
	RADIO_TIME_BY_TIME,
	STYLE_COMBOBOX,
	ACTOR_COMBOBOX,
	LAYER_BOX,
	STARTTIME_BOX,
	ENDTIME_BOX,
	DURATION_BOX,
	MARGINL_BOX,
	MARGINR_BOX,
	MARGINV_BOX,
	COMMENT_CHECKBOX,
	EDIT_MENU_SPLIT_PRESERVE,
	EDIT_MENU_SPLIT_ESTIMATE,
	EDIT_MENU_CUT,
	EDIT_MENU_COPY,
	EDIT_MENU_PASTE,
	EDIT_MENU_UNDO,
	EDIT_MENU_SELECT_ALL,

	BUTTON_BOLD,
	BUTTON_ITALICS,
	BUTTON_UNDERLINE,
	BUTTON_STRIKEOUT,
	BUTTON_FONT_NAME,
	BUTTON_COLOR1,
	BUTTON_COLOR2,
	BUTTON_COLOR3,
	BUTTON_COLOR4
};
