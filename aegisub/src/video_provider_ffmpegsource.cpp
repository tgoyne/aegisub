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

/// @file video_provider_ffmpegsource.cpp
/// @brief FFmpegSource2-based video provider
/// @ingroup video_input ffms
///

#include "config.h"

#ifdef WITH_FFMS2
#include "video_provider_ffmpegsource.h"

#include "aegisub_endian.h"
#include "compat.h"
#include "options.h"
#include "utils.h"
#include "video_context.h"

#include <libaegisub/fs.h>

#include <wx/choicdlg.h>
#include <wx/msgdlg.h>

FFmpegSourceVideoProvider::FFmpegSourceVideoProvider(agi::fs::path const& filename) try
: VideoSource(nullptr, FFMS_DestroyVideoSource)
, VideoInfo(nullptr)
, Width(-1)
, Height(-1)
, FrameNumber(-1)
{
	ErrInfo.Buffer		= FFMSErrMsg;
	ErrInfo.BufferSize	= sizeof(FFMSErrMsg);
	ErrInfo.ErrorType	= FFMS_ERROR_SUCCESS;
	ErrInfo.SubType		= FFMS_ERROR_SUCCESS;

	SetLogLevel();

	LoadVideo(filename);
}
catch (std::string const& err) {
	throw VideoOpenError(err);
}
catch (const char * err) {
	throw VideoOpenError(err);
}

