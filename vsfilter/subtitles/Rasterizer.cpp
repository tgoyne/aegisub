/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "Rasterizer.h"

Rasterizer::Rasterizer() : mpPathTypes(NULL), mpPathPoints(NULL), mPathPoints(0), mpOverlayBuffer(NULL)
{
	mOverlayWidth = mOverlayHeight = 0;
	mPathOffsetX = mPathOffsetY = 0;
	mOffsetX = mOffsetY = 0;
}

Rasterizer::~Rasterizer()
{
	_TrashPath();
	_TrashOverlay();
}

void Rasterizer::_TrashPath()
{
	delete [] mpPathTypes;
	delete [] mpPathPoints;
	mpPathTypes = NULL;
	mpPathPoints = NULL;
	mPathPoints = 0;
}

void Rasterizer::_TrashOverlay()
{
	delete [] mpOverlayBuffer;
	mpOverlayBuffer = NULL;
}

void Rasterizer::_ReallocEdgeBuffer(int edges)
{
	mEdgeHeapSize = edges;
	mpEdgeBuffer = (Edge*)realloc(mpEdgeBuffer, sizeof(Edge)*edges);
}

void Rasterizer::_EvaluateBezier(int ptbase, bool fBSpline)
{
	const POINT* pt0 = mpPathPoints + ptbase;
	const POINT* pt1 = mpPathPoints + ptbase + 1;
	const POINT* pt2 = mpPathPoints + ptbase + 2;
	const POINT* pt3 = mpPathPoints + ptbase + 3;

	double x0 = pt0->x;
	double x1 = pt1->x;
	double x2 = pt2->x;
	double x3 = pt3->x;
	double y0 = pt0->y;
	double y1 = pt1->y;
	double y2 = pt2->y;
	double y3 = pt3->y;

	double cx3, cx2, cx1, cx0, cy3, cy2, cy1, cy0;

	if(fBSpline)
	{
		// 1   [-1 +3 -3 +1]
		// - * [+3 -6 +3  0]
		// 6   [-3  0 +3  0]
		//	   [+1 +4 +1  0]

		double _1div6 = 1.0/6.0;

		cx3 = _1div6*(-  x0+3*x1-3*x2+x3);
		cx2 = _1div6*( 3*x0-6*x1+3*x2);
		cx1 = _1div6*(-3*x0	   +3*x2);
		cx0 = _1div6*(   x0+4*x1+1*x2);

		cy3 = _1div6*(-  y0+3*y1-3*y2+y3);
		cy2 = _1div6*( 3*y0-6*y1+3*y2);
		cy1 = _1div6*(-3*y0     +3*y2);
		cy0 = _1div6*(   y0+4*y1+1*y2);
	}
	else // bezier
	{
		// [-1 +3 -3 +1]
		// [+3 -6 +3  0]
		// [-3 +3  0  0]
		// [+1  0  0  0]

		cx3 = -  x0+3*x1-3*x2+x3;
		cx2 =  3*x0-6*x1+3*x2;
		cx1 = -3*x0+3*x1;
		cx0 =    x0;

		cy3 = -  y0+3*y1-3*y2+y3;
		cy2 =  3*y0-6*y1+3*y2;
		cy1 = -3*y0+3*y1;
		cy0 =    y0;
	}

	//
	// This equation is from Graphics Gems I.
	//
	// The idea is that since we're approximating a cubic curve with lines,
	// any error we incur is due to the curvature of the line, which we can
	// estimate by calculating the maximum acceleration of the curve.  For
	// a cubic, the acceleration (second derivative) is a line, meaning that
	// the absolute maximum acceleration must occur at either the beginning
	// (|c2|) or the end (|c2+c3|).  Our bounds here are a little more
	// conservative than that, but that's okay.
	//
	// If the acceleration of the parametric formula is zero (c2 = c3 = 0),
	// that component of the curve is linear and does not incur any error.
	// If a=0 for both X and Y, the curve is a line segment and we can
	// use a step size of 1.

	double maxaccel1 = fabs(2*cy2) + fabs(6*cy3);
	double maxaccel2 = fabs(2*cx2) + fabs(6*cx3);

	double maxaccel = maxaccel1 > maxaccel2 ? maxaccel1 : maxaccel2;
	double h = 1.0;

	if(maxaccel > 8.0) h = sqrt(8.0 / maxaccel);

	if(!fFirstSet) {firstp.x = (LONG)cx0; firstp.y = (LONG)cy0; lastp = firstp; fFirstSet = true;}

	for(double t = 0; t < 1.0; t += h)
	{
		double x = cx0 + t*(cx1 + t*(cx2 + t*cx3));
		double y = cy0 + t*(cy1 + t*(cy2 + t*cy3));
		_EvaluateLine(lastp.x, lastp.y, (int)x, (int)y);
	}

	double x = cx0 + cx1 + cx2 + cx3;
	double y = cy0 + cy1 + cy2 + cy3;
	_EvaluateLine(lastp.x, lastp.y, (int)x, (int)y);
}

