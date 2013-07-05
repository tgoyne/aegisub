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

#include "dialog_style_editor.h"

#include "ass_style.h"
#include "compat.h"
#include "help_button.h"
#include "include/aegisub/subtitles_provider.h"
#include "libresrc/libresrc.h"
#include "options.h"
#include "persist_location.h"
#include "style_manager.h"
#include "subs_preview.h"
#include "utils.h"
#include "validators.h"

#include <libaegisub/util.h>

#include <future>
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

struct EncodingValidator : public BinderHelper<wxComboBox, int, EncodingValidator> {
	EncodingValidator(int *value) : base(value) { }

	void ToWindow(wxComboBox *ctrl, int *value) {
		wxString str_value(std::to_wstring(*value));

#if 0
		for (size_t i = 0; i < countof(encodings); ++i) {
			if (encodings[i].StartsWith(str_value)) {
				ctrl->Select(i);
				return;
			}
		}
#endif
		ctrl->Select(0);
	}

	int FromWindow(wxComboBox *ctrl) {
		long templ = 0;
		ctrl->GetValue().BeforeFirst('-').ToLong(&templ);
		return templ;
	}
};

struct OpaqueBoxValidator : public BinderHelper<wxCheckBox, int, OpaqueBoxValidator> {
	OpaqueBoxValidator(int *value) : base(value) { }

	void ToWindow(wxCheckBox *ctrl, int *value) {
		ctrl->SetValue(*value == 3);
	}

	int FromWindow(wxCheckBox *ctrl) {
		return ctrl->IsChecked() ? 3 : 1;
	}
};

struct AlignmentValidator : public BinderHelper<wxRadioBox, int, AlignmentValidator> {
	AlignmentValidator(int *value) : base(value) { }

	void ToWindow(wxRadioBox *ctrl, int *value) {
		switch (*value) {
			case 7:  ctrl->SetSelection(0); break;
			case 8:  ctrl->SetSelection(1); break;
			case 9:  ctrl->SetSelection(2); break;
			case 4:  ctrl->SetSelection(3); break;
			case 5:  ctrl->SetSelection(4); break;
			case 6:  ctrl->SetSelection(5); break;
			case 1:  ctrl->SetSelection(6); break;
			case 2:  ctrl->SetSelection(7); break;
			case 3:  ctrl->SetSelection(8); break;
			default: ctrl->SetSelection(7); break;
		}
	}

	int FromWindow(wxRadioBox *ctrl) {
		switch (ctrl->GetSelection()) {
			case 0:  return 7;
			case 1:  return 8;
			case 2:  return 9;
			case 3:  return 4;
			case 4:  return 5;
			case 5:  return 6;
			case 6:  return 1;
			case 7:  return 2;
			case 8:  return 3;
			default: return 2;
		}
	}
};

wxComboBox *encoding_cb(wxWindow *parent, int *value) {
	wxString encodings[] = {
		wxS("0 - ") + _("ANSI"),
		wxS("1 - ") + _("Default"),
		wxS("2 - ") + _("Symbol"),
		wxS("77 - ") + _("Mac"),
		wxS("128 - ") + _("Shift_JIS"),
		wxS("129 - ") + _("Hangeul"),
		wxS("130 - ") + _("Johab"),
		wxS("134 - ") + _("GB2312"),
		wxS("136 - ") + _("Chinese BIG5"),
		wxS("161 - ") + _("Greek"),
		wxS("162 - ") + _("Turkish"),
		wxS("163 - ") + _("Vietnamese"),
		wxS("177 - ") + _("Hebrew"),
		wxS("178 - ") + _("Arabic"),
		wxS("186 - ") + _("Baltic"),
		wxS("204 - ") + _("Russian"),
		wxS("222 - ") + _("Thai"),
		wxS("238 - ") + _("East European"),
		wxS("255 - ") + _("OEM")
	};
	return new wxComboBox(parent, -1, "", wxDefaultPosition, wxDefaultSize, countof(encodings), encodings, wxCB_READONLY, EncodingValidator(value));
}

}

static void add_with_label(wxSizer *sizer, wxWindow *parent, wxString const& label, wxWindow *ctrl) {
	sizer->Add(new wxStaticText(parent, -1, label), wxSizerFlags().Center().Right().Border(wxLEFT | wxRIGHT));
	sizer->Add(ctrl, wxSizerFlags(1).Left().Expand());
}

struct adder {
	wxSizer *sizer;
	wxSizerFlags default_flags;
	int direction;

	adder(wxSizer *sizer, wxSizerFlags default_flags, int direction)
	: sizer(sizer)
	, default_flags(default_flags)
	, direction(direction)
	{ }

	void operator()() { sizer->AddStretchSpacer(); }

