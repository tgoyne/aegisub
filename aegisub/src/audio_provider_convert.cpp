// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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
//
// $Id$

/// @file audio_provider_convert.cpp
/// @brief Intermediate sample format-converting audio provider
/// @ingroup audio_input
///

#include "config.h"

#include "audio_provider_convert.h"

#include "aegisub_endian.h"
#include "include/aegisub/audio_provider.h"

#include <libaegisub/scoped_ptr.h>

/// Base class for all wrapping converters
class AudioProviderConverter : public AudioProvider {
protected:
	agi::scoped_ptr<AudioProvider> source;
public:
	AudioProviderConverter(AudioProvider *src) : source(src) {
		channels = source->GetChannels();
		num_samples = source->GetNumSamples();
		sample_rate = source->GetSampleRate();
		bytes_per_sample = source->GetBytesPerSample();
	}

	bool AreSamplesNativeEndian() const { return true; }
	wxString GetFilename() const { return source->GetFilename(); }
};

/// Endian swap converter for 16-bit audio
class EndianSwapAudioProvider : public AudioProviderConverter {
public:
	EndianSwapAudioProvider(AudioProvider *src) : AudioProviderConverter(src) {
		if (src->GetBytesPerSample() != 2)
			throw agi::InternalError("EndianSwapAudioProvider only supports 16-bit audio", 0);
		if (src->AreSamplesNativeEndian())
			throw agi::InternalError("EndianSwapAudioProvider used on provider that doesn't need it", 0);
	}

	void GetAudio(void *buf, int64_t start, int64_t count) const {
		source->GetAudio(buf, start, count);
		count *= channels;
		int16_t *buf16 = reinterpret_cast<int16_t*>(buf);
		for (int64_t i = 0; i < count; ++i)
			buf16[i] = Endian::Reverse((uint16_t)buf16[i]);
	}
};

/// unsigned 8 bit -> signed machine-endian 16 bit audio converter
class UpconvertAudioProvider : public AudioProviderConverter {
public:
	UpconvertAudioProvider(AudioProvider *src) : AudioProviderConverter(src) {
		if (src->GetBytesPerSample() != 1)
			throw agi::InternalError("UpconvertAudioProvider only supports 8-bit input", 0);
		bytes_per_sample = 2;
	}

	void GetAudio(void *buf, int64_t start, int64_t count) const {
		source->GetAudio(buf, start, count);

		count *= channels;
		int8_t *buf8 = reinterpret_cast<int8_t*>(buf);
		// walking backwards so that the conversion can be done in place
		for (int64_t i = count - 1; i >= 0; --i) {
			buf8[i * 2 + 1] = 0;
			buf8[i * 2] = buf8[i];
		}
	}
};

/// Non-mono 16-bit signed machine-endian -> mono 16-bit signed machine endian converter
class DownmixAudioProvider : public AudioProviderConverter {
	mutable std::vector<int16_t> src_buf;
	int src_channels;
public:
	DownmixAudioProvider(AudioProvider *src) : AudioProviderConverter(src) {
		if (bytes_per_sample != 2)
			throw agi::InternalError("DownmixAudioProvider requires 16-bit input", 0);
		if (channels == 1)
			throw agi::InternalError("DownmixAudioProvider requires multi-channel input", 0);
		src_channels = channels;
		channels = 1;
	}

	void GetAudio(void *buf, int64_t start, int64_t count) const {
		if (count == 0) return;

		src_buf.resize(count * src_channels);
		source->GetAudio(&src_buf[0], start, count);

		int16_t *dst = reinterpret_cast<int16_t*>(buf);
		// Just average the channels together
		while (count-- > 0) {
			int sum = 0;
			for (int c = 0; c < src_channels; ++c)
				sum += src_buf[count * src_channels + c];
			dst[count] = static_cast<int16_t>(sum / src_channels);
		}
	}
};

/// Sample doubler with linear interpolation for the new samples
/// Requires 16-bit mono input
class SampleDoublingAudioProvider : public AudioProviderConverter {
public:
	SampleDoublingAudioProvider(AudioProvider *src) : AudioProviderConverter(src) {
		if (src->GetBytesPerSample() != 2)
			throw agi::InternalError("UpsampleAudioProvider requires 16-bit input", 0);
		if (src->GetChannels() != 1)
			throw agi::InternalError("UpsampleAudioProvider requires mono input", 0);

		sample_rate *= 2;
		num_samples *= 2;
	}

	void GetAudio(void *buf, int64_t start, int64_t count) const {
		if (count == 0) return;

		bool not_end = start + count < num_samples;
		int64_t src_count = count / 2;
		source->GetAudio(buf, start / 2, src_count + not_end);

		int16_t *buf16 = reinterpret_cast<int16_t*>(buf);

		if (!not_end) {
			// We weren't able to request a sample past the end so just
			// duplicate the last sample
			buf16[src_count] = buf16[src_count + 1];
		}

		if (count % 2)
			buf16[count - 1] = buf16[src_count];

		// walking backwards so that the conversion can be done in place
		for (int64_t i = src_count - 1; i >= 0; --i) {
			buf16[i * 2] = buf16[i];
			buf16[i * 2 + 1] = (int16_t)(((int32_t)buf16[i] + buf16[i + 1]) / 2);
		}
	}
};

AudioProvider *CreateConvertAudioProvider(AudioProvider *source_provider) {
	AudioProvider *provider = source_provider;

	// Ensure 16-bit audio with proper endianness
	switch (provider->GetBytesPerSample()) {
		case 1:
			provider = new UpconvertAudioProvider(provider);
			break;
		case 2:
			if (!provider->AreSamplesNativeEndian())
				provider = new EndianSwapAudioProvider(provider);
			break;
		default:
			throw AudioOpenError("Audio format converter: audio with bitdepths greater than 16 bits/sample is currently unsupported");
	}

	// We currently only support mono audio
	if (provider->GetChannels() != 1)
		provider = new DownmixAudioProvider(provider);

	// Some players don't like low sample rate audio
	while (provider->GetSampleRate() < 32000)
		provider = new SampleDoublingAudioProvider(provider);

	return provider;
}
