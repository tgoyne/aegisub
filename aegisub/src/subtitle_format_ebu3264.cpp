// Copyright (c) 2011 Niels Martin Hansen <nielsm@aegisub.org>
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

/// @file subtitle_format_ebu3264.cpp
/// @see subtitle_format_ebu3264.h
/// @ingroup subtitle_io

// This implements support for the EBU tech 3264 (1991) subtitling data exchange format.
// Work on support for this format was sponsored by Bandai.

/*
  todo:
  - gui configuration (80% - missing store/load of last used)
  - row length enforcing (90%)
*/

#include "config.h"

#include "subtitle_format_ebu3264.h"

#ifndef AGI_PRE
#include <wx/datetime.h>
#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/valgen.h>
#include <wx/valtext.h>
#endif

#include <libaegisub/charset_conv.h>
#include <libaegisub/exception.h>
#include <libaegisub/io.h>
#include <libaegisub/scoped_ptr.h>

#include "aegisub_endian.h"
#include "ass_dialogue.h"
#include "ass_file.h"
#include "ass_override.h"
#include "ass_style.h"
#include "compat.h"
#include "main.h"
#include "text_file_writer.h"

namespace {
#pragma pack(push, 1)
	/// A binary timecode representation, packed
	struct Timecode {
		uint8_t h, m, s, f;
	};

	/// General Subtitle Information block as it appears in the file
	struct BlockGSI {
		char cpn[3];    ///< code page number
		char dfc[8];    ///< disk format code
		char dsc;       ///< display standard code
		char cct[2];    ///< character code table number
		char lc[2];     ///< language code
		char opt[32];   ///< original programme title
		char oet[32];   ///< original episode title
		char tpt[32];   ///< translated programme title
		char tet[32];   ///< translated episode title
		char tn[32];    ///< translator name
		char tcd[32];   ///< translator contact details
		char slr[16];   ///< subtitle list reference code
		char cd[6];     ///< creation date
		char rd[6];     ///< revision date
		char rn[2];     ///< revision number
		char tnb[5];    ///< total number of TTI blocks
		char tns[5];    ///< total number of subtitles
		char tng[3];    ///< total number of subtitle groups
		char mnc[2];    ///< maximum number of displayable characters in a row
		char mnr[2];    ///< maximum number of displayable rows
		char tcs;       ///< time code: status
		char tcp[8];    ///< time code: start of programme
		char tcf[8];    ///< time code: first in-cue
		char tnd;       ///< total number of disks
		char dsn;       ///< disk sequence number
		char co[3];     ///< country of origin
		char pub[32];   ///< publisher
		char en[32];    ///< editor's name
		char ecd[32];   ///< editor's contact details
		char unused[75];
		char uda[576];  ///< user defined area
	};

	/// Text and Timing Information block as it appears in the file
	struct BlockTTI {
		uint8_t  sgn; ///< subtitle group number
		uint16_t sn;  ///< subtitle number
		uint8_t  ebn; ///< extension block number
		uint8_t  cs;  ///< cumulative status
		Timecode tci; ///< time code in
		Timecode tco; ///< time code out
		uint8_t  vp;  ///< vertical position
		uint8_t  jc;  ///< justification code
		uint8_t  cf;  ///< comment flag
		char tf[112]; ///< text field
	};
#pragma pack(pop)

	/// user configuration for export
	struct EbuExportSettings {
		enum TvStandard {
			STL23     = 0, ///< 23.976 fps (non-dropframe) (marked as 24)
			STL24     = 1, ///< 24 fps (film)
			STL25     = 2, ///< 25 fps (PAL)
			STL29     = 3, ///< 29.97 fps (non-dropframe) (marked as 30)
			STL29drop = 4, ///< 29.97 fps (dropframe) (marked as 30)
			STL30     = 5, ///< 30 fps (NTSC monochrome)
		};

		enum TextEncoding {
			iso6937_2 = 0, ///< latin multibyte
			iso8859_5 = 1, ///< cyrillic
			iso8859_6 = 2, ///< arabic
			iso8859_7 = 3, ///< greek
			iso8859_8 = 4, ///< hebrew
			utf8      = 5, ///< nonstandard
		};

		/*enum LineWrappingMode {
			AutoWrap         = 0,
			AbortOverLength  = 1,
			IgnoreOverLength = 2,
		};*/

		/// Which TV standard (frame rate + timecode encoding) to use
		TvStandard tv_standard;

		/// How to encode subtitle text
		TextEncoding text_encoding;

		/// Maximum length of rows in subtitles (in characters)
		int max_line_length;

		/// Perform automatic line wrapping or ignore over-length lines?
		bool do_line_wrapping;

		/// How to deal with over-length rows
		//LineWrappingMode line_wrapping_mode;

		/// translate SSA alignments?
		bool translate_alignments;

		/// timecode of first frame the subs were timed to
		Timecode timecode_offset;

		/// are end timecodes inclusive or exclusive?
		bool inclusive_end_times;

		agi::vfr::Framerate GetFramerate() const;
		agi::charset::IconvWrapper *GetTextEncoder() const;

