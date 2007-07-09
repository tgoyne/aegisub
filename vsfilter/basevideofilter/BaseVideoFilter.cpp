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

#include "StdAfx.h"
#include <mmintrin.h>
#include "BaseVideoFilter.h"
#include "../dsutil/DSUtil.h"
#include "../dsutil/MediaTypes.h"

#include <initguid.h>
#include "../include/moreuuids.h"

//
// CBaseVideoFilter
//

CBaseVideoFilter::CBaseVideoFilter(TCHAR* pName, LPUNKNOWN lpunk, HRESULT* phr, REFCLSID clsid, long cBuffers) 
	: CTransformFilter(pName, lpunk, clsid)
	, m_cBuffers(cBuffers)
{
	if(phr) *phr = S_OK;

	if(!(m_pInput = new CBaseVideoInputPin(NAME("CBaseVideoInputPin"), this, phr, L"Video"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr)) return;

	if(!(m_pOutput = new CBaseVideoOutputPin(NAME("CBaseVideoOutputPin"), this, phr, L"Output"))) *phr = E_OUTOFMEMORY;
	if(FAILED(*phr))  {delete m_pInput, m_pInput = NULL; return;}

	m_wout = m_win = m_w = 0;
	m_hout = m_hin = m_h = 0;
	m_arxout = m_arxin = m_arx = 0;
	m_aryout = m_aryin = m_ary = 0;
}

CBaseVideoFilter::~CBaseVideoFilter()
{
}

int CBaseVideoFilter::GetPinCount()
{
	return 2;
}

CBasePin* CBaseVideoFilter::GetPin(int n)
{
	switch(n)
	{
	case 0: return m_pInput;
	case 1: return m_pOutput;
	}
	return NULL;
}

HRESULT CBaseVideoFilter::Receive(IMediaSample* pIn)
{
	_mm_empty(); // just for safety

	CAutoLock cAutoLock(&m_csReceive);

	HRESULT hr;

    AM_SAMPLE2_PROPERTIES* const pProps = m_pInput->SampleProps();
    if(pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pIn);

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED(pIn->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt(*pmt);
		m_pInput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	if(FAILED(hr = Transform(pIn)))
		return hr;

	return S_OK;
}

HRESULT CBaseVideoFilter::GetDeliveryBuffer(int w, int h, IMediaSample** ppOut)
{
	CheckPointer(ppOut, E_POINTER);

	HRESULT hr;

	if(FAILED(hr = ReconnectOutput(w, h)))
		return hr;

	if(FAILED(hr = m_pOutput->GetDeliveryBuffer(ppOut, NULL, NULL, 0)))
		return hr;

	AM_MEDIA_TYPE* pmt;
	if(SUCCEEDED((*ppOut)->GetMediaType(&pmt)) && pmt)
	{
		CMediaType mt = *pmt;
		m_pOutput->SetMediaType(&mt);
		DeleteMediaType(pmt);
	}

	(*ppOut)->SetDiscontinuity(FALSE);
	(*ppOut)->SetSyncPoint(TRUE);

	// FIXME: hell knows why but without this the overlay mixer starts very skippy
	// (don't enable this for other renderers, the old for example will go crazy if you do)
	if(GetCLSID(m_pOutput->GetConnected()) == CLSID_OverlayMixer)
		(*ppOut)->SetDiscontinuity(TRUE);

	return S_OK;
}

HRESULT CBaseVideoFilter::ReconnectOutput(int w, int h)
{
	CMediaType& mt = m_pOutput->CurrentMediaType();

	int w_org = m_w;
	int h_org = m_h;

	bool fForceReconnection = false;
	if(w != m_w || h != m_h)
	{
		fForceReconnection = true;
		m_w = w;
		m_h = h;
	}

	HRESULT hr = S_OK;

	if(fForceReconnection || m_w != m_wout || m_h != m_hout || m_arx != m_arxout || m_ary != m_aryout)
	{
		if(GetCLSID(m_pOutput->GetConnected()) == CLSID_VideoRenderer)
		{
			NotifyEvent(EC_ERRORABORT, 0, 0);
			return E_FAIL;
		}

		BITMAPINFOHEADER* bmi = NULL;

		if(mt.formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_w, m_h);
			SetRect(&vih->rcTarget, 0, 0, m_w, m_h);
			bmi = &vih->bmiHeader;
			bmi->biXPelsPerMeter = m_w * m_ary;
			bmi->biYPelsPerMeter = m_h * m_arx;
		}
		else if(mt.formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)mt.Format();
			SetRect(&vih->rcSource, 0, 0, m_w, m_h);
			SetRect(&vih->rcTarget, 0, 0, m_w, m_h);
			bmi = &vih->bmiHeader;
			vih->dwPictAspectRatioX = m_arx;
			vih->dwPictAspectRatioY = m_ary;
		}

		bmi->biWidth = m_w;
		bmi->biHeight = m_h;
		bmi->biSizeImage = m_w*m_h*bmi->biBitCount>>3;

		hr = m_pOutput->GetConnected()->QueryAccept(&mt);
		ASSERT(SUCCEEDED(hr)); // should better not fail, after all "mt" is the current media type, just with a different resolution
HRESULT hr1 = 0, hr2 = 0;
		CComPtr<IMediaSample> pOut;
		if(SUCCEEDED(hr1 = m_pOutput->GetConnected()->ReceiveConnection(m_pOutput, &mt))
		&& SUCCEEDED(hr2 = m_pOutput->GetDeliveryBuffer(&pOut, NULL, NULL, 0)))
		{
			AM_MEDIA_TYPE* pmt;
			if(SUCCEEDED(pOut->GetMediaType(&pmt)) && pmt)
			{
				CMediaType mt = *pmt;
				m_pOutput->SetMediaType(&mt);
				DeleteMediaType(pmt);
			}
			else // stupid overlay mixer won't let us know the new pitch...
			{
				long size = pOut->GetSize();
				bmi->biWidth = size / bmi->biHeight * 8 / bmi->biBitCount;
			}
		}
		else
		{
			m_w = w_org;
			m_h = h_org;
			return E_FAIL;
		}

		m_wout = m_w;
		m_hout = m_h;
		m_arxout = m_arx;
		m_aryout = m_ary;

		// some renderers don't send this
		NotifyEvent(EC_VIDEO_SIZE_CHANGED, MAKELPARAM(m_w, m_h), 0);

		return S_OK;
	}

	return S_FALSE;
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE* pIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced)
{
	int abs_h = abs(h);
	BYTE* pInYUV[3] = {pIn, pIn + pitchIn*abs_h, pIn + pitchIn*abs_h + (pitchIn>>1)*(abs_h>>1)};
	return CopyBuffer(pOut, pInYUV, w, h, pitchIn, subtype, fInterlaced);
}

