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

/// @file auto4_base.cpp
/// @brief Baseclasses for Automation 4 scripting framework
/// @ingroup scripting
///

#include "config.h"

#include "auto4_base.h"

#ifdef __WINDOWS__
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <wx/button.h>
#include <wx/dcmemory.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/thread.h>
#include <wx/tokenzr.h>

#include <tuple>

#ifndef __WINDOWS__
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "ass_file.h"
#include "ass_style.h"
#include "command/command.h"
#include "compat.h"
#include "dialog_progress.h"
#include "include/aegisub/context.h"
#include "main.h"
#include "standard_paths.h"
#include "string_codec.h"
#include "subtitle_format.h"
#include "utils.h"

#ifdef _WIN32
#include <libaegisub/charset_conv_win.h>
#endif

namespace Automation4 {
	bool CalculateTextExtents(AssStyle *style, wxString const& text, double &width, double &height, double &descent, double &extlead)
	{
		width = height = descent = extlead = 0;

		double fontsize = style->fontsize * 64;
		double spacing = style->spacing * 64;

#ifdef WIN32
		// This is almost copypasta from TextSub
		HDC thedc = CreateCompatibleDC(0);
		if (!thedc) return false;
		SetMapMode(thedc, MM_TEXT);

		LOGFONTW lf;
		ZeroMemory(&lf, sizeof(lf));
		lf.lfHeight = (LONG)fontsize;
		lf.lfWeight = style->bold ? FW_BOLD : FW_NORMAL;
		lf.lfItalic = style->italic;
		lf.lfUnderline = style->underline;
		lf.lfStrikeOut = style->strikeout;
		lf.lfCharSet = style->encoding;
		lf.lfOutPrecision = OUT_TT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = ANTIALIASED_QUALITY;
		lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
		wcsncpy(lf.lfFaceName, agi::charset::ConvertW(style->font).c_str(), 32);

		HFONT thefont = CreateFontIndirect(&lf);
		if (!thefont) return false;
		SelectObject(thedc, thefont);

		SIZE sz;
		size_t thetextlen = text.length();
		const TCHAR *thetext = text.wc_str();
		if (spacing != 0 ) {
			width = 0;
			for (unsigned int i = 0; i < thetextlen; i++) {
				GetTextExtentPoint32(thedc, &thetext[i], 1, &sz);
				width += sz.cx + spacing;
				height = sz.cy;
			}
		} else {
			GetTextExtentPoint32(thedc, thetext, (int)thetextlen, &sz);
			width = sz.cx;
			height = sz.cy;
		}

		// HACKISH FIX! This seems to work, but why? It shouldn't be needed?!?
		//fontsize = style->fontsize;
		//width = (int)(width * fontsize/height + 0.5);
		//height = (int)(fontsize + 0.5);

		TEXTMETRIC tm;
		GetTextMetrics(thedc, &tm);
		descent = tm.tmDescent;
		extlead= tm.tmExternalLeading;

		DeleteObject(thedc);
		DeleteObject(thefont);

#else // not WIN32
		wxMemoryDC thedc;

		// fix fontsize to be 72 DPI
		//fontsize = -FT_MulDiv((int)(fontsize+0.5), 72, thedc.GetPPI().y);

		// now try to get a font!
		// use the font list to get some caching... (chance is the script will need the same font very often)
		// USING wxTheFontList SEEMS TO CAUSE BAD LEAKS!
		//wxFont *thefont = wxTheFontList->FindOrCreateFont(
		wxFont thefont(
			(int)fontsize,
			wxFONTFAMILY_DEFAULT,
			style->italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
			style->bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
			style->underline,
			style->font,
			wxFONTENCODING_SYSTEM); // FIXME! make sure to get the right encoding here, make some translation table between windows and wx encodings
		thedc.SetFont(thefont);

		if (spacing) {
			// If there's inter-character spacing, kerning info must not be used, so calculate width per character
			// NOTE: Is kerning actually done either way?!
			for (unsigned int i = 0; i < text.length(); i++) {
				int a, b, c, d;
				thedc.GetTextExtent(text[i], &a, &b, &c, &d);
				double scaling = fontsize / (double)(b>0?b:1); // semi-workaround for missing OS/2 table data for scaling
				width += (a + spacing)*scaling;
				height = b > height ? b*scaling : height;
				descent = c > descent ? c*scaling : descent;
				extlead = d > extlead ? d*scaling : extlead;
			}
		} else {
			// If the inter-character spacing should be zero, kerning info can (and must) be used, so calculate everything in one go
			wxCoord lwidth, lheight, ldescent, lextlead;
			thedc.GetTextExtent(text, &lwidth, &lheight, &ldescent, &lextlead);
			double scaling = fontsize / (double)(lheight>0?lheight:1); // semi-workaround for missing OS/2 table data for scaling
			width = lwidth*scaling; height = lheight*scaling; descent = ldescent*scaling; extlead = lextlead*scaling;
		}
#endif

		// Compensate for scaling
		width = style->scalex / 100 * width / 64;
		height = style->scaley / 100 * height / 64;
		descent = style->scaley / 100 * descent / 64;
		extlead = style->scaley / 100 * extlead / 64;

		return true;
	}

