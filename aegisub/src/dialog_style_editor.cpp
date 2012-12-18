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
// Aegisub Project http://www.aegisub.org/

/// @file dialog_style_editor.cpp
/// @brief Style Editor dialogue box
/// @ingroup style_editor
///

#include "config.h"

#include "dialog_style_editor.h"

#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "ass_style_storage.h"
#include "colour_button.h"
#include "compat.h"
#include "help_button.h"
#include "include/aegisub/context.h"
#include "include/aegisub/subtitles_provider.h"
#include "libresrc/libresrc.h"
#include "main.h"
#include "persist_location.h"
#include "subs_preview.h"
#include "utils.h"

#include <libaegisub/of_type_adaptor.h>

#include <algorithm>
#include <cstdarg>

#include <wx/bmpbuttn.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/fontenum.h>
#include <wx/msgdlg.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace {

/// Style rename helper that walks a file searching for a style and optionally
/// updating references to it
class StyleRenamer {
	agi::Context *c;
	bool found_any;
	bool do_replace;
	wxString source_name;
	wxString new_name;

	/// Process a single override parameter to check if it's \r with this style name
	static void ProcessTag(wxString const& tag, AssOverrideParameter* param, void *userData) {
		StyleRenamer *self = static_cast<StyleRenamer*>(userData);
		if (tag == "\\r" && param->GetType() == VARDATA_TEXT && param->Get<wxString>() == self->source_name) {
			if (self->do_replace)
				param->Set(self->new_name);
			else
				self->found_any = true;
		}
	}

	void Walk(bool replace) {
		found_any = false;
		do_replace = replace;

		for (auto diag : c->ass->Line | agi::of_type<AssDialogue>()) {
			if (diag->Style == source_name) {
				if (replace)
					diag->Style = new_name;
				else
					found_any = true;
			}

			boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());
			for (auto block : blocks | agi::of_type<AssDialogueBlockOverride>())
				block->ProcessParameters(&StyleRenamer::ProcessTag, this);
			if (replace)
				diag->UpdateText(blocks);

			if (found_any) return;
		}
	}

public:
	StyleRenamer(agi::Context *c, wxString const& source_name, wxString const& new_name)
	: c(c)
	, found_any(false)
	, do_replace(false)
	, source_name(source_name)
	, new_name(new_name)
	{
	}

	/// Check if there are any uses of the original style name in the file
	bool NeedsReplace() {
		Walk(false);
		return found_any;
	}

	/// Replace all uses of the original style name with the new one
	void Replace() {
		Walk(true);
	}
};

template<typename Window>
struct validator_base : public wxValidator {
	validator_base(validator_base<Window> const& rhs) { SetWindow(rhs.GetWindow()); }

	bool TransferFromWindow() override {
		TransferFromWindow(static_cast<Window*>(GetWindow()));
		return true;
	}

	bool TransferToWindow() override {
		TransferToWindow(static_cast<Window*>(GetWindow()));
		return true;
	}

	virtual void TransferFromWindow(Window *) = 0;
	virtual void TransferToWindow(Window *) = 0;
};

template<typename T>
struct spin_validator : public validator_base<wxSpinCtrl> {
	T *value;
	spin_validator(T* value) : validator_base<wxSpinCtrl>(*this), value(value) { }
	wxObject *Clone() const override { return new spin_validator<T>(*this); }
	void TransferFromWindow(wxSpinCtrl *ctrl) override { *value = ctrl->GetValue(); }
	void TransferToWindow(wxSpinCtrl *ctrl) override { ctrl->SetValue(*value); }
};

struct border_style_validator : public validator_base<wxCheckBox> {
	int *value;
	border_style_validator(int* value) : validator_base<wxCheckBox>(*this), value(value) { }
	wxObject *Clone() const override { return new border_style_validator(*this); }
	void TransferFromWindow(wxCheckBox *ctrl) override { *value = ctrl->GetValue() ? 3 : 1; }
	void TransferToWindow(wxCheckBox *ctrl) override { ctrl->SetValue(*value == 3); }
};

struct encoding_validator : public validator_base<wxComboBox> {
	int *value;
	encoding_validator(int* value) : validator_base<wxComboBox>(*this), value(value) { }
	wxObject *Clone() const override { return new encoding_validator(*this); }

