#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "TabWindow.h"
#include "TabContainerWindow.h"

void hhTabWindow::CommonInit()
{
    idWindow::CommonInit();

	active = false;
    buttonRect.Empty();
    activeColor = idVec4(1, 1, 1, 1);

    buttonMat.Reset();
    buttonActiveMat.Reset();
}

hhTabWindow::hhTabWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhTabWindow::hhTabWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

const char *hhTabWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals)
{
	// need to call this to allow proper focus and capturing on embedded children
	const char *ret = idWindow::HandleEvent(event, updateVisuals);

	return ret;
}


bool hhTabWindow::ParseInternalVar(const char *_name, idParser *src)
{
	return idWindow::ParseInternalVar(_name, src);
}

void hhTabWindow::PostParse()
{
	idWindow::PostParse();

    buttonMat.Setup();
    buttonActiveMat.Setup();
}

void hhTabWindow::Draw(int time, float x, float y)
{
	if(!active)
		return;
	//idWindow::Draw(time, x, y);
}

void hhTabWindow::Activate(bool activate, idStr &act)
{
	idWindow::Activate(activate, act);

	if (activate) {
		UpdateTab();
	}
}

void hhTabWindow::UpdateTab()
{
	visible = active;
	SetVisible(active);
}

void hhTabWindow::StateChanged(bool redraw)
{
	UpdateTab();
}

void hhTabWindow::SetActive(bool b)
{
	active = b;
    if(active)
        RunScript(ON_TABACTIVATE);
	SetVisible(b);
}

void hhTabWindow::SetOffsets(float x, float y)
{
    for(int i = 0; i < children.Num(); i++)
    {
        hhTabContainerWindow *tabContainer = dynamic_cast<hhTabContainerWindow *>(children[i]);
        if(tabContainer)
            tabContainer->SetOffsets(x, y);
    }
}

void hhTabWindow::SetVisible(bool on)
{
	idWindow::SetVisible(active && on);
}

void hhTabWindow::SetButtonRect(float x, float y, float w, float h)
{
    buttonRect.x = x;
    buttonRect.y = y;
    buttonRect.w = w;
    buttonRect.h = h;
}

void hhTabWindow::GetButtonOffsetRect(idRectangle &rect, float x, float y) const
{
    rect = buttonRect;
    rect.Offset(x, y);
}

void hhTabWindow::DrawButton(float x, float y, bool hover, bool vertical)
{
    idVec4 color;
    idStr work;
    idRectangle rect;
    const float lineHeight = GetMaxCharHeight();

    GetButtonOffsetRect(rect, x, y);

    if(active)
    {
        color = activeColor;
    }
    else
    {
        if (hover) {
            color = hoverColor;
        } else {
            color = foreColor;
        }
    }

    if(active)
    {
        buttonActiveMat.Draw(dc, rect, vertical, matScalex, matScaley, flags);
    }
    else
    {
        buttonMat.Draw(dc, rect, vertical, matScalex, matScaley, flags);
    }

    rect.y += buttonRect.h / 2 - lineHeight / 2;
    rect.h = lineHeight + buttonRect.h / 2 - lineHeight / 2;

    //dc->DrawRect(textRect.x + item.x, textRect.y + item.y, item.w, item.h, 1, color);
    dc->DrawText(text, textScale, /*vertical ? idDeviceContext::ALIGN_LEFT : */idDeviceContext::ALIGN_CENTER, color, rect, false, -1);

    /*dc->PushClipRect(rect);
    dc->PopClipRect();*/
}

idWinVar * hhTabWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t **owner)
{
    if (idStr::Icmp(_name, "activeColor") == 0)
    {
        return &activeColor;
    }
    if (idStr::Icmp(_name, "buttonLeftMat") == 0)
    {
        return &buttonMat.left.name;
    }
    if (idStr::Icmp(_name, "buttonMiddleMat") == 0)
    {
        return &buttonMat.middle.name;
    }
    if (idStr::Icmp(_name, "buttonRightMat") == 0)
    {
        return &buttonMat.right.name;
    }
    if (idStr::Icmp(_name, "buttonActiveLeftMat") == 0)
    {
        return &buttonActiveMat.left.name;
    }
    if (idStr::Icmp(_name, "buttonActiveMiddleMat") == 0)
    {
        return &buttonActiveMat.middle.name;
    }
    if (idStr::Icmp(_name, "buttonActiveRightMat") == 0)
    {
        return &buttonActiveMat.right.name;
    }

    return idWindow::GetWinVarByName(_name, fixup, owner);
}
