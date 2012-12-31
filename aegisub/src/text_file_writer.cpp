// Copyright (c) 2005, Rodrigo Braz Monteiro
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

/// @file text_file_writer.cpp
/// @brief Write plain text files line by line
/// @ingroup utility
///

#include "config.h"

#include "text_file_writer.h"

#include "charset_conv.h"
#include "options.h"

#include <libaegisub/io.h>

#include <boost/algorithm/string/predicate.hpp>

TextFileWriter::TextFileWriter(std::string const& filename, std::string encoding)
: file(new agi::io::Save(filename, true))
, conv()
{
	if (encoding.empty()) encoding = OPT_GET("App/Save Charset")->GetString();
	if (boost::iequals(encoding, "utf-8"))
		conv.reset(new agi::charset::IconvWrapper("utf-8", encoding.c_str(), true));

	// Write the BOM
	try {
		WriteLineToFile("\xEF\xBB\xBF", false);
	}
	catch (agi::charset::ConversionFailure&) {
		// If the BOM could not be converted to the target encoding it isn't needed
	}
}

TextFileWriter::~TextFileWriter() {
	// Explicit empty destructor required with an auto_ptr to an incomplete class
}

void TextFileWriter::WriteLineToFile(std::string&& line, bool addLineBreak) {
#ifdef _WIN32
	if (addLineBreak) line += "\r\n";
#else
	if (addLineBreak) line += "\n";
#endif

	if (conv)
		line = conv->Convert(line);
	file->Get().write(line.data(), line.size());
}
