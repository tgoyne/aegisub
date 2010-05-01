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
//
// $Id$

/// @file visual_tool.cpp
/// @brief Base class for visual typesetting functions
/// @ingroup visual_ts
///


//////////////
// Headers
#include "config.h"

#ifndef AGI_PRE
#include <wx/glcanvas.h>
#endif

#ifdef __APPLE__
#include <OpenGL/GL.h>
#else
#include <GL/gl.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "ass_time.h"
#include "export_visible_lines.h"
#include "options.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "utils.h"
#include "vfr.h"
#include "video_context.h"
#include "video_display.h"
#include "video_provider_manager.h"
#include "visual_tool.h"


/// @brief Constructor 
/// @param par 
///
VisualTool::VisualTool(VideoDisplay *par) : eventSink(this) {
	// Config
	parent = par;
	colour[0] = wxColour(106,32,19);
	colour[1] = wxColour(255,169,40);
	colour[2] = wxColour(255,253,185);
	colour[3] = wxColour(187,0,0);

	// Holding variables
	holding = false;
	curDiag = NULL;

	// Dragging variables
	dragging = false;
	curFeature = -1;
	dragListOK = false;

	// Video options
	mouseX = mouseY = INT_MIN;
	frame_n = 0;
	if (VideoContext::Get()->IsLoaded()) {
		parent->GetClientSize(&w,&h);
		VideoContext::Get()->GetScriptSize(sw,sh);
		frame_n = VideoContext::Get()->GetFrameN();
	}

	// Features
	if (CanDrag()) PopulateFeatureList();
}



/// @brief Destructor 
///
VisualTool::~VisualTool() {
}



/// @brief Mouse event 
/// @param event 
/// @return 
///
void VisualTool::OnMouseEvent (wxMouseEvent &event) {
	// General variables
	mouseX = event.GetX();
	mouseY = event.GetY();
	parent->ConvertMouseCoords(mouseX,mouseY);
	parent->GetClientSize(&w,&h);
	VideoContext::Get()->GetScriptSize(sw,sh);
	frame_n = VideoContext::Get()->GetFrameN();
	bool realTime = Options.AsBool(_T("Video Visual Realtime"));

	// Mouse leaving control
	if (event.Leaving()) {
		mouseX = INT_MIN;
		mouseY = INT_MIN;
		mx = INT_MIN;
		my = INT_MIN;
		Update();
		return;
	}

	// Transformed mouse x/y
	mx = mouseX * sw / w;
	my = mouseY * sh / h;

	// Clicks
	leftClick = event.ButtonDown(wxMOUSE_BTN_LEFT);
	leftDClick = event.LeftDClick();
	shiftDown = event.m_shiftDown;
#ifdef __APPLE__
	ctrlDown = event.m_metaDown; // Cmd key
#else
	ctrlDown = event.m_controlDown;
#endif
	altDown = event.m_altDown;

	// Drag a feature
	if (CanDrag()) {
		// Populate list if needed
		if (!dragListOK) {
			PopulateFeatureList();
			dragListOK = true;
		}

		// Click on feature
		if (!dragging && leftClick && !DragEnabled()) {
			curFeature = GetHighlightedFeature();
			if (curFeature != -1) {
				ClickedFeature(features[curFeature]);
			}
		}

		// Start dragging
		if (!dragging && leftClick && DragEnabled()) {
			// Get a feature
			curFeature = GetHighlightedFeature();
			if (curFeature != -1) {
				// Initialize drag
				InitializeDrag(features[curFeature]);
				if (features[curFeature].lineN != -1) {
					VideoContext::Get()->grid->editBox->SetToLine(features[curFeature].lineN,true);
					VideoContext::Get()->grid->SelectRow(features[curFeature].lineN);
				}

				// Set start value
				dragStartX = mx;
				dragStartY = my;
				dragOrigX = features[curFeature].x;
				dragOrigY = features[curFeature].y;

				// Set flags
				dragging = true;
				parent->CaptureMouse();
				if (realTime) AssLimitToVisibleFilter::SetFrame(frame_n);
			}
		}

		if (dragging) {
			// Dragging
			if (event.LeftIsDown()) {
				// Update position
				features[curFeature].x = (mx - dragStartX + dragOrigX);
				features[curFeature].y = (my - dragStartY + dragOrigY);

				// Update drag
				UpdateDrag(features[curFeature]);

				if (realTime) {
					// Commit
					CommitDrag(features[curFeature]);
					Commit();
				}
			}

			// Release
			else {
				// Disable limiting
				if (realTime) AssLimitToVisibleFilter::SetFrame(-1);

				// Commit
				dragging = false;
				CommitDrag(features[curFeature]);
				Commit(true);

				// Clean up
				curFeature = -1;
				parent->ReleaseMouse();
				parent->SetFocus();
			}
		}

	}

	// Hold
	if (!dragging && CanHold()) {
		// Start holding
		if (!holding && event.LeftIsDown() && HoldEnabled()) {
			// Get a dialogue
			curDiag = GetActiveDialogueLine();
			if (curDiag) {
				// Initialize Drag
				InitializeHold();

				// Set flags
				holding = true;
				parent->CaptureMouse();
				if (realTime) AssLimitToVisibleFilter::SetFrame(frame_n);
			}
		}

		if (holding) {
			// Holding
			if (event.LeftIsDown()) {
				// Update drag
				UpdateHold();

				if (realTime) {
					// Commit
					CommitHold();
					Commit();
				}
			}

			// Release
			else {
				// Disable limiting
				if (realTime) AssLimitToVisibleFilter::SetFrame(-1);

				// Commit
				holding = false;
				CommitHold();
				Commit(true);

				// Clean up
				curDiag = NULL;
				parent->ReleaseMouse();
				parent->SetFocus();
			}
		}
	}

	// Update
	Update();
}