	template<typename T>
	void operator()(T item) {
		if (sizer->IsEmpty() || !direction || (sizer->GetItemCount() == 1 && sizer->GetItem(0u)->IsSpacer()))
			sizer->Add(item, default_flags);
		else if (direction == wxVERTICAL)
			sizer->Add(item, default_flags.Border(wxTOP));
		else
			sizer->Add(item, default_flags.Border(wxLEFT));
	}

	template<typename T>
	void operator()(wxSizerFlags flags, T item) { sizer->Add(item, flags); }

	template<typename T>
	void operator()(wxWindow *parent, wxString const& label, T item) {
		sizer->Add(new wxStaticText(parent, -1, label), wxSizerFlags().Center().Right().Border(wxLEFT | wxRIGHT));
		sizer->Add(item, wxSizerFlags(1).Left().Expand());
	}
};

template<typename F>
wxSizer *static_box_sizer(int type, wxWindow *parent, wxString const& label, F&& func, wxSizerFlags flags) {
	auto sizer = new wxStaticBoxSizer(type, parent, label);
	func(adder(sizer, flags, type));
	return sizer;
}

template<typename F>
wxSizer *horizontal(wxWindow *parent, wxString const& label, F&& func) {
	return static_box_sizer(wxHORIZONTAL, parent, label, func, wxSizerFlags());
}

template<typename F>
wxSizer *horizontal(wxSizerFlags flags, wxWindow *parent, wxString const& label, F&& func) {
	return static_box_sizer(wxHORIZONTAL, parent, label, func, flags);
}

template<typename F>
wxSizer *vertical(wxWindow *parent, wxString const& label, F&& func) {
	return static_box_sizer(wxVERTICAL, parent, label, func, wxSizerFlags());
}

template<typename F>
wxSizer *box_sizer(int type, F&& func, wxSizerFlags flags) {
	auto sizer = new wxBoxSizer(type);
	func(adder(sizer, flags, type));
	return sizer;
}

template<typename F>
wxSizer *vertical(F&& func) {
	return box_sizer(wxVERTICAL, func, wxSizerFlags());
}

template<typename F>
wxSizer *vertical(wxSizerFlags default_flags, F&& func) {
	return box_sizer(wxVERTICAL, func, default_flags);
}

template<typename F>
wxSizer *horizontal(F&& func) {
	return box_sizer(wxHORIZONTAL, func, wxSizerFlags());
}

template<typename F>
wxSizer *horizontal(wxSizerFlags default_flags, F&& func) {
	return box_sizer(wxHORIZONTAL, func, default_flags);
}

template<typename F>
wxSizer *grid(F&& func) {
	auto sizer = new wxFlexGridSizer(2, 4, 5, 5);
	func(adder(sizer, wxSizerFlags(), 0));
	return sizer;
}

template<typename T>
T *tooltip(T *control, wxString const& tooltip) {
	control->SetToolTip(tooltip);
	return control;
}

