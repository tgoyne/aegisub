#include "ffmpegsource.h"
#include "stdiostream.c"

class FFMatroskaSource : public FFBase {
private:
	StdIoStream	ST; 
    unsigned int BufferSize;
    CompressedStream *VideoCS;
    CompressedStream *AudioCS;

	AVCodecContext *VideoCodecContext;

	MatroskaFile *MF;
	char ErrorMessage[256];
    uint8_t *Buffer;

	int	CurrentFrame;

	int ReadFrame(uint64_t AFilePos, unsigned int AFrameSize, CompressedStream *ACS, IScriptEnvironment *Env);
	int DecodeNextFrame(AVFrame *AFrame, int64_t *AFirstStartTime, IScriptEnvironment* Env);

	int GetTrackIndex(int Index, unsigned char ATrackType, IScriptEnvironment *Env) {
		if (Index == -1)
			for (unsigned int i = 0; i < mkv_GetNumTracks(MF); i++)
				if (mkv_GetTrackInfo(MF, i)->Type == ATrackType) {
					Index = i;
					break;
				}

		if (Index == -1)
			Env->ThrowError("FFmpegSource: No %s track found", (ATrackType & TT_VIDEO) ? "video" : "audio");
		if (Index <= -2)
			return -2;

		if (Index >= (int)mkv_GetNumTracks(MF))
			Env->ThrowError("FFmpegSource: Invalid %s track number", (ATrackType & TT_VIDEO) ? "video" : "audio");

		TrackInfo *TI = mkv_GetTrackInfo(MF, Index);

		if (TI->Type != ATrackType)
			Env->ThrowError("FFmpegSource: Selected track is not %s", (ATrackType & TT_VIDEO) ? "video" : "audio");

		return Index;
	}

public:
	FFMatroskaSource(const char *ASource, int AVideoTrack, int AAudioTrack, const char *ATimecodes, bool AVCache, const char *AVideoCache, const char *AAudioCache, int AACCompression, const char *APPString, int AQuality, IScriptEnvironment* Env) {
		CurrentFrame = 0;
		int VideoTrack;
		int AudioTrack;
		unsigned int TrackMask = ~0;
		AVCodecContext *AudioCodecContext = NULL;
		AVCodec *AudioCodec = NULL;
		VideoCodecContext = NULL;
		AVCodec *VideoCodec = NULL;
		TrackInfo *VideoTI = NULL;
		BufferSize = 0;
		Buffer = NULL;
		VideoCS = NULL;
		AudioCS = NULL;

		memset(&ST,0,sizeof(ST));
		ST.base.read = (int (__cdecl *)(InputStream *,ulonglong,void *,int))StdIoRead;
		ST.base.scan = (longlong (__cdecl *)(InputStream *,ulonglong,unsigned int))StdIoScan;
		ST.base.getcachesize = (unsigned int (__cdecl *)(InputStream *))StdIoGetCacheSize;
		ST.base.geterror = (const char *(__cdecl *)(InputStream *))StdIoGetLastError;
		ST.base.memalloc = (void *(__cdecl *)(InputStream *,size_t))StdIoMalloc;
		ST.base.memrealloc = (void *(__cdecl *)(InputStream *,void *,size_t))StdIoRealloc;
		ST.base.memfree = (void (__cdecl *)(InputStream *,void *)) StdIoFree;
		ST.base.progress = (int (__cdecl *)(InputStream *,ulonglong,ulonglong))StdIoProgress;

		ST.fp = fopen(ASource, "rb");
		if (ST.fp == NULL)
			Env->ThrowError("FFmpegSource: Can't open '%s': %s", ASource, strerror(errno));

		setvbuf(ST.fp, NULL, _IOFBF, CACHESIZE);

		MF = mkv_OpenEx(&ST.base, 0, 0, ErrorMessage, sizeof(ErrorMessage));
		if (MF == NULL) {
			fclose(ST.fp);
			Env->ThrowError("FFmpegSource: Can't parse Matroska file: %s", ErrorMessage);
		}

		VideoTrack = GetTrackIndex(AVideoTrack, TT_VIDEO, Env);
		AudioTrack = GetTrackIndex(AAudioTrack, TT_AUDIO, Env);

		bool VCacheIsValid = true;
		bool ACacheIsValid = true;

		if (VideoTrack >= 0) {
			VCacheIsValid = LoadFrameInfoFromFile(AVideoCache, ASource, VideoTrack);

			VideoTI = mkv_GetTrackInfo(MF, VideoTrack);

			if (VideoTI->CompEnabled) {
				VideoCS = cs_Create(MF, VideoTrack, ErrorMessage, sizeof(ErrorMessage));
				if (VideoCS == NULL)
					Env->ThrowError("FFmpegSource: Can't create decompressor: %s", ErrorMessage);
			}

			VideoCodecContext = avcodec_alloc_context();
			VideoCodecContext->extradata = (uint8_t *)VideoTI->CodecPrivate;
			VideoCodecContext->extradata_size = VideoTI->CodecPrivateSize;

			VideoCodec = avcodec_find_decoder(MatroskaToFFCodecID(VideoTI));
			if (VideoCodec == NULL)
				Env->ThrowError("FFmpegSource: Video codec not found");

			if (avcodec_open(VideoCodecContext, VideoCodec) < 0)
				Env->ThrowError("FFmpegSource: Could not open video codec");

			// Fix for mpeg2 and other formats where decoding a frame is necessary to get information about the stream
			if (VideoCodecContext->pix_fmt == PIX_FMT_NONE) {
				mkv_SetTrackMask(MF, ~(1 << VideoTrack));
				int64_t Dummy;
				DecodeNextFrame(DecodeFrame, &Dummy, Env);
				mkv_Seek(MF, 0, MKVF_SEEK_TO_PREV_KEYFRAME);
			}

			VI.image_type = VideoInfo::IT_TFF;
			VI.width = VideoTI->AV.Video.PixelWidth;
			VI.height = VideoTI->AV.Video.PixelHeight;
			VI.fps_denominator = 1;
			VI.fps_numerator = 30;

			SetOutputFormat(VideoCodecContext->pix_fmt, Env);
			InitPP(VI.width, VI.height, APPString, AQuality, VideoCodecContext->pix_fmt, Env);

			if (!VCacheIsValid)
				TrackMask &= ~(1 << VideoTrack);
		}

		if (AudioTrack >= 0) {
			TrackInfo *AudioTI = mkv_GetTrackInfo(MF, AudioTrack);

			if (AudioTI->CompEnabled) {
				AudioCS = cs_Create(MF, AudioTrack, ErrorMessage, sizeof(ErrorMessage));
				if (AudioCS == NULL)
					Env->ThrowError("FFmpegSource: Can't create decompressor: %s", ErrorMessage);
			}

			AudioCodecContext = avcodec_alloc_context();
			AudioCodecContext->extradata = (uint8_t *)AudioTI->CodecPrivate;
			AudioCodecContext->extradata_size = AudioTI->CodecPrivateSize;

			AudioCodec = avcodec_find_decoder(MatroskaToFFCodecID(AudioTI));
			if (AudioCodec == NULL)
				Env->ThrowError("FFmpegSource: Audio codec not found");

			if (avcodec_open(AudioCodecContext, AudioCodec) < 0)
				Env->ThrowError("FFmpegSource: Could not open audio codec");

			// Fix for ac3 and other codecs where decoding a block of audio is required to get information about it
			if (AudioCodecContext->channels == 0 || AudioCodecContext->sample_rate == 0) {
				mkv_SetTrackMask(MF, ~(1 << AudioTrack));
				uint64_t StartTime, EndTime, FilePos;
				unsigned int Track, FrameFlags, FrameSize;
				mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags);

				uint8_t DecodingBuffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
				int Size = ReadFrame(FilePos, FrameSize, AudioCS, Env);
				uint8_t *Data = Buffer;

				while (Size > 0) {
					int TempOutputBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
					int Ret = avcodec_decode_audio2(AudioCodecContext, (int16_t *)DecodingBuffer, &TempOutputBufSize, Data, Size);
					if (Ret < 0)
						Env->ThrowError("FFmpegSource: Audio decoding error");

					Size -= Ret;
					Data += Ret;
				}

				mkv_Seek(MF, 0, MKVF_SEEK_TO_PREV_KEYFRAME);
			}

			VI.nchannels = AudioCodecContext->channels;
			VI.audio_samples_per_second = AudioCodecContext->sample_rate;

			switch (AudioCodecContext->sample_fmt) {
				case SAMPLE_FMT_U8: VI.sample_type = SAMPLE_INT8; AACCompression = -1; break;
				case SAMPLE_FMT_S16: VI.sample_type = SAMPLE_INT16; break;
				case SAMPLE_FMT_S24: VI.sample_type = SAMPLE_INT24; AACCompression = -1; break;
				case SAMPLE_FMT_S32: VI.sample_type = SAMPLE_INT32; AACCompression = -1; break;
				case SAMPLE_FMT_FLT: VI.sample_type = SAMPLE_FLOAT; AACCompression = -1; break;
				default:
					Env->ThrowError("FFmpegSource: Unsupported/unknown sample format");
			}

			ACacheIsValid = OpenAudioCache(AAudioCache, ASource, AudioTrack, Env);
			if (!ACacheIsValid)
				TrackMask &= ~(1 << AudioTrack);
		}

