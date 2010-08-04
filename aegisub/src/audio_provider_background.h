// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
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

/// @file audio_provider_background.h
/// @brief Audio cache with background loading
/// @ingroup audio_input
///

#ifndef AGI_PRE
#include <tr1/memory>
#include <vector>

#include <wx/thread.h>
#endif

#include "include/aegisub/audio_provider.h"
#include <libaegisub/mmap.h>

class BackgroundAudioDecoder;

/// @class BGAudioProvider
/// @brief Audio cache with background loading
class BGAudioProvider : public AudioProvider {
	wxMutex position_mutex;         ///< Mutex for the position variable
	wxCondition position_condition; ///< Condition for the position mutex
	int64_t position;               ///< Last available byte (not sample) + 1
	agi::mmc::MMapCache data;       ///< The decoded audio, filled by the background thread

	bool samples_native_endian;

	/// The thread which does the actual decoding
	BackgroundAudioDecoder *decoder;
public:
	BGAudioProvider(AudioProvider *source);
	~BGAudioProvider();

	bool AreSamplesNativeEndian() const { return samples_native_endian; }
	void GetAudio(void *buf, int64_t start, int64_t count);
};
