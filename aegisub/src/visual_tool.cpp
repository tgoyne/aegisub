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

/// @file visual_tool.cpp
/// @brief Base class for visual typesetting functions
/// @ingroup visual_ts

#include "config.h"

#include "visual_tool.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "ass_time.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "utils.h"
#include "video_context.h"
#include "video_display.h"
#include "visual_feature.h"
#include "visual_tool_clip.h"
#include "visual_tool_drag.h"
#include "visual_tool_vector_clip.h"

#include <libaegisub/ass/dialogue_parser.h>
#include <libaegisub/of_type_adaptor.h>

#include <algorithm>
#include <boost/format.hpp>

using std::placeholders::_1;

const wxColour VisualToolBase::colour[4] = {wxColour(106,32,19), wxColour(255,169,40), wxColour(255,253,185), wxColour(187,0,0)};

VisualToolBase::VisualToolBase(VideoDisplay *parent, agi::Context *context)
: c(context)
, parent(parent)
, holding(false)
, active_line(0)
, dragging(false)
, frame_number(c->videoController->GetFrameN())
, shift_down(false)
, ctrl_down(false)
, alt_down(false)
, file_changed_connection(c->ass->AddCommitListener(&VisualToolBase::OnCommit, this))
, commit_id(-1)
{
	int script_w, script_h;
	c->ass->GetResolution(script_w, script_h);
	script_res = Vector2D(script_w, script_h);
	active_line = GetActiveDialogueLine();
	connections.push_back(c->selectionController->AddActiveLineListener(&VisualToolBase::OnActiveLineChanged, this));
	connections.push_back(c->videoController->AddSeekListener(&VisualToolBase::OnSeek, this));
	parent->Bind(wxEVT_MOUSE_CAPTURE_LOST, &VisualToolBase::OnMouseCaptureLost, this);
}

void VisualToolBase::OnCommit(int type) {
	holding = false;
	dragging = false;

	if (type == AssFile::COMMIT_NEW || type & AssFile::COMMIT_SCRIPTINFO) {
		int script_w, script_h;
		c->ass->GetResolution(script_w, script_h);
		script_res = Vector2D(script_w, script_h);
		OnCoordinateSystemsChanged();
	}

	if (type & AssFile::COMMIT_DIAG_FULL || type & AssFile::COMMIT_DIAG_ADDREM) {
		active_line = GetActiveDialogueLine();
		OnFileChanged();
	}
}

void VisualToolBase::OnSeek(int new_frame) {
	if (frame_number == new_frame) return;

	frame_number = new_frame;
	OnFrameChanged();

	AssDialogue *new_line = GetActiveDialogueLine();
	if (new_line != active_line) {
		dragging = false;
		active_line = new_line;
		OnLineChanged();
	}
}

void VisualToolBase::OnMouseCaptureLost(wxMouseCaptureLostEvent &) {
	holding = false;
	dragging = false;
}

void VisualToolBase::OnActiveLineChanged(AssDialogue *new_line) {
	if (!IsDisplayed(new_line))
		new_line = 0;

	holding = false;
	dragging = false;
	if (new_line != active_line) {
		active_line = new_line;
		OnLineChanged();
		parent->Render();
	}
}

bool VisualToolBase::IsDisplayed(AssDialogue *line) const {
	int frame = c->videoController->GetFrameN();
	return
		line &&
		c->videoController->FrameAtTime(line->Start, agi::vfr::START) <= frame &&
		c->videoController->FrameAtTime(line->End, agi::vfr::END) >= frame;
}

void VisualToolBase::Commit(wxString message) {
	file_changed_connection.Block();
	if (message.empty())
		message = _("visual typesetting");

	commit_id = c->ass->Commit(message, AssFile::COMMIT_DIAG_TEXT, commit_id);
	file_changed_connection.Unblock();
}

AssDialogue* VisualToolBase::GetActiveDialogueLine() {
	AssDialogue *diag = c->selectionController->GetActiveLine();
	if (IsDisplayed(diag))
		return diag;
	return 0;
}

