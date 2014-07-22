// Copyright (c) 2006, 2007, Niels Martin Hansen
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

/// @file auto4_lua.cpp
/// @brief Lua 5.1-based scripting engine
/// @ingroup scripting
///

#include "auto4_lua.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_info.h"
#include "ass_style.h"
#include "async_video_provider.h"
#include "auto4_lua_factory.h"
#include "command/command.h"
#include "compat.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "project.h"
#include "selection_controller.h"
#include "subs_controller.h"
#include "video_controller.h"
#include "utils.h"

#include <libaegisub/format.h>
#include <libaegisub/lua/modules.h>
#include <libaegisub/lua/script_reader.h>
#include <libaegisub/lua/utils.h>
#include <libaegisub/make_unique.h>
#include <libaegisub/path.h>

#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/scope_exit.hpp>
#include <cassert>
#include <mutex>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/msgdlg.h>

using namespace agi::lua;
using namespace Automation4;

namespace {
	wxString get_wxstring(lua_State *L, int idx)
	{
		return wxString::FromUTF8(lua_tostring(L, idx));
	}

	wxString check_wxstring(lua_State *L, int idx)
	{
		return to_wx(check_string(L, idx));
	}

	void set_context(lua_State *L, const agi::Context *c)
	{
		// Explicit cast is needed to discard the const
		push_value(L, (void *)c);
		lua_setfield(L, LUA_REGISTRYINDEX, "project_context");
	}