		mkv_SetTrackMask(MF, TrackMask);

		// Needs to be indexed?
		if (!ACacheIsValid || !VCacheIsValid) {

			FLAC__StreamEncoder *FSE = NULL;
			FILE *RawCache = NULL;
			if (!ACacheIsValid)
				if (AACCompression >= 0)
					AudioCacheType = acFLAC;
				else
					AudioCacheType = acRaw;	

			switch (AudioCacheType) {
				case acFLAC: FSE = NewFLACCacheWriter(AAudioCache, ASource, AudioTrack, AACCompression, Env); break;
				case acRaw: RawCache = NewRawCacheWriter(AAudioCache, ASource, AudioTrack, Env); break;
			}

			uint64_t StartTime, EndTime, FilePos;
			unsigned int Track, FrameFlags, FrameSize;
	
			while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0)
				if (Track == VideoTrack && !VCacheIsValid) {
					FrameToDTS.push_back(FrameInfo(StartTime, (FrameFlags & FRAME_KF) != 0));
					VI.num_frames++;
				} else if (Track == AudioTrack && !ACacheIsValid) {
					int Size = ReadFrame(FilePos, FrameSize, AudioCS, Env);
					uint8_t *Data = Buffer;

					while (Size > 0) {
						int TempOutputBufSize = AVCODEC_MAX_AUDIO_FRAME_SIZE;
						int Ret = avcodec_decode_audio2(AudioCodecContext, (int16_t *)DecodingBuffer, &TempOutputBufSize, Data, Size);
						if (Ret < 0)
							Env->ThrowError("FFmpegSource: Audio decoding error");

						int DecodedSamples = VI.AudioSamplesFromBytes(TempOutputBufSize);

						Size -= Ret;
						Data += Ret;
						VI.num_audio_samples += DecodedSamples;

						if (AudioCacheType == acFLAC) {
							for (int i = 0; i < DecodedSamples * VI.nchannels; i++)
								FLACBuffer[i] = ((int16_t *)DecodingBuffer)[i];
							FLAC__stream_encoder_process_interleaved(FSE, FLACBuffer, DecodedSamples); 
						} else if (AudioCacheType == acRaw) {
							fwrite(DecodingBuffer, 1, TempOutputBufSize, RawCache);
						}
					}
				}

