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
#include <list>
#include <fstream>
#include "ass_file.h"
#include "ass_dialogue.h"
#include "ass_style.h"
#include "ass_override.h"
#include "ass_exporter.h"
#include "vfr.h"
#include "options.h"
#include "text_file_reader.h"
#include "text_file_writer.h"
#include "version.h"
#include "subtitle_format_reader.h"


////////////////////// AssFile //////////////////////
///////////////////////
// AssFile constructor
AssFile::AssFile () {
	AssOverrideTagProto::LoadProtos();
	Clear();
}


//////////////////////
// AssFile destructor
AssFile::~AssFile() {
	Clear();
}


/////////////////////
// Load generic subs
void AssFile::Load (const wxString _filename,const wxString charset) {
	bool ok = true;

	try {
		// Try to open file
		std::ifstream file;
		file.open(_filename.mb_str(wxConvLocal));
		if (!file.is_open()) {
			throw _T("Unable to open file \"") + _filename + _T("\". Check if it exists and if you have permissions to read it.");
		}
		file.close();

		// Find file encoding
		wxString enc;
		if (charset.IsEmpty()) enc = TextFileReader::GetEncoding(_filename);
		else enc = charset;
		TextFileReader::EnsureValid(enc);

		// Generic preparation
		Clear();
		IsASS = false;

		// Get proper format reader
		SubtitleFormatReader *reader = SubtitleFormatReader::GetReader(_filename);

		// Read file
		if (reader) {
			reader->SetTarget(this);
			reader->ReadFile(_filename,enc);
		}

		// Couldn't find a type
		else throw _T("Unknown file type.");
	}

	// String error
	catch (wchar_t *except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
		ok = false;
	}

	catch (wxString except) {
		wxMessageBox(except,_T("Error loading file"),wxICON_ERROR | wxOK);
		ok = false;
	}

	// Other error
	catch (...) {
		wxMessageBox(_T("Unknown error"),_T("Error loading file"),wxICON_ERROR | wxOK);
		ok = false;
	}

	// Verify loading
	if (ok) filename = _filename;
	else LoadDefault();

	// Set general data
	loaded = true;

	// Add comments and set vars
	AddComment(_T("Script generated by Aegisub ") + wxString(VERSION_STRING));
	AddComment(_T("http://www.aegisub.net"));
	SetScriptInfo(_T("ScriptType"),_T("v4.00+"));
	AddToRecent(_filename);
}


/////////////////////////////
// Chooses format to save in
void AssFile::Save(wxString _filename,bool setfilename,bool addToRecent,const wxString encoding) {
	// Finds last dot
	int i = 0;
	for (i=(int)_filename.size();--i>=0;) {
		if (_filename[i] == '.') break;
	}
	wxString extension = _filename.substr(i+1);
	extension.Lower();

	// ASS
	if (extension == _T("ass")) {
		SaveASS(_filename,setfilename,encoding);
		if (addToRecent) AddToRecent(_filename);
		return;
	}

	// SSA
	if (extension == _T("ssa")) {
		AssFile SSA(*this);
		SSA.SaveSSA(_filename,encoding);
		if (addToRecent) AddToRecent(_filename);
		return;
	}

	// SRT
	if (extension == _T("srt")) {
		AssFile SRT(*this);
		SRT.SaveSRT(_filename,encoding);
		if (addToRecent) AddToRecent(_filename);
		return;
	}

	// Unknown
	throw _T("Unknown file type");
}


////////////////////////////////////////////
// Exports file with proper transformations
void AssFile::Export(wxString _filename) {
	AssExporter exporter(this);
	exporter.AddAutoFilters();
	exporter.Export(_filename,_T("UTF-8"));
}


/////////////////////
// Saves ASS to disk
void AssFile::SaveASS (wxString _filename,bool setfilename,const wxString encoding) {
	// Open file
	TextFileWriter file(_filename,encoding);

	// Write lines
	using std::list;
	for (list<AssEntry*>::iterator cur=Line.begin();cur!=Line.end();cur++) {
		file.WriteLineToFile((*cur)->GetEntryData());
	}

	// Done
	if (setfilename) {
		Modified = false;
		filename = _filename;
		IsASS = true;
	}
}


/////////////////////
// Saves SSA to disk
void AssFile::SaveSSA (wxString _filename,const wxString encoding) {
	// Open file
	TextFileWriter file(_filename,encoding);

	// Convert to SSA
	ConvertToSSA();

	// Write lines
	using std::list;
	for (list<AssEntry*>::iterator cur=Line.begin();cur!=Line.end();cur++) {
		file.WriteLineToFile((*cur)->GetSSAText());
	}
}


