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
#include <wx/tokenzr.h>
#include "audio_karaoke.h"
#include "audio_display.h"
#include "audio_box.h"
#include "ass_dialogue.h"
#include "ass_override.h"
#include <algorithm>


////////////////////////
// Syllable constructor
KaraokeSyllable::KaraokeSyllable() {
	length = 0;
	position = 0;
	display_w = 0;
	display_x = 0;
	tag = _T("\\k");
	pending_splits.clear();
	selected = false;
	original_tagdata = 0;
}


///////////////
// Constructor
AudioKaraoke::AudioKaraoke(wxWindow *parent)
: wxWindow (parent,-1,wxDefaultPosition,wxSize(10,5),wxTAB_TRAVERSAL|wxBORDER_SUNKEN)
{
	enabled = false;
	splitting = false;
	split_cursor_syl = -1;
	curSyllable = 0;
	diag = NULL;
}


//////////////////////
// Load from dialogue
bool AudioKaraoke::LoadFromDialogue(AssDialogue *_diag) {
	// Make sure we're not in splitting-mode
	if (splitting) {
		EndSplit(false);
	}

	// Set dialogue
	diag = _diag;
	if (!diag) {
		Refresh(false);
		return false;
	}

	// Split
	must_rebuild = false;
	bool hasKar = ParseDialogue(diag);

	// No karaoke, autosplit
	if (!hasKar) {
		AutoSplit();
	}

	// Done
	SetSelection(curSyllable);
	Refresh(false);
	return !hasKar;
}


///////////////////////////////
// Calculate length of karaoke
AssOverrideTag * AudioKaraoke::GetKaraokeLength(AssDialogueBlockOverride *block) {
	AssOverrideTag *tag, *len = 0;
	size_t n = block->Tags.size();
	for (size_t i=0;i<n;i++) {
		tag = block->Tags.at(i);
		if (tag->Name == _T("\\k") || tag->Name == _T("\\K") || tag->Name == _T("\\kf") || tag->Name == _T("\\ko")) {
			len = tag;
		}
	}
	return len;
}


////////////////////////////
// Gets tag of nth syllable
wxString AudioKaraoke::GetSyllableTag(AssDialogueBlockOverride *block,int n) {
	return block->Tags.at(n)->Name;
}


////////////////////
// Writes line back
void AudioKaraoke::Commit() {
	if (splitting) {
		EndSplit(true);
	}
	wxString finalText = _T("");
	KaraokeSyllable *syl;
	size_t n = syllables.size();
	if (must_rebuild) {
		for (size_t i=0;i<n;i++) {
			syl = &syllables.at(i);
			finalText += wxString::Format(_T("{%s%i}"), syl->tag, syl->length) + syl->contents;
		}
		diag->Text = finalText;
		diag->ParseASSTags();
	} else {
		wxLogDebug(_T("Updating karaoke without rebuild"));
		for (size_t i = 0; i < n; i++) {
			wxLogDebug(_T("Updating syllable %d"), i);
			syl = &syllables.at(i);
			wxLogDebug(_T("Syllable pointer: %p; tagdata pointer: %p; length: %d"), syl, syl->original_tagdata, syl->length);
			syl->original_tagdata->SetInt(syl->length);
		}
		wxLogDebug(_T("Done updating syllables"));
		diag->UpdateText();
	}
}


//////////////////
// Autosplit line
void AudioKaraoke::AutoSplit() {
	// Get lengths
	int timelen = (diag->End.GetMS() - diag->Start.GetMS())/10;
	int letterlen = diag->Text.Length();
	int round = letterlen / 2;
	int curlen;
	int acumLen = 0;
	wxString newText;

	// Parse words
	wxStringTokenizer tkz(diag->Text,_T(" "),wxTOKEN_RET_DELIMS);
	while (tkz.HasMoreTokens()) {
		wxString token = tkz.GetNextToken();
		curlen = (token.Length() * timelen + round) / letterlen;
		acumLen += curlen;
		if (acumLen > timelen) {
			curlen -= acumLen - timelen;
			acumLen = timelen;
		}
		newText += wxString::Format(_T("{\\k%i}"),curlen) + token;
	}

	// Load
	must_rebuild = true;
	AssDialogue newDiag(diag->data);
	newDiag.Text = newText;
	newDiag.ParseASSTags();
	ParseDialogue(&newDiag);
}


