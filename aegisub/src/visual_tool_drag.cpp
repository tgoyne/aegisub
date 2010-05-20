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

/// @file visual_tool_drag.cpp
/// @brief Position all visible subtitles by dragging visual typesetting tool
/// @ingroup visual_ts

#include "config.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "libresrc/libresrc.h"
#include "subs_edit_box.h"
#include "subs_grid.h"
#include "utils.h"
#include "vfr.h"
#include "video_context.h"
#include "video_display.h"
#include "visual_tool_drag.h"

enum {
	BUTTON_TOGGLE_MOVE = VISUAL_SUB_TOOL_START
};

/// @brief Constructor 
/// @param _parent 
/// @param toolBar 
VisualToolDrag::VisualToolDrag(VideoDisplay *parent, VideoState const& video, wxToolBar * toolBar)
: VisualTool(parent, video)
, toolBar(toolBar)
, toggleMoveOnMove(true)
{
	toolBar->AddTool(BUTTON_TOGGLE_MOVE, _("Toggle between \\move and \\pos"), GETIMAGE(visual_move_conv_move_24));
	toolBar->Realize();
	toolBar->Show(true);
}

/// @brief Update toggle buttons 
/// @return 
void VisualToolDrag::UpdateToggleButtons() {
	// Check which bitmap to use
	bool toMove = true;
	AssDialogue *line = GetActiveDialogueLine();
	if (line) {
		int x1,y1,x2,y2,t1,t2;
		bool hasMove;
		GetLineMove(line,hasMove,x1,y1,x2,y2,t1,t2);
		toMove = !hasMove;
	}

	// No change needed
	if (toMove == toggleMoveOnMove) return;

	// Change bitmap
	if (toMove) {
		toolBar->SetToolNormalBitmap(BUTTON_TOGGLE_MOVE, GETIMAGE(visual_move_conv_move_24));
	}
	else {
		toolBar->SetToolNormalBitmap(BUTTON_TOGGLE_MOVE, GETIMAGE(visual_move_conv_pos_24));
	}
	toggleMoveOnMove = toMove;
}

/// @brief Toggle button pressed 
/// @param event 
/// @return 
void VisualToolDrag::OnSubTool(wxCommandEvent &event) {
	// Get line
	AssDialogue *line = GetActiveDialogueLine();
	if (!line) return;

	// Toggle \move <-> \pos
	if (event.GetId() == BUTTON_TOGGLE_MOVE) {
		// Get coordinates
		int x1,y1,x2,y2,t1,t2;
		bool hasMove;
		GetLinePosition(line,x1,y1);
		GetLineMove(line,hasMove,x1,y1,x2,y2,t1,t2);
		parent->ToScriptCoords(&x1, &y1);
		parent->ToScriptCoords(&x2, &y2);

		// Replace tag
		if (hasMove) SetOverride(line, L"\\pos",wxString::Format(L"(%i,%i)",x1,y1));
		else SetOverride(line, L"\\move",wxString::Format(L"(%i,%i,%i,%i,%i,%i)",x1,y1,x1,y1,0,line->End.GetMS() - line->Start.GetMS()));
		Commit(true);

		// Update display
		Refresh();
	}
}

/// @brief Refresh 
void VisualToolDrag::DoRefresh() {
	UpdateToggleButtons();
}

/// @brief Draw 
void VisualToolDrag::Draw() {
	DrawAllFeatures();

	// Draw arrows
	for (size_t i=0;i<features.size();i++) {
		if (features[i].type == DRAG_BIG_SQUARE) continue;
		VisualDraggableFeature *p2 = &features[i];
		VisualDraggableFeature *p1 = &features[features[i].parent];

		// Has arrow?
		bool hasArrow = p2->type == DRAG_BIG_CIRCLE;
		int arrowLen = hasArrow ? 10 : 0;

		// See if the distance between them is enough
		int dx = p2->x - p1->x;
		int dy = p2->y - p1->y;
		int dist = (int)sqrt(double(dx*dx + dy*dy));
		if (dist < 20+arrowLen) continue;

		// Get end points
		int x1 = p1->x + dx*10/dist;
		int x2 = p2->x - dx*(10+arrowLen)/dist;
		int y1 = p1->y + dy*10/dist;
		int y2 = p2->y - dy*(10+arrowLen)/dist;

		// Draw arrow
		if (hasArrow) {
			// Calculate angle
			double angle = atan2(double(y2-y1),double(x2-x1))+1.570796;
			int sx = int(cos(angle)*4);
			int sy = int(-sin(angle)*4);

			// Arrow line
			SetLineColour(colour[3],0.8f,2);
			DrawLine(x1,y1,x2,y2);

			// Arrow head
			DrawLine(x2+sx,y2-sy,x2-sx,y2+sy);
			DrawLine(x2+sx,y2-sy,x2+dx*10/dist,y2+dy*10/dist);
			DrawLine(x2-sx,y2+sy,x2+dx*10/dist,y2+dy*10/dist);
		}

		// Draw dashed line
		else {
			SetLineColour(colour[3],0.5f,2);
			DrawDashedLine(x1,y1,x2,y2,6);
		}
	}
}

