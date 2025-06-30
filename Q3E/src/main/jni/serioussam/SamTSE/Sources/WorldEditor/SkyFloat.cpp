/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// SkyFloat.cpp : implementation of DDX_SkyFloat

/////////////////////////////////////////////////////////////////////////////
// Public functions

#include "StdAfx.h"

//--------------------------------------------------------------------------------------------
void AFXAPI DDX_SkyFloat(CDataExchange* pDX, int nIDC, float &fNumber)
{
  BOOL bTrue = TRUE;
  DDX_SkyFloat( pDX, nIDC, fNumber, bTrue);
}

void AFXAPI DDX_SkyFloat(CDataExchange* pDX, int nIDC, float &fNumber, BOOL &bValid)
{
	HWND hWndCtrl = pDX->PrepareEditCtrl(nIDC);
	if (pDX->m_bSaveAndValidate)
	{
    if (!FloatFromString(hWndCtrl, fNumber, bValid))
		{
			AfxMessageBox(L"Invalid character entered");
			pDX->Fail();
		}
	}
	else
	{
		StringFromFloat(hWndCtrl, fNumber, bValid);
	}
}

BOOL FloatFromString(HWND hWnd, float &fNumber, BOOL &bValid)
{
	TCHAR szWindowText[20];
	::GetWindowText(hWnd, szWindowText, 19);
  if( CTString( CStringA(szWindowText)) == "")
  {
    bValid = FALSE;
    return TRUE;
  }
  
  bValid = TRUE;  
  float fTmpNumber = fNumber;
  size_t iNumLen, iRetLen;
  iNumLen = strlen( CStringA(szWindowText));
  iRetLen = sscanf( CStringA(szWindowText), "%f", &fTmpNumber);
  if( (iRetLen == 1)  || ((iNumLen == 1) && (szWindowText[0] == '-') || (iNumLen == 0)) )
  {
    fNumber = fTmpNumber;
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

void StringFromFloat(HWND hWnd, float fNumber, BOOL &bValid)
{
	if( !bValid) 
  {
    ::SetWindowText(hWnd, L"");
    return;
  }
  CString str;
	str.Format(_T("%g"), fNumber);
	::SetWindowText(hWnd, str.GetBufferSetLength(20));
}
//--------------------------------------------------------------------------------------------
