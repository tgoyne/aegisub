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

/// @file video_display.cpp
/// @brief Control displaying a video frame obtained from the video context
/// @ingroup video main_ui
///

// Includes
#include "config.h"

#ifndef AGI_PRE
#include <algorithm>

#include <wx/combobox.h>
#include <wx/dataobj.h>
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "include/aegisub/context.h"
#include "include/aegisub/hotkey.h"
#include "include/aegisub/menu.h"

#include "video_display.h"

#include "ass_file.h"
#include "main.h"
#include "threaded_frame_source.h"
#include "video_out_gl.h"
#include "video_box.h"
#include "video_context.h"
#include "visual_tool.h"
#include "visual_tool_clip.h"
#include "visual_tool_cross.h"
#include "visual_tool_drag.h"
#include "visual_tool_rotatexy.h"
#include "visual_tool_rotatez.h"
#include "visual_tool_scale.h"
#include "visual_tool_vector_clip.h"

/// Attribute list for gl canvases; set the canvases to doublebuffered rgba with an 8 bit stencil buffer
int attribList[] = { WX_GL_RGBA , WX_GL_DOUBLEBUFFER, WX_GL_STENCIL_SIZE, 8, 0 };

/// @class VideoOutRenderException
/// @extends VideoOutException
/// @brief An OpenGL error occurred while uploading or displaying a frame
class OpenGlException : public agi::Exception {
public:
	OpenGlException(const wxChar *func, int err)
		: agi::Exception(STD_STR(wxString::Format("%s failed with error code %d", func, err)))
	{ }
	const char * GetName() const { return "video/opengl"; }
	Exception * Copy() const { return new OpenGlException(*this); }
};

#define E(cmd) cmd; if (GLenum err = glGetError()) throw OpenGlException(L###cmd, err)

VideoDisplay::VideoDisplay(
	VideoBox *box,
	bool freeSize,
	wxComboBox *zoomBox,
	wxWindow* parent,
	agi::Context *c)
: wxGLCanvas (parent, -1, attribList, wxDefaultPosition, wxDefaultSize, 0, wxPanelNameStr)
, alwaysShowTools(OPT_GET("Tool/Visual/Always Show"))
, con(c)
, currentFrame(-1)
, w(8), h(8), viewport_x(0), viewport_width(0), viewport_bottom(0), viewport_top(0), viewport_height(0)
, zoomValue(OPT_GET("Video/Default Zoom")->GetInt() * .125 + .125)
, videoOut(new VideoOutGL())
, activeMode(Video_Mode_Standard)
, toolBar(box->visualSubToolBar)
, scriptW(INT_MIN)
, scriptH(INT_MIN)
, zoomBox(zoomBox)
, box(box)
, freeSize(freeSize)
{
	assert(box);

	zoomBox->SetValue(wxString::Format("%g%%", zoomValue * 100.));
	zoomBox->Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, &VideoDisplay::SetZoomFromBox, this);
	box->Bind(wxEVT_COMMAND_TOOL_CLICKED, &VideoDisplay::OnMode, this, Video_Mode_Standard, Video_Mode_Vector_Clip);

	con->videoController->Bind(EVT_FRAME_READY, &VideoDisplay::UploadFrameData, this);
	slots.push_back(con->videoController->AddSeekListener(&VideoDisplay::SetFrame, this));
	slots.push_back(con->videoController->AddVideoOpenListener(&VideoDisplay::OnVideoOpen, this));
	slots.push_back(con->videoController->AddARChangeListener(&VideoDisplay::UpdateSize, this));
	slots.push_back(con->ass->AddCommitListener(&VideoDisplay::OnCommit, this));

	Bind(wxEVT_PAINT, std::tr1::bind(&VideoDisplay::Render, this));
	if (freeSize) {
		Bind(wxEVT_SIZE, &VideoDisplay::OnSizeEvent, this);
	}
	Bind(wxEVT_CONTEXT_MENU, &VideoDisplay::OnContextMenu, this);
	Bind(wxEVT_KEY_DOWN, &VideoDisplay::OnKeyDown, this);
	Bind(wxEVT_LEFT_DOWN, &VideoDisplay::OnMouseEvent, this);
	Bind(wxEVT_LEFT_UP, &VideoDisplay::OnMouseEvent, this);
	Bind(wxEVT_MOTION, &VideoDisplay::OnMouseEvent, this);
	Bind(wxEVT_ENTER_WINDOW, &VideoDisplay::OnMouseEnter, this);
	Bind(wxEVT_LEAVE_WINDOW, &VideoDisplay::OnMouseLeave, this);
	Bind(wxEVT_MOUSEWHEEL, &VideoDisplay::OnMouseWheel, this);

	SetCursor(wxNullCursor);

	if (con->videoController->IsLoaded()) {
		con->videoController->GetScriptSize(scriptW, scriptH);
		OnVideoOpen();
	}
}

