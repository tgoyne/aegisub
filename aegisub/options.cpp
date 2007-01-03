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
#include <wx/settings.h>
#include "options.h"
#include "main.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "colorspace.h"
#include "utils.h"


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
	///// PUBLIC //////
	// Here go the options that can be edited by the options menu
	
	// General
	SetModificationType(MOD_AUTOMATIC);
	SetBool(_T("Tips enabled"),true);
	SetBool(_T("Show splash"),true);
	SetInt(_T("Undo Levels"),8);
	SetInt(_T("Recent timecodes max"),16);
	SetInt(_T("Recent keyframes max"),16);
	SetInt(_T("Recent sub max"),16);
	SetInt(_T("Recent vid max"),16);
	SetInt(_T("Recent aud max"),16);
	SetInt(_T("Recent find max"),16);
	SetInt(_T("Recent replace max"),16);

	// File Save/Load
	SetModificationType(MOD_RESTART);
	SetInt(_T("Auto save every seconds"),60);
	SetModificationType(MOD_AUTOMATIC);
	SetText(_T("Auto save path"),_T("autosave"));
	SetBool(_T("Auto backup"),true);
	SetText(_T("Auto backup path"),_T("autoback"));
	SetText(_T("Auto recovery path"),_T("recovered"));
	SetInt(_T("Autoload linked files"),2);
	SetText(_T("Text actor separator"),_T(":"));
	SetText(_T("Text comment starter"),_T("#"));
	SetText(_T("Save Charset"),_T("UTF-8"));
	SetBool(_T("Use nonstandard Milisecond Times"),false);
	SetBool(_T("Auto save on every change"),false);

	// Edit Box
	SetText(_T("Dictionaries path"),_T("dictionaries"));
	SetBool(_T("Link Time Boxes Commit"),true);
	SetBool(_T("Call Tips Enabled"),true);
	SetBool(_T("Syntax Highlight Enabled"),true);

	// Edit box cosmetic
	SetColour(_T("Syntax Highlight Normal"),wxColour(0,0,0));
	SetColour(_T("Syntax Highlight Brackets"),wxColour(20,50,255));
	SetColour(_T("Syntax Highlight Slashes"),wxColour(255,0,200));
	SetColour(_T("Syntax Highlight Tags"),wxColour(90,90,90));
	SetColour(_T("Syntax Highlight Parameters"),wxColour(40,90,40));
	SetColour(_T("Syntax Highlight Error"),wxColour(200,0,0));
	SetColour(_T("Syntax Highlight Error Background"),wxColour(255,200,200));
	SetColour(_T("Syntax Highlight Line Break"),wxColour(160,160,160));
	SetColour(_T("Edit Box Need Enter Background"),wxColour(192,192,255));
#if defined(__WINDOWS__)
	SetInt(_T("Font Size"),9);
#else
	SetInt(_T("Font Size"),11);