		/// Load saved export settings from options
		/// @param prefix Option name prefix
		void LoadFromOptions(std::string const& prefix);

		/// Save export settings to options
		/// @param prefix Option name prefix
		void SaveToOptions(std::string const& prefix) const;
	};

	/// A block of text with basic formatting information
	struct EbuFormattedText {
		wxString text;   ///< Text in this block
		bool underline;  ///< Is this block underlined?
		bool italic;     ///< Is this block italic?
		bool word_start; ///< Is it safe to line-wrap between this block and the previous one?
		EbuFormattedText(wxString const& t, bool u = false, bool i = false, bool ws = true) : text(t), underline(u), italic(i), word_start(ws) { }
	};
	typedef std::vector<EbuFormattedText> EbuTextRow;

	template<class T>
	size_t total_length(T const& row)
	{
		size_t total = 0;
		for (typename T::const_iterator it = row.begin(); it != row.end(); ++it)
			total += it->text.size();
		return total;
	}

	/// intermediate format
	struct EbuSubtitle {
		enum CumulativeStatus {
			NotCumulative    = 0,
			CumulativeStart  = 1,
			CulumativeMiddle = 2,
			CumulativeEnd    = 3
		};

		enum JustificationCode {
			UnchangedPresentation = 0,
			JustifyLeft           = 1,
			JustifyCentre         = 2,
			JustifyRight          = 3
		};

		// note: not set to constants from spec
		enum VerticalPosition {
			PositionTop,
			PositionMiddle,
			PositionBottom
		};

		int group_number; ///< always 0 for compat
		/// subtitle number is assigned when generating blocks
		CumulativeStatus cumulative_status; ///< always NotCumulative for compat
		int time_in;       ///< frame number
		int time_out;      ///< frame number
		bool comment_flag; ///< always false for compat
		JustificationCode justification_code; ///< never Unchanged presentation for compat
		VerticalPosition vertical_position;   ///< translated to row on tti conversion
		std::vector<EbuTextRow> text_rows;    ///< text split into rows, still unicode

		EbuSubtitle();
		void SetAlignment(int ass_alignment);
		void SetTextFromAss(AssDialogue *line, bool style_underline, bool style_italic, int style_align);
		void SplitLines(int max_width, int split_type);
		bool CheckLineLengths(int max_width);
	};

	/// formatting character constants
	const char EBU_FORMAT_ITALIC_ON     = '\x80';
	const char EBU_FORMAT_ITALIC_OFF    = '\x81';
	const char EBU_FORMAT_UNDERLINE_ON  = '\x82';
	const char EBU_FORMAT_UNDERLINE_OFF = '\x83';
	const char EBU_FORMAT_BOXING_ON     = '\x84';
	const char EBU_FORMAT_BOXING_OFF    = '\x85';
	const char EBU_FORMAT_LINEBREAK     = '\x8a';
	const char EBU_FORMAT_UNUSED_SPACE  = '\x8f';

	/// dialog box for getting an export configuration
	class EbuExportConfigurationDialog : public wxDialog {
		wxRadioBox *tv_standard_box;
		wxRadioBox *text_encoding_box;
		wxSpinCtrl *max_line_length_ctrl;
		wxCheckBox *auto_wrap_check;
		wxCheckBox *translate_alignments_check;
		wxCheckBox *inclusive_end_times_check;
		wxTextCtrl *timecode_offset_entry;
		void OnOk(wxCommandEvent &evt);
	public:
		EbuExportConfigurationDialog(wxWindow *owner);
		static EbuExportSettings GetExportConfiguration(wxWindow *owner = 0);
	};
}

class TimecodeValidator : public wxValidator {
	wxRegEx re;
	Timecode *value;

	static const char *timecode_regex;

	wxTextCtrl *GetCtrl() const { return dynamic_cast<wxTextCtrl*>(GetWindow()); }

	bool TransferToWindow()
	{
		wxTextCtrl *ctrl = GetCtrl();
		if (!ctrl) return false;
		ctrl->SetValue(wxString::Format("%02d:%02d:%02d:%02d", (int)value->h, (int)value->m, (int)value->s, (int)value->f));
		return true;
	}

	bool TransferFromWindow()
	{
		wxTextCtrl *ctrl = GetCtrl();
		if (!ctrl) return false;

		wxString str = ctrl->GetValue();

		if (re.Matches(str))
		{
			long h, m, s, f;
			re.GetMatch(str, 1).ToLong(&h);
			re.GetMatch(str, 2).ToLong(&m);
			re.GetMatch(str, 3).ToLong(&s);
			re.GetMatch(str, 4).ToLong(&f);
			value->h = h;
			value->m = m;
			value->s = s;
			value->f = f;
			return true;
		}
		else
		{
			return false;
		}
	}

	bool Validate(wxWindow *parent)
	{
		wxTextCtrl *ctrl = GetCtrl();
		if (!ctrl) return false;

		if (!re.Matches(ctrl->GetValue()))
		{
			wxMessageBox(_("Time code offset in incorrect format. Ensure it is entered as four groups of two digits separated by colons."), _("EBU STL export"), wxICON_EXCLAMATION);
			return false;
		}
		return true;
	}