			if (!ACacheIsValid) {
				switch (AudioCacheType) {
					case acFLAC: CloseFLACCacheWriter(FSE); break;
					case acRaw: CloseRawCacheWriter(RawCache); break;
				}

				ACacheIsValid = OpenAudioCache(AAudioCache, ASource, AudioTrack, Env);
				if (!ACacheIsValid)
					Env->ThrowError("FFmpegSource: Failed to open newly created audio cache for reading");
			}

			if (VideoTrack >= 0 && VI.num_frames == 0)
				Env->ThrowError("FFmpegSource: Video track contains no frames");

			if (AudioTrack >= 0 && VI.num_audio_samples == 0)
				Env->ThrowError("FFmpegSource: Audio track contains no samples");

			if (VideoTrack >= 0)
				mkv_Seek(MF, FrameToDTS.front().DTS, MKVF_SEEK_TO_PREV_KEYFRAME);

			if (AVCache && !VCacheIsValid)
				if (!SaveFrameInfoToFile(AVideoCache, ASource, VideoTrack))
					Env->ThrowError("FFmpegSource: Failed to write video cache info");
		}

		if (AudioTrack >= 0) {
			avcodec_close(AudioCodecContext);
			av_free(AudioCodecContext);
		}

		if (VideoTrack >= 0) {
			mkv_SetTrackMask(MF, ~(1 << VideoTrack));

			// Calculate the average framerate
			if (FrameToDTS.size() >= 2) {
				double DTSDiff = (double)(FrameToDTS.back().DTS - FrameToDTS.front().DTS);
				VI.fps_denominator = (unsigned int)(DTSDiff * mkv_TruncFloat(VideoTI->TimecodeScale) / (double)1000 / (double)(VI.num_frames - 1) + 0.5);
				VI.fps_numerator = 1000000; 
			}

			if (!SaveTimecodesToFile(ATimecodes, mkv_TruncFloat(VideoTI->TimecodeScale), 1000000))
				Env->ThrowError("FFmpegSource: Failed to write timecodes");
		}
	}

	~FFMatroskaSource() {
		free(Buffer);
		mkv_Close(MF);
		fclose(ST.fp);
		if (VideoCodecContext)
			avcodec_close(VideoCodecContext);
		av_free(VideoCodecContext);
	}

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* Env);
};

