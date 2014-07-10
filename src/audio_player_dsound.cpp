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

/// @file audio_player_dsound.cpp
/// @brief Old DirectSound-based audio output
/// @ingroup audio_output
///

#ifdef WITH_DIRECTSOUND
#include "include/aegisub/audio_player.h"

#include "audio_controller.h"
#include "frame_main.h"
#include "utils.h"

#include <libaegisub/audio/provider.h>
#include <libaegisub/log.h>
#include <libaegisub/make_unique.h>

#include <mmsystem.h>
#include <dsound.h>

namespace {
class DirectSoundPlayer;

class DirectSoundPlayerThread final : public wxThread {
	DirectSoundPlayer *parent;
	HANDLE stopnotify;

public:
	void Stop(); // Notify thread to stop audio playback. Thread safe.
	DirectSoundPlayerThread(DirectSoundPlayer *parent);
	~DirectSoundPlayerThread();

	wxThread::ExitCode Entry();
};

class DirectSoundPlayer final : public AudioPlayer {
	friend class DirectSoundPlayerThread;

	volatile bool playing = false;
	float volume = 1.0f;
	int offset = 0;

	DWORD bufSize = 0;
	volatile int64_t playPos = 0;
	int64_t startPos = 0;
	volatile int64_t endPos = 0;
	DWORD startTime = 0;

	IDirectSound8 *directSound = nullptr;
	IDirectSoundBuffer8 *buffer = nullptr;

	bool FillBuffer(bool fill);
	DirectSoundPlayerThread *thread = nullptr;

public:
	DirectSoundPlayer(agi::AudioProvider *provider, wxWindow *parent);
	~DirectSoundPlayer();

	void Play(int64_t start,int64_t count);
	void Stop();

	bool IsPlaying() { return playing; }

	int64_t GetEndPosition() { return endPos; }
	int64_t GetCurrentPosition();
	void SetEndPosition(int64_t pos);

