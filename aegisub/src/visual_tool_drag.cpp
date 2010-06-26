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
static const DraggableFeatureType DRAG_ORIGIN = DRAG_BIG_TRIANGLE;
static const DraggableFeatureType DRAG_START = DRAG_BIG_SQUARE;
static const DraggableFeatureType DRAG_END = DRAG_BIG_CIRCLE;

/// @brief Constructor 
/// @param _parent 
/// @param toolBar 
VisualToolDrag::VisualToolDrag(VideoDisplay *parent, VideoState const& video, wxToolBar * toolBar)
: VisualTool<VisualToolDragDraggableFeature>(parent, video)
, toolBar(toolBar)
, primary(NULL)
, toggleMoveOnMove(true)
{
	toolBar->AddTool(BUTTON_TOGGLE_MOVE, _("Toggle between \\move and \\pos"), GETIMAGE(visual_move_conv_move_24));
	toolBar->Realize();
	toolBar->Show(true);
}

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
void VisualToolDrag::OnSubTool(wxCommandEvent &) {
	BaseGrid* grid = VideoContext::Get()->grid;
	wxArrayInt sel = GetSelection();
	// Toggle \move <-> \pos
	for (wxArrayInt::const_iterator cur = sel.begin(); cur != sel.end(); ++cur) {
		int x1,y1,x2,y2,t1,t2;
		bool hasMove;

		AssDialogue *line = grid->GetDialogue(*cur);
		if (!line) continue;

		GetLinePosition(line,x1,y1);
		GetLineMove(line,hasMove,x1,y1,x2,y2,t1,t2);
		parent->ToScriptCoords(&x1, &y1);
		parent->ToScriptCoords(&x2, &y2);

		if (hasMove) SetOverride(line, L"\\pos",wxString::Format(L"(%i,%i)",x1,y1));
		else SetOverride(line, L"\\move",wxString::Format(L"(%i,%i,%i,%i,%i,%i)",x1,y1,x1,y1,0,line->End.GetMS() - line->Start.GetMS()));
	}

	Commit(true);
	Refresh();
}

void VisualToolDrag::DoRefresh() {
	UpdateToggleButtons();
}

void VisualToolDrag::OnSelectedSetChanged(const Selection &added, const Selection &removed) {
	if (!externalChange) return;
	externalChange = false;

	// Remove all deselected lines
	for (size_t i = 0; i < features.size(); i++) {
		if (removed.find(features[i].line) != removed.end()) {
			RemoveSelection(i);
		}
	}

	// And add all newly selected lines
	for (size_t i = 0; i < features.size(); i++) {
		if (added.find(features[i].line) != added.end() && features[i].type == DRAG_START) {
			AddSelection(i);
		}
	}

	externalChange = true;
}

