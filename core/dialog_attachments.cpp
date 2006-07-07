// Copyright (c) 2006, Rodrigo Braz Monteiro
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


///////////
// Headers
#include <wx/listctrl.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include "dialog_attachments.h"
#include "ass_file.h"
#include "ass_attachment.h"
#include "utils.h"
#include "options.h"


///////////////
// Constructor
DialogAttachments::DialogAttachments(wxWindow *parent)
: wxDialog(parent,-1,_("Attachment List"),wxDefaultPosition,wxDefaultSize)
{
	// List view
	listView = new wxListView(this,-1,wxDefaultPosition,wxSize(500,200));
	UpdateList();

	// Buttons sizer
	wxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->Add(new wxButton(this,BUTTON_ATTACH_FONT,_("Attach &Font")),1,0,0);
	buttonSizer->Add(new wxButton(this,BUTTON_ATTACH_GRAPHICS,_("Attach &Graphics")),1,0,0);
	buttonSizer->Add(new wxButton(this,BUTTON_EXTRACT,_("E&xtract")),1,0,0);
	buttonSizer->Add(new wxButton(this,BUTTON_DELETE,_("&Delete")),1,0,0);
	buttonSizer->Add(new wxButton(this,BUTTON_CLOSE,_("&Close")),1,wxLEFT,5);

	// Main sizer
	wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(listView,1,wxTOP | wxLEFT | wxRIGHT | wxEXPAND,5);
	mainSizer->Add(buttonSizer,0,wxALL | wxEXPAND,5);
	mainSizer->SetSizeHints(this);
	SetSizer(mainSizer);
}


///////////////
// Update list
void DialogAttachments::UpdateList() {
	// Clear list
	listView->ClearAll();

	// Insert list columns
	listView->InsertColumn(0, _("Attachment name"), wxLIST_FORMAT_LEFT, 280);
	listView->InsertColumn(1, _("Size"), wxLIST_FORMAT_LEFT, 100);
	listView->InsertColumn(2, _("Group"), wxLIST_FORMAT_LEFT, 100);

	// Fill list
	AssAttachment *attach;
	for (std::list<AssEntry*>::iterator cur = AssFile::top->Line.begin();cur != AssFile::top->Line.end();cur++) {
		attach = AssEntry::GetAsAttachment(*cur);
		if (attach) {
			// Add item
			int row = listView->GetItemCount();
			listView->InsertItem(row,attach->filename);
			listView->SetItem(row,1,PrettySize(attach->GetData().size()));
			listView->SetItem(row,2,attach->group);
			listView->SetItemData(row,(long)attach);
		}
	}
}


//////////////
// Destructor
DialogAttachments::~DialogAttachments() {
}


///////////////
// Event table
BEGIN_EVENT_TABLE(DialogAttachments,wxDialog)
	EVT_BUTTON(BUTTON_ATTACH_FONT,DialogAttachments::OnAttachFont)
	EVT_BUTTON(BUTTON_ATTACH_GRAPHICS,DialogAttachments::OnAttachGraphics)
	EVT_BUTTON(BUTTON_EXTRACT,DialogAttachments::OnExtract)
	EVT_BUTTON(BUTTON_DELETE,DialogAttachments::OnDelete)
	EVT_BUTTON(BUTTON_CLOSE,DialogAttachments::OnClose)
END_EVENT_TABLE()


///////////////
// Attach font
void DialogAttachments::OnAttachFont(wxCommandEvent &event) {
	// Pick files
	wxArrayString filenames;
	wxArrayString paths;
	{
		wxFileDialog diag (this,_("Choose file to be attached"), Options.AsText(_T("Fonts Collector Destination")), _T(""), _T("Font Files (*.ttf)|*.ttf"), wxOPEN | wxFILE_MUST_EXIST | wxMULTIPLE);
		if (diag.ShowModal() == wxID_CANCEL) return;
		diag.GetFilenames(filenames);
		diag.GetPaths(paths);
	}

	// Create attachments
	for (size_t i=0;i<filenames.Count();i++) {
		//wxFileName file(filenames[i]);
		AssAttachment *newAttach = new AssAttachment(filenames[i]);
		try {
			newAttach->Import(paths[i]);
		}
		catch (...) {
			delete newAttach;
			return;
		}
		newAttach->group = _T("[Fonts]");
		AssFile::top->InsertAttachment(newAttach);
	}

	// Update
	UpdateList();
}


///////////////////
// Attach graphics
void DialogAttachments::OnAttachGraphics(wxCommandEvent &event) {
	// Pick files
	wxArrayString filenames;
	wxArrayString paths;
	{
		wxFileDialog diag (this,_("Choose file to be attached"), _T(""), _T(""), _T("Graphic Files (*.bmp,*.gif,*.jpg,*.ico,*.wmf)|*.bmp;*.gif;*.jpg;*.ico;*.wmf"), wxOPEN | wxFILE_MUST_EXIST | wxMULTIPLE);
		if (diag.ShowModal() == wxID_CANCEL) return;
		diag.GetFilenames(filenames);
		diag.GetPaths(paths);
	}

	// Create attachments
	for (size_t i=0;i<filenames.Count();i++) {
		//wxFileName file(filenames[i]);
		AssAttachment *newAttach = new AssAttachment(filenames[i]);
		try {
			newAttach->Import(paths[i]);
		}
		catch (...) {
			delete newAttach;
			return;
		}
		newAttach->group = _T("[Graphics]");
		AssFile::top->InsertAttachment(newAttach);
	}

	// Update
	UpdateList();
}


///////////
// Extract
void DialogAttachments::OnExtract(wxCommandEvent &event) {
	// Check if there's a selection
	int i = listView->GetFirstSelected();

	// Get path
	if (i != -1) {
		wxString path;
		bool fullPath = false;

		// Multiple or single?
		if (listView->GetNextSelected(i) != -1) path = wxDirSelector(_("Select the path to save the files to:"),Options.AsText(_T("Fonts Collector Destination"))) + _T("/");
		else {
			path = wxFileSelector(_("Select the path to save the file to:"),Options.AsText(_T("Fonts Collector Destination")),((AssAttachment*) listView->GetItemData(i))->filename);
			fullPath = true;
		}
		if (path.IsEmpty()) return;

		// Loop through items in list
		while (i != -1) {
			AssAttachment *attach = (AssAttachment*) listView->GetItemData(i);
			wxString filename = path;
			if (!fullPath) filename += attach->filename;
			attach->Extract(filename);
			i = listView->GetNextSelected(i);
		}
	}
}


//////////
// Delete
void DialogAttachments::OnDelete(wxCommandEvent &event) {
	// Loop through items in list
	int i = listView->GetFirstSelected();
	while (i != -1) {
		AssFile::top->Line.remove((AssEntry*)listView->GetItemData(i));
		i = listView->GetNextSelected(i);
	}

	// Update list
	UpdateList();
}


/////////
// Close
void DialogAttachments::OnClose(wxCommandEvent &event) {
	EndModal(0);
}