#endif
	SetText(_T("Font Face"),_T(""));

	// Video Options
	SetInt(_T("Video Check Script Res"), 0);
	SetInt(_T("Video Default Zoom"), 7);
	SetInt(_T("Video Fast Jump Step"), 10);
	SetBool(_T("Show keyframes on video slider"),true);

	// Video Provider (Advanced)
	SetBool(_T("Threaded Video"),false);
	SetInt(_T("Avisynth MemoryMax"),64);
	SetBool(_T("Allow Ancient Avisynth"),false);
	SetText(_T("Video Provider"),_T("Avisynth"));
	SetText(_T("Video resizer"),_T("BilinearResize"));

	// Audio Options
	SetBool(_T("Audio SSA Next Line on Commit"),true);
	SetBool(_T("Audio SSA Allow Autocommit"),false);
	SetBool(_T("Audio Autofocus"),false);
	SetBool(_T("Audio Wheel Default To Zoom"),false);
	SetBool(_T("Audio lock scroll on cursor"),false);
	SetInt(_T("Timing Default Duration"), 2000);
	SetInt(_T("Audio lead in"),200);
	SetInt(_T("Audio lead out"),300);
	SetInt(_T("Audio Inactive Lines Display Mode"),1);

	// Audio Provider (Advanced)
	SetInt(_T("Audio Cache"),1);
	SetInt(_T("Audio Sample Rate"),0);
	SetText(_T("Audio Downmixer"),_T("ConvertToMono"));
	SetText(_T("Audio HD Cache Location"),_T("default"));
	SetText(_T("Audio HD Cache Name"),_T("audio%02i.tmp"));

	// Automation
	SetText(_T("Automation Base Path"), AegisubApp::folderName + _T("automation/"));
	SetText(_T("Automation Include Path"), AegisubApp::folderName + _T("automation/include/"));
	SetText(_T("Automation Autoload Path"), AegisubApp::folderName + _T("automation/autoload/"));
	SetInt(_T("Automation Trace Level"), 3);
	
	// Generate colors
	wxColour tempCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	float r = tempCol.Red() / 255.0;
	float g = tempCol.Green() / 255.0;
	float b = tempCol.Blue() / 255.0;
	wxColour textCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	wxColour background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	wxColour comment = wxColour(216,222,245);
	wxColour selection = wxColour(206,255,231);
	wxColour selComment = wxColour(211,238,238);
	wxColour header = wxColour(165,207,231);
	wxColour labels = wxColour(196,236,201);
	wxColour inframe = wxColour(255,253,234);
	wxColour active = wxColour(255,91,239);
	wxColour grid = wxColour(128,128,128);
	wxColour collision = wxColour(255,0,0);

	// Grid cosmetic
	SetColour(_T("Grid standard foreground"),textCol);
	SetColour(_T("Grid selection background"),selection);
	SetColour(_T("Grid selection foreground"),textCol);
	SetColour(_T("Grid comment background"),comment);
	SetColour(_T("Grid collision foreground"),collision);
	SetColour(_T("Grid selected comment background"),selComment);
	SetColour(_T("Grid inframe background"),inframe);
	SetColour(_T("Grid background"),background);
	SetColour(_T("Grid header"),header);
	SetColour(_T("Grid left column"),labels);
	SetColour(_T("Grid active border"),active);
	SetColour(_T("Grid lines"),grid);
	wchar_t temp = 0x2600;
	SetText(_T("Grid hide overrides char"),temp);
	SetBool(_T("Grid allow focus"),true);
	SetBool(_T("Highlight subs in frame"),true);
#if defined(__WINDOWS__)
	SetInt(_T("Grid font size"),8);
#else
	SetInt(_T("Grid font size"),10);
