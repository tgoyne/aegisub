// Copyright (c) 2007, Niels Martin Hansen, Rodrigo Braz Monteiro
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

/// @file font_file_lister_freetype.cpp
/// @brief FreeType based font collector
/// @ingroup font_collector
///

#include "config.h"

#ifdef WITH_FREETYPE2

#ifndef AGI_PRE
#include <wx/dir.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_SFNT_NAMES_H
#include <freetype/ttnameid.h>

template<typename T, typename U>
std::pair<T&, U&> tie(T& t, U& u) {
	return std::make_pair<T&, U&>(t, u);
}

#ifdef WIN32
#include <shlobj.h>

struct dirent {
	const wchar_t *d_name;
};

struct DIR {
	HANDLE h;
	dirent d;
	WIN32_FIND_DATA ffd;
	bool valid;
};

DIR *opendir(std::wstring dirname) {
	DIR *d = new DIR;
	dirname += L"/*";

	d->h = FindFirstFile(dirname.c_str(), &d->ffd);
	d->valid = true;
	return d;
}

dirent *readdir(DIR *dirp) {
	if (!dirp->valid) return NULL;

	dirp->d.d_name = dirp->ffd.cFileName;
	if (!FindNextFile(dirp->h, &dirp->ffd))
		dirp->valid = false;
	return &dirp->d;
}

int closedir(DIR *dirp) {
	FindClose(dirp->h);
	delete dirp;
	return 0;
}
#endif

#include "font_file_lister_freetype.h"

struct FontCacheKey {
	std::string name;
	int32_t weight;
	int32_t slant;
};
struct FontCacheValue {
	std::string name;
	int32_t time;
};

#define CACHE_VERSION 1

class FontIndex {
	int32_t ReadInt();
	std::string ReadStr();
	bool ReadCache(std::string folder);

	void WriteInt(int32_t value);
	void WriteStr(std::string const& value);
	void WriteCache();

	void IndexFonts();
	void Dongs();

	std::istream stream;
	std::map<std::string, int32_t> indexedFonts;
	std::map<FontCacheKey, FontCacheValue> cache;
};

int32_t FontIndex::ReadInt() {
	if (!stream.good()) throw "";
	int32_t ret;
	stream.read(&ret, 4);
	if (!stream.good()) throw "";
	return ret;
}

std::string FontIndex::ReadStr() {
	int32_t len = ReadInt(stream);
	std::string ret;
	ret.resize(len);
	stream.read(&ret[0], len);
	if (!stream.good()) throw "";
	return ret;
}

bool FontIndex::ReadCache(std::string folder) {
	if (ReadInt() != 0xDEADBEEF || ReadInt() == CACHE_VERSION) return true;
	bool anyModified = agi::io::Stat(folder) != ReadInt(stream);

	while (stream.good()) {
		std::string file = ReadStr();
		int32_t modifiedTime = ReadInt();
		bool isValid = !anyModified;
		if (isValid) {
			std::map<std::string, int32_t>::const_iterator old = indexedFonts.find(file);
			if (old != indexedFonts.end()) {
				isValid = modifiedTime == agi::io::Stat(file);
			}
		}

		FontCacheKey k;
		v.name = file;
		v.time = modifiedTime;

		FontCacheValue v;
		k.name = ReadStr();
		k.weight = ReadInt();
		k.slant = ReadInt();

		if (isValid) {
			cache[k] = v;
			indexedFonts[file] = modifiedTime;
		}
	}

	return anyModified;
}

void FontIndex::WriteInt(int32_t value) {
	std::ofstream stream;
	stream.write(&value, sizeof value);
}
void FontIndex::WriteStr(std::string const& value) {
	std::ofstream stream;
	stream.write(&value.size(), sizeof(int32_t));
	stream.write(value.data(), value.size());
}
void FontIndex::WriteCache() {
	std::ofstream stream;
	WriteInt(0xDEADBEEF);
	WriteInt(CACHE_VERSION);
	WriteInt(folderModifiedTime);

	for (std::map<FontCacheKey, FontCacheValue>::const_iterator cur = cache.begin(); cur != cache.end(); ++cur) {
		WriteStr(cur->first.name);
		WriteInt(cur->first.time);
		WriteStr(cur->second.name);
		WriteInt(cur->second.weight);
		WriteInt(cur->second.slant);
	}
}

void FontIndex::AddFont(FontCacheKey &k, FontCacheValue const& v, FT_String *family, FT_String *style) {
	k.name = family;
	if (style) {
		k.name += " ";
		k.name += style;
	}
	cache[k] = v;
}

struct FtEncoding {
	const FT_UShort platform_id;
	const FT_UShort encoding_id;
	const char      fromcode[12];
};

