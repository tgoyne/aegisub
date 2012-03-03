// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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
//
// $Id$

/// @file dialog_fonts_collector.cpp
/// @brief Font collector dialogue box
/// @ingroup tools_ui font_collector
///

#include "config.h"

#include "dialog_fonts_collector.h"

#include "font_file_lister.h"
#include "font_file_lister_fontconfig.h"
#include "font_file_lister_freetype.h"

#include "ass_file.h"
#include "compat.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "scintilla_text_ctrl.h"
#include "selection_controller.h"
#include "standard_paths.h"
#include "utils.h"

#include <libaegisub/scoped_ptr.h>

#ifndef AGI_PRE
#include <wx/button.h>
#include <wx/config.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#endif

enum FcMode {
	CheckFontsOnly = 0,
	CopyToFolder = 1,
	CopyToScriptFolder = 2,
	CopyToZip = 3
};

wxDEFINE_EVENT(EVT_ADD_TEXT, wxThreadEvent);
wxDEFINE_EVENT(EVT_COLLECTION_DONE, wxThreadEvent);

/// @class FontsCollectorThread
/// @brief Worker thread for the font collector dialog
class FontsCollectorThread : public wxThread {
	AssFile *subs;           ///< Subtitle file to process
	wxString destination;    ///< Path to write fonts to for modes 2 and 3
	wxEvtHandler *collector; ///< Parent dialog
	FcMode oper;             ///< Copying mode

	void Collect() {
		using namespace std::tr1::placeholders;

		FontCollectorStatusCallback callback(bind(&FontsCollectorThread::AppendText, this, _1, _2));

#ifdef WITH_FONTCONFIG
		FontConfigFontFileLister lister(callback);
#elif defined(WITH_FREETYPE2)
		FreetypeFontFileLister lister(callback);
#else
		AppendText(_("Aegisub was built without any font file listers enabled"), 2);
		struct DummyLister : public FontFileLister {
			std::vector<wxString> GetFontPaths(wxString const&, int, bool, std::set<wxUniChar> const&) { return std::vector<wxString>(); }
		} lister;
#endif
		std::vector<wxString> paths = FontCollector(callback, lister).GetFontPaths(subs->Line);
		if (paths.empty()) return;

		// Copy fonts
		switch (oper) {
			case CheckFontsOnly: return;
			case CopyToScriptFolder:
			case CopyToFolder: AppendText(_("Copying fonts to folder...\n"));  break;
			case CopyToZip:    AppendText(_("Copying fonts to archive...\n")); break;
		}

		// Open zip stream if saving to compressed archive
		agi::scoped_ptr<wxFFileOutputStream> out;
		agi::scoped_ptr<wxZipOutputStream> zip;
		if (oper == CopyToZip) {
			out.reset(new wxFFileOutputStream(destination));
			zip.reset(new wxZipOutputStream(*out));
		}

		bool allOk = true;
		for (std::vector<wxString>::iterator cur = paths.begin(); cur != paths.end(); ++cur) {
			int ret = 0;
			switch (oper) {
				case CopyToScriptFolder:
				case CopyToFolder: {
					wxString dest = destination + wxFileName(*cur).GetFullName();
					if (wxFileName::FileExists(dest))
						ret = 2;
					else
						ret = wxCopyFile(*cur, dest, true);
				}
				break;
				case CopyToZip: {
					wxFFileInputStream in(*cur);
					if (!in.IsOk())
						ret = false;
					else {
						ret = zip->PutNextEntry(wxFileName(*cur).GetFullName());
						zip->Write(in);
					}
				}
				default: break;
			}

			if (ret == 1)
				AppendText(wxString::Format(_("* Copied %s.\n"), *cur), 1);
			else if (ret == 2)
				AppendText(wxString::Format(_("* %s already exists on destination.\n"), wxFileName(*cur).GetFullName()), 3);
			else {
				AppendText(wxString::Format(_("* Failed to copy %s.\n"), *cur), 2);
				allOk = false;
			}
		}

		if (allOk)
			AppendText(_("Done. All fonts copied."), 1);
		else
			AppendText(_("Done. Some fonts could not be copied."), 2);
	}

	/// @brief Tell the dialog to add text to the textbox
	/// @param text Text to add
	/// @param colour 0: neutral; 1: success; 2: error
	void AppendText(wxString text, int colour=0) {
		wxThreadEvent event(EVT_ADD_TEXT);
		event.SetPayload(std::make_pair(colour, text));
		collector->AddPendingEvent(event);
	}

