// Copyright (c) 2004-2006, Rodrigo Braz Monteiro, Mike Matsnev
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

/// @file mkv_wrap.cpp
/// @brief High-level interface for obtaining various data from Matroska files
/// @ingroup video_input
///

#include "config.h"

#include "mkv_wrap.h"

#include "ass_file.h"
#include "ass_parser.h"
#include "ass_time.h"
#include "compat.h"
#include "dialog_progress.h"
#include "MatroskaParser.h"

#include <algorithm>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iterator>

#ifdef _WIN32
#include <libaegisub/charset_conv_win.h>
#endif

#include <wx/choicdlg.h> // Keep this last so wxUSE_CHOICEDLG is set.

class MkvStdIO : public InputStream {
public:
	MkvStdIO(std::string const& filename);
	~MkvStdIO() { if (fp) fclose(fp); }

	FILE *fp;
	int error;
};

#define CACHESIZE     1024

#ifdef __VISUALC__
#define std_fseek _fseeki64
#define std_ftell _ftelli64
#else
#define std_fseek fseeko
#define std_ftell ftello
#endif

static void read_subtitles(agi::ProgressSink *ps, MatroskaFile *file, MkvStdIO *input, bool srt, double totalTime, AssParser *parser) {
	std::map<int, std::string> subList;
	std::string readBuf;

	// Load blocks
	ulonglong startTime, endTime, filePos;
	unsigned int rt, frameSize, frameFlags;

	while (mkv_ReadFrame(file, 0, &rt, &startTime, &endTime, &filePos, &frameSize, &frameFlags) == 0) {
		if (ps->IsCancelled()) return;
		if (frameSize == 0) continue;

		readBuf.resize(frameSize);
		std_fseek(input->fp, filePos, SEEK_SET);
		fread(&readBuf[0], 1, frameSize, input->fp);

		// Get start and end times
		longlong timecodeScaleLow = 1000000;
		AssTime subStart = startTime / timecodeScaleLow;
		AssTime subEnd = endTime / timecodeScaleLow;

		// Process SSA/ASS
		if (!srt) {
			std::vector<boost::iterator_range<std::string::iterator>> chunks;
			boost::split(chunks, readBuf, boost::is_any_of(","));

			subList[boost::lexical_cast<int>(chunks[0])] =
				str(boost::format("Dialogue: %d,%s,%s,%s")
					% boost::lexical_cast<int>(chunks[1])
					% subStart.GetAssFormated()
					% subEnd.GetAssFormated()
					% boost::make_iterator_range(begin(chunks[2]), readBuf.end()));
		}
		// Process SRT
		else {
			readBuf = str(boost::format("Dialogue: 0,%s,%s,Default,,0,0,0,,%s") % subStart.GetAssFormated() % subEnd.GetAssFormated() % readBuf);
			boost::replace_all(readBuf, "\r\n", "\\N");
			boost::replace_all(readBuf, "\r", "\\N");
			boost::replace_all(readBuf, "\n", "\\N");

			subList[subList.size()] = readBuf;
		}

		ps->SetProgress(startTime / timecodeScaleLow, totalTime);
	}

	// Insert into file
	for (auto order_value_pair : subList)
		parser->AddLine(order_value_pair.second);
}

void MatroskaWrapper::GetSubtitles(std::string const& filename, AssFile *target) {
	MkvStdIO input(filename);
	char err[2048];
	MatroskaFile *file = mkv_Open(&input, err, sizeof(err));
	if (!file) throw MatroskaException(err);

	try {
		// Get info
		unsigned tracks = mkv_GetNumTracks(file);
		std::vector<unsigned> tracksFound;
		std::vector<std::string> tracksNames;
		unsigned trackToRead;

		// Find tracks
		for (unsigned track = 0; track < tracks; track++) {
			TrackInfo *trackInfo = mkv_GetTrackInfo(file, track);
			if (trackInfo->Type != 0x11) continue;

			// Known subtitle format
			std::string CodecID(trackInfo->CodecID);
			if (CodecID == "S_TEXT/SSA" || CodecID == "S_TEXT/ASS" || CodecID == "S_TEXT/UTF8") {
				tracksFound.push_back(track);
				tracksNames.emplace_back(str(boost::format("%d (%s %s): %s") % track % CodecID % trackInfo->Language % trackInfo->Name));
			}
		}

		// No tracks found
		if (tracksFound.empty())
			throw MatroskaException("File has no recognised subtitle tracks.");

		// Only one track found
		if (tracksFound.size() == 1)
			trackToRead = tracksFound[0];
		// Pick a track
		else {
			int choice = wxGetSingleChoiceIndex(_("Choose which track to read:"), _("Multiple subtitle tracks found"), to_wx(tracksNames));
			if (choice == -1)
				throw agi::UserCancelException("canceled");

			trackToRead = tracksFound[choice];
		}

		// Picked track
		mkv_SetTrackMask(file, ~(1 << trackToRead));
		TrackInfo *trackInfo = mkv_GetTrackInfo(file, trackToRead);
		std::string CodecID(trackInfo->CodecID);
		bool srt = CodecID == "S_TEXT/UTF8";
		bool ssa = CodecID == "S_TEXT/SSA";

		AssParser parser(target, !ssa);

		// Read private data if it's ASS/SSA
		if (!srt) {
			// Read raw data
			std::string priv((const char *)trackInfo->CodecPrivate, trackInfo->CodecPrivateSize);

			// Load into file
			boost::char_separator<char> sep("\r\n");
			for (auto const& cur : boost::tokenizer<boost::char_separator<char>>(priv, sep))
				parser.AddLine(cur);
		}
		// Load default if it's SRT
		else {
			target->LoadDefault(false);
			parser.AddLine("[Events]");
		}

		// Read timecode scale
		SegmentInfo *segInfo = mkv_GetFileInfo(file);
		longlong timecodeScale = mkv_TruncFloat(trackInfo->TimecodeScale) * segInfo->TimecodeScale;

		// Progress bar
		double totalTime = double(segInfo->Duration) / timecodeScale;
		DialogProgress progress(nullptr, _("Parsing Matroska"), _("Reading subtitles from Matroska file."));
		progress.Run([&](agi::ProgressSink *ps) { read_subtitles(ps, file, &input, srt, totalTime, &parser); });
	}
	catch (...) {
		mkv_Close(file);
		throw;
	}
}

