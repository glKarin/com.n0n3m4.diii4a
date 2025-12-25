#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "ButtonWindow.h"

void hhButtonWindow::CommonInit()
{
    idWindow::CommonInit();

    buttonMat.Reset();
    edgeWidth = -1.0f;
}

hhButtonWindow::hhButtonWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhButtonWindow::hhButtonWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

void hhButtonWindow::PostParse()
{
	idWindow::PostParse();

    buttonMat.Setup(edgeWidth);
}


bool hhButtonWindow::ParseInternalVar(const char *_name, idParser *src)
{
    if (idStr::Icmp(_name, "edgeWidth") == 0) {
        edgeWidth = src->ParseFloat();
        return true;
    }

    return idWindow::ParseInternalVar(_name, src);
}

void hhButtonWindow::Draw(int time, float x, float y)
{
    idRectangle r = rect;
    r.Offset(x, y);
    buttonMat.Draw(dc, r, false, matScalex, matScaley, flags);
	idWindow::Draw(time, x, y);
}

idWinVar * hhButtonWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t **owner)
{
    if (idStr::Icmp(_name, "leftMat") == 0)
    {
        return &buttonMat.left.name;
    }
    if (idStr::Icmp(_name, "middleMat") == 0)
    {
        return &buttonMat.middle.name;
    }
    if (idStr::Icmp(_name, "rightMat") == 0)
    {
        return &buttonMat.right.name;
    }

    return idWindow::GetWinVarByName(_name, fixup, owner);
}