void VisualToolBase::SetDisplayArea(int x, int y, int w, int h) {
	if (x == video_pos.X() && y == video_pos.Y() && w == video_res.X() && h == video_res.Y()) return;

	video_pos = Vector2D(x, y);
	video_res = Vector2D(w, h);

	holding = false;
	dragging = false;
	if (parent->HasCapture())
		parent->ReleaseMouse();
	OnCoordinateSystemsChanged();
}

Vector2D VisualToolBase::ToScriptCoords(Vector2D point) const {
	return (point - video_pos) * script_res / video_res;
}

Vector2D VisualToolBase::FromScriptCoords(Vector2D point) const {
	return (point * video_res / script_res) + video_pos;
}

template<class FeatureType>
VisualTool<FeatureType>::VisualTool(VideoDisplay *parent, agi::Context *context)
: VisualToolBase(parent, context)
, sel_changed(false)
{
	active_feature = features.begin();
}

template<class FeatureType>
void VisualTool<FeatureType>::OnMouseEvent(wxMouseEvent &event) {
	bool left_click = event.LeftDown();
	bool left_double = event.LeftDClick();
	shift_down = event.ShiftDown();
	ctrl_down = event.CmdDown();
	alt_down = event.AltDown();

	mouse_pos = event.GetPosition();

	bool need_render = false;

	if (event.Leaving()) {
		mouse_pos = Vector2D();
		parent->Render();
		return;
	}

	if (event.Entering() && OPT_GET("Tool/Visual/Autohide")->GetBool())
		need_render = true;

	if (!dragging) {
		feature_iterator prev_feature = active_feature;

		int max_layer = INT_MIN;
		active_feature = features.end();
		for (feature_iterator cur = features.begin(); cur != features.end(); ++cur) {
			if (cur->IsMouseOver(mouse_pos) && cur->layer >= max_layer) {
				active_feature = cur;
				max_layer = cur->layer;
			}
		}

		need_render |= active_feature != prev_feature;
	}

	if (dragging) {
		// continue drag
		if (event.LeftIsDown()) {
			for (auto sel : sel_features)
				sel->UpdateDrag(mouse_pos - drag_start, shift_down);
			for (auto sel : sel_features)
				UpdateDrag(sel);
			Commit();
			need_render = true;
		}
		// end drag
		else {
			dragging = false;

			// mouse didn't move, fiddle with selection
			if (active_feature != features.end() && !active_feature->HasMoved()) {
				// Don't deselect stuff that was selected in this click's mousedown event
				if (!sel_changed) {
					if (ctrl_down)
						RemoveSelection(active_feature);
					else
						SetSelection(active_feature, true);
				}
			}

			active_feature = features.end();
			parent->ReleaseMouse();
			parent->SetFocus();
		}
	}
	else if (holding) {
		if (!event.LeftIsDown()) {
			holding = false;

			parent->ReleaseMouse();
			parent->SetFocus();
		}

		UpdateHold();
		need_render = true;
		Commit();

	}
	else if (left_click) {
		drag_start = mouse_pos;

		// start drag
		if (active_feature != features.end()) {
			if (!sel_features.count(active_feature)) {
				sel_changed = true;
				SetSelection(active_feature, !ctrl_down);
			}
			else
				sel_changed = false;

			if (active_feature->line)
				c->selectionController->SetActiveLine(active_feature->line);

			if (InitializeDrag(active_feature)) {
				for (auto sel : sel_features) sel->StartDrag();
				dragging = true;
				parent->CaptureMouse();
			}
		}
		// start hold
		else {
			if (!alt_down && features.size() > 1) {
				sel_features.clear();
				SubtitleSelection sel;
				sel.insert(c->selectionController->GetActiveLine());
				c->selectionController->SetSelectedSet(sel);
				need_render = true;
			}
			if (active_line && InitializeHold()) {
				holding = true;
				parent->CaptureMouse();
			}
		}
	}

	if (active_line && left_double)
		OnDoubleClick();

	//if (need_render)
		parent->Render();

	// Only coalesce the changes made in a single drag
	if (!event.LeftIsDown())
		commit_id = -1;
}

template<class FeatureType>
void VisualTool<FeatureType>::DrawAllFeatures() {
	gl.SetLineColour(colour[0], 1.0f, 2);
	for (feature_iterator cur = features.begin(); cur != features.end(); ++cur) {
		int fill = 1;
		if (cur == active_feature)
			fill = 2;
		else if (sel_features.count(cur))
			fill = 3;
		gl.SetFillColour(colour[fill], 0.6f);
		cur->Draw(gl);
	}
}

