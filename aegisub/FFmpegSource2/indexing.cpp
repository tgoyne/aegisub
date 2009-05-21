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

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <memory>
#include <errno.h>
#include "indexing.h"
#include "wave64writer.h"

#ifdef __UNIX__
#define _fseeki64 fseeko
#define _ftelli64 ftello
#define _snprintf snprintf
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include "MatroskaParser.h"
#include "stdiostream.h"
}

#ifdef _WIN32
#	define _WIN32_DCOM
#	include <windows.h>
#	include <tchar.h>
#	include <atlbase.h>
#	include <dshow.h>
#	include "CoParser.h"
#	include <initguid.h>
#	include "guids.h"
#endif

class MatroskaAudioContext {
public:
	Wave64Writer *W64W;
	AVCodecContext *CTX;
	CompressedStream *CS;
	int64_t CurrentSample;
	uint8_t *CodecPrivate;

	MatroskaAudioContext() {
		W64W = NULL;
		CTX = NULL;
		CS = NULL;
		CurrentSample = 0;
		CodecPrivate = NULL;
	}

	~MatroskaAudioContext() {
		delete[] CodecPrivate;
		delete W64W;
		if (CTX) {
			avcodec_close(CTX);
			av_free(CTX);
		}
		if (CS)
			cs_Destroy(CS);
	}
};

class FFAudioContext {
public:
	Wave64Writer *W64W;
	AVCodecContext *CTX;
	int64_t CurrentSample;

	FFAudioContext() {
		W64W = NULL;
		CTX = NULL;
		CurrentSample = 0;
	}

	~FFAudioContext() {
		delete W64W;
		if (CTX)
			avcodec_close(CTX);
	}
};

#ifdef HAALISOURCE
class HaaliIndexMemory {
private:
	int16_t *DecodingBuffer;
	MatroskaAudioContext *AudioContexts;
public:
	HaaliIndexMemory(int Tracks, int16_t *&DecodingBuffer, MatroskaAudioContext *&AudioContexts) {
		DecodingBuffer = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE*10];
		AudioContexts = new MatroskaAudioContext[Tracks];
		this->DecodingBuffer = DecodingBuffer;
		this->AudioContexts = AudioContexts;
	}

	~HaaliIndexMemory() {
		delete [] DecodingBuffer;
		delete [] AudioContexts;
	}
};
#endif

class MatroskaIndexMemory {
private:
	int16_t *DecodingBuffer;
	MatroskaAudioContext *AudioContexts;
	MatroskaFile *MF;
	MatroskaReaderContext *MC;
public:
	MatroskaIndexMemory(int Tracks, int16_t *&DecodingBuffer, MatroskaAudioContext *&AudioContexts, MatroskaFile *MF, MatroskaReaderContext *MC) {
		DecodingBuffer = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE*10];
		AudioContexts = new MatroskaAudioContext[Tracks];
		this->DecodingBuffer = DecodingBuffer;
		this->AudioContexts = AudioContexts;
		this->MF = MF;
		this->MC = MC;
	}

	~MatroskaIndexMemory() {
		delete [] DecodingBuffer;
		delete [] AudioContexts;
		mkv_Close(MF);
		fclose(MC->ST.fp);
	}
};

class FFIndexMemory {
private:
	int16_t *DecodingBuffer;
	FFAudioContext *AudioContexts;
	AVFormatContext *FormatContext;
public:
	FFIndexMemory(int Tracks, int16_t *&DecodingBuffer, FFAudioContext *&AudioContexts, AVFormatContext *&FormatContext) {
		DecodingBuffer = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE*10];
		AudioContexts = new FFAudioContext[Tracks];
		this->DecodingBuffer = DecodingBuffer;
		this->AudioContexts = AudioContexts;
		this->FormatContext = FormatContext;
	}

	~FFIndexMemory() {
		delete [] DecodingBuffer;
		delete [] AudioContexts;
		av_close_input_file(FormatContext);
	}
};

static bool DTSComparison(TFrameInfo FI1, TFrameInfo FI2) {
	return FI1.DTS < FI2.DTS;
}

static void SortTrackIndices(FFIndex *Index) {
	for (FFIndex::iterator Cur=Index->begin(); Cur!=Index->end(); Cur++)
		std::sort(Cur->begin(), Cur->end(), DTSComparison);
}

