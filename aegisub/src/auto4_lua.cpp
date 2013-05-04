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

#include "config.h"

#include "auto4_lua.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "auto4_lua_factory.h"
#include "compat.h"
#include "include/aegisub/context.h"
#include "main.h"
#include "options.h"
#include "selection_controller.h"
#include "subs_controller.h"
#include "video_context.h"
#include "utils.h"

#include <libaegisub/access.h>
#include <libaegisub/lua/modules.h>
#include <libaegisub/lua/script_reader.h>
#include <libaegisub/lua/utils.h>
#include <libaegisub/path.h>
#include <libaegisub/scoped_ptr.h>

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>
#include <cassert>
#include <cstdint>

#include <wx/clipbrd.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/window.h>

namespace {
	using namespace agi::lua;

	wxString get_wxstring(lua_State *L, int idx)
	{
		return wxString::FromUTF8(lua_tostring(L, idx));
	}

	wxString check_wxstring(lua_State *L, int idx)
	{
		return wxString::FromUTF8(luaL_checkstring(L, idx));
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
			return 0;
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
		std::string str(luaL_checkstring(L, 1));

		bool succeeded = false;

#if wxUSE_OLE
		// OLE needs to be initialized on each thread that wants to write to
		// the clipboard, which wx does not handle automatically
		wxClipboard cb;
		wxClipboard *theCB = &cb;
#else
		wxClipboard *theCB = wxTheClipboard;
#endif
		if (theCB->Open()) {
			succeeded = theCB->SetData(new wxTextDataObject(to_wx(str)));
			theCB->Close();
			theCB->Flush();
		}

		lua_pushboolean(L, succeeded);
		return 1;
	}

	int clipboard_init(lua_State *L)
	{
		lua_newtable(L);
		set_field(L, "get", clipboard_get);
		set_field(L, "set", clipboard_set);
		return 1;
	}

	struct table_pairs {
		static void init(lua_State *L) { lua_pushnil(L); }
		static int next(lua_State *L) {
			luaL_checktype(L, 1, LUA_TTABLE);
			lua_settop(L, 2);
			if (lua_next(L, 1))
				return 2;
			lua_pushnil(L);
			return 1;
		}
		static const char *method() { return "__pairs"; }
	};

	struct table_ipairs {
		static void init(lua_State *L) { push_value(L, 0); }
		static int next(lua_State *L) {
			luaL_checktype(L, 1, LUA_TTABLE);
			int i = luaL_checkint(L, 2) + 1;
			lua_pushinteger(L, i);
			lua_rawgeti(L, 1, i);
			return lua_isnil(L, -1) ? 0 : 2;
		}
		static const char *method() { return "__ipairs"; }
	};

	template<typename TableIter>
	int pairs(lua_State *L) {
		// If the metamethod is defined, call it instead
		if (luaL_getmetafield(L, 1, TableIter::method())) {
			lua_pushvalue(L, 1);
			lua_call(L, 1, 3);
			return 3;
		}

		// No metamethod, so use the table iterators
		luaL_checktype(L, 1, LUA_TTABLE);
		lua_pushcfunction(L, &TableIter::next);
		lua_pushvalue(L, 1);
		TableIter::init(L);
		return 3;
	}

	int frame_from_ms(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		int ms = lua_tointeger(L, -1);
		lua_pop(L, 1);
		if (c && c->videoController->TimecodesLoaded())
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
		if (c && c->videoController->TimecodesLoaded())
			push_value(L, c->videoController->TimeAtFrame(frame, agi::vfr::START));
		else
			lua_pushnil(L);
		return 1;
	}

