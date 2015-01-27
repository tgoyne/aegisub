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
#include "../dialog_manager.h"
#include "../dialog_styling_assistant.h"
#include "../dialog_translation.h"
#include "../dialogs.h"
#include "../include/aegisub/context.h"
#include "../libresrc/libresrc.h"
#include "../options.h"
#include "../resolution_resampler.h"
#include "../video_controller.h"

#include <libaegisub/fs.h>
#include <libaegisub/path.h>
#include <libaegisub/make_unique.h>

#include <wx/msgdlg.h>
#include <wx/utils.h>

namespace {
	using cmd::Command;

struct tool_assdraw final : public Command {
	CMD_NAME("tool/assdraw")
	CMD_ICON(assdraw)
	STR_MENU("ASSDraw3...")
	STR_DISP("ASSDraw3")
	STR_HELP("Launch the ASSDraw3 tool for vector drawing")

	void operator()(agi::Context *) override {
		wxExecute("\"" + config::path->Decode("?data/ASSDraw3.exe").wstring() + "\"");
	}
};

struct tool_export final : public Command {
	CMD_NAME("tool/export")
	CMD_ICON(export_menu)
	STR_MENU("&Export Subtitles...")
	STR_DISP("Export Subtitles")
	STR_HELP("Save a copy of subtitles in a different format or with processing applied to it")

	void operator()(agi::Context *c) override {
		c->videoController->Stop();
		ShowExportDialog(c);
	}
};

struct tool_font_collector final : public Command {
	CMD_NAME("tool/font_collector")
	CMD_ICON(font_collector_button)
	STR_MENU("&Fonts Collector...")
	STR_DISP("Fonts Collector")
	STR_HELP("Open fonts collector")

	void operator()(agi::Context *c) override {
		ShowFontsCollectorDialog(c);
	}
};

struct tool_line_select final : public Command {
	CMD_NAME("tool/line/select")
	CMD_ICON(select_lines_button)
	STR_MENU("S&elect Lines...")
	STR_DISP("Select Lines")
	STR_HELP("Select lines based on defined criteria")

	void operator()(agi::Context *c) override {
		ShowSelectLinesDialog(c);
	}
};

struct tool_resampleres final : public Command {
	CMD_NAME("tool/resampleres")
	CMD_ICON(resample_toolbutton)
	STR_MENU("&Resample Resolution...")
	STR_DISP("Resample Resolution")
	STR_HELP("Resample subtitles to maintain their current appearance at a different script resolution")

	void operator()(agi::Context *c) override {
		c->videoController->Stop();
		ResampleSettings settings;
		if (PromptForResampleSettings(c, settings))
			ResampleResolution(c->ass.get(), settings);
	}
};

struct tool_style_assistant final : public Command {
	CMD_NAME("tool/style/assistant")
	CMD_ICON(styling_toolbutton)
	STR_MENU("St&yling Assistant...")
	STR_DISP("Styling Assistant")
	STR_HELP("Open styling assistant")

	void operator()(agi::Context *c) override {
		c->dialog->Show<DialogStyling>(c);
	}
};

struct tool_styling_assistant_validator : public Command {
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) override {
		return !!c->dialog->Get<DialogStyling>();
	}
};

struct tool_styling_assistant_commit final : public tool_styling_assistant_validator {
	CMD_NAME("tool/styling_assistant/commit")
	STR_MENU("&Accept changes")
	STR_DISP("Accept changes")
	STR_HELP("Commit changes and move to the next line")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogStyling>()->Commit(true);
	}
};

struct tool_styling_assistant_preview final : public tool_styling_assistant_validator {
	CMD_NAME("tool/styling_assistant/preview")
	STR_MENU("&Preview changes")
	STR_DISP("Preview changes")
	STR_HELP("Commit changes and stay on the current line")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogStyling>()->Commit(false);
	}
};

struct tool_style_manager final : public Command {
	CMD_NAME("tool/style/manager")
	CMD_ICON(style_toolbutton)
	STR_MENU("&Styles Manager...")
	STR_DISP("Styles Manager")
	STR_HELP("Open the styles manager")

	void operator()(agi::Context *c) override {
		ShowStyleManagerDialog(c);
	}
};

struct tool_time_kanji final : public Command {
	CMD_NAME("tool/time/kanji")
	CMD_ICON(kara_timing_copier)
	STR_MENU("&Kanji Timer...")
	STR_DISP("Kanji Timer")
	STR_HELP("Open the Kanji timer copier")

