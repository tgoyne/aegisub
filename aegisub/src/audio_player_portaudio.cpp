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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//


#include "config.h"

#ifdef WITH_PORTAUDIO


///////////
// Headers
#include "audio_player_portaudio.h"
#include "audio_provider_manager.h"
#include "options.h"
#include "utils.h"

// Uncomment to enable debug features.
//#define PORTAUDIO_DEBUG

/////////////////////
// Reference counter
int PortAudioPlayer::pa_refcount = 0;


///////////////
// Constructor
PortAudioPlayer::PortAudioPlayer() {
	// Initialize portaudio
	if (!pa_refcount) {
		PaError err = Pa_Initialize();

		if (err != paNoError) {
			static wchar_t errormsg[2048];
			swprintf(errormsg, 2048, L"Failed opening PortAudio: %s", Pa_GetErrorText(err));
			throw (const wchar_t *)errormsg;
		}

		pa_refcount++;
	}

	// Variables
	playing = false;
	stopping = false;
	volume = 1.0f;
	paStart = 0.0;
}


//////////////
// Destructor
PortAudioPlayer::~PortAudioPlayer() {
	// Deinit portaudio
	if (!--pa_refcount) Pa_Terminate();
}


///////////////
// Open stream
void PortAudioPlayer::OpenStream() {
	// Open stream
	PaStreamParameters pa_output_p;

	int pa_config_default = Options.AsInt(_T("Audio PortAudio Device"));
	PaDeviceIndex pa_device;

	if (pa_config_default < 0) {
		pa_device = Pa_GetDefaultOutputDevice();
		wxLogDebug(_T("PortAudioPlayer::OpenStream Using Default Output Device: %d"), pa_device);
	} else {
		pa_device = pa_config_default;
		wxLogDebug(_T("PortAudioPlayer::OpenStream Using Config Device: %d"), pa_device);
	}

	pa_output_p.device = pa_device;
	pa_output_p.channelCount = provider->GetChannels();
	pa_output_p.sampleFormat = paInt16;
	pa_output_p.suggestedLatency = Pa_GetDeviceInfo(pa_device)->defaultLowOutputLatency;
	pa_output_p.hostApiSpecificStreamInfo = NULL;

	wxLogDebug(_T("PortAudioPlayer::OpenStream Output channels: %d, Latency: %f  Sample Rate: %ld\n"),
	pa_output_p.channelCount, pa_output_p.suggestedLatency, pa_output_p.sampleFormat);

	PaError err = Pa_OpenStream(&stream, NULL, &pa_output_p, provider->GetSampleRate(), 256, paPrimeOutputBuffersUsingStreamCallback, paCallback, this);

	if (err != paNoError) {

		const PaHostErrorInfo *pa_err = Pa_GetLastHostErrorInfo();
		if (pa_err->errorCode != 0) {
			wxLogDebug(_T("PortAudioPlayer::OpenStream HostError: API: %d, %s (%ld)\n"), pa_err->hostApiType, pa_err->errorText, pa_err->errorCode);
		}
		throw wxString(_T("Failed initializing PortAudio stream with error: ") + wxString(Pa_GetErrorText(err),wxConvLocal));
	}
}


///////////////
// Close stream
void PortAudioPlayer::CloseStream() {
	try {
		Stop(false);
		Pa_CloseStream(stream);
	} catch (...) {}
}

// Called when the callback has finished.
void PortAudioPlayer::paStreamFinishedCallback(void *userData) {
    PortAudioPlayer *player = (PortAudioPlayer *) userData;

	player->playing = false;

	if (player->displayTimer) {
		player->displayTimer->Stop();
	}

	wxLogDebug(_T("PortAudioPlayer::paStreamFinishedCallback Stopping stream."));
}


////////
// Play
void PortAudioPlayer::Play(int64_t start,int64_t count) {
	PaError err;

	// Stop if it's already playing
	wxMutexLocker locker(PAMutex);

	// Set values
	endPos = start + count;
	playPos = start;
	startPos = start;

	// Start playing
	if (!playing) {

		err = Pa_SetStreamFinishedCallback(stream, paStreamFinishedCallback);

		if (err != paNoError) {
			wxLogDebug(_T("PortAudioPlayer::Play Could not set FinishedCallback\n"));
			return;
		}

		err = Pa_StartStream(stream);

		if (err != paNoError) {
			return;
		}
	}
	playing = true;
	paStart = Pa_GetStreamTime(stream);

	// Update timer
	if (displayTimer && !displayTimer->IsRunning()) displayTimer->Start(15);
}


////////
// Stop
void PortAudioPlayer::Stop(bool timerToo) {
	//wxMutexLocker locker(PAMutex);
	//softStop = false;

	// Stop stream
	playing = false;
	Pa_StopStream (stream);

	// Stop timer
	if (timerToo && displayTimer) {
		displayTimer->Stop();
	}
}


//////////////////////
/// PortAudio callback
int PortAudioPlayer::paCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {

	// Get provider
	PortAudioPlayer *player = (PortAudioPlayer *) userData;
	AudioProvider *provider = player->GetProvider();

#ifdef PORTAUDIO_DEBUG
	printf("paCallBack: playPos: %lld  startPos: %lld  paStart: %f Pa_GetStreamTime: %f  AdcTime: %f  DacTime: %f  framesPerBuffer: %lu  CPU: %f\n", 
	player->playPos, player->startPos, player->paStart, Pa_GetStreamTime(player->stream), 
	timeInfo->inputBufferAdcTime, timeInfo->outputBufferDacTime,  framesPerBuffer, Pa_GetStreamCpuLoad(player->stream));
#endif

	// Calculate how much left
	int64_t lenAvailable = (player->endPos - player->playPos) > 0 ? framesPerBuffer : 0;


	// Play something
	if (lenAvailable > 0) {
		provider->GetAudioWithVolume(outputBuffer, player->playPos, lenAvailable, player->GetVolume());

		// Set play position
		player->playPos += framesPerBuffer;

		// Continue as normal
		return 0;
	}

	// Abort stream and stop the callback.
	return paAbort;
}


////////////////////////
/// Get current stream position.
int64_t PortAudioPlayer::GetCurrentPosition()
{

	if (!playing) return 0;

	const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream);

#ifdef PORTAUDIO_DEBUG
	PaTime pa_getstream = Pa_GetStreamTime(stream);
	int64_t real = ((pa_getstream - paStart) * streamInfo->sampleRate) + startPos;
	printf("GetCurrentPosition: Pa_GetStreamTime: %f  startPos: %lld  playPos: %lld  paStart: %f  real: %lld  diff: %f\n",
	pa_getstream, startPos, playPos, paStart, real, pa_getstream-paStart);

	return real;
#else
	return ((Pa_GetStreamTime(stream) - paStart) * streamInfo->sampleRate) + startPos;
#endif

}


///////////////
/// Return a list of available output devices.
/// @param Setting from config file.
wxArrayString PortAudioPlayer::GetOutputDevices(wxString favorite) {
	wxArrayString list;
	int devices = Pa_GetDeviceCount();
	int i;

	if (devices < 0) {
		// some error here
	}

	for (i=0; i<devices; i++) {
		const PaDeviceInfo *dev_info = Pa_GetDeviceInfo(i);
		wxString name(dev_info->name, wxConvUTF8);
		list.Insert(name, i);
	}

	return list;
}

#endif // WITH_PORTAUDIO
