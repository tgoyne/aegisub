//  Copyright (c) 2007-2008 Fredrik Mellbin
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#include "ffvideosource.h"

int VideoBase::InitPP(const char *PP, int PixelFormat, char *ErrorMsg, unsigned MsgSize) {
	if (!strcmp(PP, ""))
		return 0;

	PPMode = pp_get_mode_by_name_and_quality(PP, PP_QUALITY_MAX);
	if (!PPMode) {
		_snprintf(ErrorMsg, MsgSize, "Invalid postprocesing settings");
		return 1;
	}

	int Flags =  GetCPUFlags();

	switch (PixelFormat) {
		case PIX_FMT_YUV420P: Flags |= PP_FORMAT_420; break;
		case PIX_FMT_YUV422P: Flags |= PP_FORMAT_422; break;
		case PIX_FMT_YUV411P: Flags |= PP_FORMAT_411; break;
		case PIX_FMT_YUV444P: Flags |= PP_FORMAT_444; break;
		default:
			_snprintf(ErrorMsg, MsgSize, "Input format is not supported for postprocessing");
			return 2;
	}

	PPContext = pp_get_context(VP.Width, VP.Height, Flags);

	if (!(PPFrame = avcodec_alloc_frame())) {
		_snprintf(ErrorMsg, MsgSize, "Failed to allocate temporary frame");
		return 3;
	}

	if (avpicture_alloc((AVPicture *)PPFrame, PixelFormat, VP.Width, VP.Height) < 0) {
		_snprintf(ErrorMsg, MsgSize, "Failed to allocate picture");
		return 4;
	}

	return 0;
}

AVFrameLite *VideoBase::OutputFrame(AVFrame *Frame) {
	if (PPContext) {
		pp_postprocess(const_cast<const uint8_t **>(Frame->data), Frame->linesize, PPFrame->data, PPFrame->linesize, VP.Width, VP.Height, Frame->qscale_table, Frame->qstride, PPMode, PPContext, Frame->pict_type | (Frame->qscale_type ? PP_PICT_TYPE_QP2 : 0));
		PPFrame->key_frame = Frame->key_frame;
		PPFrame->pict_type = Frame->pict_type;
		return reinterpret_cast<AVFrameLite *>(PPFrame);
	} else {
		return reinterpret_cast<AVFrameLite *>(Frame);
	}
}

VideoBase::VideoBase() {
	memset(&VP, 0, sizeof(VP));
	PPContext = NULL;
	PPMode = NULL;
	LastFrameNum = -1;
	CurrentFrame = 0;
	CodecContext = NULL;
	DecodeFrame = avcodec_alloc_frame();
	PPFrame = DecodeFrame;
}

VideoBase::~VideoBase() {
	if (PPMode)
		pp_free_mode(PPMode);

	if (PPContext)
		pp_free_context(PPContext);

	if (PPFrame != DecodeFrame) {
		avpicture_free((AVPicture *)PPFrame);
		av_free(PPFrame);
	}

	av_free(DecodeFrame);
}

AVFrame *GetFrameByTime(double Time, char *ErrorMsg, unsigned MsgSize) {

	//Frames.ClosestFrameFromDTS();
	//return GetFrame(, ErrorMsg, MsgSize);
	return NULL;
}

int FFVideoSource::GetTrackIndex(int &Index, char *ErrorMsg, unsigned MsgSize) {
	if (Index < 0) {
		Index = -1;
		for (unsigned int i = 0; i < FormatContext->nb_streams; i++)
			if (FormatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO) {
				Index = i;
				break;
			}
	}

	if (Index < 0) {
		_snprintf(ErrorMsg, MsgSize, "No video track found");
		return 1;
	}

	if (Index >= (int)FormatContext->nb_streams) {
		_snprintf(ErrorMsg, MsgSize, "Invalid video track number");
		return 2;
	}

	if (FormatContext->streams[Index]->codec->codec_type != CODEC_TYPE_VIDEO) {
		_snprintf(ErrorMsg, MsgSize, "Selected track is not video");
		return 3;
	}

	return 0;
}