DialogStyleEditor::DialogStyleEditor(wxWindow *parent, AssStyle *style, StyleManager *manager)
: wxDialog (parent, -1, _("Style Editor"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
, c(c)
, style(style)
, work(agi::util::make_unique<AssStyle>(*style))
, manager(manager)
{
	// Can take a while on systems with lots of fonts
	auto fontList = std::async(std::launch::async, []() -> wxArrayString {
		auto fontList = wxFontEnumerator::GetFacenames();
		fontList.Sort();
		return fontList;
	});

	// Helpers for creating controls
	auto num_text_ctrl = [&](double *value, bool allow_negative, wxSize size, wxString const& tooltip) -> wxTextCtrl * {
		wxFloatingPointValidator<double> validator(value, wxNUM_VAL_NO_TRAILING_ZEROES);
		if (!allow_negative)
			validator.SetMin(0);
		auto ctrl = new wxTextCtrl(this, -1, "", wxDefaultPosition, size, 0, validator);
		ctrl->SetToolTip(tooltip);
		return ctrl;
	};

	auto color_button = [&](agi::Color *value, wxString const& label, wxString const& tooltip) -> wxSizer * {
		auto ctrl = new ColourButton(this, wxSize(55, 16), true, *value, ColorBinder(value));
		ctrl->SetToolTip(tooltip);
		ctrl->Bind(EVT_COLOR, [=](wxThreadEvent&) { UpdateWorkStyle(); });
		return vertical(wxSizerFlags().Center().Border(wxBOTTOM), [&](adder add) {
			add(new wxStaticText(this, -1, label));
			add(ctrl);
		});
	};

	auto checkbox = [&](wxString const& label, bool *field) -> wxCheckBox * {
		auto ctrl = new wxCheckBox(this, -1, label);
		ctrl->SetValidator(wxGenericValidator(field));
		return ctrl;
	};

	auto margin_ctrl = [&](int *value, wxString const& label, wxString const& tooltip) -> wxSizer * {
		auto ctrl = new wxSpinCtrl(this, -1, "", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 0, 9999, 0);
		ctrl->SetValidator(wxGenericValidator(value));
		ctrl->SetToolTip(tooltip);

		return vertical(wxSizerFlags().Center(), [&](adder add) {
			add();
			add(new wxStaticText(this, -1, label));
			add(ctrl);
			add();
		});
	};

	auto align_ctrl = [&]() -> wxRadioBox * {
		wxString alignValues[9] = { "7", "8", "9", "4", "5", "6", "1", "2", "3" };
		auto ctrl = tooltip(new wxRadioBox(this, -1, _("Alignment"), wxDefaultPosition, wxDefaultSize, 9, alignValues, 3, wxRA_SPECIFY_COLS), _("Alignment in screen, in numpad style"));
		ctrl->SetValidator(AlignmentValidator(&work->alignment));
		return ctrl;
	};

	auto outline_cb = [&]() -> wxCheckBox * {
		auto outline_type = tooltip(new wxCheckBox(this, -1, _("&Opaque box")), _("When selected, display an opaque box behind the subtitles instead of an outline around the text"));
		outline_type->SetValidator(OpaqueBoxValidator(&work->borderstyle));
		return outline_type;
	};

	// Actually set up the dialog
	SetIcon(GETICON(style_toolbutton_16));

	auto expand_0 = wxSizerFlags().Expand();
	auto expand_1 = wxSizerFlags(1).Expand();

	wxComboBox *font_name;
	wxTextCtrl *style_name;

	SetSizerAndFit(vertical([&](adder A) {
		A(expand_1.Center().Border(), horizontal([&](adder A) {
			A(expand_0, vertical(expand_0, [&](adder A) {
				A(horizontal(this, _("Style Name"), [&](adder A) {
					A(wxSizerFlags(1), style_name = tooltip(new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, 0, StringBinder(&work->name)), _("Style name")));
				}));
				A(vertical(this, _("Font"), [&](adder A) {
					A(expand_1, horizontal([&](adder A) {
						A(wxSizerFlags(1), font_name = tooltip(new wxComboBox(this, -1, "", wxDefaultPosition, wxSize(150, -1), 0, 0, wxCB_DROPDOWN, StringBinder(&work->font)), _("Font face")));
						A(wxSizerFlags().Border(wxLEFT), num_text_ctrl(&work->fontsize, false, wxSize(50, -1), _("Font size")));
					}));
					A(expand_1.Border(wxTOP), horizontal([&](adder A) {
						A();
						A(checkbox(_("&Bold"), &work->bold));
						A(checkbox(_("&Italic"), &work->italic));
						A(checkbox(_("&Underline"), &work->underline));
						A(checkbox(_("&Strikeout"), &work->strikeout));
						A();
					}));
				}));
				A(horizontal(this, _("Colors"), [&](adder A) {
					A();
					A(color_button(&work->primary, _("Primary"), _("Choose primary color")));
					A(color_button(&work->secondary, _("Secondary"), _("Choose secondary color")));
					A(color_button(&work->outline, _("Outline"), _("Choose outline color")));
					A(color_button(&work->shadow, _("Shadow"), _("Choose shadow color")));
					A();
				}));
				A(horizontal([&](adder A) {
					A(expand_1, horizontal(expand_0, this, _("Margins"), [&](adder A) {
						A();
						A(margin_ctrl(&work->Margin[0], _("Left"), _("Distance from left edge, in pixels")));
						A(margin_ctrl(&work->Margin[1], _("Right"), _("Distance from right edge, in pixels")));
						A(margin_ctrl(&work->Margin[2], _("Vert"), _("Distance from top/bottom edge, in pixels")));
						A();
					}));
					A(expand_0.Border(wxLEFT), align_ctrl());
				}));
			}));
			A(expand_1.Border(wxLEFT), vertical(expand_0, [&](adder A) {
				A(horizontal(this, _("Outline"), [&](adder A) {
					A(this, _("Outline:"), num_text_ctrl(&work->outline_w, false, wxSize(50, -1), _("Outline width, in pixels")));
					A(this, _("Shadow:"), num_text_ctrl(&work->shadow_w, true, wxSize(50, -1), _("Shadow distance, in pixels")));
					A(wxSizerFlags().Center().Border(wxLEFT), outline_cb());
				}));
				A(vertical(this, _("Miscellaneous"), [&](adder A) {
					A(expand_0.Center(), grid([&](adder A) {
						A(this, _("Scale X%:"), num_text_ctrl(&work->scalex, false, wxSize(70, 20), _("Scale X, in percentage")));
						A(this, _("Scale Y%:"), num_text_ctrl(&work->scaley, false, wxSize(70, 20), _("Scale Y, in percentage")));
						A(this, _("Rotation:"), num_text_ctrl(&work->angle, true, wxSize(70, 20), _("Angle to rotate in Z axis, in degrees")));
						A(this, _("Spacing:"), num_text_ctrl(&work->spacing, true, wxSize(70, 20), _("Character spacing, in pixels")));
					}));
					A(expand_0.Center().Border(wxTOP), horizontal([&](adder A) {
						A(this, _("Encoding:"), tooltip(encoding_cb(this, &work->encoding), _("Encoding, only useful in unicode if the font doesn't have the proper unicode mapping")));
					}));
				}));
				A(expand_1.Border(wxTOP), vertical(this, _("Preview"), [&](adder A) {
					auto preview_button = tooltip(new ColourButton(this, wxSize(45, 16), false, OPT_GET("Colour/Style Editor/Background/Preview")->GetColor()), _("Color of preview background"));
					preview_text = tooltip(new wxTextCtrl(this, -1, to_wx(OPT_GET("Tool/Style Editor/Preview Text")->GetString())), _("Text to be used for the preview"));
					preview = tooltip(new SubtitlesPreview(this, wxSize(100, 60), wxSUNKEN_BORDER, OPT_GET("Colour/Style Editor/Background/Preview")->GetColor()), _("Preview of current style"));

					preview->SetStyle(*style);
					preview->SetText(from_wx(preview_text->GetValue()));
					preview_button->Bind(EVT_COLOR, &DialogStyleEditor::OnPreviewColourChange, this);
					preview_text->Bind(wxEVT_COMMAND_TEXT_UPDATED, &DialogStyleEditor::OnPreviewTextChange, this);

					A(expand_1.Border(wxBOTTOM), preview);
					A(expand_0, horizontal([&](adder A) {
						A(expand_1.Border(wxRIGHT), preview_text);
						A(expand_0, preview_button);
					}));
				}));
			}));
		}));
		A(expand_0.Center().Border(wxBOTTOM), CreateStdDialogButtonSizer(wxOK | wxCANCEL | wxAPPLY | wxHELP));
	}));

	persist = agi::util::make_unique<PersistLocation>(this, "Tool/Style Editor", true);

	Bind(wxEVT_CHILD_FOCUS, [&](wxChildFocusEvent &evt) {
		evt.Skip();
		UpdateWorkStyle();
	});

	auto update_preview = [&](wxCommandEvent &evt) {
		evt.Skip();
		UpdateWorkStyle();
	};

	Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, update_preview);
	Bind(wxEVT_COMMAND_COMBOBOX_SELECTED, update_preview);
	Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, update_preview);

	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Apply, this, true, true), wxID_OK);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Apply, this, true, false), wxID_APPLY);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogStyleEditor::Apply, this, false, true), wxID_CANCEL);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&HelpButton::OpenPage, "Style Editor"), wxID_HELP);

	// Fill font face list box
	font_name->Bind(wxEVT_COMMAND_TEXT_ENTER, update_preview);
	font_name->Freeze();
	font_name->Append(fontList.get());
	font_name->SetValue(to_wx(work->font));
	font_name->Thaw();
}