	wxObject * Clone() const { return new TimecodeValidator(*this); }

public:
	TimecodeValidator(Timecode *target)
	: re(timecode_regex)
	, value(target)
	{
		assert(target);
	}

	TimecodeValidator(const TimecodeValidator &other)
	: re(timecode_regex)
	, value(other.value)
	{
	}
};

const char *TimecodeValidator::timecode_regex = "([[:digit:]]{2}):([[:digit:]]{2}):([[:digit:]]{2}):([[:digit:]]{2})";

agi::vfr::Framerate EbuExportSettings::GetFramerate() const
{
	switch (tv_standard)
	{
		case STL24:     return agi::vfr::Framerate(24, 1);
		case STL25:     return agi::vfr::Framerate(25, 1);
		case STL30:     return agi::vfr::Framerate(30, 1);
		case STL23:     return agi::vfr::Framerate(24000, 1001, false);
		case STL29:     return agi::vfr::Framerate(30000, 1001, false);
		case STL29drop: return agi::vfr::Framerate(30000, 1001);
		default:        return agi::vfr::Framerate(25, 1);
	}
}

agi::charset::IconvWrapper *EbuExportSettings::GetTextEncoder() const
{
	switch (text_encoding)
	{
		case iso6937_2: return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-6937-2");
		case iso8859_5: return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-8859-5");
		case iso8859_6: return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-8859-6");
		case iso8859_7: return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-8859-7");
		case iso8859_8: return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-8859-8");
		case utf8:      return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "utf-8");
		default:        return new agi::charset::IconvWrapper(wxSTRING_ENCODING, "ISO-8859-1");
	}
}

void EbuExportSettings::SaveToOptions(std::string const& prefix) const
{
	OPT_SET(prefix + "/TV Standard")->SetInt(tv_standard);
	OPT_SET(prefix + "/Text Encoding")->SetInt(text_encoding);
	OPT_SET(prefix + "/Max Line Length")->SetInt(max_line_length);
	OPT_SET(prefix + "/Do Line Wrapping")->SetBool(do_line_wrapping);
	OPT_SET(prefix + "/Translate Alignments")->SetBool(translate_alignments);
	OPT_SET(prefix + "/Inclusive End Times")->SetBool(inclusive_end_times);
	OPT_SET(prefix + "/Timecode Offset/H")->SetInt(timecode_offset.h);
	OPT_SET(prefix + "/Timecode Offset/M")->SetInt(timecode_offset.m);
	OPT_SET(prefix + "/Timecode Offset/S")->SetInt(timecode_offset.s);
	OPT_SET(prefix + "/Timecode Offset/F")->SetInt(timecode_offset.f);
}

void EbuExportSettings::LoadFromOptions(std::string const& prefix)
{
	tv_standard = (TvStandard)OPT_GET(prefix + "/TV Standard")->GetInt();
	text_encoding = (TextEncoding)OPT_GET(prefix + "/Text Encoding")->GetInt();
	max_line_length = OPT_GET(prefix + "/Max Line Length")->GetInt();
	do_line_wrapping = OPT_GET(prefix + "/Do Line Wrapping")->GetBool();
	translate_alignments = OPT_GET(prefix + "/Translate Alignments")->GetBool();
	inclusive_end_times = OPT_GET(prefix + "/Inclusive End Times")->GetBool();
	timecode_offset.h = OPT_GET(prefix + "/Timecode Offset/H")->GetInt();
	timecode_offset.m = OPT_GET(prefix + "/Timecode Offset/M")->GetInt();
	timecode_offset.s = OPT_GET(prefix + "/Timecode Offset/S")->GetInt();
	timecode_offset.f = OPT_GET(prefix + "/Timecode Offset/F")->GetInt();
}

EbuSubtitle::EbuSubtitle()
: group_number(0)
, cumulative_status(NotCumulative)
, time_in(0)
, time_out(0)
, comment_flag(false)
, justification_code(JustifyCentre)
, vertical_position(PositionBottom)
, text_rows()
{
}

void EbuSubtitle::SetAlignment(int an)
{
	switch (an)
	{
		case 1: case 2: case 3: vertical_position = PositionBottom; break;
		case 4: case 5: case 6: vertical_position = PositionMiddle; break;
		case 7: case 8: case 9: vertical_position = PositionTop;    break;
	}

	switch (an)
	{
		case 1: case 4: case 7: justification_code = JustifyLeft;   break;
		case 2: case 5: case 8: justification_code = JustifyCentre; break;
		case 3: case 6: case 9: justification_code = JustifyRight;  break;
	}
}

