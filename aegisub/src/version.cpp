// Copyright (c) 2005, Niels Martin Hansen
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

/// @file version.cpp
/// @brief Derive and return various information about the build and version at runtime
/// @ingroup main
///

#include "config.h"

#include <wx/string.h>
#include <wx/datetime.h>
#include "version.h"

#ifdef __WINDOWS__
#include "../build/svn-revision.h"
#else

#ifndef BUILD_SVN_REVISION
#define BUILD_SVN_REVISION 0
#endif

#endif

#define _T_rec(X) _T(X)
#define _T_stringize(X) _T(#X)
#define _T_int(X) _T_stringize(X)

#define BUILD_TIMESTAMP _T_rec(__DATE__) _T(" ") _T_rec(__TIME__)

// Define FINAL_RELEASE to mark a build as a "final" version, ie. not pre-release version
// In that case it won't include the SVN revision information

struct VersionInfoStruct {
	// Some raw data
	wxChar *VersionNumber;
	bool IsDebug;
	bool IsRelease;
	int SvnRev;
	wxChar *BuildTime;
	wxChar *BuildCredit;

	// Nice strings for display
	wxString LongVersionString;
	wxString ShortVersionString;

	// Generate the above data
	VersionInfoStruct() {
		wxString VersionStr;

		SvnRev = BUILD_SVN_REVISION;

#ifdef BUILD_SVN_DATE
		BuildTime = _T_rec(BUILD_SVN_DATE);
#else
		BuildTime = BUILD_TIMESTAMP;
#endif

#ifdef BUILD_CREDIT
		BuildCredit = _T(BUILD_CREDIT);
#else
		BuildCredit = _T("");
#endif

#ifdef _DEBUG
		IsDebug = true;
#else
		IsDebug = false;
#endif

#ifdef FINAL_RELEASE
		IsRelease = true;
		VersionNumber = _T("2.2.0");
#else
		IsRelease = false;
		VersionNumber = _T("r") _T_int(BUILD_SVN_REVISION)
# ifdef BUILD_SVN_LOCALMODS
			_T("M")
# endif
			;
#endif

		if (IsRelease)
		{
			// Short is used in about box
			ShortVersionString = wxString::Format(_T("%s (built from SVN revision %d)"), VersionNumber, SvnRev);
			// Long is used in title bar
			LongVersionString = VersionNumber;
		}
		else
		{
			wxString buildcred;
#ifdef BUILD_CREDIT
			buildcred += _T(", "); buildcred += BuildCredit;
#endif
			ShortVersionString = wxString::Format(_T("%s (development version%s)"), VersionNumber, buildcred.c_str());
			LongVersionString = ShortVersionString;
		}

		if (IsDebug)
		{
			ShortVersionString += _T(" [DEBUG VERSION]");
			LongVersionString += _T(" [DEBUG VERSION]");
		}
	}
};


VersionInfoStruct versioninfo;


wxString GetAegisubLongVersionString() {
	return versioninfo.LongVersionString;
}

wxString GetAegisubShortVersionString() {
	return versioninfo.ShortVersionString;
}

wxString GetAegisubBuildTime() {
	return versioninfo.BuildTime;
}

wxString GetAegisubBuildCredit() {
	return versioninfo.BuildCredit;
}

bool GetIsOfficialRelease() {
	return versioninfo.IsRelease;
}

wxString GetVersionNumber() {
	return versioninfo.VersionNumber;
}

int GetSVNRevision() {
	return versioninfo.SvnRev;
}

