// for gui buttonDef
#ifndef _KARIN_BUTTONWINDOW_H
#define _KARIN_BUTTONWINDOW_H

#include "BackgroundImage.h"

/*
 * buttonDef
 *  Widget with left-middle-right background image
 */
class hhButtonWindow : public idWindow
{
public:
    hhButtonWindow(idUserInterfaceLocal *gui);
    hhButtonWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

    virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);

private:
    virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit();

    hhBackgroundGroup   buttonMat;
    float               edgeWidth;
};

#endif // _KARIN_BUTTONWINDOW_H