HRESULT CBaseVideoFilter::CopyBuffer(BYTE* pOut, BYTE** ppIn, int w, int h, int pitchIn, const GUID& subtype, bool fInterlaced)
{
	BITMAPINFOHEADER bihOut;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bihOut);

	int pitchOut = 0;

	if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
	{
		pitchOut = bihOut.biWidth*bihOut.biBitCount>>3;

		if(bihOut.biHeight > 0)
		{
			pOut += pitchOut*(h-1);
			pitchOut = -pitchOut;
			if(h < 0) h = -h;
		}
	}

	if(h < 0)
	{
		h = -h;
		ppIn[0] += pitchIn*(h-1);
		ppIn[1] += (pitchIn>>1)*((h>>1)-1);
		ppIn[2] += (pitchIn>>1)*((h>>1)-1);
		pitchIn = -pitchIn;
	}

	if(subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_YV12)
	{
		BYTE* pIn = ppIn[0];
		BYTE* pInU = ppIn[1];
		BYTE* pInV = ppIn[2];

		if(subtype == MEDIASUBTYPE_YV12) {BYTE* tmp = pInU; pInU = pInV; pInV = tmp;}

		BYTE* pOutU = pOut + bihOut.biWidth*h;
		BYTE* pOutV = pOut + bihOut.biWidth*h*5/4;

		if(bihOut.biCompression == '21VY') {BYTE* tmp = pOutU; pOutU = pOutV; pOutV = tmp;}

		ASSERT(w <= abs(pitchIn));

		if(bihOut.biCompression == '2YUY')
		{
			BitBltFromI420ToYUY2(w, h, pOut, bihOut.biWidth*2, pIn, pInU, pInV, pitchIn, fInterlaced);
		}
		else if(bihOut.biCompression == '024I' || bihOut.biCompression == 'VUYI' || bihOut.biCompression == '21VY')
		{
			BitBltFromI420ToI420(w, h, pOut, pOutU, pOutV, bihOut.biWidth, pIn, pInU, pInV, pitchIn);
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!BitBltFromI420ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, pIn, pInU, pInV, pitchIn))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
					memset(pOut, 0, pitchOut);
			}
		}
	}
	else if(subtype == MEDIASUBTYPE_YUY2)
	{
		if(bihOut.biCompression == '2YUY')
		{
			BitBltFromYUY2ToYUY2(w, h, pOut, bihOut.biWidth*2, ppIn[0], pitchIn);
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!BitBltFromYUY2ToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
					memset(pOut, 0, pitchOut);
			}
		}
	}
	else if(subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_RGB24 || subtype == MEDIASUBTYPE_RGB565)
	{
		int sbpp = 
			subtype == MEDIASUBTYPE_ARGB32 || subtype == MEDIASUBTYPE_RGB32 ? 32 :
			subtype == MEDIASUBTYPE_RGB24 ? 24 :
			subtype == MEDIASUBTYPE_RGB565 ? 16 : 0;

		if(bihOut.biCompression == '2YUY')
		{
			// TODO
			// BitBltFromRGBToYUY2();
		}
		else if(bihOut.biCompression == BI_RGB || bihOut.biCompression == BI_BITFIELDS)
		{
			if(!BitBltFromRGBToRGB(w, h, pOut, pitchOut, bihOut.biBitCount, ppIn[0], pitchIn, sbpp))
			{
				for(DWORD y = 0; y < h; y++, pOut += pitchOut)
					memset(pOut, 0, pitchOut);
			}
		}
	}
	else
	{
		return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::CheckInputType(const CMediaType* mtIn)
{
	BITMAPINFOHEADER bih;
	ExtractBIH(mtIn, &bih);

	return mtIn->majortype == MEDIATYPE_Video 
		&& (mtIn->subtype == MEDIASUBTYPE_YV12 
		 || mtIn->subtype == MEDIASUBTYPE_I420 
		 || mtIn->subtype == MEDIASUBTYPE_IYUV
		 || mtIn->subtype == MEDIASUBTYPE_YUY2
		 || mtIn->subtype == MEDIASUBTYPE_ARGB32
		 || mtIn->subtype == MEDIASUBTYPE_RGB32
		 || mtIn->subtype == MEDIASUBTYPE_RGB24
 		 || mtIn->subtype == MEDIASUBTYPE_RGB565)
		&& (mtIn->formattype == FORMAT_VideoInfo 
		 || mtIn->formattype == FORMAT_VideoInfo2)
		&& bih.biHeight > 0
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CBaseVideoFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	if(FAILED(CheckInputType(mtIn)) || mtOut->majortype != MEDIATYPE_Video)
		return VFW_E_TYPE_NOT_ACCEPTED;

	if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_YV12 
	 || mtIn->subtype == MEDIASUBTYPE_I420 
	 || mtIn->subtype == MEDIASUBTYPE_IYUV))
	{
		if(mtOut->subtype != MEDIASUBTYPE_YV12
		&& mtOut->subtype != MEDIASUBTYPE_I420
		&& mtOut->subtype != MEDIASUBTYPE_IYUV
		&& mtOut->subtype != MEDIASUBTYPE_YUY2
		&& mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}
	else if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_YUY2))
	{
		if(mtOut->subtype != MEDIASUBTYPE_YUY2
		&& mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}
	else if(mtIn->majortype == MEDIATYPE_Video 
	&& (mtIn->subtype == MEDIASUBTYPE_ARGB32
	|| mtIn->subtype == MEDIASUBTYPE_RGB32
	|| mtIn->subtype == MEDIASUBTYPE_RGB24
	|| mtIn->subtype == MEDIASUBTYPE_RGB565))
	{
		if(mtOut->subtype != MEDIASUBTYPE_ARGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB32
		&& mtOut->subtype != MEDIASUBTYPE_RGB24
		&& mtOut->subtype != MEDIASUBTYPE_RGB565)
			return VFW_E_TYPE_NOT_ACCEPTED;
	}

	return S_OK;
}

