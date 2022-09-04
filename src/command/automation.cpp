// Copyright (c) 2005-2010, Niels Martin Hansen
// Copyright (c) 2005-2010, Rodrigo Braz Monteiro
// Copyright (c) 2010, Amar Takhar
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//	 this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//	 this list of conditions and the following disclaimer in the documentation
//	 and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//	 may be used to endorse or promote products derived from this software
//	 without specific prior written permission.
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

#include "../auto4_base.h"
#include "../dialogs.h"
#include "../frame_main.h"
#include "../include/aegisub/context.h"
#include "../libresrc/libresrc.h"
#include "../options.h"


namespace {
	using cmd::Command;

struct reload_all final : public Command {
	CMD_NAME("am/reload")
	STR_MENU("&Reload Automation scripts")
	STR_DISP("Reload Automation scripts")
	STR_HELP("Reload all Automation scripts and rescan the autoload folder")

	void operator()(agi::Context *c) override {
		config::global_scripts->Reload();
		c->local_scripts->Reload();
		c->frame->StatusTimeout(_("Reloaded all Automation scripts"));
	}
};

struct reload_autoload final : public Command {
	CMD_NAME("am/reload/autoload")
	STR_MENU("R&eload autoload Automation scripts")
	STR_DISP("Reload autoload Automation scripts")
	STR_HELP("Rescan the Automation autoload folder")

	void operator()(agi::Context *c) override {
		config::global_scripts->Reload();
		c->frame->StatusTimeout(_("Reloaded autoload Automation scripts"));
	}
};

struct open_manager final : public Command {
	CMD_NAME("am/manager")
	CMD_ICON(automation_toolbutton)
	STR_MENU("&Automation...")
	STR_DISP("Automation")
	STR_HELP("Open automation manager")

	void operator()(agi::Context *c) override {
		ShowAutomationDialog(c);
	}
};

struct meta final : public Command {
	CMD_NAME("am/meta")
	CMD_ICON(automation_toolbutton)
	STR_MENU("&Automation...")
	STR_DISP("Automation")
	STR_HELP("Open automation manager. Ctrl: Rescan autoload folder. Ctrl+Shift: Rescan autoload folder and reload all automation scripts")

	void operator()(agi::Context *c) override {
		if (wxGetMouseState().CmdDown()) {
			if (wxGetMouseState().ShiftDown())
				cmd::call("am/reload", c);
			else
				cmd::call("am/reload/autoload", c);
		}
		else
			cmd::call("am/manager", c);
	}
};

}

namespace cmd {
	void init_automation() {
		reg(std::make_unique<meta>());
		reg(std::make_unique<open_manager>());
		reg(std::make_unique<reload_all>());
		reg(std::make_unique<reload_autoload>());
	}
}
