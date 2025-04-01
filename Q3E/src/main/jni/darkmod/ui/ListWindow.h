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
#ifndef __LISTWINDOW_H
#define __LISTWINDOW_H

class idSliderWindow;

enum {
	TAB_TYPE_TEXT = 0,
	TAB_TYPE_ICON = 1
};

struct idTabRect {
	int x;
	int w;
	int align;
	int valign;
	int	type;
	idVec2 iconSize;
	float iconVOffset;
};

class idListWindow : public idWindow {
public:
	idListWindow(idUserInterfaceLocal *gui);
	idListWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals) override;
	virtual void		PostParse() override;
	virtual void		Draw(int time, float x, float y) override;
	virtual void		Activate(bool activate, idStr &act) override;
	virtual void		HandleBuddyUpdate(idWindow *buddy) override;
	virtual void		StateChanged( bool redraw = false ) override;
	virtual size_t		Allocated() override { return idWindow::Allocated(); }
	virtual idWinVar*	GetThisWinVarByName(const char *varname) override;

	void				UpdateList();
	
private:
	virtual bool		ParseInternalVar(const char *name, idParser *src) override;
	void				CommonInit();
	void				InitScroller( bool horizontal );
	void				SetCurrentSel( int sel );
	void				AddCurrentSel( int sel );
	int					GetCurrentSel();
	bool				IsSelected( int index );
	void				ClearSelection( int sel );

	idList<idTabRect>	tabInfo;
	int					top;
	bool				horizontal;
	idStr				tabStopStr;
	idStr				tabAlignStr;
	idStr				tabVAlignStr;
	idStr				tabTypeStr;
	idStr				tabIconSizeStr;
	idStr				tabIconVOffsetStr;
	idHashTable<const idMaterial*> iconMaterials;						
	bool				multipleSel;

	idStrList			listItems;
	idSliderWindow*		scroller;
	idList<int>			currentSel;
	idStr				listName;

	int					clickTime;

	int					typedTime;
	idStr				typed;
};

#endif // __LISTWINDOW_H
