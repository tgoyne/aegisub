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
#include <afxdlgs.h>
#include <atlpath.h>
#include "resource.h"
#include "subtitles/VobSubFile.h"
#include "subtitles/RTS.h"
#include "subtitles/SSF.h"
#include "SubPic/MemSubPic.h"

//
// Generic interface
//

namespace Plugin
{

class CFilter : public CAMThread, public CCritSec
{
private:
	CString m_fn;

protected:
	float m_fps;
	CCritSec m_csSubLock;
	CComPtr<ISubPicQueue> m_pSubPicQueue;
	CComPtr<ISubPicProvider> m_pSubPicProvider;
	DWORD_PTR m_SubPicProviderId;

public:
	CFilter() : m_fps(-1), m_SubPicProviderId(0) {CAMThread::Create();}
	virtual ~CFilter() {CAMThread::CallWorker(0);}

	CString GetFileName() {CAutoLock cAutoLock(this); return m_fn;}
	void SetFileName(CString fn) {CAutoLock cAutoLock(this); m_fn = fn;}

	bool Render(SubPicDesc& dst, REFERENCE_TIME rt, float fps)
	{
		if(!m_pSubPicProvider)
			return(false);

		CSize size(dst.w, dst.h);

		if(!m_pSubPicQueue)
		{
			CComPtr<ISubPicAllocator> pAllocator = new CMemSubPicAllocator(dst.type, size);

			HRESULT hr;
			if(!(m_pSubPicQueue = new CSubPicQueueNoThread(pAllocator, &hr)) || FAILED(hr))
			{
				m_pSubPicQueue = NULL;
				return(false);
			}
		}

		if(m_SubPicProviderId != (DWORD_PTR)(ISubPicProvider*)m_pSubPicProvider)
		{
			m_pSubPicQueue->SetSubPicProvider(m_pSubPicProvider);
			m_SubPicProviderId = (DWORD_PTR)(ISubPicProvider*)m_pSubPicProvider;
		}

		CComPtr<ISubPic> pSubPic;
		if(!m_pSubPicQueue->LookupSubPic(rt, &pSubPic))
			return(false);

		CRect r;
		pSubPic->GetDirtyRect(r);

		if(dst.type == MSP_RGB32 || dst.type == MSP_RGB24 || dst.type == MSP_RGB16 || dst.type == MSP_RGB15)
			dst.h = -dst.h;

		pSubPic->AlphaBlt(r, r, &dst);

		return(true);
	}

	DWORD ThreadProc()
	{
		SetThreadPriority(m_hThread, THREAD_PRIORITY_LOWEST);

		CAtlArray<HANDLE> handles;
		handles.Add(GetRequestHandle());

		CString fn = GetFileName();
		CFileStatus fs;
		fs.m_mtime = 0;
		CFileGetStatus(fn, fs);

		while(1)
		{
			DWORD i = WaitForMultipleObjects(handles.GetCount(), handles.GetData(), FALSE, 1000);

			if(WAIT_OBJECT_0 == i)
			{
				Reply(S_OK);
				break;
			}
			else if(WAIT_OBJECT_0 + 1 >= i && i <= WAIT_OBJECT_0 + handles.GetCount())
			{
				if(FindNextChangeNotification(handles[i - WAIT_OBJECT_0]))
				{
					CFileStatus fs2;
					fs2.m_mtime = 0;
					CFileGetStatus(fn, fs2);

					if(fs.m_mtime < fs2.m_mtime)
					{
						fs.m_mtime = fs2.m_mtime;

						if(CComQIPtr<ISubStream> pSubStream = m_pSubPicProvider)
						{
							CAutoLock cAutoLock(&m_csSubLock);
							pSubStream->Reload();
						}
					}
				}
			}
			else if(WAIT_TIMEOUT == i)
			{
				CString fn2 = GetFileName();

				if(fn != fn2)
				{
					CPath p(fn2);
					p.RemoveFileSpec();
					HANDLE h = FindFirstChangeNotification(p, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE); 
					if(h != INVALID_HANDLE_VALUE)
					{
						fn = fn2;
						handles.SetCount(1);
						handles.Add(h);
					}
				}
			}
			else // if(WAIT_ABANDONED_0 == i || WAIT_FAILED == i)
			{
				break;
			}
		}

		m_hThread = 0;

		for(int i = 1; i < handles.GetCount(); i++)
			FindCloseChangeNotification(handles[i]);

		return 0;
	}
};

class CVobSubFilter : virtual public CFilter
{
public:
	CVobSubFilter(CString fn = _T(""))
	{
		if(!fn.IsEmpty()) Open(fn);
	}

