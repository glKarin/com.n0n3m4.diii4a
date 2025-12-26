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
	virtual void		PostParse(void);
	virtual void		Draw(int time, float x, float y);

private:
    virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit(void);

    hhBackgroundGroup   buttonMat; // leftMat middleMat rightMat // RO; non-ref; non-script;
    float               edgeWidth; // RO; non-ref; non-script;
    idVec4              hoverBorderColor; // RO; non-ref; non-script;
};

#endif // _KARIN_BUTTONWINDOW_H
