// Copyright (c) 2006, Rodrigo Braz Monteiro
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

/// @file subtitle_format_srt.cpp
/// @brief Reading/writing SubRip format subtitles (.SRT)
/// @ingroup subtitle_io
///

#include "config.h"

#include "subtitle_format_srt.h"

#include "ass_attachment.h"
#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_style.h"
#include "colorspace.h"
#include "compat.h"
#include "utils.h"
#include "text_file_reader.h"
#include "text_file_writer.h"

#include <libaegisub/of_type_adaptor.h>

#include <boost/format.hpp>
#include <map>
#include <wx/regex.h>

DEFINE_SIMPLE_EXCEPTION(SRTParseError, SubtitleFormatParseError, "subtitle_io/parse/srt")

namespace {
class SrtTagParser {
	struct FontAttribs {
		wxString face;
		wxString size;
		wxString color;
	};

	enum TagType {
		// leave 0 unused so indexing an unknown tag in the map won't clash
		TAG_BOLD_OPEN = 1,
		TAG_BOLD_CLOSE,
		TAG_ITALICS_OPEN,
		TAG_ITALICS_CLOSE,
		TAG_UNDERLINE_OPEN,
		TAG_UNDERLINE_CLOSE,
		TAG_STRIKEOUT_OPEN,
		TAG_STRIKEOUT_CLOSE,
		TAG_FONT_OPEN,
		TAG_FONT_CLOSE
	};

	wxRegEx tag_matcher;
	wxRegEx attrib_matcher;
	std::map<wxString,TagType> tag_name_cases;

public:
	SrtTagParser()
	: tag_matcher("^(.*?)<(/?b|/?i|/?u|/?s|/?font)([^>]*)>(.*)$", wxRE_ICASE|wxRE_ADVANCED)
	, attrib_matcher("^[[:space:]]+(face|size|color)=('[^']*'|\"[^\"]*\"|[^[:space:]]+)", wxRE_ICASE|wxRE_ADVANCED)
	{
		if (!tag_matcher.IsValid())
			throw agi::InternalError("Parsing SRT: Failed compiling tag matching regex", 0);
		if (!attrib_matcher.IsValid())
			throw agi::InternalError("Parsing SRT: Failed compiling tag attribute matching regex", 0);

		tag_name_cases["b"]  = TAG_BOLD_OPEN;
		tag_name_cases["/b"] = TAG_BOLD_CLOSE;
		tag_name_cases["i"]  = TAG_ITALICS_OPEN;
		tag_name_cases["/i"] = TAG_ITALICS_CLOSE;
		tag_name_cases["u"]  = TAG_UNDERLINE_OPEN;
		tag_name_cases["/u"] = TAG_UNDERLINE_CLOSE;
		tag_name_cases["s"]  = TAG_STRIKEOUT_OPEN;
		tag_name_cases["/s"] = TAG_STRIKEOUT_CLOSE;
		tag_name_cases["font"] = TAG_FONT_OPEN;
		tag_name_cases["/font"] = TAG_FONT_CLOSE;
	}