	int video_size(lua_State *L)
	{
		const agi::Context *c = get_context(L);
		if (c && c->videoController->IsLoaded()) {
			push_value(L, c->videoController->GetWidth());
			push_value(L, c->videoController->GetHeight());
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
		if (!c) {
			lua_pushnil(L);
			return 1;
		}

		std::vector<int> const& kf = c->videoController->GetKeyFrames();

		lua_newtable(L);
		for (size_t i = 0; i < kf.size(); ++i) {
			push_value(L, kf[i]);
			lua_rawseti(L, -2, i);
		}

		return 1;
	}

	int decode_path(lua_State *L)
	{
		std::string path = luaL_checkstring(L, 1);
		lua_pop(L, 1);
		push_value(L, config::path->Decode(path));
		return 1;
	}

	int cancel_script(lua_State *L)
	{
		lua_pushnil(L);
		return lua_error(L);
	}

	int lua_text_textents(lua_State *L)
	{
		luaL_argcheck(L, lua_istable(L, 1), 1, "");
		luaL_argcheck(L, lua_isstring(L, 2), 2, "");

		lua_pushvalue(L, 1);
		agi::scoped_ptr<AssEntry> et(Automation4::LuaAssFile::LuaToAssEntry(L));
		AssStyle *st = dynamic_cast<AssStyle*>(et.get());
		lua_pop(L, 1);
		if (!st)
			return luaL_error(L, "Not a style entry");

		double width, height, descent, extlead;
		if (!Automation4::CalculateTextExtents(st, luaL_checkstring(L, 2), width, height, descent, extlead))
			return luaL_error(L, "Some internal error occurred calculating text_extents");

		push_value(L, width);
		push_value(L, height);
		push_value(L, descent);
		push_value(L, extlead);
		return 4;
	}
}

namespace Automation4 {
	class LuaScript : public Script {
		lua_State *L;

		std::string name;
		std::string description;
		std::string author;
		std::string version;

		std::vector<cmd::Command*> macros;
		std::vector<ExportFilter*> filters;

		/// load script and create internal structures etc.
		void Create();
		/// destroy internal structures, unreg features and delete environment
		void Destroy();

		static int LuaInclude(lua_State *L);
		static int LuaModuleLoader(lua_State *L);

	public:
		LuaScript(agi::fs::path const& filename);
		~LuaScript() { Destroy(); }

		void RegisterCommand(LuaCommand *command);
		void UnregisterCommand(LuaCommand *command);
		void RegisterFilter(LuaExportFilter *filter);

		static LuaScript* GetScriptObject(lua_State *L);

		// Script implementation
		void Reload() { Create(); }

		std::string GetName() const { return name; }
		std::string GetDescription() const { return description; }
		std::string GetAuthor() const { return author; }
		std::string GetVersion() const { return version; }
		bool GetLoadedState() const { return L != 0; }

		std::vector<cmd::Command*> GetMacros() const { return macros; }
		std::vector<ExportFilter*> GetFilters() const { return filters; }
		std::vector<SubtitleFormat*> GetFormats() const { return std::vector<SubtitleFormat*>(); }
	};

	LuaScript::LuaScript(agi::fs::path const& filename)
	: Script(filename)
	, L(nullptr)
	{
		Create();
	}