VideoDisplay::~VideoDisplay () {
	con->videoController->Unbind(EVT_FRAME_READY, &VideoDisplay::UploadFrameData, this);
}

bool VideoDisplay::InitContext() {
	if (!IsShownOnScreen()) return false;
	if (!glContext.get()) {
		glContext.reset(new wxGLContext(this));
	}
	SetCurrent(*glContext.get());
	return true;
}

void VideoDisplay::ShowCursor(bool show) {
	if (show) {
		SetCursor(wxNullCursor);
	}
	else {
		SetCursor(wxCursor(wxCURSOR_BLANK));
	}
}

void VideoDisplay::SetFrame(int frameNumber) {
	currentFrame = frameNumber;

	// Render the new frame
	if (con->videoController->IsLoaded()) {
		tool->SetFrame(frameNumber);
		con->videoController->GetFrameAsync(currentFrame);
	}
}

void VideoDisplay::UploadFrameData(FrameReadyEvent &evt) {
	if (!InitContext()) return;

	try {
		videoOut->UploadFrameData(*evt.frame);
	}
	catch (const VideoOutInitException& err) {
		wxLogError(
			L"Failed to initialize video display. Closing other running "
			L"programs and updating your video card drivers may fix this.\n"
			L"Error message reported: %s",
			err.GetMessage().c_str());
		con->videoController->Reset();
	}
	catch (const VideoOutRenderException& err) {
		wxLogError(
			L"Could not upload video frame to graphics card.\n"
			L"Error message reported: %s",
			err.GetMessage().c_str());
	}
	Render();
}

void VideoDisplay::OnVideoOpen() {
	if (!con->videoController->IsLoaded()) return;
	UpdateSize();
	if (!tool.get()) tool.reset(new VisualToolCross(this, con, video, toolBar));
	SetFrame(0);
	tool->Refresh();
}

void VideoDisplay::OnCommit(int type) {
	if (type == AssFile::COMMIT_FULL || type == AssFile::COMMIT_UNDO)
		con->videoController->GetScriptSize(scriptW, scriptH);
	if (tool.get()) tool->Refresh();
}

void VideoDisplay::Render() try {
	if (!InitContext()) return;
	if (!con->videoController->IsLoaded()) return;
	assert(wxIsMainThread());
	if (!viewport_height || !viewport_width) UpdateSize();

	videoOut->Render(viewport_x, viewport_bottom, viewport_width, viewport_height);
	E(glViewport(0, std::min(viewport_bottom, 0), w, h));

	E(glMatrixMode(GL_PROJECTION));
	E(glLoadIdentity());
	E(glOrtho(0.0f, w, h, 0.0f, -1000.0f, 1000.0f));

	if (OPT_GET("Video/Overscan Mask")->GetBool()) {
		double ar = con->videoController->GetAspectRatioValue();

		// Based on BBC's guidelines: http://www.bbc.co.uk/guidelines/dq/pdf/tv/tv_standards_london.pdf
		// 16:9 or wider
		if (ar > 1.75) {
			DrawOverscanMask(w * 0.1, h * 0.05, wxColor(30,70,200),0.5);
			DrawOverscanMask(w * 0.035, h * 0.035, wxColor(30,70,200),0.5);
		}

		// Less wide than 16:9 (use 4:3 standard)
		else {
			DrawOverscanMask(w * 0.067, h * 0.05, wxColor(30,70,200),0.5);
			DrawOverscanMask(w * 0.033, h * 0.035, wxColor(30,70,200),0.5);
		}
	}

	if (video.x > INT_MIN || video.y > INT_MIN || alwaysShowTools->GetBool()) {
		tool->Draw();
	}

	SwapBuffers();
}
catch (const VideoOutException &err) {
	wxLogError(
		L"An error occurred trying to render the video frame on the screen.\n"
		L"Error message reported: %s",
		err.GetMessage().c_str());
	con->videoController->Reset();
}
catch (const OpenGlException &err) {
	wxLogError(
		L"An error occurred trying to render visual overlays on the screen.\n"
		L"Error message reported: %s",
		err.GetMessage().c_str());
	con->videoController->Reset();
}
catch (const wchar_t *err) {
	wxLogError(
		L"An error occurred trying to render the video frame on the screen.\n"
		L"Error message reported: %s",
		err);
	con->videoController->Reset();
}
catch (...) {
	wxLogError(
		L"An error occurred trying to render the video frame to screen.\n"
		L"No further error message given.");
	con->videoController->Reset();
}