static const FtEncoding ftEncoding[] = {
	{TT_PLATFORM_APPLE_UNICODE, 999,                  "UTF-16BE"},
	{TT_PLATFORM_MACINTOSH,     TT_MAC_ID_JAPANESE,   "SJIS"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_SYMBOL_CS,   "UTF-16BE"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_UNICODE_CS,  "UTF-16BE"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_SJIS,        "SJIS-WIN"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_GB2312,      "GB2312"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_BIG_5,       "BIG-5"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_WANSUNG,     "Wansung"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_JOHAB,       "Johab"},
	{TT_PLATFORM_MICROSOFT,     TT_MS_ID_UCS_4,       "UTF-16BE"}, // not a typo
	{TT_PLATFORM_ISO,           TT_ISO_ID_7BIT_ASCII, "ASCII"},
	{TT_PLATFORM_ISO,           TT_ISO_ID_10646,      "UTF-16BE"},
	{TT_PLATFORM_ISO,           TT_ISO_ID_8859_1,     "ISO-8859-1"}
};

#define NUM_FT_ENCODING (sizeof(fcFtEncoding) / sizeof(fcFtEncoding[0]))

std::pair<std::string, int> GetName(FT_SfntName const& name) {
	int language = name->platform_id << 16 + name->language_id;
	std::string sourceEnc;

	for (int i = 0; i < NUM_FT_ENCODING; ++i) {
		if (ftEncoding[i].platform_id == name->platform_id && (ftEncoding[i].encoding_id == name->encoding_id || ftEncoding[i] == 999)) {
			agi::charset::IconvWrapper conv(ftEncoding[i].fromcode, "utf-8");
			return std::make_pair(conv.Convert(std::string(name->string, name->string_len)), language);
		}
	}
	return std::make_pair("", language);
}

void FontIndex::AddAllNames(FontCacheKey &k, FontCacheValue const& v, FT_Face *face) {
	std::map<int, std::string> families;
	std::map<int, std::string> subfamilies;
	
	int count = FT_Get_Sfnt_Name_Count(face);
	for (int i = 0; i < count; ++i) {
		FT_SfntName name;
		FT_Get_Sfnt_Name(face, i, &name);
		int language;
		std::string str;
		switch (name->name_id) {
			case TT_NAME_ID_FONT_FAMILY:
				tie(str, language) = GetName(name);
				families[language].push_back(str);
				break;
			case TT_NAME_ID_FONT_SUBFAMILY:
				tie(str, language) = GetName(name);
				subfamilies[language].push_back(str);
				break;
			case TT_NAME_ID_FULL_NAME:
				tie(k.name, language) = GetName(name);
				cache[k] = v;
				break;
			default: break;
		}
	}

	for (std::map<int, std::string>::iterator cur = families.begin(); cur != families.end(); ++cur) {
		k.name = cur->second;
		std::string subFamily = subfamilies[cur->first];

		if (subFamily.empty() || subFamily == "Regular") {
			cache[k] = v;
		}
		if (!subFamily.empty()) {
			k.name += " ";
			k.name += subFamily;
			cache[k] = v;
		}
	}
}

void FontIndex::IndexFonts() {
#ifdef WIN32
	wchar_t path[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_FONTS, NULL, 0, path);
#define UTF8(str) agi::charset::ConvertW(str);
#else
	// XXXHACK: Is this always a correct assumption?
	// Fonts might be instaled in more places, I think...
	char path[] = "/Library/Fonts/";
#define UTF8(str) str
#endif

	DIR *dir = opendir(path);
	while (dirent *file = readdir(dir)) {
		std::string filename = agi::charset::ConvertW(file->d_name);
		if (indexedFonts.find(filename) != indexedFonts.end()) continue;

		const wchar_t shortname[MAX_PATH];
		GetShortPathNameW(file->d_name, shortname, MAX_PATH);
		std::string name = agi::charset::ConvertW(shortname);

		FontCacheValue v;
		v.name = filename;
		v.time = agi::io::Stat(filename);

		for (int facenum=0; ; ++facenum) {
			// Get font face
			FT_Face face;
			FT_Error fterr = FT_New_Face(ft2lib, name.c_str(), facenum, &face);
			if (fterr) break;

			FontCacheKey k;
			k.slant  = (face->style_flags & FT_STYLE_FLAG_ITALIC) == FT_STYLE_FLAG_ITALIC;
			k.weight = (face->style_flags & FT_STYLE_FLAG_BOLD) == FT_STYLE_FLAG_BOLD;

			if (FT_Get_Sfnt_Name_Count(face)) {
				AddAllNames(k, v, face);
			}
			else {
				AddFont(k, v, face->family_name, face->style_name);
			}
			FT_Done_Face(face);
			indexedFonts[filename] = v.time;
		}
	}
}

void FontIndex::Dongs() {

}

FreetypeFontFileLister::FreetypeFontFileLister(std::tr1::function<void (wxString, int)> cb)
: FontFileLister(cb) {
	FT_Init_FreeType(&ft2lib);
	if (ReadCache("")) {
		IndexFonts();
		WriteCache();
	}
}

FreetypeFontFileLister::~FreetypeFontFileLister() {
}

#endif WITH_FREETYPE2
