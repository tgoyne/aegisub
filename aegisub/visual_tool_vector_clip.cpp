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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


///////////
// Headers
#include "visual_tool_vector_clip.h"
#include "ass_dialogue.h"


///////
// IDs
enum {
	BUTTON_DRAG = 1300,
	BUTTON_LINE,
	BUTTON_BICUBIC,
	BUTTON_INSERT,
	BUTTON_REMOVE,
	BUTTON_CONVERT,
	BUTTON_FREEHAND
};


///////////////
// Constructor
VisualToolVectorClip::VisualToolVectorClip(VideoDisplay *parent,wxToolBar *toolBar)
: VisualTool(parent)
{
	DoRefresh();

	// Create toolbar
	toolBar->AddTool(BUTTON_DRAG,_("Drag"),wxBITMAP(visual_vector_clip_drag),_("Drag control points."),wxITEM_CHECK);
	toolBar->AddTool(BUTTON_LINE,_("Line"),wxBITMAP(visual_vector_clip_line),_("Appends a line."),wxITEM_CHECK);
	toolBar->AddTool(BUTTON_BICUBIC,_("Bicubic"),wxBITMAP(visual_vector_clip_bicubic),_("Appends a bezier bicubic curve."),wxITEM_CHECK);
	toolBar->AddSeparator();
	toolBar->AddTool(BUTTON_CONVERT,_("Convert"),wxBITMAP(visual_vector_clip_convert),_("Converts a segment between line and bicubic."),wxITEM_CHECK);
	toolBar->AddTool(BUTTON_INSERT,_("Insert"),wxBITMAP(visual_vector_clip_insert),_("Inserts a control point."),wxITEM_CHECK);
	toolBar->AddTool(BUTTON_REMOVE,_("Remove"),wxBITMAP(visual_vector_clip_remove),_("Removes a control point."),wxITEM_CHECK);
	toolBar->AddSeparator();
	toolBar->AddTool(BUTTON_FREEHAND,_("Freehand"),wxBITMAP(visual_vector_clip_freehand),_("Draws a freehand shape."),wxITEM_CHECK);
	toolBar->Realize();
	toolBar->Show(true);
}


//////////
// Update
void VisualToolVectorClip::Update() {
	GetParent()->Render();
}


////////
// Draw
void VisualToolVectorClip::Draw() {
	// Get line
	AssDialogue *line = GetActiveDialogueLine();
	if (!line) return;

	// Parse vector
	std::vector<Vector2D> points;
	spline.GetPointList(points);

	// Draw lines
	SetLineColour(colour[3],1.0f,2);
	SetFillColour(colour[3],0.0f);
	for (int i=0;i<((signed)points.size())-1;i++) {
		DrawLine(points[i].x,points[i].y,points[i+1].x,points[i+1].y);
	}

	// Draw stencil mask
	glEnable(GL_STENCIL_TEST);
	glColorMask(0,0,0,0);
	glStencilFunc(GL_NEVER,1,1);
	glStencilOp(GL_INVERT,GL_INVERT,GL_INVERT);
	for (int i=0;i<((signed)points.size())-1;i++) {
		glBegin(GL_TRIANGLES);
			glVertex2f(points[0].x,points[0].y);
			glVertex2f(points[i].x,points[i].y);
			glVertex2f(points[i+1].x,points[i+1].y);
		glEnd();
	}

	// Draw "outside clip" mask
	glColorMask(1,1,1,1);
	glStencilFunc(GL_EQUAL, 0, 1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	SetLineColour(colour[3],0.0f);
	SetFillColour(wxColour(0,0,0),0.5f);
	DrawRectangle(0,0,sw,sh);
	glDisable(GL_STENCIL_TEST);

	// Draw features
	DrawAllFeatures();

	// Draw lines connecting the bicubic features
	SetLineColour(colour[3],0.9f,1);
	for (std::list<SplineCurve>::iterator cur=spline.curves.begin();cur!=spline.curves.end();cur++) {
		if (cur->type == CURVE_BICUBIC) {
			DrawDashedLine(cur->p1.x,cur->p1.y,cur->p2.x,cur->p2.y,6);
			DrawDashedLine(cur->p3.x,cur->p3.y,cur->p4.x,cur->p4.y,6);
		}
	}
}


/////////////////////////
// Populate feature list
void VisualToolVectorClip::PopulateFeatureList() {
	// Clear
	features.clear();
	VisualDraggableFeature feat;
	
	// Go through each curve
	bool isFirst = true;
	int i = 0;
	for (std::list<SplineCurve>::iterator cur=spline.curves.begin();cur!=spline.curves.end();cur++,i++) {
		// First point
		if (isFirst) {
			isFirst = false;
			feat.x = cur->p1.x;
			feat.y = cur->p1.y;
			feat.type = DRAG_SMALL_CIRCLE;
			feat.value = i;
			feat.value2 = 0;
			features.push_back(feat);
		}

		// Line
		if (cur->type == CURVE_LINE) {
			feat.x = cur->p2.x;
			feat.y = cur->p2.y;
			feat.type = DRAG_SMALL_CIRCLE;
			feat.value = i;
			feat.value2 = 1;
			features.push_back(feat);
		}

		// Bicubic
		if (cur->type == CURVE_BICUBIC) {
			// Current size
			int size = features.size();

			// Control points
			feat.x = cur->p2.x;
			feat.y = cur->p2.y;
			feat.value = i;
			feat.value2 = 1;
			feat.brother[0] = size-1;
			feat.type = DRAG_SMALL_SQUARE;
			features.push_back(feat);
			feat.x = cur->p3.x;
			feat.y = cur->p3.y;
			feat.value2 = 2;
			feat.brother[0] = size+2;
			features.push_back(feat);

			// End point
			feat.x = cur->p4.x;
			feat.y = cur->p4.y;
			feat.type = DRAG_SMALL_CIRCLE;
			feat.value2 = 3;
			features.push_back(feat);
		}
	}
}


//////////
// Update
void VisualToolVectorClip::UpdateDrag(VisualDraggableFeature &feature) {
	spline.MovePoint(feature.value,feature.value2,wxPoint(feature.x,feature.y));
}


//////////
// Commit
void VisualToolVectorClip::CommitDrag(VisualDraggableFeature &feature) {
	SetOverride(_T("\\clip"),_T("(") + spline.EncodeToASS() + _T(")"));
}


///////////
// Refresh
void VisualToolVectorClip::DoRefresh() {
	if (!dragging) {
		// Get line
		AssDialogue *line = GetActiveDialogueLine();
		if (!line) return;

		// Get clip vector
		wxString vect;
		int scale;
		vect = GetLineVectorClip(line,scale);
		if (vect.IsEmpty()) return;
		spline.DecodeFromASS(vect);
		PopulateFeatureList();
	}
}
