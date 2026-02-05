
#ifndef __FEEDALERTWINDOW_H
#define __FEEDALERTWINDOW_H

#include "ui/Window.h"

class idUserInterfaceLocal;

// blendo eric: special formatted window to queue and display multiple alerts
class idFeedAlertWindow : public idWindow
{
public:
	idFeedAlertWindow(idUserInterfaceLocal *gui) : idWindow(gui){ CommonInit(); }
	idFeedAlertWindow(idDeviceContext *d, idUserInterfaceLocal *gui) : idWindow(d, gui){ CommonInit(); }
	virtual ~idFeedAlertWindow(){ disposedState = 0; }
	virtual bool ParseInternalVar(const char* name, idParser* src);
	virtual void PostParse();
	virtual void Redraw(float x, float y){ Update(); idWindow::Redraw(x, y); }
	void Update();

	// queue alert to be displayed, default values will use alert template extracted defaults
	void DisplayAlert(const char * displayText, const char * alertIcon = nullptr,
						idVec4 * textColor = nullptr, idVec4 * bgColor = nullptr, idVec4 * iconColor = nullptr,
						float durationSeconds = 0.0f, bool allowDupes = false);

	bool WasDisposed() { return disposedState != NotDisposedMagicNumber; } // double checking for bad data bug

	virtual void WriteToSaveGame(idSaveGame* savefile) const override;
	virtual void ReadFromSaveGame(idRestoreGame* savefile) override;
private:
	typedef struct alert_s
	{
		int		debugID;
		bool	transitioned; // true after fade in
		bool	ended; // true when reset, and no longer valid
		idStr	icon; // asset str
		idStr	displayText;
		idVec4	textColor;
		idVec4	bgColor;
		idVec4	iconColor;
		int		startTime; // ms time of alert creation (start of transition/fade in)
		int		fadeInTime; // ms
		int		fadeOutTime; // ms
		int		endTime; // ms time when the alert should no longer exist
		int		displayTime; // ms unaltered total time to display
		float	fade;
		float	heightScale; // alters the default rect, currently for double lines
		idWindow*	window;	// draw window assigned when window begins transition
	} alert_t;

	// void WriteSaveGameAlert(const alert_t& alert, idSaveGame* savefile) const;
	// void ReadSaveGameAlert(alert_t& alert, idRestoreGame* savefile);

	void CommonInit();
	void SetupWindow(alert_t& alertItem, idWindow* alertWindow);

	// alert list queue funcs
	alert_t&		GetAlert(int slot); // slot 0 starting from most recent to oldest
	alert_t&		ActivateNewAlert(); // adds new active alert to queue
	void			DeactivateAlert(alert_t* alertItem); // adds new active item to queue
	void			ResetAlertState(alert_t& item);

	void			UpdateAlertQueues();
	alert_t *		GetActiveAlert(int slot); // get active alert, from oldest to newest, null if does not exist
	alert_t *		GetOldestAlert(); // get active alert, from oldest to newest, null if does not exist

	bool			NearCapacity(){ return alertQueueSize == activeAlerts.Num(); }

	// alert list data structs
	int alertQueueIterator; // blendo eric: internal index to the newest item in queue
	idList<alert_t>	 alertQueue; // queue items, both displayed and waiting
	idList<alert_t*> activeAlerts; // alerts currently alive

	idList<idWindow*> alertWindows; // display windows for queue items, size = displayCount + 1
	idList<idWindow*> alertWindowsAvailable; // display windows available for waiting alerts

	// specific feed window parsed vars
	int		alertQueueSize; // maximum alerts allowed in queue, both displayed and waiting
	bool	pushUpwards; // untested for downwards
	bool	alwaysTransition; // transition new alerts even when no other alerts are present
	int		textBGMargin; // amount bg width should extend past text
	int		displayTime; // total time to display window
	int		fadeOutTime; // fade time at end
	int		fadeInTime; // transition time into starting position
	int		turnOverTime; // when at display capacity, how long oldest window waits before expiring
	float	popLerpDistance; // distance to pop in poplerp
	float	fadeEdgeY; // edge pos where alerts will always fade when crossing
	idRectangle fadeInRect; // the fade in transition rect for new alerts

	// extracted vars from gui window
	idRectangle startAlertRect; // the start rect for after transitioned alerts
	idRectangle startTextRect; // the start text rect for after transitioned alerts
	idRectangle startBGRect; // the start bg rect for after transitioned alerts
	idRectangle startIconRect; // the start bg rect for after transitioned alerts
	idVec4 defaultBGColor;
	idVec4 defaultTextColor;
	idVec4 defaultIconColor;
	idStr  defaultIcon;
	float defaultTextScale;
	float defaultTextSpacing;

	float smoothSpeedUpScale;

	const int NotDisposedMagicNumber = 0xABBAABBA;
	int disposedState;
	int debugAlertTotalCount;
};

#endif // __FEEDALERTWINDOW_H