	void TransferFromWindow(wxComboBox *ctrl) override {
		long templ = 0;
		ctrl->GetValue().BeforeFirst('-').ToLong(&templ);
		*value = templ;
	}

	void TransferToWindow(wxComboBox *ctrl) override {
		wxString prefix(wxString::Format("%d -", *value));
		auto items(ctrl->GetStrings());
		auto pos = std::lower_bound(items.begin(), items.end(), prefix);
		if (pos != items.end() && pos->StartsWith(prefix))
			ctrl->Select(std::distance(items.begin(), pos));
		else
			ctrl->Select(0);
	}
};

struct alignment_validator : public validator_base<wxRadioBox> {
	int *value;
	alignment_validator(int *value) : validator_base<wxRadioBox>(*this), value(value) { }

	void TransferFromWindow(wxRadioBox *rb) {
		switch (rb->GetSelection()) {
			case 0:  *value = 7;
			case 1:  *value = 8;
			case 2:  *value = 9;
			case 3:  *value = 4;
			case 4:  *value = 5;
			case 5:  *value = 6;
			case 6:  *value = 1;
			case 7:  *value = 2;
			case 8:  *value = 3;
			default: *value = 2;
		}
	}

	void TransferToWindow(wxRadioBox *rb) {
		switch (*value) {
			case 7:  rb->SetSelection(0);
			case 8:  rb->SetSelection(1);
			case 9:  rb->SetSelection(2);
			case 4:  rb->SetSelection(3);
			case 5:  rb->SetSelection(4);
			case 6:  rb->SetSelection(5);
			case 1:  rb->SetSelection(6);
			case 2:  rb->SetSelection(7);
			case 3:  rb->SetSelection(8);
			default: rb->SetSelection(7);
		}
	}
};

void add_with_label(wxSizer *sizer, wxWindow *parent, wxString const& label, wxWindow *ctrl) {
	sizer->Add(new wxStaticText(parent, -1, label), wxSizerFlags().Center().Right().HorzBorder());
	sizer->Add(ctrl, wxSizerFlags(1).Left().Expand());
}

template<typename T>
wxSpinCtrl *spin_ctrl(wxWindow *parent, T *value, int max_value) {
	auto ctrl = new wxSpinCtrl(parent, -1, "", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 0, max_value, *value);
	ctrl->SetValidator(spin_validator<T>(value));
	return ctrl;
}

wxTextCtrl *num_text_ctrl(wxWindow *parent, double *value, wxString const& tooltip, wxSize size = wxSize(70, 20)) {
	auto ctrl = new wxTextCtrl(parent, -1, "", wxDefaultPosition, size, 0, wxFloatingPointValidator<double>(value));
	ctrl->SetToolTip(tooltip);
	return ctrl;
}

wxTextCtrl *text_ctrl(wxWindow *parent, wxString *value, wxString const& tooltip, wxSize size) {
	auto ctrl = new wxTextCtrl(parent, -1, *value, wxDefaultPosition, size, 0, wxGenericValidator(value));
	ctrl->SetToolTip(tooltip);
	return ctrl;
}

wxCheckBox *check_box(wxWindow *parent, bool *value, wxString const& label) {
	return new wxCheckBox(parent, -1, label, wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(value));
}

wxComboBox *combo_box(wxWindow *parent, wxString const& tooltip, wxArrayString const& values, wxValidator const& validator) {
	auto ctrl = new wxComboBox(parent, -1, "", wxDefaultPosition, wxSize(150, -1), 0, 0, wxCB_DROPDOWN, validator);
	ctrl->SetToolTip(_("Font face"));
	ctrl->Freeze();
	ctrl->Set(values);
	ctrl->Thaw();
	return ctrl;
}

wxSizer *stack(wxOrientation dir, ...) {
	wxSizer *sizer = new wxBoxSizer(dir);
	wxSizerFlags flags = wxSizerFlags().Expand().Center();
	bool first = true;

	va_list argp;
	va_start(argp, dir);
	while (wxSizer *sub = va_arg(argp, wxSizer*)) {
		sizer->Add(sub, flags);
		if (first) {
			first = false;
			flags = flags.Border(dir == wxVERTICAL ? wxTOP : wxLEFT);
		}
	}

	va_end(argp);
	return sizer;
}

}

