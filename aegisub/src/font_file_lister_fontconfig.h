// Copyright (c) 2007, Rodrigo Braz Monteiro
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

/// @file font_file_lister_fontconfig.h
/// @see font_file_lister_fontconfig.cpp
/// @ingroup font_collector
///

#include <fontconfig/fontconfig.h>
#include <fontconfig/fcfreetype.h>

#include "font_file_lister.h"


/// DOCME
/// @class FontConfigFontFileLister
/// @brief fontconfig powered font lister
class FontConfigFontFileLister : public FontFileLister {
	template<typename T> class scoped {
		T data;
		std::tr1::function<void (T)> destructor;
	public:
		scoped(T data, std::tr1::function<void (T)> d) : data(data), destructor(d) { }
		~scoped() { if (data) destructor(data); }
		operator T() { return data; }
		T operator->() { return data; }
	};

	scoped<FcConfig*> config;

	/// @brief Case-insensitive match ASS/SSA font family against full name. (also known as "name for humans")
	/// @param family font fullname
	/// @param bold weight attribute
	/// @param italic italic attribute
	/// @return font set
	FcFontSet *MatchFullname(const char *family, int weight, int slant);

	wxString GetFontPath(wxString const& facename, int bold, bool italic);
public:
	FontConfigFontFileLister(std::tr1::function<void (wxString, int)> cb);
	~FontConfigFontFileLister();
};