//////////////////////////////////
// Parses text to extract karaoke
bool AudioKaraoke::ParseDialogue(AssDialogue *curDiag) {
	// Wipe
	syllables.clear();

	// Prepare syllable data
	AssDialogueBlock *block;
	AssDialogueBlockOverride *override;
	AssDialogueBlockPlain *plain;
	KaraokeSyllable temp;
	temp.contents = _T("");
	int pos = 0;
	temp.length = 0;
	temp.position = 0;
	size_t n = curDiag->Blocks.size();
	bool foundOne = false;
	bool foundBlock = false;

	// Load syllable data
	for (size_t i=0;i<n;i++) {
		block = curDiag->Blocks.at(i);
		override = AssDialogueBlock::GetAsOverride(block);
		if (override) {
			AssOverrideTag *len = GetKaraokeLength(override);
			if (len) {
				if (foundOne) syllables.push_back(temp);
				foundOne = true;
				foundBlock = true;
				pos += temp.length;
				temp.length = len->Params.at(0)->AsInt();
				temp.position = pos;
				temp.contents = _T("");
				temp.tag = len->Name;
				temp.original_tagdata = len->Params.at(0);
			}
		}
		else {
			plain = AssDialogueBlock::GetAsPlain(block);
			temp.contents += plain->text;
			if (plain->text != _T("")) foundOne = true;
		}
	}

	// Empty?
	if (curDiag->Text.IsEmpty()) {
		temp.length = (curDiag->End.GetMS() - curDiag->Start.GetMS())/10;
		temp.contents = curDiag->Text;
		temp.position = 0;
		foundBlock = true;
	}

	// Last syllable
	if (foundBlock) syllables.push_back(temp);
	return foundBlock;
}


////////////////
// Set syllable
void AudioKaraoke::SetSyllable(int n) {
	curSyllable = n;
	startClickSyl = n;
	SetSelection(n);
	Refresh(false);
}


///////////////
// Event table
BEGIN_EVENT_TABLE(AudioKaraoke,wxWindow)
	EVT_PAINT(AudioKaraoke::OnPaint)
	EVT_SIZE(AudioKaraoke::OnSize)
	EVT_MOUSE_EVENTS(AudioKaraoke::OnMouse)
END_EVENT_TABLE()


///////////////
// Paint event
void AudioKaraoke::OnPaint(wxPaintEvent &event) {
	// Get dimensions
	int w,h;
	GetClientSize(&w,&h);

	// Start Paint
	wxPaintDC dc(this);
	dc.BeginDrawing();

	// Draw background
	dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(0,0,w,h);

	// Set syllable font
	wxFont curFont(9,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,_T("Verdana"),wxFONTENCODING_SYSTEM);
	dc.SetFont(curFont);

	// Draw syllables
	if (enabled) {
		wxString temptext;
		size_t syln = syllables.size();
		int dx = 0;
		int tw,th;
		int delta;
		int dlen;
		for (size_t i=0;i<syln;i++) {
			KaraokeSyllable &syl = syllables.at(i);
			// Calculate text length
			temptext = syl.contents;
			// If we're splitting, every character must be drawn
			if (!splitting) {
				temptext.Trim(true);
				temptext.Trim(false);
			}
			GetTextExtent(temptext,&tw,&th,NULL,NULL,&curFont);
			delta = 0;
			if (tw < 10) delta = 10 - tw;
			dlen = tw + 8 + delta;

			// Draw border
			dc.SetPen(wxPen(wxColour(0,0,0)));
			if (syl.selected && !splitting) {
				dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT)));
				dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
			}
			else {
				dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT)));
				dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
			}
			dc.DrawRectangle(dx,0,dlen,h);

			// Draw text
			if (splitting) {
				// Make sure the text position is more predictable in case of splitting
				dc.DrawText(temptext,dx+4,(h-th)/2);
			} else {
				dc.DrawText(temptext,dx+(delta/2)+4,(h-th)/2);
			}

			// Draw pending splits
			if (syl.pending_splits.size() > 0) {
				wxArrayInt widths;
				if (dc.GetPartialTextExtents(temptext, widths)) {
					for (unsigned int i = 0; i < syl.pending_splits.size(); i++) {
						dc.SetPen(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
						dc.DrawLine(dx+4+widths[syl.pending_splits[i]], 0, dx+4+widths[syl.pending_splits[i]], h);
					}
				} else {
					wxLogError(_T("WTF? Failed to GetPartialTextExtents"));
				}
			}

			if (splitting && split_cursor_syl == i && split_cursor_x > 0) {
				dc.SetPen(*wxRED);
				dc.DrawLine(dx+4+split_cursor_x, 0, dx+4+split_cursor_x, h);
				dc.SetPen(wxPen(wxColour(0,0,0)));
			}

			// Set syllable data
			syl.display_x = dx;
			syl.display_w = dlen;

			// Increment dx
			dx += dlen;
		}
	}

	// End paint
	dc.EndDrawing();
	event.Skip();
}


