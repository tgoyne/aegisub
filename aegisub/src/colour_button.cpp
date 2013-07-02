// Copyright (c) 2013, Thomas Goyne <plorkyeran@aegisub.org>
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

#include "config.h"

#include "colour_button.h"

#include "compat.h"
#include "dialog_colorpicker.h"

#include <boost/gil/gil_all.hpp>

wxDEFINE_EVENT(EVT_COLOR, wxThreadEvent);

ColourButton::ColourButton(wxWindow *parent, wxSize const& size, bool alpha, agi::Color col, wxValidator const& validator)
: wxButton(parent, -1, "", wxDefaultPosition, wxSize(size.GetWidth() + 6, size.GetHeight() + 6), 0, validator)
, bmp(size)
, colour(col)
{
	UpdateBitmap();
	Bind(wxEVT_COMMAND_BUTTON_CLICKED, [=](wxCommandEvent&) {
		GetColorFromUser(GetParent(), colour, alpha, [=](agi::Color new_color) {
			colour = new_color;
			UpdateBitmap();

			wxThreadEvent evt(EVT_COLOR, GetId());
			evt.SetEventObject(this);
			evt.SetPayload(colour);
			AddPendingEvent(evt);
		});
	});
}

void ColourButton::UpdateBitmap() {
	using namespace boost::gil;
	fill_pixels(interleaved_view(bmp.GetHeight(), bmp.GetWidth(), (bgr8_pixel_t*)bmp.GetData(), 3 * bmp.GetWidth()),
		bgr8_pixel_t(colour.b, colour.g, colour.r));
	SetBitmapLabel(bmp);
}