	void LuaScript::Create()
	{
		Destroy();

		try {
			// create lua environment
			L = lua_open();
			LuaStackcheck _stackcheck(L);

			// register standard libs
			push_value(L, luaopen_base); lua_call(L, 0, 0);
			push_value(L, luaopen_io); lua_call(L, 0, 0);
			push_value(L, luaopen_math); lua_call(L, 0, 0);
			push_value(L, luaopen_os); lua_call(L, 0, 0);
			push_value(L, luaopen_package); lua_call(L, 0, 0);
			push_value(L, luaopen_string); lua_call(L, 0, 0);
			push_value(L, luaopen_table); lua_call(L, 0, 0);
			_stackcheck.check_stack(0);

			// dofile and loadfile are replaced with include
			lua_pushnil(L);
			lua_setglobal(L, "dofile");
			lua_pushnil(L);
			lua_setglobal(L, "loadfile");
			push_value(L, LuaInclude);
			lua_setglobal(L, "include");

			// replace pairs and ipairs with lua 5.2-style versions
			push_value(L, &pairs<table_pairs>);
			lua_setglobal(L, "pairs");
			push_value(L, &pairs<table_ipairs>);
			lua_setglobal(L, "ipairs");

			// add include_path to the module load path
			lua_getglobal(L, "package");
			push_value(L, "path");
			push_value(L, "path");
			lua_gettable(L, -3);

			for (auto const& path : include_path) {
				lua_pushfstring(L, ";%s/?.lua;%s/?/init.lua", path.string().c_str(), path.string().c_str());
				lua_concat(L, 2);
			}

			lua_settable(L, -3);

			// Replace the default lua module loader with our unicode compatible one
			lua_getfield(L, -1, "loaders");
			push_value(L, LuaModuleLoader);
			lua_rawseti(L, -2, 2);
			lua_pop(L, 2);
			_stackcheck.check_stack(0);

			// prepare stuff in the registry

			// store the script's filename
			push_value(L, GetFilename().stem());
			lua_setfield(L, LUA_REGISTRYINDEX, "filename");
			_stackcheck.check_stack(0);

			// reference to the script object
			push_value(L, this);
			lua_setfield(L, LUA_REGISTRYINDEX, "aegisub");
			_stackcheck.check_stack(0);

			// make "aegisub" table
			lua_pushstring(L, "aegisub");
			lua_newtable(L);

			set_field(L, "register_macro", LuaCommand::LuaRegister);
			set_field(L, "register_filter", LuaExportFilter::LuaRegister);
			set_field(L, "text_extents", lua_text_textents);
			set_field(L, "frame_from_ms", frame_from_ms);
			set_field(L, "ms_from_frame", ms_from_frame);
			set_field(L, "video_size", video_size);
			set_field(L, "keyframes", get_keyframes);
			set_field(L, "decode_path", decode_path);
			set_field(L, "cancel", cancel_script);
			set_field(L, "lua_automation_version", 4);
			set_field(L, "__init_clipboard", clipboard_init);
			set_field(L, "file_name", get_file_name);
			set_field(L, "gettext", get_translation);

			// store aegisub table to globals
			lua_settable(L, LUA_GLOBALSINDEX);
			_stackcheck.check_stack(0);

			// Preload packaged binary modules
			preload_modules(L);
			_stackcheck.check_stack(0);

			// load user script
			if (!LoadFile(L, GetFilename())) {
				std::string err = str(boost::format("Error loading Lua script \"%s\":\n\n%s") % GetPrettyFilename().string() % get_string_or_default(L, -1));
				lua_pop(L, 1);
				throw ScriptLoadError(err);
			}
			_stackcheck.check_stack(1);

			// and execute it
			// this is where features are registered
			// don't thread this, as there's no point in it and it seems to break on wx 2.8.3, for some reason
			if (lua_pcall(L, 0, 0, 0)) {
				// error occurred, assumed to be on top of Lua stack
				std::string err = str(boost::format("Error initialising Lua script \"%s\":\n\n%s") % GetPrettyFilename().string() % get_string_or_default(L, -1));
				lua_pop(L, 1);
				throw ScriptLoadError(err);
			}
			_stackcheck.check_stack(0);

			lua_getglobal(L, "version");
			if (lua_isnumber(L, -1) && lua_tointeger(L, -1) == 3) {
				lua_pop(L, 1); // just to avoid tripping the stackcheck in debug
				throw ScriptLoadError("Attempted to load an Automation 3 script as an Automation 4 Lua script. Automation 3 is no longer supported.");
			}

			name = get_global_string(L, "script_name");
			description = get_global_string(L, "script_description");
			author = get_global_string(L, "script_author");
			version = get_global_string(L, "script_version");

			if (name.empty())
				name = GetPrettyFilename().string();

			lua_pop(L, 1);
			// if we got this far, the script should be ready
			_stackcheck.check_stack(0);

		}
		catch (agi::Exception const& e) {
			Destroy();
			name = GetPrettyFilename().string();
			description = e.GetChainedMessage();
		}
	}

	void LuaScript::Destroy()
	{
		// Assume the script object is clean if there's no Lua state
		if (!L) return;

		// loops backwards because commands remove themselves from macros when
		// they're unregistered
		for (int i = macros.size() - 1; i >= 0; --i)
			cmd::unreg(macros[i]->name());

		delete_clear(filters);

		lua_close(L);
		L = nullptr;
	}

	void LuaScript::RegisterCommand(LuaCommand *command)
	{
		for (auto macro : macros) {
			if (macro->name() == command->name()) {
				luaL_error(L,
					"A macro named '%s' is already defined in script '%s'",
					command->StrDisplay(0).utf8_str().data(), name.c_str());
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
		filters.push_back(filter);
	}

	LuaScript* LuaScript::GetScriptObject(lua_State *L)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, "aegisub");
		void *ptr = lua_touserdata(L, -1);
		lua_pop(L, 1);
		return (LuaScript*)ptr;
	}

