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

#include "indexing.h"
#include "matroskaparser.h"



FFMatroskaIndexer::FFMatroskaIndexer(const char *Filename) : FFMS_Indexer(Filename) {
	SourceFile = Filename;
	char ErrorMessage[256];
	for (int i = 0; i < 32; i++) {
		Codec[i] = NULL;
	}

	InitStdIoStream(&MC.ST);
	MC.ST.fp = ffms_fopen(SourceFile, "rb");
	if (MC.ST.fp == NULL) {
		std::ostringstream buf;
		buf << "Can't open '" << SourceFile << "': " << strerror(errno);
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
	}

	setvbuf(MC.ST.fp, NULL, _IOFBF, CACHESIZE);

	MF = mkv_OpenEx(&MC.ST.base, 0, 0, ErrorMessage, sizeof(ErrorMessage));
	if (MF == NULL) {
		fclose(MC.ST.fp);
		std::ostringstream buf;
		buf << "Can't parse Matroska file: " << ErrorMessage;
		throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
	}

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++) {
		TrackInfo *TI = mkv_GetTrackInfo(MF, i);
		Codec[i] = avcodec_find_decoder(MatroskaToFFCodecID(TI->CodecID, TI->CodecPrivate, 0, TI->AV.Audio.BitDepth));
	}
}

FFMatroskaIndexer::~FFMatroskaIndexer() {
	mkv_Close(MF);
	fclose(MC.ST.fp);
}