void VisualToolDrag::Draw() {
	DrawAllFeatures();

	// Draw arrows
	for (feature_iterator cur = features.begin(); cur != features.end(); ++cur) {
		if (cur->type == DRAG_START) continue;
		VisualDraggableFeature *p2 = &*cur;
		VisualDraggableFeature *p1 = &features[cur->parent];

		// Has arrow?
		bool hasArrow = p2->type == DRAG_END;
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

void VisualToolDrag::PopulateFeatureList() {
	ClearSelection(false);
	primary = -1;
	GenerateFeatures();
}
void VisualToolDrag::GenerateFeatures() {
	features.clear();

	// Get video data
	BaseGrid* grid = VideoContext::Get()->grid;
	int numRows = grid->GetRows();

	for (int i=numRows;--i>=0;) {
		AssDialogue *diag = grid->GetDialogue(i);
		if (!diag || !BaseGrid::IsDisplayed(diag)) continue;

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
		feat.type = DRAG_START;
		feat.time = t1;
		feat.line = diag;
		feat.lineN = i;
		features.push_back(feat);
		feat.parent = features.size() - 1;
		if (grid->IsInSelection(i)) {
			AddSelection(features.size() - 1);
		}

		// Create move destination feature
		if (hasMove) {
			feat.x = x2;
			feat.y = y2;
			feat.layer = 1;
			feat.type = DRAG_END;
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
			feat.type = DRAG_ORIGIN;
			feat.time = 0;
			feat.line = diag;
			feat.lineN = i;
			features.push_back(feat);
		}
	}
}

bool VisualToolDrag::InitializeDrag(VisualToolDragDraggableFeature *feature) {
	primary = feature - &features[0];
	return true;
}

/// @brief Update drag 
/// @param feature 
void VisualToolDrag::UpdateDrag(VisualToolDragDraggableFeature* feature) {
	// Update "time" to reflect the time of the frame in which the feature is being dragged
	int time = VFR_Output.GetTimeAtFrame(frame_n,true,true);
	feature->time = MID(0,time - feature->line->Start.GetMS(),feature->line->End.GetMS()-feature->line->Start.GetMS());
}

/// @brief Commit drag 
/// @param feature 
void VisualToolDrag::CommitDrag(VisualToolDragDraggableFeature* feature) {
	if (feature->type == DRAG_ORIGIN) {
		int x = feature->x;
		int y = feature->y;
		parent->ToScriptCoords(&x, &y);
		SetOverride(feature->line, L"\\org",wxString::Format(L"(%i,%i)",x,y));
		return;
	}

	VisualToolDragDraggableFeature *p = feature->parent > -1 ? &features[feature->parent] : NULL;
	if (feature->type == DRAG_END) {
		std::swap(feature, p);
	}

	int x1 = feature->x;
	int y1 = feature->y;
	parent->ToScriptCoords(&x1, &y1);

	// Position
	if (!p) {
		SetOverride(feature->line, L"\\pos", wxString::Format(L"(%i,%i)", x1, y1));
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
bool VisualToolDrag::Update() {
	if (!leftDClick) return false;

	int dx, dy;
	int vx = video.x;
	int vy = video.y;
	parent->ToScriptCoords(&vx, &vy);
	if (primary > -1) {
		dx = features[primary].x;
		dy = features[primary].y;
	}
	else {
		AssDialogue* line = GetActiveDialogueLine();
		if (!line) return false;
		GetLinePosition(line, dx, dy);
	}
	parent->ToScriptCoords(&dx, &dy);
	dx -= vx;
	dy -= vy;

	SubtitlesGrid *grid = VideoContext::Get()->grid;
	wxArrayInt sel = grid->GetSelection();
	for (wxArrayInt::const_iterator cur = sel.begin(); cur != sel.end(); ++cur) {
		AssDialogue* line = grid->GetDialogue(*cur);
		if (!line) continue;
		int x1 = 0, y1 = 0, x2 = 0, y2 = 0, t1 = INT_MIN, t2 = INT_MIN, orgx, orgy;
		bool isMove;

		GetLinePosition(line, x1, y1, orgx, orgy);
		GetLineMove(line, isMove, x1, y1, x2, y2, t1, t2);
		parent->ToScriptCoords(&x1, &y1);
		parent->ToScriptCoords(&x2, &y2);
		parent->ToScriptCoords(&orgx, &orgy);

		if (isMove) {
			if (t1 > INT_MIN && t2 > INT_MIN)
				SetOverride(line, L"\\move", wxString::Format(L"(%i,%i,%i,%i,%i,%i)", x1 - dx, y1 - dy, x2 - dx, y2 - dy, t1, t2));
			else
				SetOverride(line, L"\\move", wxString::Format(L"(%i,%i,%i,%i)", x1, y1, x2, y2));
		}
		else {
			SetOverride(line, L"\\pos", wxString::Format(L"(%i,%i)", x1 - dx, y1 - dy));
		}
		if (orgx != x1 || orgy != y1) {
			SetOverride(line, L"\\org", wxString::Format(L"(%i,%i)", orgx - dx, orgy - dy));
		}
	}

	Commit(true, _("positioning"));

	GenerateFeatures();
	return false;
}