#endif

	// Audio Cosmetic
	SetInt(_T("Audio Spectrum Cutoff"),32);
	SetInt(_T("Audio Spectrum Window"),11);
	SetBool(_T("Audio Spectrum invert selection"), true);
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
	SetModificationType(MOD_OFF);



	///// INTERNAL //////
	// Options that are set by the program itself
	SetInt(_T("Locale Code"),-1);
	SetBool(_T("Sync video with subs"),true);
	SetText(_T("Spell checker language"),_T("en_US"));
	SetText(_T("Thesaurus language"),_T("en_US"));
	SetInt(_T("Options Page"),0);

	SetBool(_T("Audio Link"),true);
	SetBool(_T("Audio Autocommit"),false);
	SetBool(_T("Audio Autoscroll"),true);
	SetBool(_T("Audio SSA Mode"),false);
	SetBool(_T("Audio Medusa Timing Hotkeys"),false);

	SetBool(_T("Shift Times ByTime"),true);
	SetInt(_T("Shift Times Type"),0);
	SetInt(_T("Shift Times Length"),0);
	SetInt(_T("Shift Times Affect"),0);
	SetBool(_T("Shift Times Direction"),true);

	SetInt(_T("Tips current"),0);
	SetBool(_T("Show associations"),true);
	SetBool(_T("Maximized"),false);

	SetBool(_T("Find Match Case"),false);
	SetBool(_T("Find RegExp"),false);
	SetBool(_T("Find Update Video"),false);
	SetInt(_T("Find Affect"),0);
	SetInt(_T("Find Field"),0);

	SetInt(_T("Grid hide overrides"),1);
	for (int i=0;i<10;i++) SetBool(_T("Grid show column ") + IntegerToString(i),true);

	for (int i=0;i<9;i++) SetBool(wxString::Format(_T("Paste Over #%i"),i),false);
	SetBool(_T("Paste Over #9"),true);

	SetText(_T("Fonts Collector Destination"),_T("?script"));
	SetBool(_T("Fonts Collector Attachment"),0);
	SetBool(_T("Fonts Collector Archive"),1);

	SetInt(_T("Audio Display Height"),100);
	SetBool(_T("Audio Spectrum"),false);

	SetInt(_T("Timing processor key start before thres"),5);
	SetInt(_T("Timing processor key start after thres"),4);
	SetInt(_T("Timing processor key end before thres"),5);
	SetInt(_T("Timing processor key end after thres"),6);
	SetInt(_T("Timing processor adjascent thres"),300);
	SetBool(_T("Timing processor Enable lead-in"),true);
	SetBool(_T("Timing processor Enable lead-out"),true);
	SetBool(_T("Timing processor Enable keyframe"),true);
	SetBool(_T("Timing processor Enable adjascent"),true);
	SetFloat(_T("Timing processor adjascent bias"),1.0);

	SetText(_T("Select Text"),_T(""));
	SetInt(_T("Select Condition"),0);
	SetInt(_T("Select Field"),0);
	SetInt(_T("Select Action"),0);
	SetInt(_T("Select Mode"),1);
	SetBool(_T("Select Match case"),false);
	SetBool(_T("Select Match dialogues"),true);
	SetBool(_T("Select Match comments"),false);

	SetText(_T("Color Picker Recent"), _T("&H000000& &H0000FF& &H00FFFF& &H00FF00& &HFFFF00& &HFF0000& &HFF00FF& &HFFFFFF&"));
	SetInt(_T("Color Picker Mode"), 4);

	SetText(_T("Last open subtitles path"),_T(""));
	SetText(_T("Last open video path"),_T(""));
	SetText(_T("Last open audio path"),_T(""));
	SetText(_T("Last open timecodes path"),_T(""));
	SetText(_T("Last open keyframes path"),_T(""));
	SetText(_T("Last open automation path"),_T(""));
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
	if (curModType != MOD_OFF) optType[key.Lower()] = curModType;
	modified = true;
}


///////////////
// Write float
void OptionsManager::SetFloat(wxString key,double param) {
	opt[key.Lower()].SetFloat(param);
	if (curModType != MOD_OFF) optType[key.Lower()] = curModType;
	modified = true;
}


////////////////
// Write string
void OptionsManager::SetText(wxString key,wxString param) {
	opt[key.Lower()].SetText(param);
	if (curModType != MOD_OFF) optType[key.Lower()] = curModType;
	modified = true;
}


/////////////////
// Write boolean
void OptionsManager::SetBool(wxString key,bool param) {
	opt[key.Lower()].SetBool(param);
	if (curModType != MOD_OFF) optType[key.Lower()] = curModType;
	modified = true;
}


////////////////
// Write colour
void OptionsManager::SetColour(wxString key,wxColour param) {
	opt[key.Lower()].SetColour(param);
	if (curModType != MOD_OFF) optType[key.Lower()] = curModType;
	modified = true;
}


//////////////
// Reset with
void OptionsManager::ResetWith(wxString key,wxString param) {
	opt[key.Lower()].ResetWith(param);
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


/////////////////////
// Modification type
ModType OptionsManager::GetModType(wxString key) {
	std::map<wxString,ModType>::iterator cur;
	cur = (optType.find(key.Lower()));
	if (cur != optType.end()) {
		return (*cur).second;
	}
	else return MOD_AUTOMATIC;
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


/////////////////////////
// Set modification type
void OptionsManager::SetModificationType(ModType type) {
	curModType = type;
}


///////////////////
// Global instance
OptionsManager Options;