void VideoDisplay::DrawOverscanMask(int sizeH, int sizeV, wxColor color, double alpha) const {
	int rad1 = h * 0.05;
	int gapH = sizeH+rad1;
	int gapV = sizeV+rad1;
	int rad2 = sqrt(double(gapH*gapH + gapV*gapV)) + 1;

	OpenGLWrapper gl;
	E(gl.SetFillColour(color, alpha));
	gl.SetLineColour(wxColor(0, 0, 0), 0.0, 1);

	// Draw sides
	E(gl.DrawRectangle(gapH, 0, w-gapH, sizeV));   // Top
	E(gl.DrawRectangle(w-sizeH, gapV, w, h-gapV)); // Right
	E(gl.DrawRectangle(gapH, h-sizeV, w-gapH, h)); // Bottom
	E(gl.DrawRectangle(0, gapV, sizeH, h-gapV));   // Left

	// Draw rounded corners
	E(gl.DrawRing(gapH, gapV, rad1, rad2, 1.0, 180.0, 270.0));  // Top-left
	E(gl.DrawRing(w-gapH, gapV, rad1, rad2, 1.0, 90.0, 180.0)); // Top-right
	E(gl.DrawRing(w-gapH, h-gapV, rad1, rad2, 1.0, 0.0, 90.0)); // Bottom-right
	E(gl.DrawRing(gapH, h-gapV, rad1, rad2, 1.0,270.0,360.0));  // Bottom-left

	E(glDisable(GL_BLEND));
}

void VideoDisplay::UpdateSize(int arType, double arValue) {
	if (!con->videoController->IsLoaded()) return;
	if (!IsShownOnScreen()) return;

	int vidW = con->videoController->GetWidth();
	int vidH = con->videoController->GetHeight();

	if (arType == -1) {
		arType = con->videoController->GetAspectRatioType();
		arValue = con->videoController->GetAspectRatioValue();
	}

	if (freeSize) {
		GetClientSize(&w,&h);
		viewport_x = 0;
		viewport_bottom = 0;
		viewport_top = 0;
		viewport_width = w;
		viewport_height = h;

		// Set aspect ratio
		double displayAr = double(w) / h;
		double videoAr = arType == 0 ? double(vidW)/vidH : arValue;

		// Window is wider than video, blackbox left/right
		if (displayAr - videoAr > 0.01f) {
			int delta = w - videoAr * h;
			viewport_x = delta / 2;
			viewport_width = w - delta;
		}

		// Video is wider than window, blackbox top/bottom
		else if (videoAr - displayAr > 0.01f) {
			int delta = h - w / videoAr;
			viewport_top = viewport_bottom = delta / 2;
			viewport_height = h - delta;
		}
	}
	else {
		wxWindow* parent = GetParent();
		while (!parent->IsTopLevel()) parent = parent->GetParent();
		int maxH, maxW;
		parent->GetClientSize(&maxW, &maxH);

		h = vidH * zoomValue;
		w = arType == 0 ? vidW * zoomValue : vidH * zoomValue * arValue;

		// Cap the canvas size to the window size
		int cw = std::min(w, maxW), ch = std::min(h, maxH);

		viewport_x = 0;
		viewport_bottom = ch - h;
		viewport_top = 0;
		viewport_width = w;
		viewport_height = h;

		wxSize size(cw, ch);
		SetMinClientSize(size);
		SetMaxClientSize(size);

		SetEvtHandlerEnabled(false);
		box->GetParent()->Layout();

		// The sizer makes us use the full width, which at very low zoom levels
		// results in stretched video, so after using the sizer to update the 
		// parent window sizes, reset our size to the correct value
		SetSize(cw, ch);

		SetEvtHandlerEnabled(true);
	}

	con->videoController->GetScriptSize(scriptW, scriptH);
	video.w = w;
	video.h = h;

	if (tool.get()) tool->Refresh();

	Refresh(false);
}

void VideoDisplay::OnSizeEvent(wxSizeEvent &event) {
	UpdateSize();
	event.Skip();
}

void VideoDisplay::OnMouseEvent(wxMouseEvent& event) {
	assert(w > 0);

	// Disable when playing
	if (con->videoController->IsPlaying()) return;

	if (event.ButtonDown())
		SetFocus();

	video.x = event.GetX();
	video.y = event.GetY();

	tool->OnMouseEvent(event);
}

void VideoDisplay::OnMouseEnter(wxMouseEvent& event) {
	ShowCursor(activeMode != Video_Mode_Standard);
	tool->OnMouseEvent(event);
}