FFMS_Index *FFMatroskaIndexer::DoIndexing() {
	char ErrorMessage[256];
	std::vector<SharedAudioContext> AudioContexts(mkv_GetNumTracks(MF), SharedAudioContext(true));
	std::vector<SharedVideoContext> VideoContexts(mkv_GetNumTracks(MF), SharedVideoContext(true));

	std::auto_ptr<FFMS_Index> TrackIndices(new FFMS_Index(Filesize, Digest));
	TrackIndices->Decoder = FFMS_SOURCE_MATROSKA;

	for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++) {
		TrackInfo *TI = mkv_GetTrackInfo(MF, i);
		TrackIndices->push_back(FFMS_Track(mkv_TruncFloat(mkv_GetTrackInfo(MF, i)->TimecodeScale), 1000000, HaaliTrackTypeToFFTrackType(mkv_GetTrackInfo(MF, i)->Type)));

		if (HaaliTrackTypeToFFTrackType(TI->Type) == FFMS_TYPE_VIDEO && Codec[i] && (VideoContexts[i].Parser = av_parser_init(Codec[i]->id))) {

			AVCodecContext *CodecContext = avcodec_alloc_context();

			InitializeCodecContextFromMatroskaTrackInfo(TI, CodecContext);

			if (avcodec_open(CodecContext, Codec[i]) < 0) {
				av_freep(&CodecContext);
				throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
					"Could not open video codec");
			}

			if (TI->CompEnabled) {
				VideoContexts[i].CS = cs_Create(MF, i, ErrorMessage, sizeof(ErrorMessage));
				if (VideoContexts[i].CS == NULL) {
					std::ostringstream buf;
					buf << "Can't create decompressor: " << ErrorMessage;
					throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
				}
			}

			VideoContexts[i].CodecContext = CodecContext;
			VideoContexts[i].Parser->flags = PARSER_FLAG_COMPLETE_FRAMES;
		}

		if (IndexMask & (1 << i) && TI->Type == TT_AUDIO) {
			AVCodecContext *AudioCodecContext = avcodec_alloc_context();
			InitializeCodecContextFromMatroskaTrackInfo(TI, AudioCodecContext);
			AudioContexts[i].CodecContext = AudioCodecContext;

			if (TI->CompEnabled) {
				AudioContexts[i].CS = cs_Create(MF, i, ErrorMessage, sizeof(ErrorMessage));
				if (AudioContexts[i].CS == NULL) {
					av_freep(&AudioCodecContext);
					AudioContexts[i].CodecContext = NULL;
					std::ostringstream buf;
					buf << "Can't create decompressor: " << ErrorMessage;
					throw FFMS_Exception(FFMS_ERROR_PARSER, FFMS_ERROR_FILE_READ, buf.str());
				}
			}

			AVCodec *AudioCodec = Codec[i];
			if (AudioCodec == NULL) {
				av_freep(&AudioCodecContext);
				AudioContexts[i].CodecContext = NULL;
				throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_UNSUPPORTED,
					"Audio codec not found");
			}

			if (avcodec_open(AudioCodecContext, AudioCodec) < 0) {
				av_freep(&AudioCodecContext);
				AudioContexts[i].CodecContext = NULL;
				throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
					"Could not open audio codec");
			}
		} else {
			IndexMask &= ~(1 << i);
		}
	}

	//

	ulonglong StartTime, EndTime, FilePos;
	unsigned int Track, FrameFlags, FrameSize;
	AVPacket TempPacket;
	InitNullPacket(TempPacket);

	while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0) {
		// Update progress
		if (IC) {
			if ((*IC)(ftello(MC.ST.fp), Filesize, ICPrivate))
				throw FFMS_Exception(FFMS_ERROR_CANCELLED, FFMS_ERROR_USER,
					"Cancelled by user");
		}
	
		// Only create index entries for video for now to save space
		if (mkv_GetTrackInfo(MF, Track)->Type == TT_VIDEO) {
			uint8_t *OB;
			int OBSize;
			int RepeatPict = -1;

			if (VideoContexts[Track].Parser) {
				av_parser_parse2(VideoContexts[Track].Parser, VideoContexts[Track].CodecContext, &OB, &OBSize, TempPacket.data, TempPacket.size, ffms_av_nopts_value, ffms_av_nopts_value, ffms_av_nopts_value);
				RepeatPict = VideoContexts[Track].Parser->repeat_pict;
			}

			(*TrackIndices)[Track].push_back(TFrameInfo::VideoFrameInfo(StartTime, RepeatPict, (FrameFlags & FRAME_KF) != 0, FilePos, FrameSize));
		} else if (mkv_GetTrackInfo(MF, Track)->Type == TT_AUDIO && (IndexMask & (1 << Track))) {
			int64_t StartSample = AudioContexts[Track].CurrentSample;
			unsigned int CompressedFrameSize = FrameSize;
			AVCodecContext *AudioCodecContext = AudioContexts[Track].CodecContext;
			ReadFrame(FilePos, FrameSize, AudioContexts[Track].CS, MC);
			TempPacket.data = MC.Buffer;
			TempPacket.size = FrameSize;
			if ((FrameFlags & FRAME_KF) != 0)
				TempPacket.flags = AV_PKT_FLAG_KEY;
			else
				TempPacket.flags = 0;

			while (TempPacket.size > 0) {
				int dbsize = AVCODEC_MAX_AUDIO_FRAME_SIZE*10;
				int Ret = avcodec_decode_audio3(AudioCodecContext, &DecodingBuffer[0], &dbsize, &TempPacket);
				if (Ret < 0) {
					if (ErrorHandling == FFMS_IEH_ABORT) {
						throw FFMS_Exception(FFMS_ERROR_CODEC, FFMS_ERROR_DECODING,
							"Audio decoding error");
					} else if (ErrorHandling == FFMS_IEH_CLEAR_TRACK) {
						(*TrackIndices)[Track].clear();
						IndexMask &= ~(1 << Track);					
						break;
					} else if (ErrorHandling == FFMS_IEH_STOP_TRACK) {
						IndexMask &= ~(1 << Track);					
						break;
					} else if (ErrorHandling == FFMS_IEH_IGNORE) {
						break;
					}
				}

				if (Ret > 0) {
					TempPacket.size -= Ret;
					TempPacket.data += Ret;
				}

				if (dbsize > 0)
					AudioContexts[Track].CurrentSample += (dbsize * 8) / (av_get_bits_per_sample_format(AudioCodecContext->sample_fmt) * AudioCodecContext->channels);

				if (DumpMask & (1 << Track))
					WriteAudio(AudioContexts[Track], TrackIndices.get(), Track, dbsize);
			}

			(*TrackIndices)[Track].push_back(TFrameInfo::AudioFrameInfo(StartTime, StartSample,
				static_cast<unsigned int>(AudioContexts[Track].CurrentSample - StartSample), (FrameFlags & FRAME_KF) != 0, FilePos, CompressedFrameSize));
		}
	}

	TrackIndices->Sort();
	return TrackIndices.release();
}

int FFMatroskaIndexer::GetNumberOfTracks() {
	return mkv_GetNumTracks(MF);
}

FFMS_TrackType FFMatroskaIndexer::GetTrackType(int Track) {
	return HaaliTrackTypeToFFTrackType(mkv_GetTrackInfo(MF, Track)->Type);
}

const char *FFMatroskaIndexer::GetTrackCodec(int Track) {
	if (Codec[Track])
		return Codec[Track]->name;
	else
		return "Unsupported codec/Unknown codec name";
}
