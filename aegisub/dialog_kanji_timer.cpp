// Copyright (c) 2005, Dan Donovan (Dansolo)
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

#define MIN(a,b) ((a<b)?a:b)
#define KANA_SEARCH_DISTANCE 1 //Kana interpolation, in characters, set <=0 to disable
#define ROMAJI_SEARCH_DISTANCE 5 //Romaji interpolation, in karaoke groups, set <=0 to disable


///////////
// Headers
#include "dialog_kanji_timer.h"
#include "ass_file.h"
#include "ass_dialogue.h"
#include "ass_style.h"
#include "ass_override.h"
#include "subs_grid.h"
#include "validators.h"
#include "video_display.h"
#include "video_provider.h"


///////////////
// Constructor
DialogKanjiTimer::DialogKanjiTimer(wxWindow *parent, SubtitlesGrid *_grid)
: wxDialog (parent,-1,_("Kanji timing"),wxDefaultPosition)
{
	// Variables
	AssFile *subs = AssFile::top;
	grid = _grid;
	vid = grid->video;
	RegroupSourceSelected = 0;
	
	//Sizers
	wxSizer *ResBoxSizer1 = new wxStaticBoxSizer(wxVERTICAL,this,_("Text"));
	wxSizer *ResBoxSizer2 = new wxStaticBoxSizer(wxVERTICAL,this,_("Shortcut Keys"));
	wxSizer *ResBoxSizer3 = new wxStaticBoxSizer(wxVERTICAL,this,_("Groups"));
	wxSizer *ResBoxSizer4 = new wxStaticBoxSizer(wxVERTICAL,this,_("Styles"));
	wxSizer *ResBoxSizer5 = new wxStaticBoxSizer(wxVERTICAL,this,_("Commands"));
	wxBoxSizer *ResSizer1 = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer *ResSizerGrid1 = new wxFlexGridSizer(2,2,3,3);


	SourceText = new wxTextCtrl(this,TEXT_SOURCE,_T(""),wxDefaultPosition,wxSize(450,-1),wxTE_READONLY|wxTE_NOHIDESEL|wxSIMPLE_BORDER|wxTE_RIGHT|wxTE_PROCESS_ENTER);
	DestText = new wxTextCtrl(this,TEXT_DEST,_T(""),wxDefaultPosition,wxSize(450,-1),wxTE_NOHIDESEL|wxSIMPLE_BORDER|wxTE_RIGHT|wxTE_PROCESS_ENTER);
	SourceText->SetEventHandler(new DialogKanjiTimerEvent(this));
	DestText->SetEventHandler(new DialogKanjiTimerEvent(this));

	wxStaticText *ShortcutKeys = new wxStaticText(this,-1,_("When the destination textbox has focus, use the following keys:\n\nRight Arrow: Increase dest. selection length\nLeft Arrow: Decrease dest. selection length\nUp Arrow: Increase source selection length\nDown Arrow: Decrease source selection length\nEnter: Link, accept line when done\nBackspace: Unlink last"));

	SourceStyle=new wxComboBox(this,-1,_(""),wxDefaultPosition,wxSize(160,-1),
								 subs->GetStyles(),wxCB_READONLY,wxDefaultValidator,_("Source Style"));
	DestStyle = new wxComboBox(this,-1,_(""),wxDefaultPosition,wxSize(160,-1),
								 subs->GetStyles(),wxCB_READONLY,wxDefaultValidator,_("Dest Style"));

	GroupsList = new wxListCtrl(this,-1,wxDefaultPosition,wxSize(180,100),wxLC_REPORT|wxLC_NO_HEADER|wxLC_HRULES|wxLC_VRULES);
	GroupsList->InsertColumn(0, _T(""), wxLIST_FORMAT_CENTER, 72);
	GroupsList->InsertColumn(1, _T(""), wxLIST_FORMAT_CENTER, 72);

	//Buttons
	wxButton *Start = new wxButton(this,BUTTON_KTSTART,_("Start"));
	wxButton *Link = new wxButton(this,BUTTON_KTLINK,_("Link"));
	wxButton *Unlink = new wxButton(this,BUTTON_KTUNLINK,_("Unlink"));
	wxButton *SkipSourceLine = new wxButton(this,BUTTON_KTSKIPSOURCE,_("Skip Source Line"));
	wxButton *SkipDestLine = new wxButton(this,BUTTON_KTSKIPDEST,_("Skip Dest Line"));
	wxButton *GoBackLine = new wxButton(this,BUTTON_KTGOBACK,_("Go Back a Line"));
	wxButton *AcceptLine = new wxButton(this,BUTTON_KTACCEPT,_("Accept Line"));
	wxButton *CloseKT = new wxButton(this,wxID_CLOSE,_("Close"),wxDefaultPosition,wxDefaultSize,wxALIGN_RIGHT);

	//Checkbox
	Interpolate = new wxCheckBox(this,-1,_("Attempt to interpolate kanji."),wxDefaultPosition,wxDefaultSize,wxALIGN_LEFT);

	//Static Text labels for source/dest
	wxStaticText *SourceLabel = new wxStaticText(this,-1,_("Source: "));
	wxStaticText *DestLabel = new wxStaticText(this,-1,_("Dest: "));

	//Frame: Text
	ResSizerGrid1->Add(SourceLabel,1,wxALIGN_CENTER|wxRIGHT,3);
	ResSizerGrid1->Add(SourceText,0,wxALIGN_CENTER,3);
	ResSizerGrid1->Add(DestLabel,1,wxALIGN_CENTER|wxRIGHT,3);
	ResSizerGrid1->Add(DestText,0,wxALIGN_CENTER,3);
	ResBoxSizer1->Add(ResSizerGrid1,1,wxALIGN_CENTER|wxEXPAND,5);
	//Frame: Shortcut Keys
	ResBoxSizer2->Add(ShortcutKeys,0,wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL | wxRIGHT,5);
	//Frame: Groups
	ResBoxSizer3->Add(GroupsList,1,wxALIGN_CENTER,5);
	//Frame: Styles
	ResBoxSizer4->Add(SourceStyle,0,wxALIGN_CENTER | wxBOTTOM,5);
	ResBoxSizer4->Add(DestStyle,0,wxALIGN_CENTER,5);
	//Frame: Commands
	ResBoxSizer5->AddStretchSpacer(1);
	ResBoxSizer5->Add(Start,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(Link,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(Unlink,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(SkipSourceLine,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(SkipDestLine,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(GoBackLine,0,wxEXPAND | wxBOTTOM,5);
	ResBoxSizer5->Add(AcceptLine,0,wxEXPAND | wxBOTTOM,0);
	ResBoxSizer5->AddStretchSpacer(1);

	//Combine Shortcut Keys and Groups
	ResSizer1->Add(ResBoxSizer2,0,wxEXPAND | wxRIGHT,5);
	ResSizer1->Add(ResBoxSizer3,1,wxEXPAND,5);
	
	// Main sizer
	wxFlexGridSizer *MainSizer = new wxFlexGridSizer(3,2,0,0);
	MainSizer->Add(ResBoxSizer1,0,wxEXPAND | wxLEFT | wxRIGHT,5);
	MainSizer->Add(ResBoxSizer4,0,wxEXPAND | wxLEFT | wxRIGHT,5);
	MainSizer->Add(ResSizer1,0,wxEXPAND | wxALL,5);
	MainSizer->Add(ResBoxSizer5,0,wxEXPAND | wxALL,5);
	MainSizer->AddGrowableCol(0,1);
	MainSizer->Add(Interpolate,0,wxEXPAND | wxALL,5);
	MainSizer->AddGrowableCol(0,1);
	MainSizer->Add(CloseKT,0,wxEXPAND | wxALL,5);
	MainSizer->SetSizeHints(this);
	SetSizer(MainSizer);
	CenterOnParent();
}


///////////////
// Event table
BEGIN_EVENT_TABLE(DialogKanjiTimer,wxDialog)
	EVT_BUTTON(wxID_CLOSE,DialogKanjiTimer::OnClose)
	EVT_BUTTON(BUTTON_KTSTART,DialogKanjiTimer::OnStart)
	EVT_BUTTON(BUTTON_KTLINK,DialogKanjiTimer::OnLink)
	EVT_BUTTON(BUTTON_KTUNLINK,DialogKanjiTimer::OnUnlink)
	EVT_BUTTON(BUTTON_KTSKIPSOURCE,DialogKanjiTimer::OnSkipSource)
	EVT_BUTTON(BUTTON_KTSKIPDEST,DialogKanjiTimer::OnSkipDest)
	EVT_BUTTON(BUTTON_KTGOBACK,DialogKanjiTimer::OnGoBack)
	EVT_BUTTON(BUTTON_KTACCEPT,DialogKanjiTimer::OnAccept)
	EVT_KEY_DOWN(DialogKanjiTimer::OnKeyDown)
	EVT_TEXT_ENTER(TEXT_SOURCE,DialogKanjiTimer::OnKeyEnter)
	EVT_TEXT_ENTER(TEXT_DEST,DialogKanjiTimer::OnKeyEnter)
END_EVENT_TABLE()

void DialogKanjiTimer::OnClose(wxCommandEvent &event) {
	Close();
}

void DialogKanjiTimer::OnStart(wxCommandEvent &event) {
	SourceIndex = DestIndex = 0;
	if (SourceStyle->GetValue().Len() == 0 || DestStyle->GetValue().Len() == 0)
		wxMessageBox(_("Select source and destination styles first."),_("Error"),wxICON_EXCLAMATION | wxOK);
	else if (SourceStyle->GetValue() == DestStyle->GetValue())
		wxMessageBox(_("The source and destination styles must be different."),_("Error"),wxICON_EXCLAMATION | wxOK);
	else {
		OnSkipDest((wxCommandEvent)NULL);
		OnSkipSource((wxCommandEvent)NULL);
		DestText->SetFocus();
	}
}
void DialogKanjiTimer::OnLink(wxCommandEvent &event) {
	int sourceLen = SourceText->GetStringSelection().Len();
	int destLen = DestText->GetStringSelection().Len();
	if (sourceLen!=0) {
		wxString sourceText = SourceText->GetValue();
		wxString destText = DestText->GetValue();
		
		RegroupGroups[RegroupSourceSelected<<1] = SourceText->GetStringSelection();
		RegroupGroups[(RegroupSourceSelected<<1)+1] = DestText->GetStringSelection();

		wxListItem itm;
		int i = GroupsList->GetItemCount();
		GroupsList->InsertItem(i,itm);
		GroupsList->SetItem(i,0,RegroupGroups[RegroupSourceSelected<<1]);
		GroupsList->SetItem(i,1,RegroupGroups[(RegroupSourceSelected<<1)+1]);

		SourceText->ChangeValue(sourceText.Right(sourceText.Len()-sourceLen));
		DestText->ChangeValue(destText.Right(destText.Len()-destLen));
		
		SetSelected();

		RegroupSourceSelected++;
	}
	else {
		wxMessageBox(_("Select source text first."),_("Error"),wxICON_EXCLAMATION | wxOK);
	}
	DestText->SetFocus();
}
void DialogKanjiTimer::OnUnlink(wxCommandEvent &event) {
	if (RegroupSourceSelected) {
		RegroupSourceSelected--;
		SourceText->ChangeValue(RegroupGroups[RegroupSourceSelected<<1]+SourceText->GetValue());
		DestText->ChangeValue(RegroupGroups[(RegroupSourceSelected<<1)+1]+DestText->GetValue());
		GroupsList->DeleteItem(GroupsList->GetItemCount()-1);
		SetSelected();
	}
	DestText->SetFocus();
}
void DialogKanjiTimer::OnSkipSource(wxCommandEvent &event) {
	GroupsList->DeleteAllItems();
	TextBeforeKaraoke = _T("");
	RegroupSourceSelected = 0;
	SourceText->SetSelection(0,0);

	int index = ListIndexFromStyleandIndex(SourceStyle->GetValue(), SourceIndex);
	if (index != -1) {
		AssDialogue					*line = grid->GetDialogue(index);
		AssDialogueBlockOverride	*override;
		AssDialogueBlockPlain		*plain;
		AssOverrideTag				*tag;
		int							k, kIndex=0, textIndex=0, TextBeforeOffset=0;
		bool						LastWasOverride = false;

		line = grid->GetDialogue(index);
		wxString StrippedText = line->GetStrippedText();
		line->ParseASSTags();
		size_t blockn = line->Blocks.size();
		
		RegroupSourceText = new wxString[(const int)blockn];
		RegroupGroups = new wxString[(const int)blockn<<1];
		RegroupSourceKLengths = new int[(const int)blockn];
		RegroupTotalLen = 0; //The above arrays won't actually be of size blockn

		for (size_t i=0;i<blockn;i++) {
			k = 0;
			override = AssDialogueBlock::GetAsOverride(line->Blocks.at(i));
			if (override) {
				if (LastWasOverride) {
					/* Explanation for LastWasOverride:
					 * If we have a karaoke block with no text (IE for a pause)
					 * then we will get thrown off in the SourceText array
					 * because the K Length array will increase without it.
					 */
					RegroupSourceText[textIndex++] = _T("");
				}
				for (size_t j=0;j<override->Tags.size();j++) {
					tag = override->Tags.at(j);

					if (tag->Name == _T("\\k") && tag->Params.size() == 1)
						k = tag->Params[0]->AsInt();
				}
				RegroupSourceKLengths[kIndex++] = k;
				LastWasOverride = true;
			}

			plain = AssDialogueBlock::GetAsPlain(line->Blocks.at(i));
			if (plain) {
				if (kIndex==0) {
					/*kIndex hasn't been incremented yet so this is text before a \k
					 *This will throw off our processing.
					 *Ask the user to copy it over or ignore it.
					 */
					int result = wxMessageBox(_("The source line contains text before the first karaoke block.\nDo you want to carry it over to the destination?\nIt will be ignored otherwise."),
						         _("Question"),wxICON_QUESTION|wxYES_NO|wxYES_DEFAULT);
					TextBeforeOffset = plain->text.Len();
					if (result==wxYES)
						TextBeforeKaraoke = plain->text;
				}
				else {
					RegroupSourceText[textIndex++] = plain->text;
					LastWasOverride = false;
				}
			}
		}

		RegroupTotalLen = kIndex; //should be the same as textIndex
		if (kIndex != textIndex) //...or there was likely an error parsing, alert but don't halt
			wxMessageBox(_("Possible error parsing source line"),_("Error"),wxICON_EXCLAMATION | wxOK);

		SourceText->ChangeValue(StrippedText.Mid(TextBeforeOffset));

		SourceIndex++;
		SetSelected();
		DestText->SetFocus();
	}
}
void DialogKanjiTimer::OnSkipDest(wxCommandEvent &event) {
	GroupsList->DeleteAllItems();
	int index = ListIndexFromStyleandIndex(DestStyle->GetValue(), DestIndex);
	if (index != -1) {
		AssDialogue *line = grid->GetDialogue(index);
		DestText->ChangeValue(grid->GetDialogue(index)->GetStrippedText());
		
		SetSelected();
		
		DestText->SetFocus();
		DestIndex++;
	}
}
void DialogKanjiTimer::OnGoBack(wxCommandEvent &event) {
	DestIndex-=2;
	SourceIndex-=2;
	OnSkipDest((wxCommandEvent)NULL);
	OnSkipSource((wxCommandEvent)NULL);
}
void DialogKanjiTimer::OnAccept(wxCommandEvent &event) {
	if (RegroupTotalLen==0)
		wxMessageBox(_("Group some text first."),_("Error"),wxICON_EXCLAMATION | wxOK);
	else if (SourceText->GetValue().Len()!=0)
		wxMessageBox(_("Group all of the source text."),_("Error"),wxICON_EXCLAMATION | wxOK);
	else {
		wxString OutputText = TextBeforeKaraoke;
		wxString ThisText;
		AssDialogue *line = grid->GetDialogue(ListIndexFromStyleandIndex(DestStyle->GetValue(), DestIndex-1));
		int ItemCount = GroupsList->GetItemCount();
		int SourceLength;
		int WorkingK = 0;
		int SourceIndex = 0;

		for(int index=0;index!=ItemCount;index++) {
			SourceLength = RegroupGroups[index<<1].Len();

			if (RegroupSourceText[SourceIndex].Len() == 0) {
				//Karaoke block w/o text that is NOT in the middle of a group, just copy it over
				//  since we can't figure out if it should go to the previous or the next group
				OutputText = wxString::Format(_("%s{\\k%i}"),OutputText,RegroupSourceKLengths[SourceIndex]);
				SourceIndex++;
			}

			while(SourceLength>0) {
				WorkingK += RegroupSourceKLengths[SourceIndex];
				SourceLength -= (RegroupSourceText[SourceIndex]).Len();
				SourceIndex++;
			}
			OutputText = wxString::Format(_("%s{\\k%i}%s"),OutputText,WorkingK,RegroupGroups[(index<<1)+1]);
		
			WorkingK = 0;
		}
		line->Text = OutputText;
		grid->ass->FlagAsModified();
		grid->CommitChanges();

		OnSkipDest((wxCommandEvent)NULL);
		OnSkipSource((wxCommandEvent)NULL);
	}
}
void DialogKanjiTimer::OnKeyDown(wxKeyEvent &event) {
	int KeyCode = event.GetKeyCode();
	switch(KeyCode) {
		case WXK_ESCAPE :
			this->EndModal(0);
			break;
		case WXK_BACK :
			this->OnUnlink((wxCommandEvent)NULL);
			break;
		case WXK_RIGHT : //inc dest selection len
			if (DestText->GetStringSelection().Len()!=DestText->GetValue().Len())
				DestText->SetSelection(0,DestText->GetStringSelection().Len()+1);
			break;
		case WXK_LEFT : //dec dest selection len
			if (DestText->GetStringSelection().Len()!=0)
				DestText->SetSelection(0,DestText->GetStringSelection().Len()-1);
			break;
		case WXK_UP : //inc source selection len
			if (SourceText->GetStringSelection().Len()!=SourceText->GetValue().Len()) {
				SourceText->SetSelection(0,SourceText->GetStringSelection().Len()+RegroupSourceText[GetSourceArrayPos(false)].Len());
			}
			break;
		case WXK_DOWN : //dec source selection len
			if (SourceText->GetStringSelection().Len()!=0) {
				SourceText->SetSelection(0,SourceText->GetStringSelection().Len()-RegroupSourceText[GetSourceArrayPos(true)].Len());
			}
			break;
		case WXK_RETURN :
			OnKeyEnter((wxCommandEvent)NULL);
			break;
		default :
			event.Skip();
	}
}
void DialogKanjiTimer::OnKeyEnter(wxCommandEvent &event) {
	if (SourceText->GetValue().Len()==0&&RegroupTotalLen!=0)
		this->OnAccept((wxCommandEvent)NULL);
	else if (SourceText->GetStringSelection().Len()!=0)
		this->OnLink((wxCommandEvent)NULL);
}
void DialogKanjiTimer::OnMouseEvent(wxMouseEvent &event) {
	if (event.LeftDown()) DestText->SetFocus();
}
void DialogKanjiTimer::SetSelected() {
	if (SourceText->GetValue().Len()!=0)
		SourceText->SetSelection(0,RegroupSourceText[GetSourceArrayPos(false)].Len());

	if (SourceText->GetValue().Len()!=0&&SourceText->GetStringSelection().Len()==SourceText->GetValue().Len())
		DestText->SetSelection(0,DestText->GetValue().Len());

	else if (SourceText->GetStringSelection()==_T(" ")&&!DestText->GetValue().StartsWith(_T(" ")))
		DestText->SetSelection(0,0);

	else if (DestText->GetValue().StartsWith(SourceText->GetStringSelection()))
		DestText->SetSelection(0,SourceText->GetStringSelection().Len());

	else if (SourceText->GetValue().Len()!=0 && DestText->GetValue().Len()!=0) {
		bool foundit = false;
		if (Interpolate->IsChecked()) {
			KanaTable *kt = new KanaTable();
			wxString Destext = DestText->GetValue();
			wxString SrcG = SourceText->GetStringSelection();
			wxString trimmed = SrcG.Trim(true);
			SrcG = SourceText->GetStringSelection();
			int sourceindex=0, destsel=0;
			

			for(std::list<KanaEntry>::iterator iter = kt->entries.begin(); iter != kt->entries.end(); iter++) {
				KanaEntry ke = *iter;
				if ((int)Destext.Len()>=destsel&&(Destext.Mid(destsel).StartsWith(ke.hiragana)||Destext.StartsWith(ke.katakana))) {
					if (SrcG.Len()==sourceindex||trimmed.Mid(sourceindex)==ke.hepburn) {
						foundit=true;
						DestText->SetSelection(0,destsel+1);
						break;
					}
					if (ke.hepburn.Len()!=0 && trimmed.Mid(sourceindex).StartsWith(ke.hepburn)) {
						destsel++;
						sourceindex+=ke.hepburn.Len();	
						iter = kt->entries.begin();
					}
				}
			}

			if (KANA_SEARCH_DISTANCE>0 && !foundit) {
				//Try some interpolation for kanji. If we find a hiragana we know after this,
				//  then we may be able to figure this one out.
				wxString NextSGroup = RegroupSourceText[GetSourceArrayPos(false)];
				int index;

				for(std::list<KanaEntry>::iterator iter = kt->entries.begin(); iter != kt->entries.end(); iter++) {
					KanaEntry ke = *iter;
					if (NextSGroup.StartsWith(ke.hepburn)) {
						index=0;
						for(int i=KANA_SEARCH_DISTANCE;i!=0;i--) {
							wxString Destextmid = Destext.Mid(i);
							if(Destextmid.StartsWith(ke.hiragana)||Destextmid.StartsWith(ke.katakana)) {
								DestText->SetSelection(0,i);
								return;
							}
						}
					}
				}
			}

			if (ROMAJI_SEARCH_DISTANCE>0 && !foundit) {
				wxString Destext = DestText->GetValue();
				wxString NextSGroup, trimmed;
				int highlight = SourceText->GetStringSelection().Len();
				int start = GetSourceArrayPos(false);
				//GetSourceArrayPos is going to give us the next pos already
				//  and not our current pos, so subtract 1 from it for end.
				int end = MIN(RegroupTotalLen,start+ROMAJI_SEARCH_DISTANCE-1);

				for(int i=start;i!=end;i++) {
					NextSGroup = RegroupSourceText[i];
					trimmed = NextSGroup.Trim(false).Trim(true);
					NextSGroup = RegroupSourceText[i];

					for(std::list<KanaEntry>::iterator iter = kt->entries.begin(); iter != kt->entries.end(); iter++) {
						KanaEntry ke = *iter;
						if (trimmed==ke.hepburn) {
							int foundhira = Destext.Find(ke.hiragana);
							int foundkata =	Destext.Find(ke.katakana);
							int foundat;
							if (foundhira>0&&foundkata>0) foundat=MIN(foundhira,foundkata);
							else if (foundhira>0) foundat=foundhira;
							else foundat = foundkata; //-1 is fine, the if below checks that

							if (foundat>0 && foundat<=ROMAJI_SEARCH_DISTANCE) {
								SourceText->SetSelection(0,highlight);
								DestText->SetSelection(0,foundat);
								return;
							}
						}
					}//end kana search
					highlight += NextSGroup.Len();
				}
			}//end romaji interpolation
		}
		if (!foundit&&DestText->GetValue().Len()!=0&&DestText->GetStringSelection().Len()==0)
			DestText->SetSelection(0,1);
	}

}

////////////////////////////////////////////////////
// Gets the current position in RegroupSourceText //
int DialogKanjiTimer::GetSourceArrayPos(bool GoingDown) {
	int Len = 0;
	int index;
	for(index=0;index!=GroupsList->GetItemCount();index++) {
		Len+=RegroupGroups[index<<1].Len();
	}
	Len+=SourceText->GetStringSelection().Len();
	for(index=0;Len>0&&index!=RegroupTotalLen;index++) {
		Len-=RegroupSourceText[index].Len();
	}

	//Disregard \k blocks w/o text
	if (GoingDown) {
		index--;
		while(index!=RegroupTotalLen && RegroupSourceText[index].Len()==0) { index--; }
	}
	else {
		while(index!=RegroupTotalLen && RegroupSourceText[index].Len()==0) { index++; }
	}

	return index;
}


int DialogKanjiTimer::ListIndexFromStyleandIndex(wxString StyleName, int Occurance) {
	AssDialogue *line;
	int index = 0;
	int occindex = 0;
	while(line=grid->GetDialogue(index)) {
		if (line->Style == StyleName) {
			if (occindex == Occurance)
				return index;
			occindex++;
		}
		index++;
	}
	return -1;
}






DialogKanjiTimerEvent::DialogKanjiTimerEvent(DialogKanjiTimer *ctrl) {
	control = ctrl;
}

// Event table
BEGIN_EVENT_TABLE(DialogKanjiTimerEvent, wxEvtHandler)
	EVT_KEY_DOWN(DialogKanjiTimerEvent::KeyHandler)
	EVT_MOUSE_EVENTS(DialogKanjiTimerEvent::MouseHandler)
END_EVENT_TABLE()

// Redirects
void DialogKanjiTimerEvent::KeyHandler(wxKeyEvent &event) { control->OnKeyDown(event); }

void DialogKanjiTimerEvent::MouseHandler(wxMouseEvent &event) {
	control->OnMouseEvent(event);
}