DialogStyleEditor::DialogStyleEditor(wxWindow *parent, AssStyle *style, agi::Context *c, AssStyleStorage *store, wxString const& new_name)
: wxDialog (parent, -1, _("Style Editor"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
, c(c)
, is_new(false)
, style(style)
, store(store)
, preview_text(from_wx(OPT_GET("Tool/Style Editor/Preview Text")->GetString()))
, subs_preview(nullptr)
{
	if (new_name.size()) {
		is_new = true;
		style = this->style = new AssStyle(*style);
		style->name = new_name;
	}
	else if (!style) {
		is_new = true;
		style = this->style = new AssStyle;
	}

	work.reset(new AssStyle(*style));

	SetIcon(GETICON(style_toolbutton_16));

	auto Outline = num_text_ctrl(this, &work->outline_w, _("Outline width, in pixels"), wxSize(50, -1));
	auto Shadow = num_text_ctrl(this, &work->shadow_w, _("Shadow distance, in pixels"), wxSize(50, -1));

	wxString alignValues[9] = { "7", "8", "9", "4", "5", "6", "1", "2", "3" };
	auto alignment = new wxRadioBox(this, -1, _("Alignment"), wxDefaultPosition, wxDefaultSize, 9, alignValues, 3, wxRA_SPECIFY_COLS);
	alignment->SetToolTip(_("Alignment in screen, in numpad style"));
	alignment->SetValidator(alignment_validator(&work->alignment));

	auto outline_type = new wxCheckBox(this, -1, _("&Opaque box"), wxDefaultPosition, wxDefaultSize, 0, border_style_validator(&work->borderstyle));
	outline_type->SetToolTip(_("When selected, display an opaque box behind the subtitles instead of an outline around the text"));

	wxSizer *outline_box = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Outline"));
	add_with_label(outline_box, this, _("Outline:"), Outline);
	add_with_label(outline_box, this, _("Shadow:"), Shadow);
	outline_box->Add(outline_type, 0, wxLEFT | wxALIGN_CENTER, 5);

	wxSizer *preview_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Preview"));
	if (!SubtitlesProviderFactory::GetClasses().empty())
		MakePreviewBox(preview_box);
	else
		preview_box->Add(new wxStaticText(this, -1, _("No subtitle providers available. Cannot preview subs.")), wxSizerFlags().Center());

	wxSizer *control_sizer = stack(wxHORIZONTAL,
		stack(wxVERTICAL, MakeStyleNameBox(), MakeFontBox(), MakeColors(), stack(wxHORIZONTAL, MakeMargins(), alignment, 0), 0),
		stack(wxVERTICAL, outline_box, MakeMiscBox(), preview_box, 0),
		0);

	// General Layout
	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(control_sizer, wxSizerFlags().Border());
	main_sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL | wxAPPLY | wxHELP), wxSizerFlags().Expand().Border(wxBOTTOM));
	SetSizerAndFit(main_sizer);

	persist.reset(new PersistLocation(this, "Tool/Style Editor", true));

	Bind(wxEVT_CHILD_FOCUS, [=](wxChildFocusEvent& e) { UpdateWorkStyle(); });

	if (subs_preview) {
		Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &DialogStyleEditor::OnCommandPreviewUpdate, this);
		Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, &DialogStyleEditor::OnCommandPreviewUpdate, this);
		Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &DialogStyleEditor::OnCommandPreviewUpdate, this);
	}

	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Close, this, true), wxID_OK);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Apply, this), wxID_APPLY);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Close, this, false), wxID_CANCEL);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&HelpButton::OpenPage, "Style Editor"), wxID_HELP);
}

DialogStyleEditor::~DialogStyleEditor() {
	if (is_new)
		delete style;
}

wxString DialogStyleEditor::GetStyleName() const {
	return style->name;
}

wxSizer *DialogStyleEditor::MakeMargins() {
	wxString labels[] = { _("Left"), _("Right"), _("Vert") };
	wxString tooltips[] = {
		_("Distance from left edge, in pixels"),
		_("Distance from right edge, in pixels"),
		_("Distance from top/bottom edge, in pixels")
	};

	wxSizer *sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Margins"));
	for (int i = 0; i < 3; ++i) {
		auto ctrl = spin_ctrl(this, &work->Margin[i], 9999);
		ctrl->SetToolTip(tooltips[i]);
		sizer->Add(stack(wxVERTICAL, new wxStaticText(this, -1, labels[i]), ctrl, 0), 0, wxLEFT, i ? 3 : 0);
	}
	return sizer;
}