void Rasterizer::_EvaluateLine(int pt1idx, int pt2idx)
{
	const POINT* pt1 = mpPathPoints + pt1idx;
	const POINT* pt2 = mpPathPoints + pt2idx;

	_EvaluateLine(pt1->x, pt1->y, pt2->x, pt2->y);
}

void Rasterizer::_EvaluateLine(int x0, int y0, int x1, int y1)
{
	if(lastp.x != x0 || lastp.y != y0)
	{
		_EvaluateLine(lastp.x, lastp.y, x0, y0);
	}

	if(!fFirstSet) {firstp.x = x0; firstp.y = y0; fFirstSet = true;}
	lastp.x = x1; lastp.y = y1;

	if(y1 > y0)	// down
	{
		__int64 xacc = (__int64)x0 << 13;

		// prestep y0 down

		int dy = y1 - y0;
		int y = ((y0 + 3)&~7) + 4;
		int iy = y >> 3;

		y1 = (y1 - 5) >> 3;

		if(iy <= y1)
		{
			__int64 invslope = (__int64(x1 - x0) << 16) / dy;

			while(mEdgeNext + y1 + 1 - iy > mEdgeHeapSize)
				_ReallocEdgeBuffer(mEdgeHeapSize*2);

			xacc += (invslope * (y - y0)) >> 3;

			while(iy <= y1)
			{
				int ix = (int)((xacc + 32768) >> 16);

				mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
				mpEdgeBuffer[mEdgeNext].posandflag = ix*2 + 1;

				mpScanBuffer[iy] = mEdgeNext++;

				++iy;
				xacc += invslope;
			}
		}
	}
	else if(y1 < y0) // up
	{
		__int64 xacc = (__int64)x1 << 13;

		// prestep y1 down

		int dy = y0 - y1;
		int y = ((y1 + 3)&~7) + 4;
		int iy = y >> 3;

		y0 = (y0 - 5) >> 3;

		if(iy <= y0)
		{
			__int64 invslope = (__int64(x0 - x1) << 16) / dy;

			while(mEdgeNext + y0 + 1 - iy > mEdgeHeapSize)
				_ReallocEdgeBuffer(mEdgeHeapSize*2);

			xacc += (invslope * (y - y1)) >> 3;

			while(iy <= y0)
			{
				int ix = (int)((xacc + 32768) >> 16);

				mpEdgeBuffer[mEdgeNext].next = mpScanBuffer[iy];
				mpEdgeBuffer[mEdgeNext].posandflag = ix*2;

				mpScanBuffer[iy] = mEdgeNext++;

				++iy;
				xacc += invslope;
			}
		}
	}
}

bool Rasterizer::BeginPath(HDC hdc)
{
	_TrashPath();

	return !!::BeginPath(hdc);
}

bool Rasterizer::EndPath(HDC hdc)
{
	::CloseFigure(hdc);

	if(::EndPath(hdc))
	{
		mPathPoints = GetPath(hdc, NULL, NULL, 0);

		if(!mPathPoints)
			return true;

		mpPathTypes = (BYTE*)malloc(sizeof(BYTE) * mPathPoints);
		mpPathPoints = (POINT*)malloc(sizeof(POINT) * mPathPoints);

		if(mPathPoints == GetPath(hdc, mpPathPoints, mpPathTypes, mPathPoints))
			return true;
	}

	::AbortPath(hdc);

	return false;
}

bool Rasterizer::PartialBeginPath(HDC hdc, bool bClearPath)
{
	if(bClearPath)
		_TrashPath();

	return !!::BeginPath(hdc);
}