//////////////
// Size event
void AudioKaraoke::OnSize(wxSizeEvent &event) {
	Refresh(false);
}


///////////////
// Mouse event
void AudioKaraoke::OnMouse(wxMouseEvent &event) {
	// Get coordinates
	int x = event.GetX();
	int y = event.GetY();
	bool shift = event.m_shiftDown;

	if (!splitting) {
		// Left button down
		if (event.LeftDown()) {
			int syl = GetSylAtX(x);

			if (syl != -1) {
				if (shift) {
					SetSelection(syl,startClickSyl);
					Refresh(false);
				}

				else {
					SetSelection(syl);
					startClickSyl = syl;
					curSyllable = syl;
					Refresh(false);
					display->Update();
				}
			}
		}
	} else {
		int syli = GetSylAtX(x);

		if (syli != -1) {
			KaraokeSyllable &syl = syllables.at(syli);

			// Get the widths after each character in the text
			wxClientDC dc(this);
			wxFont curFont(9,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,_T("Verdana"),wxFONTENCODING_SYSTEM);
			dc.SetFont(curFont);
			wxArrayInt widths;
			dc.GetPartialTextExtents(syl.contents, widths);

			// Find the character closest to the mouse
			int rx = x - syl.display_x - 4;
			int split_cursor_char = -1;
			split_cursor_syl = -1;
			split_cursor_x = -1;
			if (syl.contents.Len() >= 2) {
				int lastx = widths[0];
				split_cursor_syl = syli;
				for (unsigned int i = 1; i < widths.size()-1; i++) {
					//wxLogDebug(_T("rx=%d, lastx=%d, widths[i]=%d, i=%d, widths.size()=%d, syli=%d"), rx, lastx, widths[i], i, widths.size(), syli);
					if (lastx - rx < widths[i] - rx) {
						if (rx - lastx < widths[i] - rx) {
							//wxLogDebug(_T("Found at PREV!"));
							split_cursor_x = lastx;
							split_cursor_char = i - 1;
							break;
						} else if (rx < widths[i]) {
							//wxLogDebug(_T("Found at CURRENT!"));
							split_cursor_x = widths[i];
							split_cursor_char = i;
							break;
						}
					}
					lastx = widths[i];
				}
				// If no split-point was caught by the for-loop, it must be over the last character,
				// ie. split at next-to-last position
				if (split_cursor_x < 0) {
					//wxLogDebug(_T("Emergency picking LAST!"));
					split_cursor_x = widths[widths.size()-2];
					split_cursor_char = widths.size() - 2;
				}
			}

			// Do something if there was a click and we're at a valid position
			if (event.LeftDown() && split_cursor_char >= 0) {
				//wxLogDebug(_T("A click!"));
				int num_removed = 0;
				std::vector<int>::iterator i = syl.pending_splits.begin();
				while (i != syl.pending_splits.end()) {
					if (split_cursor_char == *i) {
						//wxLogDebug(_T("Erasing entry"));
						num_removed++;
						syl.pending_splits.erase(i);
					} else {
						i++;
					}
				}
				if (num_removed == 0) {
					syl.pending_splits.push_back(split_cursor_char);
				}
			}

		} else {
			split_cursor_syl = -1;
			split_cursor_x = -1;
		}

		if (split_cursor_syl >= 0) {
			Refresh(false);
		}
	}
}


//////////////////////////////
// Get Syllable at position X
int AudioKaraoke::GetSylAtX(int x) {
	int dx,dw;
	size_t syln = syllables.size();
	for (size_t i=0;i<syln;i++) {
		dx = syllables.at(i).display_x; 
		dw = syllables.at(i).display_w; 
		if (x >= dx && x < dx+dw) {
			return i;
		}
	}
	return -1;
}


/////////////////
// Set selection
void AudioKaraoke::SetSelection(int start,int end) {
	// Default end
	if (end == -1) end = start;

	// Get min/max range
	size_t min = start;
	size_t max = end;
	if (max < min) {
		size_t temp = max;
		max = min;
		min = temp;
	}

	// Set values
	bool state;
	size_t syls = syllables.size();
	int sels = 0;
	for (size_t i=0;i<syls;i++) {
		state = (i >= min && i <= max);
		syllables.at(i).selected = state;
		if (state) sels++;
	}

	// Set box buttons
	box->SetKaraokeButtons(sels > 1,sels > 0);
}


