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

#include "ffaudiosource.h"
#include <errno.h>

#ifdef __UNIX__
#define _snprintf snprintf
#endif

TAudioBlock::TAudioBlock(int64_t Start, int64_t Samples, uint8_t *SrcData, size_t SrcBytes) {
	this->Start = Start;
	this->Samples = Samples;
	Data = new uint8_t[SrcBytes];
	memcpy(Data, SrcData, SrcBytes);
}

TAudioBlock::~TAudioBlock() {
	delete[] Data;
}

TAudioCache::TAudioCache() {
	MaxCacheBlocks = 0;
	BytesPerSample = 0;
}

TAudioCache::~TAudioCache() {
	for (TAudioCache::iterator it=begin(); it != end(); it++)
		delete *it;
}

void TAudioCache::Initialize(int BytesPerSample, int MaxCacheBlocks) {
	this->BytesPerSample = BytesPerSample;
	this->MaxCacheBlocks = MaxCacheBlocks;
}

void TAudioCache::CacheBlock(int64_t Start, int64_t Samples, uint8_t *SrcData) {
	if (BytesPerSample > 0) {
		for (TAudioCache::iterator it=begin(); it != end(); it++) {
			if ((*it)->Start == Start) {
				delete *it;
				erase(it);
				break;
			}
		}

		push_front(new TAudioBlock(Start, Samples, SrcData, Samples * BytesPerSample));
		if (size() >= MaxCacheBlocks) {
			delete back();
			pop_back();
		}
	}
}

bool TAudioCache::AudioBlockComp(TAudioBlock *A, TAudioBlock *B) {
	return A->Start < B->Start;
}

int64_t TAudioCache::FillRequest(int64_t Start, int64_t Samples, uint8_t *Dst) {
	// May be better to move used blocks to the front
	std::list<TAudioBlock *> UsedBlocks;
	for (TAudioCache::iterator it=begin(); it != end(); it++) {
		int64_t SrcOffset = FFMAX(0, Start - (*it)->Start);
		int64_t DstOffset = FFMAX(0, (*it)->Start - Start);
		int64_t CopySamples = FFMIN((*it)->Samples - SrcOffset, Samples - DstOffset);
		if (CopySamples > 0) {
			memcpy(Dst + DstOffset * BytesPerSample, (*it)->Data + SrcOffset * BytesPerSample, CopySamples * BytesPerSample);
			UsedBlocks.push_back(*it);
		}
	}
	UsedBlocks.sort(AudioBlockComp);
	int64_t Ret = Start;
	for (std::list<TAudioBlock *>::iterator it = UsedBlocks.begin(); it != UsedBlocks.end(); it++) {
		if (it == UsedBlocks.begin() || Ret == (*it)->Start)
			Ret = (*it)->Start + (*it)->Samples;
		else
			break;
	}
	return FFMIN(Ret, Start + Samples);
}

FFAudio::FFAudio() {
	CurrentSample = 0;
	DecodingBuffer = new uint8_t[AVCODEC_MAX_AUDIO_FRAME_SIZE * 10];
};

FFAudio::~FFAudio() {
	delete[] DecodingBuffer;
};

void FFLAVFAudio::Free(bool CloseCodec) {
	if (CloseCodec)
		avcodec_close(CodecContext);
	av_close_input_file(FormatContext);
}