	std::string ToAss(wxString srt)
	{
		int bold_level = 0;
		int italics_level = 0;
		int underline_level = 0;
		int strikeout_level = 0;
		std::vector<FontAttribs> font_stack;

		wxString ass; // result to be built

		while (!srt.empty())
		{
			if (!tag_matcher.Matches(srt))
			{
				// no more tags could be matched, end of string
				ass.append(srt);
				break;
			}

			// we found a tag, translate it
			wxString pre_text  = tag_matcher.GetMatch(srt, 1);
			wxString tag_name  = tag_matcher.GetMatch(srt, 2);
			wxString tag_attrs = tag_matcher.GetMatch(srt, 3);
			wxString post_text = tag_matcher.GetMatch(srt, 4);

			// the text before the tag goes through unchanged
			ass.append(pre_text);
			// the text after the tag is the input for next iteration
			srt = post_text;

			switch (tag_name_cases[tag_name.Lower()])
			{
			case TAG_BOLD_OPEN:
				if (bold_level == 0)
					ass.append("{\\b1}");
				bold_level++;
				break;
			case TAG_BOLD_CLOSE:
				if (bold_level == 1)
					ass.append("{\\b}");
				if (bold_level > 0)
					bold_level--;
				break;
			case TAG_ITALICS_OPEN:
				if (italics_level == 0)
					ass.append("{\\i1}");
				italics_level++;
				break;
			case TAG_ITALICS_CLOSE:
				if (italics_level == 1)
					ass.append("{\\i}");
				if (italics_level > 0)
					italics_level--;
				break;
			case TAG_UNDERLINE_OPEN:
				if (underline_level == 0)
					ass.append("{\\u1}");
				underline_level++;
				break;
			case TAG_UNDERLINE_CLOSE:
				if (underline_level == 1)
					ass.append("{\\u}");
				if (underline_level > 0)
					underline_level--;
				break;
			case TAG_STRIKEOUT_OPEN:
				if (strikeout_level == 0)
					ass.append("{\\s1}");
				strikeout_level++;
				break;
			case TAG_STRIKEOUT_CLOSE:
				if (strikeout_level == 1)
					ass.append("{\\s}");
				if (strikeout_level > 0)
					strikeout_level--;
				break;
			case TAG_FONT_OPEN:
				{
					// new attributes to fill in
					FontAttribs new_attribs;
					FontAttribs old_attribs;
					// start out with any previous ones on stack
					if (font_stack.size() > 0)
					{
						old_attribs = font_stack.back();
					}
					new_attribs = old_attribs;
					// now find all attributes on this font tag
					while (attrib_matcher.Matches(tag_attrs))
					{
						// get attribute name and values
						wxString attr_name = attrib_matcher.GetMatch(tag_attrs, 1);
						wxString attr_value = attrib_matcher.GetMatch(tag_attrs, 2);
						// clean them
						attr_name.MakeLower();
						if ((attr_value.StartsWith("'") && attr_value.EndsWith("'")) ||
							(attr_value.StartsWith("\"") && attr_value.EndsWith("\"")))
						{
							attr_value = attr_value.Mid(1, attr_value.Len()-2);
						}
						// handle the attributes
						if (attr_name == "face")
						{
							new_attribs.face = wxString::Format("{\\fn%s}", attr_value);
						}
						else if (attr_name == "size")
						{
							new_attribs.size = wxString::Format("{\\fs%s}", attr_value);
						}
						else if (attr_name == "color")
						{
							new_attribs.color = wxString::Format("{\\c%s}", to_wx(agi::Color(from_wx(attr_value)).GetAssOverrideFormatted()));
						}
						// remove this attribute to prepare for the next
						size_t attr_pos, attr_len;
						attrib_matcher.GetMatch(&attr_pos, &attr_len, 0);
						tag_attrs.erase(attr_pos, attr_len);
					}
					// the attributes changed from old are then written out
					if (new_attribs.face != old_attribs.face)
						ass.append(new_attribs.face);
					if (new_attribs.size != old_attribs.size)
						ass.append(new_attribs.size);
					if (new_attribs.color != old_attribs.color)
						ass.append(new_attribs.color);
					// lastly dump the new attributes state onto the stack
					font_stack.push_back(new_attribs);
				}
				break;
			case TAG_FONT_CLOSE:
				{
					// this requires a font stack entry
					if (font_stack.empty())
						break;
					// get the current attribs
					FontAttribs cur_attribs = font_stack.back();
					// remove them from the stack
					font_stack.pop_back();
					// grab the old attributes if there are any
					FontAttribs old_attribs;
					if (font_stack.size() > 0)
						old_attribs = font_stack.back();
					// then restore the attributes to previous settings
					if (cur_attribs.face != old_attribs.face)
					{
						if (old_attribs.face.empty())
							ass.append("{\\fn}");
						else
							ass.append(old_attribs.face);
					}
					if (cur_attribs.size != old_attribs.size)
					{
						if (old_attribs.size.empty())
							ass.append("{\\fs}");
						else
							ass.append(old_attribs.size);
					}
					if (cur_attribs.color != old_attribs.color)
					{
						if (old_attribs.color.empty())
							ass.append("{\\c}");
						else
							ass.append(old_attribs.color);
					}
				}
				break;
			default:
				// unknown tag, replicate it in the output
				ass.append("<").append(tag_name).append(tag_attrs).append(">");
				break;
			}
		}

		// make it a little prettier, join tag groups
		ass.Replace("}{", "", true);

		return from_wx(ass);
	}
};

AssTime ReadSRTTime(wxString const& ts)
{
	// For the sake of your sanity, please do not read this function.

	int d, h, m, s, ms;
	d = h = m = s = ms = 0;

	size_t ci = 0;
	int ms_chars = 0;

	for (; ci < ts.length(); ++ci)
	{
		char ch = ts[ci];
		switch (ch)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			s = s * 10 + (ch - '0');
			break;
		case ':':
			d = h;
			h = m;
			m = s;
			s = 0;
			break;
		case ',':
			ci++;
			goto milliseconds;
		default:
			goto allparsed;
		}
	}
	goto allparsed;
milliseconds:
	for (; ci < ts.length(); ++ci)
	{
		char ch = ts[ci];
		switch (ch)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			ms = ms * 10 + (ch - '0');
			ms_chars++;
			break;
		default:
			goto allparsed;
		}
	}