	bool Open(CString fn)
	{
		SetFileName(_T(""));
		m_pSubPicProvider = NULL;

		if(CVobSubFile* vsf = new CVobSubFile(&m_csSubLock))
		{
			m_pSubPicProvider = (ISubPicProvider*)vsf;
			if(vsf->Open(CString(fn))) SetFileName(fn);
			else m_pSubPicProvider = NULL;
		}

		return !!m_pSubPicProvider;
	}
};

class CTextSubFilter : virtual public CFilter
{
	int m_CharSet;

public:
	CTextSubFilter(CString fn = _T(""), int CharSet = DEFAULT_CHARSET, float fps = -1)
		: m_CharSet(CharSet)
	{
		m_fps = fps;
		if(!fn.IsEmpty()) Open(fn, CharSet);
	}

	int GetCharSet() {return(m_CharSet);}

	bool Open(CString fn, int CharSet = DEFAULT_CHARSET)
	{
		SetFileName(_T(""));
		m_pSubPicProvider = NULL;

		if(!m_pSubPicProvider)
		{
			if(ssf::CRenderer* ssf = new ssf::CRenderer(&m_csSubLock))
			{
				m_pSubPicProvider = (ISubPicProvider*)ssf;
				if(ssf->Open(CString(fn))) SetFileName(fn);
				else m_pSubPicProvider = NULL;
			}
		}

		if(!m_pSubPicProvider)
		{
			if(CRenderedTextSubtitle* rts = new CRenderedTextSubtitle(&m_csSubLock))
			{
				m_pSubPicProvider = (ISubPicProvider*)rts;
				if(rts->Open(CString(fn), CharSet)) SetFileName(fn);
				else m_pSubPicProvider = NULL;
			}
		}

		return !!m_pSubPicProvider;
	}
};

//
// VirtualDub interface
//

namespace VirtualDub
{
	#include "include/VirtualDub/VirtualDub.h"

	class CVirtualDubFilter : virtual public CFilter
	{
	public:
		CVirtualDubFilter() {}
		virtual ~CVirtualDubFilter() {}

		virtual int RunProc(const FilterActivation* fa, const FilterFunctions* ff)
		{
			SubPicDesc dst;
			dst.type = MSP_RGB32;
			dst.w = fa->src.w;
			dst.h = fa->src.h;
			dst.bpp = 32;
			dst.pitch = fa->src.pitch;
			dst.bits = (LPVOID)fa->src.data;

			Render(dst, 10000i64*fa->pfsi->lSourceFrameMS, (float)1000 / fa->pfsi->lMicrosecsPerFrame);

			return 0;
		}

		virtual long ParamProc(FilterActivation* fa, const FilterFunctions* ff)
		{
			fa->dst.offset	= fa->src.offset;
			fa->dst.modulo	= fa->src.modulo;
			fa->dst.pitch	= fa->src.pitch;

			return 0;
		}

		virtual int ConfigProc(FilterActivation* fa, const FilterFunctions* ff, HWND hwnd) = 0;
		virtual void StringProc(const FilterActivation* fa, const FilterFunctions* ff, char* str) = 0;
		virtual bool FssProc(FilterActivation* fa, const FilterFunctions* ff, char* buf, int buflen) = 0;
	};

