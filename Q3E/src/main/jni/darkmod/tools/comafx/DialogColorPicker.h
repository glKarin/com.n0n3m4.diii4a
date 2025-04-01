/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __DIALOGCOLORPICKER__
#define __DIALOGCOLORPICKER__

// Original ColorPicker/DIB source by Rajiv Ramachandran <rrajivram@hotmail.com>
// included with permission from the author

#include "CDib.h"

#define RADIUS		100
#define PI			3.14159265358

#define RECT_WIDTH	5

#define TOSCALE(x)	(((x)*RADIUS)/255.0)
#define SCALETOMAX(x) (((x)*255.0)/RADIUS)


#define RED	0
#define GREEN 1
#define BLUE 2

#define BAD_SLOPE	1000000.0


struct HSVType;

struct RGBType {
	COLORREF		color() { return RGB( r, g, b ); }
	HSVType			toHSV();
	int				r, g, b;
};

struct HSVType {
	RGBType			toRGB();
	int				h, s, v;
};

struct LineDesc {
	double			x, y;
	double			slope;
	double			c;
};


class CDialogColorPicker : public CDialog
{
// Construction
public:
					CDialogColorPicker(COLORREF c,CWnd* pParent = NULL);   // standard constructor
					~CDialogColorPicker();

	COLORREF		GetColor() { return color.color();};
	float			GetOverBright() { return overBright; };


	// Dialog Data
	//{{AFX_DATA(CDialogColorPicker)
	enum { IDD = IDD_DIALOG_COLORS };
	float			m_overBright;
	//}}AFX_DATA

	void			(*UpdateParent)( float r, float g, float b, float a );

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialogColorPicker)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialogColorPicker)
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void	OnSysColorChange();
	afx_msg void	OnPaint();
	virtual BOOL	OnInitDialog();
	afx_msg void	OnChangeEditBlue();
	afx_msg void	OnChangeEditGreen();
	afx_msg void	OnChangeEditHue();
	afx_msg void	OnChangeEditRed();
	afx_msg void	OnChangeEditSat();
	afx_msg void	OnChangeEditVal();
	afx_msg void	OnChangeEditOverbright();
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnBtnColor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void			DrawFilledColor(CDC *pDC,CRect cr,COLORREF c);
	void			DrawLines(CDC *pDC);
	void			DrawXorRect(CDC *pDC,CRect& cr);
	void			CalcSlopes();
	void			CalcCuboid();

	void			CreateBrightDIB();
	void			SetDIBPalette();
	void			DrawMarkers(CDC *pDC);
	void			TrackPoint(CPoint pt);
	void			CalcRects();
		
	BOOL			InCircle(CPoint pt);
	BOOL			InBright(CPoint pt);
	BOOL			InOverBright(CPoint pt);


	void			SetSpinVals();
	void			SetEditVals();
	void			DrawAll();

	void			DrawRGB(CDC *pDC);
	void			DrawHSB(CDC *pDC);

	void			LoadMappedBitmap(CBitmap& bitmap,UINT nIdResource,CSize& size);

	CBitmap			m_RgbBitmap,m_HsbBitmap;

	CDC				memDC;
	CPoint			m_Centre;
	CDIB			m_BrightDIB;

	int				rgbWidth;
	int				rgbHeight;
	int				hsbWidth;
	int				hsbHeight;

	int				m_nMouseIn;
	CRect			m_CurrentRect,brightMark;
	CRect			brightRect;
	CRect			overBrightRect;

	HSVType			hsvColor;	

	RGBType			color;
	RGBType			m_OldColor;
	CPoint			Vertex;
	CPoint			Top;
	CPoint			Left;
	CPoint			Right;
	CRect			rects[3];
	CPoint			m_Cuboid[8];
	BOOL			m_bInMouse;
	int				nIndex;
	int				RedLen;
	int				GreenLen;
	int				BlueLen;
	LineDesc		lines[3];


	CRect			rgbRect;
	CRect			hsbRect;
	CRect			OldColorRect;
	CRect			NewColorRect;

	BOOL			m_bInitOver;
	BOOL			m_bInDrawAll;

	float			overBright;
};

bool DoNewColor( int* i1, int* i2, int* i3, float *overBright, void (*Update)( float, float, float, float ) = NULL );

#endif /* !__DIALOGCOLORPICKER__ */
