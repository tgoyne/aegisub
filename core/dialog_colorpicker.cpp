// Copyright (c) 2005, Niels Martin Hansen
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

#include <wx/image.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/event.h>
#include <wx/tokenzr.h>
#include "dialog_colorpicker.h"
#include "colorspace.h"
#include "ass_style.h"
#include "options.h"


ColorPickerSpectrum::ColorPickerSpectrum(wxWindow *parent, wxWindowID id, wxBitmap *_background, int xx, int yy, PickerDirection _direction)
: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER), x(xx), y(yy), background(_background), direction(_direction)
{
	// empty
}

void ColorPickerSpectrum::GetXY(int &xx, int &yy)
{
	xx = x;
	yy = y;
	Refresh(false);
}

void ColorPickerSpectrum::SetXY(int xx, int yy)
{
	x = xx;
	y = yy;
	Refresh(false);
}

void ColorPickerSpectrum::SetBackground(wxBitmap *new_background)
{
	if (background == new_background) return;
	background = new_background;
	Refresh(false);
}

BEGIN_EVENT_TABLE(ColorPickerSpectrum, wxControl)
	EVT_PAINT(ColorPickerSpectrum::OnPaint)
	EVT_MOUSE_EVENTS(ColorPickerSpectrum::OnMouse)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxSPECTRUM_CHANGE)

void ColorPickerSpectrum::OnPaint(wxPaintEvent &evt)
{
	if (!background) return;

	wxPaintDC dc(this);
	dc.BeginDrawing();

	wxMemoryDC memdc;
	memdc.SelectObject(*background);
	dc.Blit(0, 0, background->GetWidth(), background->GetHeight(), &memdc, 0, 0);

	wxPen pen(dc.GetPen());
	pen.SetWidth(3);
	pen.SetStyle(wxSOLID);
	pen.SetCap(wxCAP_BUTT);
	pen.SetColour(255, 255, 255);
	dc.SetLogicalFunction(wxXOR);
	dc.SetPen(pen);
	switch (direction) {
		case HorzVert:
			// Make a little cross
			dc.DrawLine(x-5, y, x+6, y);
			dc.DrawLine(x, y-5, x, y+6);
			break;
		case Horz:
			// Make a vertical line stretching all the way across
			dc.DrawLine(x, 0, x, GetClientSize().y);
			break;
		case Vert:
			// Make a horizontal line stretching all the way across
			dc.DrawLine(0, y, GetClientSize().x, y);
			break;
	}

	dc.EndDrawing();
}

void ColorPickerSpectrum::OnMouse(wxMouseEvent &evt)
{
	evt.Skip();

	if (!evt.IsButton() && !evt.Dragging()) {
		return;
	}

	int newx = evt.GetX();
	if (newx < 0) newx = 0;
	if (newx >= GetClientSize().x) newx = GetClientSize().x-1;
	int newy = evt.GetY();
	if (newy < 0) newy = 0;
	if (newy >= GetClientSize().y) newy = GetClientSize().y-1;

	if (evt.LeftDown()) {
		CaptureMouse();
	} else if (evt.LeftUp() && HasCapture()) {
		ReleaseMouse();
	}

	if (evt.LeftDown() || (HasCapture() && evt.LeftIsDown())) {
		x = newx;
		y = newy;
		Refresh(false);
		wxCommandEvent evt2(wxSPECTRUM_CHANGE, GetId());
		AddPendingEvent(evt2);
	}
}



ColorPickerRecent::ColorPickerRecent(wxWindow *parent, wxWindowID id, wxSize size, int _cols, int _rows)
: wxControl(parent, id, wxDefaultPosition, size, wxSTATIC_BORDER), rows(_rows), cols(_cols)
{
	LoadFromString(wxEmptyString);
}

void ColorPickerRecent::LoadFromString(const wxString &recent_string)
{
	colors.clear();
	wxStringTokenizer toker(recent_string, _T(" "), false);
	while (toker.HasMoreTokens()) {
		AssColor color;
		color.ParseASS(toker.NextToken());
		colors.push_back(color.GetWXColor());
	}
	while ((int)colors.size() < rows*cols) {
		colors.push_back(*wxBLACK);
	}
}

wxString ColorPickerRecent::StoreToString()
{
	wxString res;
	for (int i = 0; i < rows*cols; i++) {
		AssColor color(colors[i]);
		res << color.GetASSFormatted(false, false, false) << _T(" ");
	}
	res.Trim(true);
	return res;
}

void ColorPickerRecent::AddColor(wxColour color)
{
	for (std::vector<wxColour>::iterator i = colors.begin(); i != colors.end(); i++) {
		if (color == *i) {
			colors.erase(i);
			break;
		}
	}
	colors.insert(colors.begin(), color);
	Refresh(false);
}

BEGIN_EVENT_TABLE(ColorPickerRecent, wxControl)
	EVT_PAINT(ColorPickerRecent::OnPaint)
	EVT_LEFT_DOWN(ColorPickerRecent::OnClick)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxRECENT_SELECT)

void ColorPickerRecent::OnClick(wxMouseEvent &evt)
{
	int cx, cy, i;
	wxSize cs = GetClientSize();
	cx = evt.GetX() * cols / cs.x;
	cy = evt.GetY() * rows / cs.y;
	i = cols*cy + cx;
	if (i >= 0 && i < (int)colors.size()) {
		AssColor color(colors[i]);
		wxCommandEvent evt(wxRECENT_SELECT, GetId());
		evt.SetString(color.GetASSFormatted(false, false, false));
		AddPendingEvent(evt);
	}
}

