// Copyright (c) 2005, Rodrigo Braz Monteiro
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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


////////////
// Includes
#include <string>
#include <wx/filename.h>
#include <fstream>
#include <wx/intl.h>
#include "options.h"
#include "main.h"
#include "text_file_reader.h"
#include "text_file_writer.h"


///////////////
// Constructor
OptionsManager::OptionsManager() {
	modified = false;
}


//////////////
// Destructor
OptionsManager::~OptionsManager() {
	opt.clear();
}


///////////////////////
// Load default values
void OptionsManager::LoadDefaults() {
	SetInt(_T("Locale Code"),-1);
	SetText(_T("Save Charset"),_T("UTF-8"));
	SetBool(_T("Use nonstandard Milisecond Times"),false);

	SetInt(_T("Undo Levels"),8);
	SetInt(_T("Recent sub max"),16);
	SetInt(_T("Recent vid max"),16);
	SetInt(_T("Recent aud max"),16);
	SetInt(_T("Recent find max"),16);
	SetInt(_T("Recent replace max"),16);

	SetBool(_T("Sync video with subs"),true);
	SetBool(_T("Show keyframes on video slider"),true);
	SetBool(_T("Link Time Boxes Commit"),true);

	SetBool(_T("Syntax Highlight Enabled"),true);
	SetColour(_T("Syntax Highlight Normal"),wxColour(0,0,0));
	SetColour(_T("Syntax Highlight Brackets"),wxColour(20,50,255));
	SetColour(_T("Syntax Highlight Slashes"),wxColour(255,0,200));
	SetColour(_T("Syntax Highlight Tags"),wxColour(90,90,90));
	SetColour(_T("Syntax Highlight Error"),wxColour(200,0,0));
	SetColour(_T("Edit Box Need Enter Background"),wxColour(192,192,255));
	SetInt(_T("Font Size"),9);
	SetText(_T("Font Face"),_T(""));

	SetBool(_T("Shift Times ByTime"),true);
	SetInt(_T("Shift Times Type"),0);
	SetInt(_T("Shift Times Length"),0);
	SetBool(_T("Shift Times All Rows"),true);
	SetBool(_T("Shift Times Direction"),true);

	SetBool(_T("Tips enabled"),true);
	SetInt(_T("Tips current"),0);

	SetBool(_T("Show splash"),true);
	SetInt(_T("Splash number"),5);
	SetBool(_T("Show associations"),true);

	SetBool(_T("Find Match Case"),false);
	SetBool(_T("Find RegExp"),false);
	SetBool(_T("Find Update Video"),false);
	SetInt(_T("Find Affect"),0);
	SetInt(_T("Find Field"),0);

	SetColour(_T("Grid selection background"),wxColour(206,255,231));
	SetColour(_T("Grid selection foreground"),wxColour(0,0,0));
	SetColour(_T("Grid comment background"),wxColour(216,222,245));
	SetColour(_T("Grid selected comment background"),wxColour(211,238,238));
	SetColour(_T("Grid inframe background"),wxColour(255,253,234));
	SetInt(_T("Grid hide overrides"),1);
	wchar_t temp = 0x2600;
	SetText(_T("Grid hide overrides char"),temp);
	SetInt(_T("Grid font size"),8);

	SetBool(_T("Highlight subs in frame"),true);

	SetText(_T("Fonts Collector Destination"),_T("?script"));

	SetBool(_T("Threaded Video"),false);
	SetInt(_T("Avisynth MemoryMax"),64);
	SetText(_T("Video resizer"),_T("BilinearResize"));
	SetInt(_T("Video Check Script Res"), 0);
	SetInt(_T("Video Default Zoom"), 7);
	SetInt(_T("Video Fast Jump Step"), 10);

	SetInt(_T("Audio Cache"),1);
	SetInt(_T("Audio Sample Rate"),0);
	SetText(_T("Audio Downmixer"),_T("ConvertToMono"));
	SetBool(_T("Audio Autocommit"),false);
	SetBool(_T("Audio Autoscroll"),true);
	SetBool(_T("Audio SSA Mode"),false);
	SetBool(_T("Audio SSA Next Line on Commit"),true);
	SetBool(_T("Audio SSA Allow Autocommit"),false);
	SetBool(_T("Audio Autofocus"),false);
	SetInt(_T("Audio Display Height"),100);
	SetInt(_T("Timing Default Duration"), 2000);
	SetBool(_T("Audio Wheel Default To Zoom"),false);
	
	SetBool(_T("Audio Spectrum"),false);
	SetInt(_T("Audio Spectrum Cutoff"),32);
	SetInt(_T("Audio Spectrum Window"),11);
	SetBool(_T("Audio Spectrum invert selection"), true);
	SetInt(_T("Audio lead in"),200);
	SetInt(_T("Audio lead out"),300);
	SetInt(_T("Audio Inactive Lines Display Mode"),1);

	SetInt(_T("Timing processor key overlen thres"),5);
	SetInt(_T("Timing processor key underlen thres"),4);
	SetInt(_T("Timing processor adjascent thres"),300);
	SetBool(_T("Timing processor Enable lead-in"),true);
	SetBool(_T("Timing processor Enable lead-out"),true);
	SetBool(_T("Timing processor Enable keyframe"),true);
	SetBool(_T("Timing processor Enable adjascent"),true);

	SetColour(_T("Audio Selection Background Modified"),wxColour(92,0,0));
	SetColour(_T("Audio Selection Background"),wxColour(64,64,64));
	SetColour(_T("Audio Seconds Boundaries"),wxColour(0,100,255));
	SetColour(_T("Audio Waveform Modified"),wxColour(255,200,200));
	SetColour(_T("Audio Waveform Selected"),wxColour(255,255,255));
	SetColour(_T("Audio Waveform Inactive"),wxColour(0,80,0));
	SetColour(_T("Audio Waveform"),wxColour(0,200,0));
	SetColour(_T("Audio Line boundary start"),wxColour(255,0,0));
	SetColour(_T("Audio Line boundary end"),wxColour(255,0,0));
	SetColour(_T("Audio Line boundary inactive line"),wxColour(128,128,128));
	SetColour(_T("Audio Syllable boundaries"),wxColour(255,255,0));
	SetColour(_T("Audio Syllable text"),wxColour(255,0,0));
	SetColour(_T("Audio Play cursor"),wxColour(255,255,255));
	SetColour(_T("Audio Background"),wxColour(0,0,0));
	SetInt(_T("Audio Line boundaries Thickness"), 2);
	SetBool(_T("Audio Draw Secondary Lines"), true);
	SetBool(_T("Audio Draw Selection Background"), true);

	SetText(_T("Automation Include Path"), AegisubApp::folderName + _T("automation/include"));

	SetText(_T("Select Text"),_T(""));
	SetInt(_T("Select Condition"),0);
	SetInt(_T("Select Field"),0);
	SetInt(_T("Select Action"),0);
	SetInt(_T("Select Mode"),1);
	SetBool(_T("Select Match case"),false);

	SetBool(_T("Auto backup"),true);
	SetInt(_T("Auto save every seconds"),60);
	SetText(_T("Auto backup path"),_T("autoback"));
	SetText(_T("Auto save path"),_T("autosave"));
	SetInt(_T("Autoload linked files"),2);

	SetText(_T("Text actor separator"),_T(":"));
	SetText(_T("Text comment starter"),_T("#"));

	SetText(_T("Color Picker Recent"), _T("&H000000& &H0000FF& &H00FFFF& &H00FF00& &HFFFF00& &HFF0000& &HFF00FF& &HFFFFFF&"));
	SetInt(_T("Color Picker Mode"), 4);
}


