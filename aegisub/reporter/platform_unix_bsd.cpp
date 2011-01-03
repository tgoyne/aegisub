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

/// @file platform_unix_bsd.cpp
/// @brief BSD Platform extensions.
/// @ingroup unix

#ifndef R_PRECOMP
#include <wx/string.h>
#endif

extern "C" {
#include <sys/types.h>
#include <sys/sysctl.h>
}

#include "include/platform.h"
#include "platform_unix.h"
#include "platform_unix_bsd.h"


std::string PlatformUnixBSD::CPUId() {
	char id[300];
	size_t len = sizeof(id);
	sysctlbyname("hw.model", &id, &len, NULL, 0);
	return id;
};

std::string PlatformUnixBSD::CPUSpeed() {
	return "";
};

int PlatformUnixBSD::CPUCores() {
	return 0;
};

int PlatformUnixBSD::CPUCount() {
	int proc;
	size_t len = sizeof(proc);
	sysctlbyname("hw.ncpu", &proc, &len, NULL, 0);
	return proc;
};

std::string PlatformUnixBSD::CPUFeatures() {
	return "";
};

std::string PlatformUnixBSD::CPUFeatures2() {
	return "";
};

uint64_t PlatformUnixBSD::Memory() {
	uint64_t memory;
	size_t len = sizeof(memory);
	sysctlbyname("hw.physmem", &memory, &len, NULL, 0);
	return memory;
};

std::string PlatformUnixBSD::UnixLibraries() {
	return "";
};
