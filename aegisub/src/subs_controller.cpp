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

#include "subs_controller.h"

#include "ass_attachment.h"
#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_info.h"
#include "ass_style.h"
#include "base_grid.h"
#include "charset_detect.h"
#include "compat.h"
#include "command/command.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "selection_controller.h"
#include "subtitle_format.h"
#include "text_file_reader.h"
#include "utils.h"
#include "video_context.h"

#include <libaegisub/fs.h>
#include <libaegisub/path.h>
#include <libaegisub/util.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/format.hpp>
#include <wx/msgdlg.h>

namespace {
	void autosave_timer_changed(wxTimer *timer) {
		int freq = OPT_GET("App/Auto/Save Every Seconds")->GetInt();
		if (freq > 0 && OPT_GET("App/Auto/Save")->GetBool())
			timer->Start(freq * 1000);
		else
			timer->Stop();
	}
}

struct SubsController::UndoInfo {
	std::vector<std::pair<std::string, std::string>> script_info;
	std::vector<AssStyle> styles;
	std::vector<AssDialogueBase> events;
	std::vector<AssAttachment> graphics;
	std::vector<AssAttachment> fonts;

	mutable std::vector<int> selection;
	int active_line_id = 0;

	wxString undo_description;
	int commit_id;

	UndoInfo(const agi::Context *c, wxString const& d, int commit_id)
	: undo_description(d), commit_id(commit_id)
	{
		script_info.reserve(c->ass->Info.size());
		for (auto const& info : c->ass->Info)
			script_info.emplace_back(info.Key(), info.Value());

		styles.reserve(c->ass->Styles.size());
		styles.assign(c->ass->Styles.begin(), c->ass->Styles.end());

		events.reserve(c->ass->Events.size());
		events.assign(c->ass->Events.begin(), c->ass->Events.end());

		for (auto const& line : c->ass->Attachments) {
			switch (line.Group()) {
			case AssEntryGroup::FONT:
				fonts.push_back(line);
				break;
			case AssEntryGroup::GRAPHIC:
				graphics.push_back(line);
				break;
			default:
				assert(false);
				break;
			}
		}

		UpdateActiveLine(c);
		UpdateSelection(c);
	}

	void Apply(agi::Context *c) const {
		// Keep old lines alive until after the commit is complete
		AssFile old;
		old.swap(*c->ass);

		sort(begin(selection), end(selection));

		AssDialogue *active_line = nullptr;
		SubtitleSelection new_sel;

		for (auto const& info : script_info)
			c->ass->Info.push_back(*new AssInfo(info.first, info.second));
		for (auto const& style : styles)
			c->ass->Styles.push_back(*new AssStyle(style));
		for (auto const& attachment : fonts)
			c->ass->Attachments.push_back(*new AssAttachment(attachment));
		for (auto const& attachment : graphics)
			c->ass->Attachments.push_back(*new AssAttachment(attachment));
		for (auto const& event : events) {
			auto copy = new AssDialogue(event);
			c->ass->Events.push_back(*copy);
			if (copy->Id == active_line_id)
				active_line = copy;
			if (binary_search(begin(selection), end(selection), copy->Id))
				new_sel.insert(copy);
		}

		c->subsGrid->BeginBatch();
		c->selectionController->SetSelectedSet(std::set<AssDialogue *>{});
		c->ass->Commit("", AssFile::COMMIT_NEW);
		c->selectionController->SetSelectionAndActive(new_sel, active_line);
		c->subsGrid->EndBatch();
	}

	void UpdateActiveLine(const agi::Context *c) {
		auto line = c->selectionController->GetActiveLine();
		if (line)
			active_line_id = line->Id;
	}

	void UpdateSelection(const agi::Context *c) {
		auto const& sel = c->selectionController->GetSelectedSet();
		selection.clear();
		selection.reserve(sel.size());
		for (const auto diag : sel)
			selection.push_back(diag->Id);
	}
};

SubsController::SubsController(agi::Context *context)
: context(context)
, undo_connection(context->ass->AddUndoManager(&SubsController::OnCommit, this))
{
	autosave_timer_changed(&autosave_timer);
	OPT_SUB("App/Auto/Save", autosave_timer_changed, &autosave_timer);
	OPT_SUB("App/Auto/Save Every Seconds", autosave_timer_changed, &autosave_timer);

	autosave_timer.Bind(wxEVT_TIMER, [=](wxTimerEvent&) {
		try {
			auto fn = AutoSave();
			if (!fn.empty())
				StatusTimeout(wxString::Format(_("File backup saved as \"%s\"."), fn.wstring()));
		}
		catch (const agi::Exception& err) {
			StatusTimeout(to_wx("Exception when attempting to autosave file: " + err.GetMessage()));
		}
		catch (...) {
			StatusTimeout("Unhandled exception when attempting to autosave file.");
		}
	});
}

