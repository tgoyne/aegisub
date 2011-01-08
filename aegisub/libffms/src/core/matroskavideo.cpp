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



void FFMatroskaVideo::Free(bool CloseCodec) {
	if (TCC)
		delete TCC;
	if (MC.ST.fp) {
		mkv_Close(MF);
	}
	if (CloseCodec)
		avcodec_close(CodecContext);
	av_freep(&CodecContext);
}

FFMatroskaVideo::FFMatroskaVideo(const char *SourceFile, int Track,
	FFMS_Index *Index, int Threads)
	: Res(FFSourceResources<FFMS_VideoSource>(this)), FFMS_VideoSource(SourceFile, Index, Track) {

	AVCodec *Codec = NULL;
	CodecContext = NULL;
	TrackInfo *TI = NULL;
	TCC = NULL;
	PacketNumber = 0;
	VideoTrack = Track;
	Frames = (*Index)[VideoTrack];

	MC.ST.fp = ffms_fopen(SourceFile, "rb");
	if (MC.ST.fp == NULL) {
		std::ostringstream buf;
		buf << "Can't open '" << SourceFile << "': " << strerror(errno);
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
	}

	setvbuf(MC.ST.fp, NULL, _IOFBF, CACHESIZE);

	MF = mkv_OpenEx(&MC.ST.base, 0, 0, ErrorMessage, sizeof(ErrorMessage));
	if (MF == NULL) {
		std::ostringstream buf;
		buf << "Can't parse Matroska file: " << ErrorMessage;
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
	}

	TI = mkv_GetTrackInfo(MF, VideoTrack);

	if (TI->CompEnabled)
		TCC = new TrackCompressionContext(MF, TI, VideoTrack);

	CodecContext = avcodec_alloc_context();
	if (avcodec_thread_init(CodecContext, Threads))
		CodecContext->thread_count = 1;

	Codec = avcodec_find_decoder(MatroskaToFFCodecID(TI->CodecID, TI->CodecPrivate));
	if (Codec == NULL)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Video codec not found");

	InitializeCodecContextFromMatroskaTrackInfo(TI, CodecContext);

	if (avcodec_open(CodecContext, Codec) < 0)
		throw FFMS_Exception(FFMS_ERROR_DECODING, FFMS_ERROR_CODEC,
			"Could not open video codec");

	Res.CloseCodec(true);

	// Always try to decode a frame to make sure all required parameters are known
	DecodeNextFrame();

	VP.FPSDenominator = 1;
	VP.FPSNumerator = 30;
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

	// Calculate the average framerate
	if (Frames.size() >= 2) {
		double PTSDiff = (double)(Frames.back().PTS - Frames.front().PTS);
		VP.FPSDenominator = (unsigned int)(PTSDiff * mkv_TruncFloat(TI->TimecodeScale) / (double)1000 / (double)(VP.NumFrames - 1) + 0.5);
		VP.FPSNumerator = 1000000; 
	}

	// attempt to correct framerate to the proper NTSC fraction, if applicable
	CorrectNTSCRationalFramerate(&VP.FPSNumerator, &VP.FPSDenominator);
	// correct the timebase, if necessary
	CorrectTimebase(&VP, &Frames.TB);

	// Output the already decoded frame so it isn't wasted
	OutputFrame(DecodeFrame);

	// Set AR variables
	VP.SARNum = TI->AV.Video.DisplayWidth * TI->AV.Video.PixelHeight;
	VP.SARDen = TI->AV.Video.DisplayHeight * TI->AV.Video.PixelWidth;

	// Set crop variables
	VP.CropLeft = TI->AV.Video.CropL;
	VP.CropRight = TI->AV.Video.CropR;
	VP.CropTop = TI->AV.Video.CropT;
	VP.CropBottom = TI->AV.Video.CropB;
}

void FFMatroskaVideo::DecodeNextFrame() {
	int FrameFinished = 0;
	AVPacket Packet;
	InitNullPacket(Packet);

	unsigned int FrameSize;

	if (InitialDecode == -1) {
		if (DelayCounter > CodecContext->has_b_frames) {
			DelayCounter--;
			goto Done;
		} else {
			InitialDecode = 0;
		}
	}
	
	while (PacketNumber < Frames.size()) {
		// The additional indirection is because the packets are stored in
		// presentation order and not decoding order, this is unnoticable
		// in the other sources where less is done manually
		const TFrameInfo &FI = Frames[Frames[PacketNumber].OriginalPos];
		FrameSize = FI.FrameSize;
		ReadFrame(FI.FilePos, FrameSize, TCC, MC);

		Packet.data = MC.Buffer;
		Packet.size = (TCC && TCC->CompressionMethod == COMP_PREPEND) ? FrameSize + TCC->CompressedPrivateDataSize : FrameSize;
		if (FI.KeyFrame)
			Packet.flags = AV_PKT_FLAG_KEY;
		else
			Packet.flags = 0;

		PacketNumber++;

		avcodec_decode_video2(CodecContext, DecodeFrame, &FrameFinished, &Packet);

		if (!FrameFinished)
			DelayCounter++;
		if (DelayCounter > CodecContext->has_b_frames && !InitialDecode)
			goto Done;

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

FFMS_Frame *FFMatroskaVideo::GetFrame(int n) {
	GetFrameCheck(n);

	if (LastFrameNum == n)
		return &LocalFrame;

	int ClosestKF = Frames.FindClosestVideoKeyFrame(n);
	if (CurrentFrame > n || ClosestKF > CurrentFrame + 10) {
		DelayCounter = 0;
		InitialDecode = 1;
		PacketNumber = ClosestKF;
		CurrentFrame = ClosestKF;
		avcodec_flush_buffers(CodecContext);
	}

	do {
		if (CurrentFrame+CodecContext->has_b_frames >= n)
			CodecContext->skip_frame = AVDISCARD_DEFAULT;
		else
			CodecContext->skip_frame = AVDISCARD_NONREF;
		DecodeNextFrame();
		CurrentFrame++;
	} while (CurrentFrame <= n);

	LastFrameNum = n;
	return OutputFrame(DecodeFrame);
}