wxSizer *DialogStyleEditor::MakeColors() {
	wxString labels[] = { _("Primary"), _("Secondary"), _("Outline"), _("Shadow") };
	wxString tooltips[] = { _("Choose primary color"), _("Choose secondary color"), _("Choose outline color"), _("Choose shadow color") };
	agi::Color *fields[] = { &work->primary, &work->secondary, &work->outline, &work->shadow };

	wxSizer *sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Colors"));
	for (int i = 0; i < 4; ++i) {
		auto field = fields[i];
		auto btn = new ColourButton(this, -1, wxSize(55, 16), *field);
		btn->SetToolTip(tooltips[i]);
		btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [=](wxCommandEvent &e) {
			e.Skip();
			*field = btn->GetColor();
			if (subs_preview)
				subs_preview->SetStyle(*work);
		});

		auto alpha = spin_ctrl(this, &field->a, 255);
		alpha->SetToolTip(_("Set opacity, from 0 (opaque) to 255 (transparent)"));

		sizer->Add(stack(wxVERTICAL, new wxStaticText(this, -1, labels[i]), btn, alpha, 0), 0, wxLEFT, i ? 5 : 0);
	}
	return sizer;
}

wxSizer *DialogStyleEditor::MakeStyleNameBox() {
	wxSizer *sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Style Name"));
	sizer->Add(text_ctrl(this, &work->name, _("Style name"), wxSize(500, -1)), wxSizerFlags(1).Border());
	return sizer;
}

wxSizer *DialogStyleEditor::MakeFontBox() {
	wxArrayString fonts = wxFontEnumerator::GetFacenames();
	fonts.Sort();
	auto name = combo_box(this, _("Font Face"), fonts, wxGenericValidator(&work->font));
	auto size = num_text_ctrl(this, &work->fontsize, _("Font size"), wxSize(50, -1));

	name->Bind(wxEVT_COMMAND_TEXT_ENTER, &DialogStyleEditor::OnCommandPreviewUpdate, this);

	wxSizer *top_sizer = new wxBoxSizer(wxHORIZONTAL);
	top_sizer->Add(name, wxSizerFlags(1));
	top_sizer->Add(size, wxSizerFlags().Left());

	bool *fields[] = { &work->bold, &work->italic, &work->underline, &work->strikeout };
	wxString labels[] = { _("&Bold"), _("&Italic"), _("&Underline"), _("&Strikeout") };

	wxSizer *bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	for (size_t i = 0; i < 4; ++i)
		bottom_sizer->Add(check_box(this, fields[i], labels[i]), wxSizerFlags().HorzBorder());

	wxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Font"));
	sizer->Add(top_sizer, wxSizerFlags(1).Expand());
	sizer->Add(bottom_sizer, wxSizerFlags(1).Center().Border(wxTOP));
	return sizer;
}

wxSizer *DialogStyleEditor::MakeMiscBox() {
	wxString labels[] = { _("Scale X%:"), _("Scale Y%:"), _("Rotation:"), _("Spacing:") };
	double *fields[] = { &work->scalex, &work->scaley, &work->angle, &work->spacing };
	wxString tooltips[] = { _("Scale X, in percentage"), _("Scale Y, in percentage"), _("Angle to rotate in Z axis, in degrees"), _("Character spacing, in pixels") };

	wxFlexGridSizer *top_sizer = new wxFlexGridSizer(2, 4, 5, 5);
	for (size_t i = 0; i < 4; ++i)
		add_with_label(top_sizer, this, labels[i], num_text_ctrl(this, fields[i], tooltips[i]));

	wxArrayString encodingStrings;
	AssStyle::GetEncodings(encodingStrings);

	wxSizer *bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	add_with_label(bottom_sizer, this, _("Encoding:"),
		combo_box(this, _("Encoding, only useful in unicode if the font doesn't have the proper unicode mapping"), encodingStrings, encoding_validator(&work->encoding)));

	wxSizer *sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Miscellaneous"));
	sizer->Add(top_sizer, wxSizerFlags().Expand().Center());
	sizer->Add(bottom_sizer, wxSizerFlags().Expand().Center().Border(wxTOP));
	return sizer;
}