	/// @brief Module loader which uses our include rather than Lua's, for unicode file support
	/// @param L The Lua state
	/// @return Always 1 per loader_Lua?
	int LuaScript::LuaModuleLoader(lua_State *L)
	{
		int pretop = lua_gettop(L);
		std::string module(luaL_checkstring(L, -1));
		boost::replace_all(module, ".", LUA_DIRSEP);

		// Get the lua package include path (which the user may have modified)
		lua_getglobal(L, "package");
		lua_getfield(L, -1, "path");
		std::string package_paths(luaL_checkstring(L, -1));
		lua_pop(L, 2);

		boost::char_separator<char> sep(";");
		for (auto filename : boost::tokenizer<boost::char_separator<char>>(package_paths, sep)) {
			boost::replace_all(filename, "?", module);

			// If there's a .moon file at that path, load it instead of the
			// .lua file
			agi::fs::path path = filename;
			if (agi::fs::HasExtension(path, "lua")) {
				agi::fs::path moonpath = path;
				moonpath.replace_extension("moon");
				if (agi::fs::FileExists(moonpath))
					path = moonpath;
			}

			try {
				if (!LoadFile(L, path))
					return luaL_error(L, "Error loading Lua module \"%s\":\n\n%s", path.string().c_str(), luaL_checkstring(L, -1));
				break;
			}
			catch (agi::fs::FileNotFound const&) {
				// Not an error so swallow and continue on
			}
			catch (agi::fs::NotAFile const&) {
				// Not an error so swallow and continue on
			}
			catch (agi::Exception const& e) {
				return luaL_error(L, "Error loading Lua module \"%s\":\n\n%s", path.string().c_str(), e.GetChainedMessage().c_str());
			}
		}

		return lua_gettop(L) - pretop;
	}

	int LuaScript::LuaInclude(lua_State *L)
	{
		const LuaScript *s = GetScriptObject(L);

		const std::string filename(luaL_checkstring(L, 1));
		agi::fs::path filepath;

		// Relative or absolute path
		if (!boost::all(filename, !boost::is_any_of("/\\")))
			filepath = s->GetFilename().parent_path()/filename;
		else { // Plain filename
			for (auto const& dir : s->include_path) {
				filepath = dir/filename;
				if (agi::fs::FileExists(filepath))
					break;
			}
		}

		if (!agi::fs::FileExists(filepath))
			return luaL_error(L, "Lua include not found: %s", filename.c_str());

		if (!LoadFile(L, filepath))
			return luaL_error(L, "Error loading Lua include \"%s\":\n\n%s", filename.c_str(), luaL_checkstring(L, -1));

		int pretop = lua_gettop(L) - 1; // don't count the function value itself
		lua_call(L, 0, LUA_MULTRET);
		return lua_gettop(L) - pretop;
	}

	void LuaThreadedCall(lua_State *L, int nargs, int nresults, std::string const& title, wxWindow *parent, bool can_open_config)
	{
		bool failed = false;
		BackgroundScriptRunner bsr(parent, title);
		bsr.Run([&](ProgressSink *ps) {
			LuaProgressSink lps(L, ps, can_open_config);

			if (lua_pcall(L, nargs, nresults, 0)) {
				if (!lua_isnil(L, -1)) {
					// if the call failed, log the error here
					ps->Log("\n\nLua reported a runtime error:\n");
					ps->Log(get_string_or_default(L, -1));
				}
				lua_pop(L, 1);
				failed = true;
			}

			lua_gc(L, LUA_GCCOLLECT, 0);
		});
		if (failed)
			throw agi::UserCancelException("Script threw an error");
	}

	// LuaFeature
	LuaFeature::LuaFeature(lua_State *L)
	: myid(0)
	, L(L)
	{
	}

	void LuaFeature::RegisterFeature()
	{
		myid = luaL_ref(L, LUA_REGISTRYINDEX);
	}

	void LuaFeature::UnregisterFeature()
	{
		luaL_unref(L, LUA_REGISTRYINDEX, myid);
	}

	void LuaFeature::GetFeatureFunction(const char *function) const
	{
		// get this feature's function pointers
		lua_rawgeti(L, LUA_REGISTRYINDEX, myid);
		// get pointer for validation function
		push_value(L, function);
		lua_rawget(L, -2);
		// remove the function table
		lua_remove(L, -2);
		assert(lua_isfunction(L, -1));
	}

	// LuaFeatureMacro
	int LuaCommand::LuaRegister(lua_State *L)
	{
		cmd::reg(new LuaCommand(L));
		return 0;
	}

