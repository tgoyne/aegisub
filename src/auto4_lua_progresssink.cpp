// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "auto4_lua.h"

#include <libaegisub/lua/ffi.h>
#include <libaegisub/lua/utils.h>
#include <libaegisub/make_unique.h>

#include <wx/filedlg.h>

using namespace agi::lua;
using namespace Automation4;

namespace {
	void set_progress(LuaEnv *env, int progress) {
		env->progress_sink->SetProgress(progress, 100);
	}

	void set_task(LuaEnv *env, const char *msg) {
		env->progress_sink->SetMessage(msg);
	}

	void set_title(LuaEnv *env, const char *title) {
		env->progress_sink->SetTitle(title);
	}

	bool get_cancelled(LuaEnv *env) {
		return env->progress_sink->IsCancelled();
	}

	int get_trace_level(LuaEnv *env) {
		return env->progress_sink->GetTraceLevel();
	}

	void log(LuaEnv *env, const char *str) {
		env->progress_sink->Log(str);
	}

	char **open_dialog(LuaEnv *env, const char *msg, const char *dir, const char *file, const char *wildcard, bool multiple, bool must_exist) {
		int flags = wxFD_OPEN;
		if (multiple)
			flags |= wxFD_MULTIPLE;
		if (must_exist)
			flags |= wxFD_FILE_MUST_EXIST;

		wxFileDialog diag(env->progress_sink->GetParentWindow(), wxString::FromUTF8(msg), wxString::FromUTF8(dir),
			wxString::FromUTF8(file), wxString::FromUTF8(wildcard), flags);
		if (env->progress_sink->ShowDialog(&diag) == wxID_CANCEL)
			return nullptr;

		if (multiple) {
			wxArrayString files;
			diag.GetPaths(files);

			auto ret = static_cast<char **>(calloc(sizeof(char *), files.size() + 1));
			for (size_t i = 0; i < files.size(); ++i)
				ret[i] = strdup(files[i].utf8_str());
			return ret;
		}

		auto ret = static_cast<char **>(malloc(sizeof(char *)));
		ret[0] = strdup(diag.GetPath().utf8_str());
		return ret;
	}

	char *save_dialog(LuaEnv *env, const char *msg, const char *dir, const char *file, const char *wildcard, bool prompt_overwrite) {
		int flags = wxFD_SAVE;
		if (prompt_overwrite)
			flags |= wxFD_OVERWRITE_PROMPT;

		wxFileDialog diag(env->progress_sink->GetParentWindow(), wxString::FromUTF8(msg), wxString::FromUTF8(dir),
			wxString::FromUTF8(file), wxString::FromUTF8(wildcard), flags);
		if (env->progress_sink->ShowDialog(&diag) == wxID_CANCEL)
			return nullptr;
		return strdup(diag.GetPath().utf8_str());
	}

	bool has_progress_sink(LuaEnv *env) {
		return !!env->progress_sink;
	}

#define FN_AND_NAME(name) #name, name
	int init_progress_sink(lua_State *L) {
		auto env = LuaEnv::FromState(L);
		env->progress_sink = agi::make_unique<ProgressSink>();
		register_lib_table(L, {}, FN_AND_NAME(set_progress), FN_AND_NAME(set_task), FN_AND_NAME(set_title), FN_AND_NAME(get_cancelled), FN_AND_NAME(get_trace_level), FN_AND_NAME(log), FN_AND_NAME(open_dialog), FN_AND_NAME(save_dialog));
		return 1;
	}
}

namespace Automation4 {
	int LuaProgressSink::LuaDisplayDialog(lua_State *L)
	{
		ProgressSink *ps = GetObjPointer(L, lua_upvalueindex(1));

		LuaDialog dlg(L, true); // magically creates the config dialog structure etc
		ps->ShowDialog(&dlg);

		// more magic: puts two values on stack: button pushed and table with control results
		return dlg.LuaReadBack(L);
	}
}