	void operator()(agi::Context *c) override {
		ShowKanjiTimerDialog(c);
	}
};

struct tool_time_postprocess final : public Command {
	CMD_NAME("tool/time/postprocess")
	CMD_ICON(timing_processor_toolbutton)
	STR_MENU("&Timing Post-Processor...")
	STR_DISP("Timing Post-Processor")
	STR_HELP("Post-process the subtitle timing to add lead-ins and lead-outs, snap timing to scene changes, etc.")

	void operator()(agi::Context *c) override {
		ShowTimingProcessorDialog(c);
	}
};

struct tool_translation_assistant final : public Command {
	CMD_NAME("tool/translation_assistant")
	CMD_ICON(translation_toolbutton)
	STR_MENU("&Translation Assistant...")
	STR_DISP("Translation Assistant")
	STR_HELP("Open translation assistant")

	void operator()(agi::Context *c) override {
		c->videoController->Stop();
		try {
			c->dialog->ShowModal<DialogTranslation>(c);
		}
		catch (DialogTranslation::NothingToTranslate const&) {
			wxMessageBox(_("There is nothing to translate in the file."));
		}
	}
};

struct tool_translation_assistant_validator : public Command {
	CMD_TYPE(COMMAND_VALIDATE)

	bool Validate(const agi::Context *c) override {
		return !!c->dialog->Get<DialogTranslation>();
	}
};

struct tool_translation_assistant_commit final : public tool_translation_assistant_validator {
	CMD_NAME("tool/translation_assistant/commit")
	STR_MENU("&Accept changes")
	STR_DISP("Accept changes")
	STR_HELP("Commit changes and move to the next line")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogTranslation>()->Commit(true);
	}
};

struct tool_translation_assistant_preview final : public tool_translation_assistant_validator {
	CMD_NAME("tool/translation_assistant/preview")
	STR_MENU("&Preview changes")
	STR_DISP("Preview changes")
	STR_HELP("Commit changes and stay on the current line")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogTranslation>()->Commit(false);
	}
};

struct tool_translation_assistant_next final : public tool_translation_assistant_validator {
	CMD_NAME("tool/translation_assistant/next")
	STR_MENU("&Next Line")
	STR_DISP("Next Line")
	STR_HELP("Move to the next line without committing changes")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogTranslation>()->NextBlock();
	}
};

struct tool_translation_assistant_prev final : public tool_translation_assistant_validator {
	CMD_NAME("tool/translation_assistant/prev")
	STR_MENU("&Previous Line")
	STR_DISP("Previous Line")
	STR_HELP("Move to the previous line without committing changes")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogTranslation>()->PrevBlock();
	}
};

struct tool_translation_assistant_insert final : public tool_translation_assistant_validator {
	CMD_NAME("tool/translation_assistant/insert_original")
	STR_MENU("&Insert Original")
	STR_DISP("Insert Original")
	STR_HELP("Insert the untranslated text")

	void operator()(agi::Context *c) override {
		c->dialog->Get<DialogTranslation>()->InsertOriginal();
	}
};
}

namespace cmd {
	void init_tool() {
		reg(agi::make_unique<tool_export>());
		reg(agi::make_unique<tool_font_collector>());
		reg(agi::make_unique<tool_line_select>());
		reg(agi::make_unique<tool_resampleres>());
		reg(agi::make_unique<tool_style_assistant>());
		reg(agi::make_unique<tool_styling_assistant_commit>());
		reg(agi::make_unique<tool_styling_assistant_preview>());
		reg(agi::make_unique<tool_style_manager>());
		reg(agi::make_unique<tool_time_kanji>());
		reg(agi::make_unique<tool_time_postprocess>());
		reg(agi::make_unique<tool_translation_assistant>());
#ifdef _WIN32
		if (agi::fs::FileExists(config::path->Decode("?data/ASSDraw3.exe")))
			reg(agi::make_unique<tool_assdraw>());
#endif
		reg(agi::make_unique<tool_translation_assistant_commit>());
		reg(agi::make_unique<tool_translation_assistant_preview>());
		reg(agi::make_unique<tool_translation_assistant_next>());
		reg(agi::make_unique<tool_translation_assistant_prev>());
		reg(agi::make_unique<tool_translation_assistant_insert>());
	}
}
