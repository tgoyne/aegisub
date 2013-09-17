// Copyright (c) 2006, Rodrigo Braz Monteiro
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

/// @file base_grid.cpp
/// @brief Base for subtitle grid in main UI
/// @ingroup main_ui
///

#include "config.h"

#include "base_grid.h"

#include "include/aegisub/context.h"
#include "include/aegisub/hotkey.h"
#include "include/aegisub/menu.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "audio_box.h"
#include "compat.h"
#include "frame_main.h"
#include "options.h"
#include "utils.h"
#include "subs_controller.h"
#include "video_context.h"
#include "video_slider.h"

#include <libaegisub/dispatch.h>
#include <libaegisub/of_type_adaptor.h>
#include <libaegisub/util.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <unordered_map>

#include <wx/dcbuffer.h>
#include <wx/kbdstate.h>
#include <wx/menu.h>
#include <wx/scrolbar.h>
#include <wx/sizer.h>

enum {
	GRID_SCROLLBAR = 1730,
	MENU_SHOW_COL = 1250 // Needs 15 IDs after this
};

enum RowColor {
	COLOR_DEFAULT = 0,
	COLOR_HEADER,
	COLOR_SELECTION,
	COLOR_COMMENT,
	COLOR_VISIBLE,
	COLOR_SELECTED_COMMENT,
	COLOR_LEFT_COL
};

template<class S1, class S2, class D>
static inline void set_difference(const S1 &src1, const S2 &src2, D &dst) {
	std::set_difference(
		src1.begin(), src1.end(), src2.begin(), src2.end(),
		std::inserter(dst, dst.begin()));
}

namespace std {
	template <typename T>
	struct hash<boost::flyweight<T>> {
		size_t operator()(boost::flyweight<T> const& ss) const {
			return hash<const void*>()(&ss.get());
		}
	};
}