void SubsController::SetSelectionController(SelectionController<AssDialogue *> *selection_controller) {
	active_line_connection = context->selectionController->AddActiveLineListener(&SubsController::OnActiveLineChanged, this);
	selection_connection = context->selectionController->AddSelectionListener(&SubsController::OnSelectionChanged, this);
}

void SubsController::Load(agi::fs::path const& filename, std::string charset) {
	try {
		try {
			if (charset.empty())
				charset = CharSetDetect::GetEncoding(filename);
		}
		catch (agi::UserCancelException const&) {
			return;
		}

		// Make sure that file isn't actually a timecode file
		if (charset != "binary") {
			try {
				TextFileReader testSubs(filename, charset);
				std::string cur = testSubs.ReadLineFromFile();
				if (boost::starts_with(cur, "# timecode")) {
					context->videoController->LoadTimecodes(filename);
					return;
				}
			}
			catch (...) {
				// if trying to load the file as timecodes fails it's fairly
				// safe to assume that it is in fact not a timecode file
			}
		}

		const SubtitleFormat *reader = SubtitleFormat::GetReader(filename, charset);

		AssFile temp;
		reader->ReadFile(&temp, filename, charset);

		// Make sure the file has at least one style and one dialogue line
		if (temp.Styles.empty())
			temp.Styles.push_back(*new AssStyle);
		if (temp.Events.empty())
			temp.Events.push_back(*new AssDialogue);

		context->ass->swap(temp);
	}
	catch (agi::UserCancelException const&) {
		return;
	}
	catch (agi::fs::FileNotFound const&) {
		wxMessageBox(filename.wstring() + " not found.", "Error", wxOK | wxICON_ERROR | wxCENTER, context->parent);
		config::mru->Remove("Subtitle", filename);
		return;
	}
	catch (agi::Exception const& err) {
		wxMessageBox(to_wx(err.GetChainedMessage()), "Error", wxOK | wxICON_ERROR | wxCENTER, context->parent);
		return;
	}
	catch (std::exception const& err) {
		wxMessageBox(to_wx(err.what()), "Error", wxOK | wxICON_ERROR | wxCENTER, context->parent);
		return;
	}
	catch (...) {
		wxMessageBox("Unknown error", "Error", wxOK | wxICON_ERROR | wxCENTER, context->parent);
		return;
	}

	SetFileName(filename);

	// Push the initial state of the file onto the undo stack
	undo_stack.clear();
	redo_stack.clear();
	autosaved_commit_id = saved_commit_id = commit_id + 1;
	context->ass->Commit("", AssFile::COMMIT_NEW);

	// Save backup of file
	if (CanSave() && OPT_GET("App/Auto/Backup")->GetBool()) {
		auto path_str = OPT_GET("Path/Auto/Backup")->GetString();
		agi::fs::path path;
		if (path_str.empty())
			path = filename.parent_path();
		else
			path = config::path->Decode(path_str);
		agi::fs::CreateDirectory(path);
		agi::fs::Copy(filename, path/(filename.stem().string() + ".ORIGINAL" + filename.extension().string()));
	}

	FileOpen(filename);
}

void SubsController::Save(agi::fs::path const& filename, std::string const& encoding) {
	const SubtitleFormat *writer = SubtitleFormat::GetWriter(filename);
	if (!writer)
		throw "Unknown file type.";

	int old_autosaved_commit_id = autosaved_commit_id, old_saved_commit_id = saved_commit_id;
	try {
		autosaved_commit_id = saved_commit_id = commit_id;

		// Have to set this now for the sake of things that want to save paths
		// relative to the script in the header
		this->filename = filename;
		config::path->SetToken("?script", filename.parent_path());

		FileSave();

		writer->WriteFile(context->ass, filename, encoding);
	}
	catch (...) {
		autosaved_commit_id = old_autosaved_commit_id;
		saved_commit_id = old_saved_commit_id;
		throw;
	}

	SetFileName(filename);
}

void SubsController::Close() {
	undo_stack.clear();
	redo_stack.clear();
	autosaved_commit_id = saved_commit_id = commit_id + 1;
	filename.clear();
	AssFile blank;
	blank.swap(*context->ass);
	context->ass->LoadDefault();
	context->ass->Commit("", AssFile::COMMIT_NEW);
}

