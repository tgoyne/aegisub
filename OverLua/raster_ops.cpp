/*
 * Raster image operations for OverLua
 *

    Copyright 2007  Niels Martin Hansen

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    Contact:
    E-mail: <jiifurusu@gmail.com>
    IRC: jfs in #aegisub on irc.rizon.net

 */

#include "cairo_wrap.h"
#include <math.h>
//#include <windows.h>
#include <omp.h>
#include <stdint.h>

#include "raster_ops.h"
#include "../lua51/src/lauxlib.h"

/*#define cimg_display_type 0
#include "CImg.h"
using namespace cimg_library;


// Type of images processed
typedef CImg<unsigned char> Img;

// Make an Img representing the image of a Cairo image surface
// from the Lua wrapper of Cairo.
static inline Img ImgFromSurf(lua_State *L, int idx)
{
	LuaCairoSurface *surfobj = LuaCairoSurface::GetObjPointer(L, idx);
	cairo_surface_t *surf = surfobj->GetSurface();
	if (cairo_surface_get_type(surf) != CAIRO_SURFACE_TYPE_IMAGE) {
		lua_pushliteral(L, "Object for raster operation is not an image surface. Video frames are not accepted.");
		lua_error(L);
	}
	cairo_surface_flush(surf);

	int width = cairo_image_surface_get_width(surf);
	int height = cairo_image_surface_get_height(surf);
	int stride = cairo_image_surface_get_stride(surf);
	int dim = 0;
	switch (cairo_image_surface_get_format(surf)) {
		case CAIRO_FORMAT_ARGB32:
		case CAIRO_FORMAT_RGB24:
			dim = 4;
			break;
		case CAIRO_FORMAT_A8:
			dim = 1;
			break;
		case CAIRO_FORMAT_A1:
			lua_pushliteral(L, "1 bpp image surfaces are not supported for raster operations");
			lua_error(L);
			break;
		default:
			lua_pushliteral(L, "Unknown pixel format for image surface");
			lua_error(L);
	}

	unsigned char *data = cairo_image_surface_get_data(surf);

	Img res;

	// Copy over data
	if (dim == 4) {
		res = Img(width, height, 1, 4);
		unsigned char *ptrA, *ptrR, *ptrG, *ptrB;
		ptrA = res.ptr(0, 0, 0, 0);
		ptrR = res.ptr(0, 0, 0, 1);
		ptrG = res.ptr(0, 0, 0, 2);
		ptrB = res.ptr(0, 0, 0, 3);
		// Can't use cimg_mapXY since we need to take stride into account
		for (int row = 0; row < height; row++) {
			unsigned char *ptrI = data + row*stride;
			for (int x = width-1; x > 0; x--) {
				*(ptrA)++ = *(ptrI++);
				*(ptrR)++ = *(ptrI++);
				*(ptrG)++ = *(ptrI++);
				*(ptrB)++ = *(ptrI++);
			}
		}
	}
	else if (dim == 1) {
		// Also need to take stride into account here
		res = Img(width, height, 1, 1);
		unsigned char *ptrO = res.ptr(0, 0, 0, 0);
		for (int row = 0; row < height; row++) {
			unsigned char *ptrI = data + row*stride;
			for (int x = width-1; x > 0; x--) {
				*(ptrO++) = *(ptrI++);
			}
		}
	}

	return res;
}

static inline void ImgToSurf(lua_State *L, int idx, const Img &img)
{
	// Assume it has already been checked that a suitable surface is in the stack index
	LuaCairoSurface *surfobj = LuaCairoSurface::GetObjPointer(L, idx);
	cairo_surface_t *surf = surfobj->GetSurface();

	int width = cairo_image_surface_get_width(surf);
	int height = cairo_image_surface_get_height(surf);
	int stride = cairo_image_surface_get_stride(surf);
	int dim = 0;
	switch (cairo_image_surface_get_format(surf)) {
		case CAIRO_FORMAT_ARGB32:
		case CAIRO_FORMAT_RGB24:
			dim = 4;
			break;
		case CAIRO_FORMAT_A8:
			dim = 1;
			break;
		case CAIRO_FORMAT_A1:
			lua_pushliteral(L, "1 bpp image surfaces are not supported for raster operations");
			lua_error(L);
			break;
		default:
			lua_pushliteral(L, "Unknown pixel format for image surface");
			lua_error(L);
	}

	if (width != img.dimx() || height != img.dimy() || dim != img.dimv()) {
		lua_pushliteral(L, "Internal error, attempting to write back CImg to image surface with mismatching dimensions");
		lua_error(L);
	}

	unsigned char *data = cairo_image_surface_get_data(surf);

	// Copy over data
	if (dim == 4) {
		const unsigned char *ptrA, *ptrR, *ptrG, *ptrB;
		ptrA = img.ptr(0, 0, 0, 0);
		ptrR = img.ptr(0, 0, 0, 1);
		ptrG = img.ptr(0, 0, 0, 2);
		ptrB = img.ptr(0, 0, 0, 3);
		// Can't use cimg_mapXY since we need to take stride into account
		for (int row = 0; row < height; row++) {
			unsigned char *ptrO = data + row*stride;
			for (int x = width-1; x > 0; x--) {
				*(ptrO)++ = *(ptrA++);
				*(ptrO)++ = *(ptrR++);
				*(ptrO)++ = *(ptrG++);
				*(ptrO)++ = *(ptrB++);
			}
		}
	}
	else if (dim == 1) {
		// Also need to take stride into account here
		const unsigned char *ptrI = img.ptr(0, 0, 0, 0);
		for (int row = 0; row < height; row++) {
			unsigned char *ptrO = data + row*stride;
			for (int x = width-1; x > 0; x--) {
				*(ptrO++) = *(ptrI++);
			}
		}
	}

	cairo_surface_mark_dirty(surf);
}*/

