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


///////////
// Headers
#include "gl_text.h"
#include "utils.h"


///////////////////
// Static instance
OpenGLText* OpenGLText::instance;


///////////////
// Constructor
OpenGLText::OpenGLText() {
	r = g = b = a = 1.0f;
}


//////////////
// Destructor
OpenGLText::~OpenGLText() {
	Reset();
}


/////////
// Reset
void OpenGLText::Reset() {
	for (unsigned int i=0;i<textures.size();i++) delete textures[i];
	textures.clear();
	glyphs.clear();
}


////////////////
// Get instance
OpenGLText* OpenGLText::GetInstance() {
	if (!instance) instance = new OpenGLText();
	wxASSERT(instance);
	return instance;
}


////////////
// Set font
void OpenGLText::DoSetFont(wxString face,int size) {
	// No change required
	if (size == fontSize && face == fontFace) return;

	// Set font
	fontFace = face;
	fontSize = size;
	font.SetFaceName(fontFace);
	font.SetPointSize(size);

	// Delete all old data
	Reset();
}


//////////////
// Set colour
void OpenGLText::DoSetColour(wxColour col,float alpha) {
	r = col.Red() / 255.0f;
	g = col.Green() / 255.0f;
	b = col.Blue() / 255.0f;
	a = alpha;
}


/////////
// Print
void OpenGLText::DoPrint(wxString text,int x,int y) {
	// Variables
	int dx=x,dy=y;
	size_t len = text.Length();
	OpenGLTextGlyph glyph;
	lineHeight = 16;	// FIXME

	// Set OpenGL
	// TODO

	// Draw string
	for (size_t i=0;i<len;i++) {
		// Get current character
		int curChar = text[i];

		// Handle carriage returns
		if (curChar == '\n') {
			dx = x;
			dy += lineHeight;
		}

		// Handle normal glyphs
		else {
			glyph = GetGlyph(curChar);
			glyph.Draw(dx,dy);
			dx += glyph.w;
		}
	}
}


///////////////
// Get a glyph
OpenGLTextGlyph OpenGLText::GetGlyph(int i) {
	glyphMap::iterator res = glyphs.find(i);

	// Found
	if (res != glyphs.end()) return res->second;

	// Not found, create it
	return CreateGlyph(i);
}


//////////////////
// Create a glyph
OpenGLTextGlyph OpenGLText::CreateGlyph(int n) {
	// Create glyph
	OpenGLTextGlyph glyph;
	glyph.value = n;
	glyph.GetMetrics();

	// Insert into some texture
	bool ok = false;
	for (unsigned int i=0;i<textures.size();i++) {
		if (textures[i]->TryToInsert(glyph)) {
			ok = true;
			break;
		}
	}

	// No texture could fit it, create a new one
	if (!ok) {
		textures.push_back(new OpenGLTextTexture(256,256));
		textures.back()->TryToInsert(glyph);
	}

	// Set glyph and return it
	glyphs[n] = glyph;
	return glyph;
}


///////////////////////
// Texture constructor
OpenGLTextTexture::OpenGLTextTexture(int w,int h) {
	// Properties
	x = y = 0;
	width = SmallestPowerOf2(w);
	height = SmallestPowerOf2(h);
	tex = 0;

	// Generate and bind
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	// Allocate texture
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,width,height,0,GL_ALPHA,GL_UNSIGNED_BYTE,NULL);
	if (glGetError()) throw _T("Could not allocate Text Texture");
}


//////////////////////
// Texture destructor
OpenGLTextTexture::~OpenGLTextTexture() {
	if (tex) {
		glDeleteTextures(1,&tex);
		tex = 0;
	}
}


//////////////////////////
// Can fit a glyph in it?
bool OpenGLTextTexture::TryToInsert(OpenGLTextGlyph glyph) {
	// Get size
	int w = glyph.w;
	int h = glyph.h;
	if (w > width) return false;
	if (y+h > height) return false;

	// Can fit in this row?
	if (x + w < width) {
		x += w;
		if (y+h > nextY) nextY = y+h;
		Insert(glyph);
		return true;
	}

	// Can fit the next row?
	else {
		y = nextY;
		if (y+h > height) return false;
		nextY = y+h;
		x = 0;
		Insert(glyph);
		return true;
	}
}


//////////
// Insert
void OpenGLTextTexture::Insert(OpenGLTextGlyph glyph) {
	// Glyph data
	wxString str = wxChar(glyph.value);
	int w = glyph.w;
	int h = glyph.h;

	// Fill glyph structure
	glyph.x1 = float(x)/width;
	glyph.y1 = float(y)/height;
	glyph.x2 = float(x+w)/width;
	glyph.y2 = float(y+h)/height;
	glyph.tex = tex;

	// Create bitmap and bind it to a DC
	wxBitmap bmp(w,h,24);
	wxMemoryDC dc(bmp);

	// Draw text and convert to image
	dc.SetFont(OpenGLText::GetFont());
	dc.DrawText(str,0,0);
	wxImage img = bmp.ConvertToImage();

	// Upload image to video memory
	const unsigned char* src = img.GetData();
	glTexSubImage2D(GL_TEXTURE_2D,0,x,y,w,h,GL_ALPHA,GL_UNSIGNED_BYTE,src);
	if (glGetError()) throw _T("Error uploading glyph data to video memory.");
}


////////////////
// Draw a glyph
void OpenGLTextGlyph::Draw(int x,int y) {
	// Store matrix and translate
	glPushMatrix();
	glTranslatef((float)x,(float)y,0.0f);

	// Set blend
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// Draw quad
	glBegin(GL_QUADS);
		// Top-left
		glTexCoord2f(x1,y1);
		glVertex2f(0,0);
		// Bottom-left
		glTexCoord2f(x1,y2);
		glVertex2f(0,h);
		// Bottom-right
		glTexCoord2f(x2,y2);
		glVertex2f(w,h);
		// Top-right
		glTexCoord2f(x2,y1);
		glVertex2f(w,0);
	glEnd();

	// Restore GL state
	glDisable(GL_BLEND);
	glPopMatrix();
}


/////////////////////
// Get glyph metrics
wxBitmap *OpenGLTextGlyph::tempBmp = NULL;
void OpenGLTextGlyph::GetMetrics() {
	// Glyph data
	wxCoord desc,lead;
	wxString str = wxChar(value);

	// Create bitmap, if needed
	if (!tempBmp) tempBmp = new wxBitmap(16,16,24);

	// Get text extents
	{
		wxMemoryDC dc(*tempBmp);
		dc.SetFont(OpenGLText::GetFont());
		dc.GetTextExtent(str,&w,&h,&desc,&lead);
	}
}
