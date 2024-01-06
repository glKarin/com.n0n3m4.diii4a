#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "TabWindow.h"
#include "TabContainerWindow.h"

void hhTabWindow::CommonInit()
{
	active = false;
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