	class CVobSubVirtualDubFilter : public CVobSubFilter, public CVirtualDubFilter
	{
	public:
		CVobSubVirtualDubFilter(CString fn = _T("")) 
			: CVobSubFilter(fn) {}

		int ConfigProc(FilterActivation* fa, const FilterFunctions* ff, HWND hwnd)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			CFileDialog fd(TRUE, NULL, GetFileName(), OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY, 
				_T("VobSub files (*.idx;*.sub)|*.idx;*.sub||"), CWnd::FromHandle(hwnd), 0);

			if(fd.DoModal() != IDOK) return 1;

			return Open(fd.GetPathName()) ? 0 : 1;
		}

		void StringProc(const FilterActivation* fa, const FilterFunctions* ff, char* str)
		{
			sprintf(str, " (%s)", !GetFileName().IsEmpty() ? CStringA(GetFileName()) : " (empty)");
		}

		bool FssProc(FilterActivation* fa, const FilterFunctions* ff, char* buf, int buflen)
		{
			CStringA fn(GetFileName());
			fn.Replace("\\", "\\\\");
			_snprintf(buf, buflen, "Config(\"%s\")", fn);
			return(true);
		}
	};

	class CTextSubVirtualDubFilter : public CTextSubFilter, public CVirtualDubFilter
	{
	public:
		CTextSubVirtualDubFilter(CString fn = _T(""), int CharSet = DEFAULT_CHARSET) 
			: CTextSubFilter(fn, CharSet) {}

		int ConfigProc(FilterActivation* fa, const FilterFunctions* ff, HWND hwnd)
		{
			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			const TCHAR formats[] = _T("TextSub files (*.sub;*.srt;*.smi;*.ssa;*.ass;*.xss;*.psb;*.txt)|*.sub;*.srt;*.smi;*.ssa;*.ass;*.xss;*.psb;*.txt||");
			CFileDialog fd(TRUE, NULL, GetFileName(), OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK, 
				formats, CWnd::FromHandle(hwnd), OPENFILENAME_SIZE_VERSION_400 /*0*/);

			UINT CALLBACK OpenHookProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam);

			fd.m_pOFN->hInstance = AfxGetResourceHandle();
			fd.m_pOFN->lpTemplateName = MAKEINTRESOURCE(IDD_TEXTSUBOPENTEMPLATE);
			fd.m_pOFN->lpfnHook = OpenHookProc;
			fd.m_pOFN->lCustData = (LPARAM)DEFAULT_CHARSET;

			if(fd.DoModal() != IDOK) return 1;

			return Open(fd.GetPathName(), fd.m_pOFN->lCustData) ? 0 : 1;
		}

		void StringProc(const FilterActivation* fa, const FilterFunctions* ff, char* str)
		{
			if(!GetFileName().IsEmpty()) sprintf(str, " (%s, %d)", CStringA(GetFileName()), GetCharSet());
			else sprintf(str, " (empty)");
		}

