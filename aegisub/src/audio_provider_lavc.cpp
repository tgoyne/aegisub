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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


///////////
// Headers
#include "config.h"

#ifdef WITH_FFMPEG

#ifdef WIN32
#define EMULATE_INTTYPES
#endif
#include <wx/wxprec.h>

/* avcodec.h uses INT64_C in a *single* place. This prolly breaks on Win32,
 * but, well. Let's at least fix it for Linux.
 */
/* Update: this used to be commented out but is now needed on Windows. 
 * Not sure about Linux, so it's wrapped in an ifdef. 
 */
#ifdef WIN32
#define __STDC_CONSTANT_MACROS 1
#include <stdint.h>
#endif /* WIN32 */
/* - done in posix/defines.h
 */

#include "audio_provider_lavc.h"
#include "mkv_wrap.h"
#include "lavc_file.h"
#include "lavc_file.h"
#include "utils.h"
#include "options.h"


///////////////
// Constructor
LAVCAudioProvider::LAVCAudioProvider(Aegisub::String _filename)
	: lavcfile(NULL), codecContext(NULL), rsct(NULL), buffer(NULL)
{
	try {
#if 0
	/* since seeking currently is likely to be horribly broken with two
	 * providers accessing the same stream, this is disabled for now.
	 */
	LAVCVideoProvider *vpro_lavc = dynamic_cast<LAVCVideoProvider *>(vpro);
	if (vpro_lavc) {
		lavcfile = vpro->lavcfile->AddRef();
		filename = vpro_lavc->GetFilename();
	} else {
#endif
		lavcfile = LAVCFile::Create(_filename);
		filename = _filename.c_str();
#if 0
	}
#endif
	audStream = -1;
	for (int i = 0; i < (int)lavcfile->fctx->nb_streams; i++) {
		codecContext = lavcfile->fctx->streams[i]->codec;
		if (codecContext->codec_type == CODEC_TYPE_AUDIO) {
			stream = lavcfile->fctx->streams[i];
			audStream = i;
			break;
		}
	}
	if (audStream == -1) {
		codecContext = NULL;
		throw _T("ffmpeg audio provider: Could not find an audio stream");
	}
	AVCodec *codec = avcodec_find_decoder(codecContext->codec_id);
	if (!codec) {
		codecContext = NULL;
		throw _T("ffmpeg audio provider: Could not find a suitable audio decoder");
	}
	if (avcodec_open(codecContext, codec) < 0)
		throw _T("ffmpeg audio provider: Failed to open audio decoder");

	sample_rate = Options.AsInt(_T("Audio Sample Rate"));
	if (!sample_rate) {
		/* aegisub wants audio with sample rate higher than 32khz */
		if (codecContext->sample_rate < 32000)
			sample_rate = 48000;
		else
			sample_rate = codecContext->sample_rate;
	}

	/* we rely on the intermediate audio provider to do downmixing for us later if necessary */
	channels = codecContext->channels;

	/* TODO: test if anything but S16 actually works! */
	switch (codecContext->sample_fmt) {
		case SAMPLE_FMT_U8:		bytes_per_sample = 1; break;
		case SAMPLE_FMT_S16:	bytes_per_sample = 2; break;
		case SAMPLE_FMT_S32:	bytes_per_sample = 4; break; /* downmixing provider doesn't support this, will definitely not work */
		default: 
			throw _T("ffmpeg audio provider: Unknown or unsupported sample format");
	}

	/* initiate resampling if necessary */
	if (sample_rate != codecContext->sample_rate) {
		rsct = audio_resample_init(channels, channels, sample_rate, codecContext->sample_rate);
		if (!rsct)
			throw _T("ffmpeg audio provider: Failed to initialize resampling");
		resample_ratio = (float)sample_rate / (float)codecContext->sample_rate;
	}
	
	/* libavcodec seems to give back invalid stream length values for Matroska files.
	 * As a workaround, we can use the overall file length.
	 */
	double length;
	if(stream->duration == AV_NOPTS_VALUE)
		length = (double)lavcfile->fctx->duration / AV_TIME_BASE;
	else
		length = (double)stream->duration * av_q2d(stream->time_base);
	num_samples = (int64_t)(length * sample_rate); /* number of samples per channel */

	buffer = (int16_t *)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
	if (!buffer)
		throw _T("ffmpeg audio provider: Failed to allocate audio decoding buffer, out of memory?");

	leftover_samples	= 0;
	last_output_sample	= -1;

	} catch (...) {
		Destroy();
		throw;
	}
}


LAVCAudioProvider::~LAVCAudioProvider()
{
	Destroy();
}

void LAVCAudioProvider::Destroy()
{
	if (buffer)
		free(buffer);
	if (rsct)
		audio_resample_close(rsct);
	if (codecContext)
		avcodec_close(codecContext);
	if (lavcfile)
		lavcfile->Release();
}