static inline cairo_surface_t *CheckSurface(lua_State *L, int idx)
{
	LuaCairoSurface *surfobj = LuaCairoSurface::GetObjPointer(L, idx);
	cairo_surface_t *surf = surfobj->GetSurface();
	if (cairo_surface_get_type(surf) != CAIRO_SURFACE_TYPE_IMAGE) {
		lua_pushliteral(L, "Object for raster operation is not an image surface. Video frames are not accepted.");
		lua_error(L);
	}
	return surf;
}

static inline double NormalDist(double sigma, double x)
{
	if (sigma <= 0 && x == 0) return 1;
	else if (sigma <= 0) return 0;
	else return exp(-(x*x)/(2*sigma*sigma)) / (sigma * sqrt(2*3.1415926535));
}


// Filter an image in horizontal direction with a one-dimensional filter
// PixelWidth is the distance in bytes between pixels
template<ptrdiff_t PixelWidth>
void SeparableFilterX(unsigned char *src, unsigned char *dst, int width, int height, ptrdiff_t stride, int *kernel, int kernel_size, int divisor)
{
#pragma omp parallel for
	for (int y = 0; y < height; y++) {
		unsigned char *in = src + y*stride;
		unsigned char *out = dst + y*stride;
		for (int x = 0; x < width; x++) {
			int accum = 0;
			for (int k = 0; k < kernel_size; k++) {
				int xofs = k - kernel_size/2;
				//if (x+xofs < 0 || x+xofs >= width) continue;
				if (x+xofs < 0) xofs += width;
				if (x+xofs >= width) xofs -= width;
				accum += (int)(in[xofs*PixelWidth] * kernel[k]);
			}
			accum /= divisor;
			if (accum > 255) accum = 255;
			if (accum < 0) accum = 0;
			*out = (unsigned char)accum;
			in+=PixelWidth;
			out+=PixelWidth;
		}
	}
}


// Filter an image in vertical direction with a one-dimensional filter
// This one templated with PixelWidth since the channel interlacing is horizontal only,
// filtering once vertically will automatically catch all channels.
// (Width must be multiplied by pixel width for that to happen though.)
void SeparableFilterY(unsigned char *src, unsigned char *dst, int width, int height, ptrdiff_t stride, int *kernel, int kernel_size, int divisor)
{
#pragma omp parallel for
	for (int  x = 0; x < width; x++) {
		unsigned char *in = src + x;
		unsigned char *out = dst + x;
		for (int y = 0; y < height; y++) {
			int accum = 0;
			for (int k = 0; k < kernel_size; k++) {
				int yofs = k - kernel_size/2;
				//if (y+yofs < 0 || y+yofs >= height) continue;
				if (y+yofs < 0) yofs += height;
				if (y+yofs >= height) yofs -= height;
				accum += (int)(in[yofs*stride] * kernel[k]);
			}
			accum /= divisor;
			if (accum > 255) accum = 255;
			if (accum < 0) accum = 0;
			*out = (unsigned char)accum;
			in += stride;
			out += stride;
		}
	}
}


void ApplySeparableFilter(lua_State *L, cairo_surface_t *surf, int *kernel, int kernel_size, int divisor)
{
	// Get surface properties
	cairo_surface_flush(surf);
	int width = cairo_image_surface_get_width(surf);
	int height = cairo_image_surface_get_height(surf);
	ptrdiff_t stride = (ptrdiff_t)cairo_image_surface_get_stride(surf);
	cairo_format_t format = cairo_image_surface_get_format(surf);
	unsigned char *data = cairo_image_surface_get_data(surf);

	if (format != CAIRO_FORMAT_ARGB32 && format != CAIRO_FORMAT_RGB24 && format != CAIRO_FORMAT_A8) {
		lua_pushliteral(L, "Unsupported image format for raster operation");
		lua_error(L);
	}

	// Work image
	unsigned char *wimg = new unsigned char[height*stride];

	// Do the filtering
	if (format == CAIRO_FORMAT_ARGB32 || format == CAIRO_FORMAT_RGB24) {
		// Horizontal
		SeparableFilterX<4>(data+0, wimg+0, width, height, stride, kernel, kernel_size, divisor);
		SeparableFilterX<4>(data+1, wimg+1, width, height, stride, kernel, kernel_size, divisor);
		SeparableFilterX<4>(data+2, wimg+2, width, height, stride, kernel, kernel_size, divisor);
		SeparableFilterX<4>(data+3, wimg+3, width, height, stride, kernel, kernel_size, divisor);
		// Vertical
		//memcpy(data, wimg, height*stride);
		SeparableFilterY(wimg, data, width*4, height, stride, kernel, kernel_size, divisor);
	} else if (format == CAIRO_FORMAT_A8) {
		// Horizontal
		SeparableFilterX<1>(data, wimg, width, height, stride, kernel, kernel_size, divisor);
		// Vertical
		SeparableFilterY(wimg, data, width, height, stride, kernel, kernel_size, divisor);
	}

	// Clean up
	cairo_surface_mark_dirty(surf);
	delete[] wimg;
}


