// Copyright (c) 2007, David Lamparter, Rodrigo Braz Monteiro
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
//
// $Id$

/// @file font_file_lister_fontconfig.cpp
/// @brief Font Config-based font collector
/// @ingroup font_collector
///

#include "config.h"

#ifdef WITH_FONTCONFIG
#include "font_file_lister_fontconfig.h"

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

FontConfigFontFileLister::FontConfigFontFileLister(std::tr1::function<void (wxString, int)> cb)
: FontFileLister(cb)
, config(FcInitLoadConfig(), FcConfigDestroy)
{
	statusCallback(_("Updating font cache\n"), 0);
	FcConfigBuildFonts(config);
}


FontConfigFontFileLister::~FontConfigFontFileLister() {
}

/* The following code is based heavily on ass_fontconfig.c from libass
 *
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 */

FcFontSet *FontConfigFontFileLister::MatchFullname(const char *family, int weight, int slant) {
	FcFontSet *sets[2];
	FcFontSet *result = FcFontSetCreate();
	int nsets = 0;

	if ((sets[nsets] = FcConfigGetFonts(config, FcSetSystem)))
		nsets++;
	if ((sets[nsets] = FcConfigGetFonts(config, FcSetApplication)))
		nsets++;

	// Run over font sets and patterns and try to match against full name
	for (int i = 0; i < nsets; i++) {
		FcFontSet *set = sets[i];
		for (int fi = 0; fi < set->nfont; fi++) {
			FcPattern *pat = set->fonts[fi];
			char *fullname;
			int pi = 0, at;
			FcBool ol;
			while (FcPatternGetString(pat, FC_FULLNAME, pi++, (FcChar8 **)&fullname) == FcResultMatch) {
				if (FcPatternGetBool(pat, FC_OUTLINE, 0, &ol) != FcResultMatch || ol != FcTrue)
					continue;
				if (FcPatternGetInteger(pat, FC_SLANT, 0, &at) != FcResultMatch || at < slant)
					continue;
				if (FcPatternGetInteger(pat, FC_WEIGHT, 0, &at) != FcResultMatch || at < weight)
					continue;
				if (strcasecmp(fullname, family) == 0) {
					FcFontSetAdd(result, FcPatternDuplicate(pat));
					break;
				}
			}
		}
	}

	return result;
}

wxString FontConfigFontFileLister::GetFontPath(wxString const& facename, int bold, bool italic) {
	wxCharBuffer family_buffer = facename.utf8_str();
	const char *family = family_buffer.data();
	if (family[0] == '@') ++family;

	int weight = bold == 0 ? 80 :
	             bold == 1 ? 200 :
	                         bold;
	int slant  = italic ? 110 : 0;

	scoped<FcPattern*> pat(FcPatternCreate(), FcPatternDestroy);
	if (!pat) return "";

	FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)family);

	// In SSA/ASS fonts are sometimes referenced by their "full name",
	// which is usually a concatenation of family name and font
	// style (ex. Ottawa Bold). Full name is available from
	// FontConfig pattern element FC_FULLNAME, but it is never
	// used for font matching.
	// Therefore, I'm removing words from the end of the name one
	// by one, and adding shortened names to the pattern. It seems
	// that the first value (full name in this case) has
	// precedence in matching.
	// An alternative approach could be to reimplement FcFontSort
	// using FC_FULLNAME instead of FC_FAMILY.
	int family_cnt = 1;
	{
		char *s = _strdup(family);
		char *p = s + strlen(s);
		while (--p > s)
			if (*p == ' ' || *p == '-') {
				*p = '\0';
				FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *) s);
				++family_cnt;
			}
		free(s);
	}

	FcPatternAddBool(pat, FC_OUTLINE, true);
	FcPatternAddInteger(pat, FC_SLANT, slant);
	FcPatternAddInteger(pat, FC_WEIGHT, weight);

	FcDefaultSubstitute(pat);

	if (!FcConfigSubstitute(config, pat, FcMatchPattern)) return "";

	FcResult result;
	scoped<FcFontSet*> fsorted(FcFontSort(config, pat, true, NULL, &result), FcFontSetDestroy);
	scoped<FcFontSet*> ffullname(MatchFullname(family, weight, slant), FcFontSetDestroy);
	if (!fsorted || !ffullname) return "";

	scoped<FcFontSet*> fset(FcFontSetCreate(), FcFontSetDestroy);
	for (int curf = 0; curf < ffullname->nfont; ++curf) {
		FcPattern *curp = ffullname->fonts[curf];
		FcPatternReference(curp);
		FcFontSetAdd(fset, curp);
	}
	for (int curf = 0; curf < fsorted->nfont; ++curf) {
		FcPattern *curp = fsorted->fonts[curf];
		FcPatternReference(curp);
		FcFontSetAdd(fset, curp);
	}

	int curf;
	for (curf = 0; curf < fset->nfont; ++curf) {
		FcBool outline;
		FcResult result = FcPatternGetBool(fset->fonts[curf], FC_OUTLINE, 0, &outline);
		if (result == FcResultMatch && outline) break;
	}

	if (curf >= fset->nfont) return "";

	// Remove all extra family names from original pattern.
	// After this, FcFontRenderPrepare will select the most relevant family
	// name in case there are more than one of them.
	for (; family_cnt > 1; --family_cnt)
		FcPatternRemove(pat, FC_FAMILY, family_cnt - 1);

	scoped<FcPattern*> rpat(FcFontRenderPrepare(config, pat, fset->fonts[curf]), FcPatternDestroy);
	if (!rpat) return "";

	FcChar8 *r_family;
	result = FcPatternGetString(rpat, FC_FAMILY, 0, &r_family);
	if (result != FcResultMatch)
		return "";

	FcChar8 *r_fullname;
	result = FcPatternGetString(rpat, FC_FULLNAME, 0, &r_fullname);
	if (result != FcResultMatch)
		return "";

	if (strcasecmp((const char *)r_family, family) && strcasecmp((const char *)r_fullname, family))
		return "";

	FcChar8 *file;
	result = FcPatternGetString(rpat, FC_FILE, 0, &file);
	if(result != FcResultMatch) return "";

	return (const char *)file;
}
#endif
