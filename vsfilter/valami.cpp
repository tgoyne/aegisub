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

#include "DSUtil/DSUtil.h"

static TCHAR str1[][256] = 
/*
{
	_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{9D2935C7-3D8B-4EF6-B0D1-C14064698794}"), // divxg400
	_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{8CE3343E-2289-4BAE-AE57-5106A40AF552}"), // divxg400force
	_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{00A95963-3BE5-48C0-AD9F-3356D67EA09D}"), // ogg sub mixer
	_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{70E102B0-5556-11CE-97C0-00AA0055595A}"), // video renderer (old)
	_T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{6BC1CFFA-8FC1-4261-AC22-CFB4CC38DB50}"), // video renderer (vmr)
};
*/
{
	{
		0x51, 0x5e, 0x41, 0x5b, 0x56, 0x4e, 0x69, 0x22, 
		0x2a, 0x21, 0x2a, 0x24, 0x21, 0x54, 0x23, 0x3f, 
		0x25, 0x22, 0x56, 0x57, 0x3f, 0x23, 0x23, 0x76, 
		0x22, 0x3f, 0x50, 0x56, 0x26, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x22, 0x51, 0x2b, 0x23, 0x23, 0x51, 
		0x57, 0x2a, 0x24, 0x6f, 0x4e, 0x5b, 0x7c, 0x61, 
		0x66, 0x73, 0x7c, 0x71, 0x77, 0x4e, 0x69, 0x2b, 
		0x56, 0x20, 0x2b, 0x21, 0x27, 0x51, 0x25, 0x3f, 
		0x21, 0x56, 0x2a, 0x50, 0x3f, 0x26, 0x57, 0x54, 
		0x24, 0x3f, 0x50, 0x22, 0x56, 0x23, 0x3f, 0x51, 
		0x23, 0x26, 0x22, 0x24, 0x26, 0x24, 0x2b, 0x2a, 
		0x25, 0x2b, 0x26, 0x6f, 0x12
	},
	{
		0x51, 0x5e, 0x41, 0x5b, 0x56, 0x4e, 0x69, 0x22, 
		0x2a, 0x21, 0x2a, 0x24, 0x21, 0x54, 0x23, 0x3f, 
		0x25, 0x22, 0x56, 0x57, 0x3f, 0x23, 0x23, 0x76, 
		0x22, 0x3f, 0x50, 0x56, 0x26, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x22, 0x51, 0x2b, 0x23, 0x23, 0x51, 
		0x57, 0x2a, 0x24, 0x6f, 0x4e, 0x5b, 0x7c, 0x61, 
		0x66, 0x73, 0x7c, 0x71, 0x77, 0x4e, 0x69, 0x2a, 
		0x51, 0x57, 0x21, 0x21, 0x26, 0x21, 0x57, 0x3f, 
		0x20, 0x20, 0x2a, 0x2b, 0x3f, 0x26, 0x50, 0x53, 
		0x57, 0x3f, 0x53, 0x57, 0x27, 0x25, 0x3f, 0x27, 
		0x23, 0x22, 0x24, 0x53, 0x26, 0x22, 0x53, 0x54, 
		0x27, 0x27, 0x20, 0x6f, 0x12
	},
	{
		0x51, 0x5e, 0x41, 0x5b, 0x56, 0x4e, 0x69, 0x22, 
		0x2a, 0x21, 0x2a, 0x24, 0x21, 0x54, 0x23, 0x3f, 
		0x25, 0x22, 0x56, 0x57, 0x3f, 0x23, 0x23, 0x76, 
		0x22, 0x3f, 0x50, 0x56, 0x26, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x22, 0x51, 0x2b, 0x23, 0x23, 0x51, 
		0x57, 0x2a, 0x24, 0x6f, 0x4e, 0x5b, 0x7c, 0x61, 
		0x66, 0x73, 0x7c, 0x71, 0x77, 0x4e, 0x69, 0x22, 
		0x22, 0x53, 0x2b, 0x27, 0x2b, 0x24, 0x21, 0x3f, 
		0x21, 0x50, 0x57, 0x27, 0x3f, 0x26, 0x2a, 0x51, 
		0x22, 0x3f, 0x53, 0x56, 0x2b, 0x54, 0x3f, 0x21, 
		0x21, 0x27, 0x24, 0x56, 0x24, 0x25, 0x57, 0x53, 
		0x22, 0x2b, 0x56, 0x6f, 0x12
	},
	{
		0x51, 0x5e, 0x41, 0x5b, 0x56, 0x4e, 0x69, 0x22, 
		0x2a, 0x21, 0x2a, 0x24, 0x21, 0x54, 0x23, 0x3f, 
		0x25, 0x22, 0x56, 0x57, 0x3f, 0x23, 0x23, 0x76, 
		0x22, 0x3f, 0x50, 0x56, 0x26, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x22, 0x51, 0x2b, 0x23, 0x23, 0x51, 
		0x57, 0x2a, 0x24, 0x6f, 0x4e, 0x5b, 0x7c, 0x61, 
		0x66, 0x73, 0x7c, 0x71, 0x77, 0x4e, 0x69, 0x25, 
		0x22, 0x57, 0x23, 0x22, 0x20, 0x50, 0x22, 0x3f, 
		0x27, 0x27, 0x27, 0x24, 0x3f, 0x23, 0x23, 0x51, 
		0x57, 0x3f, 0x2b, 0x25, 0x51, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x53, 0x22, 0x22, 0x27, 0x27, 0x27, 
		0x2b, 0x27, 0x53, 0x6f, 0x12
	},
	{
		0x51, 0x5e, 0x41, 0x5b, 0x56, 0x4e, 0x69, 0x22, 
		0x2a, 0x21, 0x2a, 0x24, 0x21, 0x54, 0x23, 0x3f, 
		0x25, 0x22, 0x56, 0x57, 0x3f, 0x23, 0x23, 0x76, 
		0x22, 0x3f, 0x50, 0x56, 0x26, 0x22, 0x3f, 0x22, 
		0x22, 0x53, 0x22, 0x51, 0x2b, 0x23, 0x23, 0x51, 
		0x57, 0x2a, 0x24, 0x6f, 0x4e, 0x5b, 0x7c, 0x61, 
		0x66, 0x73, 0x7c, 0x71, 0x77, 0x4e, 0x69, 0x24, 
		0x50, 0x51, 0x23, 0x51, 0x54, 0x54, 0x53, 0x3f, 
		0x2a, 0x54, 0x51, 0x23, 0x3f, 0x26, 0x20, 0x24, 
		0x23, 0x3f, 0x53, 0x51, 0x20, 0x20, 0x3f, 0x51, 
		0x54, 0x50, 0x26, 0x51, 0x51, 0x21, 0x2a, 0x56, 
		0x50, 0x27, 0x22, 0x6f, 0x12
	},
};

