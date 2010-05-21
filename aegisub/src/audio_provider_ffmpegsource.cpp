// Copyright (c) 2008-2009, Karl Blomster
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

/// @file audio_provider_ffmpegsource.cpp
/// @brief ffms2-based audio provider
/// @ingroup audio_input ffms
///

#include "config.h"

#ifdef WITH_FFMPEGSOURCE

///////////
// Headers
#ifndef AGI_PRE
#ifdef WIN32
#include <objbase.h>
#endif

#include <map>
#endif

#include "audio_provider_ffmpegsource.h"
#include "include/aegisub/aegisub.h"
#include "main.h"
#include "options.h"


/// @brief Constructor 
/// @param filename 
///
FFmpegSourceAudioProvider::FFmpegSourceAudioProvider(wxString filename) {
	COMInited = false;
#ifdef WIN32
	HRESULT res;
	res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(res)) 
		COMInited = true;
	else if (res != RPC_E_CHANGED_MODE)
		throw _T("FFmpegSource video provider: COM initialization failure");
#endif
	FFMS_Init(0);

	ErrInfo.Buffer		= FFMSErrMsg;
	ErrInfo.BufferSize	= sizeof(FFMSErrMsg);
	ErrInfo.ErrorType	= FFMS_ERROR_SUCCESS;
	ErrInfo.SubType		= FFMS_ERROR_SUCCESS;
	ErrorMsg = _T("FFmpegSource audio provider: ");

	AudioSource = NULL;

	SetLogLevel();

	try {
		LoadAudio(filename);
	} catch (...) {
		Close();
		throw;
	}
}



