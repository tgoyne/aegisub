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

#include <iostream>
#include <fstream>
#include <set>
#include <algorithm>
#include <memory>
#include <errno.h>
#include "indexing.h"
#include "wave64writer.h"

class AudioContext {
public:
	Wave64Writer *W64W;
	AVCodecContext *CTX;
	CompressedStream *CS;
	int64_t CurrentSample;

	AudioContext() {
		W64W = NULL;
		CTX = NULL;
		CS = NULL;
		CurrentSample = 0;
	}

	~AudioContext() {
		delete W64W;
		if (CTX) {
			avcodec_close(CTX);
			av_free(CTX);
		}
		if (CS)
			cs_Destroy(CS);
	}
};

class IndexMemory {
private:
	int16_t *DecodingBuffer;
	AudioContext *AudioContexts;
public:
	IndexMemory(int Tracks, int16_t *&DecodingBuffer, AudioContext *&AudioContexts) {
		DecodingBuffer = new int16_t[AVCODEC_MAX_AUDIO_FRAME_SIZE*10];
		AudioContexts = new AudioContext[Tracks];
		this->DecodingBuffer = DecodingBuffer;
		this->AudioContexts = AudioContexts;
	}

	~IndexMemory() {
		delete [] DecodingBuffer;
		delete [] AudioContexts;
	}
};

class MatroskaMemory {
private:
	MatroskaFile *MF;
	MatroskaReaderContext *MC;
public:
	MatroskaMemory(MatroskaFile *MF, MatroskaReaderContext *MC) {
		this->MF = MF;
		this->MC = MC;
	}

	~MatroskaMemory() {
		mkv_Close(MF);
		fclose(MC->ST.fp);
	}
};


static bool DTSComparison(FrameInfo FI1, FrameInfo FI2) {
	return FI1.DTS < FI2.DTS;
}

static void SortTrackIndices(FrameIndex *TrackIndices) {
	for (FrameIndex::iterator Cur=TrackIndices->begin(); Cur!=TrackIndices->end(); Cur++)
		std::sort(Cur->begin(), Cur->end(), DTSComparison);
}

int WriteIndex(const char *IndexFile, FrameIndex *TrackIndices, char *ErrorMsg, unsigned MsgSize) {
	std::ofstream Index(IndexFile, std::ios::out | std::ios::binary | std::ios::trunc);

	if (!Index.is_open()) {
		_snprintf(ErrorMsg, MsgSize, "Failed to open '%s' for writing", IndexFile);
		return 1;
	}

	// Write the index file header
	IndexHeader IH;
	IH.Id = INDEXID;
	IH.Version = INDEXVERSION;
	IH.Tracks = TrackIndices->size();
	IH.Decoder = TrackIndices->Decoder;
	Index.write(reinterpret_cast<char *>(&IH), sizeof(IH));
	
	for (unsigned int i = 0; i < IH.Tracks; i++) {
		// Write how many records belong to the current stream
		size_t Frames = (*TrackIndices)[i].size();
		Index.write(reinterpret_cast<char *>(&Frames), sizeof(Frames));
		int Num = (*TrackIndices)[i].TB.Num;
		Index.write(reinterpret_cast<char *>(&Num), sizeof(Num));
		int Den = (*TrackIndices)[i].TB.Den;
		Index.write(reinterpret_cast<char *>(&Den), sizeof(Den));

		for (size_t j = 0; j < Frames; j++)
			Index.write(reinterpret_cast<char *>(&(TrackIndices->at(i)[j])), sizeof(FrameInfo));
	}

	return 0;
}

