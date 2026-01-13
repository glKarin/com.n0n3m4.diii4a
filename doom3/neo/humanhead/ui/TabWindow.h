// for gui tabDef
#ifndef _KARIN_TABWINDOW_H
#define _KARIN_TABWINDOW_H

class hhTabContainerWindow;

#include "BackgroundImage.h"

/*
 * tabDef
 *  Widget with a pair of left-middle-right background image tab button and tab content widget
 */
class hhTabWindow : public idWindow
{
public:
	hhTabWindow(idUserInterfaceLocal *gui);
	hhTabWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

	virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse(void);
	virtual void		Draw(int time, float x, float y);
	virtual void		Activate(bool activate, idStr &act);
	virtual void		StateChanged(bool redraw = false);

	void				UpdateTab(void);
	void 				SetActive(bool active);
    void 				SetOffsets(float x, float y);
    void 				DrawButton(float x, float y, bool hover, bool vertical);
    void 				SetButtonRect(float x, float y, float w, float h);
    void 				GetButtonOffsetRect(idRectangle &rect, float x, float y) const;
    virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);

protected:
    virtual void 		SetVisible(bool visible);

private:
	virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit(void);


    hhBackgroundGroup   buttonMat; // buttonLeftMat buttonMiddleMat buttonRightMat // RO; non-ref; non-script;
    hhBackgroundGroup   buttonActiveMat; // buttonActiveLeftMat buttonActiveMiddleMat buttonActiveRightMat // RO; non-ref; non-script;
    idVec4	            activeColor; // RO; non-ref; non-script;
    float			    buttonEdgeWidth; // RO; non-ref; non-script;
    idVec4              seperatorLines; // RO; non-ref; non-script;
    float               seperatorMargin; // RO; non-ref; non-script;

    bool                active;
    idRectangle         buttonRect;

    friend class hhTabContainerWindow;
};

#endif // _KARIN_TABWINDOW_H