/// @brief Load audio file 
/// @param filename 
///
void FFmpegSourceAudioProvider::LoadAudio(wxString filename) {
	// clean up
	Close();

	wxString FileNameShort = wxFileName(filename).GetShortPath();

	FFMS_Indexer *Indexer = FFMS_CreateIndexer(FileNameShort.utf8_str(), &ErrInfo);
	if (Indexer == NULL) {
		// error messages that can possibly contain a filename use this method instead of
		// wxString::Format because they may contain utf8 characters
		ErrorMsg.Append(_T("Failed to create indexer: ")).Append(wxString(ErrInfo.Buffer, wxConvUTF8));
		throw ErrorMsg;
	}

	std::map<int,wxString> TrackList = GetTracksOfType(Indexer, FFMS_TYPE_AUDIO);
	if (TrackList.size() <= 0)
		throw _T("FFmpegSource audio provider: no audio tracks found");

	// initialize the track number to an invalid value so we can detect later on
	// whether the user actually had to choose a track or not
	int TrackNumber = -1;
	if (TrackList.size() > 1) {
		TrackNumber = AskForTrackSelection(TrackList, FFMS_TYPE_AUDIO);
		// if it's still -1 here, user pressed cancel
		if (TrackNumber == -1)
			throw _T("FFmpegSource audio provider: audio loading cancelled by user");
	}

	// generate a name for the cache file
	wxString CacheName = GetCacheFilename(filename);

	// try to read index
	FFMS_Index *Index = NULL;
	Index = FFMS_ReadIndex(CacheName.utf8_str(), &ErrInfo);
	bool IndexIsValid = false;
	if (Index != NULL) {
		if (FFMS_IndexBelongsToFile(Index, FileNameShort.utf8_str(), &ErrInfo)) {
			FFMS_DestroyIndex(Index);
			Index = NULL;
		}
		else
			IndexIsValid = true;
	}
	
	// index valid but track number still not set?
	if (IndexIsValid) {
		// track number not set? just grab the first track
		if (TrackNumber < 0)
			TrackNumber = FFMS_GetFirstTrackOfType(Index, FFMS_TYPE_AUDIO, &ErrInfo);
		if (TrackNumber < 0) {
			FFMS_DestroyIndex(Index);
			Index = NULL;
			ErrorMsg.Append(wxString::Format(_T("Couldn't find any audio tracks: %s"), ErrInfo.Buffer));
			throw ErrorMsg;
		}

		// index is valid and track number is now set,
		// but do we have indexing info for the desired audio track?
		FFMS_Track *TempTrackData = FFMS_GetTrackFromIndex(Index, TrackNumber);
		if (FFMS_GetNumFrames(TempTrackData) <= 0) {
			IndexIsValid = false;
			FFMS_DestroyIndex(Index);
			Index = NULL;
		}
	}
	// no valid index exists and the file only has one audio track, index it
	else if (TrackNumber < 0) 
		TrackNumber = FFMS_TRACKMASK_ALL;
	// else: do nothing (keep track mask as it is)
	
	// moment of truth
	if (!IndexIsValid) {
		int TrackMask;
		if (OPT_GET("Provider/FFmpegSource/Index All Tracks")->GetBool() || TrackNumber == FFMS_TRACKMASK_ALL)
			TrackMask = FFMS_TRACKMASK_ALL;
		else
			TrackMask = (1 << TrackNumber);

		try {
			Index = DoIndexing(Indexer, CacheName, TrackMask, GetErrorHandlingMode());
		} catch (wxString temp) {
			ErrorMsg.Append(temp);
			throw ErrorMsg;
		} catch (...) {
			throw;
		}

		// if tracknumber still isn't set we need to set it now
		if (TrackNumber == FFMS_TRACKMASK_ALL)
			TrackNumber = FFMS_GetFirstTrackOfType(Index, FFMS_TYPE_AUDIO, &ErrInfo);
	}

	// update access time of index file so it won't get cleaned away
	if (!wxFileName(CacheName).Touch()) {
		// warn user?
	}

	AudioSource = FFMS_CreateAudioSource(FileNameShort.utf8_str(), TrackNumber, Index, &ErrInfo);
	FFMS_DestroyIndex(Index);
	Index = NULL;
	if (!AudioSource) {
		ErrorMsg.Append(wxString::Format(_T("Failed to open audio track: %s"), ErrInfo.Buffer));
		throw ErrorMsg;
	}
		
	const FFMS_AudioProperties AudioInfo = *FFMS_GetAudioProperties(AudioSource);

	channels	= AudioInfo.Channels;
	sample_rate	= AudioInfo.SampleRate;
	num_samples = AudioInfo.NumSamples;
	if (channels <= 0 || sample_rate <= 0 || num_samples <= 0)
		throw _T("FFmpegSource audio provider: sanity check failed, consult your local psychiatrist");

	// FIXME: use the actual sample format too?
	// why not just bits_per_sample/8? maybe there's some oddball format with half bytes out there somewhere...
	switch (AudioInfo.BitsPerSample) {
		case 8:		bytes_per_sample = 1; break;
		case 16:	bytes_per_sample = 2; break;
		case 24:	bytes_per_sample = 3; break;
		case 32:	bytes_per_sample = 4; break;
		default:
			throw _T("FFmpegSource audio provider: unknown or unsupported sample format");
	}
}



/// @brief Destructor 
///
FFmpegSourceAudioProvider::~FFmpegSourceAudioProvider() {
	Close();
#ifdef WIN32
	if (COMInited)
		CoUninitialize();
#endif
}



/// @brief Clean up 
///
void FFmpegSourceAudioProvider::Close() {
	FFMS_DestroyAudioSource(AudioSource);
	AudioSource = NULL;
}



/// @brief Get audio 
/// @param Buf   
/// @param Start 
/// @param Count 
///
void FFmpegSourceAudioProvider::GetAudio(void *Buf, int64_t Start, int64_t Count) {
	if (FFMS_GetAudio(AudioSource, Buf, Start, Count, &ErrInfo)) {
		ErrorMsg.Append(wxString::Format(_T("Failed to get audio samples: %s"), ErrInfo.Buffer));
		throw ErrorMsg;
	}
}


#endif /* WITH_FFMPEGSOURCE */


