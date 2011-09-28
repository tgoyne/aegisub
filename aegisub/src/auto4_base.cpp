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
//
// $Id$

/// @file auto4_base.cpp
/// @brief Baseclasses for Automation 4 scripting framework
/// @ingroup scripting
///

#include "config.h"

#ifndef AGI_PRE
#ifdef __WINDOWS__
#include <tchar.h>
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
#endif

#ifndef __WINDOWS__
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#include "ass_file.h"
#include "ass_style.h"
#include "auto4_base.h"
#include "compat.h"
#include "dialog_progress.h"
#include "include/aegisub/context.h"
#include "main.h"
#include "standard_paths.h"
#include "string_codec.h"
#include "utils.h"

/// DOCME
namespace Automation4 {
	/// @brief DOCME
	/// @param style   
	/// @param text    
	/// @param width   
	/// @param height  
	/// @param descent 
	/// @param extlead 
	/// @return 
	///
	bool CalculateTextExtents(AssStyle *style, wxString &text, double &width, double &height, double &descent, double &extlead)
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
		wcsncpy(lf.lfFaceName, style->font.wc_str(), 32);

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


	// Feature


	/// @brief DOCME
	/// @param _featureclass 
	/// @param _name         
	///
	Feature::Feature(ScriptFeatureClass _featureclass, const wxString &_name)
		: featureclass(_featureclass)
		, name(_name)
	{
		// nothing to do
	}


	/// @brief DOCME
	/// @return 
	///
	ScriptFeatureClass Feature::GetClass() const
	{
		return featureclass;
	}


	/// @brief DOCME
	/// @return 
	///
	FeatureMacro* Feature::AsMacro()
	{
		if (featureclass == SCRIPTFEATURE_MACRO)
			// For VS, remember to enable building with RTTI, otherwise dynamic_cast<> won't work
			return dynamic_cast<FeatureMacro*>(this);
		return 0;
	}


	/// @brief DOCME
	/// @return 
	///
	FeatureFilter* Feature::AsFilter()
	{
		if (featureclass == SCRIPTFEATURE_FILTER)
			return dynamic_cast<FeatureFilter*>(this);
		return 0;
	}


