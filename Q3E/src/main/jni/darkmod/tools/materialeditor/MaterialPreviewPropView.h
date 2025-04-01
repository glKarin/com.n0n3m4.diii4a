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
#pragma once

#include "../common/PropTree/PropTreeView.h"
#include "MaterialPreviewView.h"

// MaterialPreviewPropView view

class MaterialPreviewPropView : public CPropTreeView
{
	DECLARE_DYNCREATE(MaterialPreviewPropView)

protected:
	MaterialPreviewPropView();           // protected constructor used by dynamic creation
	virtual ~MaterialPreviewPropView();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view

	afx_msg void OnPropertyChangeNotification( NMHDR *nmhdr, LRESULT *lresult );
	afx_msg void OnPropertyButtonClick( NMHDR *nmhdr, LRESULT *lresult );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void AddLight( void );
	void InitializePropTree( void );

	void RegisterPreviewView( MaterialPreviewView *view );

protected:

	int		numLights;

	MaterialPreviewView	*materialPreview;

	DECLARE_MESSAGE_MAP()
};


