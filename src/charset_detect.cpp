// Copyright (c) 2010, Amar Takhar
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

/// @file charset_detect.cpp
/// @brief Wrapper around text encoding detection library
/// @ingroup utility
///

#include "charset_detect.h"

#include "compat.h"

#include <libaegisub/charset.h>
#include <libaegisub/charset_conv.h>
#include <libaegisub/log.h>

#include <boost/filesystem/path.hpp>

#include <wx/arrstr.h>
#include <wx/choicdlg.h>
#include <wx/intl.h>

namespace CharSetDetect {

std::string GetEncoding(agi::fs::path const& filename) {
	auto list = agi::charset::DetectAll(filename);

	if (list.size() == 1) {
		auto charset = list.begin();
		LOG_I("charset/file") << filename << " (" << charset->second << ")";
		return charset->second;
	}

	wxArrayString choices;
	std::string log_choice;

	for (auto const& charset : list) {
		choices.push_back(to_wx(charset.second));
		log_choice.append(" " + charset.second);
	}

	LOG_I("charset/file") << filename << " (" << log_choice << ")";

	if (choices.empty())
		choices = agi::charset::GetEncodingsList<wxArrayString>();

	int choice = wxGetSingleChoiceIndex(
		_("Aegisub could not narrow down the character set to a single one.\nPlease pick one below:"),
		_("Choose character set"),
		choices);
	if (choice == -1) throw agi::UserCancelException("Cancelled encoding selection");
	if (list.empty())
		return agi::charset::GetEncodingsList<std::vector<std::string>>()[choice];
	return list[choice].second;
}

}
