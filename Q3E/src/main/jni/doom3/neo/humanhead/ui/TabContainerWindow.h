// for gui tabContainerDef
#ifndef _KARIN_TABCONTAINERWINDOW_H
#define _KARIN_TABCONTAINERWINDOW_H

class hhTabWindow;

struct hhTabRect {
    float x;
    float w;
    float y;
    float h;
    hhTabWindow *tab;
};

class hhTabContainerWindow : public idWindow
{
	public:
		hhTabContainerWindow(idUserInterfaceLocal *gui);
		hhTabContainerWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

		virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
		virtual void		PostParse();
		virtual void		Draw(int time, float x, float y);
		virtual void		Activate(bool activate, idStr &act);
		virtual void		StateChanged(bool redraw = false);

		void				UpdateTab();
        void 				SetOffsets(float x, float y);

	private:
		virtual bool		ParseInternalVar(const char *name, idParser *src);
		void				CommonInit();
        void 				SetActiveTab(int index);
		float				GetTabHeight();
        float				GetTabWidth();

		idList<hhTabRect> 	tabs;
	    // bool				horizontal;
		bool				vertical;
        float 				tabHeight;
        int 				currentTab;
        idVec2 				offsets;
};

#endif // _KARIN_TABCONTAINERWINDOW_H
