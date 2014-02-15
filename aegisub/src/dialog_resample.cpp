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

/// @file dialog_resample.cpp
/// @brief Resample Resolution dialogue box and logic
/// @ingroup tools_ui
///

#include "config.h"

#include "dialog_resample.h"

#include "ass_file.h"
#include "include/aegisub/context.h"
#include "help_button.h"
#include "libresrc/libresrc.h"
#include "resolution_resampler.h"
#include "video_context.h"

#include <boost/rational.hpp>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/valgen.h>

enum {
	LEFT = 0,
	RIGHT = 1,
	TOP = 2,
	BOTTOM = 3
};

DialogResample::DialogResample(agi::Context *c, ResampleSettings &settings)
: wxDialog(c->parent, -1, _("Resample Resolution"))
, c(c)
{
	SetIcon(GETICON(resample_toolbutton_16));

	memset(&settings, 0, sizeof(settings));
	c->ass->GetResolution(script_w, script_h);
	settings.source_x = script_w;
	settings.source_y = script_h;

	if (c->videoController->IsLoaded()) {
		settings.dest_x = video_w = c->videoController->GetWidth();
		settings.dest_y = video_h = c->videoController->GetHeight();
	}
	else {
		settings.dest_x = script_w;
		settings.dest_y = script_h;
	}

	// Create all controls and set validators
	for (size_t i = 0; i < 4; ++i) {
		margin_ctrl[i] = new wxSpinCtrl(this, -1, "0", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, -9999, 9999, 0);
		margin_ctrl[i]->SetValidator(wxGenericValidator(&settings.margin[i]));
	}

	symmetrical = new wxCheckBox(this, -1, _("&Symmetrical"));
	symmetrical->SetValue(true);

	margin_ctrl[RIGHT]->Enable(false);
	margin_ctrl[BOTTOM]->Enable(false);

	source_x = new wxSpinCtrl(this, -1, "", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, INT_MAX);
	source_y = new wxSpinCtrl(this, -1, "", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, INT_MAX);
	dest_x = new wxSpinCtrl(this, -1, "", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, INT_MAX);
	dest_y = new wxSpinCtrl(this, -1, "", wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, INT_MAX);

	source_x->SetValidator(wxGenericValidator(&settings.source_x));
	source_y->SetValidator(wxGenericValidator(&settings.source_y));
	dest_x->SetValidator(wxGenericValidator(&settings.dest_x));
	dest_y->SetValidator(wxGenericValidator(&settings.dest_y));

	from_video = new wxButton(this, -1, _("From &video"));
	from_video->Enable(false);
	from_script = new wxButton(this, -1, _("From s&cript"));
	from_script->Enable(false);

	change_ar = new wxCheckBox(this, -1, _("&Change aspect ratio"));
	change_ar->SetValidator(wxGenericValidator(&settings.change_ar));
	change_ar->Enable(false);

	// Position the controls
	auto margin_sizer = new wxGridSizer(3, 3, 5, 5);
	margin_sizer->AddSpacer(1);
	margin_sizer->Add(margin_ctrl[TOP], wxSizerFlags(1).Expand());
	margin_sizer->AddSpacer(1);
	margin_sizer->Add(margin_ctrl[LEFT], wxSizerFlags(1).Expand());
	margin_sizer->Add(symmetrical, wxSizerFlags(1).Expand());
	margin_sizer->Add(margin_ctrl[RIGHT], wxSizerFlags(1).Expand());
	margin_sizer->AddSpacer(1);
	margin_sizer->Add(margin_ctrl[BOTTOM], wxSizerFlags(1).Expand());
	margin_sizer->AddSpacer(1);

	auto margin_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Margin offset"));
	margin_box->Add(margin_sizer, wxSizerFlags(1).Expand().Border(wxBOTTOM));

	auto source_res_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Source Resolution"));
	source_res_sizer->Add(source_x, wxSizerFlags(1).Border(wxRIGHT).Align(wxALIGN_CENTER_VERTICAL));
	source_res_sizer->Add(new wxStaticText(this, -1, _("x")), wxSizerFlags().Center().Border(wxRIGHT));
	source_res_sizer->Add(source_y, wxSizerFlags(1).Border(wxRIGHT).Align(wxALIGN_CENTER_VERTICAL));
	source_res_sizer->Add(from_script, wxSizerFlags(1));

	auto dest_res_sizer = new wxBoxSizer(wxHORIZONTAL);
	dest_res_sizer->Add(dest_x, wxSizerFlags(1).Border(wxRIGHT).Align(wxALIGN_CENTER_VERTICAL));
	dest_res_sizer->Add(new wxStaticText(this, -1, _("x")), wxSizerFlags().Center().Border(wxRIGHT));
	dest_res_sizer->Add(dest_y, wxSizerFlags(1).Border(wxRIGHT).Align(wxALIGN_CENTER_VERTICAL));
	dest_res_sizer->Add(from_video, wxSizerFlags(1));

	auto dest_res_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Destination Resolution"));
	dest_res_box->Add(dest_res_sizer, wxSizerFlags(1).Expand().Border(wxBOTTOM));
	dest_res_box->Add(change_ar);

	auto main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(margin_box, wxSizerFlags(1).Expand().Border());
	main_sizer->Add(source_res_sizer, wxSizerFlags(0).Expand().Border());
	main_sizer->Add(dest_res_box, wxSizerFlags(0).Expand().Border());
	main_sizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL | wxHELP), wxSizerFlags().Expand().Border(wxALL & ~wxTOP));
	SetSizerAndFit(main_sizer);
	CenterOnParent();

	// Bind events
	using std::bind;
	Bind(wxEVT_BUTTON, bind(&HelpButton::OpenPage, "Resample resolution"), wxID_HELP);
	Bind(wxEVT_SPINCTRL, [=](wxCommandEvent&) { UpdateButtons(); });
	from_video->Bind(wxEVT_BUTTON, &DialogResample::SetDestFromVideo, this);
	from_script->Bind(wxEVT_BUTTON, &DialogResample::SetSourceFromScript, this);
	symmetrical->Bind(wxEVT_CHECKBOX, &DialogResample::OnSymmetrical, this);
	margin_ctrl[LEFT]->Bind(wxEVT_SPINCTRL, bind(&DialogResample::OnMarginChange, this, margin_ctrl[LEFT], margin_ctrl[RIGHT]));
	margin_ctrl[TOP]->Bind(wxEVT_SPINCTRL, bind(&DialogResample::OnMarginChange, this, margin_ctrl[TOP], margin_ctrl[BOTTOM]));
}