HRESULT CBaseVideoFilter::CheckOutputType(const CMediaType& mtOut)
{
	int wout = 0, hout = 0, arxout = 0, aryout = 0;
	return ExtractDim(&mtOut, wout, hout, arxout, aryout)
		&& m_h == abs((int)hout)
		&& mtOut.subtype != MEDIASUBTYPE_ARGB32
		? S_OK
		: VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CBaseVideoFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProperties)
{
	if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	BITMAPINFOHEADER bih;
	ExtractBIH(&m_pOutput->CurrentMediaType(), &bih);

	long cBuffers = m_pOutput->CurrentMediaType().formattype == FORMAT_VideoInfo ? 1 : m_cBuffers;

	pProperties->cBuffers = m_cBuffers;
	pProperties->cbBuffer = bih.biSizeImage;
	pProperties->cbAlign = 1;
	pProperties->cbPrefix = 0;

	HRESULT hr;
	ALLOCATOR_PROPERTIES Actual;
    if(FAILED(hr = pAllocator->SetProperties(pProperties, &Actual))) 
		return hr;

    return pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer
		? E_FAIL
		: NOERROR;
}

HRESULT CBaseVideoFilter::GetMediaType(int iPosition, CMediaType* pmt)
{
    if(m_pInput->IsConnected() == FALSE) return E_UNEXPECTED;

	struct {const GUID* subtype; WORD biPlanes, biBitCount; DWORD biCompression;} fmts[] =
	{
		{&MEDIASUBTYPE_YV12, 3, 12, '21VY'},
		{&MEDIASUBTYPE_I420, 3, 12, '024I'},
		{&MEDIASUBTYPE_IYUV, 3, 12, 'VUYI'},
		{&MEDIASUBTYPE_YUY2, 1, 16, '2YUY'},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_RGB},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_RGB},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_RGB},
		{&MEDIASUBTYPE_ARGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB32, 1, 32, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB24, 1, 24, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB565, 1, 16, BI_BITFIELDS},
		{&MEDIASUBTYPE_RGB555, 1, 16, BI_BITFIELDS},
	};

	// this will make sure we won't connect to the old renderer in dvd mode
	// that renderer can't switch the format dynamically

	bool fFoundDVDNavigator = false;
	CComPtr<IBaseFilter> pBF = this;
	CComPtr<IPin> pPin = m_pInput;
	for(; !fFoundDVDNavigator && (pBF = GetUpStreamFilter(pBF, pPin)); pPin = GetFirstPin(pBF))
        fFoundDVDNavigator = GetCLSID(pBF) == CLSID_DVDNavigator;

	if(fFoundDVDNavigator || m_pInput->CurrentMediaType().formattype == FORMAT_VideoInfo2)
		iPosition = iPosition*2;

	//

	if(iPosition < 0) return E_INVALIDARG;
	if(iPosition >= 2*countof(fmts)) return VFW_S_NO_MORE_ITEMS;

	pmt->majortype = MEDIATYPE_Video;
	pmt->subtype = *fmts[iPosition/2].subtype;

	int w = m_win, h = m_hin, arx = m_arxin, ary = m_aryin;
	GetOutputSize(w, h, arx, ary);

	BITMAPINFOHEADER bihOut;
	memset(&bihOut, 0, sizeof(bihOut));
	bihOut.biSize = sizeof(bihOut);
	bihOut.biWidth = w;
	bihOut.biHeight = h;
	bihOut.biPlanes = fmts[iPosition/2].biPlanes;
	bihOut.biBitCount = fmts[iPosition/2].biBitCount;
	bihOut.biCompression = fmts[iPosition/2].biCompression;
	bihOut.biSizeImage = w*h*bihOut.biBitCount>>3;

	if(iPosition&1)
	{
		pmt->formattype = FORMAT_VideoInfo;
		VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		memset(vih, 0, sizeof(VIDEOINFOHEADER));
		vih->bmiHeader = bihOut;
		vih->bmiHeader.biXPelsPerMeter = vih->bmiHeader.biWidth * ary;
		vih->bmiHeader.biYPelsPerMeter = vih->bmiHeader.biHeight * arx;
	}
	else
	{
		pmt->formattype = FORMAT_VideoInfo2;
		VIDEOINFOHEADER2* vih = (VIDEOINFOHEADER2*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
		memset(vih, 0, sizeof(VIDEOINFOHEADER2));
		vih->bmiHeader = bihOut;
		vih->dwPictAspectRatioX = arx;
		vih->dwPictAspectRatioY = ary;
		if(IsVideoInterlaced()) vih->dwInterlaceFlags = AMINTERLACE_IsInterlaced;
	}

	CMediaType& mt = m_pInput->CurrentMediaType();

	// these fields have the same field offset in all four structs
	((VIDEOINFOHEADER*)pmt->Format())->AvgTimePerFrame = ((VIDEOINFOHEADER*)mt.Format())->AvgTimePerFrame;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitRate;
	((VIDEOINFOHEADER*)pmt->Format())->dwBitErrorRate = ((VIDEOINFOHEADER*)mt.Format())->dwBitErrorRate;

	CorrectMediaType(pmt);

	return S_OK;
}