int FFMatroskaSource::ReadFrame(uint64_t AFilePos, unsigned int AFrameSize, CompressedStream *ACS, IScriptEnvironment *Env) {
	if (ACS) {
		char CSBuffer[4096];

		unsigned int DecompressedFrameSize = 0;

		cs_NextFrame(ACS, AFilePos, AFrameSize);

		for (;;) {
			int ReadBytes = cs_ReadData(ACS, CSBuffer, sizeof(CSBuffer));
			if (ReadBytes < 0)
				Env->ThrowError("FFmpegSource: Error decompressing data: %s", cs_GetLastError(ACS));
			if (ReadBytes == 0) {
				return DecompressedFrameSize;
			}

			if (BufferSize < DecompressedFrameSize + ReadBytes) {
				BufferSize = AFrameSize;
				Buffer = (uint8_t *)realloc(Buffer, BufferSize);
				if (Buffer == NULL) 
					Env->ThrowError("FFmpegSource: Out of memory");
			}

			memcpy(Buffer + DecompressedFrameSize, CSBuffer, ReadBytes);
			DecompressedFrameSize += ReadBytes;
		}
	} else {
		if (_fseeki64(ST.fp, AFilePos, SEEK_SET))
			Env->ThrowError("FFmpegSource: fseek(): %s", strerror(errno));

		if (BufferSize < AFrameSize) {
			BufferSize = AFrameSize;
			Buffer = (uint8_t *)realloc(Buffer, BufferSize);
			if (Buffer == NULL) 
				Env->ThrowError("FFmpegSource: Out of memory");
		}

		size_t ReadBytes = fread(Buffer, 1, AFrameSize, ST.fp);
		if (ReadBytes != AFrameSize) {
			if (ReadBytes == 0) {
				if (feof(ST.fp))
					Env->ThrowError("FFmpegSource: Unexpected EOF while reading frame");
				else
					Env->ThrowError("FFmpegSource: Error reading frame: %s", strerror(errno));
			} else
				Env->ThrowError("FFmpegSource: Short read while reading frame");
			Env->ThrowError("FFmpegSource: Unknown read error");
		}

		return AFrameSize;
	}

	return 0;
}