#ifdef HAALISOURCE
FFHaaliIndexer::FFHaaliIndexer(const char *SourceFile, int SourceMode, char *ErrorMsg, unsigned MsgSize) {
	this->SourceMode = SourceMode;
	memset(TrackType, FFMS_TYPE_UNKNOWN, sizeof(TrackType));
	memset(Codec, 0, sizeof(Codec));
	memset(CodecPrivate, 0, sizeof(CodecPrivate));
	memset(CodecPrivateSize, 0, sizeof(CodecPrivateSize));
	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	CLSID clsid = HAALI_TS_Parser;
	if (SourceMode == 1)
		clsid = HAALI_OGM_Parser;

	if (FAILED(pMMC.CoCreateInstance(clsid))) {
		_snprintf(ErrorMsg, MsgSize, "Can't create parser");
		throw ErrorMsg;
	}

	CComPtr<IMemAlloc> pMA;
	if (FAILED(pMA.CoCreateInstance(CLSID_MemAlloc))) {
		_snprintf(ErrorMsg, MsgSize, "Can't create memory allocator");
		throw ErrorMsg;
	}

	CComPtr<IMMStream> pMS;
	if (FAILED(pMS.CoCreateInstance(CLSID_DiskFile))) {
		_snprintf(ErrorMsg, MsgSize, "Can't create disk file reader");
		throw ErrorMsg;
	}

	WCHAR WSourceFile[2048];
	mbstowcs(WSourceFile, SourceFile, 2000);
	CComQIPtr<IMMStreamOpen> pMSO(pMS);
	if (FAILED(pMSO->Open(WSourceFile))) {
		_snprintf(ErrorMsg, MsgSize, "Can't open file");
		throw ErrorMsg;
	}

	if (FAILED(pMMC->Open(pMS, 0, NULL, pMA))) {
		_snprintf(ErrorMsg, MsgSize, "Can't parse file");
		throw ErrorMsg;
	}

	NumTracks = 0;
	CComPtr<IEnumUnknown> pEU;
	if (SUCCEEDED(pMMC->EnumTracks(&pEU))) {
		CComPtr<IUnknown> pU;
		while (pEU->Next(1, &pU, NULL) == S_OK) {
			CComQIPtr<IPropertyBag> pBag = pU;

			if (pBag) {
				CComVariant pV;

				pV.Clear();
				if (SUCCEEDED(pBag->Read(L"Type", &pV, NULL)) && SUCCEEDED(pV.ChangeType(VT_UI4)))
					TrackType[NumTracks] = HaaliTrackTypeToFFTrackType(pV.uintVal);

				pV.Clear();
				if (SUCCEEDED(pBag->Read(L"CodecPrivate", &pV, NULL))) {
					CodecPrivateSize[NumTracks] = vtSize(pV);
					CodecPrivate[NumTracks] = new uint8_t[CodecPrivateSize[NumTracks]];
					vtCopy(pV, CodecPrivate[NumTracks]);
				}

				pV.Clear();
				if (SUCCEEDED(pBag->Read(L"CodecID", &pV, NULL)) && SUCCEEDED(pV.ChangeType(VT_BSTR))) {
					char CodecID[2048];
					wcstombs(CodecID, pV.bstrVal, 2000);
					Codec[NumTracks] = avcodec_find_decoder(MatroskaToFFCodecID(CodecID, CodecPrivate[NumTracks]));
				}
			}

			pU = NULL;
			NumTracks++;
		}
	}
}

