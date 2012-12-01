// Copyright (c) 2012, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file text_file_writer.cpp
/// @brief Write plain text files line by line
/// @ingroup utility
///

#include "config.h"

#include "text_file_writer.h"

#include <libaegisub/io.h>

#include "charset_conv.h"
#include "main.h"

#include <boost/algorithm/string/predicate.hpp>

TextFileWriter::TextFileWriter(std::string const& filename, std::string encoding)
: file(new agi::io::Save(filename, true))
#ifdef _WIN32
, linebreak("\r\n")
#else
, linebreak("\n")
#endif
{
	if (encoding.empty()) encoding = OPT_GET("App/Save Charset")->GetString();
	if (!boost::iequals(encoding, "utf-8")) {
		conv.reset(new agi::charset::IconvWrapper("utf-8", encoding.c_str(), true));
		linebreak = conv->Convert(linebreak);
	}

	try {
		// Write the BOM
		WriteLineToFile("\xEF\xBB\xBF", false);
	}
	catch (agi::charset::ConversionFailure&) {
		// If the BOM could not be converted to the target encoding it isn't needed
	}
}

TextFileWriter::~TextFileWriter() {
	// Explicit empty destructor required with an auto_ptr to an incomplete class
}

void TextFileWriter::WriteLineToFile(std::string const& line, bool addLineBreak) {
	file->Get() << conv ? conv->Convert(line) : line;
	if (addLineBreak)
		file->Get() << linebreak;
}
