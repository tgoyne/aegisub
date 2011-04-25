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

/// @file dialog_fonts_collector.h
/// @see dialog_fonts_collector.cpp
/// @ingroup tools_ui font_collector
///

#ifndef AGI_PRE
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>
#endif

class AssFile;
class AssOverrideParameter;
class DialogFontsCollector;
class FontFileLister;
class FrameMain;
class ScintillaTextCtrl;
class wxZipOutputStream;

/// DOCME
/// @class FontsCollectorThread
/// @brief Worker thread for the font collector dialog
class FontsCollectorThread : public wxThread {
	AssFile *subs; ///< Subtitle file to process
	wxString destination; ///< Path to write fonts to for modes 2 and 3
	DialogFontsCollector *collector; ///< Parent dialog
	int oper; ///< 0: check fonts only; 1: copy to folder; 2: copy to zip

	std::set<wxString> fontPaths; ///< Paths of needed fonts found by the lister
	std::set<wxString> missingFonts; ///< Names of fonts which could not be found

	/// @brief Do the font collection
	void Collect();

	/// @brief Copy a found font to the target folder
	/// @param filename Font to copy
	/// @return 0: failed; 1: succeeded; 2: already present
	int CopyFont(wxString const& filename);
	/// @brief Copy a found font to the target archive
	/// @param filename Font to copy
	/// @param zip Target zip archive
	/// @return Did it succeed
	bool ArchiveFont(wxString const& filename, wxZipOutputStream &zip);

	/// @brief Tell the dialog to add text to the textbox
	/// @param text Text to add
	/// @param colour 0: neutral; 1: success; 2: error
	void AppendText(wxString text,int colour=0);

	wxThread::ExitCode Entry();
public:
	FontsCollectorThread(AssFile *subs, wxString destination, int oper, DialogFontsCollector *collector);
};

/// DOCME
/// @class DialogFontsCollector
/// @brief DOCME
///
/// DOCME
class DialogFontsCollector : public wxDialog {
	friend class FontsCollectorThread;

	AssFile *subs;

	/// DOCME
	wxTextCtrl *DestBox;

	/// DOCME
	ScintillaTextCtrl *LogBox;

	/// DOCME
	wxButton *BrowseButton;

	/// DOCME
	wxButton *StartButton;

	/// DOCME
	wxButton *CloseButton;

	/// DOCME
	wxStaticText *DestLabel;

	/// DOCME
	wxRadioBox *CollectAction;

	/// DOCME
	FrameMain *main;

	void OnStart(wxCommandEvent &event);
	void OnClose(wxCommandEvent &event) { EndModal(0); }
	void OnBrowse(wxCommandEvent &event);
	void OnRadio(wxCommandEvent &event);
	void OnAddText(wxCommandEvent &event);
	void Update(int value=-1);

public:
	DialogFontsCollector(wxWindow *parent, AssFile *subs);
	~DialogFontsCollector();

	DECLARE_EVENT_TABLE()
};


