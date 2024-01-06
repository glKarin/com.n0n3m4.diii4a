// for gui tabDef
#ifndef _KARIN_TABWINDOW_H
#define _KARIN_TABWINDOW_H

class hhTabContainerWindow;

class hhTabWindow : public idWindow
{
public:
	hhTabWindow(idUserInterfaceLocal *gui);
	hhTabWindow(idDeviceContext *d, idUserInterfaceLocal *gui);

	virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual void		Activate(bool activate, idStr &act);
	virtual void		StateChanged(bool redraw = false);

	void				UpdateTab();
	void 				SetActive(bool active);
    void 				SetOffsets(float x, float y);

protected:
    virtual void 		SetVisible(bool visible);

private:
	virtual bool		ParseInternalVar(const char *name, idParser *src);
	void				CommonInit();

	bool active;

    friend class hhTabContainerWindow;
};

#endif // _KARIN_TABWINDOW_H
