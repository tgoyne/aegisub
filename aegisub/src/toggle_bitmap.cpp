// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file toggle_bitmap.cpp
/// @brief Toggle-button rendered as a bitmap on a coloured background
/// @ingroup custom_control
///

#include "config.h"

#ifndef AGI_PRE
#include <wx/dcclient.h>
#include <wx/settings.h>
#include <wx/tglbtn.h>
#endif

#include "toggle_bitmap.h"

#include "command/command.h"
#include "include/aegisub/context.h"
#include "tooltip_manager.h"

ToggleBitmap::ToggleBitmap(wxWindow *parent, agi::Context *context, const char *cmd_name, int icon_size, const char *ht_ctx, wxSize const& size)
: wxControl(parent, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER)
, context(context)
, command(cmd::get(cmd_name))
, img(command->Icon(icon_size))
{
	int w = size.GetWidth() != -1 ? size.GetWidth() : img.GetWidth();
	int h = size.GetHeight() != -1 ? size.GetHeight() : img.GetHeight();
	SetClientSize(w, h);
	GetSize(&w, &h);
	SetSizeHints(w, h, w, h);

	ToolTipManager::Bind(this, command->StrHelp(), ht_ctx, cmd_name);
	Bind(wxEVT_PAINT, &ToggleBitmap::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &ToggleBitmap::OnMouseEvent, this);
}

void ToggleBitmap::OnMouseEvent(wxMouseEvent &) {
	(*command)(context);
	Refresh(false);
}

void ToggleBitmap::OnPaint(wxPaintEvent &) {
	wxPaintDC dc(this);
	int w,h;
	GetClientSize(&w,&h);

	// Get background color
	wxColour bgColor = command->IsActive(context) ? wxColour(0,255,0) : wxColour(255,0,0);
	wxColor sysCol = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);
	bgColor.Set(
		(sysCol.Red() + bgColor.Red()) / 2,
		(sysCol.Green() + bgColor.Green()) / 2,
		(sysCol.Blue() + bgColor.Blue()) / 2);

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(bgColor));
	dc.DrawRectangle(0, 0, w, h);

	dc.DrawBitmap(img, (w - img.GetWidth()) / 2, (h - img.GetHeight()) / 2, true);
}