allparsed:
	while (ms_chars < 3) ms *= 10, ms_chars++;
	while (ms_chars > 3) ms /= 10, ms_chars--;

	return ms + 1000*(s + 60*(m + 60*(h + d*24)));
}

std::string WriteSRTTime(AssTime const& ts)
{
	return str(boost::format("%02d:%02d:%02d %%03d") % ts.GetTimeHours() % ts.GetTimeMinutes() % ts.GetTimeSeconds() % ts.GetTimeMiliseconds());
}

}

SRTSubtitleFormat::SRTSubtitleFormat()
: SubtitleFormat("SubRip")
{
}

std::vector<std::string> SRTSubtitleFormat::GetReadWildcards() const {
	std::vector<std::string> formats;
	formats.push_back("srt");
	return formats;
}

std::vector<std::string> SRTSubtitleFormat::GetWriteWildcards() const {
	return GetReadWildcards();
}

enum ParseState {
	STATE_INITIAL,
	STATE_TIMESTAMP,
	STATE_FIRST_LINE_OF_BODY,
	STATE_REST_OF_BODY,
	STATE_LAST_WAS_BLANK
};

void SRTSubtitleFormat::ReadFile(AssFile *target, std::string const& filename, std::string const& encoding) const {
	using namespace std;

	TextFileReader file(filename, encoding);
	target->LoadDefault(false);

	// See parsing algorithm at <http://devel.aegisub.org/wiki/SubtitleFormats/SRT>

	// "hh:mm:ss,fff --> hh:mm:ss,fff" (e.g. "00:00:04,070 --> 00:00:10,04")
	wxRegEx timestamp_regex("^([0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{1,}) --> ([0-9]{2}:[0-9]{2}:[0-9]{2},[0-9]{1,})");
	if (!timestamp_regex.IsValid())
		throw agi::InternalError("Parsing SRT: Failed compiling regex", 0);

	SrtTagParser tag_parser;

	ParseState state = STATE_INITIAL;
	int line_num = 0;
	int linebreak_debt = 0;
	AssDialogue *line = 0;
	wxString text;
	while (file.HasMoreLines()) {
		wxString text_line = to_wx(file.ReadLineFromFile());
		line_num++;
		text_line.Trim(true).Trim(false);

		switch (state) {
			case STATE_INITIAL:
				// ignore leading blank lines
				if (text_line.empty()) break;
				if (text_line.IsNumber()) {
					// found the line number, throw it away and hope for timestamps
					state = STATE_TIMESTAMP;
					break;
				}
				if (timestamp_regex.Matches(text_line))
					goto found_timestamps;

				throw SRTParseError(from_wx(wxString::Format("Parsing SRT: Expected subtitle index at line %d", line_num)), 0);
			case STATE_TIMESTAMP:
				if (!timestamp_regex.Matches(text_line))
					throw SRTParseError(from_wx(wxString::Format("Parsing SRT: Expected timestamp pair at line %d", line_num)), 0);
found_timestamps:
				if (line) {
					// finalize active line
					line->Text = tag_parser.ToAss(text);
					text.clear();
				}
				// create new subtitle
				line = new AssDialogue;
				line->Start = ReadSRTTime(timestamp_regex.GetMatch(text_line, 1));
				line->End = ReadSRTTime(timestamp_regex.GetMatch(text_line, 2));
				// store pointer to subtitle, we'll continue working on it
				target->Line.push_back(*line);
				// next we're reading the text
				state = STATE_FIRST_LINE_OF_BODY;
				break;
			case STATE_FIRST_LINE_OF_BODY:
				if (text_line.empty()) {
					// that's not very interesting... blank subtitle?
					state = STATE_LAST_WAS_BLANK;
					// no previous line that needs a line break after
					linebreak_debt = 0;
					break;
				}
				text.Append(text_line);
				state = STATE_REST_OF_BODY;
				break;
			case STATE_REST_OF_BODY:
				if (text_line.empty()) {
					// Might be either the gap between two subtitles or just a
					// blank line in the middle of a subtitle, so defer adding
					// the line break until we check what's on the next line
					state = STATE_LAST_WAS_BLANK;
					linebreak_debt = 1;
					break;
				}
				text.Append("\\N").Append(text_line);
				break;
			case STATE_LAST_WAS_BLANK:
				++linebreak_debt;
				if (text_line.empty()) break;
				if (text_line.IsNumber()) {
					// Hopefully it's the start of a new subtitle, and the
					// previous blank line(s) were the gap between subtitles
					state = STATE_TIMESTAMP;
					break;
				}
				if (timestamp_regex.Matches(text_line))
					goto found_timestamps;

				// assume it's a continuation of the subtitle text
				// resolve our line break debt and append the line text
				while (linebreak_debt-- > 0)
					text.Append("\\N");
				text.Append(text_line);
				state = STATE_REST_OF_BODY;
				break;
		}
	}

	if (state == 1 || state == 2)
		throw SRTParseError("Parsing SRT: Incomplete file", 0);

	if (line)
		// an unfinalized line
		line->Text = tag_parser.ToAss(text);
}