template<class FeatureType>
void VisualTool<FeatureType>::SetSelection(feature_iterator feat, bool clear) {
	if (clear)
		sel_features.clear();

	if (sel_features.insert(feat).second && feat->line) {
		SubtitleSelection sel;
		if (!clear)
			sel = c->selectionController->GetSelectedSet();
		if (sel.insert(feat->line).second)
			c->selectionController->SetSelectedSet(sel);
	}
}

template<class FeatureType>
void VisualTool<FeatureType>::RemoveSelection(feature_iterator feat) {
	if (!sel_features.erase(feat) || !feat->line) return;

	for (auto sel : sel_features) {
		if (sel->line == feat->line) return;
	}

	SubtitleSelection sel = c->selectionController->GetSelectedSet();

	// Don't deselect the only selected line
	if (sel.size() <= 1) return;

	sel.erase(feat->line);

	// Set the active line to an arbitrary selected line if we just
	// deselected the active line
	AssDialogue *new_active = c->selectionController->GetActiveLine();
	if (feat->line == new_active)
		new_active = *sel.begin();

	c->selectionController->SetSelectionAndActive(sel, new_active);
}

//////// PARSERS

namespace {
using agi::ass::Get;

// Find a tag's parameters in a line or return nullptr if it's not found
agi::ass::OverrideTag* find_tag(std::vector<agi::ass::DialogueBlock>& blocks, std::string const& tag_name) {
	agi::ass::OverrideTag *ret = nullptr;
	VisitTags(blocks, [&](agi::ass::OverrideTag& tag, bool *stop) {
		if (tag.name == tag_name) {
			ret = &tag;
			*stop = true;
		}
	});

	return ret;
}

// Get a Vector2D from the given tag parameters, or Vector2D::Bad() if they are not valid
Vector2D vec_or_bad(agi::ass::OverrideTag* tag, size_t x_idx, size_t y_idx) {
	if (!tag || tag->params.size() <= std::max(x_idx, y_idx))
		return Vector2D();
	return Vector2D(Get<float>(*tag, x_idx), Get<float>(*tag, y_idx));
}
}

Vector2D VisualToolBase::GetLinePosition(AssDialogue *diag) {
	auto blocks = agi::ass::Parse(diag->Text);

	if (Vector2D ret = vec_or_bad(find_tag(blocks, "pos"), 0, 1)) return ret;
	if (Vector2D ret = vec_or_bad(find_tag(blocks, "move"), 0, 1)) return ret;

	// Get default position
	int margin[3];
	memcpy(margin, diag->Margin, sizeof margin);
	int align = 2;

	if (AssStyle *style = c->ass->GetStyle(diag->Style)) {
		align = style->alignment;
		for (int i = 0; i < 3; i++) {
			if (margin[i] == 0)
				margin[i] = style->Margin[i];
		}
	}

	int ovr_align = 0;
	if (auto align_tag = find_tag(blocks, "an"))
		ovr_align = Get(*align_tag, 0, ovr_align);
	else if (auto align_tag = find_tag(blocks, "a"))
		ovr_align = AssStyle::SsaToAss(Get(*align_tag, 0, 2));

	if (ovr_align > 0 && ovr_align <= 9)
		align = ovr_align;

	// Alignment type
	int hor = (align - 1) % 3;
	int vert = (align - 1) / 3;

	// Calculate positions
	int x, y;
	if (hor == 0)
		x = margin[0];
	else if (hor == 1)
		x = (script_res.X() + margin[0] - margin[1]) / 2;
	else
		x = script_res.X() - margin[1];

	if (vert == 0)
		y = script_res.Y() - margin[2];
	else if (vert == 1)
		y = script_res.Y() / 2;
	else
		y = margin[2];

	return Vector2D(x, y);
}

Vector2D VisualToolBase::GetLineOrigin(AssDialogue *diag) {
	return vec_or_bad(find_tag(agi::ass::Parse(diag->Text), "org"), 0, 1);
}

