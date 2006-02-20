// Copyright (c) 2005, Rodrigo Braz Monteiro
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


#ifndef AUDIO_PROVIDER_H
#define AUDIO_PROVIDER_H


///////////
// Headers
#include <wx/wxprec.h>
#include "avisynth_wrap.h"
#include <fstream>
#include <time.h>


//////////////
// Prototypes
class AudioDisplay;


//////////////
// Types enum
enum AudioProviderType {
	AUDIO_PROVIDER_NONE,
	AUDIO_PROVIDER_AVS,
	AUDIO_PROVIDER_CACHE,
	AUDIO_PROVIDER_DISK_CACHE
};


////////////////////////
// Audio provider class
class AudioProvider : public wxEvtHandler, public AviSynthWrapper {
private:
	static int pa_refcount;

	wxMutex diskmutex;

	AudioProviderType type;

	char** blockcache;
	int blockcount;

	void *raw;
	int raw_len;

	AudioDisplay *display;

	std::ifstream file_cache;

	wxString filename;
	PClip clip;
	int channels;
	__int64 num_samples;
	int sample_rate;
	int bytes_per_sample;

	void OpenStream();
	void CloseStream();
	void ConvertToRAMCache(PClip &tempclip);
	void ConvertToDiskCache(PClip &tempclip);
	void LoadFromClip(AVSValue clip);
	void OpenAVSAudio();
	void OnStopAudio(wxCommandEvent &event);
	static wxString DiskCachePath();
	static wxString DiskCacheName();
	void SetFile();
	void Unload();
public:
	wxMutex PAMutex;
	volatile bool stopping;
	bool softStop;
	bool playing;
	bool spectrum;
	float volume;

	volatile __int64 playPos;
	volatile __int64 startPos;
	volatile __int64 endPos;
	volatile __int64 realPlayPos;
	volatile __int64 startMS;
	void *stream;
	clock_t span;

	AudioProvider(wxString _filename, AudioDisplay *_display);
	~AudioProvider();

	wxString GetFilename();

	void GetAudio(void *buf, __int64 start, __int64 count);
	void GetWaveForm(int *min,int *peak,__int64 start,int w,int h,int samples,float scale);

	void Play(__int64 start,__int64 count);
	void Stop(bool timerToo=true);
	void RequestStop();

	int GetChannels();
	__int64 GetNumSamples();
	int GetSampleRate();

	DECLARE_EVENT_TABLE()
};


/////////
// Event
DECLARE_EVENT_TYPE(wxEVT_STOP_AUDIO, -1)


#endif
