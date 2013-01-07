// Copyright (c) 2005-2006, Rodrigo Braz Monteiro, Fredrik Mellbin
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

/// @file audio_provider_dummy.cpp
/// @brief Dummy (silence or noise) audio provider
/// @ingroup audio_input
///

#include "config.h"

#include "audio_provider_dummy.h"
#include "utils.h"

#include <boost/algorithm/string/predicate.hpp>

/*
 * scheme            ::= "dummy-audio" ":" signal-specifier "?" signal-parameters
 * signal-specifier  ::= "silence" | "noise" | "sine" "/" frequency
 * frequency         ::= integer
 * signal-parameters ::= signal-parameter [ "&" signal-parameters ]
 * signal-parameter  ::= signal-parameter-name "=" integer
 * signal-parameter-name ::= "sr" | "bd" | "ch" | "ln"
 *
 * Signal types:
 * "silence", a silent signal is generated.
 * "noise", a white noise signal is generated.
 * "sine", a sine wave is generated at the specified frequency.
 *
 * Signal parameters:
 * "sr", sample rate to generate signal at.
 * "bd", bit depth to generate signal at (usually 16).
 * "ch", number of channels to generate, usually 1 or 2. The same signal is generated
 *       in every channel even if one would be LFE.
 * "ln", length of signal in samples. ln/sr gives signal length in seconds.
 */
DummyAudioProvider::DummyAudioProvider(std::string const& uri) {
	if (!boost::starts_with(uri, "dummy-audio:"))
		throw agi::FileNotFoundError("Not a dummy audio URI");

	noise = boost::contains(uri, ":noise?");
	channels = 1;
	sample_rate = 44100;
	bytes_per_sample = 2;
	float_samples = false;
	num_samples = (int64_t)5*30*60*1000 * sample_rate / 1000;
}

void DummyAudioProvider::FillBuffer(void *buf, int64_t, int64_t count) const {
	if (noise) {
		short *workbuf = (short*)buf;
		while (count-- > 0)
			*workbuf++ = (rand() - RAND_MAX/2) * 10000 / RAND_MAX;
	}
	else {
		memset(buf, 0, count * bytes_per_sample);
	}
}