bool VisualToolBase::GetLineMove(AssDialogue *diag, Vector2D &p1, Vector2D &p2, int &t1, int &t2) {
	auto blocks = agi::ass::Parse(diag->Text);

	auto tag = find_tag(blocks, "move");
	if (!tag)
		return false;

	p1 = vec_or_bad(tag, 0, 1);
	p2 = vec_or_bad(tag, 2, 3);
	// VSFilter actually defaults to -1, but it uses <= 0 to check for default and 0 seems less bug-prone
	t1 = Get(*tag, 4, 0);
	t2 = Get(*tag, 5, 0);

	return p1 && p2;
}

void VisualToolBase::GetLineRotation(AssDialogue *diag, float &rx, float &ry, float &rz) {
	rx = ry = rz = 0.f;

	if (AssStyle *style = c->ass->GetStyle(diag->Style))
		rz = style->angle;

	auto blocks = agi::ass::Parse(diag->Text);

	if (auto tag = find_tag(blocks, "frx"))
		rx = Get(*tag, 0, rx);
	if (auto tag = find_tag(blocks, "fry"))
		ry = Get(*tag, 0, ry);
	if (auto tag = find_tag(blocks, "frz"))
		rz = Get(*tag, 0, rz);
	else if (auto tag = find_tag(blocks, "fr"))
		rz = Get(*tag, 0, rz);
}

void VisualToolBase::GetLineScale(AssDialogue *diag, Vector2D &scale) {
	float x = 100.f, y = 100.f;

	if (AssStyle *style = c->ass->GetStyle(diag->Style)) {
		x = style->scalex;
		y = style->scaley;
	}

	auto blocks = agi::ass::Parse(diag->Text);

	if (auto tag = find_tag(blocks, "fscx"))
		x = Get(*tag, 0, x);
	if (auto tag = find_tag(blocks, "fscy"))
		y = Get(*tag, 0, y);

	scale = Vector2D(x, y);
}

void VisualToolBase::GetLineClip(AssDialogue *diag, Vector2D &p1, Vector2D &p2, bool &inverse) {
	inverse = false;

	auto blocks = agi::ass::Parse(diag->Text);
	auto tag = find_tag(blocks, "iclip");
	if (tag)
		inverse = true;
	else
		tag = find_tag(blocks, "clip");

	if (tag && tag->params.size() == 4) {
		p1 = vec_or_bad(tag, 0, 1);
		p2 = vec_or_bad(tag, 2, 3);
	}
	else {
		p1 = Vector2D(0, 0);
		p2 = script_res - 1;
	}
}

std::string VisualToolBase::GetLineVectorClip(AssDialogue *diag, int &scale, bool &inverse) {
	auto blocks = agi::ass::Parse(diag->Text);

	scale = 1;
	inverse = false;

	auto tag = find_tag(blocks, "iclip");
	if (tag)
		inverse = true;
	else
		tag = find_tag(blocks, "clip");

	if (!tag || tag->params.empty()) return "";

	if (tag->params.size() == 4) {
		return str(boost::format("m %d %d l %d %d %d %d %d %d")
			% Get<int>(*tag, 0) % Get<int>(*tag, 1)
			% Get<int>(*tag, 2) % Get<int>(*tag, 1)
			% Get<int>(*tag, 2) % Get<int>(*tag, 3)
			% Get<int>(*tag, 0) % Get<int>(*tag, 3));
	}

	if (tag->params.size() == 2) {
		scale = std::max(Get(*tag, 0, scale), 1);
		return Get<std::string>(*tag, 1);
	}

	return Get<std::string>(*tag, 0);
}

void VisualToolBase::SetSelectedOverride(std::string const& tag, std::string const& value) {
	for (auto line : c->selectionController->GetSelectedSet())
		SetOverride(line, tag, value);
}

void VisualToolBase::SetOverride(AssDialogue* line, std::string const& tag, std::string const& value) {
	if (!line) return;

	line->Text = agi::ass::SetTag(line->Text, 0, tag, value);
}

// If only export worked
template class VisualTool<VisualDraggableFeature>;
template class VisualTool<ClipCorner>;
template class VisualTool<VisualToolDragDraggableFeature>;
template class VisualTool<VisualToolVectorClipDraggableFeature>;