bool Rasterizer::PartialEndPath(HDC hdc, long dx, long dy)
{
	::CloseFigure(hdc);

	if(::EndPath(hdc))
	{
		int nPoints;
		BYTE* pNewTypes;
		POINT* pNewPoints;

		nPoints = GetPath(hdc, NULL, NULL, 0);

		if(!nPoints)
			return true;

		pNewTypes = (BYTE*)realloc(mpPathTypes, (mPathPoints + nPoints) * sizeof(BYTE));
		pNewPoints = (POINT*)realloc(mpPathPoints, (mPathPoints + nPoints) * sizeof(POINT));

		if(pNewTypes)
			mpPathTypes = pNewTypes;

		if(pNewPoints)
			mpPathPoints = pNewPoints;

		BYTE* pTypes = new BYTE[nPoints];
		POINT* pPoints = new POINT[nPoints];

		if(pNewTypes && pNewPoints && nPoints == GetPath(hdc, pPoints, pTypes, nPoints))
		{
			for(int i = 0; i < nPoints; ++i)
			{
				mpPathPoints[mPathPoints + i].x = pPoints[i].x + dx;
				mpPathPoints[mPathPoints + i].y = pPoints[i].y + dy;
				mpPathTypes[mPathPoints + i] = pTypes[i];
			}

			mPathPoints += nPoints;

			delete[] pTypes;
			delete[] pPoints;
			return true;
		}
		else
			DebugBreak();

		delete[] pTypes;
		delete[] pPoints;
	}

	::AbortPath(hdc);

	return false;
}

