// Copyright (c) 2005-2007, Rodrigo Braz Monteiro
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
// $Id: audio_player_portaudio.cpp 5897 2011-11-20 03:43:52Z plorkyeran $

/// @file audio_player_portaudio.cpp
/// @brief PortAudio v18-based audio output
/// @ingroup audio_output
///


#include "config.h"

#ifdef WITH_PORTAUDIO

#include <libaegisub/log.h>

#include "audio_player_portaudio.h"

#include "compat.h"
#include "include/aegisub/audio_provider.h"
#include "main.h"
#include "utils.h"

// Uncomment to enable extremely spammy debug logging
//#define PORTAUDIO_DEBUG

/// Order that the host APIs should be tried if there are multiple available
static const PaHostApiTypeId pa_host_api_priority[] = {
	paWASAPI,
	paWDMKS,
	paASIO,
	paMME,
	// No DirectSound since we have (two) native DirectSound players

	paCoreAudio,
#ifdef __APPLE__
	paAL,
#endif

	paALSA,
	paOSS
};
static const size_t pa_host_api_priority_count = sizeof(pa_host_api_priority) / sizeof(pa_host_api_priority[0]);

PortAudioPlayer::PortAudioPlayer()
: default_device(paNoDevice)
, volume(1.0f)
, pa_start(0.0)
{
	PaError err = Pa_Initialize();

	if (err != paNoError)
		throw PortAudioError(std::string("Failed opening PortAudio:") + Pa_GetErrorText(err));

	// Build a list of host API-specific devices we can use
	// Some host APIs may not support all devices and some reported devices may
	// not actually work, so loop through each one looking for working output
	// devices which have not already been added by a more highly preferred API
	for (size_t i = 0; i < pa_host_api_priority_count; ++i) {
		PaHostApiIndex host_idx = Pa_HostApiTypeIdToHostApiIndex(pa_host_api_priority[i]);
		if (host_idx > 0)
			GatherDevices(host_idx);
	}
	GatherDevices(Pa_GetDefaultHostApi());

	if (devices.empty() || default_device == paNoDevice)
		throw PortAudioError("No PortAudio output devices found");
}

void PortAudioPlayer::GatherDevices(PaHostApiIndex host_idx) {
	const PaHostApiInfo *host_info = Pa_GetHostApiInfo(host_idx);
	if (!host_info) return;

	for (int host_device_idx = 0; host_device_idx < host_info->deviceCount; ++host_device_idx) {
		PaDeviceIndex real_idx = Pa_HostApiDeviceIndexToDeviceIndex(host_idx, host_device_idx);
		if (real_idx < 0) continue;

		const PaDeviceInfo *device_info = Pa_GetDeviceInfo(real_idx);
		if (!device_info) continue;
		if (device_info->maxOutputChannels <= 0) continue;

		// Some host APIs truncate device names, so check if this is a prefix
		// of an already known device rather than just checking for membership
		std::map<std::string, PaDeviceIndex>::iterator dev_it = devices.lower_bound(device_info->name);
		if (dev_it != devices.end() && dev_it->first.find(device_info->name) == 0) continue;

		PaStreamParameters pa_output_p;
		pa_output_p.device = real_idx;
		pa_output_p.channelCount = 1;
		pa_output_p.sampleFormat = paInt16;
		pa_output_p.suggestedLatency = device_info->defaultLowOutputLatency;
		pa_output_p.hostApiSpecificStreamInfo = NULL;

		PaStream *temp_stream = 0;

		PaError err = Pa_OpenStream(&temp_stream, NULL, &pa_output_p, 44100, 0, paNoFlag, 0, 0);
		if (err == paNoError) {
			Pa_CloseStream(temp_stream);
			devices[device_info->name] = real_idx;
			if (default_device == paNoDevice && real_idx == host_info->defaultOutputDevice)
				default_device = real_idx;
		}
	}
}

PortAudioPlayer::~PortAudioPlayer() {
	Pa_Terminate();
}

