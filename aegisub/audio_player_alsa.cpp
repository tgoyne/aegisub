// Copyright (c) 2007, Niels Martin Hansen
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
// Contact: mailto:jiifurusu@gmail.com
//


///////////
// Headers
#include <wx/wxprec.h>
#include "audio_player.h"
#include "audio_provider.h"
#include "utils.h"
#include "main.h"
#include "frame_main.h"
#include "audio_player.h"
#include <alsa/asoundlib.h>
#include "options.h"


///////////////
// Alsa player
class AlsaPlayer : public AudioPlayer {
private:
	bool open;
	volatile bool playing;
	volatile float volume;

	volatile unsigned long start_frame; // first frame of playback
	volatile unsigned long cur_frame; // last written frame + 1
	volatile unsigned long end_frame; // last frame to play
	unsigned long bpf; // bytes per frame

	AudioProvider *provider;
	snd_pcm_t *pcm_handle; // device handle
	snd_pcm_stream_t stream; // stream direction
	snd_async_handler_t *pcm_callback;

	snd_pcm_format_t sample_format;
	unsigned int rate; // sample rate of audio
	unsigned int real_rate; // actual sample rate played back
	unsigned int period_len; // length of period in microseconds
	unsigned int buflen; // length of buffer in microseconds
	snd_pcm_uframes_t period; // size of period in frames
	snd_pcm_uframes_t bufsize; // size of buffer in frames

	void SetUpHardware();
	void SetUpAsync();

	static void async_write_handler(snd_async_handler_t *pcm_callback);

public:
	AlsaPlayer();
	~AlsaPlayer();

	void OpenStream();
	void CloseStream();

	void Play(__int64 start,__int64 count);
	void Stop(bool timerToo=true);
	bool IsPlaying();

	__int64 GetStartPosition();
	__int64 GetEndPosition();
	__int64 GetCurrentPosition();
	void SetEndPosition(__int64 pos);
	void SetCurrentPosition(__int64 pos);

	void SetVolume(double vol) { volume = vol; }
	double GetVolume() { return volume; }
};



///////////
// Factory
class AlsaPlayerFactory : public AudioPlayerFactory {
public:
	AudioPlayer *CreatePlayer() { return new AlsaPlayer(); }
	AlsaPlayerFactory() : AudioPlayerFactory(_T("alsa")) {}
} registerAlsaPlayer;



///////////////
// Constructor
AlsaPlayer::AlsaPlayer()
{
	volume = 1.0f;
	open = false;
	playing = false;
	start_frame = cur_frame = end_frame = bpf = 0;
	provider = 0;
}


//////////////
// Destructor
AlsaPlayer::~AlsaPlayer()
{
	CloseStream();
}


///////////////
// Open stream
void AlsaPlayer::OpenStream()
{
	CloseStream();

	// Get provider
	provider = GetProvider();
	bpf = provider->GetChannels() * provider->GetBytesPerSample();

	// We want playback
	stream = SND_PCM_STREAM_PLAYBACK;
	// And get a device name
	wxString device = Options.AsText(_T("Audio Alsa Device"));

	// Open device for blocking access
	if (snd_pcm_open(&pcm_handle, device.mb_str(wxConvUTF8), stream, 0) < 0) { // supposedly we don't want SND_PCM_ASYNC even for async playback
		throw _T("Error opening specified PCM device");
	}

	SetUpHardware();

	// Register async handler
	SetUpAsync();

	// Now ready
	open = true;
}


void AlsaPlayer::SetUpHardware()
{
	int dir;

	// Allocate params structure
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_hw_params_malloc(&hwparams);

	// Get hardware params
	if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
		throw _T("Error setting up default PCM device");
	}

	// Set stream format
	if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		throw _T("Could not set interleaved stream format");
	}

	// Set sample format
	switch (provider->GetBytesPerSample()) {
		case 1:
			sample_format = SND_PCM_FORMAT_S8;
			break;
		case 2:
			sample_format = SND_PCM_FORMAT_S16_LE;
			break;
		default:
			throw _T("Can only handle 8 and 16 bit sound");
	}
	if (snd_pcm_hw_params_set_format(pcm_handle, hwparams, sample_format) < 0) {
		throw _T("Could not set sample format");
	}

	// Ask for resampling
	if (snd_pcm_hw_params_set_rate_resample(pcm_handle, hwparams, 1) < 0) {
		throw _T("Couldn't enable resampling");
	}

	// Set sample rate
	rate = provider->GetSampleRate();
	real_rate = rate;
	if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &real_rate, 0) < 0) {
		throw _T("Could not set sample rate");
	}
	if (rate != real_rate) {
		wxLogDebug(_T("Could not set ideal sample rate %d, using %d instead"), rate, real_rate);
	}

	// Set number of channels
	if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, provider->GetChannels()) < 0) {
		throw _T("Could not set number of channels");
	}
	printf("Set sample rate %u (wanted %u)\n", real_rate, rate);

	// Set buffer size
	unsigned int wanted_buflen = 1000000; // microseconds
	buflen = wanted_buflen;
	if (snd_pcm_hw_params_set_buffer_time_near(pcm_handle, hwparams, &buflen, &dir) < 0) {
		throw _T("Couldn't set buffer length");
	}
	if (buflen != wanted_buflen) {
		wxLogDebug(_T("Couldn't get wanted buffer size of %u, got %u instead"), wanted_buflen, buflen);
	}
	if (snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize) < 0) {
		throw _T("Couldn't get buffer size");
	}
	printf("Buffer size: %lu\n", bufsize);

	// Set period (number of frames ideally written at a time)
	// Somewhat arbitrary for now
	unsigned int wanted_period = bufsize / 4;
	period_len = wanted_period; // microseconds
	if (snd_pcm_hw_params_set_period_time_near(pcm_handle, hwparams, &period_len, &dir) < 0) {
		throw _T("Couldn't set period length");
	}
	if (period_len != wanted_period) {
		wxLogDebug(_T("Couldn't get wanted period size of %d, got %d instead"), wanted_period, period_len);
	}
	if (snd_pcm_hw_params_get_period_size(hwparams, &period, &dir) < 0) {
		throw _T("Couldn't get period size");
	}
	printf("Period size: %lu\n", period);

	// Apply parameters
	if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
		throw _T("Failed applying sound hardware settings");
	}

	// And free memory again
	snd_pcm_hw_params_free(hwparams);
}