FFIndex *FFHaaliIndexer::DoIndexing(const char *AudioFile, char *ErrorMsg, unsigned MsgSize) {
	// Audio stuff
	int16_t *db;
	MatroskaAudioContext *AudioContexts;
	HaaliIndexMemory IM = HaaliIndexMemory(NumTracks, db, AudioContexts);

	FFIndex *TrackIndices = new FFIndex();
	TrackIndices->Decoder = 2;
	if (SourceMode == 1)
		TrackIndices->Decoder = 3;


	for (int i = 0; i < NumTracks; i++) {
		TrackIndices->push_back(FFTrack(1, 1000000000, TrackType[i]));

		if (IndexMask & (1 << i) && TrackType[i] == FFMS_TYPE_AUDIO) {
			AVCodecContext *AudioCodecContext = avcodec_alloc_context();
			AudioCodecContext->extradata = CodecPrivate[i];
			AudioCodecContext->extradata_size = CodecPrivateSize[i];
			AudioContexts[i].CTX = AudioCodecContext;

			if (Codec[i] == NULL) {
				av_free(AudioCodecContext);
				AudioContexts[i].CTX = NULL;
				_snprintf(ErrorMsg, MsgSize, "Audio codec not found");
				return NULL;
			}

			if (avcodec_open(AudioCodecContext, Codec[i]) < 0) {
				av_free(AudioCodecContext);
				AudioContexts[i].CTX = NULL;
				_snprintf(ErrorMsg, MsgSize, "Could not open audio codec");
				return NULL;
			}
		} else {
			IndexMask &= ~(1 << i);
		}
	}
//

	AVPacket TempPacket;
	InitNullPacket(&TempPacket);

	for (;;) {
		if (IC) {
			if ((*IC)(0, 1, Private)) {
				_snprintf(ErrorMsg, MsgSize, "Cancelled by user");
				delete TrackIndices;
				return NULL;
			}
		}

		CComPtr<IMMFrame> pMMF;
		if (pMMC->ReadFrame(NULL, &pMMF) != S_OK)
			break;

		REFERENCE_TIME Ts, Te;
		HRESULT hr = pMMF->GetTime(&Ts, &Te);

		unsigned int CurrentTrack = pMMF->GetTrack();

		// Only create index entries for video for now to save space
		if (TrackType[CurrentTrack] == FFMS_TYPE_VIDEO) {
			(*TrackIndices)[CurrentTrack].push_back(TFrameInfo(Ts, pMMF->IsSyncPoint() == S_OK));
		} else if (TrackType[CurrentTrack] == FFMS_TYPE_AUDIO && (IndexMask & (1 << CurrentTrack))) {
			(*TrackIndices)[CurrentTrack].push_back(TFrameInfo(Ts, AudioContexts[CurrentTrack].CurrentSample, 0 /* FIXME? */, pMMF->GetActualDataLength(), pMMF->IsSyncPoint() == S_OK));
			AVCodecContext *AudioCodecContext = AudioContexts[CurrentTrack].CTX;
			pMMF->GetPointer(&TempPacket.data);
			TempPacket.size = pMMF->GetActualDataLength();

			while (TempPacket.size > 0) {
				int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
				int Ret = avcodec_decode_audio3(AudioCodecContext, db, &dbsize, &TempPacket);
				if (Ret < 0) {
					if (IgnoreDecodeErrors) {
						(*TrackIndices)[CurrentTrack].clear();
						IndexMask &= ~(1 << CurrentTrack);					
						break;
					} else {
						_snprintf(ErrorMsg, MsgSize, "Audio decoding error");
						delete TrackIndices;
						return NULL;
					}
				}

				if (Ret > 0) {
					TempPacket.size -= Ret;
					TempPacket.data += Ret;
				}

				if (dbsize > 0)
					AudioContexts[CurrentTrack].CurrentSample += (dbsize * 8) / (av_get_bits_per_sample_format(AudioCodecContext->sample_fmt) * AudioCodecContext->channels);

				if (dbsize > 0 && (DumpMask & (1 << CurrentTrack))) {
					// Delay writer creation until after an audio frame has been decoded. This ensures that all parameters are known when writing the headers.
					if (!AudioContexts[CurrentTrack].W64W) {
						AVCodecContext *CTX = AudioContexts[CurrentTrack].CTX;
						char ABuf[50];
						std::string WN(AudioFile);
						_snprintf(ABuf, sizeof(ABuf), ".%02d.delay.%d.w64", CurrentTrack, 0);
						WN += ABuf;
						
						AudioContexts[CurrentTrack].W64W = new Wave64Writer(WN.c_str(), av_get_bits_per_sample_format(AudioCodecContext->sample_fmt),
							AudioCodecContext->channels, AudioCodecContext->sample_rate, AudioFMTIsFloat(AudioCodecContext->sample_fmt));
					}

					AudioContexts[CurrentTrack].W64W->WriteData(db, dbsize);
				}
			}
		}
	}

	SortTrackIndices(TrackIndices);
	return TrackIndices;
}
#endif

FFMatroskaIndexer::FFMatroskaIndexer(const char *SourceFile, char *ErrorMsg, unsigned MsgSize) {
	char ErrorMessage[256];

	InitStdIoStream(&MC.ST);
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
}

