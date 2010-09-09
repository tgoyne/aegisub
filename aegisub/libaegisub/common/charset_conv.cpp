// Copyright (c) 2010, Thomas Goyne <plorkyeran@aegisub.org>
//
// Permission to use, copy, modify, and distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
// $Id$

/// @file charset_conv.cpp
/// @brief Wrapper for libiconv to present a more C++-friendly API
/// @ingroup libaegisub

#ifndef LAGI_PRE
#include <assert.h>
#include <errno.h>
#include <stdint.h>

#include <map>
#endif

#include "../config.h"

#include <libaegisub/charset_conv.h>
#include <iconv.h>

// Check if we can use advanced fallback capabilities added in GNU's iconv
// implementation
#if !defined(_LIBICONV_VERSION) || _LIBICONV_VERSION < 0x010A || defined(LIBICONV_PLUG)
#define ICONV_POSIX
#endif

#ifdef AGI_ICONV_CONST
#define ICONV_CONST_CAST(a) a
#else
#define ICONV_CONST_CAST(a) const_cast<char **>(a)
#endif

static const iconv_t iconv_invalid = (iconv_t)-1;
static const size_t iconv_failed = (size_t)-1;

namespace {
	struct ltstr {
		bool operator()(const char* s1, const char* s2) const {
			return strcmp(s1, s2) < 0;
		}
	};
}

/// @brief Map a user-friendly encoding name to the real encoding name
static const char* GetRealEncodingName(const char* name) {
	static std::map<const char*, const char*, ltstr> prettyNames;

	if (prettyNames.empty()) {
#		define ADD(pretty, real) prettyNames[pretty] = real
#		include <libaegisub/charsets.def>
#		undef ADD
	}

	std::map<const char*, const char*, ltstr>::iterator real = prettyNames.find(name);
	if (real != prettyNames.end()) {
		return real->second;
	}
	return name;
}


