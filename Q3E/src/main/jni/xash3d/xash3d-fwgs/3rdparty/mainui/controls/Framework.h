/*
Framework.h -- base menu fullscreen root window
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef FRAMEWORK_H
#define FRAMEWORK_H

#include "BaseWindow.h"
#include "PicButton.h"
#include "Primitive.h"

#define MAX_FRAMEWORK_PICBUTTONS 16

/*
 * WON-style menu framework
 */
class CMenuFramework : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;

	CMenuFramework( const char *name = "Unnamed Framework" );
	virtual ~CMenuFramework() override;

	void Show() override;
	void Draw() override;
	void Init() final override;
	void VidInit() final override;
	void Hide() override;
	bool IsRoot() const override { return true; }

	bool KeyDown( int key ) override;

	CMenuPicButton *AddButton( const char *szName, const char *szStatus,
		EDefaultBtns iButton, CEventCallback onReleased = CEventCallback(), int iFlags = 0 );

	CMenuPicButton *AddButton( const char *szName, const char *szStatus,
		const char *szButtonPath, CEventCallback onReleased = CEventCallback(), int iFlags = 0, int hotkey = 0 );

	void RealignButtons();

	bool DrawAnimation() override;

	void PrepareBannerAnimation( EAnimation direction, CMenuPicButton *initiator );

	class CMenuBannerBitmap : public CMenuBaseItem
	{
	public:
		CMenuBannerBitmap();
		void Draw() override;
		void SetPicture( const char *pic );

		void Draw( Point pt, Size sz );

	private:
		CImage image;
	} banner;

protected:
	EAnimation bannerAnimDirection;
	Rect bannerRects[2];

	CMenuPicButton *m_apBtns[MAX_FRAMEWORK_PICBUTTONS];
	int m_iBtnsNum;
};

#endif // FRAMEWORK_H