FFLAVFAudio::FFLAVFAudio(const char *SourceFile, int Track, FFIndex *Index, char *ErrorMsg, unsigned MsgSize) {
	FormatContext = NULL;
	AVCodec *Codec = NULL;
	AudioTrack = Track;
	Frames = (*Index)[AudioTrack];

	if (Frames.size() == 0) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Audio track contains no frames, was it indexed properly?");
		throw ErrorMsg;
	}

	if (av_open_input_file(&FormatContext, SourceFile, NULL, 0, NULL) != 0) {
		_snprintf(ErrorMsg, MsgSize, "Couldn't open '%s'", SourceFile);
		throw ErrorMsg;
	}
	
	if (av_find_stream_info(FormatContext) < 0) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Couldn't find stream information");
		throw ErrorMsg;
	}

	CodecContext = FormatContext->streams[AudioTrack]->codec;

	Codec = avcodec_find_decoder(CodecContext->codec_id);
	if (Codec == NULL) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Audio codec not found");
		throw ErrorMsg;
	}

	if (avcodec_open(CodecContext, Codec) < 0) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Could not open audio codec");
		throw ErrorMsg;
	}

	// Always try to decode a frame to make sure all required parameters are known
	int64_t Dummy;
	if (DecodeNextAudioBlock(DecodingBuffer, &Dummy, ErrorMsg, MsgSize) < 0) {
		Free(true);
		throw ErrorMsg;
	}
	av_seek_frame(FormatContext, AudioTrack, Frames[0].DTS, AVSEEK_FLAG_BACKWARD);
	avcodec_flush_buffers(CodecContext);

	FillAP(AP, CodecContext, Frames);

	if (AP.SampleRate <= 0 || AP.BitsPerSample <= 0) {
		Free(true);
		_snprintf(ErrorMsg, MsgSize, "Codec returned zero size audio");
		throw ErrorMsg;
	}

	AudioCache.Initialize((AP.Channels * AP.BitsPerSample) / 8, 50);
}

int FFLAVFAudio::DecodeNextAudioBlock(uint8_t *Buf, int64_t *Count, char *ErrorMsg, unsigned MsgSize) {
	const size_t SizeConst = (av_get_bits_per_sample_format(CodecContext->sample_fmt) * CodecContext->channels) / 8;
	int Ret = -1;
	*Count = 0;
	AVPacket Packet, TempPacket;
	InitNullPacket(&Packet);
	InitNullPacket(&TempPacket);

	while (av_read_frame(FormatContext, &Packet) >= 0) {
        if (Packet.stream_index == AudioTrack) {
			TempPacket.data = Packet.data;
			TempPacket.size = Packet.size;

			while (TempPacket.size > 0) {
				int TempOutputBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE * 10;
				Ret = avcodec_decode_audio3(CodecContext, (int16_t *)Buf, &TempOutputBufSize, &TempPacket);

				if (Ret < 0) {// throw error or something?
					av_free_packet(&Packet);
					goto Done;
				}

				if (Ret > 0) {
					TempPacket.size -= Ret;
					TempPacket.data += Ret;
					Buf += TempOutputBufSize;
					if (SizeConst)
						*Count += TempOutputBufSize / SizeConst;
				}
			}

			av_free_packet(&Packet);
			goto Done;
        }

		av_free_packet(&Packet);
	}

Done:
	return Ret;
}

