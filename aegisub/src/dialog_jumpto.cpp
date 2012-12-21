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

/// @file dialog_jumpto.cpp
/// @brief Dialogue box to enter a time to seek video to
/// @ingroup secondary_ui
///

#include "config.h"

#include "dialog_jumpto.h"

#include "include/aegisub/context.h"
#include "ass_time.h"
#include "libresrc/libresrc.h"
#include "timeedit_ctrl.h"
#include "video_context.h"
#include "wx_helpers.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

DialogJumpTo::DialogJumpTo(agi::Context *c)
: wxDialog(c->parent, -1, _("Jump to"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxWANTS_CHARS)
, c(c)
, jumpframe(c->videoController->GetFrameN())
{
	SetIcon(GETICON(jumpto_button_16));

	JumpFrame = TextCtrl(this, &jumpframe, wxDefaultSize, wxTE_PROCESS_ENTER);
	JumpFrame->SetMaxLength(std::to_string(c->videoController->GetLength() - 1).size());
	JumpTime = new TimeEdit(this, c, AssTime(c->videoController->TimeAtFrame(jumpframe)).GetAssFormated());

	wxFlexGridSizer *TimesSizer = new wxFlexGridSizer(2, 5, 5);

	TimesSizer->Add(StaticText(this, _("Frame: ")), wxSizerFlags().Center().Right());
	TimesSizer->Add(JumpFrame);

	TimesSizer->Add(StaticText(this, _("Time: ")), wxSizerFlags().Center().Right());
	TimesSizer->Add(JumpTime);

	wxSizer *MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(TimesSizer, wxSizerFlags().Border().Center());
	MainSizer->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), wxSizerFlags().Border(wxALL & ~wxTOP));

	SetSizerAndFit(MainSizer);
	CenterOnParent();

	Bind(wxEVT_INIT_DIALOG, &DialogJumpTo::OnInitDialog, this);
	Bind(wxEVT_COMMAND_TEXT_ENTER, &DialogJumpTo::OnOK, this);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DialogJumpTo::OnOK, this, wxID_OK);
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, std::bind(&DialogJumpTo::EndModal, this, 0), wxID_CANCEL);
	JumpTime->Bind(wxEVT_COMMAND_TEXT_UPDATED, &DialogJumpTo::OnEditTime, this);
	JumpFrame->Bind(wxEVT_COMMAND_TEXT_UPDATED, &DialogJumpTo::OnEditFrame, this);
}

void DialogJumpTo::OnInitDialog(wxInitDialogEvent&) {
	TransferDataToWindow();
	UpdateWindowUI(wxUPDATE_UI_RECURSE);

	// This can't simply be done in the constructor as the value hasn't been set yet
	JumpFrame->SetFocus();
	JumpFrame->SelectAll();
}

void DialogJumpTo::OnOK(wxCommandEvent &) {
	EndModal(0);
	c->videoController->JumpToFrame(std::min<int>(jumpframe, c->videoController->GetLength() - 1));
}

void DialogJumpTo::OnEditTime(wxCommandEvent &) {
	size_t newframe = c->videoController->FrameAtTime(JumpTime->GetTime());
	if (jumpframe != newframe) {
		jumpframe = newframe;
		TransferDataToWindow();
	}
}

void DialogJumpTo::OnEditFrame(wxCommandEvent &) {
	TransferDataFromWindow();
	JumpTime->SetTime(c->videoController->TimeAtFrame(jumpframe));
}