FFIndex *FFMatroskaIndexer::DoIndexing(const char *AudioFile, char *ErrorMsg, unsigned MsgSize) {
	char ErrorMessage[256];

	// Audio stuff
	int16_t *db;
	MatroskaAudioContext *AudioContexts;
	MatroskaIndexMemory IM = MatroskaIndexMemory(mkv_GetNumTracks(MF), db, AudioContexts, MF, &MC);

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++) {
		TrackInfo *TI = mkv_GetTrackInfo(MF, i);
		if (IndexMask & (1 << i) && TI->Type == TT_AUDIO) {
			AVCodecContext *AudioCodecContext = avcodec_alloc_context();
			AudioCodecContext->extradata = (uint8_t *)TI->CodecPrivate;
			AudioCodecContext->extradata_size = TI->CodecPrivateSize;
			AudioContexts[i].CTX = AudioCodecContext;

			if (TI->CompEnabled) {
				AudioContexts[i].CS = cs_Create(MF, i, ErrorMessage, sizeof(ErrorMessage));
				if (AudioContexts[i].CS == NULL) {
					av_free(AudioCodecContext);
					AudioContexts[i].CTX = NULL;
					_snprintf(ErrorMsg, MsgSize, "Can't create decompressor: %s", ErrorMessage);
					return NULL;
				}
			}

			AVCodec *AudioCodec = avcodec_find_decoder(MatroskaToFFCodecID(TI->CodecID, TI->CodecPrivate));
			if (AudioCodec == NULL) {
				av_free(AudioCodecContext);
				AudioContexts[i].CTX = NULL;
				_snprintf(ErrorMsg, MsgSize, "Audio codec not found");
				return NULL;
			}

			if (avcodec_open(AudioCodecContext, AudioCodec) < 0) {
				av_free(AudioCodecContext);
				AudioContexts[i].CTX = NULL;
				_snprintf(ErrorMsg, MsgSize, "Could not open audio codec");
				return NULL;
			}
		} else {
			IndexMask &= ~(1 << i);
		}
	}

	//

	int64_t CurrentPos = _ftelli64(MC.ST.fp);
	_fseeki64(MC.ST.fp, 0, SEEK_END);
	int64_t SourceSize = _ftelli64(MC.ST.fp);
	_fseeki64(MC.ST.fp, CurrentPos, SEEK_SET);

	FFIndex *TrackIndices = new FFIndex();
	TrackIndices->Decoder = 1;

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++)
		TrackIndices->push_back(FFTrack(mkv_TruncFloat(mkv_GetTrackInfo(MF, i)->TimecodeScale), 1000000, HaaliTrackTypeToFFTrackType(mkv_GetTrackInfo(MF, i)->Type)));

	ulonglong StartTime, EndTime, FilePos;
	unsigned int Track, FrameFlags, FrameSize;
	AVPacket TempPacket;
	InitNullPacket(&TempPacket);

	while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0) {
		// Update progress
		if (IC) {
			if ((*IC)(_ftelli64(MC.ST.fp), SourceSize, Private)) {
				_snprintf(ErrorMsg, MsgSize, "Cancelled by user");
				delete TrackIndices;
				return NULL;
			}
		}
	
		// Only create index entries for video for now to save space
		if (mkv_GetTrackInfo(MF, Track)->Type == TT_VIDEO) {
			(*TrackIndices)[Track].push_back(TFrameInfo(StartTime, FilePos, FrameSize, (FrameFlags & FRAME_KF) != 0));
		} else if (mkv_GetTrackInfo(MF, Track)->Type == TT_AUDIO && (IndexMask & (1 << Track))) {
			(*TrackIndices)[Track].push_back(TFrameInfo(StartTime, AudioContexts[Track].CurrentSample, FilePos, FrameSize, (FrameFlags & FRAME_KF) != 0));
			ReadFrame(FilePos, FrameSize, AudioContexts[Track].CS, MC, ErrorMsg, MsgSize);
			AVCodecContext *AudioCodecContext = AudioContexts[Track].CTX;
			TempPacket.data = MC.Buffer;
			TempPacket.size = FrameSize;

			while (TempPacket.size > 0) {
				int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
				int Ret = avcodec_decode_audio3(AudioCodecContext, db, &dbsize, &TempPacket);
				if (Ret < 0) {
					if (IgnoreDecodeErrors) {
						(*TrackIndices)[Track].clear();
						IndexMask &= ~(1 << Track);					
						break;
					} else {
						_snprintf(ErrorMsg, MsgSize, "Audio decoding error");
						delete TrackIndices;
						return NULL;
					}
				}

				if (Ret > 0) {
					TempPacket.size -= Ret;
					TempPacket.data += Ret;
				}

				if (dbsize > 0)
					AudioContexts[Track].CurrentSample += (dbsize * 8) / (av_get_bits_per_sample_format(AudioCodecContext->sample_fmt) * AudioCodecContext->channels);

				if (dbsize > 0 && (DumpMask & (1 << Track))) {
					// Delay writer creation until after an audio frame has been decoded. This ensures that all parameters are known when writing the headers.
					if (!AudioContexts[Track].W64W) {
						char ABuf[50];
						std::string WN(AudioFile);
						int Offset = StartTime * mkv_TruncFloat(mkv_GetTrackInfo(MF, Track)->TimecodeScale) / (double)1000000;
						_snprintf(ABuf, sizeof(ABuf), ".%02d.delay.%d.w64", Track, Offset);
						WN += ABuf;
						
						AudioContexts[Track].W64W = new Wave64Writer(WN.c_str(), av_get_bits_per_sample_format(AudioCodecContext->sample_fmt),
							AudioCodecContext->channels, AudioCodecContext->sample_rate, AudioFMTIsFloat(AudioCodecContext->sample_fmt));
					}

					AudioContexts[Track].W64W->WriteData(db, dbsize);
				}
			}
		}
	}

	SortTrackIndices(TrackIndices);
	return TrackIndices;
}