FFVideoSource::FFVideoSource(const char *SourceFile, int Track, FrameIndex *TrackIndices,
	const char *PP, int Threads, int SeekMode, char *ErrorMsg, unsigned MsgSize) {

	FormatContext = NULL;
	AVCodec *Codec = NULL;
	this->SeekMode = SeekMode;

	if (av_open_input_file(&FormatContext, SourceFile, NULL, 0, NULL) != 0) {
		_snprintf(ErrorMsg, MsgSize, "Couldn't open '%s'", SourceFile);
		throw ErrorMsg;
	}
	
	if (av_find_stream_info(FormatContext) < 0) {
		_snprintf(ErrorMsg, MsgSize, "Couldn't find stream information");
		throw ErrorMsg;
	}

	VideoTrack = Track;
	if (GetTrackIndex(VideoTrack, ErrorMsg, MsgSize))
		throw ErrorMsg;

	Frames = (*TrackIndices)[VideoTrack];

	if (Frames.size() == 0) {
		_snprintf(ErrorMsg, MsgSize, "Video track contains no frames");
		throw ErrorMsg;
	}

	if (SeekMode >= 0 && av_seek_frame(FormatContext, VideoTrack, Frames[0].DTS, AVSEEK_FLAG_BACKWARD) < 0) {
		_snprintf(ErrorMsg, MsgSize, "Video track is unseekable");
		throw ErrorMsg;
	}

	CodecContext = FormatContext->streams[VideoTrack]->codec;
	CodecContext->thread_count = Threads;

	Codec = avcodec_find_decoder(CodecContext->codec_id);
	if (Codec == NULL) {
		_snprintf(ErrorMsg, MsgSize, "Video codec not found");
		throw ErrorMsg;
	}

	if (avcodec_open(CodecContext, Codec) < 0) {
		_snprintf(ErrorMsg, MsgSize, "Could not open video codec");
		throw ErrorMsg;
	}

	// Always try to decode a frame to make sure all required parameters are known
	int64_t Dummy;
	if (DecodeNextFrame(DecodeFrame, &Dummy, ErrorMsg, MsgSize))
		throw ErrorMsg;

	//VP.image_type = VideoInfo::IT_TFF;
	VP.Width = CodecContext->width;
	VP.Height = CodecContext->height;
	VP.FPSDenominator = FormatContext->streams[VideoTrack]->time_base.num;
	VP.FPSNumerator = FormatContext->streams[VideoTrack]->time_base.den;
	VP.NumFrames = Frames.size();
	VP.PixelFormat = CodecContext->pix_fmt;

	if (VP.Width <= 0 || VP.Height <= 0) {
		_snprintf(ErrorMsg, MsgSize, "Codec returned zero size video");
		throw ErrorMsg;
	}

	// sanity check framerate
	if (VP.FPSDenominator > VP.FPSNumerator || VP.FPSDenominator <= 0 || VP.FPSNumerator <= 0) {
		VP.FPSDenominator = 1;
		VP.FPSNumerator = 30;
	}

	InitPP(PP, CodecContext->pix_fmt, ErrorMsg, MsgSize);

	// Adjust framerate to match the duration of the first frame
	if (Frames.size() >= 2) {
		unsigned int DTSDiff = (unsigned int)FFMAX(Frames[1].DTS - Frames[0].DTS, 1);
		VP.FPSDenominator *= DTSDiff;
	}

	// Cannot "output" to PPFrame without doing all other initialization
	// This is the additional mess required for seekmode=-1 to work in a reasonable way
	OutputFrame(DecodeFrame);
	LastFrameNum = 0;


	// Set AR variables
	VP.SARNum = CodecContext->sample_aspect_ratio.num;
	VP.SARDen = CodecContext->sample_aspect_ratio.den;
}

FFVideoSource::~FFVideoSource() {
	if (VideoTrack >= 0)
		avcodec_close(CodecContext);
	av_close_input_file(FormatContext);
}

int FFVideoSource::DecodeNextFrame(AVFrame *AFrame, int64_t *AStartTime, char *ErrorMsg, unsigned MsgSize) {
	AVPacket Packet;
	int FrameFinished = 0;
	*AStartTime = -1;

	while (av_read_frame(FormatContext, &Packet) >= 0) {
        if (Packet.stream_index == VideoTrack) {
			if (*AStartTime < 0)
				*AStartTime = Packet.dts;

			avcodec_decode_video(CodecContext, AFrame, &FrameFinished, Packet.data, Packet.size);
        }

        av_free_packet(&Packet);

		if (FrameFinished)
			goto Done;
	}

	// Flush the last frames
	if (CodecContext->has_b_frames)
		avcodec_decode_video(CodecContext, AFrame, &FrameFinished, NULL, 0);

	if (!FrameFinished)
		goto Error;

// Ignore errors for now
Error:
Done:
	return 0;
}

