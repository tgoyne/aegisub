// Copyright (c) 2006, Niels Martin Hansen
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

/// @file dialog_automation.cpp
/// @brief Manage loaded Automation scripts
/// @ingroup secondary_ui
///


#include "config.h"

#include "dialog_automation.h"

#include "auto4_base.h"
#include "compat.h"
#include "command/command.h"
#include "help_button.h"
#include "include/aegisub/context.h"
#include "libresrc/libresrc.h"
#include "options.h"
#include "main.h"
#include "subtitle_format.h"

#include <algorithm>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

using std::placeholders::_1;

DialogAutomation::DialogAutomation(agi::Context *c)
: wxDialog(c->parent, -1, _("Automation Manager"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
, context(c)
, local_manager(c->local_scripts)
, local_scripts_changed(local_manager->AddScriptChangeListener(&DialogAutomation::RebuildList, this))
, global_manager(wxGetApp().global_scripts)
, global_scripts_changed(global_manager->AddScriptChangeListener(&DialogAutomation::RebuildList, this))
{
	SetIcon(GETICON(automation_toolbutton_16));

	// create main controls
	list = new wxListView(this, -1, wxDefaultPosition, wxSize(600, 175), wxLC_REPORT|wxLC_SINGLE_SEL);
	wxButton *add_button = new wxButton(this, -1, _("&Add"));
	remove_button = new wxButton(this, -1, _("&Remove"));
	reload_button = new wxButton(this, -1, _("Re&load"));
	wxButton *info_button = new wxButton(this, -1, _("Show &Info"));
	wxButton *reload_autoload_button = new wxButton(this, -1, _("Re&scan Autoload Dir"));
	wxButton *close_button = new wxButton(this, wxID_CANCEL, _("&Close"));

	list->Bind(wxEVT_COMMAND_LIST_ITEM_SELECTED, std::bind(&DialogAutomation::UpdateDisplay, this));
	list->Bind(wxEVT_COMMAND_LIST_ITEM_DESELECTED, std::bind(&DialogAutomation::UpdateDisplay, this));
	add_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogAutomation::OnAdd, this);
	remove_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogAutomation::OnRemove, this);
	reload_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogAutomation::OnReload, this);
	info_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogAutomation::OnInfo, this);
	reload_autoload_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogAutomation::OnReloadAutoload, this);

	// add headers to list view
	list->InsertColumn(0, "", wxLIST_FORMAT_CENTER, 20);
	list->InsertColumn(1, _("Name"), wxLIST_FORMAT_LEFT, 140);
	list->InsertColumn(2, _("Filename"), wxLIST_FORMAT_LEFT, 90);
	list->InsertColumn(3, _("Description"), wxLIST_FORMAT_LEFT, 330);

	// button layout
	wxSizer *button_box = new wxBoxSizer(wxHORIZONTAL);
	button_box->AddStretchSpacer(2);
	button_box->Add(add_button, 0);
	button_box->Add(remove_button, 0);
	button_box->AddSpacer(10);
	button_box->Add(reload_button, 0);
	button_box->Add(info_button, 0);
	button_box->AddSpacer(10);
	button_box->Add(reload_autoload_button, 0);
	button_box->AddSpacer(10);
	button_box->Add(new HelpButton(this,"Automation Manager"), 0);
	button_box->Add(close_button, 0);
	button_box->AddStretchSpacer(2);

	// main layout
	wxSizer *main_box = new wxBoxSizer(wxVERTICAL);
	main_box->Add(list, wxSizerFlags(1).Expand().Border());
	main_box->Add(button_box, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));
	SetSizerAndFit(main_box);
	Center();

	// why doesn't this work... the button gets the "default" decoration but doesn't answer to Enter
	// ("esc" does work)
	SetDefaultItem(close_button);
	SetAffirmativeId(wxID_CANCEL);
	close_button->SetDefault();

	RebuildList();
}

template<class Container, class Pred>
static inline void for_each(Container const& c, Pred p)
{
	std::for_each(c.begin(), c.end(), p);
}

template<class Container, class Out, class Func>
static inline void transform(Container const& c, Out o, Func f)
{
	std::transform(c.begin(), c.end(), o, f);
}

void DialogAutomation::RebuildList()
{
	script_info.clear();
	list->DeleteAllItems();

	for_each(local_manager->GetScripts(), std::bind(&DialogAutomation::AddScript, this, _1, false));
	for_each(global_manager->GetScripts(), std::bind(&DialogAutomation::AddScript, this, _1, true));

	UpdateDisplay();
}

void DialogAutomation::SetScriptInfo(int i, Automation4::Script *script)
{
	list->SetItem(i, 1, to_wx(script->GetName()));
	list->SetItem(i, 2, to_wx(script->GetPrettyFilename()));
	list->SetItem(i, 3, to_wx(script->GetDescription()));
	if (!script->GetLoadedState())
		list->SetItemBackgroundColour(i, wxColour(255,128,128));
	else
		list->SetItemBackgroundColour(i, list->GetBackgroundColour());
}

