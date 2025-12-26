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
		virtual void		PostParse(void);
		virtual void		Draw(int time, float x, float y);
		virtual void		Activate(bool activate, idStr &act);
		virtual void		StateChanged(bool redraw = false);

		void				UpdateTab(bool onlyOffset = false);
        void 				SetOffsets(float x, float y);
        virtual idWinVar *  GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t **owner = NULL);

	private:
		virtual bool		ParseInternalVar(const char *name, idParser *src);
		void				CommonInit(void);
        void 				SetActiveTab(int index);
		float				GetTabHeight();
        float				GetTabWidth();
        bool 				ButtonContains(const hhTabWindow *tab);

        idWinInt            activeTab; // RW; non-ref; script;
        idVec2              tabMargins; // RO; non-ref; non-script;
        idVec4              sepColor; // RO; non-ref; non-script;
	    // bool				horizontal;
        bool    			vertical; // RO; non-ref; non-script; can't auto parsing
        float			    tabHeight; // RO; non-ref; non-script;

        idList<hhTabWindow *> tabs;
        int 				currentTab;
        idVec2 				offsets;
};

#endif // _KARIN_TABCONTAINERWINDOW_H