void EbuSubtitle::SetTextFromAss(AssDialogue *line, bool style_underline, bool style_italic, int style_align)
{
	// Helper for finding special characters
	wxRegEx special_char_search("\\\\[nN]| ", wxRE_ADVANCED);

	line->ParseASSTags();

	text_rows.clear();
	text_rows.push_back(EbuTextRow());

	// current row being worked on
	EbuTextRow *cur_row = &text_rows.back();

	// create initial text part
	cur_row->push_back(EbuFormattedText("", style_underline, style_italic, true));

	for (std::vector<AssDialogueBlock*>::iterator bl = line->Blocks.begin(); bl != line->Blocks.end(); ++bl)
	{
		AssDialogueBlock *b = *bl;
		switch (b->GetType())
		{
		case BLOCK_PLAIN:
			// find special characters and convert them
			{
				wxString text = b->GetText();

				// Skip comments
				if (text.size() && text[0] =='{')
					continue;

				// Replace hard spaces and tabs with regular spaces
				text.Replace("\\h", " ");
				text.Replace("\\t", " ");

				while (special_char_search.Matches(text))
				{
					size_t start, len;
					special_char_search.GetMatch(&start, &len);

					// add first part of text to current part
					cur_row->back().text.append(text.Left(start));

					// process special character
					wxString substr = text.Mid(start, len);
					if (substr == "\\N" || substr == "\\n")
					{
						// create a new row with current style
						bool underline = cur_row->back().underline, italic = cur_row->back().italic;
						text_rows.push_back(EbuTextRow());
						cur_row = &text_rows.back();
						cur_row->push_back(EbuFormattedText("", underline, italic, true));
					}
					else if (substr == " ")
					{
						// space character, create new word
						cur_row->back().text.append(" ");
						cur_row->push_back(EbuFormattedText("", cur_row->back().underline, cur_row->back().italic, true));
					}

					text = text.Mid(start+len);
				}
				// add the remaining text
				cur_row->back().text.append(text);
			}
			break;

		case BLOCK_OVERRIDE:
			// find relevant tags and process them
			{
				AssDialogueBlockOverride *ob = static_cast<AssDialogueBlockOverride*>(b);
				ob->ParseTags();
				bool underline = cur_row->back().underline, italic = cur_row->back().italic;
				for (std::vector<AssOverrideTag*>::iterator tag = ob->Tags.begin(); tag != ob->Tags.end(); ++tag)
				{
					AssOverrideTag *t = *tag;
					if (t->Name == "\\u")
						underline = t->Params[0]->Get<bool>(style_underline);
					else if (t->Name == "\\i")
						italic = t->Params[0]->Get<bool>(style_italic);
					else if (t->Name == "\\an")
						SetAlignment(t->Params[0]->Get<int>(style_align));
					else if (t->Name == "\\a")
					{
						if (t->Params[0]->omitted)
							SetAlignment(style_align);
						else
						{
							switch (t->Params[0]->Get<int>())
							{
								case  1: SetAlignment(1); break;
								case  2: SetAlignment(2); break;
								case  3: SetAlignment(3); break;
								case  5: SetAlignment(7); break;
								case  6: SetAlignment(8); break;
								case  7: SetAlignment(9); break;
								case  9: SetAlignment(4); break;
								case 10: SetAlignment(5); break;
								case 11: SetAlignment(6); break;
							}
						}
					}
				}

				// apply any changes
				if (underline != cur_row->back().underline || italic != cur_row->back().italic)
				{
					if (!cur_row->back().text)
					{
						// current part is empty, we can safely change formatting on it
						cur_row->back().underline = underline;
						cur_row->back().italic = italic;
					}
					else
					{
						// create a new empty part with new style
						cur_row->push_back(EbuFormattedText("", underline, italic, false));
					}
				}
			}
			break;

		default:
			// ignore block, we don't want to output it (drawing or unknown)
			break;
		}
	}

	line->ClearBlocks();
}

void EbuSubtitle::SplitLines(int max_width, int split_type)
{
	// split_type is an SSA wrap style number
	if (split_type == 2) return; // no wrapping here!
	if (split_type < 0) return;
	if (split_type > 3) return;

	std::vector<EbuTextRow> new_text;
	for (std::vector<EbuTextRow>::iterator row = text_rows.begin(); row != text_rows.end(); ++row)
	{
		int len = total_length(*row);
		if (len <= max_width)
		{
			// doesn't need splitting, copy straight over
			new_text.push_back(*row);
		}
		else
		{
			// line needs splitting
			// formatting handling has already split line into words!
			int lines_required = (len + max_width - 1) / max_width;
			int line_max_chars = len / lines_required;
			if (split_type == 1)
				line_max_chars = max_width;
			// cur_block is the walking iterator, cur_word keeps track of the last word start
			EbuTextRow::iterator cur_block, cur_word;
			cur_block = row->begin();
			cur_word = cur_block;
			int cur_word_len = 0;
			int cur_line_len = 0;
			new_text.push_back(EbuTextRow());
			while (cur_block != row->end())
			{
				cur_word_len += cur_block->text.length();
				cur_block++;
				if (cur_block == row->end() || cur_block->word_start)
				{
					// next block starts a new word, we may have to break here
					if (cur_line_len + cur_word_len > line_max_chars)
					{
						if (split_type == 0)
						{
							// include word on this row, create new blank row
							new_text.back().insert(new_text.back().end(), cur_word, cur_block);
							new_text.push_back(EbuTextRow());
							cur_line_len = 0;
							// if this was the first row in the sub, make short lines from now on
							if (new_text.size() == 2) split_type = 3;
						}
						else // (split_type == 1 || split_type == 3)
						{
							// create new row, add word there
							new_text.push_back(EbuTextRow());
							new_text.back().insert(new_text.back().end(), cur_word, cur_block);
							cur_line_len = cur_word_len;
							// if this was the first row in the row and we are doing first line short,
							// switch the rest of the lines to long
							if (new_text.size() == 2 && split_type == 3) split_type = 0;
						}
					}
					else
					{
						// no need to split, just add word to current row
						new_text.back().insert(new_text.back().end(), cur_word, cur_block);
						cur_line_len += cur_word_len;
					}
					cur_word_len = 0;
					cur_word = cur_block;
				}
			}
		}
	}
	// replace old text
	text_rows = new_text;
}