	ExportFilter::ExportFilter(std::string const& name, std::string const& description, int priority)
	: AssExportFilter(name, description, priority)
	{
		AssExportFilterChain::Register(this);
	}

	ExportFilter::~ExportFilter()
	{
		AssExportFilterChain::Unregister(this);
	}

	std::string ExportFilter::GetScriptSettingsIdentifier()
	{
		return inline_string_encode("Automation Settings " + GetName());
	}

	wxWindow* ExportFilter::GetConfigDialogWindow(wxWindow *parent, agi::Context *c) {
		config_dialog.reset(GenerateConfigDialog(parent, c));

		if (config_dialog) {
			std::string val = c->ass->GetScriptInfo(GetScriptSettingsIdentifier());
			if (!val.empty())
				config_dialog->Unserialise(val);
			return config_dialog->CreateWindow(parent);
		}

		return 0;
	}

	void ExportFilter::LoadSettings(bool is_default, agi::Context *c) {
		if (config_dialog) {
			std::string val = config_dialog->Serialise();
			if (!val.empty())
				c->ass->SetScriptInfo(GetScriptSettingsIdentifier(), val);
		}
	}

	// ProgressSink
	ProgressSink::ProgressSink(agi::ProgressSink *impl, BackgroundScriptRunner *bsr)
	: impl(impl)
	, bsr(bsr)
	, trace_level(OPT_GET("Automation/Trace Level")->GetInt())
	{
	}

	void ProgressSink::ShowDialog(ScriptDialog *config_dialog)
	{
		InvokeOnMainThread([=] {
			wxDialog w; // container dialog box
			w.SetExtraStyle(wxWS_EX_VALIDATE_RECURSIVELY);
			w.Create(bsr->GetParentWindow(), -1, bsr->GetTitle());
			wxBoxSizer *s = new wxBoxSizer(wxHORIZONTAL); // sizer for putting contents in
			wxWindow *ww = config_dialog->CreateWindow(&w); // generate actual dialog contents
			s->Add(ww, 0, wxALL, 5); // add contents to dialog
			w.SetSizerAndFit(s);
			w.CenterOnParent();
			w.ShowModal();
		});
	}

	int ProgressSink::ShowDialog(wxDialog *dialog)
	{
		int ret = 0;
		InvokeOnMainThread([&] { ret = dialog->ShowModal(); });
		return ret;
	}

	BackgroundScriptRunner::BackgroundScriptRunner(wxWindow *parent, wxString const& title)
	: impl(new DialogProgress(parent, title))
	{
	}

	BackgroundScriptRunner::~BackgroundScriptRunner()
	{
	}

	void BackgroundScriptRunner::Run(std::function<void (ProgressSink*)> task)
	{
		int prio = OPT_GET("Automation/Thread Priority")->GetInt();
		if (prio == 0) prio = 50; // normal
		else if (prio == 1) prio = 30; // below normal
		else if (prio == 2) prio = 10; // lowest
		else prio = 50; // fallback normal

		impl->Run([&](agi::ProgressSink *ps) {
			ProgressSink aps(ps, this);
			task(&aps);
		}, prio);
	}

	wxWindow *BackgroundScriptRunner::GetParentWindow() const
	{
		return impl.get();
	}

	wxString BackgroundScriptRunner::GetTitle() const
	{
		return impl->GetTitle();
	}

	// Script
	Script::Script(wxString const& filename)
	: filename(filename)
	{
		// copied from auto3
		include_path.clear();
		include_path.EnsureFileAccessible(filename);
		wxStringTokenizer toker(to_wx(OPT_GET("Path/Automation/Include")->GetString()), "|", wxTOKEN_STRTOK);
		while (toker.HasMoreTokens()) {
			// todo? make some error reporting here
			wxFileName path(StandardPaths::DecodePath(toker.GetNextToken()));
			if (!path.IsOk()) continue;
			if (path.IsRelative()) continue;
			if (!path.DirExists()) continue;
			include_path.Add(path.GetLongPath());
		}
	}

