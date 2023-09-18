#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/DeviceContext.h"
#include "../../ui/Window.h"
#include "../../ui/UserInterfaceLocal.h"
#include "TabContainerWindow.h"
#include "TabWindow.h"

// Number of pixels above the text that the rect starts
static const int pixelOffset = 3;

// number of pixels between columns
static const int tabBorder = 4;

void hhTabContainerWindow::CommonInit()
{
	activeTab = 0;
	horizontal = false;
	tabHeight = 0;
	tabMargins = idVec2(40, 0);
	currentTab = -1;
	offsets.x = 0;
	offsets.y = 0;
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
				const hhTabRect &t = tabs[i];
				idRectangle r(textRect.x + offsets.x + t.x, textRect.y + offsets.y + t.y, t.w, t.h);
				//common->Printf("OOO %d  %f %f in %f %f %f %f -> %d\n" ,i,gui->CursorX(), gui->CursorY(), r.x, r.h, r.w, r.h, Contains(r, gui->CursorX(), gui->CursorY()));
				if(Contains(r, gui->CursorX(), gui->CursorY()))
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


bool hhTabContainerWindow::ParseInternalVar(const char *_name, idParser *src)
{
	if (idStr::Icmp(_name, "horizontal") == 0) {
		horizontal = src->ParseBool();
		return true;
	}
	if (idStr::Icmp(_name, "tabHeight") == 0) {
		tabHeight = src->ParseFloat();
		return true;
	}

	return idWindow::ParseInternalVar(_name, src);
}

void hhTabContainerWindow::PostParse()
{
	idWindow::PostParse();

    tabs.Clear();
	int index = 0;
	idList<hhTabWindow *> list;
	for(int i = 0; i < children.Num(); i++)
	{
		idWindow *child = children[i];
		hhTabWindow *tab = dynamic_cast<hhTabWindow *>(child);
		if(tab)
		{
			list.Append(tab);
		}
		else
		{
		}
	}

	float lineHeight = GetMaxCharHeight();
	float width = rect.w() - tabMargins.x();
	for(int i = 0; i < list.Num(); i++)
	{
        hhTabWindow *tab = list[i];
		hhTabRect r;
		if (horizontal) {
			r.x = width * i / (float)list.Num();
			r.y = 0;
			r.w = width / (float)list.Num();
			r.h = lineHeight;
		} else {
			r.x = 0;
			r.y = i * lineHeight + pixelOffset;
			r.w = tabMargins.x();
			r.h = lineHeight;
		}
		r.tab = tab;
		tabs.Append(r);
	}

	for(int i = 0; i < tabs.Num(); i++)
	{
		tabs[i].tab->SetOffsets(tabMargins.x(), tabMargins.y());
	}

	currentTab = activeTab;
    for(int i = 0; i < tabs.Num(); i++)
    {
		tabs[i].tab->SetActive(i == activeTab);
    }
}

void hhTabContainerWindow::Draw(int time, float x, float y)
{
	idVec4 color;
	idStr work;
	int count = tabs.Num();
	idRectangle rect;

	for (int i = 0; i < count; i++) {
		const hhTabRect &item = tabs[i];
		const hhTabWindow *tab = item.tab;

        rect.x = textRect.x + offsets.x + item.x;
        rect.y = textRect.y + offsets.y + item.y;
        rect.w = item.w;
        rect.h = item.h;

		if(i == currentTab)
		{
			color = tab->activeColor;
		}
		else
		{
			if (Contains(rect, gui->CursorX(), gui->CursorY())) {
				color = tab->hoverColor;
			} else {
				color = tab->foreColor;
			}
		}

		dc->DrawText(tab->text, tab->textScale, horizontal ? idDeviceContext::ALIGN_CENTER : idDeviceContext::ALIGN_LEFT, color, rect, false, -1);

		/*dc->PushClipRect(rect);
		dc->PopClipRect();*/
	}
}

void hhTabContainerWindow::Activate(bool activate, idStr &act)
{
	idWindow::Activate(activate, act);

	if (activate) {
		UpdateTab();
	}
}

void hhTabContainerWindow::UpdateTab()
{
	float lineHeight = GetMaxCharHeight();

    for(int i = 0; i < tabs.Num(); i++)
    {
		hhTabRect &r = tabs[i];
		r.tab->SetActive(currentTab == i);
        r.tab->SetOffsets(tabMargins.x(), tabMargins.y());
		if (horizontal) {
			float width = rect.w() - tabMargins.x() - offsets.x;
			r.x = width * i / (float)tabs.Num();
			r.y = 0;
			r.w = width / (float)tabs.Num();
			r.h = lineHeight;
		} else {
			r.x = 0;
			r.y = i * lineHeight + pixelOffset;
			r.w = tabMargins.x();
			r.h = lineHeight;
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
		tabs[currentTab].tab->SetActive(false);
	}
	currentTab = -1;
	if(index >= 0 && index < tabs.Num())
	{
		currentTab = index;
		tabs[index].tab->SetActive(true);
	}
	activeTab.Set(va("%d", currentTab));
}

void hhTabContainerWindow::SetOffsets(float x, float y)
{
	float lineHeight = GetMaxCharHeight();
	offsets.x = x;
	offsets.y = y;

	for(int i = 0; i < tabs.Num(); i++)
	{
		hhTabRect &r = tabs[i];
		if (horizontal) {
			float width = rect.w() - tabMargins.x() - offsets.x;
			r.x = width * i / (float)tabs.Num();
			r.y = 0;
			r.w = width / (float)tabs.Num();
			r.h = lineHeight;
		} else {
			r.x = 0;
			r.y = i * lineHeight + pixelOffset;
			r.w = tabMargins.x();
			r.h = lineHeight;
		}
	}
}