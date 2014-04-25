// Copyright (c) 2007, Rodrigo Braz Monteiro, Niels Martin Hansen
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

/// @file tooltip_manager.cpp
/// @brief Generate tooltips for controls by combining a base text and any hotkeys found for the function
/// @ingroup custom_control
///

#include "tooltip_manager.h"

#include "compat.h"
#include "include/aegisub/hotkey.h"

#include <libaegisub/hotkey.h>

#include <list>
#include <wx/weakref.h>
#include <wx/window.h>

struct ToolTipBinding {
	wxWeakRef<wxWindow> window;
	wxString toolTip;
	const char *command;
	const char *context;
	void Update();
};

void ToolTipManager::Bind(wxWindow *window, wxString tooltip, const char *context, const char *command) {
	ToolTipBinding tip{window, tooltip, command, context};
	tip.Update();

	static std::list<ToolTipBinding> tips;
	tips.push_back(tip);
	hotkey::inst->AddHotkeyChangeListener(&ToolTipBinding::Update, &tips.back());
}

void ToolTipBinding::Update() {
	if (!window.get()) return;

	std::vector<std::string> hotkeys = hotkey::get_hotkey_strs(context, command);

	std::string str;
	for (size_t i = 0; i < hotkeys.size(); ++i) {
		if (i > 0) str += "/";
		str += hotkeys[i];
	}
	if (str.empty()) {
		window->SetToolTip(toolTip);
	}
	else {
		window->SetToolTip(toolTip + to_wx(" (" + str + ")"));
	}
}