//////////////////
// Convert to SSA
void AssFile::ConvertToSSA () {

}


////////////////////
// Save SRT to disk
// ----------------
// Note that this function will convert the whole AssFile to SRT
//
void AssFile::SaveSRT (wxString _filename,const wxString encoding) {
	// Open file
	TextFileWriter file(_filename,encoding);

	// Convert to SRT
	ConvertToSRT();

	// Write lines
	int i=1;
	using std::list;
	for (list<AssEntry*>::iterator cur=Line.begin();cur!=Line.end();cur++) {
		AssDialogue *current = AssEntry::GetAsDialogue(*cur);
		if (current) {
			// Get line
			if (current->Comment) throw _T("Unexpected line type (comment)");

			// Write line
			file.WriteLineToFile(wxString::Format(_T("%i"),i));
			file.WriteLineToFile(current->Start.GetSRTFormated() + _T(" --> ") + current->End.GetSRTFormated());
			file.WriteLineToFile(current->Text);
			file.WriteLineToFile(_T(""));

			i++;
		}
		else throw _T("Unexpected line type");
	}
}


///////////////////////
// Convert line to SRT
void AssFile::DialogueToSRT(AssDialogue *current,std::list<AssEntry*>::iterator prev) {
	using std::list;
	AssDialogue *previous;
	if (prev != Line.end()) previous = AssEntry::GetAsDialogue(*prev);
	else previous = NULL;

	// Strip ASS tags
	current->ConvertTagsToSRT();

	// Join equal lines
	if (previous != NULL) {
		if (previous->Text == current->Text) {
			if (abs(current->Start.GetMS() - previous->End.GetMS()) < 20) {
				current->Start = (current->Start < previous->Start ? current->Start : previous->Start);
				current->End = (current->End > previous->End ? current->End : previous->End);
				delete *prev;
				Line.erase(prev);
			}
		}
	}

	// Fix line breaks
	size_t cur = 0;
	while ((cur = current->Text.find(_T("\\n"),cur)) != wxString::npos) {
		current->Text.replace(cur,2,_T("\r\n"));
	}
	cur = 0;
	while ((cur = current->Text.find(_T("\\N"),cur)) != wxString::npos) {
		current->Text.replace(cur,2,_T("\r\n"));
	}
	cur = 0;
	while ((cur = current->Text.find(_T("\r\n\r\n"),cur)) != wxString::npos) {
		current->Text.replace(cur,2,_T("\r\n"));
		cur = 0;
	}
}


//////////////////////////////
// Converts whole file to SRT
void AssFile::ConvertToSRT () {
	using std::list;
	list<AssEntry*>::iterator next;
	list<AssEntry*>::iterator prev = Line.end();

	// Sort lines
	Line.sort(LessByPointedToValue<AssEntry>());

	// Process lines
	bool notfirst = false;
	for (list<AssEntry*>::iterator cur=Line.begin();cur!=Line.end();cur=next) {
		next = cur;
		next++;

		// Dialogue line (not comment)
		AssDialogue *current = AssEntry::GetAsDialogue(*cur);
		if (current && !current->Comment) {
			DialogueToSRT(current,prev);
			notfirst = true;
			prev = cur;
		}

		// Other line, delete it
		else {
			delete *cur;
			Line.erase(cur);
		}
	}
}


