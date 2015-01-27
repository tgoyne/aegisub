// Copyright (c) 2011, Thomas Goyne <plorkyeran@aegisub.org>
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
// Aegisub Project http://www.aegisub.org/

#include "pen.h"

#include "compat.h"
#include "options.h"

void Pen::OnColourChanged(agi::OptionValue const& opt) {
	impl.SetColour(to_wx(opt.GetColor()));
}

void Pen::OnWidthChanged(agi::OptionValue const& opt) {
	impl.SetWidth(opt.GetInt());
}

Pen::Pen(const char *colour_opt, const char *width_opt, wxPenStyle style)
: impl(to_wx(OPT_GET(colour_opt)->GetColor()), OPT_GET(width_opt)->GetInt(), style)
, colour_con(OPT_SUB(colour_opt, &Pen::OnColourChanged, this))
, width_con(OPT_SUB(width_opt, &Pen::OnWidthChanged, this))
{
}

Pen::Pen(const char *colour_opt, int width, wxPenStyle style)
: impl(to_wx(OPT_GET(colour_opt)->GetColor()), width, style)
, colour_con(OPT_SUB(colour_opt, &Pen::OnColourChanged, this))
{
}

Pen::~Pen() { }
