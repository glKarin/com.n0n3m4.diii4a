// for gui buttonDef
#ifndef _KARIN_SUPERWINDOW_H
#define _KARIN_SUPERWINDOW_H

#include "BackgroundImage.h"

/*
 * superWindowDef
 *  Widget with top-middle-bottom background image
 */
class hhSuperWindow : public idWindow
{
public:
    hhSuperWindow(idUserInterfaceLocal *gui);
    hhSuperWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

    virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);
	virtual void		PostParse(void);
	virtual void		Draw(int time, float x, float y);

private:
    virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit(void);

    hhBackgroundGroup   barMat; // leftMat topMat cornerMat // RO; non-ref; non-script;
    idVec2              cornerSize; // RO; non-ref; non-script;
    idVec2              edgeSize; // RO; non-ref; non-script;
    idVec4              margins; // left, right, top, bottom // RO; non-ref; non-script;
};

#endif // _KARIN_SUPERWINDOW_H

