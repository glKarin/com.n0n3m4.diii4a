#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "TabContainerWindow.h"
#include "TabWindow.h"

#define DEFAULT_HEIGHT_IN_VERTICAL 48.0f

// Number of pixels above the text that the rect starts
static const int pixelOffset = 3;

// number of pixels between columns
static const int tabBorder = 4;

void hhTabContainerWindow::CommonInit(void)
{
    idWindow::CommonInit();

	activeTab = 0;
	// horizontal = false;
	vertical = false;
	tabHeight = 0;
	tabMargins = idVec2(0, 0);
	currentTab = -1;
	offsets.x = 0;
	offsets.y = 0;
    sepColor.Zero();
}

hhTabContainerWindow::hhTabContainerWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g)
{
	dc = d;
	gui = g;
	CommonInit();
}

hhTabContainerWindow::hhTabContainerWindow(idUserInterfaceLocal *g) : idWindow(g)
{
	gui = g;
	CommonInit();
}

bool hhTabContainerWindow::ParseInternalVar(const char *_name, idParser *src)
{
	if (idStr::Icmp(_name, "horizontal") == 0) {
		// horizontal = !src->ParseBool();
		vertical = src->ParseBool();
		return true;
	}
    if (idStr::Icmp(_name, "tabMargins") == 0)
    {
        tabMargins[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        tabMargins[1] = src->ParseFloat();
        return true;
    }
    if (idStr::Icmp(_name, "sepColor") == 0)
    {
        sepColor[0] = src->ParseFloat();
        src->ExpectTokenString(",");
        sepColor[1] = src->ParseFloat();
        src->ExpectTokenString(",");
        sepColor[2] = src->ParseFloat();
        src->ExpectTokenString(",");
        sepColor[3] = src->ParseFloat();
        return true;
    }
    if (idStr::Icmp(_name, "tabHeight") == 0)
    {
        tabHeight = src->ParseFloat();
        return true;
    }

	return idWindow::ParseInternalVar(_name, src);
}

idWinVar * hhTabContainerWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t **owner)
{
    if (idStr::Icmp(_name, "activeTab") == 0)
    {
        return &activeTab;
    }

    return idWindow::GetWinVarByName(_name, fixup, owner);
}

void hhTabContainerWindow::PostParse(void)
{
	idWindow::PostParse();

    tabs.Clear();
	for(int i = 0; i < children.Num(); i++)
	{
		idWindow *child = children[i];
		hhTabWindow *tab = dynamic_cast<hhTabWindow *>(child);
		if(tab)
		{
            tabs.Append(tab);
		}
		else
		{
			common->Warning("not tabDef under tabContainerDef: %d\n", i);
		}
	}

    currentTab = activeTab;

	UpdateTab();
}

void hhTabContainerWindow::Draw(int time, float x, float y)
{
	for (int i = 0; i < tabs.Num(); i++) {
		hhTabWindow *tab = tabs[i];
        tab->DrawButton(drawRect.x, drawRect.y, ButtonContains(tab), vertical);
	}
}

const char *hhTabContainerWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals)
{
    // need to call this to allow proper focus and capturing on embedded children
    const char *ret = idWindow::HandleEvent(event, updateVisuals);

    int key = event->evValue;

    if (event->evType == SE_KEY) {
        if (!event->evValue2) {
            // We only care about key down, not up
            return ret;
        }

        if (key == K_MOUSE1) {
            for(int i = 0; i < tabs.Num(); i++)
            {
                hhTabWindow *t = tabs[i];
                if(ButtonContains(t))
                {
                    SetActiveTab(i);
                    break;
                }
            }
        } else {
            return ret;
        }
    } else {
        return ret;
    }

    return ret;
}

void hhTabContainerWindow::Activate(bool activate, idStr &act)
{
	idWindow::Activate(activate, act);

	if (activate) {
		UpdateTab();
	}
}

void hhTabContainerWindow::UpdateTab(bool onlyOffset)
{
	const float lineHeight = GetTabHeight();
    const float lineWidth = GetTabWidth();

    for(int i = 0; i < tabs.Num(); i++)
    {
		hhTabWindow *tab = tabs[i];
        if(!onlyOffset)
        {
            tab->SetActive(currentTab == i);
            tab->SetOffsets(tabMargins[0], tabMargins[1]);
        }
		if (vertical) {
            tab->SetButtonRect(
                0,
                (float)i * lineHeight,
                lineWidth,
                lineHeight
            );
		} else {
            tab->SetButtonRect(
                lineWidth * (float)i + offsets.x + tabMargins[0],
                0,
                lineWidth,
                lineHeight
            );
		}
    }
}

void hhTabContainerWindow::StateChanged(bool redraw)
{
	UpdateTab();
}

void hhTabContainerWindow::SetActiveTab(int index)
{
/*    for(int i = 0; i < tabs.Num(); i++)
        tabs[i].tab->SetActive(false);*/
	if(currentTab >= 0 && currentTab < tabs.Num())
	{
		tabs[currentTab]->SetActive(false);
	}
	currentTab = -1;
	if(index >= 0 && index < tabs.Num())
	{
		currentTab = index;
		tabs[index]->SetActive(true);
	}
	activeTab.Set(va("%d", currentTab));
}

void hhTabContainerWindow::SetOffsets(float x, float y)
{
	offsets.x = x;
	offsets.y = y;

    UpdateTab(true);
}

float hhTabContainerWindow::GetTabHeight()
{
	const float lineHeight = GetMaxCharHeight() + tabBorder;
	if(vertical)
		return Max(lineHeight + pixelOffset, Min((rect.h() - tabMargins[0]) / (float)tabs.Num(), DEFAULT_HEIGHT_IN_VERTICAL/*tabHeight / 2*/));
	else
		return lineHeight; // Max(lineHeight, tabHeight / 2);
}

float hhTabContainerWindow::GetTabWidth()
{
    if(vertical)
        return Max(tabHeight, tabMargins[0]);
    else
        return (rect.w() - tabMargins[0] - offsets.x) / (float)tabs.Num();
}

bool hhTabContainerWindow::ButtonContains(const hhTabWindow *tab)
{
    idRectangle r;
    tab->GetButtonOffsetRect(r, drawRect.x, drawRect.y);
    //common->Printf("OOO %d  %f %f in %f %f %f %f -> %d\n" ,i,gui->CursorX(), gui->CursorY(), r.x, r.h, r.w, r.h, Contains(r, gui->CursorX(), gui->CursorY()));
    return Contains(r, gui->CursorX(), gui->CursorY());
}