/// @brief Commit 
/// @param full 
/// @return 
///
void VisualTool::Commit(bool full) {
	// Get grid
	SubtitlesGrid *grid = VideoContext::Get()->grid;

	// See if anything actually changed
	// Fix by jfs: Only if not doing a  full commit. Not sure why, but this avoids bug #532
	if (!full) {
		AssDialogue *diag = grid->GetDialogue(grid->editBox->linen);
		if (diag && grid->editBox->TextEdit->GetText() == diag->Text) return;
	}

	// Commit changes
	grid->editBox->CommitText();
	if (full) grid->ass->FlagAsModified(_("visual typesetting"));
	grid->CommitChanges(false,!full);
}



/// @brief Get active dialogue line 
/// @return 
///
AssDialogue* VisualTool::GetActiveDialogueLine() {
	SubtitlesGrid *grid = VideoContext::Get()->grid;
	AssDialogue *diag = grid->GetDialogue(grid->editBox->linen);

	// Check if it's within range
	if (diag) {
		int f1 = VFR_Output.GetFrameAtTime(diag->Start.GetMS(),true);
		int f2 = VFR_Output.GetFrameAtTime(diag->End.GetMS(),false);

		// Invisible
		if (f1 > frame_n || f2 < frame_n) return NULL;
	}

	return diag;
}



/// @brief Get feature under mouse 
/// @return 
///
int VisualTool::GetHighlightedFeature() {
	int highestLayerFound = -99999;
	int bestMatch = -1;
	for (size_t i=0;i<features.size();i++) {
		if (features[i].IsMouseOver(mx,my) && features[i].layer > highestLayerFound) {
			bestMatch = i;
			highestLayerFound = features[i].layer;
		}
	}
	return bestMatch;
}



/// @brief Draw all features 
///
void VisualTool::DrawAllFeatures() {
	// Populate list, if needed
	if (!dragListOK) {
		PopulateFeatureList();
		dragListOK = true;
	}

	// Get feature that mouse is over
	int mouseOver = curFeature;
	if (curFeature == -1) mouseOver = GetHighlightedFeature();

	// Draw features
	for (size_t i=0;i<features.size();i++) {
		SetFillColour(colour[(signed)i == mouseOver ? 2 : 1],0.6f);
		SetLineColour(colour[0],1.0f,2);
		features[i].Draw(this);
	}
}



/// @brief Refresh 
///
void VisualTool::Refresh() {
	frame_n = VideoContext::Get()->GetFrameN();
	if (!dragging) dragListOK = false;
	DoRefresh();
}



/// @brief Get position of line 
/// @param diag 
/// @param x    
/// @param y    
///
void VisualTool::GetLinePosition(AssDialogue *diag,int &x, int &y) {
	int orgx=0,orgy=0;
	GetLinePosition(diag,x,y,orgx,orgy);
}