	// ScriptManager
	ScriptManager::~ScriptManager()
	{
		delete_clear(scripts);
	}

	void ScriptManager::Add(Script *script)
	{
		if (find(scripts.begin(), scripts.end(), script) == scripts.end())
			scripts.push_back(script);

		ScriptsChanged();
	}

	void ScriptManager::Remove(Script *script)
	{
		auto i = find(scripts.begin(), scripts.end(), script);
		if (i != scripts.end()) {
			delete *i;
			scripts.erase(i);
		}

		ScriptsChanged();
	}

	void ScriptManager::RemoveAll()
	{
		delete_clear(scripts);
		ScriptsChanged();
	}

	void ScriptManager::Reload(Script *script)
	{
		if (find(scripts.begin(), scripts.end(), script) != scripts.end())
		{
			script->Reload();
			ScriptsChanged();
		}
	}

	const std::vector<cmd::Command*>& ScriptManager::GetMacros()
	{
		macros.clear();
		for (auto script : scripts) {
			std::vector<cmd::Command*> sfs = script->GetMacros();
			copy(sfs.begin(), sfs.end(), back_inserter(macros));
		}
		return macros;
	}


	// AutoloadScriptManager
	AutoloadScriptManager::AutoloadScriptManager(wxString const& path)
	: path(path)
	{
		Reload();
	}

	void AutoloadScriptManager::Reload()
	{
		delete_clear(scripts);

		int error_count = 0;

		wxStringTokenizer tok(path, "|", wxTOKEN_STRTOK);
		while (tok.HasMoreTokens()) {
			wxDir dir;
			wxString dirname = StandardPaths::DecodePath(tok.GetNextToken());
			if (!dir.Exists(dirname)) {
				//wxLogWarning("A directory was specified in the Automation autoload path, but it doesn't exist: %s", dirname.c_str());
				continue;
			}
			if (!dir.Open(dirname)) {
				//wxLogWarning("Failed to open a directory in the Automation autoload path: %s", dirname.c_str());
				continue;
			}

			wxString fn;
			wxFileName script_path(dirname + "/", "");
			bool more = dir.GetFirst(&fn, wxEmptyString, wxDIR_FILES);
			while (more) {
				script_path.SetName(fn);
				wxString fullpath = script_path.GetFullPath();
				if (ScriptFactory::CanHandleScriptFormat(fullpath)) {
					Script *s = ScriptFactory::CreateFromFile(fullpath, true);
					scripts.push_back(s);
					if (!s->GetLoadedState()) error_count++;
				}
				more = dir.GetNext(&fn);
			}
		}

		if (error_count == 1) {
			wxLogWarning("A script in the Automation autoload directory failed to load.\nPlease review the errors, fix them and use the Rescan Autoload Dir button in Automation Manager to load the scripts again.");
		}
		else if (error_count > 1) {
			wxLogWarning("Multiple scripts in the Automation autoload directory failed to load.\nPlease review the errors, fix them and use the Rescan Autoload Dir button in Automation Manager to load the scripts again.");
		}

		ScriptsChanged();
	}

	LocalScriptManager::LocalScriptManager(agi::Context *c)
	: context(c)
	{
		slots.push_back(c->ass->AddFileSaveListener(&LocalScriptManager::OnSubtitlesSave, this));
		slots.push_back(c->ass->AddFileOpenListener(&LocalScriptManager::Reload, this));
	}

	void LocalScriptManager::Reload()
	{
		delete_clear(scripts);

		wxString local_scripts(to_wx(context->ass->GetScriptInfo("Automation Scripts")));
		if (local_scripts.empty()) {
			ScriptsChanged();
			return;
		}

		wxStringTokenizer tok(local_scripts, "|", wxTOKEN_STRTOK);
		wxFileName assfn(context->ass->filename);
		wxString autobasefn(lagi_wxString(OPT_GET("Path/Automation/Base")->GetString()));
		while (tok.HasMoreTokens()) {
			wxString trimmed = tok.GetNextToken().Trim(true).Trim(false);
			char first_char = trimmed[0];
			trimmed.Remove(0, 1);

			wxString basepath;
			if (first_char == '~') {
				basepath = assfn.GetPath();
			} else if (first_char == '$') {
				basepath = autobasefn;
			} else if (first_char == '/') {
			} else {
				wxLogWarning("Automation Script referenced with unknown location specifier character.\nLocation specifier found: %c\nFilename specified: %s",
					first_char, trimmed);
				continue;
			}
			wxFileName sfname(trimmed);
			sfname.MakeAbsolute(basepath);
			if (sfname.FileExists()) {
				scripts.push_back(Automation4::ScriptFactory::CreateFromFile(sfname.GetFullPath(), true));
			}
			else {
				wxLogWarning("Automation Script referenced could not be found.\nFilename specified: %c%s\nSearched relative to: %s\nResolved filename: %s",
					first_char, trimmed, basepath, sfname.GetFullPath());
			}
		}

		ScriptsChanged();
	}

