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

/// @file audio_provider_background.cpp
/// @brief Audio cache with background loading
/// @ingroup audio_input
///

#include "audio_provider_background.h"

class BackgroundAudioDecoder : public wxThread {
	std::auto_ptr<AudioProvider> provider;
	wxCondition *position_condition;
	wxMutex *position_mutex;
	int64_t *position;
	agi::mmc::MMapCache *data;

	int64_t total_size;
	int64_t page_size;
	ExitCode Entry();
public:
	BackgroundAudioDecoder(
		AudioProvider *provider,
		wxCondition *position_condition,
		wxMutex *position_mutex,
		int64_t *position,
		agi::mmc::MMapCache *data)
	: provider(provider)
	, position_condition(position_condition)
	, position_mutex(position_mutex)
	, position(position)
	, data(data)
	{
		total_size = provider->GetNumSamples() * provider->GetBytesPerSample();
		page_size = data->GetPageSize() << 3;
		Create();
		Run();
	}
};

wxThread::ExitCode BackgroundAudioDecoder::Entry() {
	int64_t bytes_per_sample = provider->GetBytesPerSample();
	try {
		while (!TestDestroy() && *position < total_size) {
			int64_t block_size = std::min(page_size, total_size - *position);
			provider->GetAudio(data->Write(*position, block_size), *position / bytes_per_sample, block_size / bytes_per_sample);

			wxMutexLocker locker(*position_mutex);
			*position += block_size;
			position_condition->Broadcast();
		}
	}
	catch (...) {
		/// @todo If the audio display ever gets some sort of error handling
		///       this needs to start forwarding errors to the GUI thread.
		///       For now there's no real point.
		memset(data->Write(*position, total_size - *position), 0, total_size - *position);
		*position = total_size;
	}

	return EXIT_SUCCESS;
}

BGAudioProvider::BGAudioProvider(AudioProvider *source)
: position(0)
, data(source->GetBytesPerSample() * source->GetNumSamples())
, position_condition(position_mutex)
, decoder(new BackgroundAudioDecoder(source, &position_condition, &position_mutex, &position, &data))
{
	bytes_per_sample = source->GetBytesPerSample();
	num_samples      = source->GetNumSamples();
	channels         = source->GetChannels();
	sample_rate      = source->GetSampleRate();
	filename         = source->GetFilename();
}

BGAudioProvider::~BGAudioProvider() {
	wxMutexLocker locker(position_mutex);
	if (position < num_samples * bytes_per_sample) {
		decoder->Delete();
	}
}

void BGAudioProvider::GetAudio(void *vbuf, int64_t start, int64_t count) {
	char *buf = reinterpret_cast<char*>(vbuf);
	start *= bytes_per_sample;
	count *= bytes_per_sample;

	// Requested beyond the length of audio
	if (start + count > num_samples * bytes_per_sample) {
		int64_t oldcount = count;
		count = num_samples * bytes_per_sample - start;
		if (count < 0) count = 0;
		memset(buf + count, 0, oldcount - count);
	}
	if (count <= 0) return;

	// If the requested chunk isn't ready yet block until it is
	/// @todo Blocking until the data is read almost certainly isn't the best
	///       way to handle this, but the other simple option (simply return
	///       blank audio) doesn't work with the spectrum cache. The audio
	///       display probably needs to be made aware of the concept of audio
	///       loading progress so that it can invalidate the bad cache and
	///       possibly show loading progress in the UI somehow.
	while (start + count > position) {
		wxMutexLocker locker(position_mutex);
		if (start + count > position) {
			position_condition.Wait();
		}
	}

	memcpy(buf, data.Read(start, count), count);
}