HRESULT CBaseVideoFilter::SetMediaType(PIN_DIRECTION dir, const CMediaType* pmt)
{
	if(dir == PINDIR_INPUT)
	{
		m_w = m_h = m_arx = m_ary = 0;
		ExtractDim(pmt, m_w, m_h, m_arx, m_ary);
		m_win = m_w;
		m_hin = m_h;
		m_arxin = m_arx;
		m_aryin = m_ary;
		GetOutputSize(m_w, m_h, m_arx, m_ary);
	}
	else if(dir == PINDIR_OUTPUT)
	{
		int wout = 0, hout = 0, arxout = 0, aryout = 0;
		ExtractDim(pmt, wout, hout, arxout, aryout);
		if(m_w == wout && m_h == hout && m_arx == arxout && m_ary == aryout)
		{
			m_wout = wout;
			m_hout = hout;
			m_arxout = arxout;
			m_aryout = aryout;
		}
	}

	return __super::SetMediaType(dir, pmt);
}

//
// CBaseVideoInputAllocator
//
	
CBaseVideoInputAllocator::CBaseVideoInputAllocator(HRESULT* phr)
	: CMemAllocator(NAME("CBaseVideoInputAllocator"), NULL, phr)
{
	if(phr) *phr = S_OK;
}

void CBaseVideoInputAllocator::SetMediaType(const CMediaType& mt)
{
	m_mt = mt;
}