	/// Do the font collection
	wxThread::ExitCode Entry() {
		Collect();
		collector->AddPendingEvent(wxThreadEvent(EVT_COLLECTION_DONE));
		return 0;
	}
public:
	FontsCollectorThread(AssFile *subs, wxString destination, FcMode oper, wxEvtHandler *collector)
	: wxThread(wxTHREAD_DETACHED)
	, subs(subs)
	, destination(destination)
	, collector(collector)
	, oper(oper)
	{
		Create();
		Run();
	}
};

DialogFontsCollector::DialogFontsCollector(wxWindow *parent, AssFile *ass)
: wxDialog(parent, -1, _("Fonts Collector"))
, subs(ass)
{
	SetIcon(BitmapToIcon(GETIMAGE(font_collector_button_24)));

	wxString modes[] = {
		_("Check fonts for availability"),
		_("Copy fonts to folder"),
		_("Copy fonts to subtitle file's folder"),
		_("Copy fonts to zipped archive")
	};
	collection_mode = new wxRadioBox(this, -1, "Action", wxDefaultPosition, wxDefaultSize, 4, modes, 1);
	collection_mode->SetSelection(mid<int>(0, OPT_GET("Tool/Fonts Collector/Action")->GetInt(), 3));

	if (!ass->filename)
		collection_mode->Enable(2, false);

	wxStaticBoxSizer *destination_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Destination"));

	dest_label = new wxStaticText(destination_box->GetStaticBox(), -1, " ");
	dest_ctrl = new wxTextCtrl(destination_box->GetStaticBox(), -1, StandardPaths::DecodePath(lagi_wxString(OPT_GET("Path/Fonts Collector Destination")->GetString())));
	dest_browse_button = new wxButton(destination_box->GetStaticBox(), -1, _("&Browse..."));

	wxSizer *dest_browse_sizer = new wxBoxSizer(wxHORIZONTAL);
	dest_browse_sizer->Add(dest_ctrl, wxSizerFlags(1).Border(wxRIGHT));
	dest_browse_sizer->Add(dest_browse_button, wxSizerFlags());

	destination_box->Add(dest_label, wxSizerFlags().Border(wxBOTTOM));
	destination_box->Add(dest_browse_sizer, wxSizerFlags().Expand());

	wxStaticBoxSizer *log_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Log"));
	collection_log = new ScintillaTextCtrl(log_box->GetStaticBox(), -1, "", wxDefaultPosition, wxSize(600, 300));
	collection_log->SetWrapMode(wxSTC_WRAP_WORD);
	collection_log->SetMarginWidth(1, 0);
	collection_log->SetReadOnly(true);
	collection_log->StyleSetForeground(1, wxColour(0, 200, 0));
	collection_log->StyleSetForeground(2, wxColour(200, 0, 0));
	collection_log->StyleSetForeground(3, wxColour(200, 100, 0));
	log_box->Add(collection_log, wxSizerFlags().Border());

	wxStdDialogButtonSizer *button_sizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL | wxHELP);
	start_btn = button_sizer->GetAffirmativeButton();
	close_btn = button_sizer->GetCancelButton();
	start_btn->SetLabel(_("&Start!"));
	start_btn->SetDefault();

	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(collection_mode, wxSizerFlags().Expand().Border());
	main_sizer->Add(destination_box, wxSizerFlags().Expand().Border(wxALL & ~wxTOP));
	main_sizer->Add(log_box, wxSizerFlags().Border(wxALL & ~wxTOP));
	main_sizer->Add(button_sizer, wxSizerFlags().Right().Border(wxALL & ~wxTOP));

	SetSizerAndFit(main_sizer);
	CenterOnParent();

	// Update the browse button and label
	wxCommandEvent evt;
	OnRadio(evt);

	start_btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogFontsCollector::OnStart, this);
	dest_browse_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogFontsCollector::OnBrowse, this);
	collection_mode->Bind(wxEVT_COMMAND_RADIOBOX_SELECTED, &DialogFontsCollector::OnRadio, this);
	button_sizer->GetHelpButton()->Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::tr1::bind(&HelpButton::OpenPage, "Fonts Collector"));
	Bind(EVT_ADD_TEXT, &DialogFontsCollector::OnAddText, this);
	Bind(EVT_COLLECTION_DONE, &DialogFontsCollector::OnCollectionComplete, this);
}