bool EbuSubtitle::CheckLineLengths(int max_width)
{
	for (std::vector<EbuTextRow>::iterator row = text_rows.begin(); row != text_rows.end(); ++row)
	{
		if ((int)total_length(*row) > max_width)
			// early return as soon as any line is over length
			return false;
	}
	// no lines failed
	return true;
}

EbuExportSettings EbuExportConfigurationDialog::GetExportConfiguration(wxWindow *owner)
{
	EbuExportSettings s;
	s.LoadFromOptions("Export EBU STL Settings");

	EbuExportConfigurationDialog dlg(owner);

	// set up validators to move data in and out
	dlg.tv_standard_box->SetValidator(wxGenericValidator((int*)&s.tv_standard));
	dlg.text_encoding_box->SetValidator(wxGenericValidator((int*)&s.text_encoding));
	dlg.translate_alignments_check->SetValidator(wxGenericValidator(&s.translate_alignments));
	dlg.max_line_length_ctrl->SetValidator(wxGenericValidator(&s.max_line_length));
	dlg.auto_wrap_check->SetValidator(wxGenericValidator(&s.do_line_wrapping));
	dlg.inclusive_end_times_check->SetValidator(wxGenericValidator(&s.inclusive_end_times));
	dlg.timecode_offset_entry->SetValidator(TimecodeValidator(&s.timecode_offset));

	if (dlg.ShowModal() == wxID_OK)
	{
		s.SaveToOptions("Export EBU STL Settings");
		return s;
	}
	else
		throw agi::UserCancelException("EBU/STL export");
}

EbuExportConfigurationDialog::EbuExportConfigurationDialog(wxWindow *owner)
: wxDialog(owner, -1, _("Export to EBU STL format"))
{
	wxString tv_standards[] = {
		_("23.976 fps (non-standard, STL24.01)"),
		_("24 fps (non-standard, STL24.01)"),
		_("25 fps (STL25.01)"),
		_("29.97 fps (non-dropframe, STL30.01)"),
		_("29.97 fps (dropframe, STL30.01)"),
		_("30 fps (STL30.01)")
	};
	tv_standard_box = new wxRadioBox(this, -1, _("TV standard"), wxDefaultPosition, wxDefaultSize, 6, tv_standards, 0, wxRA_SPECIFY_ROWS);

	wxStaticBox *timecode_control_box = new wxStaticBox(this, -1, _("Time codes"));
	timecode_offset_entry = new wxTextCtrl(this, -1, "00:00:00:00");
	inclusive_end_times_check = new wxCheckBox(this, -1, _("Out-times are inclusive"));

	wxString text_encodings[] = {
		_("ISO 6937-2 (Latin/Western Europe)"),
		_("ISO 8859-5 (Cyrillic)"),
		_("ISO 8859-6 (Arabic)"),
		_("ISO 8859-7 (Greek)"),
		_("ISO 8859-8 (Hebrew)"),
		_("UTF-8 Unicode (non-standard)")
	};
	text_encoding_box = new wxRadioBox(this, -1, _("Text encoding"), wxDefaultPosition, wxDefaultSize, 6, text_encodings, 0, wxRA_SPECIFY_ROWS);

	wxStaticBox *text_formatting_box = new wxStaticBox(this, -1, _("Text formatting"));
	max_line_length_ctrl = new wxSpinCtrl(this, -1, wxString(), wxDefaultPosition, wxSize(65, -1));
	auto_wrap_check = new wxCheckBox(this, -1, _("Automatic wrapping"));
	translate_alignments_check = new wxCheckBox(this, -1, _("Translate alignments"));

	// Default values
	// TODO: Remember last used settings
	max_line_length_ctrl->SetRange(10, 99);

	wxSizer *max_line_length_labelled = new wxBoxSizer(wxHORIZONTAL);
	max_line_length_labelled->Add(new wxStaticText(this, -1, _("Max. line length:")), 1, wxALIGN_CENTRE|wxRIGHT, 12);
	max_line_length_labelled->Add(max_line_length_ctrl, 0, 0, 0);

	wxSizer *timecode_offset_labelled = new wxBoxSizer(wxHORIZONTAL);
	timecode_offset_labelled->Add(new wxStaticText(this, -1, _("Time code offset:")), 1, wxALIGN_CENTRE|wxRIGHT, 12);
	timecode_offset_labelled->Add(timecode_offset_entry, 0, 0, 0);

	wxSizer *text_formatting_sizer = new wxStaticBoxSizer(text_formatting_box, wxVERTICAL);
	text_formatting_sizer->Add(max_line_length_labelled, 0, wxEXPAND|wxALL&~wxTOP, 6);
	text_formatting_sizer->Add(auto_wrap_check, 0, wxEXPAND|wxALL&~wxTOP, 6);
	text_formatting_sizer->Add(translate_alignments_check, 0, wxEXPAND|wxALL&~wxTOP, 6);

	wxSizer *timecode_control_sizer = new wxStaticBoxSizer(timecode_control_box, wxVERTICAL);
	timecode_control_sizer->Add(timecode_offset_labelled, 0, wxEXPAND|wxALL&~wxTOP, 6);
	timecode_control_sizer->Add(inclusive_end_times_check, 0, wxEXPAND|wxALL&~wxTOP, 6);

	wxSizer *left_column = new wxBoxSizer(wxVERTICAL);
	left_column->Add(tv_standard_box, 0, wxEXPAND|wxBOTTOM, 6);
	left_column->Add(timecode_control_sizer, 0, wxEXPAND, 0);

	wxSizer *right_column = new wxBoxSizer(wxVERTICAL);
	right_column->Add(text_encoding_box, 0, wxEXPAND|wxBOTTOM, 6);
	right_column->Add(text_formatting_sizer, 0, wxEXPAND, 0);

	wxSizer *vertical_split_sizer = new wxBoxSizer(wxHORIZONTAL);
	vertical_split_sizer->Add(left_column, 0, wxRIGHT, 6);
	vertical_split_sizer->Add(right_column, 0, 0, 0);

	wxSizer *buttons_sizer = new wxBoxSizer(wxHORIZONTAL);
	// Developers are requested to leave this message in! Intentionally not translateable.
	wxStaticText *sponsor_label = new wxStaticText(this, -1, "EBU STL format writing sponsored by Bandai");
	sponsor_label->Enable(false);
	buttons_sizer->Add(sponsor_label, 1, wxALIGN_BOTTOM, 0);
	wxButton *ok_button = new wxButton(this, wxID_OK);
	buttons_sizer->Add(ok_button, 0, wxLEFT, 6);
	buttons_sizer->Add(new wxButton(this, wxID_CANCEL), 0, wxLEFT, 6);

	wxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(vertical_split_sizer, 0, wxEXPAND|wxALL, 12);
	main_sizer->Add(buttons_sizer, 0, wxEXPAND|wxALL&~wxTOP, 12);

	SetAffirmativeId(wxID_OK);
	ok_button->SetDefault();

	SetSizerAndFit(main_sizer);
	CenterOnParent();

	Bind(wxEVT_COMMAND_BUTTON_CLICKED, &EbuExportConfigurationDialog::OnOk, this, wxID_OK);
}

