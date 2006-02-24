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
// -----------------------------------------------------------------------------
//
// AEGISUB
//
// Website: http://aegisub.cellosoft.com
// Contact: mailto:zeratul@cellosoft.com
//

#pragma once


///////////
// Headers
#ifdef USE_LAVC
#define EMULATE_INTTYPES
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>
#include "video_provider.h"


///////////////////////
// LibAVCodec provider
class LAVCVideoProvider : public VideoProvider {
private:
	AVFormatContext *formatContext;
	AVCodecContext *codecContext;
	AVStream *stream;
	AVCodec *codec;
	AVFrame *frame;
	int vidStream;

	__int64 lastDecodeTime;
	int frameNumber;
	wxBitmap curFrame;

	uint8_t *buffer;
	int bufferSize;

	void GetNextFrame();
	void LoadVideo(wxString filename);
	void Close();
	wxBitmap AVFrameToWX(AVFrame *frame);

public:
	LAVCVideoProvider(wxString filename, wxString subfilename, double zoom);
	~LAVCVideoProvider();

	void RefreshSubtitles();

	wxBitmap GetFrame(int n);
	void GetFloatFrame(float* Buffer, int n);

	int GetPosition();
	int GetFrameCount();
	double GetFPS();

	void SetDAR(double dar);
	void SetZoom(double zoom);
	int GetWidth();
	int GetHeight();
	double GetZoom();

	int GetSourceWidth();
	int GetSourceHeight();
};

#endif