namespace agi {
	namespace charset {

#ifdef ICONV_POSIX
struct iconv_fallbacks {
};
#endif

class Converter : public iconv_fallbacks {
	bool subst;
	size_t bomSize;
	char invalidRep[8];
	size_t invalidRepSize;
	static void fallback(
		unsigned int code,
		void (*callback) (const char *buf, size_t buflen, void* callback_arg),
		void *callback_arg,
		void *convPtr)
	{
		// At some point in the future, this should probably switch to a real mapping
		// For now, there's just three cases: BOM to nothing, '\' to itself
		// (for Shift-JIS, which does not have \) and everything else to '?'
		if (code == 0xFEFF) return;
		if (code == 0x5C) callback("\\", 1, callback_arg);
		else {
			Converter *self = static_cast<Converter *>(convPtr);
			callback(self->invalidRep, self->invalidRepSize, callback_arg);
		}
	}
public:
	Converter(bool subst, const char* targetEnc)
	: subst(subst)
	{

#ifndef ICONV_POSIX
		data = this;
		mb_to_uc_fallback = NULL;
		mb_to_wc_fallback = NULL;
		uc_to_mb_fallback = fallback;
		wc_to_mb_fallback = NULL;
#endif

		char buff[8];

		iconv_t cd = iconv_open(GetRealEncodingName(targetEnc), "UTF-8");
		assert(cd != iconv_invalid);

		// Most (but not all) iconv implementations automatically insert a BOM
		// at the beginning of text converted to UTF-8, UTF-16 and UTF-32, but
		// we usually don't want this, as some of the wxString using code
		// assumes there is no BOM (as the exact encoding is known externally)
		// As such, when doing conversions we will strip the BOM if it exists,
		// then manually add it when writing files

		const char* src = "";
		char *dst = buff;
		size_t srcLen = 1;
		size_t dstLen = 8;

		size_t res = iconv(cd, ICONV_CONST_CAST(&src), &srcLen, &dst, &dstLen);
		assert(res != iconv_failed);
		assert(srcLen == 0);

		bomSize = 0;
		for (src = buff; src < dst; ++src) {
			if (*src) ++bomSize;
		}
		if (bomSize) {
			// If there is a BOM, it will always be at least as big as the NUL
			bomSize = std::min(bomSize, (8 - dstLen) / 2);
		}

		// Get fallback character
		char sbuff[] = "?";
		src = sbuff;
		dst = invalidRep;
		dstLen = 4;
		srcLen = 1;

		res = operator()(cd, &src, &srcLen, &dst, &dstLen);
		assert(res != iconv_failed);
		assert(srcLen == 0);

		invalidRepSize = 4 - dstLen;

		iconv_close(cd);
	}
	size_t operator()(iconv_t cd, const char** inbuf, size_t* inbytesleft, char** outbuf, size_t* outbytesleft) {
		// If this encoding has a forced BOM (i.e. it's UTF-16 or UTF-32 without
		// a specified byte order), skip over it
		if (bomSize > 0 && inbytesleft && *inbytesleft) {
			// libiconv marks the bom as written after writing the first
			// character after the bom rather than when it writes the bom, so
			// convert at least one extra character
			char bom[8];
			char *dst = bom;
			size_t dstSize = std::min((size_t)8, bomSize + *outbytesleft);
			const char *src = *inbuf;
			size_t srcSize = *inbytesleft;
			iconv(cd, ICONV_CONST_CAST(&src), &srcSize, &dst, &dstSize);
		}
		size_t res = iconv(cd, ICONV_CONST_CAST(inbuf), inbytesleft, outbuf, outbytesleft);

#ifndef ICONV_POSIX
		if (!subst) return res;

		// Save original errno so we can return it rather than the result from iconvctl
		int err = errno;

		// Some characters in the input string do not exist in the output encoding
		if (res == iconv_failed && err == EILSEQ) {
			// first try transliteration only
			int transliterate = 1;
			iconvctl(cd, ICONV_SET_TRANSLITERATE, &transliterate);
			res = iconv(cd, ICONV_CONST_CAST(inbuf), inbytesleft, outbuf, outbytesleft);
			err = errno;
			transliterate = 0;
			iconvctl(cd, ICONV_SET_TRANSLITERATE, &transliterate);
		}
		if (res == iconv_failed && err == EILSEQ) {
			// Conversion still failed with transliteration enabled, so try our substitution
			iconvctl(cd, ICONV_SET_FALLBACKS, this);
			res = iconv(cd, ICONV_CONST_CAST(inbuf), inbytesleft, outbuf, outbytesleft);
			err = errno;
			iconvctl(cd, ICONV_SET_FALLBACKS, NULL);
		}
		if (res == iconv_failed && err == E2BIG && *outbytesleft == 0) {
			// Check for E2BIG false positives
			char buff[8];
			size_t buffsize = 8;
			char* out = buff;
			const char* in = *inbuf;
			size_t insize = *inbytesleft;

			iconvctl(cd, ICONV_SET_FALLBACKS, this);
			res = iconv(cd, ICONV_CONST_CAST(&in), &insize, &out, &buffsize);
			// If no bytes of the output buffer were used, the original
			// conversion may have been successful
			if (buffsize == 8) {
				err = errno;
			}
			else {
				res = iconv_failed;
			}
			iconvctl(cd, ICONV_SET_FALLBACKS, NULL);
		}

		errno = err;
#endif
		return res;
	}
};

// Calculate the size of NUL in the given character set
static size_t NulSize(const char* encoding) {
	// We need a character set to convert from with a known encoding of NUL
	// UTF-8 seems like the obvious choice
	iconv_t cd = iconv_open(GetRealEncodingName(encoding), "UTF-8");
	assert(cd != iconv_invalid);
	Converter conv(false, GetRealEncodingName(encoding));

	char dbuff[4];
	char sbuff[] = "";
	char* dst = dbuff;
	const char* src = sbuff;
	size_t dstLen = sizeof(dbuff);
	size_t srcLen = 1;

	size_t ret = conv(cd, &src, &srcLen, &dst, &dstLen);
	assert(ret != iconv_failed);
	assert(dst - dbuff > 0);
	iconv_close(cd);

	return dst - dbuff;
}

IconvWrapper::IconvWrapper(const char* sourceEncoding, const char* destEncoding, bool enableSubst)
: toNulLen(0)
, fromNulLen(0)
, conv(NULL)
{
	cd = iconv_open(GetRealEncodingName(destEncoding), GetRealEncodingName(sourceEncoding));
	if (cd == iconv_invalid) {
		throw UnsupportedConversion(std::string("Cannot convert from ") + sourceEncoding + " to " + destEncoding);
	}

	// These need to be set only after we verify that the source and des
	// charsets are valid
	toNulLen = NulSize(destEncoding);
	fromNulLen = NulSize(sourceEncoding);
	conv.reset(new Converter(enableSubst, destEncoding));
}
IconvWrapper::~IconvWrapper() {
	if (cd != iconv_invalid) iconv_close(cd);
}

std::string IconvWrapper::Convert(std::string const& source) {
	std::string dest;
	Convert(source, dest);
	return dest;
}
void IconvWrapper::Convert(std::string const& source, std::string &dest) {
	char buff[512];

	const char *src = source.data();
	size_t srcLen = source.size();
	size_t res;
	do {
		char *dst = buff;
		size_t dstLen = sizeof(buff);
		res = (*conv)(cd, &src, &srcLen, &dst, &dstLen);
		if (res == 0) (*conv)(cd, NULL, NULL, &dst, &dstLen);

		dest.append(buff, sizeof(buff) - dstLen);
	} while (res == iconv_failed && errno == E2BIG);
	
	if (res == iconv_failed) {
		switch (errno) {
			case EINVAL:
				throw BadInput(
					"One or more characters in the input string were not valid "
					"characters in the given input encoding");
			case EILSEQ:
				throw BadOutput(
					"One or more characters could not be converted to the "
					"selected target encoding and the version of iconv "
					"Aegisub was built with does not have useful fallbacks. "
					"For best results, please build Aegisub using a recent "
					"version of GNU iconv.");
			default:
				throw ConversionFailure("An unknown conversion failure occurred");
		}
	}
}

size_t IconvWrapper::Convert(const char* source, size_t sourceSize, char *dest, size_t destSize) {
	if (sourceSize == (size_t)-1) {
		sourceSize = SrcStrLen(source);
	}

	
	size_t res = (*conv)(cd, &source, &sourceSize, &dest, &destSize);
	if (res == 0) res = (*conv)(cd, NULL, NULL, &dest, &destSize);

	if (res == iconv_failed) {
		switch (errno) {
			case E2BIG:
				throw BufferTooSmall(
					"Destination buffer was not large enough to fit converted "
					"string.");
			case EINVAL:
				throw BadInput(
					"One or more characters in the input string were not valid "
					"characters in the given input encoding");
			case EILSEQ:
				throw BadOutput(
					"One or more characters could not be converted to the "
					"selected target encoding and the version of iconv "
					"Aegisub was built with does not have useful fallbacks. "
					"For best results, please build Aegisub using a recent "
					"version of GNU iconv.");
			default:
				throw ConversionFailure("An unknown conversion failure occurred");
		}
	}
	return res;
}

size_t IconvWrapper::Convert(const char** source, size_t* sourceSize, char** dest, size_t* destSize) {
	return (*conv)(cd, source, sourceSize, dest, destSize);
}

size_t IconvWrapper::RequiredBufferSize(std::string const& str) {
	return RequiredBufferSize(str.data(), str.size());
}

size_t IconvWrapper::RequiredBufferSize(const char* src, size_t srcLen) {
	char buff[512];
	size_t charsWritten = 0;
	size_t res;

	do {
		char* dst = buff;
		size_t dstSize = sizeof(buff);
		res = (*conv)(cd, &src, &srcLen, &dst, &dstSize);
		(*conv)(cd, NULL, NULL, &dst, &dstSize);

		charsWritten += dst - buff;
	} while (res == iconv_failed && errno == E2BIG);

	if (res == iconv_failed) {
		switch (errno) {
			case EINVAL:
				throw BadInput(
					"One or more characters in the input string were not valid "
					"characters in the given input encoding");
			case EILSEQ:
				throw BadOutput(
					"One or more characters could not be converted to the "
					"selected target encoding and the version of iconv "
					"Aegisub was built with does not have useful fallbacks. "
					"For best results, please build Aegisub using a recent "
					"version of GNU iconv.");
			default:
				throw ConversionFailure("An unknown conversion failure occured");
		}
	}
	return charsWritten;
}

static size_t mbstrlen(const char* str, size_t nulLen) {
	const char *ptr;
	switch (nulLen) {
		case 1:
			return strlen(str);
		case 2:
			for (ptr = str; *reinterpret_cast<const uint16_t *>(ptr) != 0; ptr += 2) ;
			return ptr - str;
		case 4:
			for (ptr = str; *reinterpret_cast<const uint32_t *>(ptr) != 0; ptr += 4) ;
			return ptr - str;
		default:
			return (size_t)-1;
	}
}

size_t IconvWrapper::SrcStrLen(const char* str) {
	return mbstrlen(str, fromNulLen);

}
size_t IconvWrapper::DstStrLen(const char* str) {
	return mbstrlen(str, toNulLen);
}
	}
}