STDMETHODIMP CBaseVideoInputAllocator::GetBuffer(IMediaSample** ppBuffer, REFERENCE_TIME* pStartTime, REFERENCE_TIME* pEndTime, DWORD dwFlags)
{
	if(!m_bCommitted)
        return VFW_E_NOT_COMMITTED;

	HRESULT hr = __super::GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);

	if(SUCCEEDED(hr) && m_mt.majortype != GUID_NULL)
	{
		(*ppBuffer)->SetMediaType(&m_mt);
		m_mt.majortype = GUID_NULL;
	}

	return hr;
}

//
// CBaseVideoInputPin
//

CBaseVideoInputPin::CBaseVideoInputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName) 
	: CTransformInputPin(pObjectName, pFilter, phr, pName)
	, m_pAllocator(NULL)
{
}

CBaseVideoInputPin::~CBaseVideoInputPin()
{
	delete m_pAllocator;
}

STDMETHODIMP CBaseVideoInputPin::GetAllocator(IMemAllocator** ppAllocator)
{
    CheckPointer(ppAllocator, E_POINTER);

    if(m_pAllocator == NULL)
	{
		HRESULT hr = S_OK;
        m_pAllocator = new CBaseVideoInputAllocator(&hr);
        m_pAllocator->AddRef();
    }

    (*ppAllocator = m_pAllocator)->AddRef();

    return S_OK;
} 

STDMETHODIMP CBaseVideoInputPin::ReceiveConnection(IPin* pConnector, const AM_MEDIA_TYPE* pmt)
{
	CAutoLock cObjectLock(m_pLock);

	if(m_Connected)
	{
		CMediaType mt(*pmt);

		if(FAILED(CheckMediaType(&mt)))
			return VFW_E_TYPE_NOT_ACCEPTED;

		ALLOCATOR_PROPERTIES props, actual;

		CComPtr<IMemAllocator> pMemAllocator;
		if(FAILED(GetAllocator(&pMemAllocator))
		|| FAILED(pMemAllocator->Decommit())
		|| FAILED(pMemAllocator->GetProperties(&props)))
			return E_FAIL;

		BITMAPINFOHEADER bih;
		if(ExtractBIH(pmt, &bih) && bih.biSizeImage)
			props.cbBuffer = bih.biSizeImage;

		if(FAILED(pMemAllocator->SetProperties(&props, &actual))
		|| FAILED(pMemAllocator->Commit())
		|| props.cbBuffer != actual.cbBuffer)
			return E_FAIL;

		if(m_pAllocator) 
			m_pAllocator->SetMediaType(mt);

		return SetMediaType(&mt) == S_OK
			? S_OK
			: VFW_E_TYPE_NOT_ACCEPTED;
	}

	return __super::ReceiveConnection(pConnector, pmt);
}

//
// CBaseVideoOutputPin
//

CBaseVideoOutputPin::CBaseVideoOutputPin(TCHAR* pObjectName, CBaseVideoFilter* pFilter, HRESULT* phr, LPCWSTR pName)
	: CTransformOutputPin(pObjectName, pFilter, phr, pName)
{
}

HRESULT CBaseVideoOutputPin::CheckMediaType(const CMediaType* mtOut)
{
	if(IsConnected())
	{
		HRESULT hr = ((CBaseVideoFilter*)m_pFilter)->CheckOutputType(*mtOut);
		if(FAILED(hr)) return hr;
	}

	return __super::CheckMediaType(mtOut);
}