BaseGrid::BaseGrid(wxWindow* parent, agi::Context *context, const wxSize& size, long style, const wxString& name)
: wxWindow(parent, -1, wxDefaultPosition, size, style, name)
, lineHeight(1) // non-zero to avoid div by 0
, holding(false)
, scrollBar(new wxScrollBar(this, GRID_SCROLLBAR, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL))
, byFrame(false)
, extendRow(-1)
, active_line(nullptr)
, batch_level(0)
, batch_active_line_changed(false)
, seek_listener(context->videoController->AddSeekListener(std::bind(&BaseGrid::Render, this)))
, rendering(false)
, wants_render(false)
, yPos(0)
, context(context)
{
	scrollBar->SetScrollbar(0,10,100,10);

	wxBoxSizer *scrollbarpositioner = new wxBoxSizer(wxHORIZONTAL);
	scrollbarpositioner->AddStretchSpacer();
	scrollbarpositioner->Add(scrollBar, 0, wxEXPAND, 0);

	SetSizerAndFit(scrollbarpositioner);

	SetBackgroundStyle(wxBG_STYLE_PAINT);

	UpdateStyle();
	OnHighlightVisibleChange(*OPT_GET("Subtitle/Grid/Highlight Subtitles in Frame"));

	OPT_SUB("Subtitle/Grid/Font Face", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Subtitle/Grid/Font Size", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Subtitle/Grid/Highlight Subtitles in Frame", &BaseGrid::OnHighlightVisibleChange, this);
	context->ass->AddCommitListener(&BaseGrid::OnSubtitlesCommit, this);
	context->subsController->AddFileOpenListener(&BaseGrid::OnSubtitlesOpen, this);
	context->subsController->AddFileSaveListener(&BaseGrid::OnSubtitlesSave, this);

	OPT_SUB("Colour/Subtitle Grid/Active Border", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Background/Background", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Background/Comment", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Background/Inframe", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Background/Selected Comment", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Background/Selection", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Collision", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Header", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Left Column", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Lines", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Selection", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Colour/Subtitle Grid/Standard", &BaseGrid::UpdateStyle, this);
	OPT_SUB("Subtitle/Grid/Hide Overrides", std::bind(&BaseGrid::Render, this));

	Bind(wxEVT_CONTEXT_MENU, &BaseGrid::OnContextMenu, this);
}

BaseGrid::~BaseGrid() {
	ClearMaps();
}

BEGIN_EVENT_TABLE(BaseGrid,wxWindow)
	EVT_PAINT(BaseGrid::OnPaint)
	EVT_SIZE(BaseGrid::OnSize)
	EVT_COMMAND_SCROLL(GRID_SCROLLBAR,BaseGrid::OnScroll)
	EVT_MOUSE_EVENTS(BaseGrid::OnMouseEvent)
	EVT_KEY_DOWN(BaseGrid::OnKeyDown)
	EVT_CHAR_HOOK(BaseGrid::OnCharHook)
	EVT_MENU_RANGE(MENU_SHOW_COL,MENU_SHOW_COL+15,BaseGrid::OnShowColMenu)
END_EVENT_TABLE()

void BaseGrid::OnSubtitlesCommit(int type) {
	if (type == AssFile::COMMIT_NEW)
		UpdateMaps(true);
	else if (type & AssFile::COMMIT_ORDER || type & AssFile::COMMIT_DIAG_ADDREM)
		UpdateMaps(false);
	else if (type & AssFile::COMMIT_DIAG_META) {
		SetColumnWidths();
		Render();
	}
	else if (type & AssFile::COMMIT_DIAG_TIME || type & AssFile::COMMIT_DIAG_TEXT)
		Render();
}

void BaseGrid::OnSubtitlesOpen() {
	BeginBatch();
	ClearMaps();
	UpdateMaps();

	if (GetRows()) {
		int row = context->ass->GetScriptInfoAsInt("Active Line");
		if (row < 0 || row >= GetRows())
			row = 0;

		SetActiveLine(GetDialogue(row));
		SelectRow(row);
	}

	ScrollTo(context->ass->GetScriptInfoAsInt("Scroll Position"));

	EndBatch();
	SetColumnWidths();
}

void BaseGrid::OnSubtitlesSave() {
	context->ass->SetScriptInfo("Scroll Position", std::to_string(yPos));
	context->ass->SetScriptInfo("Active Line", std::to_string(GetDialogueIndex(active_line)));
}

void BaseGrid::OnShowColMenu(wxCommandEvent &event) {
	int item = event.GetId() - MENU_SHOW_COL;
	showCol[item] = !showCol[item];

	std::vector<bool> map(std::begin(showCol), std::end(showCol));
	OPT_SET("Subtitle/Grid/Column")->SetListBool(map);

	SetColumnWidths();
	Render();
}

void BaseGrid::OnHighlightVisibleChange(agi::OptionValue const& opt) {
	if (opt.GetBool())
		seek_listener.Unblock();
	else
		seek_listener.Block();
}

void BaseGrid::UpdateStyle() {
	wxString fontname = to_wx(OPT_GET("Subtitle/Grid/Font Face")->GetString());
	if (fontname.empty()) fontname = "Tahoma";
	font.SetFaceName(fontname);
	font.SetPointSize(OPT_GET("Subtitle/Grid/Font Size")->GetInt());
	font.SetWeight(wxFONTWEIGHT_NORMAL);

	// Set line height
	{
		wxClientDC dc(this);
		dc.SetFont(font);
		int fw,fh;
		dc.GetTextExtent("#TWFfgGhH", &fw, &fh, nullptr, nullptr, &font);
		lineHeight = fh + 4;
	}

	// Set row brushes
	assert(rowColors.size() >= COLOR_LEFT_COL);
	rowColors[COLOR_DEFAULT].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Background/Background")->GetColor()));
	rowColors[COLOR_HEADER].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Header")->GetColor()));
	rowColors[COLOR_SELECTION].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Background/Selection")->GetColor()));
	rowColors[COLOR_COMMENT].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Background/Comment")->GetColor()));
	rowColors[COLOR_VISIBLE].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Background/Inframe")->GetColor()));
	rowColors[COLOR_SELECTED_COMMENT].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Background/Selected Comment")->GetColor()));
	rowColors[COLOR_LEFT_COL].SetColour(to_wx(OPT_GET("Colour/Subtitle Grid/Left Column")->GetColor()));

	// Set column widths
	std::vector<bool> column_array(OPT_GET("Subtitle/Grid/Column")->GetListBool());
	assert(column_array.size() == showCol.size());
	for (size_t i = 0; i < showCol.size(); ++i)
		showCol[i] = column_array[i];
	SetColumnWidths();

	// Update
	AdjustScrollbar();
	Refresh();
}

void BaseGrid::ClearMaps() {
	Selection old_selection(selection);

	index_line_map.clear();
	line_index_map.clear();
	selection.clear();
	yPos = 0;
	AdjustScrollbar();

	AnnounceSelectedSetChanged(Selection(), old_selection);
}

void BaseGrid::UpdateMaps(bool preserve_selected_rows) {
	BeginBatch();
	int active_row = line_index_map[active_line];

	std::vector<int> sel_rows;
	if (preserve_selected_rows) {
		sel_rows.reserve(selection.size());
		transform(selection.begin(), selection.end(), back_inserter(sel_rows),
			[this](AssDialogue *diag) { return GetDialogueIndex(diag); });
	}

	index_line_map.clear();
	line_index_map.clear();

	for (auto curdiag : context->ass->Line | agi::of_type<AssDialogue>()) {
		line_index_map[curdiag] = (int)index_line_map.size();
		index_line_map.push_back(curdiag);
	}

	if (preserve_selected_rows) {
		Selection sel;

		// If the file shrank enough that no selected rows are left, select the
		// last row
		if (sel_rows.empty())
			sel_rows.push_back(index_line_map.size() - 1);
		else if (sel_rows[0] >= (int)index_line_map.size())
			sel_rows[0] = index_line_map.size() - 1;

		for (int row : sel_rows) {
			if (row >= (int)index_line_map.size()) break;
			sel.insert(index_line_map[row]);
		}

		SetSelectedSet(sel);
	}
	else {
		Selection lines;
		copy(index_line_map.begin(), index_line_map.end(), inserter(lines, lines.begin()));
		Selection new_sel;
		// Remove lines which no longer exist from the selection
		set_intersection(selection.begin(), selection.end(),
			lines.begin(), lines.end(),
			inserter(new_sel, new_sel.begin()));

		SetSelectedSet(new_sel);
	}

	// Force a reannounce of the active line if it hasn't changed, as it isn't
	// safe to touch the active line while processing a commit event which would
	// cause this function to be called
	AssDialogue *line = active_line;
	active_line = nullptr;

	// The active line may have ceased to exist; pick a new one if so
	if (line_index_map.size() && line_index_map.find(line) == line_index_map.end()) {
		if (active_row < (int)index_line_map.size())
			SetActiveLine(index_line_map[active_row]);
		else if (preserve_selected_rows && !selection.empty())
			SetActiveLine(index_line_map[sel_rows[0]]);
		else
			SetActiveLine(index_line_map.back());
	}
	else {
		SetActiveLine(line);
	}

	if (selection.empty() && active_line) {
		Selection sel;
		sel.insert(active_line);
		SetSelectedSet(sel);
	}

	EndBatch();

	SetColumnWidths();

	Render();
}

void BaseGrid::BeginBatch() {
	++batch_level;
}

void BaseGrid::EndBatch() {
	--batch_level;
	assert(batch_level >= 0);
	if (batch_level == 0) {
		if (batch_active_line_changed)
			AnnounceActiveLineChanged(active_line);
		batch_active_line_changed = false;
		if (!batch_selection_added.empty() || !batch_selection_removed.empty())
			AnnounceSelectedSetChanged(batch_selection_added, batch_selection_removed);
		batch_selection_added.clear();
		batch_selection_removed.clear();
	}

	AdjustScrollbar();
}

void BaseGrid::MakeRowVisible(int row) {
	int h = GetClientSize().GetHeight();

	if (row < yPos + 1)
		ScrollTo(row - 1);
	else if (row > yPos + h/lineHeight - 3)
		ScrollTo(row - h/lineHeight + 3);
}

void BaseGrid::SelectRow(int row, bool addToSelected, bool select) {
	if (row < 0 || (size_t)row >= index_line_map.size()) return;

	AssDialogue *line = index_line_map[row];

	if (!addToSelected) {
		Selection sel;
		if (select) sel.insert(line);
		SetSelectedSet(sel);
		return;
	}

	if (select && selection.find(line) == selection.end()) {
		selection.insert(line);

		Selection added;
		added.insert(line);

		AnnounceSelectedSetChanged(added, Selection());
	}
	else if (!select && selection.find(line) != selection.end()) {
		selection.erase(line);

		Selection removed;
		removed.insert(line);

		AnnounceSelectedSetChanged(Selection(), removed);
	}

	int w = GetClientSize().GetWidth();
	RefreshRect(wxRect(0, (row + 1 - yPos) * lineHeight, w, lineHeight), false);
}

wxArrayInt BaseGrid::GetSelection() const {
	wxArrayInt res;
	res.reserve(selection.size());
	transform(selection.begin(), selection.end(), std::back_inserter(res),
		std::bind(&BaseGrid::GetDialogueIndex, this, std::placeholders::_1));
	std::sort(res.begin(), res.end());
	return res;
}

void BaseGrid::Render() {
	if (rendering) {
		wants_render = true;
		return;
	}

	rendering = true;
	wants_render = false;

	wxSize cs = GetClientSize();
	cs.SetWidth(cs.GetWidth() - scrollBar->GetSize().GetWidth());

	int line_count = mid(0, cs.GetHeight() / lineHeight + 1, GetRows() - yPos);

	// Make a copy of all of the data we need to avoid data races
	auto lines = std::make_shared<std::vector<AssDialogue>>();
	lines->reserve(line_count);
	std::vector<int> visible(line_count, false);
	std::vector<int> selected(line_count, false);
	std::vector<std::pair<int, int>> line_frames;
	if (byFrame)
		line_frames.reserve(line_count);
	bool highlight = OPT_GET("Subtitle/Grid/Highlight Subtitles in Frame")->GetBool();
	size_t active = -1;

	for (int i = 0; i < line_count; ++i) {
		auto line = GetDialogue(i + yPos);
		lines->emplace_back(*line);
		visible[i] = highlight && IsDisplayed(line);
		selected[i] = !!selection.count(line);
		if (line == active_line)
			active = i;

		if (byFrame && (colWidth[2] || colWidth[3]))
			line_frames.emplace_back(
				context->videoController->FrameAtTime(line->Start, agi::vfr::START),
				context->videoController->FrameAtTime(line->End, agi::vfr::END));
	}

	auto font = this->font;
	auto rowColors = this->rowColors;
	auto lineHeight = this->lineHeight;
	auto colWidth = this->colWidth;
	auto yPos = this->yPos;
	std::shared_ptr<AssDialogue> active_line;
	if (this->active_line)
		active_line = std::make_shared<AssDialogue>(*this->active_line);

	auto text_standard = to_wx(OPT_GET("Colour/Subtitle Grid/Standard")->GetColor());
	auto text_selection = to_wx(OPT_GET("Colour/Subtitle Grid/Selection")->GetColor());
	auto text_collision = to_wx(OPT_GET("Colour/Subtitle Grid/Collision")->GetColor());
	auto grid_pen = to_wx(OPT_GET("Colour/Subtitle Grid/Lines")->GetColor());
	auto active_pen = wxPen(to_wx(OPT_GET("Colour/Subtitle Grid/Active Border")->GetColor()));
	int override_mode = OPT_GET("Subtitle/Grid/Hide Overrides")->GetInt();

	wxString replace_char;
	if (override_mode == 1)
		replace_char = to_wx(OPT_GET("Subtitle/Grid/Hide Overrides Char")->GetString());

	// wxObject ref counting isn't thread-safe, so we need a real copy
	for (auto& rc : rowColors)
		rc.UnShare();

	agi::dispatch::Background().Async([=]{
		auto bmp = agi::util::make_unique<wxBitmap>(cs);
		wxMemoryDC dc(*bmp);

		//dc.SetFont(font);

		//dc.SetBackground(rowColors[COLOR_DEFAULT]);
		//dc.Clear();

		//// Draw labels
		//dc.SetPen(*wxTRANSPARENT_PEN);
		//dc.SetBrush(rowColors[COLOR_LEFT_COL]);
		//dc.DrawRectangle(0, lineHeight, colWidth[0], cs.GetHeight() - lineHeight);

		//// First grid row
		//dc.SetPen(grid_pen);
		//dc.DrawLine(0, 0, cs.GetWidth(), 0);
		//dc.SetPen(*wxTRANSPARENT_PEN);

		//wxString strings[] = {
		//	("#"), ("L"), ("Start"), ("End"), ("Style"), ("Actor"),
		//	("Effect"), ("Left"), ("Right"), ("Vert"), ("Text")
		//};

		//for (size_t i = 0; i < lines->size() + 1; ++i) {
		//	RowColor curColor = COLOR_HEADER;

		//	if (i == 0)
		//		dc.SetTextForeground(text_standard);
		//	else {
		//		auto& line = (*lines)[i - 1];

		//		if (colWidth[0]) strings[0] = std::to_wstring(yPos + i);
		//		if (colWidth[1]) strings[1] = std::to_wstring(line.Layer);
		//		if (line_frames.size()) {
		//			if (colWidth[2]) strings[2] = std::to_wstring(line_frames[i - 1].first);
		//			if (colWidth[3]) strings[3] = std::to_wstring(line_frames[i - 1].second);
		//		}
		//		else {
		//			if (colWidth[2]) strings[2] = to_wx(line.Start.GetAssFormated());
		//			if (colWidth[3]) strings[3] = to_wx(line.End.GetAssFormated());
		//		}
		//		if (colWidth[4]) strings[4] = to_wx(line.Style);
		//		if (colWidth[5]) strings[5] = to_wx(line.Actor);
		//		if (colWidth[6]) strings[6] = to_wx(line.Effect);
		//		if (colWidth[7]) strings[7] = std::to_wstring(line.Margin[0]);
		//		if (colWidth[8]) strings[8] = std::to_wstring(line.Margin[1]);
		//		if (colWidth[9]) strings[9] = std::to_wstring(line.Margin[2]);

		//		if (colWidth[10]) {
		//			strings[10].clear();

		//			// Show overrides
		//			if (!override_mode)
		//				strings[10] = to_wx(line.Text);
		//			// Hidden overrides
		//			else {
		//				auto const& text = line.Text.get();
		//				strings[10].reserve(text.size());
		//				size_t start = 0, pos;
		//				while ((pos = text.find('{', start)) != std::string::npos) {
		//					strings[10] += to_wx(text.substr(start, pos - start));
		//					strings[10] += replace_char;
		//					start = text.find('}', pos);
		//					if (start != std::string::npos) ++start;
		//				}
		//				if (start != std::string::npos)
		//					strings[10] += to_wx(text.substr(start));
		//			}

		//			// Cap length and set text
		//			if (strings[10].size() > 512)
		//				strings[10] = strings[10].Left(512) + "...";
		//		}

		//		if (selected[i - 1] && line.Comment)
		//			curColor = COLOR_SELECTED_COMMENT;
		//		else if (selected[i - 1])
		//			curColor = COLOR_SELECTION;
		//		else if (line.Comment)
		//			curColor = COLOR_COMMENT;
		//		else if (visible[i - 1])
		//			curColor = COLOR_VISIBLE;
		//		else
		//			curColor = COLOR_DEFAULT;

		//		if (i + 1 != active && active_line && active_line->CollidesWith(&line))
		//			dc.SetTextForeground(text_collision);
		//		else if (selected[i - 1])
		//			dc.SetTextForeground(text_selection);
		//		else
		//			dc.SetTextForeground(text_standard);
		//	}

		//	// Draw row background color
		//	if (curColor) {
		//		dc.SetBrush(rowColors[curColor]);
		//		dc.DrawRectangle(curColor == 1 ? 0 : colWidth[0], i*lineHeight+1, cs.GetWidth(), lineHeight);
		//	}

		//	// Draw text
		//	int dx = 0;
		//	int dy = i*lineHeight;
		//	for (size_t j = 0; j < colWidth.size(); j++) {
		//		if (colWidth[j] == 0) continue;

		//		wxSize ext = dc.GetTextExtent(strings[j]);

		//		int left = dx + 4;
		//		int top = dy + (lineHeight - ext.GetHeight()) / 2;

		//		// Centered columns
		//		if (!(j == 4 || j == 5 || j == 6 || j == 10))
		//			left += (colWidth[j] - 6 - ext.GetWidth()) / 2;

		//		dc.DrawText(strings[j], left, top);

		//		dx += colWidth[j];
		//	}

		//	// Draw grid
		//	dc.DestroyClippingRegion();
		//	dc.SetPen(grid_pen);
		//	dc.DrawLine(0, dy+lineHeight, cs.GetWidth(), dy+lineHeight);
		//	dc.SetPen(*wxTRANSPARENT_PEN);
		//}

		// Draw grid columns
		int dx = 0;
		dc.SetPen(grid_pen);
		for (auto width : colWidth) {
			dx += width;
			dc.DrawLine(dx, 0, dx, cs.GetHeight());
		}
		dc.DrawLine(0, 0, 0, cs.GetHeight());
		dc.DrawLine(cs.GetWidth() - 1, 0, cs.GetWidth() - 1, cs.GetHeight());

		// Draw currently active line border
		if (active < lines->size()) {
			dc.SetPen(active_pen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			dc.DrawRectangle(0, active * lineHeight, cs.GetWidth(), lineHeight+1);
		}

		agi::dispatch::Main().Sync([&]{
			rendered = std::move(bmp);
			rendering = false;
			if (wants_render)
				Render();
			Refresh(false);
		});
	});
}

void BaseGrid::OnPaint(wxPaintEvent &) {
	wxPaintDC(this).DrawBitmap(*rendered, 0, 0);
}

void BaseGrid::OnSize(wxSizeEvent &) {
	AdjustScrollbar();

	int w, h;
	GetClientSize(&w, &h);
	colWidth[10] = w - std::accumulate(begin(colWidth), end(colWidth) - 1, 0);

	Render();
}

void BaseGrid::OnScroll(wxScrollEvent &event) {
	int newPos = event.GetPosition();
	if (yPos != newPos) {
		yPos = newPos;
		Render();
	}
}

void BaseGrid::OnMouseEvent(wxMouseEvent &event) {
	int h = GetClientSize().GetHeight();
	bool shift = event.ShiftDown();
	bool alt = event.AltDown();
	bool ctrl = event.CmdDown();

	// Row that mouse is over
	bool click = event.LeftDown();
	bool dclick = event.LeftDClick();
	int row = event.GetY() / lineHeight + yPos - 1;
	if (holding && !click)
		row = mid(0, row, GetRows()-1);
	AssDialogue *dlg = GetDialogue(row);
	if (!dlg) row = 0;

	if (event.ButtonDown() && OPT_GET("Subtitle/Grid/Focus Allow")->GetBool())
		SetFocus();

	if (holding) {
		if (!event.LeftIsDown()) {
			if (dlg)
				MakeRowVisible(row);
			holding = false;
			ReleaseMouse();
		}
		else {
			// Only scroll if the mouse has moved to a different row to avoid
			// scrolling on sloppy clicks
			if (row != extendRow) {
				if (row <= yPos)
					ScrollTo(yPos - 3);
				// When dragging down we give a 3 row margin to make it easier
				// to see what's going on, but we don't want to scroll down if
				// the user clicks on the bottom row and drags up
				else if (row > yPos + h / lineHeight - (row > extendRow ? 3 : 1))
					ScrollTo(yPos + 3);
			}
		}
	}
	else if (click && dlg) {
		holding = true;
		CaptureMouse();
	}

	if ((click || holding || dclick) && dlg) {
		int old_extend = extendRow;

		// SetActiveLine will scroll the grid if the row is only half-visible,
		// but we don't want to scroll until the mouse moves or the button is
		// released, to avoid selecting multiple lines on a click
		int old_y_pos = yPos;
		SetActiveLine(dlg);
		ScrollTo(old_y_pos);

		// Toggle selected
		if (click && ctrl && !shift && !alt) {
			bool isSel = !!selection.count(dlg);
			if (isSel && selection.size() == 1) return;
			SelectRow(row, true, !isSel);
			return;
		}

		// Normal click
		if ((click || dclick) && !shift && !ctrl && !alt) {
			if (dclick) {
				context->audioBox->ScrollToActiveLine();
				context->videoController->JumpToTime(dlg->Start);
			}
			SelectRow(row, false);
			return;
		}

		// Change active line only
		if (click && !shift && !ctrl && alt)
			return;

		// Block select
		if ((click && shift && !alt) || holding) {
			extendRow = old_extend;
			int i1 = row;
			int i2 = extendRow;

			if (i1 > i2)
				std::swap(i1, i2);

			// Toggle each
			Selection newsel;
			if (ctrl) newsel = selection;
			for (int i = i1; i <= i2; i++) {
				newsel.insert(GetDialogue(i));
			}
			SetSelectedSet(newsel);
			return;
		}

		return;
	}

	// Mouse wheel
	if (event.GetWheelRotation() != 0) {
		if (ForwardMouseWheelEvent(this, event)) {
			int step = shift ? h / lineHeight - 2 : 3;
			ScrollTo(yPos - step * event.GetWheelRotation() / event.GetWheelDelta());
		}
		return;
	}

	event.Skip();
}

void BaseGrid::OnContextMenu(wxContextMenuEvent &evt) {
	wxPoint pos = evt.GetPosition();
	if (pos == wxDefaultPosition || ScreenToClient(pos).y > lineHeight) {
		if (!context_menu) context_menu = menu::GetMenu("grid_context", context);
		menu::OpenPopupMenu(context_menu.get(), this);
	}
	else {
		const wxString strings[] = {
			_("Line Number"),
			_("Layer"),
			_("Start"),
			_("End"),
			_("Style"),
			_("Actor"),
			_("Effect"),
			_("Left"),
			_("Right"),
			_("Vert"),
		};

		wxMenu menu;
		for (size_t i = 0; i < showCol.size(); ++i)
			menu.Append(MENU_SHOW_COL + i, strings[i], "", wxITEM_CHECK)->Check(showCol[i]);
		PopupMenu(&menu);
	}
}

void BaseGrid::ScrollTo(int y) {
	int nextY = mid(0, y, GetRows() - 1);
	if (yPos != nextY) {
		yPos = nextY;
		scrollBar->SetThumbPosition(yPos);
		Render();
	}
}

void BaseGrid::AdjustScrollbar() {
	wxSize clientSize = GetClientSize();
	wxSize scrollbarSize = scrollBar->GetSize();

	scrollBar->Freeze();
	scrollBar->SetSize(clientSize.GetWidth() - scrollbarSize.GetWidth(), 0, scrollbarSize.GetWidth(), clientSize.GetHeight());

	if (GetRows() <= 1) {
		scrollBar->Enable(false);
		scrollBar->Thaw();
		return;
	}

	if (!scrollBar->IsEnabled())
		scrollBar->Enable(true);

	int drawPerScreen = clientSize.GetHeight() / lineHeight;
	int rows = GetRows();

	yPos = mid(0, yPos, rows - 1);

	scrollBar->SetScrollbar(yPos, drawPerScreen, rows + drawPerScreen - 1, drawPerScreen - 2, true);
	scrollBar->Thaw();
}

void BaseGrid::SetColumnWidths() {
	int w, h;
	GetClientSize(&w, &h);

	// DC for text extents test
	wxClientDC dc(this);
	dc.SetFont(font);

	// O(1) widths
	int marginLen = dc.GetTextExtent("0000").GetWidth();

	int labelLen = dc.GetTextExtent(std::to_wstring(GetRows())).GetWidth();
	int startLen = 0;
	int endLen = 0;
	if (!byFrame)
		startLen = endLen = dc.GetTextExtent(to_wx(AssTime().GetAssFormated())).GetWidth();

	std::unordered_map<boost::flyweight<std::string>, int> widths;
	auto get_width = [&](boost::flyweight<std::string> const& str) -> int {
		auto it = widths.find(str);
		if (it != end(widths)) return it->second;
		int width = dc.GetTextExtent(to_wx(str)).GetWidth();
		widths[str] = width;
		return width;
	};

	// O(n) widths
	bool showMargin[3] = { false, false, false };
	int styleLen = 0;
	int actorLen = 0;
	int effectLen = 0;
	int maxLayer = 0;
	int maxStart = 0;
	int maxEnd = 0;
	for (int i = 0; i < GetRows(); i++) {
		AssDialogue *curDiag = GetDialogue(i);

		maxLayer = std::max(maxLayer, curDiag->Layer);
		actorLen = std::max(actorLen, get_width(curDiag->Actor));
		styleLen = std::max(styleLen, get_width(curDiag->Style));
		effectLen = std::max(effectLen, get_width(curDiag->Effect));

		// Margins
		for (int j = 0; j < 3; j++) {
			if (curDiag->Margin[j])
				showMargin[j] = true;
		}

		// Times
		if (byFrame) {
			maxStart = std::max(maxStart, context->videoController->FrameAtTime(curDiag->Start, agi::vfr::START));
			maxEnd = std::max(maxEnd, context->videoController->FrameAtTime(curDiag->End, agi::vfr::END));
		}
	}

	// Finish layer
	int layerLen = maxLayer ? dc.GetTextExtent(std::to_wstring(maxLayer)).GetWidth() : 0;

	// Finish times
	if (byFrame) {
		startLen = dc.GetTextExtent(std::to_wstring(maxStart)).GetWidth();
		endLen = dc.GetTextExtent(std::to_wstring(maxEnd)).GetWidth();
	}

	// Set column widths
	colWidth[0] = labelLen;
	colWidth[1] = layerLen;
	colWidth[2] = startLen;
	colWidth[3] = endLen;
	colWidth[4] = styleLen;
	colWidth[5] = actorLen;
	colWidth[6] = effectLen;
	for (int i = 0; i < 3; i++)
		colWidth[i + 7] = showMargin[i] ? marginLen : 0;
	colWidth[10] = 1;

	// Hide columns
	for (size_t i = 0; i < showCol.size(); ++i) {
		if (!showCol[i])
			colWidth[i] = 0;
	}

	wxString col_names[11] = {
		_("#"),
		_("L"),
		_("Start"),
		_("End"),
		_("Style"),
		_("Actor"),
		_("Effect"),
		_("Left"),
		_("Right"),
		_("Vert"),
		_("Text")
	};

	// Ensure every visible column is at least as big as its header
	for (size_t i = 0; i < 11; ++i) {
		if (colWidth[i])
			colWidth[i] = std::max(colWidth[i], dc.GetTextExtent(col_names[i]).GetWidth());
	}

	// Add padding to all non-empty columns
	for (size_t i = 0; i < 10; ++i) {
		if (colWidth[i])
			colWidth[i] += 10;
	}


	// Set size of last
	int total = std::accumulate(begin(colWidth), end(colWidth) - 1, 0);
	colWidth[10] = std::max(w - total, 0);
}

AssDialogue *BaseGrid::GetDialogue(int n) const {
	if (static_cast<size_t>(n) >= index_line_map.size()) return nullptr;
	return index_line_map[n];
}

int BaseGrid::GetDialogueIndex(AssDialogue *diag) const {
	auto it = line_index_map.find(diag);
	return it != line_index_map.end() ? it->second : -1;
}

bool BaseGrid::IsDisplayed(const AssDialogue *line) const {
	if (!context->videoController->IsLoaded()) return false;
	int frame = context->videoController->GetFrameN();
	return context->videoController->FrameAtTime(line->Start,agi::vfr::START) <= frame
		&& context->videoController->FrameAtTime(line->End,agi::vfr::END) >= frame;
}

void BaseGrid::OnCharHook(wxKeyEvent &event) {
	if (hotkey::check("Subtitle Grid", context, event))
		return;

	int key = event.GetKeyCode();

	if (key == WXK_UP || key == WXK_DOWN ||
		key == WXK_PAGEUP || key == WXK_PAGEDOWN ||
		key == WXK_HOME || key == WXK_END)
	{
		event.Skip();
		return;
	}

	hotkey::check("Audio", context, event);
}

void BaseGrid::OnKeyDown(wxKeyEvent &event) {
	int w,h;
	GetClientSize(&w, &h);

	int key = event.GetKeyCode();
	bool ctrl = event.CmdDown();
	bool alt = event.AltDown();
	bool shift = event.ShiftDown();

	int dir = 0;
	int step = 1;
	if (key == WXK_UP) dir = -1;
	else if (key == WXK_DOWN) dir = 1;
	else if (key == WXK_PAGEUP) {
		dir = -1;
		step = h / lineHeight - 2;
	}
	else if (key == WXK_PAGEDOWN) {
		dir = 1;
		step = h / lineHeight - 2;
	}
	else if (key == WXK_HOME) {
		dir = -1;
		step = GetRows();
	}
	else if (key == WXK_END) {
		dir = 1;
		step = GetRows();
	}

	if (!dir) {
		event.Skip();
		return;
	}

	int old_extend = extendRow;
	int next = mid(0, GetDialogueIndex(active_line) + dir * step, GetRows() - 1);
	SetActiveLine(GetDialogue(next));

	// Move selection
	if (!ctrl && !shift && !alt) {
		SelectRow(next);
		return;
	}

	// Move active only
	if (alt && !shift && !ctrl) {
		Render();
		return;
	}

	// Shift-selection
	if (shift && !ctrl && !alt) {
		extendRow = old_extend;
		// Set range
		int begin = next;
		int end = extendRow;
		if (end < begin)
			std::swap(begin, end);

		// Select range
		Selection newsel;
		for (int i = begin; i <= end; i++)
			newsel.insert(GetDialogue(i));

		SetSelectedSet(newsel);

		MakeRowVisible(next);
		return;
	}
}

void BaseGrid::SetByFrame(bool state) {
	if (byFrame == state) return;
	byFrame = state;
	SetColumnWidths();
	Render();
}

void BaseGrid::SetSelectedSet(const Selection &new_selection) {
	Selection inserted, removed;
	set_difference(new_selection, selection, inserted);
	set_difference(selection, new_selection, removed);
	selection = new_selection;
	AnnounceSelectedSetChanged(inserted, removed);
	Render();
}

void BaseGrid::SetActiveLine(AssDialogue *new_line) {
	if (new_line != active_line) {
		assert(new_line == 0 || line_index_map.count(new_line));
		active_line = new_line;
		AnnounceActiveLineChanged(active_line);
		MakeRowVisible(GetDialogueIndex(active_line));
		Render();
		extendRow = GetDialogueIndex(new_line);
	}
}

void BaseGrid::SetSelectionAndActive(Selection const& new_selection, AssDialogue *new_line) {
	BeginBatch();
	SetSelectedSet(new_selection);
	SetActiveLine(new_line);
	EndBatch();
}

void BaseGrid::PrevLine() {
	int cur_line_i = GetDialogueIndex(GetActiveLine());
	if (AssDialogue *prev_line = GetDialogue(cur_line_i-1)) {
		SetActiveLine(prev_line);
		Selection newsel;
		newsel.insert(prev_line);
		SetSelectedSet(newsel);
	}
}

void BaseGrid::NextLine() {
	int cur_line_i = GetDialogueIndex(GetActiveLine());
	if (AssDialogue *next_line = GetDialogue(cur_line_i+1)) {
		SetActiveLine(next_line);
		Selection newsel;
		newsel.insert(next_line);
		SetSelectedSet(newsel);
	}
}

void BaseGrid::AnnounceActiveLineChanged(AssDialogue *new_line) {
	if (batch_level > 0)
		batch_active_line_changed = true;
	else
		SubtitleSelectionController::AnnounceActiveLineChanged(new_line);
}

void BaseGrid::AnnounceSelectedSetChanged(const Selection &lines_added, const Selection &lines_removed) {
	if (batch_level > 0) {
		// Remove all previously added lines that are now removed
		Selection temp;
		set_difference(batch_selection_added, lines_removed, temp);
		std::swap(temp, batch_selection_added);
		temp.clear();

		// Remove all previously removed lines that are now added
		set_difference(batch_selection_removed, lines_added, temp);
		std::swap(temp, batch_selection_removed);

		// Add new stuff to batch sets
		batch_selection_added.insert(lines_added.begin(), lines_added.end());
		batch_selection_removed.insert(lines_removed.begin(), lines_removed.end());
	}
	else {
		SubtitleSelectionController::AnnounceSelectedSetChanged(lines_added, lines_removed);
	}
}