	LuaCommand::LuaCommand(lua_State *L)
	: LuaFeature(L)
	, display(check_wxstring(L, 1))
	, help(get_wxstring(L, 2))
	, cmd_type(cmd::COMMAND_NORMAL)
	{
		lua_getfield(L, LUA_REGISTRYINDEX, "filename");
		cmd_name = str(boost::format("automation/lua/%s/%s") % luaL_checkstring(L, -1) % luaL_checkstring(L, 1));

		if (!lua_isfunction(L, 3))
			luaL_error(L, "The macro processing function must be a function");

		if (lua_isfunction(L, 4))
			cmd_type |= cmd::COMMAND_VALIDATE;

		if (lua_isfunction(L, 5))
			cmd_type |= cmd::COMMAND_TOGGLE;

		// new table for containing the functions for this feature
		lua_newtable(L);

		// store processing function
		push_value(L, "run");
		lua_pushvalue(L, 3);
		lua_rawset(L, -3);

		// store validation function
		push_value(L, "validate");
		lua_pushvalue(L, 4);
		lua_rawset(L, -3);

		// store active function
		push_value(L, "isactive");
		lua_pushvalue(L, 5);
		lua_rawset(L, -3);

		// store the table in the registry
		RegisterFeature();

		LuaScript::GetScriptObject(L)->RegisterCommand(this);
	}

	LuaCommand::~LuaCommand()
	{
		UnregisterFeature();
		LuaScript::GetScriptObject(L)->UnregisterCommand(this);
	}

	static int transform_selection(lua_State *L, const agi::Context *c)
	{
		SubtitleSelection const& sel = c->selectionController->GetSelectedSet();
		AssDialogue *active_line = c->selectionController->GetActiveLine();

		lua_newtable(L);
		int active_idx = -1;

		int row = 0;
		int idx = 1;
		for (auto& line : c->ass->Line) {
			++row;
			AssDialogue *diag = dynamic_cast<AssDialogue*>(&line);
			if (!diag) continue;

			if (diag == active_line) active_idx = row;
			if (sel.count(diag)) {
				push_value(L, row);
				lua_rawseti(L, -2, idx++);
			}
		}

		return active_idx;
	}

	bool LuaCommand::Validate(const agi::Context *c)
	{
		if (!(cmd_type & cmd::COMMAND_VALIDATE)) return true;

		set_context(L, c);

		GetFeatureFunction("validate");
		LuaAssFile *subsobj = new LuaAssFile(L, c->ass);
		push_value(L, transform_selection(L, c));

		int err = lua_pcall(L, 3, 2, 0);

		subsobj->ProcessingComplete();

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
		LuaStackcheck stackcheck(L);
		set_context(L, c);
		stackcheck.check_stack(0);

		GetFeatureFunction("run");
		LuaAssFile *subsobj = new LuaAssFile(L, c->ass, true, true);
		push_value(L, transform_selection(L, c));

		try {
			LuaThreadedCall(L, 3, 2, from_wx(StrDisplay(c)), c->parent, true);

			subsobj->ProcessingComplete(StrDisplay(c));

			AssDialogue *active_line = 0;
			int active_idx = 0;

			// Check for a new active row
			if (lua_isnumber(L, -1)) {
				active_idx = lua_tointeger(L, -1);
				if (active_idx < 1 || active_idx > (int)c->ass->Line.size()) {
					wxLogError("Active row %d is out of bounds (must be 1-%u)", active_idx, c->ass->Line.size());
					active_idx = 0;
				}
			}

			stackcheck.check_stack(2);
			lua_pop(L, 1);

			// top of stack will be selected lines array, if any was returned
			if (lua_istable(L, -1)) {
				std::set<AssDialogue*> sel;
				entryIter it = c->ass->Line.begin();
				int last_idx = 1;
				lua_pushnil(L);
				while (lua_next(L, -2)) {
					if (lua_isnumber(L, -1)) {
						int cur = lua_tointeger(L, -1);
						if (cur < 1 || cur > (int)c->ass->Line.size()) {
							wxLogError("Selected row %d is out of bounds (must be 1-%u)", cur, c->ass->Line.size());
							break;
						}

						advance(it, cur - last_idx);

						AssDialogue *diag = dynamic_cast<AssDialogue*>(&*it);
						if (!diag) {
							wxLogError("Selected row %d is not a dialogue line", cur);
							break;
						}

						sel.insert(diag);
						last_idx = cur;
						if (!active_line || active_idx == cur)
							active_line = diag;
					}
					lua_pop(L, 1);
				}

				AssDialogue *new_active = c->selectionController->GetActiveLine();
				if (active_line && (active_idx > 0 || !sel.count(new_active)))
					new_active = active_line;
				c->selectionController->SetSelectionAndActive(sel, new_active);
			}

			stackcheck.check_stack(1);
			lua_pop(L, 1);
		}
		catch (agi::UserCancelException const&) {
			subsobj->Cancel();
		}
		stackcheck.check_stack(0);
	}