void SRTSubtitleFormat::WriteFile(const AssFile *src, std::string const& filename, std::string const& encoding) const {
	TextFileWriter file(filename, encoding);

	// Convert to SRT
	AssFile copy(*src);
	copy.Sort();
	StripComments(copy);
	RecombineOverlaps(copy);
	MergeIdentical(copy);
#ifdef _WIN32
	ConvertNewlines(copy, "\r\n", false);
#else
	ConvertNewlines(copy, "\n", false);
#endif

	// Write lines
	int i=0;
	for (auto current : copy.Line | agi::of_type<AssDialogue>()) {
		file.WriteLineToFile(std::to_string(++i));
		file.WriteLineToFile(WriteSRTTime(current->Start) + " --> " + WriteSRTTime(current->End));
		file.WriteLineToFile(ConvertTags(current));
		file.WriteLineToFile("");
	}
}

bool SRTSubtitleFormat::CanSave(const AssFile *file) const {
	std::string supported_tags[] = { "\\b", "\\i", "\\s", "\\u" };

	AssStyle defstyle;
	for (auto const& line : file->Line) {
		// Check style, if anything non-default is found, return false
		if (const AssStyle *curstyle = dynamic_cast<const AssStyle*>(&line)) {
			if (curstyle->GetEntryData() != defstyle.GetEntryData())
				return false;
		}

		// Check for attachments, if any is found, return false
		if (dynamic_cast<const AssAttachment*>(&line)) return false;

		// Check dialogue
		if (const AssDialogue *curdiag = dynamic_cast<const AssDialogue*>(&line)) {
			boost::ptr_vector<AssDialogueBlock> blocks(curdiag->ParseTags());
			for (auto ovr : blocks | agi::of_type<AssDialogueBlockOverride>()) {
				// Verify that all overrides used are supported
				for (auto const& tag : ovr->Tags) {
					if (!std::binary_search(supported_tags, std::end(supported_tags), tag.Name))
						return false;
				}
			}
		}
	}

	return true;
}

std::string SRTSubtitleFormat::ConvertTags(const AssDialogue *diag) const {
	std::string final;
	std::map<char, bool> tag_states;
	tag_states['i'] = false;
	tag_states['b'] = false;
	tag_states['u'] = false;
	tag_states['s'] = false;

	boost::ptr_vector<AssDialogueBlock> blocks(diag->ParseTags());

	for (auto& block : blocks) {
		if (AssDialogueBlockOverride* ovr = dynamic_cast<AssDialogueBlockOverride*>(&block)) {
			// Iterate through overrides
			for (auto const& tag : ovr->Tags) {
				if (tag.IsValid() && tag.Name.size() == 2) {
					auto it = tag_states.find(tag.Name[1]);
					if (it != tag_states.end()) {
						bool temp = tag.Params[0].Get(false);
						if (temp && !it->second)
							final += str(boost::format("<%c>") % it->first);
						if (!temp && it->second)
							final += str(boost::format("</%c>") % it->first);
						it->second = temp;
					}
				}
			}
		}
		// Plain text
		else if (AssDialogueBlockPlain *plain = dynamic_cast<AssDialogueBlockPlain*>(&block))
			final += plain->GetText();
	}

	// Ensure all tags are closed
	// Otherwise unclosed overrides might affect lines they shouldn't, see bug #809 for example
	for (auto it : tag_states) {
		if (it.second)
			final += str(boost::format("</%c>") % it.first);
	}

	return final;
}