// raster.gaussian_blur(imgsurf, sigma)
static int gaussian_blur(lua_State *L)
{
	// Get arguments
	cairo_surface_t *surf = CheckSurface(L, 1);
	double sigma = luaL_checknumber(L, 2);

	// Generate gaussian kernel
	int kernel_size = (int)(sigma*3 + 0.5) | 1; // binary-or with 1 to make sure the number is odd
	if (kernel_size < 3) kernel_size = 3;
	int *kernel = new int[kernel_size];
	kernel[kernel_size/2] = (int)(NormalDist(sigma, 0) * 255);
	int ksum = kernel[kernel_size/2];
	for (int x = kernel_size/2-1; x >= 0; x--) {
		int val = (int)(NormalDist(sigma, kernel_size/2-x) * 255 + 0.5);
		ksum += val*2;
		kernel[x] = val;
		kernel[kernel_size - x - 1] = val;
	}

	ApplySeparableFilter(L, surf, kernel, kernel_size, ksum);

	delete[] kernel;

	return 0;
}


// n tap box blur
static int box_blur(lua_State *L)
{
	cairo_surface_t *surf = CheckSurface(L, 1);
	int width = luaL_checkint(L, 2);
	if (width <= 0) { luaL_error(L, "Width of box kernel for raster.box must be larger than zero, specified to %d.", width); return 0; }
	int applications = luaL_optint(L, 3, 1);
	int *kernel = new int[width];
	for (int i = 0; i < width; i++)
		kernel[i] = 1;
	while (applications-- > 0)
		ApplySeparableFilter(L, surf, kernel, width, width);
	delete[] kernel;
	return 0;
}


static int invert_image(lua_State *L)
{
	cairo_surface_t *surf = CheckSurface(L, 1);
	cairo_surface_flush(surf);
	int width = cairo_image_surface_get_width(surf);
	int height = cairo_image_surface_get_height(surf);
	ptrdiff_t stride = (ptrdiff_t)cairo_image_surface_get_stride(surf);
	unsigned char *data = cairo_image_surface_get_data(surf);
	cairo_format_t format = cairo_image_surface_get_format(surf);

	// ARGB32 and RGB24 must be treated differently due to the premultipled alpha in ARGB32
	// Also the alpha in ARGB32 is not inverted, only the colour channels
	if (format == CAIRO_FORMAT_ARGB32) {
#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			uint32_t *line = (uint32_t*)(data + y*stride);
			for (int x = 0; x < width; x++, line++) {
				// The R, G and B channels are in range 0..a, and inverting means calculating
				// max-current, so inversion here is a-c for each channel.
				unsigned char a = (*line & 0xff000000) >> 24;
				unsigned char r = (*line & 0x00ff0000) >> 16;
				unsigned char g = (*line & 0x0000ff00) >> 8;
				unsigned char b = *line & 0x000000ff;
				*line = (a<<24) | ((a-r)<<16) | ((a-g)<<8) | (a-b);
			}
		}
	}
	else if (format == CAIRO_FORMAT_RGB24) {
#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			uint32_t *line = (uint32_t*)(data + y*stride);
			for (int x = 0; x < width; x++, line++) {
				// Invert R, G and B channels by XOR'ing them with 0xff each.
				*line = *line ^ 0x00ffffff;
			}
		}
	}
	else if (format == CAIRO_FORMAT_A8) {
#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			unsigned char *line = data + y*stride;
			for (int x = 0; x < width; x++, line++) {
				*line = ~ *line;
			}
		}
	}
	else if (format == CAIRO_FORMAT_A1) {
		int lwidth = (width + 31) / 32; // "long-width", width in 32 bit longints, rounded up
#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			// Pixels are stored packed into 32 bit quantities
			uint32_t *line = (uint32_t*)(data + y*stride);
			for (int x = 0; x < lwidth; x++, line++) {
				// Yes this means we might end up inverting too many bits.. hopefully won't happen.
				// (And even then, who would seriously use A1 surfaces?)
				*line = ~ *line;
			}
		}
	}

	cairo_surface_mark_dirty(surf);

	return 0;
}


// Registration

static luaL_Reg rasterlib[] = {
	{"gaussian_blur", gaussian_blur}, {"box_blur", box_blur},
	{"invert", invert_image},
	{NULL, NULL}
};

int luaopen_raster(lua_State *L)
{
	luaL_register(L, "raster", rasterlib);
	return 0;
}