/// @brief DOCME
/// @param diag 
/// @param x    
/// @param y    
/// @param orgx 
/// @param orgy 
/// @return 
///
void VisualTool::GetLinePosition(AssDialogue *diag,int &x, int &y, int &orgx, int &orgy) {
	// No dialogue
	if (!diag) {
		x = INT_MIN;
		y = INT_MIN;
		orgx = INT_MIN;
		orgy = INT_MIN;
		return;
	}

	// Default values
	int margin[4];
	for (int i=0;i<4;i++) margin[i] = diag->Margin[i];
	int align = 2;

	// Get style
	AssStyle *style = VideoContext::Get()->grid->ass->GetStyle(diag->Style);
	if (style) {
		align = style->alignment;
		for (int i=0;i<4;i++) {
			if (margin[i] == 0) margin[i] = style->Margin[i];
		}
	}

	// Script size
	int sw,sh;
	VideoContext::Get()->GetScriptSize(sw,sh);

	// Process margins
	margin[3] = margin[2];
	margin[1] = sw - margin[1];
	margin[3] = sh - margin[3];

	// Position
	bool posSet = false;
	bool orgSet = false;
	int posx = INT_MIN;
	int posy = INT_MIN;

	// Overrides processing
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();
	for (size_t i=0;i<blockn;i++) {
		override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(i));
		if (override) {
			for (size_t j=0;j<override->Tags.size();j++) {
				tag = override->Tags.at(j);

				// Position
				if ((tag->Name == _T("\\pos") || tag->Name == _T("\\move")) && tag->Params.size() >= 2) {
					if (!posSet) {
						posx = tag->Params[0]->AsInt();
						posy = tag->Params[1]->AsInt();
						posSet = true;
					}
				}

				// Alignment
				else if ((tag->Name == _T("\\an") || tag->Name == _T("\\a")) && tag->Params.size() >= 1) {
					align = tag->Params[0]->AsInt();
					if (tag->Name == _T("\\a")) {
						switch(align) {
							case 1: align = 1; break;
							case 2: align = 2; break;
							case 3: align = 3; break;
							case 5: align = 7; break;
							case 6: align = 8; break;
							case 7: align = 9; break;
							case 9: align = 4; break;
							case 10: align = 5; break;
							case 11: align = 6; break;
							default: align = 2; break;
						}
					}
				}

				// Origin
				else if (!orgSet && tag->Name == _T("\\org") && tag->Params.size() >= 2) {
					orgx = tag->Params[0]->AsInt();
					orgy = tag->Params[1]->AsInt();
					orgSet = true;
				}
			}
		}
	}
	diag->ClearBlocks();

	// Got position
	if (posSet) {
		x = posx;
		y = posy;
		if (!orgSet) {
			orgx = x;
			orgy = y;
		}
		return;
	}

	// Alignment type
	int hor = (align - 1) % 3;
	int vert = (align - 1) / 3;

	// Calculate positions
	if (hor == 0) x = margin[0];
	else if (hor == 1) x = (margin[0] + margin[1])/2;
	else if (hor == 2) x = margin[1];
	if (vert == 0) y = margin[3];
	else if (vert == 1) y = (margin[2] + margin[3])/2;
	else if (vert == 2) y = margin[2];

	// No origin?
	if (!orgSet) {
		orgx = x;
		orgy = y;
	}
}



/// @brief Get the destination of move, if any 
/// @param diag    
/// @param hasMove 
/// @param x1      
/// @param y1      
/// @param x2      
/// @param y2      
/// @param t1      
/// @param t2      
/// @return 
///
void VisualTool::GetLineMove(AssDialogue *diag,bool &hasMove,int &x1,int &y1,int &x2,int &y2,int &t1,int &t2) {
	// Parse tags
	hasMove = false;
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();

	// For each block
	for (size_t i=0;i<blockn;i++) {
		override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(i));
		if (override) {
			for (size_t j=0;j<override->Tags.size();j++) {
				tag = override->Tags.at(j);

				// Position
				if (tag->Name == _T("\\move") && tag->Params.size() >= 4) {
					hasMove = true;
					x1 = tag->Params[0]->AsInt();
					y1 = tag->Params[1]->AsInt();
					x2 = tag->Params[2]->AsInt();
					y2 = tag->Params[3]->AsInt();
					if (tag->Params.size() >= 6 &&
						!tag->Params[4]->ommited &&
						!tag->Params[5]->ommited) {

						t1 = tag->Params[4]->AsInt();
						t2 = tag->Params[5]->AsInt();
					}
					return;
				}
			}
		}
	}
	diag->ClearBlocks();
}



/// @brief Get line's rotation 
/// @param diag 
/// @param rx   
/// @param ry   
/// @param rz   
/// @return 
///
void VisualTool::GetLineRotation(AssDialogue *diag,float &rx,float &ry,float &rz) {
	// Default values
	rx = ry = rz = 0.0f;

	// No dialogue
	if (!diag) return;

	// Prepare overrides
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();
	if (blockn == 0) {
		diag->ClearBlocks();
		return;
	}

	// Process override
	override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(0));
	if (override) {
		for (size_t j=0;j<override->Tags.size();j++) {
			tag = override->Tags.at(j);
			if (tag->Name == _T("\\frx") && tag->Params.size() == 1) {
				rx = tag->Params[0]->AsFloat();
			}
			if (tag->Name == _T("\\fry") && tag->Params.size() == 1) {
				ry = tag->Params[0]->AsFloat();
			}
			if ((tag->Name == _T("\\frz") || tag->Name == _T("\fr")) && tag->Params.size() == 1) {
				rz = tag->Params[0]->AsFloat();
			}
		}
	}
	diag->ClearBlocks();
}