////////////////
// Set filename
void OptionsManager::SetFile(wxString file) {
	filename = file;
}


////////
// Save
void OptionsManager::Save() {
	// Check if it's actually modified
	if (!modified) return;

	// Open file
	TextFileWriter file(filename,_T("UTF-8"));
	file.WriteLineToFile(_T("[Config]"));

	// Put variables in it
	for (std::map<wxString,VariableData>::iterator cur=opt.begin();cur!=opt.end();cur++) {
		file.WriteLineToFile((*cur).first + _T("=") + (*cur).second.AsText());
	}

	// Close
	modified = false;
}


////////
// Load
void OptionsManager::Load() {
	// Load defaults
	LoadDefaults();

	// Check if file exists (create if it doesn't)
	wxFileName path(filename);
	if (!path.FileExists()) {
		modified = true;
		Save();
		return;
	}

	// Read header
	TextFileReader file(filename);
	wxString header = file.ReadLineFromFile();
	if (header != _T("[Config]")) throw _T("Invalid config file");

	// Get variables
	std::map<wxString,VariableData>::iterator cur;
	wxString curLine;
	while (file.HasMoreLines()) {
		// Parse line
		curLine = file.ReadLineFromFile();
		if (curLine.IsEmpty()) continue;
		size_t pos = curLine.Find(_T("="));
		if (pos == wxString::npos) continue;
		wxString key = curLine.Left(pos);
		wxString value = curLine.Mid(pos+1);

		// Find it
		cur = opt.find(key);
		if (cur != opt.end()) {
			(*cur).second.ResetWith(value);
		}
		else SetText(key,value);
	}

	// Close
	Save();
}


