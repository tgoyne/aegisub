//  Copyright (c) 2007-2009 Fredrik Mellbin
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

#include "videosource.h"



void FFLAVFVideo::Free(bool CloseCodec) {
	if (CloseCodec)
		avcodec_close(CodecContext);
	av_close_input_file(FormatContext);
}

FFLAVFVideo::FFLAVFVideo(const char *SourceFile, int Track, FFMS_Index *Index,
	int Threads, int SeekMode)
	: Res(FFSourceResources<FFMS_VideoSource>(this)), FFMS_VideoSource(SourceFile, Index, Track) {

	FormatContext = NULL;
	AVCodec *Codec = NULL;
	this->SeekMode = SeekMode;
	VideoTrack = Track;
	Frames = (*Index)[VideoTrack];

	LAVFOpenFile(SourceFile, FormatContext);

	if (SeekMode >= 0 && av_seek_frame(FormatContext, VideoTrack, Frames[0].PTS, AVSEEK_FLAG_BACKWARD) < 0)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Video track is unseekable");

	CodecContext = FormatContext->streams[VideoTrack]->codec;
	if (avcodec_thread_init(CodecContext, Threads))
		CodecContext->thread_count = 1;

	Codec = avcodec_find_decoder(CodecContext->codec_id);
	if (Codec == NULL)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Video codec not found");

	if (avcodec_open(CodecContext, Codec) < 0)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Could not open video codec");

	Res.CloseCodec(true);

	// Always try to decode a frame to make sure all required parameters are known
	int64_t Dummy;
	DecodeNextFrame(&Dummy);

	//VP.image_type = VideoInfo::IT_TFF;
	VP.FPSDenominator = FormatContext->streams[VideoTrack]->time_base.num;
	VP.FPSNumerator = FormatContext->streams[VideoTrack]->time_base.den;
	VP.RFFDenominator = CodecContext->time_base.num;
	VP.RFFNumerator = CodecContext->time_base.den;
	if (CodecContext->codec_id == CODEC_ID_H264) {
		if (VP.RFFNumerator & 1)
			VP.RFFDenominator *= 2;
		else
			VP.RFFNumerator /= 2;
	}
	VP.NumFrames = Frames.size();
	VP.TopFieldFirst = DecodeFrame->top_field_first;
#ifdef FFMS_HAVE_FFMPEG_COLORSPACE_INFO
	VP.ColorSpace = CodecContext->colorspace;
	VP.ColorRange = CodecContext->color_range;
#else
	VP.ColorSpace = 0;
	VP.ColorRange = 0;
#endif
	// these pixfmt's are deprecated but still used
	if (
		CodecContext->pix_fmt == PIX_FMT_YUVJ420P 
		|| CodecContext->pix_fmt == PIX_FMT_YUVJ422P
		|| CodecContext->pix_fmt == PIX_FMT_YUVJ444P
	)
		VP.ColorRange = AVCOL_RANGE_JPEG;


	VP.FirstTime = ((Frames.front().PTS * Frames.TB.Num) / (double)Frames.TB.Den) / 1000;
	VP.LastTime = ((Frames.back().PTS * Frames.TB.Num) / (double)Frames.TB.Den) / 1000;

	if (CodecContext->width <= 0 || CodecContext->height <= 0)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Codec returned zero size video");

	// sanity check framerate
	if (VP.FPSDenominator > VP.FPSNumerator || VP.FPSDenominator <= 0 || VP.FPSNumerator <= 0) {
		VP.FPSDenominator = 1;
		VP.FPSNumerator = 30;
	}

	// Calculate the average framerate
	if (Frames.size() >= 2) {
		double PTSDiff = (double)(Frames.back().PTS - Frames.front().PTS);
		double TD = (double)(Frames.TB.Den);
		double TN = (double)(Frames.TB.Num);
		VP.FPSDenominator = (unsigned int)(((double)1000000) / (double)((VP.NumFrames - 1) / ((PTSDiff * TN/TD) / (double)1000)));
		VP.FPSNumerator = 1000000; 
	}

	// attempt to correct framerate to the proper NTSC fraction, if applicable
	CorrectNTSCRationalFramerate(&VP.FPSNumerator, &VP.FPSDenominator);
	// correct the timebase, if necessary
	CorrectTimebase(&VP, &Frames.TB);

	// Cannot "output" to PPFrame without doing all other initialization
	// This is the additional mess required for seekmode=-1 to work in a reasonable way
	OutputFrame(DecodeFrame);

	// Set AR variables
	VP.SARNum = CodecContext->sample_aspect_ratio.num;
	VP.SARDen = CodecContext->sample_aspect_ratio.den;
}

