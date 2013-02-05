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

/// @file video_provider_dummy.cpp
/// @brief Video provider returning a constant frame
/// @ingroup video_input
///

#include "config.h"

#include "video_provider_dummy.h"

#include "colorspace.h"

#include <libaegisub/color.h>
#include <libaegisub/fs.h>
#include <libaegisub/util.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

void DummyVideoProvider::Create(double fps, int frames, int width, int height, unsigned char red, unsigned char green, unsigned char blue, bool pattern) {
	this->framecount = frames;
	this->fps = fps;
	this->width = width;
	this->height = height;
	this->frame = AegiVideoFrame(width, height);

	unsigned char *dst = frame.data;
	unsigned char colors[2][4] = {
		{ blue, green, red, 0 },
		{ 0, 0, 0, 0 }
	};

	if (pattern) {
		// Generate light version
		unsigned char h, s, l;
		rgb_to_hsl(red, blue, green, &h, &s, &l);
		l += 24;
		if (l < 24) l -= 48;
		hsl_to_rgb(h, s, l, &colors[1][2], &colors[1][1], &colors[1][0]);

		// Divide into a 8x8 grid and use light colours when row % 2 != col % 2
		int ppitch = frame.pitch / frame.GetBpp();
		for (unsigned int y = 0; y < frame.h; ++y) {
			for (int x = 0; x < ppitch; ++x) {
				memcpy(dst, colors[((y / 8) & 1) != ((x / 8) & 1)], 4);
				dst += 4;
			}
		}
	}
	else {
		for (int i = frame.pitch * frame.h / frame.GetBpp() - 1; i >= 0; --i)
			memcpy(dst + i * 4, colors[0], 4);
	}
}

DummyVideoProvider::DummyVideoProvider(agi::fs::path const& filename) {
	if (!boost::starts_with(filename.string(), "?dummy"))
		throw agi::fs::FileNotFound(std::string("Attempted creating dummy video provider with non-dummy filename"));

	std::vector<std::string> toks;
	auto const& fields = filename.string().substr(7);
	boost::split(toks, fields, boost::is_any_of(":"));
	if (toks.size() != 8)
		throw VideoOpenError("Too few fields in dummy video parameter list");

	size_t i = 0;
	double fps;
	int frames, width, height, red, green, blue;

	using agi::util::try_parse;
	if (!try_parse(toks[i++], &fps))    throw VideoOpenError("Unable to parse fps field in dummy video parameter list");
	if (!try_parse(toks[i++], &frames)) throw VideoOpenError("Unable to parse framecount field in dummy video parameter list");
	if (!try_parse(toks[i++], &width))  throw VideoOpenError("Unable to parse width field in dummy video parameter list");
	if (!try_parse(toks[i++], &height)) throw VideoOpenError("Unable to parse height field in dummy video parameter list");
	if (!try_parse(toks[i++], &red))    throw VideoOpenError("Unable to parse red colour field in dummy video parameter list");
	if (!try_parse(toks[i++], &green))  throw VideoOpenError("Unable to parse green colour field in dummy video parameter list");
	if (!try_parse(toks[i++], &blue))   throw VideoOpenError("Unable to parse blue colour field in dummy video parameter list");

	bool pattern = toks[i] == "c";

	Create(fps, frames, width, height, red, green, blue, pattern);
}

DummyVideoProvider::DummyVideoProvider(double fps, int frames, int width, int height, agi::Color colour, bool pattern) {
	Create(fps, frames, width, height, colour.r, colour.g, colour.b, pattern);
}

DummyVideoProvider::~DummyVideoProvider() {
	frame.Clear();
}

std::string DummyVideoProvider::MakeFilename(double fps, int frames, int width, int height, agi::Color colour, bool pattern) {
	return str(boost::format("?dummy:%f:%d:%d:%d:%d:%d:%d:%s") % fps % frames % width % height % (int)colour.r % (int)colour.g % (int)colour.b % (pattern ? "c" : ""));
}