		bool FssProc(FilterActivation* fa, const FilterFunctions* ff, char* buf, int buflen)
		{
			CStringA fn(GetFileName());
			fn.Replace("\\", "\\\\");
			_snprintf(buf, buflen, "Config(\"%s\", %d)", fn, GetCharSet());
			return(true);
		}
	};

	int vobsubInitProc(FilterActivation* fa, const FilterFunctions* ff)
	{
		return !(*(CVirtualDubFilter**)fa->filter_data = new CVobSubVirtualDubFilter());
	}

	int textsubInitProc(FilterActivation* fa, const FilterFunctions* ff)
	{
		return !(*(CVirtualDubFilter**)fa->filter_data = new CTextSubVirtualDubFilter());
	}

	void baseDeinitProc(FilterActivation* fa, const FilterFunctions* ff)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		if(f) delete f, f = NULL;
	}

	int baseRunProc(const FilterActivation* fa, const FilterFunctions* ff)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		return f ? f->RunProc(fa, ff) : 1;
	}

	long baseParamProc(FilterActivation* fa, const FilterFunctions* ff)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		return f ? f->ParamProc(fa, ff) : 1;
	}

	int baseConfigProc(FilterActivation* fa, const FilterFunctions* ff, HWND hwnd)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		return f ? f->ConfigProc(fa, ff, hwnd) : 1;
	}

	void baseStringProc(const FilterActivation* fa, const FilterFunctions* ff, char* str)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		if(f) f->StringProc(fa, ff, str);
	}

	bool baseFssProc(FilterActivation* fa, const FilterFunctions* ff, char* buf, int buflen)
	{
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		return f ? f->FssProc(fa, ff, buf, buflen) : false;
	}

	void vobsubScriptConfig(IScriptInterpreter* isi, void* lpVoid, CScriptValue* argv, int argc)
	{
		FilterActivation* fa = (FilterActivation*)lpVoid;
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		if(f) delete f;
		f = new CVobSubVirtualDubFilter(CString(*argv[0].asString()));
		*(CVirtualDubFilter**)fa->filter_data = f;
	}

	void textsubScriptConfig(IScriptInterpreter* isi, void* lpVoid, CScriptValue* argv, int argc)
	{
		FilterActivation* fa = (FilterActivation*)lpVoid;
		CVirtualDubFilter* f = *(CVirtualDubFilter**)fa->filter_data;
		if(f) delete f;
		f = new CTextSubVirtualDubFilter(CString(*argv[0].asString()), argv[1].asInt());
		*(CVirtualDubFilter**)fa->filter_data = f;
	}

	ScriptFunctionDef vobsub_func_defs[]={
		{ (ScriptFunctionPtr)vobsubScriptConfig, "Config", "0s" },
		{ NULL },
	};

	CScriptObject vobsub_obj={
		NULL, vobsub_func_defs
	};

	struct FilterDefinition filterDef_vobsub = 
	{
		NULL, NULL, NULL,       // next, prev, module
		"VobSub",				// name
		"Adds subtitles from a vob sequence.", // desc
		"Gabest",               // maker
		NULL,                   // private_data
		sizeof(CVirtualDubFilter**), // inst_data_size
		vobsubInitProc,         // initProc
		baseDeinitProc,			// deinitProc
		baseRunProc,			// runProc
		baseParamProc,			// paramProc
		baseConfigProc,			// configProc
		baseStringProc,			// stringProc
		NULL,					// startProc
		NULL,					// endProc
		&vobsub_obj,			// script_obj
		baseFssProc,			// fssProc
	};

	ScriptFunctionDef textsub_func_defs[]={
		{ (ScriptFunctionPtr)textsubScriptConfig, "Config", "0si" },
		{ NULL },
	};

	CScriptObject textsub_obj={
		NULL, textsub_func_defs
	};

	struct FilterDefinition filterDef_textsub = 
	{
		NULL, NULL, NULL,       // next, prev, module
		"TextSub",				// name
		"Adds subtitles from srt, sub, psb, smi, ssa, ass file formats.", // desc
		"Gabest",               // maker
		NULL,                   // private_data
		sizeof(CVirtualDubFilter**), // inst_data_size
		textsubInitProc,        // initProc
		baseDeinitProc,			// deinitProc
		baseRunProc,			// runProc
		baseParamProc,			// paramProc
		baseConfigProc,			// configProc
		baseStringProc,			// stringProc
		NULL,					// startProc
		NULL,					// endProc
		&textsub_obj,			// script_obj
		baseFssProc,			// fssProc
	};

	static FilterDefinition* fd_vobsub;
	static FilterDefinition* fd_textsub;

	extern "C" __declspec(dllexport) int __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat)
	{
		if(!(fd_vobsub = ff->addFilter(fm, &filterDef_vobsub, sizeof(FilterDefinition)))
		|| !(fd_textsub = ff->addFilter(fm, &filterDef_textsub, sizeof(FilterDefinition))))
			return 1;

		vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
		vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE;

		return 0;
	}

	extern "C" __declspec(dllexport) void __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff)
	{
		ff->removeFilter(fd_textsub);
		ff->removeFilter(fd_vobsub);
	}
}