static FrameIndex *MakeMatroskaIndex(const char *SourceFile, int AudioTrackMask, const char *AudioFile, IndexCallback IP, void *Private, char *ErrorMsg, unsigned MsgSize) {
	MatroskaFile *MF;
	char ErrorMessage[256];
	MatroskaReaderContext MC;
	MC.Buffer = NULL;
	MC.BufferSize = 0;

	InitStdIoStream(&MC.ST);
	MC.ST.fp = fopen(SourceFile, "rb");
	if (MC.ST.fp == NULL) {
		_snprintf(ErrorMsg, MsgSize, "Can't open '%s': %s", SourceFile, strerror(errno));
		return NULL;
	}

	setvbuf(MC.ST.fp, NULL, _IOFBF, CACHESIZE);

	MF = mkv_OpenEx(&MC.ST.base, 0, 0, ErrorMessage, sizeof(ErrorMessage));
	if (MF == NULL) {
		fclose(MC.ST.fp);
		_snprintf(ErrorMsg, MsgSize, "Can't parse Matroska file: %s", ErrorMessage);
		return NULL;
	}

	MatroskaMemory MM = MatroskaMemory(MF, &MC);

	// Audio stuff

	int16_t *db;
	AudioContext *AudioContexts;
	IndexMemory IM = IndexMemory(mkv_GetNumTracks(MF), db, AudioContexts);

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++) {
		if (AudioTrackMask & (1 << i) && mkv_GetTrackInfo(MF, i)->Type == TT_AUDIO) {
			AVCodecContext *AudioCodecContext = avcodec_alloc_context();
			AudioCodecContext->extradata = (uint8_t *)mkv_GetTrackInfo(MF, i)->CodecPrivate;
			AudioCodecContext->extradata_size = mkv_GetTrackInfo(MF, i)->CodecPrivateSize;
			AudioContexts[i].CTX = AudioCodecContext;

			if (mkv_GetTrackInfo(MF, i)->CompEnabled) {
				AudioContexts[i].CS = cs_Create(MF, i, ErrorMessage, sizeof(ErrorMessage));
				if (AudioContexts[i].CS == NULL) {
					_snprintf(ErrorMsg, MsgSize, "Can't create decompressor: %s", ErrorMessage);
					return NULL;
				}
			}

			AVCodec *AudioCodec = avcodec_find_decoder(MatroskaToFFCodecID(mkv_GetTrackInfo(MF, i)));
			if (AudioCodec == NULL) {
					_snprintf(ErrorMsg, MsgSize, "Audio codec not found");
					return NULL;
			}

			if (avcodec_open(AudioCodecContext, AudioCodec) < 0) {
					_snprintf(ErrorMsg, MsgSize, "Could not open audio codec");
					return NULL;
			}
		} else {
			AudioTrackMask &= ~(1 << i);
		}
	}

	//

	int64_t CurrentPos = _ftelli64(MC.ST.fp);
	_fseeki64(MC.ST.fp, 0, SEEK_END);
	int64_t SourceSize = _ftelli64(MC.ST.fp);
	_fseeki64(MC.ST.fp, CurrentPos, SEEK_SET);

	FrameIndex *TrackIndices = new FrameIndex();
	TrackIndices->Decoder = 1;

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++)
		TrackIndices->push_back(FrameInfoVector(mkv_TruncFloat(mkv_GetTrackInfo(MF, i)->TimecodeScale), 1000000));

	ulonglong StartTime, EndTime, FilePos;
	unsigned int Track, FrameFlags, FrameSize;

	while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0) {
		// Update progress
		if (IP) {
			if ((*IP)(0, _ftelli64(MC.ST.fp), SourceSize, Private)) {
				_snprintf(ErrorMsg, MsgSize, "Cancelled by user");
				delete TrackIndices;
				return NULL;
			}
		}

		// Only create index entries for video for now to save space
		if (mkv_GetTrackInfo(MF, Track)->Type == TT_VIDEO)
			(*TrackIndices)[Track].push_back(FrameInfo(StartTime, (FrameFlags & FRAME_KF) != 0));

		if (AudioTrackMask & (1 << Track)) {
			ReadFrame(FilePos, FrameSize, AudioContexts[Track].CS, MC, ErrorMsg, MsgSize);
			(*TrackIndices)[Track].push_back(FrameInfo(AudioContexts[Track].CurrentSample, FilePos, FrameSize, (FrameFlags & FRAME_KF) != 0));

			int Size = FrameSize;
			uint8_t *Data = MC.Buffer;		
			AVCodecContext *AudioCodecContext = AudioContexts[Track].CTX;

			while (Size > 0) {
				int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
				int Ret = avcodec_decode_audio2(AudioCodecContext, db, &dbsize, Data, Size);
				if (Ret < 0) {
					_snprintf(ErrorMsg, MsgSize, "Audio decoding error");
					delete TrackIndices;
					return NULL;
				}

				if (Ret > 0) {
					Size -= Ret;
					Data += Ret;
				}
				if (dbsize > 0) {
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

					AudioContexts[Track].CurrentSample += (dbsize * 8) / (av_get_bits_per_sample_format(AudioCodecContext->sample_fmt) * AudioCodecContext->channels);
					AudioContexts[Track].W64W->WriteData(db, dbsize);
				}
			}
		}
	}

	SortTrackIndices(TrackIndices);
	return TrackIndices;
}

