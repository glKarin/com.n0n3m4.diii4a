#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "SuperWindow.h"

void hhSuperWindow::CommonInit(void)
{
    idWindow::CommonInit();

    barMat.Reset();
    cornerSize = idVec2(-1.0f, -1.0f);
    edgeSize = idVec2(-1.0f, -1.0f);
	margins.Zero();
}

hhSuperWindow::hhSuperWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhSuperWindow::hhSuperWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

bool hhSuperWindow::ParseInternalVar(const char *_name, idParser *src)
{
    if (idStr::Icmp(_name, "leftMat") == 0) { // TODO
        idStr unused;
		ParseString(src, unused);
        return true;
    }
    if (idStr::Icmp(_name, "cornerSize") == 0)
    {
        cornerSize[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        cornerSize[1] = src->ParseFloat();
        return true;
    }
    if (idStr::Icmp(_name, "edgeSize") == 0)
    {
        edgeSize[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        edgeSize[1] = src->ParseFloat();
        return true;
    }
    if (idStr::Icmp(_name, "margins") == 0)
    {
        margins[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        margins[1] = src->ParseFloat();
        src->ExpectTokenString(",");
        margins[2] = src->ParseFloat();
        src->ExpectTokenString(",");
        margins[3] = src->ParseFloat();
        return true;
    }

    return idWindow::ParseInternalVar(_name, src);
}

idWinVar * hhSuperWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t **owner)
{
    if (idStr::Icmp(_name, "topMat") == 0)
    {
        return &barMat.middle.name;
    }
    if (idStr::Icmp(_name, "cornerMat") == 0)
    {
        return &barMat.left.name;
    }

    return idWindow::GetWinVarByName(_name, fixup, owner);
}

void hhSuperWindow::PostParse(void)
{
    idWindow::PostParse();

    barMat.Setup(cornerSize[0]);
}

void hhSuperWindow::Draw(int time, float x, float y)
{
    // top
	if(rect.h() > cornerSize[1] * 2.0f) // hide top if height <= 2x
	{
		idRectangle r = rect;
		r.h = cornerSize[1];
		//r.x += margins[0];
		//r.w -= margins[1] * 2.0f;
		//r.y += margins[2];
		//r.h += margins[3] * 2.0f;
		r.Offset(x, y);
		barMat.Draw(dc, r, false, matScalex, matScaley, flags, matColor);
	}

    // bottom
    idRectangle r = rect;
	r.y = r.y + r.h - cornerSize[1];
	r.h = -cornerSize[1];
	//common->Printf("xx %f, %f|%s|%s|%s\n", x,y,rect.ToVec4().ToString(), r.ToVec4().ToString(),((idVec4)margins).ToString());
	//r.x += margins[0];
	//r.w -= margins[1] * 2.0f;
	//r.y -= margins[2];
	//r.h += margins[3] * 2.0f;
	r.Offset(x, y);
    barMat.Draw(dc, r, false, matScalex, matScaley, flags, matColor);

	idWindow::Draw(time, x, y);
}