	void LocalScriptManager::OnSubtitlesSave()
	{
		// Store Automation script data
		// Algorithm:
		// 1. If script filename has Automation Base Path as a prefix, the path is relative to that (ie. "$")
		// 2. Otherwise try making it relative to the ass filename
		// 3. If step 2 failed, or absolute path is shorter than path relative to ass, use absolute path ("/")
		// 4. Otherwise, use path relative to ass ("~")
		std::string scripts_string;
		wxString autobasefn(to_wx(OPT_GET("Path/Automation/Base")->GetString()));

		for (auto script : GetScripts()) {
			if (!scripts_string.empty())
				scripts_string += "|";

			wxString scriptfn(script->GetFilename());
			wxString autobase_rel = MakeRelativePath(scriptfn, autobasefn);
			wxString assfile_rel = MakeRelativePath(scriptfn, to_wx(context->ass->filename));

			if (autobase_rel.size() <= scriptfn.size() && autobase_rel.size() <= assfile_rel.size()) {
				scriptfn = "$" + autobase_rel;
			} else if (assfile_rel.size() <= scriptfn.size() && assfile_rel.size() <= autobase_rel.size()) {
				scriptfn = "~" + assfile_rel;
			} else {
				scriptfn = "/" + wxFileName(scriptfn).GetFullPath(wxPATH_UNIX);
			}

			scripts_string += from_wx(scriptfn);
		}
		context->ass->SetScriptInfo("Automation Scripts", scripts_string);
	}

	// ScriptFactory
	ScriptFactory::ScriptFactory(wxString const& engine_name, wxString const& filename_pattern)
	: engine_name(engine_name)
	, filename_pattern(filename_pattern)
	{
	}

	void ScriptFactory::Register(ScriptFactory *factory)
	{
		if (find(Factories().begin(), Factories().end(), factory) != Factories().end())
			throw agi::InternalError("Automation 4: Attempt to register the same script factory multiple times. This should never happen.", 0);

		Factories().push_back(factory);
	}

	void ScriptFactory::Unregister(ScriptFactory *factory)
	{
		auto i = find(Factories().begin(), Factories().end(), factory);
		if (i != Factories().end()) {
			delete *i;
			Factories().erase(i);
		}
	}

	Script* ScriptFactory::CreateFromFile(wxString const& filename, bool log_errors)
	{
		for (auto factory : Factories()) {
			Script *s = factory->Produce(filename);
			if (s) {
				if (!s->GetLoadedState() && log_errors) {
					wxLogError(_("An Automation script failed to load. File name: '%s', error reported: %s"), filename, s->GetDescription());
				}
				return s;
			}
		}

		if (log_errors) {
			wxLogError(_("The file was not recognised as an Automation script: %s"), filename);
		}


		return new UnknownScript(filename);
	}

	bool ScriptFactory::CanHandleScriptFormat(wxString const& filename)
	{
		using std::placeholders::_1;
		// Just make this always return true to bitch about unknown script formats in autoload
		return any_of(Factories().begin(), Factories().end(),
			[&](ScriptFactory *sf) { return filename.Matches(sf->GetFilenamePattern()); });
	}

	std::vector<ScriptFactory*>& ScriptFactory::Factories()
	{
		static std::vector<ScriptFactory*> factories;
		return factories;
	}

	const std::vector<ScriptFactory*>& ScriptFactory::GetFactories()
	{
		return Factories();
	}

	wxString ScriptFactory::GetWildcardStr()
	{
		wxString fnfilter, catchall;
		for (auto fact : Factories()) {
			if (fact->GetEngineName().empty() || fact->GetFilenamePattern().empty())
				continue;

			fnfilter = wxString::Format("%s%s scripts (%s)|%s|", fnfilter, fact->GetEngineName(), fact->GetFilenamePattern(), fact->GetFilenamePattern());
			catchall += fact->GetFilenamePattern() + ";";
		}
		fnfilter += _("All Files") + " (*.*)|*.*";

		if (!catchall.empty())
			catchall.RemoveLast();

		if (Factories().size() > 1)
			fnfilter = _("All Supported Formats") + "|" + catchall + "|" + fnfilter;

		return fnfilter;
	}

	// UnknownScript
	UnknownScript::UnknownScript(wxString const& filename)
	: Script(filename)
	{
	}
}
