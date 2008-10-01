// Copyright (c) 2008, Karl Blomster
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

#ifdef WITH_FFMPEGSOURCE

///////////
// Headers
#include "video_provider_ffmpegsource.h"
#include "video_context.h"
#include "options.h"


///////////////
// Constructor
FFmpegSourceVideoProvider::FFmpegSourceVideoProvider(Aegisub::String filename, double fps) {
	// initialize ffmpegsource
	FFMS_Init();

	// clean up variables
	VideoSource = NULL;
	Index = NULL;
	DstFormat = FFMS_PIX_FMT_NONE;
	LastDstFormat = FFMS_PIX_FMT_NONE;
	KeyFramesLoaded = false;
	FrameNumber = -1;
	MessageSize = sizeof(FFMSErrorMessage);
	ErrorMsg = _T("FFmpegSource video provider: ");

	// and here we go
	try {
		LoadVideo(filename, fps);
	} catch (...) {
		Close();
		throw;
	}
}

///////////////
// Destructor
FFmpegSourceVideoProvider::~FFmpegSourceVideoProvider() {
	Close();
}

///////////////
// Open video
void FFmpegSourceVideoProvider::LoadVideo(Aegisub::String filename, double fps) {
	// make sure we don't have anything messy lying around
	Close();

	wxString FileNameWX(filename.c_str(), wxConvFile);

	// generate a name for the cache file
	wxString CacheName(filename.c_str(), wxConvFile);
	CacheName.append(_T(".ffindex"));

	// try to read index
	Index = FFMS_ReadIndex(CacheName.char_str(), FFMSErrorMessage, MessageSize);
	if (Index == NULL) {
		// index didn't exist or was invalid, we'll have to (re)create it
		try {
			// ignore audio decoding errors here, we don't care right now
			Index = DoIndexing(Index, FileNameWX, CacheName, FFMSTrackMaskAll, true);
		} catch (wxString temp) {
			ErrorMsg << temp;
			throw ErrorMsg;
		} catch (...) {
			throw;
		}
	}

	// set thread count
	int Threads = Options.AsInt(_T("FFmpegSource decoding threads"));
	if (Threads < 1)
		throw _T("FFmpegSource video provider: invalid decoding thread count");

	// set seekmode
	// TODO: give this its own option?
	int SeekMode;
	if (Options.AsBool(_T("FFmpeg allow unsafe seeking")))
		SeekMode = 2;
	else 
		SeekMode = 1;

	// FIXME: provide a way to choose which audio track to load?
	int TrackNumber = FFMS_GetFirstTrackOfType(Index, FFMS_TYPE_VIDEO, FFMSErrorMessage, MessageSize);
	if (TrackNumber < 0) {
		wxString temp(FFMSErrorMessage, wxConvUTF8);
		ErrorMsg << _T("Couldn't find any video tracks: ") << temp;
		throw ErrorMsg;
	}

	VideoSource = FFMS_CreateVideoSource(FileNameWX.char_str(), TrackNumber, Index, "", Threads, SeekMode, FFMSErrorMessage, MessageSize);
	if (VideoSource == NULL) {
		wxString temp(FFMSErrorMessage, wxConvUTF8);
		ErrorMsg << _T("Failed to open video track: ") << temp;
		throw ErrorMsg;
	}

	// load video properties
	VideoInfo = FFMS_GetVideoProperties(VideoSource);

	// get frame info data
	FrameInfoVector *FrameData = FFMS_GetVSTrackIndex(VideoSource);
	if (FrameData == NULL)
		throw _T("FFmpegSource video provider: failed to get frame data");
	const TrackTimeBase *TimeBase = FFMS_GetTimeBase(FrameData);
	if (TimeBase == NULL)
		throw _T("FFmpegSource video provider: failed to get track time base");

	const FrameInfo *CurFrameData;

	// build list of keyframes and timecodes
	for (int CurFrameNum = 0; CurFrameNum < VideoInfo->NumFrames; CurFrameNum++) {
		CurFrameData = FFMS_GetFrameInfo(FrameData, CurFrameNum, FFMSErrorMessage, MessageSize);
		if (CurFrameData == NULL) {
			wxString temp(FFMSErrorMessage, wxConvUTF8);
			ErrorMsg << _T("Couldn't get framedata for frame ") << CurFrameNum << _T(": ") << temp;
			throw ErrorMsg;
		}

		// keyframe?
		if (CurFrameData->KeyFrame)
			KeyFramesList.Add(CurFrameNum);

		// calculate timestamp and add to timecodes vector
		int64_t Timestamp = (int64_t)((CurFrameData->DTS * TimeBase->Num) / (double)TimeBase->Den);
		TimecodesVector.push_back(Timestamp);
	}
	KeyFramesLoaded = true;

	// override already loaded timecodes?
	Timecodes.SetVFR(TimecodesVector);
	int OverrideTC = wxYES;
	if (VFR_Output.IsLoaded()) {
		OverrideTC = wxMessageBox(_("You already have timecodes loaded. Would you like to replace them with timecodes from the video file?"), _("Replace timecodes?"), wxYES_NO | wxICON_QUESTION);
		if (OverrideTC == wxYES) {
			VFR_Input.SetVFR(TimecodesVector);
			VFR_Output.SetVFR(TimecodesVector);
		}
	} else { // no timecodes loaded, go ahead and apply
		VFR_Input.SetVFR(TimecodesVector);
		VFR_Output.SetVFR(TimecodesVector);
	}

	FrameNumber = 0;
}