	/// @brief DOCME
	/// @return 
	///
	FeatureSubtitleFormat* Feature::AsSubFormat()
	{
		if (featureclass == SCRIPTFEATURE_SUBFORMAT)
			return dynamic_cast<FeatureSubtitleFormat*>(this);
		return 0;
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Feature::GetName() const
	{
		return name;
	}


	// FeatureMacro


	/// @brief DOCME
	/// @param _name        
	/// @param _description 
	///
	FeatureMacro::FeatureMacro(const wxString &_name, const wxString &_description)
		: Feature(SCRIPTFEATURE_MACRO, _name)
		, description(_description)
	{
		// nothing to do
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& FeatureMacro::GetDescription() const
	{
		return description;
	}


	// FeatureFilter


	/// @brief DOCME
	/// @param _name        
	/// @param _description 
	/// @param _priority    
	///
	FeatureFilter::FeatureFilter(const wxString &_name, const wxString &_description, int _priority)
		: Feature(SCRIPTFEATURE_FILTER, _name)
		, AssExportFilter(_name, _description, _priority)
		, config_dialog(0)
	{
		AssExportFilterChain::Register(this);
	}


	/// @brief DOCME
	///
	FeatureFilter::~FeatureFilter()
	{
		AssExportFilterChain::Unregister(this);
	}


	/// @brief DOCME
	/// @return 
	///
	wxString FeatureFilter::GetScriptSettingsIdentifier()
	{
		return inline_string_encode(wxString::Format("Automation Settings %s", AssExportFilter::GetName()));
	}


	/// @brief DOCME
	/// @param parent 
	/// @return 
	///
	wxWindow* FeatureFilter::GetConfigDialogWindow(wxWindow *parent) {
		if (config_dialog) {
			delete config_dialog;
			config_dialog = 0;
		}
		if ((config_dialog = GenerateConfigDialog(parent)) != NULL) {
			wxString val = AssFile::top->GetScriptInfo(GetScriptSettingsIdentifier());
			if (!val.IsEmpty()) {
				config_dialog->Unserialise(val);
			}
			return config_dialog->GetWindow(parent);
		} else {
			return 0;
		}
	}


	/// @brief DOCME
	/// @param IsDefault 
	///
	void FeatureFilter::LoadSettings(bool IsDefault) {
		if (config_dialog) {
			config_dialog->ReadBack();

			wxString val = config_dialog->Serialise();
			if (!val.IsEmpty()) {
				AssFile::top->SetScriptInfo(GetScriptSettingsIdentifier(), val);
			}
		}
	}


	// FeatureSubtitleFormat


	/// @brief DOCME
	/// @param _name      
	/// @param _extension 
	///
	FeatureSubtitleFormat::FeatureSubtitleFormat(const wxString &_name, const wxString &_extension)
		: Feature(SCRIPTFEATURE_SUBFORMAT, _name)
		, SubtitleFormat(_name)
		, extension(_extension)
	{
		// nothing to do
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& FeatureSubtitleFormat::GetExtension() const
	{
		return extension;
	}


	/// @brief DOCME
	/// @param filename 
	/// @return 
	///
	bool FeatureSubtitleFormat::CanWriteFile(wxString filename)
	{
		return !filename.Right(extension.Length()).CmpNoCase(extension);
	}


	/// @brief DOCME
	/// @param filename 
	/// @return 
	///
	bool FeatureSubtitleFormat::CanReadFile(wxString filename)
	{
		return !filename.Right(extension.Length()).CmpNoCase(extension);
	}

	// ScriptConfigDialog


	/// @brief DOCME
	/// @param parent 
	/// @return 
	///
	wxWindow* ScriptConfigDialog::GetWindow(wxWindow *parent)
	{
		if (win) return win;
		return win = CreateWindow(parent);
	}


	/// @brief DOCME
	///
	void ScriptConfigDialog::DeleteWindow()
	{
		if (win) delete win;
		win = 0;
	}


	/// @brief DOCME
	/// @return 
	///
	wxString ScriptConfigDialog::Serialise()
	{
		return "";
	}


	// ProgressSink
	wxDEFINE_EVENT(EVT_SHOW_CONFIG_DIALOG, wxThreadEvent);

	ProgressSink::ProgressSink(agi::ProgressSink *impl, BackgroundScriptRunner *bsr)
	: impl(impl)
	, bsr(bsr)
	, trace_level(OPT_GET("Automation/Trace Level")->GetInt())
	{
	}

	void ProgressSink::ShowConfigDialog(ScriptConfigDialog *config_dialog)
	{
		wxSemaphore sema(0, 1);
		wxThreadEvent *evt = new wxThreadEvent(EVT_SHOW_CONFIG_DIALOG);
		evt->SetPayload(std::make_pair(config_dialog, &sema));
		bsr->QueueEvent(evt);
		sema.Wait();
	}

	BackgroundScriptRunner::BackgroundScriptRunner(wxWindow *parent, wxString const& title)
	: impl(new DialogProgress(parent, title))
	{
		impl->Bind(EVT_SHOW_CONFIG_DIALOG, &BackgroundScriptRunner::OnConfigDialog, this);
	}

	BackgroundScriptRunner::~BackgroundScriptRunner()
	{
	}

	void BackgroundScriptRunner::OnConfigDialog(wxThreadEvent &evt)
	{
		std::pair<ScriptConfigDialog*, wxSemaphore*> payload = evt.GetPayload<std::pair<ScriptConfigDialog*, wxSemaphore*> >();

		wxDialog w(impl.get(), -1, impl->GetTitle()); // container dialog box
		wxBoxSizer *s = new wxBoxSizer(wxHORIZONTAL); // sizer for putting contents in
		wxWindow *ww = payload.first->GetWindow(&w); // get/generate actual dialog contents
		s->Add(ww, 0, wxALL, 5); // add contents to dialog
		w.SetSizerAndFit(s);
		w.CenterOnParent();
		w.ShowModal();
		payload.first->ReadBack();
		payload.first->DeleteWindow();

		payload.second->Post();
	}

	void BackgroundScriptRunner::QueueEvent(wxEvent *evt) {
		wxQueueEvent(impl.get(), evt);
	}

	// Convert a function taking an Automation4::ProgressSink to one taking an
	// agi::ProgressSink so that we can pass it to an agi::BackgroundWorker
	static void progress_sink_wrapper(std::tr1::function<void (ProgressSink*)> task, agi::ProgressSink *ps, BackgroundScriptRunner *bsr)
	{
		ProgressSink aps(ps, bsr);
		task(&aps);
	}

	void BackgroundScriptRunner::Run(std::tr1::function<void (ProgressSink*)> task)
	{
		int prio = OPT_GET("Automation/Thread Priority")->GetInt();
		if (prio == 0) prio = 50; // normal
		else if (prio == 1) prio = 30; // below normal
		else if (prio == 2) prio = 10; // lowest
		else prio = 50; // fallback normal

		impl->Run(bind(progress_sink_wrapper, task, std::tr1::placeholders::_1, this), prio);
	}


	// Script


	/// @brief DOCME
	/// @param _filename 
	///
	Script::Script(const wxString &_filename)
		: filename(_filename)
		, name("")
		, description("")
		, author("")
		, version("")
		, loaded(false)
	{
		// copied from auto3
		include_path.clear();
		include_path.EnsureFileAccessible(filename);
		wxStringTokenizer toker(lagi_wxString(OPT_GET("Path/Automation/Include")->GetString()), "|", wxTOKEN_STRTOK);
		while (toker.HasMoreTokens()) {
			// todo? make some error reporting here
			wxFileName path(StandardPaths::DecodePath(toker.GetNextToken()));
			if (!path.IsOk()) continue;
			if (path.IsRelative()) continue;
			if (!path.DirExists()) continue;
			include_path.Add(path.GetLongPath());
		}
	}


	/// @brief DOCME
	///
	Script::~Script()
	{
		for (std::vector<Feature*>::iterator f = features.begin(); f != features.end(); ++f) {
			delete *f;
		}
	}


	/// @brief DOCME
	/// @return 
	///
	wxString Script::GetPrettyFilename() const
	{
		wxFileName fn(filename);
		return fn.GetFullName();
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Script::GetFilename() const
	{
		return filename;
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Script::GetName() const
	{
		return name;
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Script::GetDescription() const
	{
		return description;
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Script::GetAuthor() const
	{
		return author;
	}


	/// @brief DOCME
	/// @return 
	///
	const wxString& Script::GetVersion() const
	{
		return version;
	}


	/// @brief DOCME
	/// @return 
	///
	bool Script::GetLoadedState() const
	{
		return loaded;
	}


	/// @brief DOCME
	/// @return 
	///
	std::vector<Feature*>& Script::GetFeatures()
	{
		return features;
	}


	// ScriptManager


	/// @brief DOCME
	///
	ScriptManager::ScriptManager()
	{
		// do nothing...?
	}


	/// @brief DOCME
	///
	ScriptManager::~ScriptManager()
	{
		RemoveAll();
	}


	/// @brief DOCME
	/// @param script 
	/// @return 
	///
	void ScriptManager::Add(Script *script)
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			if (script == *i) return;
		}
		scripts.push_back(script);
	}


	/// @brief DOCME
	/// @param script 
	/// @return 
	///
	void ScriptManager::Remove(Script *script)
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			if (script == *i) {
				delete *i;
				scripts.erase(i);
				return;
			}
		}
	}


	/// @brief DOCME
	///
	void ScriptManager::RemoveAll()
	{
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			delete *i;
		}
		scripts.clear();
	}


	/// @brief DOCME
	/// @return 
	///
	const std::vector<Script*>& ScriptManager::GetScripts() const
	{
		return scripts;
	}


	/// @brief DOCME
	/// @return 
	///
	const std::vector<FeatureMacro*>& ScriptManager::GetMacros()
	{
		macros.clear();
		for (std::vector<Script*>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
			std::vector<Feature*> &sfs = (*i)->GetFeatures();
			for (std::vector<Feature*>::iterator j = sfs.begin(); j != sfs.end(); ++j) {
				FeatureMacro *m = dynamic_cast<FeatureMacro*>(*j);
				if (!m) continue;
				macros.push_back(m);
			}
		}
		return macros;
	}


	// AutoloadScriptManager


	/// @brief DOCME
	/// @param _path 
	///
	AutoloadScriptManager::AutoloadScriptManager(const wxString &_path)
		: path(_path)
	{
		Reload();
	}


	/// @brief DOCME
	///
	void AutoloadScriptManager::Reload()
	{
		RemoveAll();

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
						Add(s);
						if (!s->GetLoadedState()) error_count++;
					}
				more = dir.GetNext(&fn);
			}
		}
		if (error_count > 0) {
			wxLogWarning("One or more scripts placed in the Automation autoload directory failed to load\nPlease review the errors above, correct them and use the Reload Autoload dir button in Automation Manager to attempt loading the scripts again.");
		}
	}

	LocalScriptManager::LocalScriptManager(agi::Context *c)
	: context(c)
	{
		slots.push_back(c->ass->AddFileSaveListener(&LocalScriptManager::OnSubtitlesSave, this));
		slots.push_back(c->ass->AddFileOpenListener(&LocalScriptManager::Reload, this));
	}

	void LocalScriptManager::Reload() {
		RemoveAll();

		wxString local_scripts = context->ass->GetScriptInfo("Automation Scripts");
		if (local_scripts.empty()) return;

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
				basepath = "";
			} else {
				wxLogWarning("Automation Script referenced with unknown location specifier character.\nLocation specifier found: %c\nFilename specified: %s",
					first_char, trimmed);
				continue;
			}
			wxFileName sfname(trimmed);
			sfname.MakeAbsolute(basepath);
			if (sfname.FileExists()) {
				wxString err;
				Add(Automation4::ScriptFactory::CreateFromFile(sfname.GetFullPath(), true));
			} else {
				wxLogWarning("Automation Script referenced could not be found.\nFilename specified: %c%s\nSearched relative to: %s\nResolved filename: %s",
					first_char, trimmed, basepath, sfname.GetFullPath());
			}
		}
	}

	void LocalScriptManager::OnSubtitlesSave() {
		// Store Automation script data
		// Algorithm:
		// 1. If script filename has Automation Base Path as a prefix, the path is relative to that (ie. "$")
		// 2. Otherwise try making it relative to the ass filename
		// 3. If step 2 failed, or absolute path is shorter than path relative to ass, use absolute path ("/")
		// 4. Otherwise, use path relative to ass ("~")
		wxString scripts_string;
		wxString autobasefn(lagi_wxString(OPT_GET("Path/Automation/Base")->GetString()));

		for (size_t i = 0; i < GetScripts().size(); i++) {
			Script *script = GetScripts()[i];

			if (i != 0)
				scripts_string += "|";

			wxString autobase_rel, assfile_rel;
			wxString scriptfn(script->GetFilename());
			autobase_rel = MakeRelativePath(scriptfn, autobasefn);
			assfile_rel = MakeRelativePath(scriptfn, context->ass->filename);

			if (autobase_rel.size() <= scriptfn.size() && autobase_rel.size() <= assfile_rel.size()) {
				scriptfn = "$" + autobase_rel;
			} else if (assfile_rel.size() <= scriptfn.size() && assfile_rel.size() <= autobase_rel.size()) {
				scriptfn = "~" + assfile_rel;
			} else {
				scriptfn = "/" + wxFileName(scriptfn).GetFullPath(wxPATH_UNIX);
			}

			scripts_string += scriptfn;
		}
		context->ass->SetScriptInfo("Automation Scripts", scripts_string);
	}

	// ScriptFactory
	std::vector<ScriptFactory*> *ScriptFactory::factories = 0;

	ScriptFactory::ScriptFactory(wxString engine_name, wxString filename_pattern)
	: engine_name(engine_name)
	, filename_pattern(filename_pattern)
	{
	}

	void ScriptFactory::Register(ScriptFactory *factory)
	{
		GetFactories();

		if (find(factories->begin(), factories->end(), factory) != factories->end())
			throw agi::InternalError("Automation 4: Attempt to register the same script factory multiple times. This should never happen.", 0);

		factories->push_back(factory);
	}

	void ScriptFactory::Unregister(ScriptFactory *factory)
	{
		if (!factories) return;

		std::vector<ScriptFactory*>::iterator i = find(factories->begin(), factories->end(), factory);
		if (i != factories->end()) {
			delete *i;
			factories->erase(i);
		}
	}

	Script* ScriptFactory::CreateFromFile(wxString const& filename, bool log_errors)
	{
		GetFactories();

		for (std::vector<ScriptFactory*>::iterator i = factories->begin(); i != factories->end(); ++i) {
			Script *s = (*i)->Produce(filename);
			if (s) {
				if (!s->GetLoadedState() && log_errors)
					wxLogError(_("An Automation script failed to load. File name: '%s', error reported: %s"), filename, s->GetDescription());
				return s;
			}
		}

		if (log_errors)
			wxLogError(_("The file was not recognised as an Automation script: %s"), filename);

		return new UnknownScript(filename);
	}

	bool ScriptFactory::CanHandleScriptFormat(wxString const& filename)
	{
		using std::tr1::placeholders::_1;
		// Just make this always return true to bitch about unknown script formats in autoload
		GetFactories();
		return find_if(factories->begin(), factories->end(),
			bind(&wxString::Matches, filename, bind(&ScriptFactory::GetFilenamePattern, _1))) != factories->end();
	}

	const std::vector<ScriptFactory*>& ScriptFactory::GetFactories()
	{
		if (!factories)
			factories = new std::vector<ScriptFactory*>();

		return *factories;
	}

	wxString ScriptFactory::GetWildcardStr()
	{
		GetFactories();

		wxString fnfilter, catchall;
		for (size_t i = 0; i < factories->size(); ++i) {
			const ScriptFactory *fact = (*factories)[i];
			if (fact->GetEngineName().empty() || fact->GetFilenamePattern().empty())
				continue;

			fnfilter = wxString::Format("%s%s scripts (%s)|%s|", fnfilter, fact->GetEngineName(), fact->GetFilenamePattern(), fact->GetFilenamePattern());
			catchall += fact->GetFilenamePattern() + ";";
		}
#ifdef __WINDOWS__
		fnfilter += "All files|*.*";
#else
		fnfilter += "All files|*";
#endif
		if (!catchall.empty())
			catchall.RemoveLast();

		if (factories->size() > 1)
			fnfilter = "All supported scripts|" + catchall + "|" + fnfilter;

		return fnfilter;
	}


	// UnknownScript


	/// @brief DOCME
	/// @param filename 
	///
	UnknownScript::UnknownScript(const wxString &filename)
		: Script(filename)
	{
		wxFileName fn(filename);
		name = fn.GetName();
		description = _("File was not recognized as a script");
		loaded = false;
	}

};