bool Rasterizer::ScanConvert()
{
	int lastmoveto = -1;
	int i;

	// Drop any outlines we may have.

	mOutline.clear();
	mWideOutline.clear();

	// Determine bounding box

	if(!mPathPoints)
	{
		mPathOffsetX = mPathOffsetY = 0;
		mWidth = mHeight = 0;
		return 0;
	}

	int minx = INT_MAX;
	int miny = INT_MAX;
	int maxx = INT_MIN;
	int maxy = INT_MIN;

	for(i=0; i<mPathPoints; ++i)
	{
		int ix = mpPathPoints[i].x;
		int iy = mpPathPoints[i].y;

		if(ix < minx) minx = ix;
		if(ix > maxx) maxx = ix;
		if(iy < miny) miny = iy;
		if(iy > maxy) maxy = iy;
	}

	minx = (minx >> 3) & ~7;
	miny = (miny >> 3) & ~7;
	maxx = (maxx + 7) >> 3;
	maxy = (maxy + 7) >> 3;

	for(i=0; i<mPathPoints; ++i)
	{
		mpPathPoints[i].x -= minx*8;
		mpPathPoints[i].y -= miny*8;
	}

	if(minx > maxx || miny > maxy)
	{
		mWidth = mHeight = 0;
		mPathOffsetX = mPathOffsetY = 0;
		_TrashPath();
		return true;
	}

	mWidth = maxx + 1 - minx;
	mHeight = maxy + 1 - miny;

	mPathOffsetX = minx;
	mPathOffsetY = miny;

	// Initialize edge buffer.  We use edge 0 as a sentinel.

	mEdgeNext = 1;
	mEdgeHeapSize = 2048;
	mpEdgeBuffer = (Edge*)malloc(sizeof(Edge)*mEdgeHeapSize);

	// Initialize scanline list.

	mpScanBuffer = new unsigned int[mHeight];
	memset(mpScanBuffer, 0, mHeight*sizeof(unsigned int));

	// Scan convert the outline.  Yuck, Bezier curves....

	// Unfortunately, Windows 95/98 GDI has a bad habit of giving us text
	// paths with all but the first figure left open, so we can't rely
	// on the PT_CLOSEFIGURE flag being used appropriately.

	fFirstSet = false;
	firstp.x = firstp.y = 0;
	lastp.x = lastp.y = 0;

	for(i=0; i<mPathPoints; ++i)
	{
		BYTE t = mpPathTypes[i] & ~PT_CLOSEFIGURE;

		switch(t)
		{
		case PT_MOVETO:
			if(lastmoveto >= 0 && firstp != lastp)
				_EvaluateLine(lastp.x, lastp.y, firstp.x, firstp.y);
			lastmoveto = i;
			fFirstSet = false;
			lastp = mpPathPoints[i];
			break;
		case PT_MOVETONC:
			break;
		case PT_LINETO:
			if(mPathPoints - (i-1) >= 2) _EvaluateLine(i-1, i);
			break;
		case PT_BEZIERTO:
			if(mPathPoints - (i-1) >= 4) _EvaluateBezier(i-1, false);
			i += 2;
			break;
		case PT_BSPLINETO:
			if(mPathPoints - (i-1) >= 4) _EvaluateBezier(i-1, true);
			i += 2;
			break;
		case PT_BSPLINEPATCHTO:
			if(mPathPoints - (i-3) >= 4) _EvaluateBezier(i-3, true);
			break;
		}
	}

	if(lastmoveto >= 0 && firstp != lastp)
		_EvaluateLine(lastp.x, lastp.y, firstp.x, firstp.y);

	// Free the path since we don't need it anymore.

	_TrashPath();

	// Convert the edges to spans.  We couldn't do this before because some of
	// the regions may have winding numbers >+1 and it would have been a pain
	// to try to adjust the spans on the fly.  We use one heap to detangle
	// a scanline's worth of edges from the singly-linked lists, and another
	// to collect the actual scans.

	std::vector<int> heap;

	mOutline.reserve(mEdgeNext / 2);

	__int64 y = 0;

	for(y=0; y<mHeight; ++y)
	{
		int count = 0;

		// Detangle scanline into edge heap.

		for(unsigned ptr = (unsigned)(mpScanBuffer[y]&0xffffffff); ptr; ptr = mpEdgeBuffer[ptr].next)
		{
			heap.push_back(mpEdgeBuffer[ptr].posandflag);
		}

		// Sort edge heap.  Note that we conveniently made the opening edges
		// one more than closing edges at the same spot, so we won't have any
		// problems with abutting spans.

		std::sort(heap.begin(), heap.end()/*begin() + heap.size()*/);

		// Process edges and add spans.  Since we only check for a non-zero
		// winding number, it doesn't matter which way the outlines go!

		std::vector<int>::iterator itX1 = heap.begin();
		std::vector<int>::iterator itX2 = heap.end(); // begin() + heap.size();

		int x1, x2;

		for(; itX1 != itX2; ++itX1)
		{
			int x = *itX1;

			if(!count) 
				x1 = (x>>1);

			if(x&1) 
				++count;
			else 
				--count;

			if(!count)
			{
				x2 = (x>>1);

				if(x2>x1)
					mOutline.push_back(std::pair<__int64,__int64>((y<<32)+x1+0x4000000040000000i64, (y<<32)+x2+0x4000000040000000i64)); // G: damn Avery, this is evil! :)
			}
		}

		heap.clear();
	}

	// Dump the edge and scan buffers, since we no longer need them.

	free(mpEdgeBuffer);
	delete [] mpScanBuffer;

	// All done!

	return true;
}

using namespace std;

