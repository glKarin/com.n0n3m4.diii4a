#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "ButtonWindow.h"

void hhButtonWindow::CommonInit(void)
{
    idWindow::CommonInit();

    buttonMat.Reset();
    edgeWidth = -1.0f;
    hoverBorderColor = idVec4(1.0f, 1.0f, 1.0f, 1.0f);
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

bool hhButtonWindow::ParseInternalVar(const char *_name, idParser *src)
{
    if (idStr::Icmp(_name, "edgeWidth") == 0)
    {
        edgeWidth = src->ParseFloat();
        return true;
    }
    if (idStr::Icmp(_name, "hoverBorderColor") == 0)
    {
        hoverBorderColor[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[1] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[2] = src->ParseFloat();
        src->ExpectTokenString(",");
        hoverBorderColor[3] = src->ParseFloat();
        return true;
    }
    return idWindow::ParseInternalVar(_name, src);
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

void hhButtonWindow::PostParse(void)
{
    idWindow::PostParse();

    buttonMat.Setup(edgeWidth);
}

void hhButtonWindow::Draw(int time, float x, float y)
{
    idRectangle r = rect;
    r.Offset(x, y);
    buttonMat.Draw(dc, r, false, matScalex, matScaley, flags, matColor);
	idWindow::Draw(time, x, y);
}
