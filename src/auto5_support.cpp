// Copyright (c) 2014, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "ass_style.h"
#include "compat.h"
#include "include/aegisub/context.h"
#include "options.h"
#include "utils.h"

#include <libaegisub/charset_conv.h>
#include <libaegisub/charset_conv_win.h>
#include <libaegisub/path.h>

namespace {
struct string {
	const char *data;
	size_t len;
};

struct error_string {
	string value;
};

struct extents {
	double width, height, descent, extlead;
};

#define MAYBE(type)                                                       \
struct maybe_##type {                                                     \
	bool is_error;                                                        \
	union {                                                               \
		string error;                                                     \
		type result;                                                      \
	};                                                                    \
                                                                          \
	maybe_##type(error_string err) : is_error(true), error(err.value) { } \
	maybe_##type(type res) : is_error(false), result(res) { }             \
};

MAYBE(string)
MAYBE(extents)

struct library {
	agi::Context *c = nullptr;
	std::string holder;

	error_string error(std::string err) {
		holder = std::move(err);
		return {{holder.data(), holder.size()}};
	}

	maybe_string result(std::string ret) {
		holder = std::move(ret);
		return maybe_string(string{ret.data(), ret.size()});
	}
};
}

extern "C" library *auto5_init() {
	return new library;
}

extern "C" void auto5_deinit(library *lib) {
	delete lib;
}

extern "C" maybe_string auto5_clipboard_get(library *lib) {
	return lib->result(GetClipboard());
}

extern "C" bool auto5_clipboard_set(library *lib, const char *str, size_t len) {
	bool succeeded = false;

#if wxUSE_OLE
	// OLE needs to be initialized on each thread that wants to write to
	// the clipboard, which wx does not handle automatically
	wxClipboard cb;
#else
	wxClipboard &cb = *wxTheClipboard;
#endif
	if (cb.Open()) {
		succeeded = cb.SetData(new wxTextDataObject(wxString::FromUTF8(str, len)));
		cb.Close();
		cb.Flush();
	}
	return succeeded;
}

extern "C" maybe_string auto5_gettext(library *lib, string str) {
	return lib->result(from_wx(wxGetTranslation(wxString::FromUTF8(str.data, str.len))));
}

extern "C" maybe_string auto5_path_decode(library *lib, string str) {
	return lib->result(config::path->Decode(std::string(str.data, str.len)).string());
}

extern "C" maybe_extents auto5_text_extents(library *lib, AssStyle *style, string str) {
	extents ret{0.0, 0.0, 0.0, 0.0};

	double fontsize = style->fontsize * 64;
	double spacing = style->spacing * 64;

#ifdef WIN32
	// This is almost copypasta from TextSub
	HDC thedc = CreateCompatibleDC(0);
	if (!thedc) return lib->error("Failed to create DC");
	SetMapMode(thedc, MM_TEXT);

	LOGFONTW lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfHeight = (LONG)fontsize;
	lf.lfWeight = style->bold ? FW_BOLD : FW_NORMAL;
	lf.lfItalic = style->italic;
	lf.lfUnderline = style->underline;
	lf.lfStrikeOut = style->strikeout;
	lf.lfCharSet = style->encoding;
	lf.lfOutPrecision = OUT_TT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = ANTIALIASED_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
	wcsncpy(lf.lfFaceName, agi::charset::ConvertW(style->font).c_str(), 32);

	HFONT thefont = CreateFontIndirect(&lf);
	if (!thefont) return lib->error("Failed to create DC");
	SelectObject(thedc, thefont);

	std::wstring wtext(agi::charset::ConvertW(std::string(str.data, str.len)));
	SIZE sz;
	if (spacing != 0 ) {
		ret.width = 0;
		for (auto c : wtext) {
			GetTextExtentPoint32(thedc, &c, 1, &sz);
			ret.width += sz.cx + spacing;
			ret.height = sz.cy;
		}
	}
	else {
		GetTextExtentPoint32(thedc, &wtext[0], (int)wtext.size(), &sz);
		ret.width = sz.cx;
		ret.height = sz.cy;
	}

	TEXTMETRIC tm;
	GetTextMetrics(thedc, &tm);
	ret.descent = tm.tmDescent;
	ret.extlead= tm.tmExternalLeading;

	DeleteObject(thedc);
	DeleteObject(thefont);

#else // not WIN32
	wxMemoryDC thedc;

	// fix fontsize to be 72 DPI
	//fontsize = -FT_MulDiv((int)(fontsize+0.5), 72, thedc.GetPPI().y);

	// now try to get a font!
	// use the font list to get some caching... (chance is the script will need the same font very often)
	// USING wxTheFontList SEEMS TO CAUSE BAD LEAKS!
	//wxFont *thefont = wxTheFontList->FindOrCreateFont(
	wxFont thefont(
		(int)fontsize,
		wxFONTFAMILY_DEFAULT,
		style->italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
		style->bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
		style->underline,
		to_wx(style->font),
		wxFONTENCODING_SYSTEM); // FIXME! make sure to get the right encoding here, make some translation table between windows and wx encodings
	thedc.SetFont(thefont);

	wxString wtext(to_wx(text));
	if (spacing) {
		// If there's inter-character spacing, kerning info must not be used, so calculate width per character
		// NOTE: Is kerning actually done either way?!
		for (auto const& wc : wtext) {
			int a, b, c, d;
			thedc.GetTextExtent(wc, &a, &b, &c, &d);
			double scaling = fontsize / (double)(b > 0 ? b : 1); // semi-workaround for missing OS/2 table data for scaling
			width += (a + spacing)*scaling;
			height = b > height ? b*scaling : height;
			descent = c > descent ? c*scaling : descent;
			extlead = d > extlead ? d*scaling : extlead;
		}
	} else {
		// If the inter-character spacing should be zero, kerning info can (and must) be used, so calculate everything in one go
		wxCoord lwidth, lheight, ldescent, lextlead;
		thedc.GetTextExtent(wtext, &lwidth, &lheight, &ldescent, &lextlead);
		double scaling = fontsize / (double)(lheight > 0 ? lheight : 1); // semi-workaround for missing OS/2 table data for scaling
		width = lwidth*scaling; height = lheight*scaling; descent = ldescent*scaling; extlead = lextlead*scaling;
	}
#endif

	// Compensate for scaling
	ret.width = style->scalex / 100 * ret.width / 64;
	ret.height = style->scaley / 100 * ret.height / 64;
	ret.descent = style->scaley / 100 * ret.descent / 64;
	ret.extlead = style->scaley / 100 * ret.extlead / 64;

	return ret;
}