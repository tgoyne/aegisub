// Copyright (c) 2005-2010, Rodrigo Braz Monteiro
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

/// @file video_display.h
/// @see video_display.cpp
/// @ingroup video main_ui
///


#ifndef AGI_PRE
#include <list>

#include <wx/glcanvas.h>
#endif

#include <libaegisub/scoped_ptr.h>
#include <libaegisub/signal.h>

// Prototypes
class FrameReadyEvent;
class VideoBox;
class VideoContext;
class VideoOutGL;
class IVisualTool;
class wxComboBox;
class wxTextCtrl;
class wxToolBar;

namespace agi {
	struct Context;
	class OptionValue;
}

struct VideoState {
	int x;
	int y;
	int w;
	int h;
	VideoState() : x(INT_MIN), y(INT_MIN), w(INT_MIN), h(INT_MIN) { }
};

/// @class VideoDisplay
/// @brief DOCME
class VideoDisplay : public wxGLCanvas {
	/// Signals the display is connected to
	std::list<agi::signal::Connection> slots;

	const agi::OptionValue* alwaysShowTools;

	agi::Context *con;

	agi::scoped_ptr<wxMenu> context_menu;

	/// The frame number currently being displayed
	int currentFrame;

	/// The width of the canvas in screen pixels
	int w;
	/// The height of the canvas in screen pixels
	int h;

	/// Screen pixels between the left of the canvas and the left of the video
	int viewport_x;
	/// The width of the video in screen pixels
	int viewport_width;
	/// Screen pixels between the bottom of the canvas and the bottom of the video; used for glViewport
	int viewport_bottom;
	/// Screen pixels between the bottom of the canvas and the top of the video; used for coordinate space conversion
	int viewport_top;
	/// The height of the video in screen pixels
	int viewport_height;

	/// The current zoom level, where 1.0 = 100%
	double zoomValue;

	/// The video renderer
	agi::scoped_ptr<VideoOutGL> videoOut;

	/// The active visual typesetting tool
	agi::scoped_ptr<IVisualTool> tool;
	/// The current tool's ID
	int activeMode;
	/// The toolbar used by individual typesetting tools
	wxToolBar* toolBar;

	/// The OpenGL context for this display
	agi::scoped_ptr<wxGLContext> glContext;

	/// The current script width
	int scriptW;
	/// The current script height
	int scriptH;

	VideoState video;

	/// The dropdown box for selecting zoom levels
	wxComboBox *zoomBox;

	/// The VideoBox this display is contained in
	VideoBox *box;

	/// Whether the display can be freely resized by the user
	bool freeSize;

	/// @brief Draw an overscan mask
	/// @param sizeH  The amount of horizontal overscan on one side
	/// @param sizeV  The amount of vertical overscan on one side
	/// @param colour The color of the mask
	/// @param alpha  The alpha of the mask
	void DrawOverscanMask(int sizeH, int sizeV, wxColor color, double alpha) const;

	/// Upload the image for the current frame to the video card
	void UploadFrameData(FrameReadyEvent&);

	/// @brief Initialize the gl context and set the active context to this one
	/// @return Could the context be set?
	bool InitContext();

	/// @brief Set this video display to the given frame
	/// @frameNumber The desired frame number
	void SetFrame(int frameNumber);

	void OnVideoOpen();
	void OnCommit(int type);

	void SetMode(int mode);
	/// @brief Switch the active tool to a new object of the specified class
	/// @param T The class of the new visual typesetting tool
	template <class T> void SetTool();

	/// @brief Set the cursor to either default or blank
	/// @param show Whether or not the cursor should be visible
	void ShowCursor(bool show);
	/// @brief Set the size of the display based on the current zoom and video resolution
	void UpdateSize(int arType = -1, double arValue = -1.);
	/// @brief Set the zoom level to that indicated by the dropdown
	void SetZoomFromBox(wxCommandEvent&);

	/// @brief Key event handler
	void OnKeyDown(wxKeyEvent &event);
	/// @brief Mouse event handler
	void OnMouseEvent(wxMouseEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	/// @brief Recalculate video positioning and scaling when the available area or zoom changes
	void OnSizeEvent(wxSizeEvent &event);
	void OnMode(const wxCommandEvent &event);
	void OnContextMenu(wxContextMenuEvent&);

public:
	/// @brief Constructor
	VideoDisplay(
		VideoBox *box,
		bool isDetached,
		wxComboBox *zoomBox,
		wxWindow* parent,
		agi::Context *context);
	~VideoDisplay();

	/// @brief Render the currently visible frame
	void Render();

	/// @brief Set the zoom level
	/// @param value The new zoom level
	void SetZoom(double value);
	/// @brief Get the current zoom level
	double GetZoom() const { return zoomValue; }

	/// @brief Convert a point from screen to script coordinate frame
	/// @param x x coordinate; in/out
	/// @param y y coordinate; in/out
	void ToScriptCoords(int *x, int *y) const;
	/// @brief Convert a point from script to screen coordinate frame
	/// @param x x coordinate; in/out
	/// @param y y coordinate; in/out
	void FromScriptCoords(int *x, int *y) const;

	/// Get the last seen position of the mouse in screen coordinates
	/// @param[out] x x coordinate
	/// @param[out] y y coordinate
	void GetMousePosition(int *x, int *y) const;
};
