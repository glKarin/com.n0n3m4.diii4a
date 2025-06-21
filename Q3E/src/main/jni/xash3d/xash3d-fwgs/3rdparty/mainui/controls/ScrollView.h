#ifndef SCROLLVIEW_H
#define SCROLLVIEW_H

#include "ItemsHolder.h"

class CMenuScrollView : public CMenuItemsHolder
{
	typedef CMenuItemsHolder BaseClass;
public:
	CMenuScrollView();

	void VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	bool MouseMove( int x, int y ) override;

	Point GetPositionOffset() const override;

private:
	bool IsRectVisible( Point pt, Size sz );

	Point m_scScrollBarPos;
	Size  m_scScrollBarSize;
	bool  m_bScrollBarSliding;
	bool  m_bDisableScrolling; // can't actually scroll due to item placement
	bool  m_bHoldingMouse1;
	Point m_HoldingPoint;

	int m_iPos;
	int m_iMax;
	// float m_flOverScrolling;
};

#endif // SCROLLVIEW_H
