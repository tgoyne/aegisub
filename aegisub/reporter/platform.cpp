// Copyright (c) 2009, Amar Takhar <verm@aegisub.org>
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
// $Id$

/// @file platform.cpp
/// @brief Base functions for the Platform class.
/// @ingroup base

#ifndef R_PRECOMP
#include <wx/gdicmn.h>	// Display* functions.
#include <wx/version.h>	// Version info.
#include <wx/intl.h>	// Locale info.
#endif

#include "include/platform.h"
#include "platform_unix.h"
#include "platform_unix_bsd.h"
#include "platform_unix_linux.h"
#include "platform_unix_osx.h"

/// @brief Constructor.
Platform* Platform::GetPlatform() {

#ifdef __UNIX__
#   if defined(__FREEBSD__)
		Platform *p = new PlatformUnixBSD;
#   elif defined(__LINUX__)
		Platform *p = new PlatformUnixLinux;
#   elif defined(__APPLE__)
		Platform *p = new PlatformUnixOSX;
#   else
		Platform *p = new PlatformUnix;
#   endif
#endif // __UNIX__
	p->Init();
	return p;
}

/// @brief Init variables to avoid duplicate instantiations.
void Platform::Init() {
	locale = new wxLocale();
	locale->Init();
}

wxString Platform::ArchName() {
	return plat.GetArchName();
};

wxString Platform::OSFamily() {
	return plat.GetOperatingSystemFamilyName();
};

wxString Platform::OSName() {
	return plat.GetOperatingSystemIdName();
};

wxString Platform::Endian() {
	return plat.GetEndiannessName();
};

wxString Platform::DisplayColour() {
	return wxString::Format(L"%d", wxColourDisplay());
}

wxString Platform::DisplayDepth() {
	return wxString::Format(L"%d", wxDisplayDepth());
}

wxString Platform::DisplaySize() {
	int x, y;
	wxDisplaySize(&x, &y);
	return wxString::Format(L"%d %d", x, y);
}

wxString Platform::DisplayPPI() {
	return wxString::Format(L"%d %d", wxGetDisplayPPI().GetWidth(), wxGetDisplayPPI().GetHeight());
}

wxString Platform::wxVersion() {
	return wxString::Format(L"%d.%d.%d.%d", wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER, wxSUBRELEASE_NUMBER);
}

wxString Platform::Locale() {
	return wxLocale().GetSysName();
}

wxString Platform::Language() {
	const wxLanguageInfo *info = locale->GetLanguageInfo(locale->GetLanguage());
	return info->CanonicalName;
}

wxString Platform::SystemLanguage() {
	const wxLanguageInfo *info = locale->GetLanguageInfo(locale->GetSystemLanguage());
	return info->CanonicalName;
}

wxString Platform::Date() {
	return "";
}

wxString Platform::Signature() {
	return "";
}

wxString Platform::DesktopEnvironment() {
	return "";
}

#ifdef __APPLE__

wxString Platform::PatchLevel() {
	return "";
}

wxString Platform::QuickTimeExt() {
	return "";
}

wxString Platform::HardwareModel() {
	return "";
}

#endif
