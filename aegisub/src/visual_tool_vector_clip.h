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

/// @file visual_tool_vector_clip.h
/// @see visual_tool_vector_clip.cpp
/// @ingroup visual_ts
///




///////////
// Headers
#ifndef AGI_PRE
#include <wx/toolbar.h>
#endif

#include "visual_tool.h"
#include "spline.h"


/// DOCME
/// @class VisualToolVectorClip
/// @brief DOCME
///
/// DOCME
class VisualToolVectorClip : public VisualTool {
private:

	/// DOCME
	Spline spline;

	/// DOCME
	wxToolBar *toolBar;

	/// DOCME
	int mode;

	/// DOCME

	/// DOCME
	int lastX,lastY;

	/// DOCME
	bool inverse;

	void SetMode(int mode);
	

	/// @brief DOCME
	/// @return 
	///
	bool CanHold() { return true; }
	bool HoldEnabled();
	void InitializeHold();
	void UpdateHold();
	void CommitHold();


	/// @brief DOCME
	///
	bool CanDrag() { return true; }
	bool DragEnabled();
	void PopulateFeatureList();
	void UpdateDrag(VisualDraggableFeature &feature);
	void CommitDrag(VisualDraggableFeature &feature);
	void ClickedFeature(VisualDraggableFeature &feature);

	void DoRefresh();
	void OnSubTool(wxCommandEvent &event);

public:
	VisualToolVectorClip(VideoDisplay *parent,wxToolBar *toolbar);

	void Update();
	void Draw();
};