//
// Avisynth interface
//

namespace AviSynth1
{
	#include "include/avisynth/avisynth1.h"

	class CAvisynthFilter : public GenericVideoFilter, virtual public CFilter
	{
	public:
		CAvisynthFilter(PClip c, IScriptEnvironment* env) : GenericVideoFilter(c) {}

		PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
		{
			PVideoFrame frame = child->GetFrame(n, env);

			env->MakeWritable(&frame);

			SubPicDesc dst;
			dst.w = vi.width;
			dst.h = vi.height;
			dst.pitch = frame->GetPitch();
			dst.bits = (void**)frame->GetWritePtr();
			dst.bpp = vi.BitsPerPixel();
			dst.type = 
				vi.IsRGB32() ? MSP_RGB32 : 
				vi.IsRGB24() ? MSP_RGB24 : 
				vi.IsYUY2() ? MSP_YUY2 : 
				-1;

			float fps = m_fps > 0 ? m_fps : (float)vi.fps_numerator / vi.fps_denominator;

			Render(dst, (REFERENCE_TIME)(10000000i64 * n / fps), fps);

			return(frame);
		}
	};

	class CVobSubAvisynthFilter : public CVobSubFilter, public CAvisynthFilter
	{
	public:
		CVobSubAvisynthFilter(PClip c, const char* fn, IScriptEnvironment* env)
			: CVobSubFilter(CString(fn))
			, CAvisynthFilter(c, env)
		{
			if(!m_pSubPicProvider)
				env->ThrowError("VobSub: Can't open \"%s\"", fn);
		}
	};

	AVSValue __cdecl VobSubCreateS(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CVobSubAvisynthFilter(args[0].AsClip(), args[1].AsString(), env));
	}
    
	class CTextSubAvisynthFilter : public CTextSubFilter, public CAvisynthFilter
	{
	public:
		CTextSubAvisynthFilter(PClip c, IScriptEnvironment* env, const char* fn, int CharSet = DEFAULT_CHARSET, float fps = -1)
			: CTextSubFilter(CString(fn), CharSet, fps)
			, CAvisynthFilter(c, env)
		{
			if(!m_pSubPicProvider)
				env->ThrowError("TextSub: Can't open \"%s\"", fn);
		}
	};

	AVSValue __cdecl TextSubCreateS(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString()));
	}

	AVSValue __cdecl TextSubCreateSI(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString(), args[2].AsInt()));
	}

	AVSValue __cdecl TextSubCreateSIF(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString(), args[2].AsInt(), args[3].AsFloat()));
	}

	extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit(IScriptEnvironment* env)
	{
		env->AddFunction("VobSub", "cs", VobSubCreateS, 0);
		env->AddFunction("TextSub", "cs", TextSubCreateS, 0);
		env->AddFunction("TextSub", "csi", TextSubCreateSI, 0);
		env->AddFunction("TextSub", "csif", TextSubCreateSIF, 0);
		return(NULL);
	}
}

namespace AviSynth25
{
	#include "include/avisynth/avisynth25.h"

	static bool s_fSwapUV = false;

	class CAvisynthFilter : public GenericVideoFilter, virtual public CFilter
	{
	public:
		CAvisynthFilter(PClip c, IScriptEnvironment* env) : GenericVideoFilter(c) {}

		PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
		{
			PVideoFrame frame = child->GetFrame(n, env);

			env->MakeWritable(&frame);

			SubPicDesc dst;
			dst.w = vi.width;
			dst.h = vi.height;
			dst.pitch = frame->GetPitch();
			dst.pitchUV = frame->GetPitch(PLANAR_U);
			dst.bits = (void**)frame->GetWritePtr();
			dst.bitsU = frame->GetWritePtr(PLANAR_U);
			dst.bitsV = frame->GetWritePtr(PLANAR_V);
			dst.bpp = dst.pitch/dst.w*8; //vi.BitsPerPixel();
			dst.type = 
				vi.IsRGB32() ? MSP_RGB32 : 
				vi.IsRGB24() ? MSP_RGB24 : 
				vi.IsYUY2() ? MSP_YUY2 : 
				/*vi.IsYV12()*/ vi.pixel_type == VideoInfo::CS_YV12 ? (s_fSwapUV?MSP_IYUV:MSP_YV12) : 
				/*vi.IsIYUV()*/ vi.pixel_type == VideoInfo::CS_IYUV ? (s_fSwapUV?MSP_YV12:MSP_IYUV) : 
				-1;

			float fps = m_fps > 0 ? m_fps : (float)vi.fps_numerator / vi.fps_denominator;

			Render(dst, (REFERENCE_TIME)(10000000i64 * n / fps), fps);

			return(frame);
		}
	};

	class CVobSubAvisynthFilter : public CVobSubFilter, public CAvisynthFilter
	{
	public:
		CVobSubAvisynthFilter(PClip c, const char* fn, IScriptEnvironment* env)
			: CVobSubFilter(CString(fn))
			, CAvisynthFilter(c, env)
		{
			if(!m_pSubPicProvider)
				env->ThrowError("VobSub: Can't open \"%s\"", fn);
		}
	};

	AVSValue __cdecl VobSubCreateS(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CVobSubAvisynthFilter(args[0].AsClip(), args[1].AsString(), env));
	}
    
	class CTextSubAvisynthFilter : public CTextSubFilter, public CAvisynthFilter
	{
	public:
		CTextSubAvisynthFilter(PClip c, IScriptEnvironment* env, const char* fn, int CharSet = DEFAULT_CHARSET, float fps = -1)
			: CTextSubFilter(CString(fn), CharSet, fps)
			, CAvisynthFilter(c, env)
		{
			if(!m_pSubPicProvider)
				env->ThrowError("TextSub: Can't open \"%s\"", fn);
		}
	};

	AVSValue __cdecl TextSubCreateS(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString()));
	}

	AVSValue __cdecl TextSubCreateSI(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString(), args[2].AsInt()));
	}

	AVSValue __cdecl TextSubCreateSIF(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		return(new CTextSubAvisynthFilter(args[0].AsClip(), env, args[1].AsString(), args[2].AsInt(), args[3].AsFloat()));
	}

	AVSValue __cdecl TextSubSwapUV(AVSValue args, void* user_data, IScriptEnvironment* env)
	{
		s_fSwapUV = args[0].AsBool(false);
		return(AVSValue());
	}

	extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
	{
		env->AddFunction("VobSub", "cs", VobSubCreateS, 0);
		env->AddFunction("TextSub", "cs", TextSubCreateS, 0);
		env->AddFunction("TextSub", "csi", TextSubCreateSI, 0);
		env->AddFunction("TextSub", "csif", TextSubCreateSIF, 0);
		env->AddFunction("TextSubSwapUV", "b", TextSubSwapUV, 0);
		return(NULL);
	}
}

}

UINT CALLBACK OpenHookProc(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uiMsg)
	{
		case WM_NOTIFY:
		{
			OPENFILENAME* ofn = ((OFNOTIFY *)lParam)->lpOFN;

			if(((NMHDR *)lParam)->code == CDN_FILEOK)
			{
				ofn->lCustData = (LPARAM)CharSetList[SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCURSEL, 0, 0)];
			}

			break;
		}

		case WM_INITDIALOG:
		{
			SetWindowLong(hDlg, GWL_USERDATA, lParam);

			for(int i = 0; i < CharSetLen; i++)
			{
				CString s;
				s.Format(_T("%s (%d)"), CharSetNames[i], CharSetList[i]);
				SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING, 0, (LONG)(LPCTSTR)s);
				if(CharSetList[i] == (int)((OPENFILENAME*)lParam)->lCustData)
					SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL, i, 0);
			}

			break;
		}
		
		default:
			break;
	}

	return FALSE;
}