FFIndexer *FFIndexer::CreateFFIndexer(const char *Filename, char *ErrorMsg, unsigned MsgSize) {
	AVFormatContext *FormatContext = NULL;

	if (av_open_input_file(&FormatContext, Filename, NULL, 0, NULL) != 0) {
		_snprintf(ErrorMsg, MsgSize, "Can't open '%s'", Filename);
		return NULL;
	}

	// Do matroska indexing instead?
	if (!strcmp(FormatContext->iformat->name, "matroska")) {
		av_close_input_file(FormatContext);
		return new FFMatroskaIndexer(Filename, ErrorMsg, MsgSize);
	}

#ifdef HAALISOURCE
	// Do haali ts indexing instead?
	if (!strcmp(FormatContext->iformat->name, "mpeg") || !strcmp(FormatContext->iformat->name, "mpegts")) {
		av_close_input_file(FormatContext);
		return new FFHaaliIndexer(Filename, 0, ErrorMsg, MsgSize);
	}

	if (!strcmp(FormatContext->iformat->name, "ogg")) {
		av_close_input_file(FormatContext);
		return new FFHaaliIndexer(Filename, 1, ErrorMsg, MsgSize);
	}
#endif

	return new FFLAVFIndexer(FormatContext, ErrorMsg, MsgSize);
}

FFLAVFIndexer::FFLAVFIndexer(AVFormatContext *FormatContext, char *ErrorMsg, unsigned MsgSize) {
	IsIndexing = false;
	this->FormatContext = FormatContext;
	
	if (av_find_stream_info(FormatContext) < 0) {
		av_close_input_file(FormatContext);
		_snprintf(ErrorMsg, MsgSize, "Couldn't find stream information");
		throw ErrorMsg;
	}
}

FFLAVFIndexer::~FFLAVFIndexer() {
	if (!IsIndexing)
		av_close_input_file(FormatContext);
}