void ColorPickerRecent::OnPaint(wxPaintEvent &evt)
{
	wxPaintDC dc(this);
	wxBrush brush;
	wxSize cs = GetClientSize();

	int i = 0;
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.BeginDrawing();

	for (int cy = 0; cy < rows; cy++) {
		for (int cx = 0; cx < cols; cx++) {
			int x1, x2, y1, y2;
			x1 = cs.x * cx / cols;
			x2 = cs.x * (cx+1) / cols;
			y1 = cs.y * cy / rows;
			y2 = cs.y * (cy+1) / rows;

			dc.SetBrush(wxBrush(colors[i]));
			dc.DrawRectangle(x1, y1, x2-x1, y2-y1);

			i++;
		}
	}

	dc.EndDrawing();
}



ColorPickerScreenDropper::ColorPickerScreenDropper(wxWindow *parent, wxWindowID id, int _resx, int _resy, int _magnification)
: wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxSTATIC_BORDER), resx(_resx), resy(_resy), magnification(_magnification)
{
	SetClientSize(resx*magnification, resy*magnification);
	SetMinSize(GetSize());
	SetMaxSize(GetSize());

	capture = wxBitmap(resx, resy);
	wxMemoryDC capdc;
	capdc.SelectObject(capture);
	capdc.SetPen(*wxTRANSPARENT_PEN);
	capdc.SetBrush(*wxWHITE_BRUSH);
	capdc.DrawRectangle(0, 0, resx, resy);
}

BEGIN_EVENT_TABLE(ColorPickerScreenDropper, wxControl)
	EVT_PAINT(ColorPickerScreenDropper::OnPaint)
	EVT_MOUSE_EVENTS(ColorPickerScreenDropper::OnMouse)
END_EVENT_TABLE()

DEFINE_EVENT_TYPE(wxDROPPER_SELECT)

void ColorPickerScreenDropper::OnMouse(wxMouseEvent &evt)
{
	int x, y;
	x = evt.GetX() / magnification;
	y = evt.GetY() / magnification;

	if (HasCapture() && evt.LeftIsDown()) {

		wxMemoryDC capdc;
		capdc.SelectObject(capture);
		wxScreenDC screen;

		wxPoint pos = ClientToScreen(evt.GetPosition());

		screen.StartDrawingOnTop();
		capdc.Blit(0, 0, resx, resy, &screen, pos.x-resx/2, pos.y-resy/2);
		screen.EndDrawingOnTop();

		Refresh(false);

	} else if (evt.LeftDown()) {

		if (x == 0 && y == 0) {
			SetCursor(*wxCROSS_CURSOR);
			CaptureMouse();

		} else if (x >= 0 && y >= 0 && x < resx && y < resy) {
			wxMemoryDC capdc;
			capdc.SelectObject(capture);
			wxColour color;
			capdc.GetPixel(x, y, &color);
			AssColor ass(color);
			wxCommandEvent evt(wxDROPPER_SELECT, GetId());
			evt.SetString(ass.GetASSFormatted(false, false, false));
			AddPendingEvent(evt);
		}

	} else if (HasCapture() && evt.LeftUp()) {
		ReleaseMouse();
		SetCursor(wxNullCursor);
	}
}

void ColorPickerScreenDropper::OnPaint(wxPaintEvent &evt)
{
	wxPaintDC pdc(this);
	wxMemoryDC capdc;
	capdc.SelectObject(capture);

	pdc.SetPen(*wxTRANSPARENT_PEN);

	for (int x = 0; x < resx; x++) {
		for (int y = 0; y < resy; y++) {
			if (x==0 && y==0) continue;

			wxColour color;
			capdc.GetPixel(x, y, &color);
			pdc.SetBrush(wxBrush(color));

			pdc.DrawRectangle(x*magnification, y*magnification, magnification, magnification);
		}
	}

	wxBrush cbrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	pdc.SetBrush(cbrush);
	pdc.DrawRectangle(0, 0, magnification, magnification);
	cbrush.SetStyle(wxCROSSDIAG_HATCH);
	cbrush.SetColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
	pdc.SetBrush(cbrush);
	pdc.DrawRectangle(0, 0, magnification, magnification);
}



wxColour GetColorFromUser(wxWindow *parent, wxColour original)
{
	DialogColorPicker dialog(parent, original);
	if (dialog.ShowModal() == wxID_OK) {
		return dialog.GetColor();
	} else {
		return original;
	}
}