void DialogFontsCollector::OnStart(wxCommandEvent &) {
	collection_log->SetReadOnly(false);
	collection_log->ClearAll();
	collection_log->SetReadOnly(true);

	wxString dest;
	int action = collection_mode->GetSelection();
	OPT_SET("Tool/Fonts Collector/Action")->SetInt(action);
	if (action != CheckFontsOnly) {
		if (action == CopyToScriptFolder)
			dest = "?script/";
		else
			dest = dest_ctrl->GetValue();
		dest = StandardPaths::DecodePath(dest);
		wxFileName folder = dest;

		if (action != CopyToZip) {
			if (dest.Last() != '/' && dest.Last() != '\\') {
				dest += wxFileName::GetPathSeparator();
				folder = dest;
			}

			if (folder.FileExists())
				wxMessageBox(_("Invalid destination."), _("Error"), wxICON_EXCLAMATION | wxOK);
			if (!folder.DirExists())
				folder.Mkdir(0777, wxPATH_MKDIR_FULL);
			if (!folder.DirExists()) {
				wxMessageBox(_("Could not create destination folder."), _("Error"), wxICON_EXCLAMATION | wxOK);
				return;
			}
		}
		else if (folder.IsDir() || folder.GetName().empty()) {
			wxMessageBox(_("Invalid path for .zip file."), _("Error"), wxICON_EXCLAMATION | wxOK);
			return;
		}
	}

	if (action != CheckFontsOnly)
		OPT_SET("Path/Fonts Collector Destination")->SetString(STD_STR(dest));

	// Disable the UI while it runs as we don't support canceling
	EnableCloseButton(false);
	start_btn->Enable(false);
	dest_browse_button->Enable(false);
	dest_ctrl->Enable(false);
	close_btn->Enable(false);
	collection_mode->Enable(false);
	dest_label->Enable(false);

	new FontsCollectorThread(subs, dest, static_cast<FcMode>(action), GetEventHandler());
}

void DialogFontsCollector::OnBrowse(wxCommandEvent &) {
	wxString dest;
	if (collection_mode->GetSelection() == CopyToZip) {
		dest = wxFileSelector(
			_("Select archive file name"),
			dest_ctrl->GetValue(),
			wxFileName(dest_ctrl->GetValue()).GetFullName(),
			".zip", _("Zip Archives (*.zip)|*.zip"),
			wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	}
	else
		dest = wxDirSelector(_("Select folder to save fonts on"), dest_ctrl->GetValue(), 0);

	if (!dest.empty())
		dest_ctrl->SetValue(dest);
}

void DialogFontsCollector::OnRadio(wxCommandEvent &) {
	int value = collection_mode->GetSelection();
	wxString dst = dest_ctrl->GetValue();

	if (value == CheckFontsOnly || value == CopyToScriptFolder) {
		dest_ctrl->Enable(false);
		dest_browse_button->Enable(false);
		dest_label->Enable(false);
		dest_label->SetLabel(_("N/A"));
	}
	else {
		dest_ctrl->Enable(true);
		dest_browse_button->Enable(true);
		dest_label->Enable(true);

		if (value == CopyToFolder) {
			dest_label->SetLabel(_("Choose the folder where the fonts will be collected to. It will be created if it doesn't exist."));

			// Remove filename from browse box
			if (dst.Right(4) == ".zip")
				dest_ctrl->SetValue(wxFileName(dst).GetPath());
		}
		else {
			dest_label->SetLabel(_("Enter the name of the destination zip file to collect the fonts to. If a folder is entered, a default name will be used."));

			// Add filename to browse box
			if (!dst.EndsWith(".zip")) {
				wxFileName fn(dst + "//");
				fn.SetFullName("fonts.zip");
				dest_ctrl->SetValue(fn.GetFullPath());
			}
		}
	}
}

void DialogFontsCollector::OnAddText(wxThreadEvent &event) {
	std::pair<int, wxString> str = event.GetPayload<std::pair<int, wxString> >();
	collection_log->SetReadOnly(false);
	int pos = collection_log->GetReverseUnicodePosition(collection_log->GetLength());
	collection_log->AppendText(str.second);
	if (str.first) {
		collection_log->StartUnicodeStyling(pos, 31);
		collection_log->SetUnicodeStyling(pos, str.second.size(), str.first);
	}
	collection_log->GotoPos(pos);
	collection_log->SetReadOnly(true);
}

void DialogFontsCollector::OnCollectionComplete(wxThreadEvent &) {
	EnableCloseButton(true);
	start_btn->Enable();
	close_btn->Enable();
	collection_mode->Enable();
	if (!subs->filename)
		collection_mode->Enable(2, false);

	wxCommandEvent evt;
	OnRadio(evt);
}