DialogStyleEditor::~DialogStyleEditor() { }

void DialogStyleEditor::Apply(bool apply, bool close) {
	if (apply) {
		UpdateWorkStyle();

		auto existing = manager->GetStyle(work->name);
		if (existing && existing != style) {
			wxMessageBox(_("There is already a style with this name. Please choose another name."), _("Style name conflict"), wxOK | wxICON_ERROR | wxCENTER);
			return;
		}

		if (work->name != style->name) {
			if (manager->NeedsRenameProcessing(style->name, work->name)) {
				int answer = wxMessageBox(
					_("Do you want to change all instances of this style in the script to this new name?"),
					_("Update script?"),
					wxYES_NO | wxCANCEL);

				if (answer == wxCANCEL) return;

				if (answer == wxYES)
					manager->RenameStyle(style, work->name);
			}
		}

		*style = *work;
		style->UpdateData();
		manager->Save(_("style change"));
	}

	if (close) {
		EndModal(apply);
		OPT_SET("Tool/Style Editor/Preview Text")->SetString(from_wx(preview_text->GetValue()));
	}
}

void DialogStyleEditor::UpdateWorkStyle() {
	TransferDataFromWindow();
	preview->SetStyle(*work);
}

void DialogStyleEditor::OnPreviewTextChange(wxCommandEvent &event) {
	preview->SetText(from_wx(preview_text->GetValue()));
	event.Skip();
}

void DialogStyleEditor::OnPreviewColourChange(wxThreadEvent &evt) {
	preview->SetColour(evt.GetPayload<agi::Color>());
	OPT_SET("Colour/Style Editor/Background/Preview")->SetColor(evt.GetPayload<agi::Color>());
}