	void SetVolume(double vol) { volume = vol; }
};

DirectSoundPlayer::DirectSoundPlayer(agi::AudioProvider *provider, wxWindow *parent)
: AudioPlayer(provider)
{
	// Initialize the DirectSound object
	HRESULT res;
	res = DirectSoundCreate8(&DSDEVID_DefaultPlayback,&directSound,nullptr); // TODO: support selecting audio device
	if (FAILED(res)) throw AudioPlayerOpenError("Failed initializing DirectSound");

	// Set DirectSound parameters
	directSound->SetCooperativeLevel((HWND)parent->GetHandle(),DSSCL_PRIORITY);

	// Create the wave format structure
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nSamplesPerSec = provider->GetSampleRate();
	waveFormat.nChannels = provider->GetChannels();
	waveFormat.wBitsPerSample = provider->GetBytesPerSample() * 8;
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = sizeof(waveFormat);

	// Create the buffer initializer
	int aim = waveFormat.nAvgBytesPerSec * 15/100; // 150 ms buffer
	int min = DSBSIZE_MIN;
	int max = DSBSIZE_MAX;
	bufSize = std::min(std::max(min,aim),max);
	DSBUFFERDESC desc;
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = bufSize;
	desc.dwReserved = 0;
	desc.lpwfxFormat = &waveFormat;
	desc.guid3DAlgorithm = GUID_NULL;

	// Create the buffer
	IDirectSoundBuffer *buf;
	res = directSound->CreateSoundBuffer(&desc,&buf,nullptr);
	if (res != DS_OK) throw AudioPlayerOpenError("Failed creating DirectSound buffer");

	// Copy interface to buffer
	res = buf->QueryInterface(IID_IDirectSoundBuffer8,(LPVOID*) &buffer);
	if (res != S_OK) throw AudioPlayerOpenError("Failed casting interface to IDirectSoundBuffer8");

	// Set data
	offset = 0;
}

DirectSoundPlayer::~DirectSoundPlayer() {
	Stop();

	if (buffer)
		buffer->Release();

	if (directSound)
		directSound->Release();
}

bool DirectSoundPlayer::FillBuffer(bool fill) {
	if (playPos >= endPos) return false;

	// Variables
	HRESULT res;
	void *ptr1, *ptr2;
	unsigned long int size1, size2;
	int bytesps = provider->GetBytesPerSample();

	// To write length
	int toWrite = 0;
	if (fill) {
		toWrite = bufSize;
	}
	else {
		DWORD bufplay;
		res = buffer->GetCurrentPosition(&bufplay, nullptr);
		if (FAILED(res)) return false;
		toWrite = (int)bufplay - (int)offset;
		if (toWrite < 0) toWrite += bufSize;
	}
	if (toWrite == 0) return true;

	// Make sure we only get as many samples as are available
	if (playPos + toWrite/bytesps > endPos) {
		toWrite = (endPos - playPos) * bytesps;
	}

	// If we're going to fill the entire buffer (ie. at start of playback) start by zeroing it out
	// If it's not zeroed out we might have a playback selection shorter than the buffer
	// and then everything after the playback selection will be junk, which we don't want played.
	if (fill) {
RetryClear:
		res = buffer->Lock(0, bufSize, &ptr1, &size1, &ptr2, &size2, 0);
		if (res == DSERR_BUFFERLOST) {
			buffer->Restore();
			goto RetryClear;
		}
		memset(ptr1, 0, size1);
		memset(ptr2, 0, size2);
		buffer->Unlock(ptr1, size1, ptr2, size2);
	}

	// Lock buffer
RetryLock:
	if (fill) {
		res = buffer->Lock(offset, toWrite, &ptr1, &size1, &ptr2, &size2, 0);
	}
	else {
		res = buffer->Lock(offset, toWrite, &ptr1, &size1, &ptr2, &size2, 0);//DSBLOCK_FROMWRITECURSOR);
	}

	// Buffer lost?
	if (res == DSERR_BUFFERLOST) {
		LOG_D("audio/player/dsound1") << "lost dsound buffer";
		buffer->Restore();
		goto RetryLock;
	}

	if (FAILED(res)) return false;

	// Convert size to number of samples
	unsigned long int count1 = size1 / bytesps;
	unsigned long int count2 = size2 / bytesps;

	LOG_D_IF(count1, "audio/player/dsound1") << "DS fill: " << (unsigned long)playPos << " -> " << (unsigned long)playPos+count1;
	LOG_D_IF(count2, "audio/player/dsound1") << "DS fill: " << (unsigned long)playPos+count1 << " -> " << (unsigned long)playPos+count1+count2;
	LOG_D_IF(!count1 && !count2, "audio/player/dsound1") << "DS fill: nothing";

	// Get source wave
	if (count1) provider->GetAudioWithVolume(ptr1, playPos, count1, volume);
	if (count2) provider->GetAudioWithVolume(ptr2, playPos+count1, count2, volume);
	playPos += count1+count2;

	buffer->Unlock(ptr1,count1*bytesps,ptr2,count2*bytesps);

	offset = (offset + count1*bytesps + count2*bytesps) % bufSize;

	return playPos < endPos;
}

void DirectSoundPlayer::Play(int64_t start,int64_t count) {
	// Make sure that it's stopped
	Stop();
	// The thread is now guaranteed dead

	HRESULT res;

	// We sure better have a buffer
	assert(buffer);

	// Set variables
	startPos = start;
	endPos = start+count;
	playPos = start;
	offset = 0;

	// Fill whole buffer
	FillBuffer(true);

	DWORD play_flag = 0;
	if (count*provider->GetBytesPerSample() > bufSize) {
		// Start thread
		thread = new DirectSoundPlayerThread(this);
		thread->Create();
		thread->Run();
		play_flag = DSBPLAY_LOOPING;
	}

	// Play
	buffer->SetCurrentPosition(0);
	res = buffer->Play(0,0,play_flag);
	if (SUCCEEDED(res)) playing = true;
	startTime = GetTickCount();
}

void DirectSoundPlayer::Stop() {
	// Stop the thread
	if (thread) {
		if (thread->IsAlive()) {
			thread->Stop();
			thread->Wait();
		}
		thread = nullptr;
	}
	// The thread is now guaranteed dead and there are no concurrency problems to worry about

	if (buffer) buffer->Stop(); // the thread should have done this already

	// Reset variables
	playing = false;
	playPos = 0;
	startPos = 0;
	endPos = 0;
	offset = 0;
}

void DirectSoundPlayer::SetEndPosition(int64_t pos) {
	if (playing) endPos = pos;
}

int64_t DirectSoundPlayer::GetCurrentPosition() {
	// Check if buffer is loaded
	if (!buffer || !playing) return 0;

	// FIXME: this should be based on not duration played but actual sample being heard
	// (during vidoeo playback, cur_frame might get changed to resync)
	DWORD curtime = GetTickCount();
	int64_t tdiff = curtime - startTime;
	return startPos + tdiff * provider->GetSampleRate() / 1000;
}

DirectSoundPlayerThread::DirectSoundPlayerThread(DirectSoundPlayer *par) : wxThread(wxTHREAD_JOINABLE) {
	parent = par;
	stopnotify = CreateEvent(nullptr, true, false, nullptr);
}

DirectSoundPlayerThread::~DirectSoundPlayerThread() {
	CloseHandle(stopnotify);
}

wxThread::ExitCode DirectSoundPlayerThread::Entry() {
	CoInitialize(0);

	// Wake up thread every half second to fill buffer as needed
	// This more or less assumes the buffer is at least one second long
	while (WaitForSingleObject(stopnotify, 50) == WAIT_TIMEOUT) {
		if (!parent->FillBuffer(false)) {
			// FillBuffer returns false when end of stream is reached
			LOG_D("audio/player/dsound1") << "DS thread hit end of stream";
			break;
		}
	}

	// Now fill buffer with silence
	DWORD bytesFilled = 0;
	while (WaitForSingleObject(stopnotify, 50) == WAIT_TIMEOUT) {
		void *buf1, *buf2;
		DWORD size1, size2;
		DWORD playpos;
		HRESULT res;
		res = parent->buffer->GetCurrentPosition(&playpos, nullptr);
		if (FAILED(res)) break;
		int toWrite = playpos - parent->offset;
		while (toWrite < 0) toWrite += parent->bufSize;
		res = parent->buffer->Lock(parent->offset, toWrite, &buf1, &size1, &buf2, &size2, 0);
		if (FAILED(res)) break;
		if (size1) memset(buf1, 0, size1);
		if (size2) memset(buf2, 0, size2);
		LOG_D_IF(size1, "audio/player/dsound1") << "DS blnk:" << (unsigned long)parent->playPos+bytesFilled << " -> " << (unsigned long)parent->playPos+bytesFilled+size1;
		LOG_D_IF(size2, "audio/player/dsound1") << "DS blnk:" << (unsigned long)parent->playPos+bytesFilled+size1 << " -> " << (unsigned long)parent->playPos+bytesFilled+size1+size2;
		bytesFilled += size1 + size2;
		parent->buffer->Unlock(buf1, size1, buf2, size2);
		if (bytesFilled > parent->bufSize) break;
		parent->offset = (parent->offset + size1 + size2) % parent->bufSize;
	}

	WaitForSingleObject(stopnotify, 150);

	LOG_D("audio/player/dsound1") << "DS thread dead";

	parent->playing = false;
	parent->buffer->Stop();

	CoUninitialize();
	return 0;
}

void DirectSoundPlayerThread::Stop() {
	// Increase the stopnotify by one, causing a wait for it to succeed
	SetEvent(stopnotify);
}
}

std::unique_ptr<AudioPlayer> CreateDirectSoundPlayer(agi::AudioProvider *provider, wxWindow *parent) {
	return agi::make_unique<DirectSoundPlayer>(provider, parent);
}

#endif // WITH_DIRECTSOUND
