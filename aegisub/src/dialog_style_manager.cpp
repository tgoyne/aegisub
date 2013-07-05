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

#include "dialog_style_manager.h"

#include "ass_file.h"
#include "dialog_style_editor.h"
#include "include/aegisub/context.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "options.h"
#include "persist_location.h"
#include "style_catalog_list.h"
#include "style_list.h"
#include "style_manager.h"

#include <libaegisub/fs.h>
#include <libaegisub/path.h>
#include <libaegisub/util.h>

#include <functional>

#include <wx/intl.h>

DialogStyleManager::DialogStyleManager(agi::Context *context)
: wxDialog(context->parent, -1, _("Styles Manager"))
, c(context)
, current_manager(StyleManager::Create(c->ass))
{
	using std::bind;
	SetIcon(GETICON(style_toolbutton_16));

	StyleListOptions current_options = {
		"",
		_("Current script"),
		_("<- Copy to &storage"),
		true
	};

	StyleListOptions storage_options = {
		"",
		_("Storage"),
		_("Copy to &current script ->"),
		false
	};

	auto catalog_list = new StyleCatalogList(this, c->ass->GetScriptInfo("Last Style Storage"));
	catalog_list->Bind(EVT_CATALOG_CHANGED, &DialogStyleManager::OnChangeCatalog, this);

	wxStdDialogButtonSizer *button_sizer = CreateStdDialogButtonSizer(wxCANCEL | wxHELP);
	button_sizer->GetCancelButton()->SetLabel(_("Close"));
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, bind(&HelpButton::OpenPage, "Styles Manager"), wxID_HELP);

	wxSizer *style_lists_sizer = new wxBoxSizer(wxHORIZONTAL);
	style_lists_sizer->Add(new StyleList(storage_manager.get(), current_manager.get(), storage_options), wxSizerFlags().Expand().Border(wxRIGHT));
	style_lists_sizer->Add(new StyleList(current_manager.get(), storage_manager.get(), current_options), wxSizerFlags().Expand());

	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(catalog_list, wxSizerFlags().Expand().Border(wxALL & ~wxBOTTOM));
	main_sizer->Add(style_lists_sizer, wxSizerFlags(1).Expand().Border());
	main_sizer->Add(button_sizer, wxSizerFlags().Expand().Border(wxBOTTOM));

	SetSizerAndFit(main_sizer);

	persist = agi::util::make_unique<PersistLocation>(this, "Tool/Style Manager");
}

DialogStyleManager::~DialogStyleManager() { }

void DialogStyleManager::OnChangeCatalog(wxThreadEvent &evt) {
	std::string catalog = evt.GetPayload<std::string>();
	c->ass->SetScriptInfo("Last Style Storage", catalog);
	storage_manager = StyleManager::Create(config::path->Decode("?user/catalog/" + catalog + ".sty"));
}

void DialogStyleManager::ShowEditor(AssStyle *style, StyleManager *manager, wxListBox *list) {
	DialogStyleEditor editor(this, style, manager);
	editor.ShowModal();
}