void DialogResample::SetDestFromVideo(wxCommandEvent &) {
	dest_x->SetValue(video_w);
	dest_y->SetValue(video_h);
}

void DialogResample::SetSourceFromScript(wxCommandEvent&) {
	source_x->SetValue(script_w);
	source_y->SetValue(script_h);
}

void DialogResample::UpdateButtons() {
	from_video->Enable(c->videoController->IsLoaded() &&
		(dest_x->GetValue() != video_w || dest_y->GetValue() != video_h));
	from_script->Enable(source_x->GetValue() != script_w || source_y->GetValue() != script_h);

	boost::rational<int> source_ar(source_x->GetValue(), source_y->GetValue());
	boost::rational<int> dest_ar(dest_x->GetValue(), dest_y->GetValue());
	change_ar->Enable(source_ar != dest_ar);
}

void DialogResample::OnSymmetrical(wxCommandEvent &) {
	bool state = !symmetrical->IsChecked();

	margin_ctrl[RIGHT]->Enable(state);
	margin_ctrl[BOTTOM]->Enable(state);

	if (!state) {
		margin_ctrl[RIGHT]->SetValue(margin_ctrl[LEFT]->GetValue());
		margin_ctrl[BOTTOM]->SetValue(margin_ctrl[TOP]->GetValue());
	}
}

void DialogResample::OnMarginChange(wxSpinCtrl *src, wxSpinCtrl *dst) {
	if (symmetrical->IsChecked())
		dst->SetValue(src->GetValue());
}