int FFMatroskaSource::DecodeNextFrame(AVFrame *AFrame, int64_t *AFirstStartTime, IScriptEnvironment* Env) {
	int FrameFinished = 0;
	int Ret = -1;
	*AFirstStartTime = -1;

	uint64_t StartTime, EndTime, FilePos;
	unsigned int Track, FrameFlags, FrameSize;
	
	while (mkv_ReadFrame(MF, 0, &Track, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0) {
		FrameSize = ReadFrame(FilePos, FrameSize, VideoCS, Env);
		if (*AFirstStartTime < 0)
			*AFirstStartTime = StartTime;
		Ret = avcodec_decode_video(VideoCodecContext, AFrame, &FrameFinished, Buffer, FrameSize);

		if (FrameFinished)
			goto Done;
	}

	// Flush the last frame
	if (CurrentFrame == VI.num_frames - 1 && VideoCodecContext->has_b_frames)
		Ret = avcodec_decode_video(VideoCodecContext, AFrame, &FrameFinished, NULL, 0);

Done:
	return Ret;
}

PVideoFrame __stdcall FFMatroskaSource::GetFrame(int n, IScriptEnvironment* Env) {
	bool HasSeeked = false;

	if (n < CurrentFrame || FindClosestKeyFrame(n) > CurrentFrame) {
		mkv_Seek(MF, FrameToDTS[n].DTS, MKVF_SEEK_TO_PREV_KEYFRAME);
		avcodec_flush_buffers(VideoCodecContext);
		HasSeeked = true;
	}

	do {
		int64_t StartTime;
		int Ret = DecodeNextFrame(DecodeFrame, &StartTime, Env);

		if (HasSeeked) {
			HasSeeked = false;

			if (StartTime < 0 || (CurrentFrame = FrameFromDTS(StartTime)) < 0)
				Env->ThrowError("FFmpegSource: Frame accurate seeking is not possible in this file");
		}

		CurrentFrame++;
	} while (CurrentFrame <= n);

	return OutputFrame(DecodeFrame, Env);
}

AVSValue __cdecl CreateFFmpegSource(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	if (!UserData) {
		av_register_all();
		UserData = (void *)-1;
	}

	if (!Args[0].Defined())
    	Env->ThrowError("FFmpegSource: No source specified");

	const char *Source = Args[0].AsString();
	int VTrack = Args[1].AsInt(-1);
	int ATrack = Args[2].AsInt(-2);
	const char *Timecodes = Args[3].AsString("");
	bool VCache = Args[4].AsBool(true);
	const char *VCacheFile = Args[5].AsString("");
	const char *ACacheFile = Args[6].AsString("");
	int ACCompression = Args[7].AsInt(-1);
	const char *PPString = Args[8].AsString("");
	int PPQuality = Args[9].AsInt(PP_QUALITY_MAX);
	int SeekMode = Args[10].AsInt(1);

	if (VTrack <= -2 && ATrack <= -2)
		Env->ThrowError("FFmpegSource: No tracks selected");

	if (ACCompression < -1 || ACCompression > 8)
		Env->ThrowError("FFmpegSource: Invalid audio cache compression selected");

	AVFormatContext *FormatContext;

	if (av_open_input_file(&FormatContext, Source, NULL, 0, NULL) != 0)
		Env->ThrowError("FFmpegSource: Couldn't open %s", Args[0].AsString());
	bool IsMatroska = !strcmp(FormatContext->iformat->name, "matroska");
	av_close_input_file(FormatContext);

	if (IsMatroska)
		return new FFMatroskaSource(Source, VTrack, ATrack, Timecodes, VCache, VCacheFile, ACacheFile, ACCompression, PPString, PPQuality, Env);
	else
		return new FFmpegSource(Source, VTrack, ATrack, Timecodes, VCache, VCacheFile, ACacheFile, ACCompression, PPString, PPQuality, SeekMode, Env);
}

AVSValue __cdecl CreateFFPP(AVSValue Args, void* UserData, IScriptEnvironment* Env) {
	return new FFPP(Args[0].AsClip(), Args[1].AsString(""), Args[2].AsInt(PP_QUALITY_MAX), Env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* Env) {
    Env->AddFunction("FFmpegSource", "[source]s[vtrack]i[atrack]i[timecodes]s[vcache]b[vcachefile]s[acachefile]s[accompression]i[pp]s[ppquality]i[seekmode]i", CreateFFmpegSource, 0);
    Env->AddFunction("FFPP", "c[pp]s[ppquality]i", CreateFFPP, 0);
    return "FFmpegSource";
};