//////////////////
// Join syllables
void AudioKaraoke::Join() {
	// Variables
	bool gotOne = false;
	size_t syls = syllables.size();
	KaraokeSyllable *curSyl;
	int first = 0;

	// Loop
	for (size_t i=0;i<syls;i++) {
		curSyl = &syllables.at(i);
		if (curSyl->selected) {
			if (!gotOne) {
				gotOne = true;
				first = i;
			}
			else {
				syllables.at(i-1).length += curSyl->length;
				syllables.at(i-1).contents += curSyl->contents;
				syllables.erase(syllables.begin()+i);
				i--;
				syls--;
			}
		}
	}

	// Set selection
	curSyllable = first;

	// Update
	must_rebuild = true;
	display->NeedCommit = true;
	display->Update();
	Refresh(false);
}


////////////////////////
// Enter splitting-mode
void AudioKaraoke::BeginSplit() {
	splitting = true;
	split_cursor_syl = -1;
	split_cursor_x = -1;
	box->SetKaraokeButtons(false, true);
	Refresh(false);
}


////////////////////////////////////////////
// Leave splitting-mode, committing changes
void AudioKaraoke::EndSplit(bool commit) {
	splitting = false;
	bool hasSplit = false;
	for (unsigned int i = 0; i < syllables.size(); i ++) {
		if (syllables[i].pending_splits.size() > 0) {
			if (commit) {
				SplitSyl(i);
				hasSplit = true;
			} else {
				syllables[i].pending_splits.clear();
			}
		}
	}

	SetSelection(0);

	// Update
	if (hasSplit) {
		must_rebuild = true;
		display->NeedCommit = true;
		display->Update();
	}
	// Always redraw, since the display is different in splitting mode
	Refresh(false);
}


/////////////////////////////////////////////////
// Split a syllable using the pending_slits data
int AudioKaraoke::SplitSyl (unsigned int n) {
	syllables.reserve(syllables.size() + syllables[n].pending_splits.size());

	// Start by sorting the split points
	std::sort(syllables[n].pending_splits.begin(),syllables[n].pending_splits.end());

	wxString originalText = syllables[n].contents;
	int originalDuration = syllables[n].length;

	// Fixup the first syllable
	syllables[n].contents = originalText.Mid(0, syllables[n].pending_splits[0] + 1);
	syllables[n].length = originalDuration * syllables[n].contents.Length() / originalText.Length();
	int curpos = syllables[n].position + syllables[n].length;

	// For each split, make a new syllable
	for (unsigned int i = 0; i < syllables[n].pending_splits.size(); i++) {
		KaraokeSyllable temp;
		if (i < syllables[n].pending_splits.size()-1) {
			// in the middle
			temp.contents = originalText.Mid(syllables[n].pending_splits[i]+1, syllables[n].pending_splits[i+1] - syllables[n].pending_splits[i]);
		} else {
			// the last one (take the rest)
			temp.contents = originalText.Mid(syllables[n].pending_splits[i]+1);
		}
		temp.length = originalDuration * temp.contents.Length() / originalText.Length();
		temp.position = curpos;
		temp.tag = syllables[n].tag;
		curpos += temp.length;
		syllables.insert(syllables.begin()+n+i+1, temp);
	}

	// The total duration of the new syllables will be equal to or less than the original duration
	// Fix this, so they'll always add up
	// Use an unfair method, just adding 1 to each syllable one after another, until it's correct
	int newDuration = 0;
	for (unsigned int j = n; j < syllables[n].pending_splits.size()+n+1; j++) {
		newDuration += syllables[j].length;
	}
	unsigned int k = n;
	while (newDuration < originalDuration) {
		syllables[k].length++;
		k++;
		if (k >= syllables.size()) {
			k = n;
		}
		newDuration++;
	}

	// Prepare for return and clear pending splits
	int numsplits = syllables[n].pending_splits.size();
	syllables[n].pending_splits.clear();
	return numsplits;
}


//////////////////////////////////
// Apply delta length to syllable
bool AudioKaraoke::SyllableDelta(int n,int delta,int mode) {
	// Get syllable and next
	KaraokeSyllable *curSyl=NULL,*nextSyl=NULL;
	curSyl = &syllables.at(n);
	int nkar = syllables.size();
	if (n < nkar-1) {
		nextSyl = &syllables.at(n+1);
	}

	// Get variables
	int len = curSyl->length;

	// Cap delta
	int minLen = 0;
	if (len + delta < minLen) delta = minLen-len;
	if (mode == 0 && nextSyl && (nextSyl->length - delta) < minLen) delta = nextSyl->length - minLen;

	// Apply
	if (delta != 0) {
		curSyl->length += delta;

		// Normal mode
		if (mode == 0 && nextSyl) {
			nextSyl->length -= delta;
			nextSyl->position += delta;
		}

		// Shift mode
		if (mode == 1) {
			for (int i=n+1;i<nkar;i++) {
				syllables.at(i).position += delta;
			}
		}

		// Flag update
		return true;
	}
	return false;
}