void VideoDisplay::OnMouseLeave(wxMouseEvent& event) {
	video.x = INT_MIN;
	video.y = INT_MIN;
	tool->OnMouseEvent(event);
}

void VideoDisplay::OnMouseWheel(wxMouseEvent& event) {
	if (int wheel = event.GetWheelRotation())
		SetZoom (zoomValue + .125 * (wheel / event.GetWheelDelta()));
}

void VideoDisplay::OnContextMenu(wxContextMenuEvent&) {
	if (!context_menu.get()) context_menu.reset(menu::GetMenu("video_context", con));
	ShowCursor(true);
	menu::OpenPopupMenu(context_menu.get(), this);
}

void VideoDisplay::OnKeyDown(wxKeyEvent &event) {
	/// @todo
	int kc = event.GetKeyCode();
	if      (kc == 'A') SetMode(Video_Mode_Standard);
	else if (kc == 'S') SetMode(Video_Mode_Drag);
	else if (kc == 'D') SetMode(Video_Mode_Rotate_Z);
	else if (kc == 'F') SetMode(Video_Mode_Rotate_XY);
	else if (kc == 'G') SetMode(Video_Mode_Scale);
	else if (kc == 'H') SetMode(Video_Mode_Clip);
	else if (kc == 'J') SetMode(Video_Mode_Vector_Clip);
	else {
		event.StopPropagation();
		if (hotkey::check("Video Display", con, event.GetKeyCode(), event.GetUnicodeKey(), event.GetModifiers()))
			return;
	}
}


void VideoDisplay::SetZoom(double value) {
	zoomValue = std::max(value, .125);
	zoomBox->SetValue(wxString::Format("%g%%", zoomValue * 100.));
	UpdateSize();
}
void VideoDisplay::SetZoomFromBox(wxCommandEvent &) {
	wxString strValue = zoomBox->GetValue();
	strValue.EndsWith(L"%", &strValue);
	double value;
	if (strValue.ToDouble(&value)) {
		zoomValue = value / 100.;
		UpdateSize();
	}
}

template<class T>
void VideoDisplay::SetTool() {
	tool.reset();
	tool.reset(new T(this, con, video, toolBar));
	box->Bind(wxEVT_COMMAND_TOOL_CLICKED, &T::OnSubTool, static_cast<T*>(tool.get()), VISUAL_SUB_TOOL_START, VISUAL_SUB_TOOL_END);
}
void VideoDisplay::OnMode(const wxCommandEvent &event) {
	SetMode(event.GetId());
}
void VideoDisplay::SetMode(int mode) {
	if (activeMode == mode) return;

	toolBar->ClearTools();
	toolBar->Realize();
	toolBar->Show(false);

	if (!box->visualToolBar->GetToolState(mode)) {
		box->visualToolBar->ToggleTool(mode, true);
	}

	activeMode = mode;
	switch (mode) {
		case Video_Mode_Standard:    SetTool<VisualToolCross>();      break;
		case Video_Mode_Drag:        SetTool<VisualToolDrag>();       break;
		case Video_Mode_Rotate_Z:    SetTool<VisualToolRotateZ>();    break;
		case Video_Mode_Rotate_XY:   SetTool<VisualToolRotateXY>();   break;
		case Video_Mode_Scale:       SetTool<VisualToolScale>();      break;
		case Video_Mode_Clip:        SetTool<VisualToolClip>();       break;
		case Video_Mode_Vector_Clip: SetTool<VisualToolVectorClip>(); break;
		default: assert(false); break;
	}

	// Update size as the new typesetting tool may have changed the subtoolbar size
	UpdateSize();
	ShowCursor(activeMode != Video_Mode_Standard);
}

void VideoDisplay::ToScriptCoords(int *x, int *y) const {
	int sx = *x - viewport_x > 0 ? viewport_width : -viewport_width;
	int sy = *y - viewport_top > 0 ? viewport_height : -viewport_height;
	*x = ((*x - viewport_x) * scriptW + sx / 2) / viewport_width;
	*y = ((*y - viewport_top) * scriptH + sy / 2) / viewport_height;
}
void VideoDisplay::FromScriptCoords(int *x, int *y) const {
	int sx = *x > 0 ? scriptW : -scriptW;
	int sy = *y > 0 ? scriptH : -scriptH;
	*x = (*x * viewport_width + sx / 2) / scriptW + viewport_x;
	*y = (*y * viewport_height + sy / 2) / scriptH + viewport_top;
}

void VideoDisplay::GetMousePosition(int *x, int *y) const {
	*x = video.x;
	*y = video.y;
}