void PortAudioPlayer::OpenStream() {
	PaDeviceIndex pa_device = paNoDevice;

	std::string device_name = OPT_GET("Player/Audio/PortAudio/Device Name")->GetString();

	if (devices.count(device_name)) {
		pa_device = devices[device_name];
		LOG_D("audio/player/portaudio") << "using config device: " << device_name << ": " << pa_device;
	}
	else {
		pa_device = default_device;
		LOG_D("audio/player/portaudio") << "using default output device:" << pa_device;
	}

	PaStreamParameters pa_output_p;
	pa_output_p.device = pa_device;
	pa_output_p.channelCount = provider->GetChannels();
	pa_output_p.sampleFormat = paInt16;
	pa_output_p.suggestedLatency = Pa_GetDeviceInfo(pa_device)->defaultLowOutputLatency;
	pa_output_p.hostApiSpecificStreamInfo = NULL;

	LOG_D("audio/player/portaudio") << "OpenStream:"
		<< " output channels: " << pa_output_p.channelCount
		<< " latency: " << pa_output_p.suggestedLatency
		<< " sample rate: " << pa_output_p.sampleFormat;

	PaError err = Pa_OpenStream(&stream, NULL, &pa_output_p, provider->GetSampleRate(), 0, paPrimeOutputBuffersUsingStreamCallback, paCallback, this);

	if (err != paNoError) {
		const PaHostErrorInfo *pa_err = Pa_GetLastHostErrorInfo();
		LOG_D_IF(pa_err->errorCode != 0, "audio/player/portaudio") << "HostError: API: " << pa_err->hostApiType << ", " << pa_err->errorText << ", " << pa_err->errorCode;
		LOG_D("audio/player/portaudio") << "Failed initializing PortAudio stream with error: " << Pa_GetErrorText(err);
		throw PortAudioError("Failed initializing PortAudio stream with error: " + std::string(Pa_GetErrorText(err)));
	}
}

void PortAudioPlayer::CloseStream() {
	Stop(false);
	Pa_CloseStream(stream);
}

void PortAudioPlayer::paStreamFinishedCallback(void *userData) {
	PortAudioPlayer *player = (PortAudioPlayer *) userData;

	if (player->displayTimer)
		player->displayTimer->Stop();
	LOG_D("audio/player/portaudio") << "stopping stream";
}

void PortAudioPlayer::Play(int64_t start_sample, int64_t count) {
	current = start_sample;
	start = start_sample;
	end = start_sample + count;

	// Start playing
	if (!IsPlaying()) {
		PaError err = Pa_SetStreamFinishedCallback(stream, paStreamFinishedCallback);
		if (err != paNoError) {
			LOG_D("audio/player/portaudio") << "could not set FinishedCallback";
			return;
		}

		err = Pa_StartStream(stream);
		if (err != paNoError) {
			LOG_D("audio/player/portaudio") << "error playing stream";
			return;
		}
	}
	pa_start = Pa_GetStreamTime(stream);

	// Update timer
	if (displayTimer && !displayTimer->IsRunning())
		displayTimer->Start(15);
}

void PortAudioPlayer::Stop(bool timerToo) {
	Pa_StopStream(stream);

	if (timerToo && displayTimer)
		displayTimer->Stop();
}

int PortAudioPlayer::paCallback(const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, void *userData)
{
	PortAudioPlayer *player = (PortAudioPlayer *)userData;

#ifdef PORTAUDIO_DEBUG
	LOG_D("audio/player/portaudio") << "psCallback:"
		<< " current: " << player->current
		<< " start: " << player->start
		<< " pa_start: " << player->pa_start
		<< " currentTime: " << timeInfo->currentTime
		<< " AdcTime: " << timeInfo->inputBufferAdcTime
		<< " DacTime: " << timeInfo->outputBufferDacTime
		<< " framesPerBuffer: " << framesPerBuffer
		<< " CPU: " << Pa_GetStreamCpuLoad(player->stream);
#endif

	// Calculate how much left
	int64_t lenAvailable = std::min<int64_t>(player->end - player->current, framesPerBuffer);

	// Play something
	if (lenAvailable > 0) {
		player->GetProvider()->GetAudioWithVolume(outputBuffer, player->current, lenAvailable, player->GetVolume());

		// Set play position
		player->current += lenAvailable;

		// Continue as normal
		return 0;
	}

	// Abort stream and stop the callback.
	return paAbort;
}

int64_t PortAudioPlayer::GetCurrentPosition() {
	if (!IsPlaying()) return 0;

	PaTime pa_time = Pa_GetStreamTime(stream);
	int64_t real = (pa_time - pa_start) * provider->GetSampleRate() + start;

	// If portaudio isn't giving us time info then estimate based on buffer fill and current latency
	if (pa_time == 0 && pa_start == 0)
		real = current - Pa_GetStreamInfo(stream)->outputLatency * provider->GetSampleRate();

#ifdef PORTAUDIO_DEBUG
	LOG_D("audio/player/portaudio") << "GetCurrentPosition:"
		<< " pa_time: " << pa_time
		<< " start: " << start
		<< " current: " << current
		<< " pa_start: " << pa_start
		<< " real: " << real
		<< " diff: " << pa_time - pa_start;
#endif

	return real;
}

wxArrayString PortAudioPlayer::GetOutputDevices() {
	wxArrayString list;
	list.push_back("Default");

	try {
		PortAudioPlayer player;

		for (std::map<std::string, PaDeviceIndex>::iterator it = player.devices.begin(); it != player.devices.end(); ++it)
			list.push_back(lagi_wxString(it->first));
	}
	catch (PortAudioError const&) {
		// No output devices, just return the list with only Default
	}

	return list;
}

bool PortAudioPlayer::IsPlaying() {
	return !!Pa_IsStreamActive(stream);
}

#endif // WITH_PORTAUDIO