int SubsController::TryToClose(bool allow_cancel) const {
	if (!IsModified())
		return wxYES;

	int flags = wxYES_NO;
	if (allow_cancel)
		flags |= wxCANCEL;
	int result = wxMessageBox(wxString::Format(_("Do you want to save changes to %s?"), Filename().wstring()), _("Unsaved changes"), flags, context->parent);
	if (result == wxYES) {
		cmd::call("subtitle/save", context);
		// If it fails saving, return cancel anyway
		return IsModified() ? wxCANCEL : wxYES;
	}
	return result;
}

agi::fs::path SubsController::AutoSave() {
	if (commit_id == autosaved_commit_id)
		return "";

	auto path = config::path->Decode(OPT_GET("Path/Auto/Save")->GetString());
	if (path.empty())
		path = filename.parent_path();

	agi::fs::CreateDirectory(path);

	auto name = filename.filename();
	if (name.empty())
		name = "Untitled";

	path /= str(boost::format("%s.%s.AUTOSAVE.ass") % name.string() % agi::util::strftime("%Y-%m-%d-%H-%M-%S"));

	SubtitleFormat::GetWriter(path)->WriteFile(context->ass, path);
	autosaved_commit_id = commit_id;

	return path;
}

bool SubsController::CanSave() const {
	try {
		return SubtitleFormat::GetWriter(filename)->CanSave(context->ass);
	}
	catch (...) {
		return false;
	}
}

void SubsController::SetFileName(agi::fs::path const& path) {
	filename = path;
	config::path->SetToken("?script", path.parent_path());
	config::mru->Add("Subtitle", path);
	OPT_SET("Path/Last/Subtitles")->SetString(filename.parent_path().string());
}

void SubsController::OnCommit(AssFileCommit c) {
	if (c.message.empty() && !undo_stack.empty()) return;

	static int next_commit_id = 1;

	commit_id = next_commit_id++;
	// Allow coalescing only if it's the last change and the file has not been
	// saved since the last change
	if (commit_id == *c.commit_id+1 && redo_stack.empty() && saved_commit_id+1 != commit_id && autosaved_commit_id+1 != commit_id) {
		// If only one line changed just modify it instead of copying the file
		if (c.single_line && c.single_line->Group() == AssEntryGroup::DIALOGUE) {
			auto src_diag = static_cast<const AssDialogue *>(c.single_line);
			for (auto& diag : undo_stack.back().events) {
				if (diag.Id == src_diag->Id) {
					diag = *src_diag;
					break;
				}
			}
			*c.commit_id = commit_id;
			return;
		}

		undo_stack.pop_back();
	}

	redo_stack.clear();

	undo_stack.emplace_back(context, c.message, commit_id);

	int depth = std::max<int>(OPT_GET("Limits/Undo Levels")->GetInt(), 2);
	while ((int)undo_stack.size() > depth)
		undo_stack.pop_front();

	if (undo_stack.size() > 1 && OPT_GET("App/Auto/Save on Every Change")->GetBool() && !filename.empty() && CanSave())
		Save(filename);

	*c.commit_id = commit_id;
}

void SubsController::OnActiveLineChanged() {
	if (!undo_stack.empty())
		undo_stack.back().UpdateActiveLine(context);
}

void SubsController::OnSelectionChanged() {
	if (!undo_stack.empty())
		undo_stack.back().UpdateSelection(context);
}

void SubsController::Undo() {
	if (undo_stack.size() <= 1) return;
	redo_stack.splice(redo_stack.end(), undo_stack, std::prev(undo_stack.end()));

	commit_id = undo_stack.back().commit_id;
	undo_stack.back().Apply(context);
}

void SubsController::Redo() {
	if (redo_stack.empty()) return;
	undo_stack.splice(undo_stack.end(), redo_stack, std::prev(redo_stack.end()));

	commit_id = undo_stack.back().commit_id;
	undo_stack.back().Apply(context);
}

wxString SubsController::GetUndoDescription() const {
	return IsUndoStackEmpty() ? "" : undo_stack.back().undo_description;
}

wxString SubsController::GetRedoDescription() const {
	return IsRedoStackEmpty() ? "" : redo_stack.back().undo_description;
}

agi::fs::path SubsController::Filename() const {
	if (!filename.empty()) return filename;

	// Apple HIG says "untitled" should not be capitalised
#ifndef __WXMAC__
	return _("Untitled").wx_str();
#else
	return _("untitled").wx_str();
#endif
}
