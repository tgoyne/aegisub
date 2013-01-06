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
// Aegisub Project http://www.aegisub.org/

/// @file agi_pre.h
/// @brief Precompiled headers include file, including all headers that should be precompiled
/// @ingroup main
///
/// In order to use it, set the project to use this header as precompiled and
/// insert it in every source file (under C/C++ -> Advanced -> Force Includes),
/// then set stdwx.cpp to generate the precompiled header
///
/// @note Make sure that you disable use of precompiled headers on md5.c and
///       MatroskaParser.c, as well as any possible future .c files.

// Block msvc from complaining about not using msvc-specific versions for
// insecure C functions.
#ifdef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS_DEFINED
#else
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "config.h"

#include "../libaegisub/lagi_pre.h"

// General headers
#include <array>
#include <cctype>
#include <fcntl.h>
#include <inttypes.h>
#include <limits>
#include <typeinfo>
#include <utility>

#include <boost/flyweight.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/algorithm_ext.hpp>

// wxWidgets headers
#include <wx/defs.h> // Leave this first.
#include <wx/wx.h>

#include <wx/accel.h>
#include <wx/choicebk.h>
#include <wx/clipbrd.h>
#include <wx/config.h>
#include <wx/dataview.h>
#include <wx/datetime.h>
#include <wx/display.h>
#include <wx/docview.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/fontdlg.h>
#include <wx/gbsizer.h>
#include <wx/glcanvas.h>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/power.h>
#include <wx/rawbmp.h>
#include <wx/regex.h>
#include <wx/sashwin.h>
#include <wx/spinctrl.h>
#include <wx/stackwalk.h>
#include <wx/statline.h>
#include <wx/stc/stc.h>
#include <wx/strconv.h>
#include <wx/sysopt.h>
#include <wx/textfile.h>
#include <wx/tglbtn.h>
#include <wx/thread.h>
#include <wx/treebook.h>
#include <wx/valgen.h>
#include <wx/validate.h>
#include <wx/valnum.h>

#ifdef HAVE_OPENGL_GL_H
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#ifndef _CRT_SECURE_NO_WARNINGS_DEFINED
#undef _CRT_SECURE_NO_WARNINGS
#endif
