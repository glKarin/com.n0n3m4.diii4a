// for gui tabContainerDef
#ifndef _KARIN_TABCONTAINERWINDOW_H
#define _KARIN_TABCONTAINERWINDOW_H

class hhTabWindow;

/*
 * tabContainerDef
 *  Widget with many tabs(tabDef)
 */
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

		void				UpdateTab(bool onlyOffset = false);
        void 				SetOffsets(float x, float y);
        virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);

	private:
		virtual bool		ParseInternalVar(const char *name, idParser *src);
		void				CommonInit();
        void 				SetActiveTab(int index);
		float				GetTabHeight();
        float				GetTabWidth();
        bool 				ButtonContains(const hhTabWindow *tab);

        idWinInt            activeTab;
        idWinVec2           tabMargins;
        idWinVec4           sepColor;

		idList<hhTabWindow *> tabs;
	    // bool				horizontal;
		bool				vertical;
        float 				tabHeight;
        int 				currentTab;
        idVec2 				offsets;
};

#endif // _KARIN_TABCONTAINERWINDOW_H