bool MatroskaWrapper::HasSubtitles(std::string const& filename) {
	char err[2048];
	try {
		MkvStdIO input(filename);
		MatroskaFile* file = mkv_Open(&input, err, sizeof(err));
		if (!file) return false;

		// Find tracks
		int tracks = mkv_GetNumTracks(file);
		for (int track = 0; track < tracks; track++) {
			TrackInfo *trackInfo = mkv_GetTrackInfo(file, track);

			if (trackInfo->Type == 0x11) {
				std::string CodecID(trackInfo->CodecID);
				if (CodecID == "S_TEXT/SSA" || CodecID == "S_TEXT/ASS" || CodecID == "S_TEXT/UTF8") {
					mkv_Close(file);
					return true;
				}
			}
		}

		mkv_Close(file);
		return false;
	}
	catch (...) {
		// We don't care about why we couldn't read subtitles here
		return false;
	}
}

int StdIoRead(InputStream *_st, ulonglong pos, void *buffer, int count) {
  MkvStdIO *st = (MkvStdIO *) _st;
  size_t  rd;
  if (std_fseek(st->fp, pos, SEEK_SET)) {
    st->error = errno;
    return -1;
  }
  rd = fread(buffer, 1, count, st->fp);
  if (rd == 0) {
    if (feof(st->fp))
      return 0;
    st->error = errno;
    return -1;
  }
  return rd;
}

/// @brief scan for a signature sig(big-endian) starting at file position pos
/// @return position of the first byte of signature or -1 if error/not found
longlong StdIoScan(InputStream *_st, ulonglong start, unsigned signature) {
  MkvStdIO *st = (MkvStdIO *) _st;
  int	      c;
  unsigned    cmp = 0;
  FILE	      *fp = st->fp;

  if (std_fseek(fp, start, SEEK_SET))
    return -1;

  while ((c = getc(fp)) != EOF) {
    cmp = ((cmp << 8) | c) & 0xffffffff;
    if (cmp == signature)
      return std_ftell(fp) - 4;
  }

  return -1;
}

/// @brief This is used to limit readahead.
unsigned StdIoGetCacheSize(InputStream *st) {
  return CACHESIZE;
}

/// @brief Get last error message
const char *StdIoGetLastError(InputStream *st) {
  return strerror(((MkvStdIO *)st)->error);
}

/// @brief Memory allocation, this is done via stdlib
void *StdIoMalloc(InputStream *, size_t size) {
  return malloc(size);
}

void *StdIoRealloc(InputStream *, void *mem, size_t size) {
  return realloc(mem, size);
}

void StdIoFree(InputStream *, void *mem) {
  free(mem);
}

int StdIoProgress(InputStream *, ulonglong cur, ulonglong max) {
  return 1;
}

longlong StdIoGetFileSize(InputStream *_st) {
	MkvStdIO *st = (MkvStdIO *) _st;
	longlong epos = 0;
	longlong cpos = std_ftell(st->fp);
	std_fseek(st->fp, 0, SEEK_END);
	epos = std_ftell(st->fp);
	std_fseek(st->fp, cpos, SEEK_SET);
	return epos;
}

MkvStdIO::MkvStdIO(std::string const& filename)
: error(0)
{
	read = StdIoRead;
	scan = StdIoScan;
	getcachesize = StdIoGetCacheSize;
	geterror = StdIoGetLastError;
	memalloc = StdIoMalloc;
	memrealloc = StdIoRealloc;
	memfree = StdIoFree;
	progress = StdIoProgress;
	getfilesize = StdIoGetFileSize;

#ifdef __VISUALC__
	fp = _wfopen(agi::charset::ConvertW(filename).c_str(), L"rb");
#else
	fp = fopen(filename.c_str(), "rb");
#endif
	if (!fp)
		throw agi::FileNotFoundError(filename);

	setvbuf(fp, nullptr, _IOFBF, CACHESIZE);
}