void LAVCAudioProvider::GetAudio(void *buf, int64_t start, int64_t count)
{
	int16_t *_buf = (int16_t *)buf;

	/* this exception disabled for now */
	/* if (last_output_sample != start-1)
		 throw _T("ffmpeg audio provider: nonlinear access attempted, try loading audio to RAM or HD cache"); */

	last_output_sample += count;

	int64_t samples_to_decode = (num_samples - start) * channels; /* samples left to the end of the stream */
	if (count < samples_to_decode) /* haven't reached the end yet, so just decode the requested number of samples */
		samples_to_decode = count * channels; /* times the number of channels */
	if (samples_to_decode < 0) /* requested beyond the end of the stream */
		samples_to_decode = 0;

	/* if we got asked for more samples than there are left in the stream, add zeros to the decoding buffer until
	we have enough to fill the request */
	memset(_buf + samples_to_decode, 0, ((count * channels) - samples_to_decode) * bytes_per_sample);

	/* do we have leftover samples from last time we were called? */
	/* FIXME: this assumes that requests are always linear! attempts at random access give bogus results! */
	if (leftover_samples > 0) {
		int length = (samples_to_decode > leftover_samples) ? leftover_samples : samples_to_decode;
		samples_to_decode -= length;
		leftover_samples -= length;
		
		/* put them in the output buffer */
		samples_to_decode -= leftover_samples;
		while (length > 0) {
			*(_buf++) = *(overshoot_buffer++);
			length--;
		}
	}

	AVPacket packet;
	while (samples_to_decode > 0 && av_read_frame(lavcfile->fctx, &packet) >= 0) {
		/* we're not dealing with video packets in this here provider */
		if (packet.stream_index == audStream) {
			int size = packet.size;
			uint8_t *data = packet.data;

			while (size > 0) {
				int temp_output_buffer_size = AVCODEC_MAX_AUDIO_FRAME_SIZE; /* see constructor, it malloc()'s buffer to this */
				int retval, decoded_bytes, decoded_samples;
			
				retval = avcodec_decode_audio2(codecContext, buffer, &temp_output_buffer_size, data, size);
				/* decoding failed, skip this packet and hope next one doesn't fail too */
				if (retval < 0)
					break;
					/* throw _T("ffmpeg audio provider: failed to decode audio"); */

				size -= retval;
				data += retval;
				/* decoding succeeded but this audio frame is empty, continue to next frame */
				if (temp_output_buffer_size <= 0)
					continue;

				decoded_bytes	= temp_output_buffer_size;
				decoded_samples = decoded_bytes / bytes_per_sample; /* FIXME: stop assuming everything is 16-bit! */

				/* do we need to resample? */
				if (rsct) {
					/* allocate some memory to save the resampled data in */
					int16_t *temp_output_buffer = (int16_t *)malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
					if (!temp_output_buffer)
						throw _T("ffmpeg audio provider: Failed to allocate audio resampling buffer, out of memory?");

					/* do the actual resampling */
					decoded_samples = audio_resample(rsct, temp_output_buffer, buffer, decoded_samples / codecContext->channels);

					/* did we end up with more samples than we were asked for? */
					if (decoded_samples > samples_to_decode) {
						/* in that case, count them */
						leftover_samples = decoded_samples - samples_to_decode;
						/* and put them aside for later */
						memcpy(buffer, &temp_output_buffer[samples_to_decode+1], leftover_samples * bytes_per_sample);
						overshoot_buffer = buffer;
						/* output the other samples that didn't overflow */
						memcpy(_buf, temp_output_buffer, samples_to_decode * bytes_per_sample);
						_buf += samples_to_decode;
					} else {
						memcpy(_buf, temp_output_buffer, decoded_samples * bytes_per_sample);

						_buf += decoded_samples;
					}
					
					free(temp_output_buffer);
				} else {	/* no resampling needed */
					/* overflow? (as above) */
					if (decoded_samples > samples_to_decode) {
						/* count sheep^H^H^H^H^Hsamples */
						leftover_samples = decoded_samples - samples_to_decode;
						/* and put them aside for later (mm, lamb chops) */
						overshoot_buffer = &buffer[samples_to_decode+1];
						/* output the other samples that didn't overflow */
						memcpy(_buf, buffer, samples_to_decode * bytes_per_sample);

						_buf += samples_to_decode;
					} else {
						/* just do a straight copy to buffer */
						memcpy(_buf, buffer, decoded_bytes);

						_buf += decoded_samples;
					}
				}

				samples_to_decode	-= decoded_samples;
			}
		}

		av_free_packet(&packet);
	}
	
}

#endif