FFIndex *FFLAVFIndexer::DoIndexing(const char *AudioFile, char *ErrorMsg, unsigned MsgSize) {
	IsIndexing = true;

	// Audio stuff
	int16_t *db;
	FFAudioContext *AudioContexts;
	FFIndexMemory IM = FFIndexMemory(FormatContext->nb_streams, db, AudioContexts, FormatContext);

	for (unsigned int i = 0; i < FormatContext->nb_streams; i++) {
		if (IndexMask & (1 << i) && FormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
			AVCodecContext *AudioCodecContext = FormatContext->streams[i]->codec;

			AVCodec *AudioCodec = avcodec_find_decoder(AudioCodecContext->codec_id);
			if (AudioCodec == NULL) {
				_snprintf(ErrorMsg, MsgSize, "Audio codec not found");
				return NULL;
			}

			if (avcodec_open(AudioCodecContext, AudioCodec) < 0) {
				_snprintf(ErrorMsg, MsgSize, "Could not open audio codec");
				return NULL;
			}

			AudioContexts[i].CTX = AudioCodecContext;
		} else {
			IndexMask &= ~(1 << i);
		}
	}

	//

	FFIndex *TrackIndices = new FFIndex();
	TrackIndices->Decoder = 0;

	for (unsigned int i = 0; i < FormatContext->nb_streams; i++)
		TrackIndices->push_back(FFTrack((int64_t)FormatContext->streams[i]->time_base.num * 1000, 
		FormatContext->streams[i]->time_base.den,
		static_cast<FFMS_TrackType>(FormatContext->streams[i]->codec->codec_type)));

	AVPacket Packet, TempPacket;
	InitNullPacket(&Packet);
	InitNullPacket(&TempPacket);
	while (av_read_frame(FormatContext, &Packet) >= 0) {
		// Update progress
		if (IC) {
			if ((*IC)(FormatContext->pb->pos, FormatContext->file_size, Private)) {
				_snprintf(ErrorMsg, MsgSize, "Cancelled by user");
				delete TrackIndices;
				return NULL;
			}
		}

		// Only create index entries for video for now to save space
		if (FormatContext->streams[Packet.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO) {
			(*TrackIndices)[Packet.stream_index].push_back(TFrameInfo(Packet.dts, (Packet.flags & PKT_FLAG_KEY) ? 1 : 0));
		} else if (FormatContext->streams[Packet.stream_index]->codec->codec_type == CODEC_TYPE_AUDIO && (IndexMask & (1 << Packet.stream_index))) {
			(*TrackIndices)[Packet.stream_index].push_back(TFrameInfo(Packet.dts, AudioContexts[Packet.stream_index].CurrentSample, (Packet.flags & PKT_FLAG_KEY) ? 1 : 0));
			AVCodecContext *AudioCodecContext = FormatContext->streams[Packet.stream_index]->codec;
			TempPacket.data = Packet.data;
			TempPacket.size = Packet.size;

			while (TempPacket.size > 0) {
				int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
				int Ret = avcodec_decode_audio3(AudioCodecContext, db, &dbsize, &TempPacket);
				if (Ret < 0) {
					if (IgnoreDecodeErrors) {
						(*TrackIndices)[Packet.stream_index].clear();
						IndexMask &= ~(1 << Packet.stream_index);					
						break;
					} else {
						_snprintf(ErrorMsg, MsgSize, "Audio decoding error");
						delete TrackIndices;
						return NULL;
					}
				}

				if (Ret > 0) {
					TempPacket.size -= Ret;
					TempPacket.data += Ret;
				}

				if (dbsize > 0)
					AudioContexts[Packet.stream_index].CurrentSample += (dbsize * 8) / (av_get_bits_per_sample_format(AudioCodecContext->sample_fmt) * AudioCodecContext->channels);

				if (dbsize > 0 && (DumpMask & (1 << Packet.stream_index))) {
					// Delay writer creation until after an audio frame has been decoded. This ensures that all parameters are known when writing the headers.
					if (!AudioContexts[Packet.stream_index].W64W) {
						char ABuf[50];
						std::string WN(AudioFile);
						int Offset = (Packet.dts * FormatContext->streams[Packet.stream_index]->time_base.num)
							/ (double)(FormatContext->streams[Packet.stream_index]->time_base.den * 1000);
						_snprintf(ABuf, sizeof(ABuf), ".%02d.delay.%d.w64", Packet.stream_index, Offset);
						WN += ABuf;
						
						AudioContexts[Packet.stream_index].W64W = new Wave64Writer(WN.c_str(), av_get_bits_per_sample_format(AudioCodecContext->sample_fmt),
							AudioCodecContext->channels, AudioCodecContext->sample_rate, AudioFMTIsFloat(AudioCodecContext->sample_fmt));
					}

					AudioContexts[Packet.stream_index].W64W->WriteData(db, dbsize);
				}
			}
		}

		av_free_packet(&Packet);
	}

	SortTrackIndices(TrackIndices);
	return TrackIndices;
}