void DialogAutomation::AddScript(Automation4::Script *script, bool is_global)
{
	ExtraScriptInfo ei = { script, is_global };
	script_info.push_back(ei);

	wxListItem itm;
	itm.SetText(is_global ? "G" : "L");
	itm.SetData((int)script_info.size()-1);
	itm.SetId(list->GetItemCount());
	SetScriptInfo(list->InsertItem(itm), script);
}

void DialogAutomation::UpdateDisplay()
{
	int i = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	bool selected = i >= 0;
	bool local = selected && !script_info[list->GetItemData(i)].is_global;
	remove_button->Enable(local);
	reload_button->Enable(selected);
}

template<class Container>
static bool has_file(Container const& c, wxFileName const& fn)
{
	return any_of(c.begin(), c.end(),
		[&](const Automation4::Script *s) { return fn.SameAs(to_wx(s->GetFilename())); });
}

void DialogAutomation::OnAdd(wxCommandEvent &)
{
	wxFileDialog diag(this,
		_("Add Automation script"),
		to_wx(OPT_GET("Path/Last/Automation")->GetString()),
		"",
		to_wx(Automation4::ScriptFactory::GetWildcardStr()),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	if (diag.ShowModal() == wxID_CANCEL) return;

	wxArrayString fnames;
	diag.GetPaths(fnames);

	for (auto const& fname : fnames) {
		wxFileName fnpath(fname);
		OPT_SET("Path/Last/Automation")->SetString(from_wx(fnpath.GetPath()));

		if (has_file(local_manager->GetScripts(), fnpath) || has_file(global_manager->GetScripts(), fnpath)) {
			wxLogError("Script '%s' is already loaded", fname);
			continue;
		}

		local_manager->Add(Automation4::ScriptFactory::CreateFromFile(from_wx(fname), true));
	}
}

void DialogAutomation::OnRemove(wxCommandEvent &)
{
	int i = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (i < 0) return;
	const ExtraScriptInfo &ei = script_info[list->GetItemData(i)];
	if (ei.is_global) return;

	local_manager->Remove(ei.script);
}

void DialogAutomation::OnReload(wxCommandEvent &)
{
	int i = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	if (i < 0) return;

	ExtraScriptInfo const& ei = script_info[list->GetItemData(i)];
	if (ei.is_global)
		global_manager->Reload(ei.script);
	else
		local_manager->Reload(ei.script);
}

static wxString fac_to_str(const Automation4::ScriptFactory* f) {
	return wxString::Format("- %s (%s)", f->GetEngineName(), f->GetFilenamePattern());
}

static wxString cmd_to_str(const cmd::Command *f, agi::Context *c) {
	return wxString::Format(_("    Macro: %s (%s)"), f->StrDisplay(c), f->name());
}

static wxString filt_to_str(const Automation4::ExportFilter* f) {
	return wxString::Format(_("    Export filter: %s"), to_wx(f->GetName()));
}

static wxString form_to_str(const SubtitleFormat* f) {
	return wxString::Format(_("    Subtitle format handler: %s"), f->GetName());
}

void DialogAutomation::OnInfo(wxCommandEvent &)
{
	int i = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	ExtraScriptInfo *ei = i >= 0 ? &script_info[list->GetItemData(i)] : 0;

	wxArrayString info;
	std::back_insert_iterator<wxArrayString> append_info(info);

	info.push_back(wxString::Format(
		_("Total scripts loaded: %d\nGlobal scripts loaded: %d\nLocal scripts loaded: %d\n"),
		(int)local_manager->GetScripts().size() + (int)global_manager->GetScripts().size(),
		(int)global_manager->GetScripts().size(),
		(int)local_manager->GetScripts().size()));

	info.push_back(_("Scripting engines installed:"));
	transform(Automation4::ScriptFactory::GetFactories(), append_info, fac_to_str);

	if (ei) {
		info.push_back(wxString::Format(_("\nScript info:\nName: %s\nDescription: %s\nAuthor: %s\nVersion: %s\nFull path: %s\nState: %s\n\nFeatures provided by script:"),
			ei->script->GetName(),
			ei->script->GetDescription(),
			ei->script->GetAuthor(),
			ei->script->GetVersion(),
			ei->script->GetFilename(),
			ei->script->GetLoadedState() ? _("Correctly loaded") : _("Failed to load")));

		transform(ei->script->GetMacros(), append_info, std::bind(cmd_to_str, _1, context));
		transform(ei->script->GetFilters(), append_info, filt_to_str);
		transform(ei->script->GetFormats(), append_info, form_to_str);
	}

	wxMessageBox(wxJoin(info, '\n', 0), _("Automation Script Info"));
}

void DialogAutomation::OnReloadAutoload(wxCommandEvent &)
{
	global_manager->Reload();
}