int FFLAVFAudio::GetAudio(void *Buf, int64_t Start, int64_t Count, char *ErrorMsg, unsigned MsgSize) {
	const int64_t SizeConst = (av_get_bits_per_sample_format(CodecContext->sample_fmt) * CodecContext->channels) / 8;
	memset(Buf, 0, SizeConst * Count);

	int PreDecBlocks = 0;
	uint8_t *DstBuf = static_cast<uint8_t *>(Buf);

	// Fill with everything in the cache
	int64_t CacheEnd = AudioCache.FillRequest(Start, Count, DstBuf);
	// Was everything in the cache?
	if (CacheEnd == Start + Count)
		return 0;

	size_t CurrentAudioBlock;
	// Is seeking required to decode the requested samples?
//	if (!(CurrentSample >= Start && CurrentSample <= CacheEnd)) {
	if (CurrentSample != CacheEnd) {
		PreDecBlocks = 15;
		CurrentAudioBlock = FFMAX((int64_t)Frames.FindClosestAudioKeyFrame(CacheEnd) - PreDecBlocks - 20, (int64_t)0);
		av_seek_frame(FormatContext, AudioTrack, Frames[CurrentAudioBlock].DTS, AVSEEK_FLAG_BACKWARD);
		avcodec_flush_buffers(CodecContext);

		AVPacket Packet;
		InitNullPacket(&Packet);

		// Establish where we actually are
		// Trigger on packet dts difference since groups can otherwise be indistinguishable
		int64_t LastDTS = - 1;
		while (av_read_frame(FormatContext, &Packet) >= 0) {
			if (Packet.stream_index == AudioTrack) {
				if (LastDTS < 0) {
					LastDTS = Packet.dts;
				} else if (LastDTS != Packet.dts) {
					for (size_t i = 0; i < Frames.size(); i++)
						if (Frames[i].DTS == Packet.dts) {
							// The current match was consumed
							CurrentAudioBlock = i + 1;
							break;
						}

					av_free_packet(&Packet);
					break;
				}
			}

			av_free_packet(&Packet);
		}
	} else {
		CurrentAudioBlock = Frames.FindClosestAudioKeyFrame(CurrentSample);
	}

	int64_t DecodeCount;

	do {
		int Ret = DecodeNextAudioBlock(DecodingBuffer, &DecodeCount, ErrorMsg, MsgSize);
		if (Ret < 0) {
			// FIXME
			//Env->ThrowError("Bleh, bad audio decoding");
		}

		// Cache the block if enough blocks before it have been decoded to avoid garbage
		if (PreDecBlocks == 0) {
			AudioCache.CacheBlock(Frames[CurrentAudioBlock].SampleStart, DecodeCount, DecodingBuffer);
			CacheEnd = AudioCache.FillRequest(CacheEnd, Start + Count - CacheEnd, DstBuf + (CacheEnd - Start) * SizeConst);
		} else {
			PreDecBlocks--;
		}

		CurrentAudioBlock++;
		if (CurrentAudioBlock < Frames.size())
			CurrentSample = Frames[CurrentAudioBlock].SampleStart;
	} while (Start + Count - CacheEnd > 0 && CurrentAudioBlock < Frames.size());

	return 0;
}

FFLAVFAudio::~FFLAVFAudio() {
	Free(true);
}

void FFMatroskaAudio::Free(bool CloseCodec) {
	if (CS)
		cs_Destroy(CS);
	if (MC.ST.fp) {
		mkv_Close(MF);
		fclose(MC.ST.fp);
	}
	if (CloseCodec)
		avcodec_close(CodecContext);
	av_free(CodecContext);
}
	
FFMatroskaAudio::FFMatroskaAudio(const char *SourceFile, int Track, FFIndex *Index, char *ErrorMsg, unsigned MsgSize) {
	CodecContext = NULL;
	AVCodec *Codec = NULL;
	TrackInfo *TI = NULL;
	CS = NULL;
	Frames = (*Index)[Track];

	if (Frames.size() == 0) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Audio track contains no frames, was it indexed properly?");
		throw ErrorMsg;
	}

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

	mkv_SetTrackMask(MF, ~(1 << Track));
	TI = mkv_GetTrackInfo(MF, Track);

	if (TI->CompEnabled) {
		CS = cs_Create(MF, Track, ErrorMessage, sizeof(ErrorMessage));
		if (CS == NULL) {
			Free(false);
			_snprintf(ErrorMsg, MsgSize, "Can't create decompressor: %s", ErrorMessage);
			throw ErrorMsg;
		}
	}

	CodecContext = avcodec_alloc_context();
	CodecContext->extradata = (uint8_t *)TI->CodecPrivate;
	CodecContext->extradata_size = TI->CodecPrivateSize;

	Codec = avcodec_find_decoder(MatroskaToFFCodecID(TI->CodecID, TI->CodecPrivate));
	if (Codec == NULL) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Video codec not found");
		throw ErrorMsg;
	}

	if (avcodec_open(CodecContext, Codec) < 0) {
		Free(false);
		_snprintf(ErrorMsg, MsgSize, "Could not open video codec");
		throw ErrorMsg;
	}

	// Always try to decode a frame to make sure all required parameters are known
	int64_t Dummy;
	if (DecodeNextAudioBlock(DecodingBuffer, &Dummy, Frames[0].FilePos, Frames[0].FrameSize, ErrorMsg, MsgSize) < 0) {
		Free(true);
		throw ErrorMsg;
	}
	avcodec_flush_buffers(CodecContext);

	FillAP(AP, CodecContext, Frames);

	if (AP.SampleRate <= 0 || AP.BitsPerSample <= 0) {
		Free(true);
		_snprintf(ErrorMsg, MsgSize, "Codec returned zero size audio");
		throw ErrorMsg;
	}

	AudioCache.Initialize((AP.Channels * AP.BitsPerSample) / 8, 50);
}

