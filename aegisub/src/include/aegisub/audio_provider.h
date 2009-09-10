// Copyright (c) 2006, Rodrigo Braz Monteiro
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

/// @file audio_provider.h
/// @brief Declaration of base-class for audio providers
/// @ingroup main_headers audio_input
///


#pragma once


///////////
// Headers
#include <wx/string.h>
#include "aegisub.h"

//////////////
// Prototypes
class VideoProvider;



/// @class AudioProvider
/// @brief DOCME
///
/// DOCME
class AudioProvider {
private:

	/// DOCME
	char *raw;

	/// DOCME
	int raw_len;

protected:

	/// DOCME
	int channels;

	/// DOCME
	int64_t num_samples; // for one channel, ie. number of PCM frames

	/// DOCME
	int sample_rate;

	/// DOCME
	int bytes_per_sample;


	/// DOCME
	wxString filename;

public:
	AudioProvider();
	virtual ~AudioProvider();

	virtual wxString GetFilename();
	virtual void GetAudio(void *buf, int64_t start, int64_t count)=0;
	void GetAudioWithVolume(void *buf, int64_t start, int64_t count, double volume);

	int64_t GetNumSamples();
	int GetSampleRate();
	int GetBytesPerSample();
	int GetChannels();
	virtual bool AreSamplesNativeEndian() = 0;

	void GetWaveForm(int *min,int *peak,int64_t start,int w,int h,int samples,float scale);
};



/// @class AudioProviderFactory
/// @brief DOCME
///
/// DOCME
class AudioProviderFactory {
public:

	/// @brief DOCME
	///
	virtual ~AudioProviderFactory() {}
	virtual AudioProvider *CreateProvider(wxString filename)=0;
};
