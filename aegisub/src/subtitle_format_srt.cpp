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

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <map>
#include <unordered_map>

DEFINE_SIMPLE_EXCEPTION(SRTParseError, SubtitleFormatParseError, "subtitle_io/parse/srt")

namespace {

using boost::xpressive::sregex;

class SrtTagParser {
	std::unordered_map<std::string, std::string> tag_aliases;
	std::unordered_map<std::string, std::string> font_attributes_to_ass;

public:
	SrtTagParser() {
		tag_aliases["strong"] = "b";
		tag_aliases["em"] = "i";
		tag_aliases["strike"] = "s";
		tag_aliases["del"] = "s";

		font_attributes_to_ass["face"] = "\\fn";
		font_attributes_to_ass["size"] = "\\fs";
		font_attributes_to_ass["color"] = "\\c";
		font_attributes_to_ass["outline-color"] = "\\3c";
		font_attributes_to_ass["shadow-color"] = "\\4c";
		font_attributes_to_ass["outline-level"] = "\\bord";
		font_attributes_to_ass["shadow-level"] = "\\shad";
	}

	std::string ToAss(std::string const& srt)
	{
		std::string font_tags_to_close;

		std::string ass;
		ass.reserve(srt.size());

		size_t end = 0;
		for (size_t pos = 0; pos = srt.find('<', end); pos != std::string::npos) {
			// Append everything between this tag and the end of the last one
			ass.append(srt.begin() + end, srt.begin() + pos);

			// Find the end of the tag
			end = srt.find('>', pos);
			if (end == std::string::npos) {
				end = pos;
				break;
			}

			// end needs to point at the character after the > for the sake of
			// the code after the loop
			++end;

			// Things below marked as VSFilter compatibility are incorrect for
			// parsing HTML, but match VSFilter's (very broken) parsing

			// Incorrect for VSFilter compatibility
			if (end - pos > 4 && !strncmp(&srt[pos + 1], "!--", 3)) {
				ass +='{';
				ass.append(
					// Skipping <!--
					srt.begin() + pos + 4,
					// Skipping -->, if present
					srt.begin() + end - (strncmp(&srt[end - 4], "--", 2) ? 1 : 3));
				ass += '}';
				continue;
			}

			// Check if this is a closing tag
			size_t tag_name_start = pos + 1;
			bool closing_tag = false;
			if (srt[tag_name_start] == '/') {
				++tag_name_start;
				closing_tag = true;
			}

			// Incorrect for VSFilter compatibility
			tag_name_start = srt.find_first_not_of("/ ", tag_name_start);

			// Incorrect for VSFilter compatibility
			size_t tag_name_end = srt.find(' ', tag_name_start, end - tag_name_start - 1);
			if (tag_name_end == std::string::npos)
				tag_name_end = end - 1;

			std::string tag_name = srt.substr(tag_name_start, tag_name_end - tag_name_start);
			boost::to_lower(tag_name);

			auto alias_it = tag_aliases.find(tag_name);
			if (alias_it != tag_aliases.end())
				tag_name = *alias_it;

			if (tag_name.size() == 1 && boost::is_any_of("bisu")(tag_name)) {
				ass += "{\\";
				ass += tag_name;
				if (!closing_tag)
					ass += '1';
				ass += '}';
				continue;
			}

			if (tag_name == "font") {
				if (closing_tag) {
					// Incorrect for VSFilter compatibility
					if (!font_tags_to_close.empty()) {
						ass += '{';
						ass += font_tags_to_close;
						ass += '}';
						font_tags_to_close.clear();
					}
					continue;
				}

				size_t attr_name_start = tag_name_end;
				size_t attr_name_end;
				while ((attr_name_end = srt.find('=', attr_name_start) != std::string::npos) {
					std::string attr_name = srt.substr(attr_name_start, attr_name_end - attr_name_start);
					boost::trim(attr_name);
					boost::to_lower(attr_name);

					size_t attr_value_start = attr_name_end;
					while (isspace(++attr_value_start)) ;

					size_t attr_value_end;
					if (srt[attr_value_start] == '"') {
						++attr_value_start;
						attr_value_end = srt.find('"', attr_value_start);
					}
					else
						attr_value_end = srt.find(' ', attr_value_start);

					if (attr_value_end >= end)
						attr_value_end = end - 1;

					attr_name_start = attr_value_end;

					auto attr_it = font_attributes_to_ass.find(attr_name);
					if (attr_it == font_attributes_to_ass.end())
						continue;

					boost::iterator_range<std::string::iterator> attr_value(srt.begin() + attr_value_start, srt.begin() + attr_value_end);
					boost::trim(attr_value);
					// VSFilter also lowercases the value for some reason, but
					// that makes font names ugly

					font_tags_to_close += *attr_it;
					ass += '{';
					ass += *attr_it;

					if (attr_name.back() == 'c')
						// Doesn't support HTML color names, while VSFilter
						// does. Not really worth caring about.
						ass += agi::Color(attr_value).GetAssOverrideFormatted();
					else
						ass.append(attr_value.begin(), attr_value.end());

					ass += '}';
				}
			}

			// Unrecognized tag, so just append it
			ass.append(srt.begin() + pos, srt.begin() + end + 1);
		}

		// Append everything after the last tag
		ass.append(srt.begin() + end, srt.end());

		// Join adjacent override blocks
		boost::replace_all(ass, "}{", "");

		return ass;
	}
};

AssTime ReadSRTTime(std::string const& ts)
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
