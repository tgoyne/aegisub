// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

#if WITH_LIBLZMA
#include <array>
#include <cstdint>
#include <sstream>

#include <lzma.h>

namespace agi { namespace lzma {

const int streamend			= LZMA_STREAM_END;
const int unsupported_check	= LZMA_UNSUPPORTED_CHECK;
const int mem_error			= LZMA_MEM_ERROR;
const int options_error		= LZMA_OPTIONS_ERROR;
const int data_error		   = LZMA_DATA_ERROR;
const int buf_error			= LZMA_BUF_ERROR;
const int prog_error		   = LZMA_PROG_ERROR;

inline void check(lzma_ret value) {
	if (value != LZMA_OK && value != LZMA_STREAM_END)
		throw value;
}

template<typename Stream>
std::unique_ptr<std::stringstream> decompress(Stream&& istream) {
	std::array<char, 65536> inbuf, outbuf;

	std::unique_ptr<std::stringstream> ret{new std::stringstream};

	lzma_stream stream = LZMA_STREAM_INIT;
	lzma_ret result = lzma_stream_decoder(&stream, UINT64_MAX, 0);
	check(result);

	while (istream.good()) {
		istream.read(&inbuf[0], inbuf.size());
		stream.avail_in = istream.gcount();
		stream.next_in = reinterpret_cast<uint8_t *>(&inbuf[0]);

		do {
			stream.next_out = reinterpret_cast<uint8_t *>(&outbuf[0]);
			stream.avail_out = outbuf.size();

			lzma_ret result = lzma_code(&stream, LZMA_RUN);
			check(result);

			ret->write(&outbuf[0], outbuf.size() - stream.avail_out);
		} while (stream.avail_in);
	}

	lzma_end(&stream);
	return ret;
}

} }

#endif
