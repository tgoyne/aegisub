// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// Aegisub Project http://www.aegisub.org/
//
// $Id$

/// @file visual_tool.h
/// @see visual_tool.cpp
/// @ingroup visual_ts

#pragma once

#ifndef AGI_PRE
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>

#include <wx/log.h>
#include <wx/event.h>
#include <wx/button.h>
#endif

#include <libaegisub/signal.h>

#include "gl_wrap.h"
#include "selection_controller.h"
#include "vector2d.h"

class AssDialogue;
class SubtitlesGrid;
class VideoDisplay;
class wxToolBar;
namespace agi {
	struct Context;
	class OptionValue;
}

/// @class VisualToolBase
/// @brief Base class for visual tools containing all functionality that doesn't interact with features
///
/// This is required so that visual tools can be used polymorphically, as
/// different VisualTool<T>s are unrelated types otherwise. In addition, as much
/// functionality as possible is implemented here to avoid having four copies
/// of each method for no good reason (and four times as many error messages)
class VisualToolBase : protected SelectionListener<AssDialogue> {
	std::deque<agi::signal::Connection> connections;

	void OnCommit(int type);
	void OnSeek(int new_frame);

	void OnMouseCaptureLost(wxMouseCaptureLostEvent &);

	/// @brief Get the dialogue line currently in the edit box
	/// @return NULL if the line is not active on the current frame
	AssDialogue *GetActiveDialogueLine();

	// SubtitleSelectionListener implementation
	void OnActiveLineChanged(AssDialogue *new_line);
	void OnSelectedSetChanged(const Selection &, const Selection &) { }

	// Below here are the virtuals that must be implemented

	/// Called when the script, video or screen resolutions change
	virtual void OnCoordinateSystemsChanged() { DoRefresh(); }

	/// Called when the file is changed by something other than a visual tool
	virtual void OnFileChanged() { DoRefresh(); }

	/// Called when the frame number changes
	virtual void OnFrameChanged() { }

	/// Called when the active line changes
	virtual void OnLineChanged() { DoRefresh(); }

	/// Generic refresh to simplify tools which have no interesting state and
	/// can simply do do the same thing for any external change (i.e. most of
	/// them). Called only by the above virtual methods.
	virtual void DoRefresh() { }

protected:
	OpenGLWrapper gl;

	/// Called when the user double-clicks
	virtual void OnDoubleClick() { }

	static const wxColour colour[4];

	agi::Context *c;
	VideoDisplay *parent;

	bool holding; ///< Is a hold currently in progress?
	AssDialogue *active_line; ///< Active dialogue line; NULL if it is not visible on the current frame
	bool dragging; ///< Is a drag currently in progress?

	int frame_number; ///< Current frame number

	bool left_click; ///< Is a left click event currently being processed?
	bool left_double; ///< Is a left double click event currently being processed?
	bool shift_down; ///< Is shift down?
	bool ctrl_down; ///< Is ctrl down?
	bool alt_down; ///< Is alt down?

	Vector2D mouse_pos; ///< Last seen mouse position
	Vector2D drag_start; ///< Mouse position at the beginning of the last drag
	Vector2D script_res; ///< Script resolution
	Vector2D video_pos; ///< Top-left corner of the video in the display area
	Vector2D video_res; ///< Video resolution

	agi::signal::Connection file_changed_connection;
	int commit_id; ///< Last used commit id for coalescing

	/// @brief Commit the current file state
	/// @param message Description of changes for undo
	void Commit(wxString message = wxString());
	bool IsDisplayed(AssDialogue *line) const;

	/// Get the line's position if it's set, or it's default based on style if not
	Vector2D GetLinePosition(AssDialogue *diag);
	/// Get the line's origin if it's set, or Vector2D::Bad() if not
	Vector2D GetLineOrigin(AssDialogue *diag);
	bool GetLineMove(AssDialogue *diag, Vector2D &p1, Vector2D &p2, int &t1, int &t2);
	void GetLineRotation(AssDialogue *diag, float &rx, float &ry, float &rz);
	void GetLineScale(AssDialogue *diag, Vector2D &scale);
	void GetLineClip(AssDialogue *diag, Vector2D &p1, Vector2D &p2, bool &inverse);
	wxString GetLineVectorClip(AssDialogue *diag, int &scale, bool &inverse);

	void SetOverride(AssDialogue* line, wxString const& tag, wxString const& value);
	void SetSelectedOverride(wxString const& tag, wxString const& value);

	VisualToolBase(VideoDisplay *parent, agi::Context *context);

public:
	/// Convert a point from video to script coordinates
	Vector2D ToScriptCoords(Vector2D point) const;
	/// Convert a point from script to video coordinates
	Vector2D FromScriptCoords(Vector2D point) const;

	// Stuff called by VideoDisplay
	virtual void OnMouseEvent(wxMouseEvent &event)=0;
	virtual void Draw()=0;
	virtual void SetDisplayArea(int x, int y, int w, int h);
	virtual void SetToolbar(wxToolBar *) { }
	virtual ~VisualToolBase();
};

/// @class VisualTool
/// @brief Visual tool base class containing all common feature-related functionality
/// DOCME
template<class FeatureType>
class VisualTool : public VisualToolBase {
protected:
	typedef FeatureType Feature;
	typedef typename std::list<FeatureType>::iterator feature_iterator;
	typedef typename std::list<FeatureType>::const_iterator feature_const_iterator;

private:
	struct ltaddr {
		template<class T>
		bool operator()(T lft, T rgt) const {
			return &*lft < &*rgt;
		}
	};

	std::list<agi::signal::Connection> slots;

	typedef typename std::set<feature_iterator, ltaddr>::iterator selection_iterator;

	bool sel_changed; /// Has the selection already been changed in the current click?

	/// @brief Called when a hold is begun
	/// @return Should the hold actually happen?
	virtual bool InitializeHold() { return false; }
	/// @brief Called on every mouse event during a hold
	virtual void UpdateHold() { }

	/// @brief Called at the beginning of a drag
	/// @param feature The visual feature clicked on
	/// @return Should the drag happen?
	virtual bool InitializeDrag(feature_iterator feature) { return true; }
	/// @brief Called on every mouse event during a drag
	/// @param feature The current feature to process; not necessarily the one clicked on
	virtual void UpdateDrag(feature_iterator feature) { }

	/// @brief Draw stuff
	virtual void Draw()=0;

protected:
	std::set<feature_iterator, ltaddr> sel_features; ///< Currently selected visual features
	typedef typename std::set<feature_iterator, ltaddr>::const_iterator sel_iterator;

	/// Topmost feature under the mouse; generally only valid during a drag
	feature_iterator active_feature;
	/// List of features which are drawn and can be clicked on
	/// List is used here for the iterator invalidation properties
	std::list<FeatureType> features;

	/// Draw all of the features in the list
	void DrawAllFeatures();

	/// @brief Remove a feature from the selection
	/// @param i Index in the feature list
	/// Also deselects lines if all features for that line have been deselected
	void RemoveSelection(feature_iterator feat);

	/// @brief Set the selection to a single feature, deselecting everything else
	/// @param i Index in the feature list
	void SetSelection(feature_iterator feat, bool clear);

public:
	/// @brief Handler for all mouse events
	/// @param event Shockingly enough, the mouse event
	void OnMouseEvent(wxMouseEvent &event);

	/// @brief Constructor
	/// @param parent The VideoDisplay to use for coordinate conversion
	/// @param video Video and mouse information passing blob
	VisualTool(VideoDisplay *parent, agi::Context *context);
};
