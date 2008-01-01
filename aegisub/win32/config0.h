// Copyright (c) 2007, Rodrigo Braz Monteiro
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


// Enable Automation
// Requires: Lua (in repository)
//#define WITH_AUTOMATION


// Enable Automation 3
// Requires: auto3 dll (in repository), Lua (in repository)
//#define WITH_AUTO3


// Enable DirectShow video provider
// Requires: DirectShow "baseclasses", Platform SDK?
//#define WITH_DIRECTSHOW


// Enable DirectSound audio player
// Requires: PlatformSDK?
//#define WITH_DIRECTSOUND


// Enable FFmpeg video and audio decoders
// Requires: libavcodec and libavformat
//#define WITH_FFMPEG


// Enable Ruby support for Automation
// Requires: Ruby 1.9
//#define WITH_RUBY


// Enable FreeType2 font lister for the fonts collector
// Requires: FreeType2
//#define WITH_FREETYPE2


// Enable Hunspell-based spellchecker
// Requires: hunspell
//#define WITH_HUNSPELL


// Enable FontConfig
// Requires: fontconfig
//#define WITH_FONTCONFIG


// Enable CSRI
// Requires: csri
//#define WITH_CSRI


// Enable libass
// Requires: libass
//#define WITH_LIBASS


// Enable universal charset detector
// Requires: universalcharset (in repository)
//#define WITH_UNIVCHARDET