void AlsaPlayer::SetUpAsync()
{
	// Prepare software params struct
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_sw_params_malloc (&sw_params);

	// Get current parameters
	if (snd_pcm_sw_params_current(pcm_handle, sw_params) < 0) {
		throw _T("Couldn't get current SW params");
	}

	// How full the buffer must be before playback begins
	if (snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, bufsize - period) < 0) {
		throw _T("Failed setting start threshold");
	}

	// The the largest write guaranteed never to block
	if (snd_pcm_sw_params_set_avail_min(pcm_handle, sw_params, period) < 0) {
		throw _T("Failed setting min available buffer");
	}

	// Apply settings
	if (snd_pcm_sw_params(pcm_handle, sw_params) < 0) {
		throw _T("Failed applying SW params");
	}

	// And free struct again
	snd_pcm_sw_params_free(sw_params);

	// Attach async handler
	if (snd_async_add_pcm_handler(&pcm_callback, pcm_handle, async_write_handler, this) < 0) {
		throw _T("Failed attaching async handler");
	}
}


////////////////
// Close stream
void AlsaPlayer::CloseStream()
{
	if (!open) return;

	Stop();

	// Remove async handler
	snd_async_del_handler(pcm_callback);

	// Close device
	snd_pcm_close(pcm_handle);

	// No longer working
	open = false;
}


////////
// Play
void AlsaPlayer::Play(__int64 start,__int64 count)
{
	if (playing) {
		// Quick reset
		playing = false;
		snd_pcm_drop(pcm_handle);
	}

	// Set params
	start_frame = start;
	cur_frame = start;
	end_frame = start + count;
	playing = true;

	// Prepare a bit
	snd_pcm_prepare (pcm_handle);
	async_write_handler(pcm_callback);

	// And go!
	snd_pcm_start(pcm_handle);

	// Update timer
	if (displayTimer && !displayTimer->IsRunning()) displayTimer->Start(15);
}


////////
// Stop
void AlsaPlayer::Stop(bool timerToo)
{
	if (!open) return;
	if (!playing) return;

	// Reset data
	playing = false;
	start_frame = 0;
	cur_frame = 0;
	end_frame = 0;

	// Then drop the playback
	snd_pcm_drop(pcm_handle);

        if (timerToo && displayTimer) {
                displayTimer->Stop();
        }
}


bool AlsaPlayer::IsPlaying()
{
	return playing;
}


///////////
// Set end
void AlsaPlayer::SetEndPosition(__int64 pos)
{
	end_frame = pos;
}


////////////////////////
// Set current position
void AlsaPlayer::SetCurrentPosition(__int64 pos)
{
	cur_frame = pos;
}


__int64 AlsaPlayer::GetStartPosition()
{
	return start_frame;
}


__int64 AlsaPlayer::GetEndPosition()
{
	return end_frame;
}


////////////////////////
// Get current position
__int64 AlsaPlayer::GetCurrentPosition()
{
	snd_pcm_sframes_t delay = 0;
	snd_pcm_delay(pcm_handle, &delay); // don't bother catching errors here
	return cur_frame - delay;
}


void AlsaPlayer::async_write_handler(snd_async_handler_t *pcm_callback)
{
	// TODO: check for broken pipes in here and restore as needed
	AlsaPlayer *player = (AlsaPlayer*)snd_async_handler_get_callback_private(pcm_callback);

	if (player->cur_frame >= player->end_frame + player->rate) {
		// More than a second past end of stream
		snd_pcm_drain(player->pcm_handle);
		player->playing = false;
		return;
	}

	snd_pcm_sframes_t frames = snd_pcm_avail_update(player->pcm_handle);

	// TODO: handle underrun

	if (player->cur_frame >= player->end_frame) {
		// Past end of stream, add some silence
		void *buf = calloc(frames, player->bpf);
		snd_pcm_writei(player->pcm_handle, buf, frames);
		free(buf);
		player->cur_frame += frames;
		return;
	}

	void *buf = malloc(player->period * player->bpf);
	while (frames >= player->period) {
		unsigned long start = player->cur_frame;
		player->provider->GetAudioWithVolume(buf, player->cur_frame, player->period, player->volume);
		snd_pcm_writei(player->pcm_handle, buf, player->period);
		player->cur_frame += player->period;
		frames = snd_pcm_avail_update(player->pcm_handle);
	}
	free(buf);
}