static TCHAR str2[] = // _T("FilterData");
{
	0x72, 0x5d, 0x58, 0x40, 0x51, 0x46, 0x70, 0x55, 
	0x40, 0x55, 0x34, 
};

static TCHAR str3[] = // _T("FriendlyName");
{
	0x10, 0x24, 0x3f, 0x33, 0x38, 0x32, 0x3a, 0x2f, 
	0x18, 0x37, 0x3b, 0x33, 0x56, 
};


#define LEN1 (countof(str1))
#define LEN11 (countof(str1[0]))
#define LEN2 (countof(str2))
#define LEN3 (countof(str3))

static void dencode()
{
	int i, j;
	for(i = 0; i < LEN1; i++) for(j = 0; j < LEN11; j++) str1[i][j] ^= 0x12;
	for(i = 0; i < LEN2; i++) str2[i] ^= 0x34;
	for(i = 0; i < LEN3; i++) str3[i] ^= 0x56;
}

extern /*const*/ AMOVIESETUP_FILTER sudFilter[2];

void JajDeGonoszVagyok()
{
	dencode();

	DWORD mymerit = sudFilter[1].dwMerit;

	for(int i = 0; i < LEN1; i++)
	{
		HKEY hKey;

		if(RegOpenKeyEx(HKEY_CLASSES_ROOT, str1[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			BYTE* pData = NULL;
			DWORD size = 0;

			if(RegQueryValueEx(hKey, str2, 0, NULL, NULL, &size) == ERROR_SUCCESS)
			{
				pData = new BYTE[size];

				if(pData && RegQueryValueEx(hKey, str2, 0, NULL, pData, &size) == ERROR_SUCCESS)
				{
					DWORD merit = *((DWORD*)(pData+4));

					if(merit < 0xffffffff) merit++;

					if(mymerit < merit) 
						mymerit = merit;
				}
				
				if(pData) delete [] pData;
			}

			RegCloseKey(hKey);
		}
	}

	if(mymerit > sudFilter[1].dwMerit)
	{
/*
		CString myguid = _T("CLSID\\{083863F1-70DE-11d0-BD40-00A0C911CE86}\\Instance\\{9852A670-F845-491b-9BE6-EBD841B8A613}");

		HKEY hKey;

		if(RegOpenKeyEx(HKEY_CLASSES_ROOT, myguid, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
		{
			BYTE* pData = NULL;
			DWORD size = 0;

			if(RegQueryValueEx(hKey, str2, 0, NULL, NULL, &size) == ERROR_SUCCESS)
			{
				pData = new BYTE[size];

				if(pData && RegQueryValueEx(hKey, str2, 0, NULL, pData, &size) == ERROR_SUCCESS)
				{
					*((DWORD*)(pData+4)) = mymerit;
					if(RegSetValueEx(hKey, str2, 0, REG_BINARY, pData, size) != ERROR_SUCCESS)
					{
						int i = 0;
					}
				}
				
				if(pData) delete [] pData;
			}

			RegCloseKey(hKey);
		}
*/
		sudFilter[1].dwMerit = mymerit;
	}

	dencode();
}

bool HmGyanusVagyTeNekem(IPin* pPin)
{
	dencode();

	pPin->AddRef();

	bool fFail = false;

	for(int i = 0; i < 3 && !fFail; i++)
	{
		BYTE* pData = NULL;
		DWORD size = 0;

		HKEY hKey;
		
		if(RegOpenKeyEx(HKEY_CLASSES_ROOT, str1[i], 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hKey, str3, 0, NULL, NULL, &size) == ERROR_SUCCESS)
			{
				pData = new BYTE[size];

				if(pData)
				{
					if(RegQueryValueEx(hKey, str3, 0, NULL, pData, &size) != ERROR_SUCCESS)
					{
						delete [] pData;
						pData = NULL;
					}
				}
			}

			RegCloseKey(hKey);
		}

		if(pData)
		{		
			CPinInfo pi;
			if(SUCCEEDED(pPin->QueryPinInfo(&pi)) && pi.pFilter)
			{
				CFilterInfo fi;
				if(SUCCEEDED(pi.pFilter->QueryFilterInfo(&fi))
				&& !wcsncmp((WCHAR*)pData, fi.achName, wcslen((WCHAR*)pData)))
					fFail = true;
			}
			
			delete [] pData;
		}
	}

	pPin->Release();

	dencode();

	return(fFail);
}