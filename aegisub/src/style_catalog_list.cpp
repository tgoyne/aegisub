// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "config.h"

#include "style_catalog_list.h"

#include "compat.h"
#include "options.h"
#include "style_manager.h"

#include <libaegisub/fs.h>
#include <libaegisub/path.h>
#include <libaegisub/util.h>

#include <wx/button.h>

wxDEFINE_EVENT(EVT_CATALOG_CHANGED, wxThreadEvent);

StyleCatalogList::StyleCatalogList(wxWindow *parent, std::string const& active_catalog)
: wxStaticBoxSizer(wxHORIZONTAL, parent, _("Catalog of available storages"))
{
	wxArrayString catalogs;
	for (auto const& file : agi::fs::DirectoryIterator(config::path->Decode("?user/catalog/"), "*.sty"))
		catalogs.push_back(agi::fs::path(file).stem().wstring());

	// Create a default storage if there are none
	if (catalogs.empty()) {
		auto storage_manager = StyleManager::Create(config::path->Decode("?user/catalog/Default.sty"));
		storage_manager->Create();
		storage_manager->Save("");
		catalogs.push_back("Default");
	}

	auto catalog_list = new wxComboBox(parent, -1, "", wxDefaultPosition, wxDefaultSize, catalogs, wxCB_READONLY);
	auto new_button = new wxButton(parent, -1, _("New"));
	auto delete_button = new wxButton(parent, -1, _("Delete"));
	Add(catalog_list,1,wxEXPAND | wxRIGHT | wxALIGN_RIGHT,5);
	Add(new_button,0,wxRIGHT,5);
	Add(delete_button,0,0,0);

	delete_button->Enable(catalogs.size() > 1);

	if (std::find(catalogs.begin(), catalogs.end(), to_wx(active_catalog)) != catalogs.end())
		catalog_list->SetStringSelection(to_wx(active_catalog));
	else
		catalog_list->SetSelection(0);

	new_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [=](wxCommandEvent&){
		wxString name = wxGetTextFromUser(_("New storage name:"), _("New catalog entry"), "", parent);
		if (!name) return;

		wxString badchars = wxFileName::GetForbiddenChars(wxPATH_DOS);
		int badchars_removed = 0;
		for (wxUniCharRef chr : name) {
			if (badchars.find(chr) != badchars.npos) {
				chr = '_';
				++badchars_removed;
			}
		}

		if (catalog_list->FindString(name, false) != wxNOT_FOUND) {
			wxMessageBox(_("A catalog with that name already exists."), _("Catalog name conflict"), wxOK | wxICON_ERROR | wxCENTER);
			return;
		}

		if (badchars_removed) {
			wxMessageBox(
				wxString::Format(_("The specified catalog name contains one or more illegal characters. They have been replaced with underscores instead.\nThe catalog has been renamed to \"%s\"."), name),
				_("Invalid characters"));
		}

		catalog_list->Append(name);
		catalog_list->SetStringSelection(name);
		ChangeCatalog(from_wx(name));
		delete_button->Enable();
	});

	delete_button->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [=](wxCommandEvent&){
		if (catalog_list->GetCount() == 1) return;

		wxString wxname = catalog_list->GetStringSelection();
		wxString message = wxString::Format(_("Are you sure you want to delete the storage \"%s\" from the catalog? This cannot be undone."), wxname);
		if (wxYES != wxMessageBox(message, _("Confirm delete"), wxYES_NO | wxICON_EXCLAMATION , parent))
			return;

		auto name = from_wx(wxname);
		agi::fs::Remove(config::path->Decode("?user/catalog/" + name + ".sty"));
		catalog_list->Delete(catalog_list->GetSelection());
		catalog_list->SetSelection(0);
		delete_button->Enable(catalog_list->GetCount() > 1);
		ChangeCatalog(name);
	});

	catalog_list->Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, [=](wxCommandEvent&){
		ChangeCatalog(from_wx(catalog_list->GetStringSelection()));
	});
}

void StyleCatalogList::ChangeCatalog(std::string const& new_name) {
	auto evt = agi::util::make_unique<wxThreadEvent>();
	evt->SetPayload(new_name);
	QueueEvent(evt.release());
}