void DialogStyleEditor::MakePreviewBox(wxSizer *sizer) {
	auto preview_text_ctrl = text_ctrl(this, &preview_text, _("Text to be used for the preview"), wxDefaultSize);
	preview_text_ctrl->Bind(wxEVT_COMMAND_TEXT_UPDATED, [=](wxCommandEvent &) {
		TransferDataFromWindow();
		subs_preview->SetText(preview_text);
	});

	auto color_btn = new ColourButton(this, -1, wxSize(45, 16), OPT_GET("Colour/Style Editor/Background/Preview")->GetColor());
	color_btn->SetToolTip(_("Color of preview background"));
	color_btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, [=](wxCommandEvent& evt) {
		evt.Skip();
		subs_preview->SetColour(color_btn->GetColor());
		OPT_SET("Colour/Style Editor/Background/Preview")->SetColor(color_btn->GetColor());
	});

	subs_preview = new SubtitlesPreview(this, wxSize(100, 60), wxSUNKEN_BORDER, OPT_GET("Colour/Style Editor/Background/Preview")->GetColor());
	subs_preview->SetToolTip(_("Preview of current style"));
	subs_preview->SetStyle(*style);
	subs_preview->SetText(preview_text);

	wxSizer *bottom_sizer = new wxBoxSizer(wxHORIZONTAL);
	bottom_sizer->Add(preview_text_ctrl, 1, wxEXPAND | wxRIGHT, 5);
	bottom_sizer->Add(color_btn, 0, wxEXPAND, 0);
	sizer->Add(subs_preview, 1, wxEXPAND | wxBOTTOM, 5);
	sizer->Add(bottom_sizer, 0, wxEXPAND | wxBOTTOM, 0);
}

bool DialogStyleEditor::NameIsDuplicate() const {
	if (store)
		return store->GetStyle(work->name) != style;

	for (auto s : c->ass->Line | agi::of_type<AssStyle>()) {
		if (s != style && work->name.CmpNoCase(s->name) == 0)
			return true;
	}
}

bool DialogStyleEditor::StyleRefsNeedRename(StyleRenamer& renamer) const {
	if (style->name == work->name) return false;
	return !store && !is_new && renamer.NeedsReplace();
}

void DialogStyleEditor::Apply() {
	UpdateWorkStyle();

	// Check if style name is unique
	if (NameIsDuplicate()) {
		wxMessageBox(_("There is already a style with this name. Please choose another name."), _("Style name conflict"), wxOK | wxICON_ERROR | wxCENTER);
		return;
	}

	// Style name change
	bool did_rename = false;
	StyleRenamer renamer(c, style->name, work->name);
	if (StyleRefsNeedRename(renamer)) {
		// See if user wants to update style name through script
		int answer = wxMessageBox(
			_("Do you want to change all instances of this style in the script to this new name?"),
			_("Update script?"),
			wxYES_NO | wxCANCEL);

		if (answer == wxCANCEL) return;

		if (answer == wxYES) {
			did_rename = true;
			renamer.Replace();
		}
	}

	*style = *work;
	style->UpdateData();
	if (is_new) {
		if (store)
			store->push_back(style);
		else
			c->ass->InsertLine(style);
		is_new = false;
	}
	if (!store)
		c->ass->Commit(_("style change"), AssFile::COMMIT_STYLES | (did_rename ? AssFile::COMMIT_DIAG_FULL : 0));
}

void DialogStyleEditor::Close(bool apply) {
	if (apply) Apply();
	OPT_SET("Tool/Style Editor/Preview Text")->SetString(from_wx(preview_text));
	EndModal(apply);
}

void DialogStyleEditor::UpdateWorkStyle() {
	TransferDataFromWindow();
	if (subs_preview) subs_preview->SetStyle(*work);
}

void DialogStyleEditor::OnCommandPreviewUpdate(wxCommandEvent &event) {
	if (!IsShownOnScreen()) return;
	UpdateWorkStyle();
	event.Skip();
}