void FFmpegSourceVideoProvider::LoadVideo(agi::fs::path const& filename) {
	FFMS_Indexer *Indexer = FFMS_CreateIndexer(filename.string().c_str(), &ErrInfo);
	if (!Indexer) {
		if (ErrInfo.SubType == FFMS_ERROR_FILE_READ)
			throw agi::fs::FileNotFound(std::string(ErrInfo.Buffer));
		else
			throw VideoNotSupported(ErrInfo.Buffer);
	}

	std::map<int, std::string> TrackList = GetTracksOfType(Indexer, FFMS_TYPE_VIDEO);
	if (TrackList.size() <= 0)
		throw VideoNotSupported("no video tracks found");

	// initialize the track number to an invalid value so we can detect later on
	// whether the user actually had to choose a track or not
	int TrackNumber = -1;
	if (TrackList.size() > 1) {
		TrackNumber = AskForTrackSelection(TrackList, FFMS_TYPE_VIDEO);
		// if it's still -1 here, user pressed cancel
		if (TrackNumber == -1)
			throw agi::UserCancelException("video loading cancelled by user");
	}

	// generate a name for the cache file
	auto CacheName = GetCacheFilename(filename);

	// try to read index
	agi::scoped_holder<FFMS_Index*, void (FFMS_CC*)(FFMS_Index*)>
		Index(FFMS_ReadIndex(CacheName.string().c_str(), &ErrInfo), FFMS_DestroyIndex);

	if (Index && FFMS_IndexBelongsToFile(Index, filename.string().c_str(), &ErrInfo))
		Index = nullptr;

	// time to examine the index and check if the track we want is indexed
	// technically this isn't really needed since all video tracks should always be indexed,
	// but a bit of sanity checking never hurt anyone
	if (Index && TrackNumber >= 0) {
		FFMS_Track *TempTrackData = FFMS_GetTrackFromIndex(Index, TrackNumber);
		if (FFMS_GetNumFrames(TempTrackData) <= 0)
			Index = nullptr;
	}

	// moment of truth
	if (!Index) {
		int TrackMask = FFMS_TRACKMASK_NONE;
		if (OPT_GET("Provider/FFmpegSource/Index All Tracks")->GetBool() || OPT_GET("Video/Open Audio")->GetBool())
			TrackMask = FFMS_TRACKMASK_ALL;
		Index = DoIndexing(Indexer, CacheName, TrackMask, GetErrorHandlingMode());
	}
	else {
		FFMS_CancelIndexing(Indexer);
	}

	// update access time of index file so it won't get cleaned away
	agi::fs::Touch(CacheName);

	// we have now read the index and may proceed with cleaning the index cache
	CleanCache();

	// track number still not set?
	if (TrackNumber < 0) {
		// just grab the first track
		TrackNumber = FFMS_GetFirstIndexedTrackOfType(Index, FFMS_TYPE_VIDEO, &ErrInfo);
		if (TrackNumber < 0)
			throw VideoNotSupported(std::string("Couldn't find any video tracks: ") + ErrInfo.Buffer);
	}

	// set thread count
	int Threads = OPT_GET("Provider/Video/FFmpegSource/Decoding Threads")->GetInt();
	if (FFMS_GetVersion() < ((2 << 24) | (17 << 16) | (2 << 8) | 1) && FFMS_GetSourceType(Index) == FFMS_SOURCE_LAVF)
		Threads = 1;

	// set seekmode
	// TODO: give this its own option?
	int SeekMode;
	if (OPT_GET("Provider/Video/FFmpegSource/Unsafe Seeking")->GetBool())
		SeekMode = FFMS_SEEK_UNSAFE;
	else
		SeekMode = FFMS_SEEK_NORMAL;

	VideoSource = FFMS_CreateVideoSource(filename.string().c_str(), TrackNumber, Index, Threads, SeekMode, &ErrInfo);
	if (!VideoSource)
		throw VideoOpenError(std::string("Failed to open video track: ") + ErrInfo.Buffer);

	// load video properties
	VideoInfo = FFMS_GetVideoProperties(VideoSource);

	const FFMS_Frame *TempFrame = FFMS_GetFrame(VideoSource, 0, &ErrInfo);
	if (!TempFrame)
		throw VideoOpenError(std::string("Failed to decode first frame: ") + ErrInfo.Buffer);

	Width  = TempFrame->EncodedWidth;
	Height = TempFrame->EncodedHeight;
	if (VideoInfo->SARDen > 0 && VideoInfo->SARNum > 0)
		DAR = double(Width) * VideoInfo->SARNum / ((double)Height * VideoInfo->SARDen);
	else
		DAR = double(Width) / Height;

	// Assuming TV for unspecified
	ColorSpace = TempFrame->ColorRange == FFMS_CR_JPEG ? "PC" : "TV";

	int CS = TempFrame->ColorSpace;
#if FFMS_VERSION >= ((2 << 24) | (17 << 16) | (1 << 8) | 0)
	if (CS != FFMS_CS_RGB && CS != FFMS_CS_BT470BG && OPT_GET("Video/Force BT.601")->GetBool()) {
		if (FFMS_SetInputFormatV(VideoSource, FFMS_CS_BT470BG, TempFrame->ColorRange, FFMS_GetPixFmt(""), &ErrInfo))
			throw VideoOpenError(std::string("Failed to set input format: ") + ErrInfo.Buffer);

		CS = FFMS_CS_BT470BG;
	}
#endif

	switch (CS) {
		case FFMS_CS_RGB:
			ColorSpace = "None";
			break;
		case FFMS_CS_BT709:
			ColorSpace += ".709";
			break;
		case FFMS_CS_UNSPECIFIED:
			ColorSpace += Width > 1024 || Height >= 600 ? "709" : "601";
			break;
		case FFMS_CS_FCC:
			ColorSpace += ".FCC";
			break;
		case FFMS_CS_BT470BG:
		case FFMS_CS_SMPTE170M:
			ColorSpace += ".601";
			break;
		case FFMS_CS_SMPTE240M:
			ColorSpace += ".240M";
			break;
		default:
			throw VideoOpenError("Unknown video color space");
			break;
	}

	const int TargetFormat[] = { FFMS_GetPixFmt("bgra"), -1 };
	if (FFMS_SetOutputFormatV2(VideoSource, TargetFormat, Width, Height, FFMS_RESIZER_BICUBIC, &ErrInfo)) {
		throw VideoOpenError(std::string("Failed to set output format: ") + ErrInfo.Buffer);
	}

	// get frame info data
	FFMS_Track *FrameData = FFMS_GetTrackFromVideo(VideoSource);
	if (FrameData == nullptr)
		throw VideoOpenError("failed to get frame data");
	const FFMS_TrackTimeBase *TimeBase = FFMS_GetTimeBase(FrameData);
	if (TimeBase == nullptr)
		throw VideoOpenError("failed to get track time base");

	// build list of keyframes and timecodes
	std::vector<int> TimecodesVector;
	for (int CurFrameNum = 0; CurFrameNum < VideoInfo->NumFrames; CurFrameNum++) {
		const FFMS_FrameInfo *CurFrameData = FFMS_GetFrameInfo(FrameData, CurFrameNum);
		if (!CurFrameData)
			throw VideoOpenError("Couldn't get info about frame " + std::to_string(CurFrameNum));

		// keyframe?
		if (CurFrameData->KeyFrame)
			KeyFramesList.push_back(CurFrameNum);

		// calculate timestamp and add to timecodes vector
		int Timestamp = (int)((CurFrameData->PTS * TimeBase->Num) / TimeBase->Den);
		TimecodesVector.push_back(Timestamp);
	}
	if (TimecodesVector.size() < 2)
		Timecodes = 25.0;
	else
		Timecodes = agi::vfr::Framerate(TimecodesVector);

	FrameNumber = 0;
}

const AegiVideoFrame FFmpegSourceVideoProvider::GetFrame(int n) {
	FrameNumber = mid(0, n, GetFrameCount() - 1);

	// decode frame
	const FFMS_Frame *SrcFrame = FFMS_GetFrame(VideoSource, FrameNumber, &ErrInfo);
	if (!SrcFrame)
		throw VideoDecodeError(std::string("Failed to retrieve frame: ") +  ErrInfo.Buffer);

	CurFrame.SetTo(SrcFrame->Data[0], Width, Height, SrcFrame->Linesize[0]);
	return CurFrame;
}

#endif /* WITH_FFMS2 */