///////////////////////
// Appends line to Ass
int AssFile::AddLine (wxString data,wxString group,int lasttime,bool &IsSSA) {
	AssEntry *entry = NULL;

	// Dialogue
	if (group == _T("[Events]")) {
		if ((data.Left(9) == _T("Dialogue:") || data.Left(8) == _T("Comment:"))) {
			AssDialogue *diag = new AssDialogue(data,IsSSA);
			lasttime = diag->Start.GetMS();
			//diag->ParseASSTags();
			entry = diag;
			entry->StartMS = lasttime;
			entry->group = group;
		}
		if (data.Left(7) == _T("Format:")) {
			entry = new AssEntry(_T("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"));
			entry->StartMS = lasttime;
			entry->group = group;
		}
	}

	// Style
	else if (group == _T("[V4+ Styles]")) {
		if (data.Left(6) == _T("Style:")) {
			AssStyle *style = new AssStyle(data,IsSSA);
			entry = style;
			entry->StartMS = lasttime;
			entry->group = group;
		}
		if (data.Left(7) == _T("Format:")) {
			entry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
			entry->StartMS = lasttime;
			entry->group = group;
		}
	}

	// Comment in script info
	else if (group == _T("[Script Info]")) {
		if (data.Left(1) == _T(";")) {
			// Skip stupid comments added by other programs
			// Of course, we add our own in place...
			return lasttime;
		}
		if (data.Left(11) == _T("ScriptType:")) {
			wxString version = data.Mid(11);
			version.Trim(true);
			version.Trim(false);
			version.MakeLower();
			bool trueSSA;
			if (version == _T("v4.00")) trueSSA = true;
			else if (version == _T("v4.00+")) trueSSA = false;
			else throw _T("Unknown file version");
			if (trueSSA != IsSSA) {
				wxLogMessage(_T("Warning: File has the wrong extension."));
				IsSSA = trueSSA;
			}
		}
		entry = new AssEntry(data);
		entry->StartMS = lasttime;
		entry->group = group;
	}

	// Common entry
	if (entry == NULL) {
		entry = new AssEntry(data);
		entry->StartMS = lasttime;
		entry->group = group;
	}

	Line.push_back(entry);
	return lasttime;
}


//////////////////////////////
// Clears contents of assfile
void AssFile::Clear () {
	for (std::list<AssEntry*>::iterator cur=Line.begin();cur != Line.end();cur++) {
		if (*cur) delete *cur;
	}
	Line.clear();

	IsASS = false;
	loaded = false;
	filename = _T("");
	Modified = false;
}


//////////////////////
// Loads default subs
void AssFile::LoadDefault (bool defline) {
	// Clear first
	Clear();

	// Write headers
	AssStyle defstyle;
	bool IsSSA = false;
	AddLine(_T("[Script Info]"),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T("Title: Default Aegisub file"),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T("ScriptType: v4.00+"),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T("PlayResX: 640"),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T("PlayResY: 480"),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T(""),_T("[Script Info]"),-1,IsSSA);
	AddLine(_T("[V4+ Styles]"),_T("[V4+ Styles]"),-1,IsSSA);
	AddLine(_T("Format:  Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"),_T("[V4+ Styles]"),-1,IsSSA);
	AddLine(defstyle.GetEntryData(),_T("[V4+ Styles]"),-1,IsSSA);
	AddLine(_T(""),_T("[V4+ Styles]"),-1,IsSSA);
	AddLine(_T("[Events]"),_T("[Events]"),-1,IsSSA);
	AddLine(_T("Format:  Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text"),_T("[Events]"),-1,IsSSA);

	if (defline) {
		AssDialogue def;
		AddLine(def.GetEntryData(),_T("[Events]"),0,IsSSA);
	}

	loaded = true;
	IsASS = true;
}


////////////////////
// Copy constructor
AssFile::AssFile (AssFile &from) {
	using std::list;

	// Copy standard variables
	filename = from.filename;
	IsASS = from.IsASS;
	loaded = from.loaded;
	Modified = from.Modified;
	bool IsSSA = false;

	// Copy lines
	int lasttime = -1;
	for (list<AssEntry*>::iterator cur=from.Line.begin();cur!=from.Line.end();cur++) {
		lasttime = AddLine((*cur)->GetEntryData(),(*cur)->group,lasttime,IsSSA);
	}

	// Add comments
	AddComment(_T("Script generated by Aegisub"));
	AddComment(_T("http://www.aegisub.net"));
}


//////////////////////
// Insert a new style
void AssFile::InsertStyle (AssStyle *style) {
	// Variables
	using std::list;
	AssEntry *curEntry;
	list<AssEntry*>::iterator lastStyle = Line.end();
	list<AssEntry*>::iterator cur;
	int lasttime;
	wxString lastGroup;

	// Look for insert position
	for (cur=Line.begin();cur!=Line.end();cur++) {
		curEntry = *cur;
		if (curEntry->GetType() == ENTRY_STYLE || (lastGroup == _T("[V4+ Styles]") && curEntry->GetEntryData().substr(0,7) == _T("Format:"))) {
			lastStyle = cur;
		}
		lasttime = curEntry->StartMS;
		lastGroup = curEntry->group;
	}

	// No styles found, add them
	if (lastStyle == Line.end()) {
		// Add space
		curEntry = new AssEntry(_T(""));
		curEntry->group = lastGroup;
		curEntry->StartMS = lasttime;
		Line.push_back(curEntry);

		// Add header
		curEntry = new AssEntry(_T("[V4+ Styles]"));
		curEntry->group = _T("[V4+ Styles]");
		curEntry->StartMS = lasttime;
		Line.push_back(curEntry);

		// Add format line
		curEntry = new AssEntry(_T("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding"));
		curEntry->group = _T("[V4+ Styles]");
		curEntry->StartMS = lasttime;
		Line.push_back(curEntry);

		// Add style
		style->group = _T("[V4+ Styles]");
		style->StartMS = lasttime;
		Line.push_back(style);
	}

	// Add to end of list
	else {
		lastStyle++;
		style->group = (*lastStyle)->group;
		style->StartMS = lasttime;
		Line.insert(lastStyle,style);
	}
}


