// Copyright (c) 2005-2010, Niels Martin Hansen
// Copyright (c) 2005-2010, Rodrigo Braz Monteiro
// Copyright (c) 2010, Amar Takhar
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

#include "command.h"

#include "../compat.h"
#include "../include/aegisub/context.h"
#include "../libresrc/libresrc.h"
#include "../options.h"
#include "../project.h"
#include "../utils.h"

#include <libaegisub/keyframe.h>
#include <libaegisub/make_unique.h>

namespace {
	using cmd::Command;

struct keyframe_close final : public Command {
	CMD_NAME("keyframe/close")
	CMD_ICON(close_keyframes_menu)
	STR_MENU("Close Keyframes")
	STR_DISP("Close Keyframes")
	STR_HELP("Discard the currently loaded keyframes and use those from the video, if any")
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) override {
		return c->project->CanCloseKeyframes();
	}

	void operator()(agi::Context *c) override {
		c->project->CloseKeyframes();
	}
};

struct keyframe_open final : public Command {
	CMD_NAME("keyframe/open")
	CMD_ICON(open_keyframes_menu)
	STR_MENU("Open Keyframes...")
	STR_DISP("Open Keyframes")
	STR_HELP("Open a keyframe list file")

	void operator()(agi::Context *c) override {
		auto filename = OpenFileSelector(
			_("Open keyframes file"),
			"Path/Last/Keyframes", "" ,".txt",
			from_wx(_("All Supported Formats") +
				" (*.txt, *.pass, *.stats, *.log)|*.txt;*.pass;*.stats;*.log|" +
				_("All Files") + " (*.*)|*.*"),
			c->parent);

		if (!filename.empty())
			c->project->LoadKeyframes(filename);
	}
};

struct keyframe_save final : public Command {
	CMD_NAME("keyframe/save")
	CMD_ICON(save_keyframes_menu)
	STR_MENU("Save Keyframes...")
	STR_DISP("Save Keyframes")
	STR_HELP("Save the current list of keyframes to a file")
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) override {
		return !c->project->Keyframes().empty();
	}

	void operator()(agi::Context *c) override {
		auto filename = SaveFileSelector(_("Save keyframes file"), "Path/Last/Keyframes", "", "*.key.txt", "Text files (*.txt)|*.txt", c->parent);
		if (filename.empty()) return;

		agi::keyframe::Save(filename, c->project->Keyframes());
		config::mru->Add("Keyframes", filename);
	}
};
}

namespace cmd {
	void init_keyframe() {
		reg(agi::make_unique<keyframe_close>());
		reg(agi::make_unique<keyframe_open>());
		reg(agi::make_unique<keyframe_save>());
	}
}