void EbuExportConfigurationDialog::OnOk(wxCommandEvent &evt)
{
	// TODO: Store settings chosen for use as defaults later
	if (Validate() && TransferDataFromWindow())
	{
		EndModal(wxID_OK);
	}
}

Ebu3264SubtitleFormat::Ebu3264SubtitleFormat()
: SubtitleFormat("EBU subtitling data exchange format (EBU tech 3264, 1991)")
{
}

wxArrayString Ebu3264SubtitleFormat::GetWriteWildcards() const
{
	wxArrayString formats;
	formats.Add("stl");
	return formats;
}

void Ebu3264SubtitleFormat::WriteFile(const AssFile *src, wxString const& filename, wxString const& encoding) const
{
	// collect data from user
	EbuExportSettings export_settings = EbuExportConfigurationDialog::GetExportConfiguration();

	// apply split/merge algorithm
	AssFile copy(*src);
	StripComments(copy.Line);
	copy.Sort();
	RecombineOverlaps(copy.Line);
	MergeIdentical(copy.Line);

	int line_wrap_type = copy.GetScriptInfoAsInt("WrapStyle");

	// time->frame conversion
	// set a separator just to humor FractionalTime
	// (we do our own format string stuff and use binary representation most of the time anyway)
	agi::vfr::Framerate fps = export_settings.GetFramerate();
	Timecode &tcofs = export_settings.timecode_offset;
	int timecode_bias = fps.FrameAtSmpte(tcofs.h, tcofs.m, tcofs.s, tcofs.s);

	// convert to intermediate format
	std::vector<EbuSubtitle> subs_list;
	for (entryIter orgline = copy.Line.begin(); orgline != copy.Line.end(); ++orgline)
	{
		AssDialogue *line = dynamic_cast<AssDialogue*>(*orgline);
		if (!line) continue;

		// add a new subtitle and work on it
		subs_list.push_back(EbuSubtitle());
		EbuSubtitle &imline = subs_list.back();
		// some defaults for compatibility
		imline.group_number = 0;
		imline.comment_flag = false;
		imline.cumulative_status = EbuSubtitle::NotCumulative;
		// convert times
		imline.time_in = fps.FrameAtTime(line->Start) + timecode_bias;
		imline.time_out = fps.FrameAtTime(line->End) + timecode_bias;
		if (export_settings.inclusive_end_times)
			// cheap and possibly wrong way to ensure exclusive times, subtract one frame from end time
			imline.time_out -= 1;
		// convert alignment from style
		AssStyle *style = copy.GetStyle(line->Style);
		if (style != 0)
			imline.SetAlignment(style->alignment);
		// add text, translate formatting
		if (style != 0)
			imline.SetTextFromAss(line, style->underline, style->italic, style->alignment);
		else
			imline.SetTextFromAss(line, false, false, 2);
		// line breaking handling
		if (export_settings.do_line_wrapping)
			imline.SplitLines(export_settings.max_line_length, line_wrap_type);
		/*if (export_settings.line_wrapping_mode == EbuExportSettings::AutoWrap)
		{
			imline.SplitLines(export_settings.max_line_length, line_wrap_type);
		}
		else if (export_settings.line_wrapping_mode == EbuExportSettings::AbortOverLength)
		{
			wxLogError(_("Line over maximum length: %s"), line->Text.c_str());
			throw new Aegisub::UserCancelException("Line over length limit"); // fixme: should be something else
		}*/
	}

	// produce an empty line if there are none
	// (it still has to contain a space to not get ignored)
	if (subs_list.empty())
	{
		subs_list.push_back(EbuSubtitle());
		subs_list.back().text_rows.push_back(EbuTextRow());
		subs_list.back().text_rows.back().push_back(EbuFormattedText(" "));
	}

	// for later use!
	wxString scriptinfo_title = copy.GetScriptInfo("Title");
	wxString scriptinfo_translation = copy.GetScriptInfo("Original Translation");
	wxString scriptinfo_editing = copy.GetScriptInfo("Original Editing");

	// line breaking/length checking
	// skip for now

	// prepare a text encoder
	agi::scoped_ptr<agi::charset::IconvWrapper> encoder(export_settings.GetTextEncoder());

	// produce blocks
	uint16_t subtitle_number = 0;
	std::vector<BlockTTI> tti;
	tti.reserve(subs_list.size());
	for (std::vector<EbuSubtitle>::iterator sub = subs_list.begin(); sub != subs_list.end(); ++sub)
	{
		std::string fullstring;
		for (std::vector<EbuTextRow>::iterator row = sub->text_rows.begin(); row != sub->text_rows.end(); ++row)
		{
			// formatting is reset at the start of every row, so keep track per row
			bool underline = false, italic = false;
			for (std::vector<EbuFormattedText>::iterator block = row->begin(); block != row->end(); ++block)
			{
				// insert codes for changed formatting
				if (underline != block->underline)
				{
					fullstring += (block->underline ? EBU_FORMAT_UNDERLINE_ON : EBU_FORMAT_UNDERLINE_OFF);
					underline = block->underline;
				}
				if (italic != block->italic)
				{
					fullstring += (block->italic ? EBU_FORMAT_ITALIC_ON : EBU_FORMAT_ITALIC_OFF);
					italic = block->italic;
				}

				// convert text to specified encoding
				const char *data = reinterpret_cast<const char *>(block->text.wx_str());
#if wxUSE_UNICODE_UTF8
				size_t len = block->text.utf8_length();
#else
				size_t len = block->text.length() * sizeof(wxStringCharType);
#endif
				fullstring += encoder->Convert(std::string(data, len));
			}
			// check that this is not the last row
			if (row+1 != sub->text_rows.end())
			{
				// insert linebreak for non-final lines
				fullstring += EBU_FORMAT_LINEBREAK;
			}
		}

		// construct a base block that can be copied and filled
		BlockTTI base;
		base.sgn = sub->group_number;
		base.sn = Endian::MachineToLittle(subtitle_number);
		base.ebn = 255;
		base.cf = sub->comment_flag;
		memset(base.tf, EBU_FORMAT_UNUSED_SPACE, sizeof(base.tf));
		// time codes
		{
			int h=0, m=0, s=0, f=0;
			fps.SmpteAtFrame(sub->time_in, &h, &m, &s, &f);
			base.tci.h = h;
			base.tci.m = m;
			base.tci.s = s;
			base.tci.f = f;
			fps.SmpteAtFrame(sub->time_out, &h, &m, &s, &f);
			base.tco.h = h;
			base.tco.m = m;
			base.tco.s = s;
			base.tco.f = f;
		}
		// cumulative status
		base.cs = sub->cumulative_status;

		// vertical position
		if (sub->vertical_position == EbuSubtitle::PositionTop)
			base.vp = 0;
		else if (sub->vertical_position == EbuSubtitle::PositionMiddle)
			base.vp = std::min<size_t>(0, 50 - (10 * sub->text_rows.size()));
		else //if (sub->vertical_position == EbuSubtitle::PositionBottom)
			base.vp = 99;

		// justification code
		base.jc = sub->justification_code;

		// produce blocks from string
		uint8_t num_blocks = 0;
		size_t bytes_remaining = fullstring.size();
		std::string::iterator pos = fullstring.begin();
		while (bytes_remaining > 0)
		{
			tti.push_back(base);
			BlockTTI &block = tti.back();
			if (bytes_remaining > sizeof(block.tf))
				// the text does not end in this block, write an extension block number
				block.ebn = num_blocks;
			else
				// this was the last block for the subtitle
				block.ebn = 255;
			size_t max_writeable = std::min(sizeof(block.tf), bytes_remaining);
			std::copy(pos, pos+max_writeable, block.tf);
			bytes_remaining -= max_writeable;
			pos += max_writeable;
		}

		// increment subtitle number for next
		subtitle_number += 1;
	}

	// build header
	wxCSConv gsi_encoder(wxFONTENCODING_CP850);
	if (!gsi_encoder.IsOk())
		wxLogWarning("Could not get an IBM 850 text encoder, EBU/STL header may contain incorrectly encoded data.");
	BlockGSI gsi;
	memset(&gsi, 0x20, sizeof(gsi)); // fill with spaces
	memcpy(gsi.cpn, "850", 3);
	{
		const char *dfcstr;
		switch (export_settings.tv_standard)
		{
		case EbuExportSettings::STL23:
		case EbuExportSettings::STL24:
			dfcstr = "STL24.01";
			break;
		case EbuExportSettings::STL29:
		case EbuExportSettings::STL29drop:
		case EbuExportSettings::STL30:
			dfcstr = "STL30.01";
			break;
		case EbuExportSettings::STL25:
		default:
			dfcstr = "STL25.01";
			break;
		}
		memcpy(gsi.dfc, dfcstr, 8);
	}
	gsi.dsc = '0'; // open subtitling
	gsi.cct[0] = '0';
	if (export_settings.text_encoding == EbuExportSettings::iso6937_2)
		gsi.cct[1] = '0';
	else if (export_settings.text_encoding == EbuExportSettings::iso8859_5)
		gsi.cct[1] = '1';
	else if (export_settings.text_encoding == EbuExportSettings::iso8859_6)
		gsi.cct[1] = '2';
	else if (export_settings.text_encoding == EbuExportSettings::iso8859_7)
		gsi.cct[1] = '3';
	else if (export_settings.text_encoding == EbuExportSettings::iso8859_8)
		gsi.cct[1] = '4';
	else if (export_settings.text_encoding == EbuExportSettings::utf8)
		memcpy(gsi.cct, "U8", 2);
	memcpy(gsi.lc, "00", 2);
	gsi_encoder.FromWChar(gsi.opt, 32, scriptinfo_title.wc_str(), scriptinfo_title.length());
	gsi_encoder.FromWChar(gsi.tn, 32, scriptinfo_translation.wc_str(), scriptinfo_translation.length());
	{
		char buf[20];
		time_t now;
		time(&now);
		tm *thetime = localtime(&now);
		strftime(buf, 20, "AGI-%y%m%d%H%M%S", thetime);
		memcpy(gsi.slr, buf, 16);
		strftime(buf, 20, "%y%m%d", thetime);
		memcpy(gsi.cd, buf, 6);
		memcpy(gsi.rd, buf, 6);
		memcpy(gsi.rn, "00", 2);
		sprintf(buf, "%5u", (unsigned int)tti.size());
		memcpy(gsi.tnb, buf, 5);
		sprintf(buf, "%5u", (unsigned int)subs_list.size());
		memcpy(gsi.tns, buf, 5);
		memcpy(gsi.tng, "001", 3);
		sprintf(buf, "%02u", (unsigned int)export_settings.max_line_length);
		memcpy(gsi.mnc, buf, 2);
		memcpy(gsi.mnr, "99", 2);
		gsi.tcs = '1';
		int h, m, s, f;
		fps.SmpteAtFrame(timecode_bias, &h, &m, &s, &f);
		sprintf(buf, "%02u%02u%02u%02u", h, m, s, f);
		memcpy(gsi.tcp, buf, 8);
		BlockTTI &block0 = tti.front();
		sprintf(buf, "%02u%02u%02u%02u", (unsigned int)block0.tci.h, (unsigned int)block0.tci.m, (unsigned int)block0.tci.s, (unsigned int)block0.tci.f);
		memcpy(gsi.tcf, buf, 8);
	}
	gsi.tnd = '1';
	gsi.dsn = '1';
	memcpy(gsi.co, "NTZ", 3); // neutral zone!
	gsi_encoder.FromWChar(gsi.en, 32, scriptinfo_editing.wc_str(), scriptinfo_editing.length());
	if (export_settings.text_encoding == EbuExportSettings::utf8)
	{
		char warning_str[] = "This file was exported by Aegisub using non-standard UTF-8 encoding for the subtitle blocks. The TTI.TF field contains UTF-8-encoded text interspersed with the standard formatting codes, which are not encoded. GSI.CCT is set to 'U8' to signify this.";
		memcpy(gsi.uda, warning_str, sizeof(warning_str));
	}
	
	// write file
	agi::io::Save f(STD_STR(filename), true);
	f.Get().write((const char *)&gsi, sizeof(gsi));
	for (std::vector<BlockTTI>::iterator block = tti.begin(); block != tti.end(); ++block)
	{
		f.Get().write((const char *)&*block, sizeof(*block));
	}
}