AVFrameLite *FFVideoSource::GetFrame(int n, char *ErrorMsg, unsigned MsgSize) {
	// PPFrame always holds frame LastFrameNum even if no PP is applied
	if (LastFrameNum == n)
		return reinterpret_cast<AVFrameLite *>(PPFrame);

	bool HasSeeked = false;

	if (SeekMode >= 0) {
		int ClosestKF = Frames.FindClosestKeyFrame(n);

		if (SeekMode == 0) {
			if (n < CurrentFrame) {
				av_seek_frame(FormatContext, VideoTrack, Frames[0].DTS, AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(CodecContext);
				CurrentFrame = 0;
			}
		} else {
			// 10 frames is used as a margin to prevent excessive seeking since the predicted best keyframe isn't always selected by avformat
			if (n < CurrentFrame || ClosestKF > CurrentFrame + 10 || (SeekMode == 3 && n > CurrentFrame + 10)) {
				av_seek_frame(FormatContext, VideoTrack, (SeekMode == 3) ? Frames[n].DTS : Frames[ClosestKF].DTS, AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(CodecContext);
				HasSeeked = true;
			}
		}
	} else if (n < CurrentFrame) {
		_snprintf(ErrorMsg, MsgSize, "Non-linear access attempted");
		return NULL;
	}

	do {
		int64_t StartTime;
		if (DecodeNextFrame(DecodeFrame, &StartTime, ErrorMsg, MsgSize))
			return NULL;

		if (HasSeeked) {
			HasSeeked = false;

			// Is the seek destination time known? Does it belong to a frame?
			if (StartTime < 0 || (CurrentFrame = Frames.FrameFromDTS(StartTime)) < 0) {
				switch (SeekMode) {
					case 1:
						_snprintf(ErrorMsg, MsgSize, "Frame accurate seeking is not possible in this file");
						return NULL;
					case 2:
					case 3:
						CurrentFrame = Frames.ClosestFrameFromDTS(StartTime);
						break;
					default:
						_snprintf(ErrorMsg, MsgSize, "Failed assertion");
						return NULL;
				}	
			}
		}

		CurrentFrame++;
	} while (CurrentFrame <= n);

	LastFrameNum = n;
	return OutputFrame(DecodeFrame);
}


int MatroskaVideoSource::GetTrackIndex(int &Index, char *ErrorMsg, unsigned MsgSize) {
	if (Index < 0) {
		Index = -1;
		for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++)
			if (mkv_GetTrackInfo(MF, i)->Type == TT_VIDEO) {
				Index = i;
				break;
			}
	}

	if (Index < 0) {
		_snprintf(ErrorMsg, MsgSize, "No video track found");
		return 1;
	}

	if (Index >= (int)mkv_GetNumTracks(MF)) {
		_snprintf(ErrorMsg, MsgSize, "Invalid video track number");
		return 2;
	}

	if (mkv_GetTrackInfo(MF, Index)->Type != TT_VIDEO) {
		_snprintf(ErrorMsg, MsgSize, "Selected track is not video");
		return 3;
	}

	return 0;
}

MatroskaVideoSource::MatroskaVideoSource(const char *SourceFile, int Track,
	FrameIndex *TrackIndices, const char *PP,
	int Threads, char *ErrorMsg, unsigned MsgSize) {

	unsigned int TrackMask = ~0;
	AVCodec *Codec = NULL;
	TrackInfo *TI = NULL;
	CS = NULL;

	MC.ST.fp = fopen(SourceFile, "rb");
	if (MC.ST.fp == NULL) {
		_snprintf(ErrorMsg, MsgSize, "Can't open '%s': %s", SourceFile, strerror(errno));
		throw ErrorMsg;
	}

	setvbuf(MC.ST.fp, NULL, _IOFBF, CACHESIZE);

	MF = mkv_OpenEx(&MC.ST.base, 0, 0, ErrorMessage, sizeof(ErrorMessage));
	if (MF == NULL) {
		fclose(MC.ST.fp);
		_snprintf(ErrorMsg, MsgSize, "Can't parse Matroska file: %s", ErrorMessage);
		throw ErrorMsg;
	}

	VideoTrack = Track;
	if (GetTrackIndex(VideoTrack, ErrorMsg, MsgSize))
		throw ErrorMsg;

	Frames = (*TrackIndices)[VideoTrack];

	if (Frames.size() == 0) {
		_snprintf(ErrorMsg, MsgSize, "Video track contains no frames");
		throw ErrorMsg;
	}

	mkv_SetTrackMask(MF, ~(1 << VideoTrack));
	TI = mkv_GetTrackInfo(MF, VideoTrack);

		if (TI->CompEnabled) {
			CS = cs_Create(MF, VideoTrack, ErrorMessage, sizeof(ErrorMessage));
			if (CS == NULL) {
				_snprintf(ErrorMsg, MsgSize, "Can't create decompressor: %s", ErrorMessage);
				throw ErrorMsg;
			}
		}

	CodecContext = avcodec_alloc_context();
	CodecContext->extradata = (uint8_t *)TI->CodecPrivate;
	CodecContext->extradata_size = TI->CodecPrivateSize;
	CodecContext->thread_count = Threads;

	Codec = avcodec_find_decoder(MatroskaToFFCodecID(TI));
	if (Codec == NULL) {
		_snprintf(ErrorMsg, MsgSize, "Video codec not found");
		throw ErrorMsg;
	}

	if (avcodec_open(CodecContext, Codec) < 0) {
		_snprintf(ErrorMsg, MsgSize, "Could not open video codec");
		throw ErrorMsg;
	}

	// Always try to decode a frame to make sure all required parameters are known
	int64_t Dummy;
	if (DecodeNextFrame(DecodeFrame, &Dummy, ErrorMsg, MsgSize))
		throw ErrorMsg;

	VP.Width = CodecContext->width;
	VP.Height = CodecContext->height;;
	VP.FPSDenominator = 1;
	VP.FPSNumerator = 30;
	VP.NumFrames = Frames.size();
	VP.PixelFormat = CodecContext->pix_fmt;

	if (VP.Width <= 0 || VP.Height <= 0) {
		_snprintf(ErrorMsg, MsgSize, "Codec returned zero size video");
		throw ErrorMsg;
	}

	InitPP(PP, CodecContext->pix_fmt, ErrorMsg, MsgSize);

	// Calculate the average framerate
	if (Frames.size() >= 2) {
		double DTSDiff = (double)(Frames.back().DTS - Frames.front().DTS);
		VP.FPSDenominator = (unsigned int)(DTSDiff * mkv_TruncFloat(TI->TimecodeScale) / (double)1000 / (double)(VP.NumFrames - 1) + 0.5);
		VP.FPSNumerator = 1000000; 
	}

	// Output the already decoded frame so it isn't wasted
	OutputFrame(DecodeFrame);
	LastFrameNum = 0;

	// Set AR variables
	VP.SARNum = TI->AV.Video.DisplayWidth * TI->AV.Video.PixelHeight;
	VP.SARDen = TI->AV.Video.DisplayHeight * TI->AV.Video.PixelWidth;

	// Set crop variables
	VP.CropLeft = TI->AV.Video.CropL;
	VP.CropRight = TI->AV.Video.CropR;
	VP.CropTop = TI->AV.Video.CropT;
	VP.CropBottom = TI->AV.Video.CropB;
}

MatroskaVideoSource::~MatroskaVideoSource() {
	free(Buffer);
	mkv_Close(MF);
	fclose(MC.ST.fp);
	if (CodecContext)
		avcodec_close(CodecContext);
	av_free(CodecContext);
}

int MatroskaVideoSource::DecodeNextFrame(AVFrame *AFrame, int64_t *AFirstStartTime, char *ErrorMsg, unsigned MsgSize) {
	int FrameFinished = 0;
	*AFirstStartTime = -1;

	uint64_t StartTime, EndTime, FilePos;
	unsigned int Track, FrameFlags, FrameSize;
	
	while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0) {
		if (*AFirstStartTime < 0)
			*AFirstStartTime = StartTime;

		if (ReadFrame(FilePos, FrameSize, CS, MC, ErrorMsg, MsgSize))
			return 1;

		avcodec_decode_video(CodecContext, AFrame, &FrameFinished, MC.Buffer, FrameSize);

		if (FrameFinished)
			goto Done;
	}

	// Flush the last frames
	if (CodecContext->has_b_frames)
		 avcodec_decode_video(CodecContext, AFrame, &FrameFinished, NULL, 0);

	if (!FrameFinished)
		goto Error;

Error:
Done:
	return 0;
}

AVFrameLite *MatroskaVideoSource::GetFrame(int n, char *ErrorMsg, unsigned MsgSize) {
	// PPFrame always holds frame LastFrameNum even if no PP is applied
	if (LastFrameNum == n)
		return reinterpret_cast<AVFrameLite *>(PPFrame);

	bool HasSeeked = false;

	if (n < CurrentFrame || Frames.FindClosestKeyFrame(n) > CurrentFrame) {
		mkv_Seek(MF, Frames[n].DTS, MKVF_SEEK_TO_PREV_KEYFRAME);
		avcodec_flush_buffers(CodecContext);
		HasSeeked = true;
	}

	do {
		int64_t StartTime;
		if (DecodeNextFrame(DecodeFrame, &StartTime, ErrorMsg, MsgSize))
				return NULL;

		if (HasSeeked) {
			HasSeeked = false;

			if (StartTime < 0 || (CurrentFrame = Frames.FrameFromDTS(StartTime)) < 0) {
				_snprintf(ErrorMsg, MsgSize, "Frame accurate seeking is not possible in this file");
				return NULL;
			}
		}

		CurrentFrame++;
	} while (CurrentFrame <= n);

	LastFrameNum = n;
	return OutputFrame(DecodeFrame);
}