	bool LuaCommand::IsActive(const agi::Context *c)
	{
		if (!(cmd_type & cmd::COMMAND_TOGGLE)) return false;

		LuaStackcheck stackcheck(L);

		set_context(L, c);
		stackcheck.check_stack(0);

		GetFeatureFunction("isactive");
		LuaAssFile *subsobj = new LuaAssFile(L, c->ass);
		push_value(L, transform_selection(L, c));

		int err = lua_pcall(L, 3, 1, 0);
		subsobj->ProcessingComplete();

		bool result = false;
		if (err)
			wxLogWarning("Runtime error in Lua macro IsActive function:\n%s", get_wxstring(L, -1));
		else
			result = !!lua_toboolean(L, -1);

		// clean up stack (result or error message)
		stackcheck.check_stack(1);
		lua_pop(L, 1);

		return result;
	}

	// LuaFeatureFilter
	LuaExportFilter::LuaExportFilter(lua_State *L)
	: ExportFilter(luaL_checkstring(L, 1), lua_tostring(L, 2), lua_tointeger(L, 3))
	, LuaFeature(L)
	{
		if (!lua_isfunction(L, 4))
			luaL_error(L, "The filter processing function must be a function");

		// new table for containing the functions for this feature
		lua_newtable(L);

		// store processing function
		push_value(L, "run");
		lua_pushvalue(L, 4);
		lua_rawset(L, -3);

		// store config function
		push_value(L, "config");
		lua_pushvalue(L, 5);
		has_config = lua_isfunction(L, -1);
		lua_rawset(L, -3);

		// store the table in the registry
		RegisterFeature();

		LuaScript::GetScriptObject(L)->RegisterFilter(this);
	}

	int LuaExportFilter::LuaRegister(lua_State *L)
	{
		(void)new LuaExportFilter(L);

		return 0;
	}

	void LuaExportFilter::ProcessSubs(AssFile *subs, wxWindow *export_dialog)
	{
		LuaStackcheck stackcheck(L);

		GetFeatureFunction("run");
		stackcheck.check_stack(1);

		// The entire point of an export filter is to modify the file, but
		// setting undo points makes no sense
		LuaAssFile *subsobj = new LuaAssFile(L, subs, true);
		assert(lua_isuserdata(L, -1));
		stackcheck.check_stack(2);

		// config
		if (has_config && config_dialog) {
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

	ScriptDialog* LuaExportFilter::GenerateConfigDialog(wxWindow *parent, agi::Context *c)
	{
		if (!has_config)
			return 0;

		set_context(L, c);

		GetFeatureFunction("config");

		// prepare function call
		LuaAssFile *subsobj = new LuaAssFile(L, c->ass);
		// stored options
		lua_newtable(L); // TODO, nothing for now

		// do call
		int err = lua_pcall(L, 2, 1, 0);
		subsobj->ProcessingComplete();

		if (err) {
			wxLogWarning("Runtime error in Lua config dialog function:\n%s", get_wxstring(L, -1));
			lua_pop(L, 1); // remove error message
		} else {
			// Create config dialogue from table on top of stack
			config_dialog = new LuaDialog(L, false);
		}

		return config_dialog;
	}

	LuaScriptFactory::LuaScriptFactory()
	: ScriptFactory("Lua", "*.lua,*.moon")
	{
		Register(this);
	}

	Script* LuaScriptFactory::Produce(agi::fs::path const& filename) const
	{
		if (agi::fs::HasExtension(filename, "lua") || agi::fs::HasExtension(filename, "moon"))
			return new LuaScript(filename);
		return nullptr;
	}
}
