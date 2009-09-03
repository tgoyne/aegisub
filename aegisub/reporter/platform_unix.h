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

/// @@file platform_unix.h
/// @brief @see platform_unix.cpp

class Platform;

/// @brief General Unix functions.
class PlatformUnix : public Platform {
public:
	PlatformUnix() {};
	virtual ~PlatformUnix() {};
	wxString OSVersion();
	wxString DesktopEnvironment();

	// Hardware
	virtual wxString CPUId();
	virtual wxString CPUSpeed();
	virtual wxString CPUCores();
	virtual wxString CPUCount();
	virtual wxString CPUFeatures();
	virtual wxString CPUFeatures2();
	virtual wxString Memory();
	virtual wxString Video();

	// Unix Specific
	virtual wxString UnixLibraries();
};