///////////////
// Close video
void FFmpegSourceVideoProvider::Close() {
	if (VideoSource)
		FFMS_DestroyVideoSource(VideoSource);
	VideoSource = NULL;
	if (Index)
		FFMS_DestroyFrameIndex(Index);

	DstFormat = FFMS_PIX_FMT_NONE;
	LastDstFormat = FFMS_PIX_FMT_NONE;
	KeyFramesLoaded = false;
	KeyFramesList.clear();
	TimecodesVector.clear();
	FrameNumber = -1;
}


///////////////
// Get frame
const AegiVideoFrame FFmpegSourceVideoProvider::GetFrame(int _n, int FormatType) {
	// don't try to seek to insane places
	int n = _n;
	if (n < 0)
		n = 0;
	if (n >= GetFrameCount())
		n = GetFrameCount()-1;
	// set position
	FrameNumber = n;
	
	// these are for convenience
	int w = VideoInfo->Width;
	int h = VideoInfo->Height;

	// this is what we'll return eventually
	AegiVideoFrame &DstFrame = CurFrame;

	// choose output format
	if (FormatType & FORMAT_RGB32) {
		DstFormat		= FFMS_PIX_FMT_RGB32;
		DstFrame.format	= FORMAT_RGB32;
	} else if (FormatType & FORMAT_RGB24) {
		DstFormat		= FFMS_PIX_FMT_RGB24;
		DstFrame.format	= FORMAT_RGB24;
	} else if (FormatType & FORMAT_YV12) {
		DstFormat		= FFMS_PIX_FMT_YUV420P; // may or may not work
		DstFrame.format	= FORMAT_YV12;
	} else if (FormatType & FORMAT_YUY2) {
		DstFormat		= FFMS_PIX_FMT_YUYV422;
		DstFrame.format	= FORMAT_YUY2;
	} else 
		throw _T("FFmpegSource video provider: upstream provider requested unknown or unsupported pixel format");

	// requested format was changed since last time we were called, (re)set output format
	if (LastDstFormat != DstFormat) {
		if (FFMS_SetOutputFormat(VideoSource, DstFormat, w, h, FFMSErrorMessage, MessageSize)) {
			wxString temp(FFMSErrorMessage, wxConvUTF8);
			ErrorMsg << _T("Failed to set output format: ") << temp;
			throw ErrorMsg;
		}
		LastDstFormat = DstFormat;
	}

	// decode frame
	const AVFrameLite *SrcFrame = FFMS_GetFrame(VideoSource, n, FFMSErrorMessage, MessageSize);
	if (SrcFrame == NULL) {
		wxString temp(FFMSErrorMessage, wxConvUTF8);
		ErrorMsg << _T("Failed to retrieve frame: ") << temp;
		throw ErrorMsg;
	}

	// set some properties
	DstFrame.w = w;
	DstFrame.h = h;
	DstFrame.flipped = false;
	if (DstFrame.format == FORMAT_RGB32 || DstFrame.format == FORMAT_RGB24)
		DstFrame.invertChannels = true;
	else
		DstFrame.invertChannels = false;

	// allocate destination
	for (int i = 0; i < 4; i++)
		DstFrame.pitch[i] = SrcFrame->Linesize[i];
	DstFrame.Allocate();

	// copy data to destination, skipping planes with no data in them
	for (int j = 0; j < 4; j++) {
		if (SrcFrame->Linesize[j] > 0)
			memcpy(DstFrame.data[j], SrcFrame->Data[j], DstFrame.pitch[j] * DstFrame.h);
	}

	return DstFrame;
}


///////////////
// Utility functions
int FFmpegSourceVideoProvider::GetWidth() {
	return VideoInfo->Width;
}

int FFmpegSourceVideoProvider::GetHeight() {
	return VideoInfo->Height;
}

int FFmpegSourceVideoProvider::GetFrameCount() {
	return VideoInfo->NumFrames;
}

int FFmpegSourceVideoProvider::GetPosition() {
	return FrameNumber;
}

double FFmpegSourceVideoProvider::GetFPS() {
	return double(VideoInfo->FPSNumerator) / double(VideoInfo->FPSDenominator);
}


#endif /* WITH_FFMPEGSOURCE */