/////////////
// Write int
void OptionsManager::SetInt(wxString key,int param) {
	opt[key.Lower()].SetInt(param);
	modified = true;
}


///////////////
// Write float
void OptionsManager::SetFloat(wxString key,double param) {
	opt[key.Lower()].SetFloat(param);
	modified = true;
}


////////////////
// Write string
void OptionsManager::SetText(wxString key,wxString param) {
	opt[key.Lower()].SetText(param);
	modified = true;
}


/////////////////
// Write boolean
void OptionsManager::SetBool(wxString key,bool param) {
	opt[key.Lower()].SetBool(param);
	modified = true;
}


////////////////
// Write colour
void OptionsManager::SetColour(wxString key,wxColour param) {
	opt[key.Lower()].SetColour(param);
	modified = true;
}


//////////
// As int
int OptionsManager::AsInt(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	if (cur != opt.end()) {
		return (*cur).second.AsInt();
	}
	else throw _T("Undefined name");
}


//////////////
// As boolean
bool OptionsManager::AsBool(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	if (cur != opt.end()) {
		return (*cur).second.AsBool();
	}
	else throw _T("Undefined name");
}


////////////
// As float
double OptionsManager::AsFloat(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	if (cur != opt.end()) {
		return (*cur).second.AsFloat();
	}
	else throw _T("Undefined name");
}


/////////////
// As string
wxString OptionsManager::AsText(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	if (cur != opt.end()) {
		return (*cur).second.AsText();
	}
	else throw _T("Undefined name");
}


/////////////
// As colour
wxColour OptionsManager::AsColour(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	if (cur != opt.end()) {
		return (*cur).second.AsColour();
	}
	else throw _T("Undefined name");
}


///////////////
// Is defined?
bool OptionsManager::IsDefined(wxString key) {
	std::map<wxString,VariableData>::iterator cur;
	cur = (opt.find(key.Lower()));
	return (cur != opt.end());
}


/////////////////////////////////////
// Adds an item to a list of recents
void OptionsManager::AddToRecentList (wxString entry,wxString list) {
	// Find strings already in recent list
	wxArrayString orig;
	wxString cur;
	int recentMax = AsInt(list + _T(" max"));
	int n = 0;
	for (int i=0;i<recentMax;i++) {
		wxString key = list + _T(" #") + wxString::Format(_T("%i"),i+1);
		if (IsDefined(key)) {
			cur = AsText(key);
			if (cur != entry) {
				orig.Add(cur);
				n++;
			}
		}
		else break;
	}

	// Write back to options
	SetText(list + _T(" #1"),entry);
	if (n > recentMax-1) n = recentMax-1;
	for (int i=0;i<n;i++) {
		wxString key = list + _T(" #") + wxString::Format(_T("%i"),i+2);
		SetText(key,orig[i]);
	}

	// Save options
	Save();
}


///////////////////
// Get recent list
wxArrayString OptionsManager::GetRecentList (wxString list) {
	wxArrayString work;
	int recentMax = AsInt(list + _T(" max"));
	for (int i=0;i<recentMax;i++) {
		wxString n = wxString::Format(_T("%i"),i+1);
		wxString key = list + _T(" #") + n;
		if (IsDefined(key)) {
			work.Add(Options.AsText(key));
		}
		else break;
	}
	return work;
}


///////////////////
// Global instance
OptionsManager Options;