void Rasterizer::_OverlapRegion(tSpanBuffer& dst, tSpanBuffer& src, int dx, int dy)
{
	tSpanBuffer temp;

	temp.reserve(dst.size() + src.size());

	dst.swap(temp);

	tSpanBuffer::iterator itA = temp.begin();
	tSpanBuffer::iterator itAE = temp.end();
	tSpanBuffer::iterator itB = src.begin();
	tSpanBuffer::iterator itBE = src.end();

	// Don't worry -- even if dy<0 this will still work! // G: hehe, the evil twin :)

	unsigned __int64 offset1 = (((__int64)dy)<<32) - dx;
	unsigned __int64 offset2 = (((__int64)dy)<<32) + dx;

	while(itA != itAE && itB != itBE)
	{
		if((*itB).first + offset1 < (*itA).first)
		{
			// B span is earlier.  Use it.

			unsigned __int64 x1 = (*itB).first + offset1;
			unsigned __int64 x2 = (*itB).second + offset2;

			++itB;

			// B spans don't overlap, so begin merge loop with A first.

			for(;;)
			{
				// If we run out of A spans or the A span doesn't overlap,
				// then the next B span can't either (because B spans don't
				// overlap) and we exit.

				if(itA == itAE || (*itA).first > x2)
					break;

				do {x2 = _MAX(x2, (*itA++).second);}
				while(itA != itAE && (*itA).first <= x2);

				// If we run out of B spans or the B span doesn't overlap,
				// then the next A span can't either (because A spans don't
				// overlap) and we exit.

				if(itB == itBE || (*itB).first + offset1 > x2)
					break;

				do {x2 = _MAX(x2, (*itB++).second + offset2);}
				while(itB != itBE && (*itB).first + offset1 <= x2);
			}

			// Flush span.

			dst.push_back(tSpan(x1, x2));	
		}
		else
		{
			// A span is earlier.  Use it.

			unsigned __int64 x1 = (*itA).first;
			unsigned __int64 x2 = (*itA).second;

			++itA;

			// A spans don't overlap, so begin merge loop with B first.

			for(;;)
			{
				// If we run out of B spans or the B span doesn't overlap,
				// then the next A span can't either (because A spans don't
				// overlap) and we exit.

				if(itB == itBE || (*itB).first + offset1 > x2)
					break;

				do {x2 = _MAX(x2, (*itB++).second + offset2);}
				while(itB != itBE && (*itB).first + offset1 <= x2);

				// If we run out of A spans or the A span doesn't overlap,
				// then the next B span can't either (because B spans don't
				// overlap) and we exit.

				if(itA == itAE || (*itA).first > x2)
					break;

				do {x2 = _MAX(x2, (*itA++).second);}
				while(itA != itAE && (*itA).first <= x2);
			}

			// Flush span.

			dst.push_back(tSpan(x1, x2));	
		}
	}

	// Copy over leftover spans.

	while(itA != itAE)
		dst.push_back(*itA++);

	while(itB != itBE)
	{
		dst.push_back(tSpan((*itB).first + offset1, (*itB).second + offset2));	
		++itB;
	}
}

bool Rasterizer::CreateWidenedRegion(int r)
{
	if(r < 0) r = 0;

	for(int y = -r; y <= r; ++y)
	{
		int x = (int)(0.5 + sqrt(float(r*r - y*y)));

		_OverlapRegion(mWideOutline, mOutline, x, y);
	}

	mWideBorder = r;

	return true;
}

void Rasterizer::DeleteOutlines()
{
	mWideOutline.clear();
	mOutline.clear();
}