void FFLAVFVideo::DecodeNextFrame(int64_t *AStartTime) {
	AVPacket Packet;
	InitNullPacket(Packet);
	int FrameFinished = 0;
	*AStartTime = -1;

	if (InitialDecode == -1) {
		if (DelayCounter > CodecContext->has_b_frames) {
			DelayCounter--;
			goto Done;
		} else {
			InitialDecode = 0;
		}
	}

	while (av_read_frame(FormatContext, &Packet) >= 0) {
        if (Packet.stream_index == VideoTrack) {
			if (*AStartTime < 0) {
				if (Frames.UseDTS)
					*AStartTime = Packet.dts;
				else
					*AStartTime = Packet.pts;
			}

			avcodec_decode_video2(CodecContext, DecodeFrame, &FrameFinished, &Packet);

			if (!FrameFinished)
				DelayCounter++;
			if (DelayCounter > CodecContext->has_b_frames && !InitialDecode) {
				av_free_packet(&Packet);
				goto Done;
			}
        }

        av_free_packet(&Packet);

		if (FrameFinished)
			goto Done;
	}

	// Flush the last frames
	if (CodecContext->has_b_frames) {
		AVPacket NullPacket;
		InitNullPacket(NullPacket);
		avcodec_decode_video2(CodecContext, DecodeFrame, &FrameFinished, &NullPacket);
	}

	if (!FrameFinished)
		goto Error;

Error:
Done:
	if (InitialDecode == 1) InitialDecode = -1;
}

FFMS_Frame *FFLAVFVideo::GetFrame(int n) {
	GetFrameCheck(n);

	if (LastFrameNum == n)
		return &LocalFrame;

	bool HasSeeked = false;
	int SeekOffset = 0;

	int ClosestKF = 0;
	if (SeekMode >= 0) {
		ClosestKF = Frames.FindClosestVideoKeyFrame(n);

		if (SeekMode == 0) {
			if (n < CurrentFrame) {
				av_seek_frame(FormatContext, VideoTrack, Frames[0].PTS, AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(CodecContext);
				CurrentFrame = 0;
				DelayCounter = 0;
				InitialDecode = 1;
			}
		} else {
			// 10 frames is used as a margin to prevent excessive seeking since the predicted best keyframe isn't always selected by avformat
			if (n < CurrentFrame || ClosestKF > CurrentFrame + 10 || (SeekMode == 3 && n > CurrentFrame + 10)) {
ReSeek:
				av_seek_frame(FormatContext, VideoTrack,
					(SeekMode == 3) ? Frames[n].PTS : Frames[ClosestKF + SeekOffset].PTS,
					AVSEEK_FLAG_BACKWARD);
				avcodec_flush_buffers(CodecContext);
				HasSeeked = true;
				DelayCounter = 0;
				InitialDecode = 1;
			}
		}
	} else if (n < CurrentFrame) {
		throw FFMS_Exception(FFMS_ERROR_SEEKING, FFMS_ERROR_INVALID_ARGUMENT,
			"Non-linear access attempted");
	}

	do {
		int64_t StartTime;
		if (CurrentFrame+CodecContext->has_b_frames >= n)
			CodecContext->skip_frame = AVDISCARD_DEFAULT;
		else
			CodecContext->skip_frame = AVDISCARD_NONREF;
		DecodeNextFrame(&StartTime);

		if (HasSeeked) {
			HasSeeked = false;

			// Is the seek destination time known? Does it belong to a frame?
			if (StartTime < 0 || (CurrentFrame = Frames.FrameFromPTS(StartTime)) < 0) {
				switch (SeekMode) {
					case 1:
						// No idea where we are so go back a bit further
						if (ClosestKF + SeekOffset == 0)
							throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
								"Frame accurate seeking is not possible in this file");


						SeekOffset -= FFMIN(10, ClosestKF + SeekOffset);
						goto ReSeek;
					case 2:
					case 3:
						CurrentFrame = Frames.ClosestFrameFromPTS(StartTime);
						break;
					default:
						throw FFMS_Exception(FFMS_ERROR_SEEKING, FFMS_ERROR_UNKNOWN,
							"Failed assertion");
				}	
			}
		}

		CurrentFrame++;
	} while (CurrentFrame <= n);

	LastFrameNum = n;
	return OutputFrame(DecodeFrame);
}