/// @brief Get line's scale 
/// @param diag  
/// @param scalX 
/// @param scalY 
/// @return 
///
void VisualTool::GetLineScale(AssDialogue *diag,float &scalX,float &scalY) {
	// Default values
	scalX = scalY = 100.0f;

	// Prepare overrides
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();
	if (blockn == 0) {
		diag->ClearBlocks();
		return;
	}

	// Process override
	override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(0));
	if (override) {
		for (size_t j=0;j<override->Tags.size();j++) {
			tag = override->Tags.at(j);
			if (tag->Name == _T("\\fscx") && tag->Params.size() == 1) {
				scalX = tag->Params[0]->AsFloat();
			}
			if (tag->Name == _T("\\fscy") && tag->Params.size() == 1) {
				scalY = tag->Params[0]->AsFloat();
			}
		}
	}
	diag->ClearBlocks();
}



/// @brief Get line's clip 
/// @param diag    
/// @param x1      
/// @param y1      
/// @param x2      
/// @param y2      
/// @param inverse 
/// @return 
///
void VisualTool::GetLineClip(AssDialogue *diag,int &x1,int &y1,int &x2,int &y2,bool &inverse) {
	// Default values
	x1 = y1 = 0;
	int sw,sh;
	VideoContext::Get()->GetScriptSize(sw,sh);
	x2 = sw-1;
	y2 = sh-1;
	inverse = false;

	// Prepare overrides
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();
	if (blockn == 0) {
		diag->ClearBlocks();
		return;
	}

	// Process override
	override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(0));
	if (override) {
		for (size_t j=0;j<override->Tags.size();j++) {
			tag = override->Tags.at(j);
			if (tag->Name == _T("\\clip") && tag->Params.size() == 4) {
				x1 = tag->Params[0]->AsInt();
				y1 = tag->Params[1]->AsInt();
				x2 = tag->Params[2]->AsInt();
				y2 = tag->Params[3]->AsInt();
				inverse = false;
			}
			else if (tag->Name == _T("\\iclip") && tag->Params.size() == 4) {
				x1 = tag->Params[0]->AsInt();
				y1 = tag->Params[1]->AsInt();
				x2 = tag->Params[2]->AsInt();
				y2 = tag->Params[3]->AsInt();
				inverse = true;
			}
		}
	}
	diag->ClearBlocks();
}



/// @brief Get line vector clip, if it exists 
/// @param diag    
/// @param scale   
/// @param inverse 
/// @return 
///
wxString VisualTool::GetLineVectorClip(AssDialogue *diag,int &scale,bool &inverse) {
	// Prepare overrides
	wxString result;
	scale = 1;
	inverse = false;
	diag->ParseASSTags();
	AssDialogueBlockOverride *override;
	AssOverrideTag *tag;
	size_t blockn = diag->Blocks.size();
	if (blockn == 0) {
		diag->ClearBlocks();
		return result;
	}

	// Process override
	override = AssDialogueBlock::GetAsOverride(diag->Blocks.at(0));
	if (override) {
		for (size_t j=0;j<override->Tags.size();j++) {
			tag = override->Tags.at(j);
			if (tag->Name == _T("\\clip") || tag->Name == _T("\\iclip")) {
				if (tag->Params.size() == 1) {
					result = tag->Params[0]->AsText();
				}
				else if (tag->Params.size() == 2) {
					scale = tag->Params[0]->AsInt();
					result = tag->Params[1]->AsText();
				}
				else if (tag->Params.size() == 4) {
					int x1 = tag->Params[0]->AsInt(),
						y1 = tag->Params[1]->AsInt(),
						x2 = tag->Params[2]->AsInt(),
						y2 = tag->Params[3]->AsInt();
					result = wxString::Format(_T("m %d %d l %d %d %d %d %d %d"), x1, y1, x2, y1, x2, y2, x1, y2);
				}
				inverse = tag->Name == _T("\\iclip");
			}
		}
	}
	diag->ClearBlocks();
	return result;
}



/// @brief Set override 
/// @param tag   
/// @param value 
///
void VisualTool::SetOverride(wxString tag,wxString value) {
	VideoContext::Get()->grid->editBox->SetOverride(tag,value,0,false);
	parent->SetFocus();
}



/// @brief Connect button 
/// @param button 
///
void VisualTool::ConnectButton(wxButton *button) {
	button->Connect(wxEVT_COMMAND_BUTTON_CLICKED,wxCommandEventHandler(VisualToolEvent::OnButton),NULL,&eventSink);
}



/// @brief Event sink 
/// @param _tool 
///
VisualToolEvent::VisualToolEvent(VisualTool *_tool) {
	tool = _tool;
}

/// @brief DOCME
/// @param event 
///
void VisualToolEvent::OnButton(wxCommandEvent &event) {
	tool->OnButton(event);
}