////////////////////
// Gets script info
wxString AssFile::GetScriptInfo(const wxString _key) {
	// Prepare
	wxString key = _key;;
	key.Lower();
	key += _T(":");
	std::list<AssEntry*>::iterator cur;
	bool GotIn = false;

	// Look for it
	for (cur=Line.begin();cur!=Line.end();cur++) {
		if ((*cur)->group == _T("[Script Info]")) {
			GotIn = true;
			wxString curText = (*cur)->GetEntryData();
			curText.Lower();

			// Found
			if (curText.Left(key.length()) == key) {
				wxString result = curText.Mid(key.length());
				result.Trim(false);
				result.Trim(true);
				return result;
			}
		}
		else if (GotIn) break;
	}

	// Couldn't find
	return _T("");
}


//////////////////////////
// Get script info as int
int AssFile::GetScriptInfoAsInt(const wxString key) {
	long temp = 0;
	try {
		GetScriptInfo(key).ToLong(&temp);
	}
	catch (...) {
		temp = 0;
	}
	return temp;
}


//////////////////////////
// Set a script info line
void AssFile::SetScriptInfo(const wxString _key,const wxString value) {
	// Prepare
	wxString key = _key;;
	key.Lower();
	key += _T(":");
	std::list<AssEntry*>::iterator cur;
	std::list<AssEntry*>::iterator prev;
	bool GotIn = false;

	// Look for it
	for (cur=Line.begin();cur!=Line.end();cur++) {
		if ((*cur)->group == _T("[Script Info]")) {
			GotIn = true;
			wxString curText = (*cur)->GetEntryData();
			curText.Lower();

			// Found
			if (curText.Left(key.length()) == key) {
				// Set value
				if (value != _T("")) {
					wxString result = _key;
					result += _T(": ");
					result += value;
					(*cur)->SetEntryData(result);
				}

				// Remove key
				else {
					Line.erase(cur);
					return;
				}
				return;
			}

			if (!(*cur)->GetEntryData().empty()) prev = cur;
		}

		// Add
		else if (GotIn) {
			if (value != _T("")) {
				wxString result = _key;
				result += _T(": ");
				result += value;
				AssEntry *entry = new AssEntry(result);
				entry->group = (*prev)->group;
				entry->StartMS = (*prev)->StartMS;
				Line.insert(++prev,entry);
			}
			return;
		}
	}
}


///////////////////////////////////
// Adds a comment to [Script Info]
void AssFile::AddComment(const wxString _comment) {
	wxString comment = _T("; ");
	comment += _comment;
	std::list<AssEntry*>::iterator cur;
	int step = 0;

	// Find insert position
	for (cur=Line.begin();cur!=Line.end();cur++) {
		// Start of group
		if (step == 0 && (*cur)->group == _T("[Script Info]")) step = 1;

		// First line after a ;
		else if (step == 1 && (*cur)->GetEntryData().Left(1) != _T(";")) {
			AssEntry *prev = *cur;
			AssEntry *comm = new AssEntry(comment);
			comm->group = prev->group;
			comm->StartMS = prev->StartMS;
			Line.insert(cur,comm);
			break;
		}
	}
}


//////////////////////
// Get list of styles
wxArrayString AssFile::GetStyles() {
	wxArrayString styles;
	AssStyle *curstyle;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		curstyle = AssEntry::GetAsStyle(*cur);
		if (curstyle) {
			styles.Add(curstyle->name);
		}
		if (!curstyle && (*cur)->GetEntryData().Left(5) == _T("Style")) {
			wxLogMessage(_T("Style ignored: ") + (*cur)->GetEntryData());
			curstyle = AssEntry::GetAsStyle(*cur);
		}
	}
	if (styles.GetCount() == 0) styles.Add(_T("Default"));
	return styles;
}


