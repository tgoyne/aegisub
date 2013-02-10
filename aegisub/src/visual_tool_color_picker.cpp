// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "config.h"

#include "visual_tool_color_picker.h"

#include "command/command.h"
#include "main.h"

VisualToolColorPicker::VisualToolColorPicker(VideoDisplay *parent, agi::Context *context)
: VisualTool<VisualDraggableFeature>(parent, context)
, mode(1)
{
}

void VisualToolColorPicker::SetToolbar(wxToolBar *toolbar) {
	int icon_size = OPT_GET("App/Toolbar Icon Size")->GetInt();

	std::string colors[] = { "primary", "secondary", "outline", "shadow" };

	for (int i = 0; i < 4; ++i) {
		cmd::Command *c = cmd::get("edit/color/" + colors[i]);
		int id = toolbar->AddTool(-1, c->StrDisplay(nullptr), c->Icon(icon_size), c->StrHelp(), wxITEM_RADIO)->GetId();
		toolbar->Bind(wxEVT_COMMAND_TOOL_CLICKED, [=](wxCommandEvent&) { mode = i + 1; }, id);
		if (i == 0) toolbar->ToggleTool(id, true);
	}
	toolbar->Realize();
	toolbar->Show(true);
}

bool VisualToolColorPicker::InitializeHold() { return true; }

void VisualToolColorPicker::UpdateHold() {
	SetSelectedOverride("\\c", "&H000000&");
}

void VisualToolColorPicker::Draw() { }
