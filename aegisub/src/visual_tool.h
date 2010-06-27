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

/// @file visual_tool.h
/// @see visual_tool.cpp
/// @ingroup visual_ts

#pragma once

#ifndef AGI_PRE
#include <map>
#include <set>
#include <vector>

#include <wx/log.h>
#include <wx/event.h>
#include <wx/button.h>
#endif

#include "base_grid.h"
#include "gl_wrap.h"

class VideoDisplay;
class AssDialogue;
struct VideoState;
namespace agi {
	class OptionValue;
}

/// First window id for visualsubtoolbar items
#define VISUAL_SUB_TOOL_START 1300

/// Last window id for visualsubtoolbar items
#define VISUAL_SUB_TOOL_END (VISUAL_SUB_TOOL_START+100)

class IVisualTool : public OpenGLWrapper {
protected:
	/// DOCME
	static const wxColour colour[4];
public:
	virtual void OnMouseEvent(wxMouseEvent &event)=0;
	virtual void OnSubTool(wxCommandEvent &)=0;
	virtual bool Update()=0;
	virtual void Draw()=0;
	virtual void Refresh()=0;
	virtual ~IVisualTool() { };
};

/// DOCME
/// @class VisualTool
/// @brief DOCME
/// DOCME
template<class FeatureType>
class VisualTool : public IVisualTool, protected SubtitleSelectionListener {
private:
	agi::OptionValue* realtime; /// Realtime updating option
	int dragStartX; /// Starting x coordinate of the current drag, if any
	int dragStartY; /// Starting y coordinate of the current drag, if any

	/// Set curFeature and curFeatureI to the topmost feature under the mouse,
	/// or NULL and -1 if there are none
	void GetHighlightedFeature();

	typedef typename std::set<int>::iterator selection_iterator;

	std::set<int> selFeatures; /// Currently selected visual features
	std::map<int, int> lineSelCount; /// Number of selected features for each line

	/// @brief Set the edit box's active line, ensuring proper sync with grid
	/// @param lineN Line number or -1 for automatic selection
	///
	/// This function ensures that the selection is not empty and that the line
	/// displayed in the edit box is part of the selection, by either setting
	/// the edit box to the selection or setting the selection to the edit
	/// box's line, as is appropriate.
	void SetEditbox(int lineN = -1);

	bool selChanged; /// Has the selection already been changed in the current click?
protected:
	VideoDisplay *parent; /// VideoDisplay which this belongs to, used to frame conversion
	bool holding; /// Is a hold currently in progress?
	AssDialogue *curDiag; /// Active dialogue line for a hold; only valid when holding = true
	bool dragging; /// Is a drag currently in progress?
	bool externalChange; /// Only invalid drag lists when refreshing due to external changes

	FeatureType* curFeature; /// Topmost feature under the mouse; generally only valid during a drag
	unsigned curFeatureI; /// Index of the current feature in the list
	std::vector<FeatureType> features; /// List of features which are drawn and can be clicked on
	bool dragListOK; /// Do the features not need to be regenerated?

	int frame_n; /// Current frame number
	VideoState const& video; /// Mouse and video information

	bool leftClick; /// Is a left click event currently being processed?
	bool leftDClick; /// Is a left double click event currently being processed?
	bool shiftDown; /// Is shift down?
	bool ctrlDown; /// Is ctrl down?
	bool altDown; /// Is alt down?

	void GetLinePosition(AssDialogue *diag,int &x,int &y);
	void GetLinePosition(AssDialogue *diag,int &x,int &y,int &orgx,int &orgy);
	void GetLineMove(AssDialogue *diag,bool &hasMove,int &x1,int &y1,int &x2,int &y2,int &t1,int &t2);
	void GetLineRotation(AssDialogue *diag,float &rx,float &ry,float &rz);
	void GetLineScale(AssDialogue *diag,float &scalX,float &scalY);
	void GetLineClip(AssDialogue *diag,int &x1,int &y1,int &x2,int &y2,bool &inverse);
	wxString GetLineVectorClip(AssDialogue *diag,int &scale,bool &inverse);
	void SetOverride(AssDialogue* line, wxString tag, wxString value);

	/// @brief Get the dialogue line currently in the edit box
	/// @return NULL if the line is not active on the current frame
	AssDialogue *GetActiveDialogueLine();
	/// Draw all of the features in the list
	void DrawAllFeatures();
	/// @brief Commit the current file state
	/// @param message Description of changes for undo
	void Commit(bool full=false, wxString message = L"");

	/// @brief Called when a hold is begun
	/// @return Should the hold actually happen?
	virtual bool InitializeHold() { return false; }

	/// @brief Called on every mouse event during a hold
	virtual void UpdateHold() { }

	/// @brief Called at the end of a hold
	virtual void CommitHold() { }

	/// @brief Called when the feature list needs to be (re)generated
	virtual void PopulateFeatureList() { }

	/// @brief Called at the beginning of a drag
	/// @param feature The visual feature clicked on
	/// @return Should the drag happen?
	virtual bool InitializeDrag(FeatureType* feature) { return true; }

	/// @brief Called on every mouse event during a drag
	/// @param feature The current feature to process; not necessarily the one clicked on
	virtual void UpdateDrag(FeatureType* feature) { }

	// @brief Called at the end of a drag
	virtual void CommitDrag(FeatureType* feature) { }

	/// @brief Called by Refresh when there's stuff
	virtual void DoRefresh() { }

	/// @brief Add a feature (and its line) to the selection
	/// @param i Index in the feature list
	void AddSelection(unsigned i);
	/// @brief Remove a feature from the selection
	/// @param i Index in the feature list
	/// Also deselects lines if all features for that line have been deselected
	void RemoveSelection(unsigned i);
	/// @brief Clear the selection
	/// @param hard Should the grid's selection be cleared as well?
	void ClearSelection(bool hard=true);
	/// @brief Get the currently selected lines
	wxArrayInt GetSelection();

	typedef typename std::vector<FeatureType>::iterator feature_iterator;
	typedef typename std::vector<FeatureType>::const_iterator feature_const_iterator;

protected:
	// SubtitleSelectionListener implementation
	// (overridden by deriving classes too)
	virtual void OnActiveLineChanged(AssDialogue *new_line) { }
	virtual void OnSelectedSetChanged(const Selection &lines_added, const Selection &lines_removed) { }

public:
	/// @brief Handler for all mouse events
	/// @param event Shockingly enough, the mouse event
	void OnMouseEvent(wxMouseEvent &event);

	/// @brief Event handler for the subtoolbar
	virtual void OnSubTool(wxCommandEvent &) { }
	/// @brief Called when there's stuff
	/// @return Should the display rerender?
	virtual bool Update() { return false; };
	/// @brief Draw stuff
	virtual void Draw()=0;
	/// @brief Called by stuff when there's stuff, and sometimes when there isn't stuff
	void Refresh();

	/// @brief Constructor
	/// @param parent The VideoDisplay to use for coordinate conversion
	/// @param video Video and mouse information passing blob
	VisualTool(VideoDisplay *parent, VideoState const& video);

	/// @brief Destructor
	virtual ~VisualTool();
};
