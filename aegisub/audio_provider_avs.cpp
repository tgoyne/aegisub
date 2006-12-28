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
#include <wx/wxprec.h>
#ifdef __WINDOWS__

#include <wx/filename.h>
#include <Mmreg.h>
#include "avisynth_wrap.h"
#include "utils.h"
#include "audio_provider_avs.h"
#include "options.h"


//////////////
// Constructor
AvisynthAudioProvider::AvisynthAudioProvider(wxString _filename) {
	filename = _filename;

	try {
		OpenAVSAudio();
	}
	catch (...) {
		Unload();
		throw;
	}
}


//////////////
// Destructor
AvisynthAudioProvider::~AvisynthAudioProvider() {
	Unload();
}


////////////////
// Unload audio
void AvisynthAudioProvider::Unload() {
	// Clean up avisynth
	clip = NULL;
}


////////////////////////////
// Load audio from avisynth
void AvisynthAudioProvider::OpenAVSAudio() {
	// Set variables
	AVSValue script;

	// Prepare avisynth
	wxMutexLocker lock(AviSynthMutex);

	try {
		const char * argnames[3] = { 0, "video", "audio" };
		AVSValue args[3] = { env->SaveString(filename.mb_str(wxConvLocal)), false, true };
		script = env->Invoke("DirectShowSource", AVSValue(args,3),argnames);

		LoadFromClip(script);

	}
	
	catch (AvisynthError &err) {
		throw wxString::Format(_T("AviSynth error: %s"), wxString(err.msg,wxConvLocal));
	}
}


/////////////////////////
// Read from environment
void AvisynthAudioProvider::LoadFromClip(AVSValue _clip) {	
	// Prepare avisynth
	AVSValue script;

	// Check if it has audio
	VideoInfo vi = _clip.AsClip()->GetVideoInfo();
	if (!vi.HasAudio()) throw wxString(_T("No audio found."));

	// Convert to one channel
	char buffer[1024];
	strcpy(buffer,Options.AsText(_T("Audio Downmixer")).mb_str(wxConvLocal));
	script = env->Invoke(buffer, _clip);

	// Convert to 16 bits per sample
	script = env->Invoke("ConvertAudioTo16bit", script);

	// Convert sample rate
	int setsample = Options.AsInt(_T("Audio Sample Rate"));
	if (setsample != 0) {
		AVSValue args[2] = { script, setsample };
		script = env->Invoke("ResampleAudio", AVSValue(args,2));
	}

	// Set clip
	PClip tempclip = script.AsClip();
	vi = tempclip->GetVideoInfo();

	// Read properties
	channels = vi.AudioChannels();
	num_samples = vi.num_audio_samples;
	sample_rate = vi.SamplesPerSecond();
	bytes_per_sample = vi.BytesPerAudioSample();

	// Set
	clip = tempclip;
}


////////////////
// Get filename
wxString AvisynthAudioProvider::GetFilename() {
	return filename;
}

/////////////
// Get audio
void AvisynthAudioProvider::GetAudio(void *buf, __int64 start, __int64 count) {
	// Requested beyond the length of audio
	if (start+count > num_samples) {
		__int64 oldcount = count;
		count = num_samples-start;
		if (count < 0) count = 0;

		// Fill beyond with zero
		if (bytes_per_sample == 1) {
			char *temp = (char *) buf;
			for (int i=count;i<oldcount;i++) {
				temp[i] = 0;
			}
		}
		if (bytes_per_sample == 2) {
			short *temp = (short *) buf;
			for (int i=count;i<oldcount;i++) {
				temp[i] = 0;
			}
		}
	}

	if (count) {
		clip->GetAudio(buf,start,count,env);
	}
}

#endif