bool Rasterizer::Rasterize(int xsub, int ysub, bool fBlur)
{
	_TrashOverlay();

	if(!mWidth || !mHeight)
	{
		mOverlayWidth = mOverlayHeight = 0;
		return true;
	}

	xsub &= 7;
	ysub &= 7;

	int width = mWidth + xsub;
	int height = mHeight + ysub;

	mOffsetX = mPathOffsetX - xsub;
	mOffsetY = mPathOffsetY - ysub;

	mWideBorder = (mWideBorder+7)&~7;

	if(!mWideOutline.empty())
	{
		width += 2*mWideBorder;
		height += 2*mWideBorder;

		xsub += mWideBorder;
		ysub += mWideBorder;

		mOffsetX -= mWideBorder;
		mOffsetY -= mWideBorder;
	}

	mOverlayWidth = ((width+7)>>3) + 1;
	mOverlayHeight = ((height+7)>>3) + 1;

	mpOverlayBuffer = new byte[2 * mOverlayWidth * mOverlayHeight];
	memset(mpOverlayBuffer, 0, 2 * mOverlayWidth * mOverlayHeight);

	// Are we doing a border?

	tSpanBuffer* pOutline[2] = {&mOutline, &mWideOutline};

	for(int i = countof(pOutline)-1; i >= 0; i--)
	{
		tSpanBuffer::iterator it = pOutline[i]->begin();
		tSpanBuffer::iterator itEnd = pOutline[i]->end();

		for(; it!=itEnd; ++it)
		{
			int y = (int)(((*it).first >> 32) - 0x40000000 + ysub);
			int x1 = (int)(((*it).first & 0xffffffff) - 0x40000000 + xsub);
			int x2 = (int)(((*it).second & 0xffffffff) - 0x40000000 + xsub);

			if(x2 > x1)
			{
				int first = x1>>3;
				int last = (x2-1)>>3;
				byte* dst = mpOverlayBuffer + 2*(mOverlayWidth*(y>>3) + first) + i;

				if(first == last)
					*dst += x2-x1;
				else
				{
					*dst += ((first+1)<<3) - x1;
					dst += 2;

					while(++first < last)
					{
						*dst += 0x08;
						dst += 2;
					}

					*dst += x2 - (last<<3);
				}
			}
		}
	}

	if(fBlur && mOverlayWidth >= 3 && mOverlayHeight >= 3)
	{
		int pitch = mOverlayWidth*2;

		byte* tmp = new byte[pitch*mOverlayHeight];
		if(!tmp) return(false);

		memcpy(tmp, mpOverlayBuffer, pitch*mOverlayHeight);

		int border = !mWideOutline.empty() ? 1 : 0;

		for(int j = 1; j < mOverlayHeight-1; j++)
		{
			byte* src = tmp + pitch*j + 2 + border;
			byte* dst = mpOverlayBuffer + pitch*j + 2 + border;

			for(int i = 1; i < mOverlayWidth-1; i++, src+=2, dst+=2)
			{
				*dst = (src[-2-pitch] + (src[-pitch]<<1) + src[+2-pitch]
					+ (src[-2]<<1) + (src[0]<<2) + (src[+2]<<1)
					+ src[-2+pitch] + (src[+pitch]<<1) + src[+2+pitch]) >> 4;
			}
		}

		delete [] tmp;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////

/*#define pixmix(s)																 \
	int a = (((s)*(color>>24))>>6)&0xff;										 \
	int ia = 256-a;																 \
																				 \
	dst[wt] = ((((dst[wt]&0x00ff00ff)*ia + (color&0x00ff00ff)*a)&0xff00ff00)>>8) \
			| ((((dst[wt]&0x0000ff00)*ia + (color&0x0000ff00)*a)&0x00ff0000)>>8) \
			| ((((dst[wt]>>8)&0x00ff0000)*ia)&0xff000000);

#define pixmix2(s)																 \
	int a = ((((s)*(am[wt]))*(color>>24))>>12)&0xff;							 \
	int ia = 256-a;																 \
																				 \
	dst[wt] = ((((dst[wt]&0x00ff00ff)*ia + (color&0x00ff00ff)*a)&0xff00ff00)>>8) \
			| ((((dst[wt]&0x0000ff00)*ia + (color&0x0000ff00)*a)&0x00ff0000)>>8) \
			| ((((dst[wt]>>8)&0x00ff0000)*ia)&0xff000000);*/

static __forceinline void pixmix(DWORD *dst, DWORD color, DWORD alpha)
{
	int a = (((alpha)*(color>>24))>>12)&0xff;
	int ia = 256-a;

	*dst = ((((*dst&0x00ff00ff)*ia + (color&0x00ff00ff)*a)&0xff00ff00)>>8)
			| ((((*dst&0x0000ff00)*ia + (color&0x0000ff00)*a)&0x00ff0000)>>8)
			| ((((*dst>>8)&0x00ff0000)*ia)&0xff000000);
}

#include <xmmintrin.h>
#include <emmintrin.h>

static __forceinline void pixmix_sse2(DWORD* dst, DWORD color, DWORD alpha)
{
	alpha = ((alpha * (color>>24)) >> 12) & 0xff;
	color &= 0xffffff;

	__m128i zero = _mm_setzero_si128();
	__m128i a = _mm_set1_epi32((alpha << 16) | (0x100 - alpha));
	__m128i d = _mm_unpacklo_epi8(_mm_cvtsi32_si128(*dst), zero);
	__m128i s = _mm_unpacklo_epi8(_mm_cvtsi32_si128(color), zero);
	__m128i r = _mm_unpacklo_epi16(d, s);

	r = _mm_madd_epi16(r, a);
	r = _mm_srli_epi32(r, 8);
	r = _mm_packs_epi32(r, r);
	r = _mm_packus_epi16(r, r);

	*dst = (DWORD)_mm_cvtsi128_si32(r);
}

#include "../dsutil/vd.h"

static const __int64 _00ff00ff00ff00ff = 0x00ff00ff00ff00ffi64;

CRect Rasterizer::Draw(SubPicDesc& spd, CRect& clipRect, byte* pAlphaMask, int xsub, int ysub, const long* switchpts, bool fBody, bool fBorder)
{
	CRect bbox(0, 0, 0, 0);

	if(!switchpts || !fBody && !fBorder) return(bbox);

	// clip

	CRect r(0, 0, spd.w, spd.h);
	r &= clipRect;

	int x = (xsub + mOffsetX + 4)>>3;
	int y = (ysub + mOffsetY + 4)>>3;
	int w = mOverlayWidth;
	int h = mOverlayHeight;
	int xo = 0, yo = 0;

	if(x < r.left) {xo = r.left-x; w -= r.left-x; x = r.left;}
	if(y < r.top) {yo = r.top-y; h -= r.top-y; y = r.top;}
	if(x+w > r.right) w = r.right-x;
	if(y+h > r.bottom) h = r.bottom-y;

	if(w <= 0 || h <= 0) return(bbox);

	bbox.SetRect(x, y, x+w, y+h);
	bbox &= CRect(0, 0, spd.w, spd.h);

	// draw

	const byte* src = mpOverlayBuffer + 2*(mOverlayWidth * yo + xo);
	const byte* s = fBorder ? (src+1) : src;
	const byte* am = pAlphaMask + spd.w * y + x;
	unsigned long* dst = (unsigned long *)((char *)spd.bits + spd.pitch * y) + x;

	unsigned long color = switchpts[0];

	bool fSSE2 = !!(g_cpuid.m_flags & CCpuID::sse2);

	while(h--)
	{
		if(!pAlphaMask)
		{
			if(switchpts[1] == 0xffffffff)
			{
				if(fBody)
				{
					const byte* s = fBorder?(src+1):src;
/*
					for(int wt=0; wt<w; ++wt)
					{
						pixmix(s[wt*2]);
					}
*/
					/*__asm
					{
						pxor		mm0, mm0

						mov			eax, color
						mov			ebx, eax
						and			eax, 0x00ffffff
						movd		mm3, eax
						punpcklbw	mm3, mm0 // mm3 = color&0xffffff
						shr			ebx, 24 // bl = color>>24

						mov			ecx, w
						mov			esi, s
						mov			edi, dst

				pixmixloop:

						xor			eax, eax
						mov			al, [esi] // s[wt*2]
						imul		eax, ebx
						shr			eax, 6
						and			eax, 0xff
						imul		eax, 0x01010101

						movd		mm1, eax
						movq		mm2, _00ff00ff00ff00ff
						punpcklbw	mm1, mm0 // a
						psubsw		mm2, mm1 // ia
						pmullw		mm1, mm3 // a *= color
						movd		mm4, [edi]
						punpcklbw	mm4, mm0 // dst[wt]
						pmullw		mm2, mm4 // ia *= dst[wt]
						paddsw		mm1, mm2 // a*color += ia*dst[wt]
						psrlw		mm1, 8
						packuswb	mm1, mm1
						movd		[edi], mm1

						add			esi, 2
						add			edi, 4
						loop		pixmixloop
					}*/
					if(fSSE2)
						for(int wt=0; wt<w; ++wt)
							pixmix_sse2(&dst[wt], color, s[wt*2]<<6);
					else
						for(int wt=0; wt<w; ++wt)
							pixmix(&dst[wt], color, s[wt*2]<<6);
				}
				else
				{
/*					for(int wt=0; wt<w; ++wt)
					{
						pixmix(src[wt*2+1]-src[wt*2]);
					}
*/
					/*__asm
					{
						pxor		mm0, mm0

						mov			eax, color
						mov			ebx, eax
						and			eax, 0x00ffffff
						movd		mm3, eax
						punpcklbw	mm3, mm0 // mm3 = color&0xffffff
						shr			ebx, 24 // bl = color>>24

						mov			ecx, w
						mov			esi, src
						mov			edi, dst

					pixmixloop2:

						xor			eax, eax
						mov			al, [esi+1] // src[wt*2+1]-src[wt*2]
						sub			al, [esi]
						imul		eax, ebx
						shr			eax, 6
						and			eax, 0xff
						imul		eax, 0x01010101

						movd		mm1, eax
						movq		mm2, _00ff00ff00ff00ff
						punpcklbw	mm1, mm0 // a
						psubsw		mm2, mm1 // ia
						pmullw		mm1, mm3 // a *= color
						movd		mm4, [edi]
						punpcklbw	mm4, mm0 // dst[wt]
						pmullw		mm2, mm4 // ia *= dst[wt]
						paddsw		mm1, mm2 // a*color += ia*dst[wt]
						psrlw		mm1, 8
						packuswb	mm1, mm1
						movd		[edi], mm1

						add			esi, 2
						add			edi, 4
						loop		pixmixloop2
					}*/
					if(fSSE2)
						for(int wt=0; wt<w; ++wt)
							pixmix_sse2(&dst[wt], color, (src[wt*2+1] - src[wt*2])<<6);
					else
						for(int wt=0; wt<w; ++wt)
							pixmix(&dst[wt], color, (src[wt*2+1] - src[wt*2])<<6);
				}
				//__asm emms;
			}
			else
			{
				const long *sw = switchpts;

				if(fBody)
				{
					/*const byte* s = fBorder?(src+1):src;
					
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1])
						{
							while(wt+xo >= sw[1]) sw += 2;
							color = sw[-2];
						}

						pixmix(s[wt*2]);
					}*/
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];}
						pixmix_sse2(&dst[wt], color, s[wt*2]<<6);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];}
						pixmix(&dst[wt], color, s[wt*2]<<6);
					}
				}
				else
				{
					/*for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1])
						{
							while(wt+xo >= sw[1]) sw += 2;
							color = sw[-2];
						}

						pixmix(src[wt*2+1]-src[wt*2]);
					}*/
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];} 
						pixmix_sse2(&dst[wt], color, (src[wt*2+1] - src[wt*2])<<6);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {while(wt+xo >= sw[1]) sw += 2; color = sw[-2];} 
						pixmix(&dst[wt], color, (src[wt*2+1] - src[wt*2])<<6);
					}
				}
			}
		}
		else
		{
			if(switchpts[1] == 0xffffffff)
			{
				if(fBody)
				{
					/*const byte* s = fBorder?(src+1):src;

					for(int wt=0; wt<w; ++wt)
					{
						pixmix2(s[wt*2]);
					}*/
					if(fSSE2)
						for(int wt=0; wt<w; ++wt)
							pixmix_sse2(&dst[wt], color, s[wt*2] * am[wt]);
					else
						for(int wt=0; wt<w; ++wt)
							pixmix(&dst[wt], color, s[wt*2] * am[wt]);
				}
				else
				{
					/*for(int wt=0; wt<w; ++wt)
					{
						pixmix2(src[wt*2+1]-src[wt*2]);
					}*/
					if(fSSE2)
						for(int wt=0; wt<w; ++wt)
							pixmix_sse2(&dst[wt], color, (src[wt*2+1] - src[wt*2]) * am[wt]);
					else
						for(int wt=0; wt<w; ++wt)
							pixmix(&dst[wt], color, (src[wt*2+1] - src[wt*2]) * am[wt]);
				}
			}
			else
			{
				const long *sw = switchpts;

				if(fBody)
				{
					/*const byte* s = fBorder?(src+1):src;
					
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1])
						{
							while(wt+xo >= sw[1]) sw += 2;
							color = sw[-2];
						}

						pixmix2(s[wt*2]);
					}*/
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {
							while(wt+xo >= sw[1])
								sw += 2; color = sw[-2];
						}
						pixmix_sse2(&dst[wt], color, s[wt*2] * am[wt]);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {
							while(wt+xo >= sw[1])
								sw += 2; color = sw[-2];
						}
						pixmix(&dst[wt], color, s[wt*2] * am[wt]);
					}
				}
				else
				{
					/*for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1])
						{
							while(wt+xo >= sw[1]) sw += 2;
							color = sw[-2];
						}

						pixmix2(src[wt*2+1]-src[wt*2]);
					}*/
					if(fSSE2) 
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {
							while(wt+xo >= sw[1])
								sw += 2; color = sw[-2];
						} 
						pixmix_sse2(&dst[wt], color, (src[wt*2+1] - src[wt*2]) * am[wt]);
					}
					else
					for(int wt=0; wt<w; ++wt)
					{
						if(wt+xo >= sw[1]) {
							while(wt+xo >= sw[1])
								sw += 2; color = sw[-2];
						} 
						pixmix(&dst[wt], color, (src[wt*2+1] - src[wt*2]) * am[wt]);
					}
				}
			}
		}

		src += 2*mOverlayWidth;
		s += 2*mOverlayWidth;
		am += spd.w;
		dst = (unsigned long *)((char *)dst + spd.pitch);
	}

	return bbox;
}
