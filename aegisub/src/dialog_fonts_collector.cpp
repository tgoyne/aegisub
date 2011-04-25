// Copyright (c) 2007, Rodrigo Braz Monteiro
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

/// @file dialog_fonts_collector.cpp
/// @brief Font collector dialogue box
/// @ingroup tools_ui font_collector
///

#include "config.h"

#ifndef AGI_PRE
#include <wx/config.h>
#include <wx/filename.h>
#include <wx/fontenum.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#endif

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "compat.h"
#include "dialog_fonts_collector.h"
#include "font_file_lister.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "scintilla_text_ctrl.h"
#include "selection_controller.h"
#include "standard_paths.h"
#include "utils.h"

enum {
	START_BUTTON = 1150,
	BROWSE_BUTTON,
	RADIO_BOX
};

DECLARE_EVENT_TYPE(EVT_ADD_TEXT, -1)
DEFINE_EVENT_TYPE(EVT_ADD_TEXT)

DialogFontsCollector::DialogFontsCollector(wxWindow *parent, AssFile *ass)
: wxDialog(parent,-1,_("Fonts Collector"),wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
, subs(ass)
{
	SetIcon(BitmapToIcon(GETIMAGE(font_collector_button_24)));

	// Destination box
	wxString dest = lagi_wxString(OPT_GET("Path/Fonts Collector Destination")->GetString());
	if (dest == _T("?script")) {
		wxFileName filename(subs->filename);
		dest = filename.GetPath();
	}
	while (dest.Right(1) == _T("/")) dest = dest.Left(dest.Length()-1);
	DestBox = new wxTextCtrl(this,-1,dest,wxDefaultPosition,wxSize(250,20),0);
	BrowseButton = new wxButton(this,BROWSE_BUTTON,_("&Browse..."));
	wxSizer *DestBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	DestLabel = new wxStaticText(this,-1,_("Choose the folder where the fonts will be collected to.\nIt will be created if it doesn't exist."));
	DestBottomSizer->Add(DestBox,1,wxEXPAND | wxRIGHT,5);
	DestBottomSizer->Add(BrowseButton,0,0,0);
	wxSizer *DestSizer = new wxStaticBoxSizer(wxVERTICAL,this,_("Destination"));
	DestSizer->Add(DestLabel,0,wxEXPAND | wxBOTTOM,5);
	DestSizer->Add(DestBottomSizer,0,wxEXPAND,0);

	// Action radio box
	wxArrayString choices;
	choices.Add(_("Check fonts for availability"));
	choices.Add(_("Copy fonts to folder"));
	choices.Add(_("Copy fonts to subtitle file's folder"));
	choices.Add(_("Copy fonts to zipped archive"));
	CollectAction = new wxRadioBox(this,RADIO_BOX,_T("Action"),wxDefaultPosition,wxDefaultSize,choices,1);
	size_t lastAction = OPT_GET("Tool/Fonts Collector/Action")->GetInt();
	if (lastAction >= choices.GetCount()) lastAction = 0;
	CollectAction->SetSelection(lastAction);
	if (ass->filename.empty())
		CollectAction->Enable(2, false);

	// Log box
	LogBox = new ScintillaTextCtrl(this,-1,_T(""),wxDefaultPosition,wxSize(300,210));
	LogBox->SetWrapMode(wxSTC_WRAP_WORD);
	LogBox->SetMarginWidth(1,0);
	LogBox->SetReadOnly(true);
	LogBox->StyleSetForeground(1,wxColour(0,200,0));
	LogBox->StyleSetForeground(2,wxColour(200,0,0));
	LogBox->StyleSetForeground(3,wxColour(200,100,0));
	wxSizer *LogSizer = new wxStaticBoxSizer(wxVERTICAL,this,_("Log"));
	LogSizer->Add(LogBox,1,wxEXPAND,0);

	// Buttons sizer
	StartButton = new wxButton(this,START_BUTTON,_("&Start!"));
	CloseButton = new wxButton(this,wxID_CANCEL);
	StartButton->SetDefault();
	wxStdDialogButtonSizer *ButtonSizer = new wxStdDialogButtonSizer();
	ButtonSizer->AddButton(StartButton);
	ButtonSizer->AddButton(CloseButton);
	ButtonSizer->AddButton(new HelpButton(this,_T("Fonts Collector")));
	ButtonSizer->SetAffirmativeButton(StartButton);
	ButtonSizer->Realize();

	// Main sizer
	wxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(CollectAction,0,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);
	MainSizer->Add(DestSizer,0,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);
	MainSizer->Add(LogSizer,0,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);
	MainSizer->Add(ButtonSizer,0,wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM,5);

	// Set sizer
	SetSizer(MainSizer);
	MainSizer->SetSizeHints(this);
	CenterOnParent();

	// Run dummy event to update label
	Update();
}

DialogFontsCollector::~DialogFontsCollector() {
}

BEGIN_EVENT_TABLE(DialogFontsCollector, wxDialog)
	EVT_BUTTON(START_BUTTON,DialogFontsCollector::OnStart)
	EVT_BUTTON(BROWSE_BUTTON,DialogFontsCollector::OnBrowse)
	EVT_BUTTON(wxID_CLOSE,DialogFontsCollector::OnClose)
	EVT_RADIOBOX(RADIO_BOX,DialogFontsCollector::OnRadio)
	EVT_COMMAND(0,EVT_ADD_TEXT,DialogFontsCollector::OnAddText)
END_EVENT_TABLE()

void DialogFontsCollector::OnStart(wxCommandEvent &event) {
	LogBox->SetReadOnly(false);
	LogBox->ClearAll();
	LogBox->SetReadOnly(true);

	wxString dest;
	int action = CollectAction->GetSelection();
	OPT_SET("Tool/Fonts Collector/Action")->SetInt(action);
	if (action > 0) {
		if (action == 2) {
			action = 1;
			dest = "?script";
		}
		else {
			if (action == 3) action = 2;
			dest = DestBox->GetValue();
		}
		dest = StandardPaths::DecodePath(dest);
		wxFileName folder = dest;

		if (action != 2) {
			if (dest.Last() != '/' && dest.Last() != '\\') {
				dest += "/";
				folder = dest;
			}
			if (folder.FileExists())
		wxMessageBox(_("Invalid destination."),_("Error"),wxICON_EXCLAMATION | wxOK);
			if (!folder.DirExists())
			folder.Mkdir(0777,wxPATH_MKDIR_FULL);
			if (!folder.DirExists()) {
				wxMessageBox(_("Could not create destination folder."),_("Error"),wxICON_EXCLAMATION | wxOK);
				return;
			}
		}
		else if (folder.IsDir() || folder.GetName().empty()) {
			wxMessageBox(_("Invalid path for .zip file."),_("Error"),wxICON_EXCLAMATION | wxOK);
			return;
		}
	}

	// Start thread
	wxThread *worker = new FontsCollectorThread(subs,dest,action,this);
	worker->Create();
	worker->Run();

	if (action > 0)
		OPT_SET("Path/Fonts Collector Destination")->SetString(STD_STR(dest));

	// Set buttons
	StartButton->Enable(false);
	BrowseButton->Enable(false);
	DestBox->Enable(false);
	CloseButton->Enable(false);
	CollectAction->Enable(false);
	DestLabel->Enable(false);
	if (!worker->IsDetached()) worker->Wait();
}

void DialogFontsCollector::OnBrowse(wxCommandEvent &event) {
	if (CollectAction->GetSelection() == 3) {
		wxFileName fname(DestBox->GetValue());
		wxString dest = wxFileSelector(_("Select archive file name"),DestBox->GetValue(),fname.GetFullName(),_T(".zip"),_("Zip Archives (*.zip)|*.zip"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
		if (!dest.empty()) {
			DestBox->SetValue(dest);
		}
	}

	// Choose folder
	else {
		wxString dest = wxDirSelector(_("Select folder to save fonts on"),DestBox->GetValue(),0);
		if (!dest.empty()) {
			DestBox->SetValue(dest);
		}
	}
}

void DialogFontsCollector::OnRadio(wxCommandEvent &event) {
	Update(event.GetInt());
}

void DialogFontsCollector::Update(int value) {
	CloseButton->Enable(true);
	StartButton->Enable(true);
	CollectAction->Enable(true);
	wxString dst = DestBox->GetValue();

	// Get value if -1
	if (value == -1) {
		value = CollectAction->GetSelection();
	}

	// Check or attach
	if (value == 0 || value == 2) {
		DestBox->Enable(false);
		BrowseButton->Enable(false);
		DestLabel->Enable(false);
		DestLabel->SetLabel(_T("N/A\n"));
	}

	// Collect to folder
	else if (value == 1) {
		DestBox->Enable(true);
		BrowseButton->Enable(true);
		DestLabel->Enable(true);
		DestLabel->SetLabel(_("Choose the folder where the fonts will be collected to.\nIt will be created if it doesn't exist."));

		// Remove filename from browse box
		if (dst.Right(4) == _T(".zip")) {
			wxFileName fn(dst);
			DestBox->SetValue(fn.GetPath());
		}
	}

	// Collect to zip
	else if (value == 2) {
		DestBox->Enable(true);
		BrowseButton->Enable(true);
		DestLabel->Enable(true);
		DestLabel->SetLabel(_("Enter the name of the destination zip file to collect the fonts to.\nIf a folder is entered, a default name will be used."));

		// Add filename to browse box
		if (dst.Right(4) != _T(".zip")) {
			wxFileName fn(dst + _T("//"));
			fn.SetFullName(_T("fonts.zip"));
			DestBox->SetValue(fn.GetFullPath());
		}
	}
}

void DialogFontsCollector::OnAddText(wxCommandEvent &event) {
	std::pair<int, wxString> *str = (std::pair<int, wxString>*) event.GetClientData();
	LogBox->SetReadOnly(false);
	int pos = LogBox->GetReverseUnicodePosition(LogBox->GetLength());
	LogBox->AppendText(str->second);
	if (str->first) {
		LogBox->StartUnicodeStyling(pos,31);
		LogBox->SetUnicodeStyling(pos,str->second.Length(),str->first);
	}
	delete str;
	LogBox->GotoPos(pos);
	LogBox->SetReadOnly(true);
}

FontsCollectorThread::FontsCollectorThread(AssFile *subs, wxString destination, int oper, DialogFontsCollector *collector)
: wxThread(wxTHREAD_DETACHED)
, subs(subs)
, destination(destination)
, oper(oper)
, collector(collector)
{
}

wxThread::ExitCode FontsCollectorThread::Entry() {
	try {
		Collect();
	}
	catch (...) {
		collector->Update();
		if (IsDetached() && TestDestroy()) Delete();
		throw;
	}

	// After done, restore status
	collector->Update();

	return 0;
}

void FontsCollectorThread::Collect() {
	using namespace std::tr1::placeholders;
	std::vector<wxString> paths =
		FontFileLister::GetFontPaths(
			subs->Line,
			std::tr1::bind(&FontsCollectorThread::AppendText, this, _1, _2));


	if (oper == 0 || paths.empty()) return;

	// Copy fonts
	switch (oper) {
		case 1: AppendText(_("Copying fonts to folder...\n")); break;
		case 2: AppendText(_("Copying fonts to archive...\n")); break;
		case 3: AppendText(_("Attaching fonts to file...\n")); break;
	}

	// Open zip stream if saving to compressed archive
	std::auto_ptr<wxFFileOutputStream> out;
	std::auto_ptr<wxZipOutputStream> zip;
	if (oper == 2) {
		out.reset(new wxFFileOutputStream(destination));
		zip.reset(new wxZipOutputStream(*out));
	}

	bool allOk = true;
	for (std::vector<wxString>::iterator cur = paths.begin(); cur != paths.end(); ++cur) {
		int ret = 0;
		switch (oper) {
			case 1: ret = CopyFont(*cur); break;
			case 2: ret = ArchiveFont(*cur, *zip); break;
		}

		if (ret == 1) {
			AppendText(wxString::Format(_("* Copied %s.\n"), cur->c_str()),1);
		}
		else if (ret == 2) {
			wxFileName fn(*cur);
			AppendText(wxString::Format(_("* %s already exists on destination.\n"), fn.GetFullName().c_str()), 3);
		}
		else {
			AppendText(wxString::Format(_("* Failed to copy %s.\n"), cur->c_str()), 2);
			allOk = false;
		}
	}

	if (allOk) {
		AppendText(_("Done. All fonts copied."), 1);
	}
	else {
		AppendText(_("Done. Some fonts could not be copied."), 2);
	}
}

int FontsCollectorThread::CopyFont(wxString const& filename) {
	wxFileName fn(filename);
	wxString dstName = destination + fn.GetFullName();
	if (wxFileName::FileExists(dstName)) return 2;
	return wxCopyFile(filename ,dstName, true);
}

bool FontsCollectorThread::ArchiveFont(wxString const& filename, wxZipOutputStream &zip) {
	wxFFileInputStream in(filename);
	if (!in.IsOk()) return false;

	// Write to archive
	try {
		wxFileName fn(filename);
		zip.PutNextEntry(fn.GetFullName());
		zip.Write(in);
	}
	catch (...) {
		return false;
	}

	return true;
}

void FontsCollectorThread::AppendText(wxString text, int colour) {
	wxCommandEvent event(EVT_ADD_TEXT, 0);
	event.SetClientData(new std::pair<int, wxString>(colour, text));
	collector->AddPendingEvent(event);
}
