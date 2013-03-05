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

/// @file export_framerate.h
/// @see export_framerate.cpp
/// @ingroup export
///

#include "ass_export_filter.h"

#include <libaegisub/vfr.h>

class AssDialogue;
class AssOverrideParameter;
class wxCheckBox;
class wxRadioButton;
class wxTextCtrl;

struct FramerateTransformSettings {
	agi::vfr::Framerate source;
	agi::vfr::Framerate dest;
};

int ConvertTime(int time, FramerateTransformSettings const& settings) {
	int frame_number   = settings.source.FrameAtTime(time);
	int frame_start    = settings.source.TimeAtFrame(frame_number);
	int frame_end      = settings.source.TimeAtFrame(frame_number + 1);
	int frame_duration = frame_end - frame_start;
	double dist = double(time - frame_start) / frame_duration;

	int frame_start_new    = settings.dest.TimeAtFrame(frame_number);
	int frame_end_new      = settings.dest.TimeAtFrame(frame_number + 1);
	int frame_duration_new = frame_end_new - frame_start_new;

	// Match the same relative position between the frames as the old time had
	return frame_start_new + frame_duration_new * double(time - frame_start) / frame_duration;
}

int TruncateToCentiseconds(int time) {
	return (time / 10) * 10;
}

void TransformLine(AssDialogue *line, FramerateTransformSettings const& settings) {
	AssTime new_start_time = TruncateToCentiseconds(ConvertTime(line->Start, settings));
	AssTime new_end_time   = TruncateToCentiseconds(ConvertTime(line->End, settings));
		newK = 0;
		oldK = 0;
		newStart = trunc_cs(ConvertTime(curDialogue->Start));
		newEnd = trunc_cs(ConvertTime(curDialogue->End) + 9);

		// Process stuff
		boost::ptr_vector<AssDialogueBlock> blocks;
		for (auto block : blocks | agi::of_type<AssDialogueBlockOverride>())
			block->ProcessParameters(TransformTimeTags, this);
		curDialogue->Start = newStart;
		curDialogue->End = newEnd;
		curDialogue->UpdateText(blocks);

}

/// @class AssTransformFramerateFilter
/// @brief Transform subtitle times, including those in override tags, from an input framerate to an output framerate
class AssTransformFramerateFilter : public AssExportFilter {
	agi::Context *c;
	AssDialogue *line;
	int newStart;
	int newEnd;
	int newK;
	int oldK;

	// Yes, these are backwards
	const agi::vfr::Framerate *Input;  ///< Destination frame rate
	const agi::vfr::Framerate *Output; ///< Source frame rate

	agi::vfr::Framerate t1,t2;

	wxTextCtrl *InputFramerate; ///< Input frame rate text box
	wxTextCtrl *OutputFramerate; ///< Output frame rate text box

	wxRadioButton *RadioOutputCFR; ///< CFR radio control
	wxRadioButton *RadioOutputVFR; ///< VFR radio control

	wxCheckBox *Reverse; ///< Switch input and output

	/// @brief Apply the transformation to a file
	/// @param subs File to process
	void TransformFrameRate(AssFile *subs);
	/// @brief Transform a single tag
	/// @param name Name of the tag
	/// @param curParam Current parameter being processed
	/// @param userdata Filter instance
	static void TransformTimeTags(std::string const& name, AssOverrideParameter *curParam, void *userdata);

	/// @brief Convert a time from the input frame rate to the output frame rate
	/// @param time Time in ms to convert
	/// @return Time in ms
	///
	/// This preserves two things:
	///   1. The frame number
	///   2. The relative distance between the beginning of the frame which time
	///      is in and the beginning of the next frame
	int ConvertTime(int time);
public:
	/// Constructor
	AssTransformFramerateFilter();
	void ProcessSubs(AssFile *subs, wxWindow *);
	wxWindow *GetConfigDialogWindow(wxWindow *parent, agi::Context *c);
	void LoadSettings(bool is_default, agi::Context *c);
};