	const agi::Context *get_context(lua_State *L)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, "project_context");
		if (!lua_islightuserdata(L, -1)) {
			lua_pop(L, 1);
			return nullptr;
		}
		const agi::Context * c = static_cast<const agi::Context *>(lua_touserdata(L, -1));
		lua_pop(L, 1);
		return c;
	}

	int get_file_name(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		if (c && !c->subsController->Filename().empty())
			push_value(L, c->subsController->Filename().filename());
		else
			lua_pushnil(L);
		return 1;
	}

	int get_translation(lua_State *L)
	{
		wxString str(check_wxstring(L, 1));
		push_value(L, _(str).utf8_str());
		return 1;
	}

	int clipboard_get(lua_State *L)
	{
		std::string data = GetClipboard();
		if (data.empty())
			lua_pushnil(L);
		else
			push_value(L, data);
		return 1;
	}

	int clipboard_set(lua_State *L)
	{
		std::string str(check_string(L, 1));

		bool succeeded = false;

#if wxUSE_OLE
		// OLE needs to be initialized on each thread that wants to write to
		// the clipboard, which wx does not handle automatically
		wxClipboard cb;
#else
		wxClipboard &cb = *wxTheClipboard;
#endif
		if (cb.Open()) {
			succeeded = cb.SetData(new wxTextDataObject(to_wx(str)));
			cb.Close();
			cb.Flush();
		}

		lua_pushboolean(L, succeeded);
		return 1;
	}

	int clipboard_init(lua_State *L)
	{
		lua_createtable(L, 0, 2);
		set_field<clipboard_get>(L, "get");
		set_field<clipboard_set>(L, "set");
		return 1;
	}

	int frame_from_ms(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		int ms = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (c && c->project->Timecodes().IsLoaded())
			push_value(L, c->videoController->FrameAtTime(ms, agi::vfr::START));
		else
			lua_pushnil(L);

		return 1;
	}

	int ms_from_frame(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		int frame = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (c && c->project->Timecodes().IsLoaded())
			push_value(L, c->videoController->TimeAtFrame(frame, agi::vfr::START));
		else
			lua_pushnil(L);
		return 1;
	}

	int video_size(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		if (c && c->project->VideoProvider()) {
			auto provider = c->project->VideoProvider();
			push_value(L, provider->GetWidth());
			push_value(L, provider->GetHeight());
			push_value(L, c->videoController->GetAspectRatioValue());
			push_value(L, (int)c->videoController->GetAspectRatioType());
			return 4;
		}
		else {
			lua_pushnil(L);
			return 1;
		}
	}

	int get_keyframes(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		if (c)
			push_value(L, c->project->Keyframes());
		else
			lua_pushnil(L);
		return 1;
	}

	int decode_path(lua_State *L)
	{
		std::string path = check_string(L, 1);
		lua_pop(L, 1);
		push_value(L, config::path->Decode(path));
		return 1;
	}

	int cancel_script(lua_State *L)
	{
		lua_pushnil(L);
		throw error_tag();
	}

	bool lua_text_textents(AssEntry *entry, const char *str, double *out)
	{
		auto st = dynamic_cast<AssStyle*>(entry);
		if (!st) return false;
		return Automation4::CalculateTextExtents(st, str, out[0], out[1], out[2], out[3]);
	}

	int project_properties(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		if (!c)
			lua_pushnil(L);
		else {
			lua_createtable(L, 0, 14);
#define PUSH_FIELD(name) set_field(L, #name, c->ass->Properties.name)
			PUSH_FIELD(automation_scripts);
			PUSH_FIELD(export_filters);
			PUSH_FIELD(export_encoding);
			PUSH_FIELD(style_storage);
			PUSH_FIELD(video_zoom);
			PUSH_FIELD(ar_value);
			PUSH_FIELD(scroll_position);
			PUSH_FIELD(active_row);
			PUSH_FIELD(ar_mode);
			PUSH_FIELD(video_position);
#undef PUSH_FIELD
			set_field(L, "audio_file", config::path->MakeAbsolute(c->ass->Properties.audio_file, "?script"));
			set_field(L, "video_file", config::path->MakeAbsolute(c->ass->Properties.video_file, "?script"));
			set_field(L, "timecodes_file", config::path->MakeAbsolute(c->ass->Properties.timecodes_file, "?script"));
			set_field(L, "keyframes_file", config::path->MakeAbsolute(c->ass->Properties.keyframes_file, "?script"));
		}
		return 1;
	}

	struct LuaFeature {
		lua_State *L;

		void GetFeatureFunction(int id) const
		{
			lua_getfield(L, LUA_REGISTRYINDEX, "aegisub function table");
			lua_rawgeti(L, -1, id);
			lua_remove(L, -2);
			assert(lua_isfunction(L, -1));
		}
	};

	/// Run a lua function on a background thread
	/// @param L Lua state
	/// @param nargs Number of arguments the function takes
	/// @param nresults Number of values the function returns
	/// @param title Title to use for the progress dialog
	/// @param parent Parent window for the progress dialog
	/// @param can_open_config Can the function open its own dialogs?
	/// @throws agi::UserCancelException if the function fails to run to completion (either due to cancelling or errors)
	void LuaThreadedCall(lua_State *L, int nargs, int nresults, std::string const& title, wxWindow *parent, bool can_open_config);

	class LuaScript;

	class LuaCommand final : public cmd::Command {
		LuaScript *script;
		std::string cmd_name;
		wxString display;
		wxString help;
		int cmd_type = cmd::COMMAND_NORMAL;
		int run;
		int validate;
		int is_active;

	public:
		LuaCommand(LuaScript *script, const char *display, const char *help, int run, int validate, int is_active);
		~LuaCommand();

		const char* name() const override { return cmd_name.c_str(); }
		wxString StrMenu(const agi::Context *) const override { return display; }
		wxString StrDisplay(const agi::Context *) const override { return display; }
		wxString StrHelp() const override { return help; }

		int Type() const override { return cmd_type; }

		void operator()(agi::Context *c) override;
		bool Validate(const agi::Context *c) override;
		virtual bool IsActive(const agi::Context *c) override;
	};

	class LuaExportFilter final : public ExportFilter, private LuaFeature {
		LuaScript *script;
		LuaDialog *config_dialog = nullptr;
		int run;
		int config;

	protected:
		std::unique_ptr<ScriptDialog> GenerateConfigDialog(wxWindow *parent, agi::Context *c) override;

	public:
		LuaExportFilter(LuaScript *script);
		static int LuaRegister(lua_State *L);

		void ProcessSubs(AssFile *subs, wxWindow *export_dialog) override;
	};

	class LuaScript final : public Script {
		lua_State *L = nullptr;
		const agi::Context *context = nullptr;
		agi::Context *mutable_context = nullptr;
		LuaAssFile *laf = nullptr;

		std::string name;
		std::string description;
		std::string author;
		std::string version;

		std::vector<cmd::Command*> macros;
		std::vector<std::unique_ptr<ExportFilter>> filters;

		/// load script and create internal structures etc.
		void Create();
		/// destroy internal structures, unreg features and delete environment
		void Destroy();

		static int LoadFile(lua_State *L) {
			return 0;
		}

		void SetProperties(const char *name, const char *description, const char *author, const char *version) {
			this->name = name;
			this->description = description;
			this->author = author;
			this->version = version;

			if (this->name.empty())
				this->name = GetPrettyFilename().string();
		}

		void RegisterCommand(const char *display, const char *help, int run, int validate, int active) {

		}

	public:
		LuaScript(agi::fs::path const& filename)
		: Script(filename)
		{
			Create();
		}
		~LuaScript() { Destroy(); }

		void SetContext(agi::Context *c, bool allow_undo)
		{
			assert(!laf);
			context = mutable_context = c;
			laf = new Automation4::LuaAssFile(L, c->ass, true, allow_undo);
		}

		void SetContext(const agi::Context *c)
		{
			assert(!laf);
			context = c;
			laf = new Automation4::LuaAssFile(L, c->ass, false, false);
		}

		void ClearContext()
		{
			laf->ProcessingComplete();
			context = mutable_context = nullptr;
		}

		void PushFeatureFunction(int id);

		agi::Context *GetMutableContext() { return mutable_context; }
		const agi::Context *GetContext() { return context; }
		lua_State *GetState() { return L; }

		// Script implementation
		void Reload() override { Create(); }
		std::string GetName() const override { return name; }
		std::string GetDescription() const override { return description; }
		std::string GetAuthor() const override { return author; }
		std::string GetVersion() const override { return version; }
		bool GetLoadedState() const override { return L != nullptr; }

		std::vector<cmd::Command*> GetMacros() const override { return macros; }
		std::vector<ExportFilter*> GetFilters() const override;
	};


	void LuaScript::Create()
	{
		Destroy();

		name = GetPrettyFilename().string();

		// create lua environment
		L = lua_open();
		if (!L) return;

		bool loaded = false;
		BOOST_SCOPE_EXIT_ALL(&) { if (!loaded) Destroy(); };
		LuaStackcheck stackcheck(L);

		// register standard libs
		preload_modules(L);
		stackcheck.check_stack(0);

		// Replace the default lua module loader with our unicode compatible
		// one and set the module search path
		if (!Install(L, include_path)) {
			description = get_string_or_default(L, 1);
			lua_pop(L, 1);
			return;
		}
		stackcheck.check_stack(0);

		// Load the Lua implementation of the auto4 env
		lua_getglobal(L, "require");
		push_value(L, "aegisub.auto4");
		if (lua_pcall(L, 2, 1, 0)) {
			description = agi::format("Error initializing auto4 environment: %s", get_string_or_default(L, -1));
			lua_pop(L, 1);
			return;
		}

		// Leaves a function on the stack
		stackcheck.check_stack(1);

		// Set up the stuff the auto4 framework needs
		push_value(L, GetFilename());
		push_value(L, this);

		lua_createtable(L, 0, 13);
		set_field<LuaCommand::LuaRegister>(L, "register_macro");
		set_field<LuaExportFilter::LuaRegister>(L, "register_filter");
		set_field<lua_text_textents>(L, "text_extents");
		set_field<frame_from_ms>(L, "frame_from_ms");
		set_field<ms_from_frame>(L, "ms_from_frame");
		set_field<video_size>(L, "video_size");
		set_field<get_keyframes>(L, "keyframes");
		set_field<decode_path>(L, "decode_path");
		set_field<cancel_script>(L, "cancel");
		set_field(L, "lua_automation_version", 4);
		set_field<clipboard_init>(L, "__init_clipboard");
		set_field<get_file_name>(L, "file_name");
		set_field<get_translation>(L, "gettext");
		set_field<project_properties>(L, "project_properties");

		// And load/run the user's script
		if (lua_pcall(L, 3, 0, 0)) {
			// Should be an error message on top of the stack if it fails
			description = agi::format("Error initialising Lua script \"%s\":\n\n%s", GetPrettyFilename().string(), get_string_or_default(L, -1));
			lua_pop(L, 1);
			return;
		}
		stackcheck.check_stack(0);

		// if we got this far, the script should be ready
		loaded = true;
	}

	void LuaScript::Destroy()
	{
		// Assume the script object is clean if there's no Lua state
		if (!L) return;

		// loops backwards because commands remove themselves from macros when
		// they're unregistered
		for (int i = macros.size() - 1; i >= 0; --i)
			cmd::unreg(macros[i]->name());

		filters.clear();

		lua_close(L);
		L = nullptr;
	}

	std::vector<ExportFilter*> LuaScript::GetFilters() const
	{
		std::vector<ExportFilter *> ret;
		ret.reserve(filters.size());
		for (auto& filter : filters) ret.push_back(filter.get());
		return ret;
	}

	void LuaScript::RegisterCommand(LuaCommand *command)
	{
		static std::mutex mutex;
		auto command = agi::make_unique<LuaCommand>(L);
		{
			std::lock_guard<std::mutex> lock(mutex);
			cmd::reg(std::move(command));
		}

		for (auto macro : macros) {
			if (macro->name() == command->name()) {
				error(L, "A macro named '%s' is already defined in script '%s'",
					command->StrDisplay(nullptr).utf8_str().data(), name.c_str());
			}
		}
		macros.push_back(command);
	}

	void LuaScript::UnregisterCommand(LuaCommand *command)
	{
		macros.erase(remove(macros.begin(), macros.end(), command), macros.end());
	}

	void LuaScript::RegisterFilter(LuaExportFilter *filter)
	{
		filters.emplace_back(filter);
	}

	void LuaThreadedCall(lua_State *L, int nargs, int nresults, std::string const& title, wxWindow *parent, bool can_open_config)
	{
		bool failed = false;
		BackgroundScriptRunner bsr(parent, title);
		bsr.Run([&](ProgressSink *ps) {
			LuaProgressSink lps(L, ps, can_open_config);

			// Insert our error handler under the function to call
			lua_pushcclosure(L, add_stack_trace, 0);
			lua_insert(L, -nargs - 2);

			if (lua_pcall(L, nargs, nresults, -nargs - 2)) {
				if (!lua_isnil(L, -1)) {
					// if the call failed, log the error here
					ps->Log("\n\nLua reported a runtime error:\n");
					ps->Log(get_string_or_default(L, -1));
				}
				lua_pop(L, 2);
				failed = true;
			}
			else
				lua_remove(L, -nresults - 1);

			lua_gc(L, LUA_GCCOLLECT, 0);
		});
		if (failed)
			throw agi::UserCancelException("Script threw an error");
	}

	LuaCommand::LuaCommand(LuaScript *script, const char *display, const char *help, int run, int validate, int is_active)
	: script{script}
	, display(wxString::FromUTF8(display))
	, help(wxString::FromUTF8(help))
	, run(run)
	, validate(validate)
	, is_active(is_active)
	{
		cmd_name = agi::format("automation/lua/%s/%s", script->GetFilename(), display);
		if (validate)  cmd_type |= cmd::COMMAND_VALIDATE;
		if (is_active) cmd_type |= cmd::COMMAND_TOGGLE;
	}

	static std::vector<int> selected_rows(const agi::Context *c)
	{
		auto const& sel = c->selectionController->GetSelectedSet();
		int offset = c->ass->Info.size() + c->ass->Styles.size();
		std::vector<int> rows;
		rows.reserve(sel.size());
		for (auto line : sel)
			rows.push_back(line->Row + offset + 1);
		sort(begin(rows), end(rows));
		return rows;
	}

	bool LuaCommand::Validate(const agi::Context *c)
	{
		if (!validate) return true;

		script->PushFeatureFunction(validate);
		script->SetContext(c);

		auto L = script->GetState();
		push_value(L, selected_rows(c));
		if (auto active_line = c->selectionController->GetActiveLine())
			push_value(L, active_line->Row + c->ass->Info.size() + c->ass->Styles.size() + 1);

		int err = lua_pcall(L, 3, 2, 0);
		script->ClearContext();

		if (err) {
			wxLogWarning("Runtime error in Lua macro validation function:\n%s", get_wxstring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		bool result = !!lua_toboolean(L, -2);

		wxString new_help_string(get_wxstring(L, -1));
		if (new_help_string.size()) {
			help = new_help_string;
			cmd_type |= cmd::COMMAND_DYNAMIC_HELP;
		}

		lua_pop(L, 2);

		return result;
	}

	void LuaCommand::operator()(agi::Context *c)
	{
		script->PushFeatureFunction(run);
		script->SetContext(c, true);

		int original_offset = c->ass->Info.size() + c->ass->Styles.size() + 1;
		auto original_sel = selected_rows(c);
		int original_active = 0;
		if (auto active_line = c->selectionController->GetActiveLine())
			original_active = active_line->Row + original_offset;

		auto L = script->GetState();
		push_value(L, original_sel);
		push_value(L, original_active);

		try {
			LuaThreadedCall(L, 3, 2, from_wx(StrDisplay(c)), c->parent, true);
			script->ClearContext();

			auto lines = subsobj->ProcessingComplete(StrDisplay(c));

			AssDialogue *active_line = nullptr;
			int active_idx = original_active;

			// Check for a new active row
			if (lua_isnumber(L, -1)) {
				active_idx = lua_tointeger(L, -1);
				if (active_idx < 1 || active_idx > (int)lines.size()) {
					wxLogError("Active row %d is out of bounds (must be 1-%u)", active_idx, lines.size());
					active_idx = original_active;
				}
			}

			stackcheck.check_stack(2);
			lua_pop(L, 1);

			// top of stack will be selected lines array, if any was returned
			if (lua_istable(L, -1)) {
				std::set<AssDialogue*> sel;
				lua_for_each(L, [&] {
					if (lua_isnumber(L, -1)) {
						int cur = lua_tointeger(L, -1);
						if (cur < 1 || cur > (int)lines.size()) {
							wxLogError("Selected row %d is out of bounds (must be 1-%u)", cur, lines.size());
							throw LuaForEachBreak();
						}

						auto diag = dynamic_cast<AssDialogue*>(lines[cur - 1]);
						if (!diag) {
							wxLogError("Selected row %d is not a dialogue line", cur);
							throw LuaForEachBreak();
						}

						sel.insert(diag);
						if (!active_line || active_idx == cur)
							active_line = diag;
					}
				});

				AssDialogue *new_active = c->selectionController->GetActiveLine();
				if (active_line && (active_idx > 0 || !sel.count(new_active)))
					new_active = active_line;
				c->selectionController->SetSelectionAndActive(std::move(sel), new_active);
			}
			else {
				lua_pop(L, 1);

				Selection new_sel;
				AssDialogue *new_active = nullptr;

				int prev = original_offset;
				auto it = c->ass->Events.begin();
				for (int row : original_sel) {
					while (row > prev && it != c->ass->Events.end()) {
						++prev;
						++it;
					}
					if (row != prev) break;
					new_sel.insert(&*it);
					if (row == original_active)
						new_active = &*it;
				}

				if (new_sel.empty() && !c->ass->Events.empty())
					new_sel.insert(&c->ass->Events.front());
				if (!new_sel.count(new_active))
					new_active = *new_sel.begin();
				c->selectionController->SetSelectionAndActive(std::move(new_sel), new_active);
			}

			stackcheck.check_stack(0);
		}
		catch (agi::UserCancelException const&) {
			subsobj->Cancel();
		}
		stackcheck.check_stack(0);
	}

	bool LuaCommand::IsActive(const agi::Context *c)
	{
		if (!is_active) return false;

		script->PushFeatureFunction(is_active);
		script->SetContext(c);

		auto L = script->GetState();
		push_value(L, selected_rows(c));
		if (auto active_line = c->selectionController->GetActiveLine())
			push_value(L, active_line->Row + c->ass->Info.size() + c->ass->Styles.size() + 1);

		int err = lua_pcall(L, 3, 1, 0);
		script->ClearContext();

		bool result = false;
		if (err)
			wxLogWarning("Runtime error in Lua macro IsActive function:\n%s", get_wxstring(L, -1));
		else
			result = !!lua_toboolean(L, -1);
		lua_pop(L, 1);

		return result;
	}

	// LuaFeatureFilter
	LuaExportFilter::LuaExportFilter(LuaScript *script, const char *name, const char *help, int priority, int run, int config)
	: ExportFilter(name, help, priority)
	, script(script)
	, run(run)
	, config(config)
	{
	}

	int LuaExportFilter::LuaRegister(lua_State *L)
	{
		static std::mutex mutex;
		auto filter = agi::make_unique<LuaExportFilter>(L);
		{
			std::lock_guard<std::mutex> lock(mutex);
			AssExportFilterChain::Register(std::move(filter));
		}
		return 0;
	}

	void LuaExportFilter::ProcessSubs(AssFile *subs, wxWindow *export_dialog)
	{
		script->PushFeatureFunction(run);
//		script->SetContext(<#agi::Context *c#>, <#bool allow_undo#>)

		// config
		if (config_dialog) {
			int results_produced = config_dialog->LuaReadBack(L);
			assert(results_produced == 1);
			(void) results_produced;	// avoid warning on release builds
			// TODO, write back stored options here
		} else {
			// no config so put an empty table instead
			lua_newtable(L);
		}
		assert(lua_istable(L, -1));
		stackcheck.check_stack(3);

		try {
			LuaThreadedCall(L, 2, 0, GetName(), export_dialog, false);
			stackcheck.check_stack(0);
			subsobj->ProcessingComplete();
		}
		catch (agi::UserCancelException const&) {
			subsobj->Cancel();
			throw;
		}
	}

	std::unique_ptr<ScriptDialog> LuaExportFilter::GenerateConfigDialog(wxWindow *parent, agi::Context *c)
	{
		if (!config) return nullptr;

		script->PushFeatureFunction(config);
		script->SetContext(c, false);

		auto L = script->GetState();
		lua_newtable(L); // TODO: persist config stuff
		int err = lua_pcall(L, 2, 1, 0);
		script->ClearContext();

		if (err) {
			wxLogWarning("Runtime error in Lua config dialog function:\n%s", get_wxstring(L, -1));
			lua_pop(L, 1); // remove error message
		} else {
			// Create config dialogue from table on top of stack
			config_dialog = new LuaDialog(L, false);
		}

		return std::unique_ptr<ScriptDialog>{config_dialog};
	}
}

namespace Automation4 {
	LuaScriptFactory::LuaScriptFactory() : ScriptFactory("Lua", "*.lua,*.moon") { }

	std::unique_ptr<Script> LuaScriptFactory::Produce(agi::fs::path const& filename) const
	{
		if (agi::fs::HasExtension(filename, "lua") || agi::fs::HasExtension(filename, "moon"))
			return agi::make_unique<LuaScript>(filename);
		return nullptr;
	}
}