/// @brief Populate list 
void VisualToolDrag::PopulateFeatureList() {
	features.clear();

	// Get video data
	int numRows = VideoContext::Get()->grid->GetRows();
	int framen = VideoContext::Get()->GetFrameN();

	// For each line
	AssDialogue *diag;
	for (int i=numRows;--i>=0;) {
		diag = VideoContext::Get()->grid->GetDialogue(i);
		if (diag) {
			// Line visible?
			int f1 = VFR_Output.GetFrameAtTime(diag->Start.GetMS(),true);
			int f2 = VFR_Output.GetFrameAtTime(diag->End.GetMS(),false);
			if (f1 <= framen && f2 >= framen) {
				// Get position
				int x1,x2,y1,y2;
				int t1=0;
				int t2=diag->End.GetMS()-diag->Start.GetMS();
				int torgx,torgy;
				bool hasMove;
				GetLinePosition(diag,x1,y1,torgx,torgy);
				GetLineMove(diag,hasMove,x1,y1,x2,y2,t1,t2);

				// Create \pos feature
				VisualToolDragDraggableFeature feat;
				feat.x = x1;
				feat.y = y1;
				feat.layer = 0;
				feat.type = DRAG_BIG_SQUARE;
				feat.time = t1;
				feat.line = diag;
				feat.lineN = i;
				features.push_back(feat);
				feat.parent = features.size() - 1;

				// Create move destination feature
				if (hasMove) {
					feat.x = x2;
					feat.y = y2;
					feat.layer = 1;
					feat.type = DRAG_BIG_CIRCLE;
					feat.time = t2;
					feat.line = diag;
					feat.lineN = i;
					features.push_back(feat);
					features[feat.parent].parent = features.size() - 1;
				}
				// Create org feature
				if (torgx != x1 || torgy != y1) {
					feat.x = torgx;
					feat.y = torgy;
					feat.layer = -1;
					feat.type = DRAG_BIG_TRIANGLE;
					feat.time = 0;
					feat.line = diag;
					feat.lineN = i;
					features.push_back(feat);
				}
			}
		}
	}
}

/// @brief Update drag 
/// @param feature 
void VisualToolDrag::UpdateDrag(VisualToolDragDraggableFeature &feature) {
	// Update "time" to reflect the time of the frame in which the feature is being dragged
	int time = VFR_Output.GetTimeAtFrame(frame_n,true,true);
	feature.time = MID(0,time - feature.line->Start.GetMS(),feature.line->End.GetMS()-feature.line->Start.GetMS());
}

/// @brief Commit drag 
/// @param feature 
void VisualToolDrag::CommitDrag(VisualToolDragDraggableFeature &feature) {
	// Origin
	if (feature.type == DRAG_BIG_TRIANGLE) {
		int x = feature.x;
		int y = feature.y;
		parent->ToScriptCoords(&x, &y);
		SetOverride(feature.line, L"\\org",wxString::Format(L"(%i,%i)",x,y));
		return;
	}

	VisualToolDragDraggableFeature *p = feature->parent > -1 ? &features[feature->parent] : NULL;
	if (feature->type == DRAG_BIG_CIRCLE) {
		std::swap(feature, p);
	}

	int x1 = feature->x;
	int y1 = feature->y;
	parent->ToScriptCoords(&x1, &y1);

	// Position
	if (!p) {
		SetOverride(feature.line, L"\\pos", wxString::Format(L"(%i,%i)", x1, y1));
	}
	// Move
	else {
		int x2 = p->x;
		int y2 = p->y;
		parent->ToScriptCoords(&x2, &y2);

		// Set override
		SetOverride(feature->line, L"\\move", wxString::Format(L"(%i,%i,%i,%i,%i,%i)", x1, y1, x2, y2, feature->time, p->time));
	}
}