///////////////////////////////
// Gets style of specific name
AssStyle *AssFile::GetStyle(wxString name) {
	AssStyle *curstyle;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		curstyle = AssEntry::GetAsStyle(*cur);
		if (curstyle) {
			if (curstyle->name == name) return curstyle;
		}
	}
	return NULL;
}


////////////////////////////////////
// Adds file name to list of recent
void AssFile::AddToRecent(wxString file) {
	Options.AddToRecentList(file,_T("Recent sub"));
}


////////////////////////////////////////////
// Compress/decompress for storage on stack
void AssFile::CompressForStack(bool compress) {
	AssDialogue *diag;
	for (entryIter cur=Line.begin();cur!=Line.end();cur++) {
		diag = AssEntry::GetAsDialogue(*cur);
		if (diag) {
			if (compress) {
				diag->SetEntryData(_T(""));
				diag->ClearBlocks();
			}
			else diag->UpdateData();
		}
	}
}


//////////////////////////////
// Checks if file is modified
bool AssFile::IsModified() {
	return Modified;
}


/////////////////////////
// Flag file as modified
void AssFile::FlagAsModified() {
	// Clear redo
	if (!RedoStack.empty()) {
		//StackPush();
		//UndoStack.push_back(new AssFile(*UndoStack.back()));
		for (std::list<AssFile*>::iterator cur=RedoStack.begin();cur!=RedoStack.end();cur++) {
			delete *cur;
		}
		RedoStack.clear();
	}

	Modified = true;
	StackPush();
}


//////////////
// Stack push
void AssFile::StackPush() {
	// Places copy on stack
	AssFile *curcopy = new AssFile(*top);
	curcopy->CompressForStack(true);
	UndoStack.push_back(curcopy);
	StackModified = true;

	// Cap depth
	int n = 0;
	for (std::list<AssFile*>::iterator cur=UndoStack.begin();cur!=UndoStack.end();cur++) {
		n++;
	}
	int depth = Options.AsInt(_T("Undo levels"));
	while (n > depth) {
		delete UndoStack.front();
		UndoStack.pop_front();
		n--;
	}
}


/////////////
// Stack pop
void AssFile::StackPop() {
	bool addcopy = false;
	if (StackModified) {
		UndoStack.pop_back();
		StackModified = false;
		addcopy = true;
	}

	if (!UndoStack.empty()) {
		//delete top;
		top->CompressForStack(true);
		RedoStack.push_back(top);
		top = UndoStack.back();
		top->CompressForStack(false);
		UndoStack.pop_back();
		Popping = true;
	}

	if (addcopy) {
		StackPush();
	}
}


//////////////
// Stack redo
void AssFile::StackRedo() {

	bool addcopy = false;
	if (StackModified) {
		UndoStack.pop_back();
		StackModified = false;
		addcopy = true;
	}

	if (!RedoStack.empty()) {
		top->CompressForStack(true);
		UndoStack.push_back(top);
		top = RedoStack.back();
		top->CompressForStack(false);
		RedoStack.pop_back();
		Popping = true;
		//StackModified = false;
	}

	if (addcopy) {
		StackPush();
	}
}


///////////////
// Stack clear
void AssFile::StackClear() {
	// Clear undo
	for (std::list<AssFile*>::iterator cur=UndoStack.begin();cur!=UndoStack.end();cur++) {
		delete *cur;
	}
	UndoStack.clear();

	// Clear redo
	for (std::list<AssFile*>::iterator cur=RedoStack.begin();cur!=RedoStack.end();cur++) {
		delete *cur;
	}
	RedoStack.clear();

	Popping = false;
}


///////////////
// Stack reset
void AssFile::StackReset() {
	StackClear();
	delete top;
	top = new AssFile;
	StackModified = false;
}


//////////////////////////////////
// Returns if undo stack is empty
bool AssFile::IsUndoStackEmpty() {
	if (StackModified) return (UndoStack.size() <= 1);
	else return UndoStack.empty();
}


//////////////////////////////////
// Returns if redo stack is empty
bool AssFile::IsRedoStackEmpty() {
	return RedoStack.empty();
}


//////////
// Global
AssFile *AssFile::top;
std::list<AssFile*> AssFile::UndoStack;
std::list<AssFile*> AssFile::RedoStack;
bool AssFile::Popping;
bool AssFile::StackModified;