FFMatroskaAudio::~FFMatroskaAudio() {
	Free(true);
}

int FFMatroskaAudio::GetAudio(void *Buf, int64_t Start, int64_t Count, char *ErrorMsg, unsigned MsgSize) {
	const int64_t SizeConst = (av_get_bits_per_sample_format(CodecContext->sample_fmt) * CodecContext->channels) / 8;
	memset(Buf, 0, SizeConst * Count);

	int PreDecBlocks = 0;
	uint8_t *DstBuf = static_cast<uint8_t *>(Buf);

	// Fill with everything in the cache
	int64_t CacheEnd = AudioCache.FillRequest(Start, Count, DstBuf);
	// Was everything in the cache?
	if (CacheEnd == Start + Count)
		return 0;

	size_t CurrentAudioBlock;
	// Is seeking required to decode the requested samples?
//	if (!(CurrentSample >= Start && CurrentSample <= CacheEnd)) {
	if (CurrentSample != CacheEnd) {
		PreDecBlocks = 15;
		CurrentAudioBlock = FFMAX((int64_t)Frames.FindClosestAudioKeyFrame(CacheEnd) - PreDecBlocks, (int64_t)0);
		avcodec_flush_buffers(CodecContext);
	} else {
		CurrentAudioBlock = Frames.FindClosestAudioKeyFrame(CurrentSample);
	}

	int64_t DecodeCount;

	do {
		int Ret = DecodeNextAudioBlock(DecodingBuffer, &DecodeCount, Frames[CurrentAudioBlock].FilePos, Frames[CurrentAudioBlock].FrameSize, ErrorMsg, MsgSize);
		if (Ret < 0) {
			// FIXME
			//Env->ThrowError("Bleh, bad audio decoding");
		}

		// Cache the block if enough blocks before it have been decoded to avoid garbage
		if (PreDecBlocks == 0) {
			AudioCache.CacheBlock(Frames[CurrentAudioBlock].SampleStart, DecodeCount, DecodingBuffer);
			CacheEnd = AudioCache.FillRequest(CacheEnd, Start + Count - CacheEnd, DstBuf + (CacheEnd - Start) * SizeConst);
		} else {
			PreDecBlocks--;
		}

		CurrentAudioBlock++;
		if (CurrentAudioBlock < Frames.size())
			CurrentSample = Frames[CurrentAudioBlock].SampleStart;
	} while (Start + Count - CacheEnd > 0 && CurrentAudioBlock < Frames.size());

	return 0;
}

int FFMatroskaAudio::DecodeNextAudioBlock(uint8_t *Buf, int64_t *Count, uint64_t FilePos, unsigned int FrameSize, char *ErrorMsg, unsigned MsgSize) {
	const size_t SizeConst = (av_get_bits_per_sample_format(CodecContext->sample_fmt) * CodecContext->channels) / 8;
	int Ret = -1;
	*Count = 0;
	AVPacket TempPacket;
	InitNullPacket(&TempPacket);

	// FIXME check return
	ReadFrame(FilePos, FrameSize, CS, MC, ErrorMsg, MsgSize);
	TempPacket.size = FrameSize;
	TempPacket.data = MC.Buffer;

	while (TempPacket.size > 0) {
		int TempOutputBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
		Ret = avcodec_decode_audio3(CodecContext, (int16_t *)Buf, &TempOutputBufSize, &TempPacket);

		if (Ret < 0) // throw error or something?
			goto Done;

		if (Ret > 0) {
			TempPacket.size -= Ret;
			TempPacket.data += Ret;
			Buf += TempOutputBufSize;
			if (SizeConst)
				*Count += TempOutputBufSize / SizeConst;
		}
	}

Done:
	return Ret;
}