FrameIndex *MakeIndex(const char *SourceFile, int AudioTrackMask, const char *AudioFile, IndexCallback IP, void *Private, char *ErrorMsg, unsigned MsgSize) {
	AVFormatContext *FormatContext = NULL;

	if (av_open_input_file(&FormatContext, SourceFile, NULL, 0, NULL) != 0) {
		_snprintf(ErrorMsg, MsgSize, "Can't open '%s'", SourceFile);
		return NULL;
	}

	// Do matroska indexing instead?
	if (!strcmp(FormatContext->iformat->name, "matroska")) {
		av_close_input_file(FormatContext);
		return MakeMatroskaIndex(SourceFile, AudioTrackMask, AudioFile, IP, Private, ErrorMsg, MsgSize);
	}
	
	if (av_find_stream_info(FormatContext) < 0) {
		av_close_input_file(FormatContext);
		_snprintf(ErrorMsg, MsgSize, "Couldn't find stream information");
		return NULL;
	}

	// Audio stuff

	int16_t *db;
	AudioContext *AudioContexts;
	IndexMemory IM = IndexMemory(FormatContext->nb_streams, db, AudioContexts);

	for (unsigned int i = 0; i < FormatContext->nb_streams; i++) {
		if (AudioTrackMask & (1 << i) && FormatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO) {
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
		} else {
			AudioTrackMask &= ~(1 << i);
		}
	}

	//

	FrameIndex *TrackIndices = new FrameIndex();
	TrackIndices->Decoder = 0;

	for (unsigned int i = 0; i < FormatContext->nb_streams; i++)
		TrackIndices->push_back(FrameInfoVector(FormatContext->streams[i]->time_base.num * 1000, 
		FormatContext->streams[i]->time_base.den));

	AVPacket Packet;
	while (av_read_frame(FormatContext, &Packet) >= 0) {
		// Update progress
		if (IP) {
			if ((*IP)(0, FormatContext->pb->pos, FormatContext->file_size, Private)) {
				_snprintf(ErrorMsg, MsgSize, "Cancelled by user");
				delete TrackIndices;
				return NULL;
			}
		}

		// Only create index entries for video for now to save space
		if (FormatContext->streams[Packet.stream_index]->codec->codec_type == CODEC_TYPE_VIDEO)
			(*TrackIndices)[Packet.stream_index].push_back(FrameInfo(Packet.dts, (Packet.flags & PKT_FLAG_KEY) ? 1 : 0));

		if (AudioTrackMask & (1 << Packet.stream_index)) {
				AVCodecContext *AudioCodecContext = FormatContext->streams[Packet.stream_index]->codec;
				int Size = Packet.size;
				uint8_t *Data = Packet.data;

				while (Size > 0) {
					int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
					int Ret = avcodec_decode_audio2(AudioCodecContext, db, &dbsize, Data, Size);
					if (Ret < 0) {
						_snprintf(ErrorMsg, MsgSize, "Audio decoding error");
						delete TrackIndices;
						return NULL;
					}

					if (Ret > 0) {
						Size -= Ret;
						Data += Ret;
					}

					if (dbsize > 0) {
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

	av_close_input_file(FormatContext);

	SortTrackIndices(TrackIndices);
	return TrackIndices;
}

FrameIndex *ReadIndex(const char *IndexFile, char *ErrorMsg, unsigned MsgSize) {
	std::ifstream Index(IndexFile, std::ios::in | std::ios::binary);

	if (!Index.is_open()) {
		_snprintf(ErrorMsg, MsgSize, "Failed to open '%s' for reading", IndexFile);
		return NULL;
	}
	
	// Read the index file header
	IndexHeader IH;
	Index.read(reinterpret_cast<char *>(&IH), sizeof(IH));
	if (IH.Id != INDEXID) {
		_snprintf(ErrorMsg, MsgSize, "'%s' is not a valid index file", IndexFile);
		return NULL;
	}

	if (IH.Version != INDEXVERSION) {
		_snprintf(ErrorMsg, MsgSize, "'%s' is not the expected index version", IndexFile);
		return NULL;
	}

	FrameIndex *TrackIndices = new FrameIndex();

	try {

		TrackIndices->Decoder = IH.Decoder;

		for (unsigned int i = 0; i < IH.Tracks; i++) {
			// Read how many records belong to the current stream
			size_t Frames;
			Index.read(reinterpret_cast<char *>(&Frames), sizeof(Frames));
			int Num;
			Index.read(reinterpret_cast<char *>(&Num), sizeof(Num));
			int Den;
			Index.read(reinterpret_cast<char *>(&Den), sizeof(Den));
			TrackIndices->push_back(FrameInfoVector(Num, Den));

			FrameInfo FI(0, false);
			for (size_t j = 0; j < Frames; j++) {
				Index.read(reinterpret_cast<char *>(&FI), sizeof(FrameInfo));
				TrackIndices->at(i).push_back(FI);
			}
		}

	} catch (...) {
		delete TrackIndices;
		_snprintf(ErrorMsg, MsgSize, "Unknown error while reading index information in '%s'", IndexFile);	
		return NULL;
	}

	return TrackIndices;
}

FrameInfo::FrameInfo(int64_t DTS, bool KeyFrame) {
	this->DTS = DTS;
	this->FilePos = 0;
	this->FrameSize = 0;
	this->KeyFrame = KeyFrame;
}

FrameInfo::FrameInfo(int64_t SampleStart, int64_t FilePos, unsigned int FrameSize, bool KeyFrame) {
	this->SampleStart = SampleStart;
	this->FilePos = FilePos;
	this->FrameSize = FrameSize;
	this->KeyFrame = KeyFrame;
}

int FrameInfoVector::WriteTimecodes(const char *TimecodeFile, char *ErrorMsg, unsigned MsgSize) {
	std::ofstream Timecodes(TimecodeFile, std::ios::out | std::ios::trunc);

	if (!Timecodes.is_open()) {
		_snprintf(ErrorMsg, MsgSize, "Failed to open '%s' for writing", TimecodeFile);
		return 1;
	}

	Timecodes << "# timecode format v2\n";

	for (iterator Cur=begin(); Cur!=end(); Cur++)
		Timecodes << (int64_t)((Cur->DTS * TB.Num) / (double)TB.Den) << "\n";

	return 0;
}

int FrameInfoVector::FrameFromDTS(int64_t DTS) {
	for (int i = 0; i < static_cast<int>(size()); i++)
		if (at(i).DTS == DTS)
			return i;
	return -1;
}

int FrameInfoVector::ClosestFrameFromDTS(int64_t DTS) {
	int Frame = 0; 
	int64_t BestDiff = 0xFFFFFFFFFFFFFFLL; // big number
	for (int i = 0; i < static_cast<int>(size()); i++) {
		int64_t CurrentDiff = FFABS(at(i).DTS - DTS);
		if (CurrentDiff < BestDiff) {
			BestDiff = CurrentDiff;
			Frame = i;
		}
	}
	return Frame;
}

int FrameInfoVector::FindClosestKeyFrame(int Frame) {
	Frame = FFMIN(FFMAX(Frame, 0), size() - 1);
	for (int i = Frame; i > 0; i--)
		if (at(i).KeyFrame)
			return i;
	return 0;
}

FrameInfoVector::FrameInfoVector() {
	TB.Num = 0; 
	TB.Den = 0;
}

FrameInfoVector::FrameInfoVector(int Num, int Den) {
	TB.Num = Num; 
	TB.Den = Den;
}