// Constructor
DialogColorPicker::DialogColorPicker(wxWindow *parent, wxColour initial_color)
: wxDialog(parent, -1, _("Select Color"), wxDefaultPosition, wxDefaultSize)
{
	rgb_spectrum[0] =
	rgb_spectrum[1] =
	rgb_spectrum[2] =
	yuv_spectrum    =
	hsl_spectrum    =
	hsv_spectrum    = 0;
	spectrum_dirty = true;

	// generate spectrum slider bar images
	wxImage sliderimg(slider_width, 256, true);
	unsigned char *oslid, *slid;

	// red
	oslid = slid = (unsigned char *)malloc(slider_width*256*3);
	for (int  y = 0; y < 256; y++) {
		for (int x = 0; x < slider_width; x++) {
			*slid++ = clip_colorval(y);
			*slid++ = 0;
			*slid++ = 0;
		}
	}
	sliderimg.SetData(oslid);
	rgb_slider[0] = new wxBitmap(sliderimg);

	// green
	oslid = slid = (unsigned char *)malloc(slider_width*256*3);
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < slider_width; x++) {
			*slid++ = 0;
			*slid++ = clip_colorval(y);
			*slid++ = 0;
		}
	}
	sliderimg.SetData(oslid);
	rgb_slider[1] = new wxBitmap(sliderimg);

	// blue
	oslid = slid = (unsigned char *)malloc(slider_width*256*3);
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < slider_width; x++) {
			*slid++ = 0;
			*slid++ = 0;
			*slid++ = clip_colorval(y);
		}
	}
	sliderimg.SetData(oslid);
	rgb_slider[2] = new wxBitmap(sliderimg);

	// Y/luminance
	oslid = slid = (unsigned char *)malloc(slider_width*256*3);
	for (int y = 0; y < 256; y++) {
		int x = 0;
		for (; x < slider_width; x++) {
			*slid++ = clip_colorval(y);
			*slid++ = clip_colorval(y);
			*slid++ = clip_colorval(y);
		}
		/*for (x*=3; x < slider_width*3; x++) {
			*slid++ = 0;
		}*/
	}
	sliderimg.SetData(oslid);
	hsl_slider = yuv_slider = new wxBitmap(sliderimg);

	oslid = slid = (unsigned char *)malloc(slider_width*256*3);
	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < slider_width; x++) {
			/*int r, g, b;
			hsv_to_rgb(y, 128, 255, &r, &g, &b);
			*slid++ = r;
			*slid++ = g;
			*slid++ = b;*/
			hsv_to_rgb(y, 255, 255, slid, slid+1, slid+2);
			slid += 3;
		}
	}
	sliderimg.SetData(oslid);
	hsv_slider = new wxBitmap(sliderimg);

	// Create the controls for the dialog
	wxSizer *spectrum_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Color spectrum"));
	spectrum = new ColorPickerSpectrum(this, SELECTOR_SPECTRUM, 0, -1, -1, ColorPickerSpectrum::HorzVert);
	spectrum->SetClientSize(256, 256);
	spectrum->SetMinSize(spectrum->GetSize());
	slider = new ColorPickerSpectrum(this, SELECTOR_SLIDER, 0, -1, -1, ColorPickerSpectrum::Vert);
	slider->SetClientSize(wxSize(slider_width, 256));
	slider->SetMinSize(slider->GetSize());
	wxString modes[] = { _("RGB/R"), _("RGB/G"), _("RGB/B"), _("YUV/Y"), _("HSL/L"), _("HSV/H") };
	colorspace_choice = new wxChoice(this, SELECTOR_MODE, wxDefaultPosition, wxDefaultSize, 6, modes);

	wxSize colorinput_size(70, -1);
	wxSize colorinput_labelsize(40, -1);

	wxSizer *rgb_box = new wxStaticBoxSizer(wxVERTICAL, this, _("RGB color"));
	rgb_input[0] = new wxSpinCtrl(this, SELECTOR_RGB_R, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	rgb_input[1] = new wxSpinCtrl(this, SELECTOR_RGB_G, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	rgb_input[2] = new wxSpinCtrl(this, SELECTOR_RGB_B, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);

	wxSizer *yuv_box = new wxStaticBoxSizer(wxVERTICAL, this, _("YUV color"));
	yuv_input[0] = new wxSpinCtrl(this, SELECTOR_YUV_Y, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	yuv_input[1] = new wxSpinCtrl(this, SELECTOR_YUV_U, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	yuv_input[2] = new wxSpinCtrl(this, SELECTOR_YUV_V, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);

	wxSizer *hsl_box = new wxStaticBoxSizer(wxVERTICAL, this, _("HSL color"));
	hsl_input[0] = new wxSpinCtrl(this, SELECTOR_HSL_H, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	hsl_input[1] = new wxSpinCtrl(this, SELECTOR_HSL_S, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	hsl_input[2] = new wxSpinCtrl(this, SELECTOR_HSL_L, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);

	wxSizer *hsv_box = new wxStaticBoxSizer(wxVERTICAL, this, _("HSV color"));
	hsv_input[0] = new wxSpinCtrl(this, SELECTOR_HSV_H, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	hsv_input[1] = new wxSpinCtrl(this, SELECTOR_HSV_S, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);
	hsv_input[2] = new wxSpinCtrl(this, SELECTOR_HSV_V, _T(""), wxDefaultPosition, colorinput_size, wxSP_ARROW_KEYS, 0, 255);

	ass_input = new wxTextCtrl(this, SELECTOR_ASS_INPUT, _T(""), wxDefaultPosition, colorinput_size);
	html_input = new wxTextCtrl(this, SELECTOR_HTML_INPUT, _T(""), wxDefaultPosition, colorinput_size);

	preview_bitmap = wxBitmap(40, 40, 24);
	preview_box = new wxStaticBitmap(this, -1, preview_bitmap, wxDefaultPosition, wxSize(40, 40), wxSTATIC_BORDER);
	//preview_box = new wxControl(this, -1, wxDefaultPosition, wxSize(40, 40), wxSTATIC_BORDER);
	//preview_box->Connect(wxEVT_PAINT, (wxObjectEventFunction)DialogColorPicker::OnPreviewBoxPaint);

	recent_box = new ColorPickerRecent(this, SELECTOR_RECENT, wxDefaultSize, 12, 2);
	recent_box->SetClientSize(12*16, 2*16);
	recent_box->SetMinSize(recent_box->GetSize());
	recent_box->SetMaxSize(recent_box->GetSize());

	screen_dropper = new ColorPickerScreenDropper(this, SELECTOR_DROPPER, 7, 7, 8);

	ok_button = new wxButton(this, wxID_OK);
	cancel_button = new wxButton(this, wxID_CANCEL);

	// Arrange the controls in a nice way
	wxSizer *spectop_sizer = new wxBoxSizer(wxHORIZONTAL);
	spectop_sizer->Add(new wxStaticText(this, -1, _("Spectrum mode:")), 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxRIGHT, 5);
	spectop_sizer->Add(colorspace_choice, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT);
	spectop_sizer->Add(5, 5, 1, wxEXPAND);
	spectop_sizer->Add(preview_box, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT);
	wxSizer *spectrum_sizer = new wxFlexGridSizer(2, 2, 5, 5);
	spectrum_sizer->Add(spectop_sizer, wxEXPAND);
	spectrum_sizer->AddStretchSpacer(1);
	spectrum_sizer->Add(spectrum);
	spectrum_sizer->Add(slider);
	spectrum_box->Add(spectrum_sizer, 0, wxALL, 3);

	wxSizer *rgb_sizer = new wxFlexGridSizer(3, 2, 5, 5);
	rgb_sizer->Add(new wxStaticText(this, -1, _("Red:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	rgb_sizer->Add(rgb_input[0], 0);
	rgb_sizer->Add(new wxStaticText(this, -1, _("Green:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	rgb_sizer->Add(rgb_input[1], 0);
	rgb_sizer->Add(new wxStaticText(this, -1, _("Blue:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	rgb_sizer->Add(rgb_input[2], 0);
	rgb_box->Add(rgb_sizer, 0, wxALL, 3);

	wxSizer *yuv_sizer = new wxFlexGridSizer(3, 2, 5, 5);
	yuv_sizer->Add(new wxStaticText(this, -1, _("Y:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	yuv_sizer->Add(yuv_input[0], 0);
	yuv_sizer->Add(new wxStaticText(this, -1, _("U:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	yuv_sizer->Add(yuv_input[1], 0);
	yuv_sizer->Add(new wxStaticText(this, -1, _("V:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	yuv_sizer->Add(yuv_input[2], 0);
	yuv_box->Add(yuv_sizer, 0, wxALL, 3);

	wxSizer *hsl_sizer = new wxFlexGridSizer(3, 2, 5, 5);
	hsl_sizer->Add(new wxStaticText(this, -1, _("Hue:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsl_sizer->Add(hsl_input[0], 0);
	hsl_sizer->Add(new wxStaticText(this, -1, _("Sat.:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsl_sizer->Add(hsl_input[1], 0);
	hsl_sizer->Add(new wxStaticText(this, -1, _("Lum.:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsl_sizer->Add(hsl_input[2], 0);
	hsl_box->Add(hsl_sizer, 0, wxALL, 3);

	wxSizer *hsv_sizer = new wxFlexGridSizer(3, 2, 5, 5);
	hsv_sizer->Add(new wxStaticText(this, -1, _("Hue:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsv_sizer->Add(hsv_input[0], 0);
	hsv_sizer->Add(new wxStaticText(this, -1, _("Sat.:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsv_sizer->Add(hsv_input[1], 0);
	hsv_sizer->Add(new wxStaticText(this, -1, _("Value:"), wxDefaultPosition, colorinput_labelsize), 0, wxALIGN_CENTER_VERTICAL);
	hsv_sizer->Add(hsv_input[2], 0);
	hsv_box->Add(hsv_sizer, 0, wxALL, 3);

	wxSizer *ass_input_sizer = new wxBoxSizer(wxHORIZONTAL);
	ass_input_sizer->Add(new wxStaticText(this, -1, _T("ASS:")), 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	ass_input_sizer->Add(ass_input, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10);
	ass_input_sizer->Add(new wxStaticText(this, -1, _T("HTML:")), 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 5);
	ass_input_sizer->Add(html_input, 0, wxRIGHT|wxALIGN_CENTER_VERTICAL, 10);

	wxStdDialogButtonSizer *button_sizer = new wxStdDialogButtonSizer();
	button_sizer->AddButton(ok_button);
	button_sizer->AddButton(cancel_button);
	button_sizer->Realize();

	wxGridBagSizer *input_sizer = new wxGridBagSizer(5, 5);
	input_sizer->Add(rgb_box, wxGBPosition(0, 0), wxGBSpan(1, 2), wxALIGN_CENTER);
	input_sizer->Add(yuv_box, wxGBPosition(1, 0), wxGBSpan(1, 2), wxALIGN_CENTER);
	input_sizer->Add(hsl_box, wxGBPosition(0, 2), wxGBSpan(1, 2), wxALIGN_CENTER);
	input_sizer->Add(hsv_box, wxGBPosition(1, 2), wxGBSpan(1, 2), wxALIGN_CENTER);
	input_sizer->Add(ass_input_sizer, wxGBPosition(2, 0), wxGBSpan(1, 4), wxALIGN_CENTER);
	input_sizer->Add(screen_dropper, wxGBPosition(3, 0), wxGBSpan(2, 1), wxALIGN_CENTER);
	input_sizer->Add(recent_box, wxGBPosition(3, 1), wxGBSpan(1, 3), wxALIGN_CENTER);
	input_sizer->Add(button_sizer, wxGBPosition(4, 1), wxGBSpan(1, 3), wxALIGN_RIGHT|wxALIGN_BOTTOM);

	wxSizer *main_sizer = new wxBoxSizer(wxHORIZONTAL);
	main_sizer->Add(spectrum_box, 0, wxALL, 5);
	main_sizer->Add(input_sizer, 0, wxALL&~wxLEFT, 5);

	SetSizer(main_sizer);
	main_sizer->SetSizeHints(this);
	CentreOnParent();

	// Fill the controls
	updating_controls = false;
	int mode = Options.AsInt(_T("Color Picker Mode"));
	if (mode < 0 || mode > 5) mode = 4; // HSL default
	colorspace_choice->SetSelection(mode);
	SetColor(initial_color);
	recent_box->LoadFromString(Options.AsText(_T("Color Picker Recent")));
}


// Destructor
DialogColorPicker::~DialogColorPicker()
{
	delete rgb_spectrum[0];
	delete rgb_spectrum[1];
	delete rgb_spectrum[2];
	delete yuv_spectrum;
	delete hsl_spectrum;
	delete hsv_spectrum;
	delete rgb_slider[0];
	delete rgb_slider[1];
	delete rgb_slider[2];
	delete yuv_slider;
	// hsl_slider points to the same bitmap as yuv_slider, so don't try to delete
	delete hsv_slider;
}


// Sets the currently selected color, and updates all controls
void DialogColorPicker::SetColor(wxColour new_color)
{
	cur_color = new_color;
	rgb_input[0]->SetValue(new_color.Red());
	rgb_input[1]->SetValue(new_color.Green());
	rgb_input[2]->SetValue(new_color.Blue());
	UpdateFromRGB();
}


// Get the currently selected color
wxColour DialogColorPicker::GetColor()
{
	recent_box->AddColor(cur_color);
	Options.SetText(_T("Color Picker Recent"), recent_box->StoreToString());
	Options.Save();
	return cur_color;
}


// Use the values entered in the RGB controls to update the other controls
void DialogColorPicker::UpdateFromRGB()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int h2, s2, v2;
	r = rgb_input[0]->GetValue();
	g = rgb_input[1]->GetValue();
	b = rgb_input[2]->GetValue();
	rgb_to_yuv(r, g, b, &y, &u, &v);
	rgb_to_hsl(r, g, b, &h, &s, &l);
	rgb_to_hsv(r, g, b, &h2, &s2, &v2);
	yuv_input[0]->SetValue(y);
	yuv_input[1]->SetValue(u);
	yuv_input[2]->SetValue(v);
	hsl_input[0]->SetValue(h);
	hsl_input[1]->SetValue(s);
	hsl_input[2]->SetValue(l);
	hsv_input[0]->SetValue(h2);
	hsv_input[1]->SetValue(s2);
	hsv_input[2]->SetValue(v2);
	cur_color = wxColor(r, g, b);
	ass_input->SetValue(AssColor(cur_color).GetASSFormatted(false, false, false));
	html_input->SetValue(color_to_html(cur_color));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


// Use the values entered in the YUV controls to update the other controls
void DialogColorPicker::UpdateFromYUV()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int h2, s2, v2;
	y = yuv_input[0]->GetValue();
	u = yuv_input[1]->GetValue();
	v = yuv_input[2]->GetValue();
	yuv_to_rgb(y, u, v, &r, &g, &b);
	rgb_to_hsl(r, g, b, &h, &s, &l);
	rgb_to_hsv(r, g, b, &h2, &s2, &v2);
	rgb_input[0]->SetValue(r);
	rgb_input[1]->SetValue(g);
	rgb_input[2]->SetValue(b);
	hsl_input[0]->SetValue(h);
	hsl_input[1]->SetValue(s);
	hsl_input[2]->SetValue(l);
	hsv_input[0]->SetValue(h2);
	hsv_input[1]->SetValue(s2);
	hsv_input[2]->SetValue(v2);
	cur_color = wxColor(r, g, b);
	ass_input->SetValue(AssColor(cur_color).GetASSFormatted(false, false, false));
	html_input->SetValue(color_to_html(cur_color));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


// Use the values entered in the HSL controls to update the other controls
void DialogColorPicker::UpdateFromHSL()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int h2, s2, v2;
	h = hsl_input[0]->GetValue();
	s = hsl_input[1]->GetValue();
	l = hsl_input[2]->GetValue();
	hsl_to_rgb(h, s, l, &r, &g, &b);
	rgb_to_yuv(r, g, b, &y, &u, &v);
	rgb_to_hsv(r, g, b, &h2, &s2, &v2);
	rgb_input[0]->SetValue(r);
	rgb_input[1]->SetValue(g);
	rgb_input[2]->SetValue(b);
	yuv_input[0]->SetValue(y);
	yuv_input[1]->SetValue(u);
	yuv_input[2]->SetValue(v);
	hsv_input[0]->SetValue(h2);
	hsv_input[1]->SetValue(s2);
	hsv_input[2]->SetValue(v2);
	cur_color = wxColor(r, g, b);
	ass_input->SetValue(AssColor(cur_color).GetASSFormatted(false, false, false));
	html_input->SetValue(color_to_html(cur_color));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


void DialogColorPicker::UpdateFromHSV()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int r, g, b, h2, s2, v2;
	h2 = hsv_input[0]->GetValue();
	s2 = hsv_input[1]->GetValue();
	v2 = hsv_input[2]->GetValue();
	hsv_to_rgb(h2, s2, v2, &r, &g, &b);
	rgb_to_yuv(r, g, b, &y, &u, &v);
	rgb_to_hsl(r, g, b, &h, &s, &l);
	rgb_input[0]->SetValue(r);
	rgb_input[1]->SetValue(g);
	rgb_input[2]->SetValue(b);
	yuv_input[0]->SetValue(y);
	yuv_input[1]->SetValue(u);
	yuv_input[2]->SetValue(v);
	hsl_input[0]->SetValue(h);
	hsl_input[1]->SetValue(s);
	hsl_input[2]->SetValue(l);
	cur_color = wxColor(r, g, b);
	ass_input->SetValue(AssColor(cur_color).GetASSFormatted(false, false, false));
	html_input->SetValue(color_to_html(cur_color));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


// Use the value entered in the ASS hex control to update the other controls
void DialogColorPicker::UpdateFromASS()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int h2, s2, v2;
	AssColor ass;
	ass.ParseASS(ass_input->GetValue());
	r = ass.r;
	g = ass.g;
	b = ass.b;
	rgb_to_yuv(r, g, b, &y, &u, &v);
	rgb_to_hsl(r, g, b, &h, &s, &l);
	rgb_to_hsv(r, g, b, &h2, &s2, &v2);
	rgb_input[0]->SetValue(r);
	rgb_input[1]->SetValue(g);
	rgb_input[2]->SetValue(b);
	yuv_input[0]->SetValue(y);
	yuv_input[1]->SetValue(u);
	yuv_input[2]->SetValue(v);
	hsl_input[0]->SetValue(h);
	hsl_input[1]->SetValue(s);
	hsl_input[2]->SetValue(l);
	hsv_input[0]->SetValue(h2);
	hsv_input[1]->SetValue(s2);
	hsv_input[2]->SetValue(v2);
	cur_color = wxColor(r, g, b);
	html_input->SetValue(color_to_html(cur_color));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


void DialogColorPicker::UpdateFromHTML()
{
	if (updating_controls) return;
	updating_controls = true;

	unsigned char r, g, b, y, u, v, h, s, l, h2, s2, v2;
	//int h2, s2, v2;
	cur_color = html_to_color(html_input->GetValue());
	r = cur_color.Red();
	g = cur_color.Green();
	b = cur_color.Blue();
	rgb_to_yuv(r, g, b, &y, &u, &v);
	rgb_to_hsl(r, g, b, &h, &s, &l);
	rgb_to_hsv(r, g, b, &h2, &s2, &v2);
	rgb_input[0]->SetValue(r);
	rgb_input[1]->SetValue(g);
	rgb_input[2]->SetValue(b);
	yuv_input[0]->SetValue(y);
	yuv_input[1]->SetValue(u);
	yuv_input[2]->SetValue(v);
	hsl_input[0]->SetValue(h);
	hsl_input[1]->SetValue(s);
	hsl_input[2]->SetValue(l);
	hsv_input[0]->SetValue(h2);
	hsv_input[1]->SetValue(s2);
	hsv_input[2]->SetValue(v2);
	cur_color = wxColor(r, g, b);
	ass_input->SetValue(AssColor(cur_color).GetASSFormatted(false, false, false));
	UpdateSpectrumDisplay();

	updating_controls = false;
}


void DialogColorPicker::UpdateSpectrumDisplay()
{
	int i = colorspace_choice->GetSelection();
	switch (i) {
		case 0:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeGBSpectrum());
			slider->SetBackground(rgb_slider[0]);
			slider->SetXY(0, rgb_input[0]->GetValue());
			spectrum->SetXY(rgb_input[2]->GetValue(), rgb_input[1]->GetValue());
			break;
		case 1:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeRBSpectrum());
			slider->SetBackground(rgb_slider[1]);
			slider->SetXY(0, rgb_input[1]->GetValue());
			spectrum->SetXY(rgb_input[2]->GetValue(), rgb_input[0]->GetValue());
			break;
		case 2:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeRGSpectrum());
			slider->SetBackground(rgb_slider[2]);
			slider->SetXY(0, rgb_input[2]->GetValue());
			spectrum->SetXY(rgb_input[1]->GetValue(), rgb_input[0]->GetValue());
			break;
		case 3:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeUVSpectrum());
			slider->SetBackground(yuv_slider);
			slider->SetXY(0, yuv_input[0]->GetValue());
			spectrum->SetXY(yuv_input[2]->GetValue(), yuv_input[1]->GetValue());
			break;
		case 4:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeHSSpectrum());
			slider->SetBackground(hsl_slider);
			slider->SetXY(0, hsl_input[2]->GetValue());
			spectrum->SetXY(hsl_input[1]->GetValue(), hsl_input[0]->GetValue());
			break;
		case 5:
			if (spectrum_dirty)
				spectrum->SetBackground(MakeSVSpectrum());
			slider->SetBackground(hsv_slider);
			slider->SetXY(0, hsv_input[0]->GetValue());
			spectrum->SetXY(hsv_input[1]->GetValue(), hsv_input[2]->GetValue());
			break;
	}
	spectrum_dirty = false;

	wxMemoryDC previewdc;
	previewdc.SelectObject(preview_bitmap);
	previewdc.SetPen(*wxTRANSPARENT_PEN);
	wxBrush brush(previewdc.GetBrush());
	brush.SetColour(cur_color);
	previewdc.SetBrush(brush);
	previewdc.DrawRectangle(0, 0, 40, 40);
	preview_box->Refresh(false);
}


wxBitmap *DialogColorPicker::MakeGBSpectrum()
{
	if (rgb_spectrum[0]) delete rgb_spectrum[0];

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);
	for (int g = 0; g < 256; g++) {
		for (int b = 0; b < 256; b++) {
			*spec++ = cur_color.Red();
			*spec++ = g;
			*spec++ = b;
		}
	}
	spectrum_image.SetData(ospec);
	rgb_spectrum[0] = new wxBitmap(spectrum_image);

	return rgb_spectrum[0];
}


wxBitmap *DialogColorPicker::MakeRBSpectrum()
{
	if (rgb_spectrum[1]) delete rgb_spectrum[1];

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);
	for (int r = 0; r < 256; r++) {
		for (int b = 0; b < 256; b++) {
			*spec++ = r;
			*spec++ = cur_color.Green();
			*spec++ = b;
		}
	}
	spectrum_image.SetData(ospec);
	rgb_spectrum[1] = new wxBitmap(spectrum_image);

	return rgb_spectrum[1];
}


wxBitmap *DialogColorPicker::MakeRGSpectrum()
{
	if (rgb_spectrum[2]) delete rgb_spectrum[2];

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);
	for (int r = 0; r < 256; r++) {
		for (int g = 0; g < 256; g++) {
			*spec++ = r;
			*spec++ = g;
			*spec++ = cur_color.Blue();
		}
	}
	spectrum_image.SetData(ospec);
	rgb_spectrum[2] = new wxBitmap(spectrum_image);

	return rgb_spectrum[2];
}


wxBitmap *DialogColorPicker::MakeUVSpectrum()
{
	if (yuv_spectrum) delete yuv_spectrum;

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);
	int y = yuv_input[0]->GetValue();
	for (int u = -128; u < 128; u++) {
		int r, g, b, v = -128;
		r = (y<<20)                          + int(1.140*(1<<20)) * v;
		g = (y<<20) - int(0.395*(1<<20)) * u - int(0.581*(1<<20)) * v;
		b = (y<<20) + int(2.032*(1<<20)) * u;
		for (; v < 128; v++) {
			//yuv_to_rgb(yuv_input[0]->GetValue(), u, v, spec, spec+1, spec+2);
			//spec += 3;
			*spec++ = clip_colorval(r >> 20);
			*spec++ = clip_colorval(g >> 20);
			*spec++ = clip_colorval(b >> 20);
			r += int(1.140*(1<<20));
			g -= int(0.581*(1<<20));
		}
	}
	spectrum_image.SetData(ospec);
	yuv_spectrum = new wxBitmap(spectrum_image);

	return yuv_spectrum;
}


wxBitmap *DialogColorPicker::MakeHSSpectrum()
{
	if (hsl_spectrum) delete hsl_spectrum;

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);
	int l = hsl_input[2]->GetValue();

	for (int h = 0; h < 256; h++) {
		unsigned char maxr, maxg, maxb;
		hsl_to_rgb(h, 255, l, &maxr, &maxg, &maxb);

		for (int s = 0; s < 256; s++) {
			//hsl_to_rgb(h, s, l, spec, spec+1, spec+2);
			//spec += 3;
			*spec++ = maxr * s / 256 + (255-s) * l / 256;
			*spec++ = maxg * s / 256 + (255-s) * l / 256;
			*spec++ = maxb * s / 256 + (255-s) * l / 256;
		}
	}
	spectrum_image.SetData(ospec);
	hsl_spectrum = new wxBitmap(spectrum_image);

	return hsl_spectrum;
}


wxBitmap *DialogColorPicker::MakeSVSpectrum()
{
	if (hsv_spectrum) delete hsv_spectrum;

	wxImage spectrum_image(256, 256, false);
	unsigned char *ospec, *spec;

	ospec = spec = (unsigned char *)malloc(256*256*3);

	int h = hsv_input[0]->GetValue();
	unsigned char maxr, maxg, maxb;
	hsv_to_rgb(h, 255, 255, &maxr, &maxg, &maxb);

	for (int v = 0; v < 256; v++) {
		int rr, rg, rb;
		rr = (255-maxr) * v / 256;
		rg = (255-maxg) * v / 256;
		rb = (255-maxb) * v / 256;
		for (int s = 0; s < 256; s++) {
			//hsv_to_rgb(h, s, v, spec, spec+1, spec+2);
			//spec += 3;
			int r, g, b;
			r = 255 - rr * s / 256 - (255-v);
			g = 255 - rg * s / 256 - (255-v);
			b = 255 - rb * s / 256 - (255-v);
			*spec++ = r;
			*spec++ = g;
			*spec++ = b;
		}
	}
	spectrum_image.SetData(ospec);
	hsv_spectrum = new wxBitmap(spectrum_image);

	return hsv_spectrum;
}



BEGIN_EVENT_TABLE(DialogColorPicker, wxDialog)
	EVT_SPINCTRL(SELECTOR_RGB_R, DialogColorPicker::OnSpinRGB)
	EVT_SPINCTRL(SELECTOR_RGB_G, DialogColorPicker::OnSpinRGB)
	EVT_SPINCTRL(SELECTOR_RGB_B, DialogColorPicker::OnSpinRGB)
	EVT_SPINCTRL(SELECTOR_YUV_Y, DialogColorPicker::OnSpinYUV)
	EVT_SPINCTRL(SELECTOR_YUV_U, DialogColorPicker::OnSpinYUV)
	EVT_SPINCTRL(SELECTOR_YUV_V, DialogColorPicker::OnSpinYUV)
	EVT_SPINCTRL(SELECTOR_HSL_H, DialogColorPicker::OnSpinHSL)
	EVT_SPINCTRL(SELECTOR_HSL_S, DialogColorPicker::OnSpinHSL)
	EVT_SPINCTRL(SELECTOR_HSL_L, DialogColorPicker::OnSpinHSL)
	EVT_SPINCTRL(SELECTOR_HSV_H, DialogColorPicker::OnSpinHSV)
	EVT_SPINCTRL(SELECTOR_HSV_S, DialogColorPicker::OnSpinHSV)
	EVT_SPINCTRL(SELECTOR_HSV_V, DialogColorPicker::OnSpinHSV)
	EVT_TEXT(SELECTOR_RGB_R, DialogColorPicker::OnChangeRGB)
	EVT_TEXT(SELECTOR_RGB_G, DialogColorPicker::OnChangeRGB)
	EVT_TEXT(SELECTOR_RGB_B, DialogColorPicker::OnChangeRGB)
	EVT_TEXT(SELECTOR_YUV_Y, DialogColorPicker::OnChangeYUV)
	EVT_TEXT(SELECTOR_YUV_U, DialogColorPicker::OnChangeYUV)
	EVT_TEXT(SELECTOR_YUV_V, DialogColorPicker::OnChangeYUV)
	EVT_TEXT(SELECTOR_HSL_H, DialogColorPicker::OnChangeHSL)
	EVT_TEXT(SELECTOR_HSL_S, DialogColorPicker::OnChangeHSL)
	EVT_TEXT(SELECTOR_HSL_L, DialogColorPicker::OnChangeHSL)
	EVT_TEXT(SELECTOR_HSV_H, DialogColorPicker::OnChangeHSV)
	EVT_TEXT(SELECTOR_HSV_S, DialogColorPicker::OnChangeHSV)
	EVT_TEXT(SELECTOR_HSV_V, DialogColorPicker::OnChangeHSV)
	EVT_TEXT(SELECTOR_ASS_INPUT, DialogColorPicker::OnChangeASS)
	EVT_TEXT(SELECTOR_HTML_INPUT, DialogColorPicker::OnChangeHTML)
	EVT_CHOICE(SELECTOR_MODE, DialogColorPicker::OnChangeMode)
	EVT_COMMAND(SELECTOR_SPECTRUM, wxSPECTRUM_CHANGE, DialogColorPicker::OnSpectrumChange)
	EVT_COMMAND(SELECTOR_SLIDER, wxSPECTRUM_CHANGE, DialogColorPicker::OnSliderChange)
	EVT_COMMAND(SELECTOR_RECENT, wxRECENT_SELECT, DialogColorPicker::OnRecentSelect)
	EVT_COMMAND(SELECTOR_DROPPER, wxDROPPER_SELECT, DialogColorPicker::OnRecentSelect)
END_EVENT_TABLE()


void DialogColorPicker::OnSpinRGB(wxSpinEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromRGB();
}


void DialogColorPicker::OnSpinYUV(wxSpinEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromYUV();
}


void DialogColorPicker::OnSpinHSL(wxSpinEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromHSL();
}


void DialogColorPicker::OnSpinHSV(wxSpinEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromHSV();
}


void DialogColorPicker::OnChangeRGB(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromRGB();
}


void DialogColorPicker::OnChangeYUV(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromYUV();
}


void DialogColorPicker::OnChangeHSL(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromHSL();
}


void DialogColorPicker::OnChangeHSV(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromHSV();
}


void DialogColorPicker::OnChangeASS(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromASS();
}


void DialogColorPicker::OnChangeHTML(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	UpdateFromHTML();
}


void DialogColorPicker::OnChangeMode(wxCommandEvent &evt)
{
	if (!updating_controls)
		spectrum_dirty = true;
	Options.SetInt(_T("Color Picker Mode"), colorspace_choice->GetSelection());
	UpdateSpectrumDisplay();
}


void DialogColorPicker::OnSpectrumChange(wxCommandEvent &evt)
{
	updating_controls = true;

	int i = colorspace_choice->GetSelection();
	int x, y;
	spectrum->GetXY(x, y);
	switch (i) {
		case 0:
			rgb_input[2]->SetValue(x);
			rgb_input[1]->SetValue(y);
			updating_controls = false;
			UpdateFromRGB();
			break;
		case 1:
			rgb_input[2]->SetValue(x);
			rgb_input[0]->SetValue(y);
			updating_controls = false;
			UpdateFromRGB();
			break;
		case 2:
			rgb_input[1]->SetValue(x);
			rgb_input[0]->SetValue(y);
			updating_controls = false;
			UpdateFromRGB();
			break;
		case 3:
			yuv_input[2]->SetValue(x);
			yuv_input[1]->SetValue(y);
			updating_controls = false;
			UpdateFromYUV();
			break;
		case 4:
			hsl_input[1]->SetValue(x);
			hsl_input[0]->SetValue(y);
			updating_controls = false;
			UpdateFromHSL();
			break;
		case 5:
			hsv_input[1]->SetValue(x);
			hsv_input[2]->SetValue(y);
			updating_controls = false;
			UpdateFromHSV();
			break;
	}

}


void DialogColorPicker::OnSliderChange(wxCommandEvent &evt)
{
	spectrum_dirty = true;
	int i = colorspace_choice->GetSelection();
	int x, y; // only y is used, x is garbage for this control
	slider->GetXY(x, y);
	switch (i) {
		// setting the value of a component input automatically invalidates the spectrum
		// and calls the according UpdateFromXXX() function
		case 0:
			rgb_input[0]->SetValue(y);
			break;
		case 1:
			rgb_input[1]->SetValue(y);
			break;
		case 2:
			rgb_input[2]->SetValue(y);
			break;
		case 3:
			yuv_input[0]->SetValue(y);
			break;
		case 4:
			hsl_input[2]->SetValue(y);
			break;
		case 5:
			hsv_input[0]->SetValue(y);
			break;
	}
}


/*
void DialogColorPicker::OnPreviewBoxPaint(wxPaintEvent &evt)
{
	if (!preview_box) {
		wxPaintDC dc(this);
		return;
	}

	wxPaintDC dc(preview_box);
	dc.SetPen(wxPen(*wxTRANSPARENT_PEN));
	wxBrush brush(dc.GetBrush());
	brush.SetColour(cur_color);
	dc.SetBrush(brush);
	dc.DrawRectangle(0, 0, 40, 40);
	// just make sure the background isn't erased
	evt.Skip(false);
}
*/


void DialogColorPicker::OnRecentSelect(wxCommandEvent &evt)
{
	AssColor color;
	color.ParseASS(evt.GetString());
	SetColor(color.GetWXColor());
}

