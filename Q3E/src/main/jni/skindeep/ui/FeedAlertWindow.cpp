
#include "ui/FeedAlertWindow.h"

#include "sys/platform.h"
#include "idlib/LangDict.h"
#include "framework/KeyInput.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"
#include "Game_local.h"

const char * AlertWindowTemplateName = "AlertTemplate";
const char * AlertBGTemplateName = "AlertBG";
const char * AlertIconTemplateName = "AlertIcon";
const char * AlertTextTemplateName = "AlertText";

void RescaleTime(float newDisplayTime, float& transition, float& fade)
{
	// fit the transition and fade into the display time
	if( newDisplayTime < (transition + fade) )
	{
		float rescale = newDisplayTime/ (transition + fade);
		transition *= rescale;
		fade *= rescale;
	}
}

template<typename T> T GetChildWinVar(drawWin_t* drawChild, const char * varName )
{
	if( idWindow * subChild = drawChild->win )
	{
		return static_cast<T>(subChild->GetWinVarByName(varName));
	}
	else if( idSimpleWindow * subChild = drawChild->simp )
	{
		return static_cast<T>(subChild->GetWinVarByName(varName));
	}
	return nullptr;
}

template<typename T> T GetChildWinVar(idWindow* window, const char * childName, const char * varName )
{
	if( drawWin_t * drawChild = window->FindChildByName(childName) )
	{
		return GetChildWinVar<T>(drawChild,varName);
	}
	return nullptr;
}


void idFeedAlertWindow::CommonInit()
{
	alertQueueIterator = 0;

	alertQueueSize = 0;
	pushUpwards = true;
	alwaysTransition = true;
	textBGMargin = 0;

	displayTime = 15000;
	fadeOutTime = 500; // avoid div by 0
	fadeInTime = 500; // avoid div by 0
	turnOverTime = 1000;

	popLerpDistance = 1;
	fadeEdgeY = 0.0f;

	fadeInRect = idRectangle(100,0,100,100);


	startAlertRect = idRectangle(0,0,100,100);
	startTextRect = idRectangle(0,0,100,100);
	startBGRect = idRectangle(0,0,100,100);
	startIconRect = idRectangle(0,0,100,100);
	defaultBGColor = idVec4(0,0,0,1);
	defaultTextColor = idVec4(1,1,1,1);
	defaultIconColor = idVec4(1,1,1,1);
	defaultIcon = "";
	defaultTextScale = 1.0f;
	defaultTextSpacing = 0.0f;

	smoothSpeedUpScale = 0.0f;

	debugAlertTotalCount = 0;

	disposedState = NotDisposedMagicNumber;
}

bool idFeedAlertWindow::ParseInternalVar(const char* name, idParser* src)
{
	if (idStr::Icmp(name, "alertDisplayCount") == 0)
	{
		// display count is targeted window count
		// +1 buffer window used during force fadeout
		// cloned from template in window.cpp
		cloneChildTemplate = src->ParseInt() + 1;
		return true;
	}
	else if (idStr::Icmp(name, "alertQueueSize") == 0)
	{
		alertQueueSize = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "alertPushUpwards") == 0)
	{
		pushUpwards = src->ParseBool();
		return true;
	}
	else if (idStr::Icmp(name, "alertTextMargin") == 0)
	{
		textBGMargin = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "alertDisplayTime") == 0)
	{
		displayTime = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "alertFadeOutTime") == 0) {
		fadeOutTime = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "alertFadeInTime") == 0)
	{
		fadeInTime = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "turnOverTime") == 0)
	{
		turnOverTime = src->ParseInt();
		return true;
	}
	else if (idStr::Icmp(name, "alertAlwaysTransition") == 0)
	{
		alwaysTransition = src->ParseBool();
		return true;
	}
	else if (idStr::Icmp(name, "alertPopInDist") == 0)
	{
		popLerpDistance = src->ParseFloat();
		return true;
	}
	else if (idStr::Icmp(name, "alertFadeEdge") == 0)
	{
		fadeEdgeY = src->ParseFloat();
		return true;
	}
	else if (idStr::Icmp(name, "alertFadeInRect") == 0)
	{	
		fadeInRect.x = src->ParseFloat();
		src->CheckTokenString(",");
		fadeInRect.y = src->ParseFloat();
		src->CheckTokenString(",");
		fadeInRect.w = src->ParseFloat();
		src->CheckTokenString(",");
		fadeInRect.h = src->ParseFloat();
		return true;
	}

	if (idWindow::ParseInternalVar(name, src))
	{
		return true;
	}
	return false;
}


/*
====================
idFeedAlertWindow::PostParse()
====================
*/
void idFeedAlertWindow::PostParse() {
	idWindow::PostParse();

	alertWindows.SetNum(0);
	alertWindowsAvailable.SetNum(0);

	// get alert window template clones
	for( int idx = 0; idx < GetChildCount(); idx++)
	{
		idWindow * curChild = GetChild(idx);
		if(curChild && idStr::FindText(curChild->GetName(),AlertWindowTemplateName) >= 0)
		{
			curChild->GetWinVarByName("visible")->Set("0");
			alertWindows.Append(curChild);
			alertWindowsAvailable.Append(curChild);
		}
	}

	if(alertWindows.Num() > 0) // copy defaults from first template found
	{
		idWindow * defaultChild = alertWindows[0];
		startAlertRect = (*static_cast<idWinRectangle*>(defaultChild->GetWinVarByName("rect")));
		if( idWinRectangle * winVar = GetChildWinVar<idWinRectangle*>(defaultChild,AlertTextTemplateName,"rect") )
		{
			startTextRect = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertTextTemplateName, GetName());
		}

		if( idWinFloat * winVar = GetChildWinVar<idWinFloat*>(defaultChild,AlertTextTemplateName,"textScale") )
		{
			defaultTextScale = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertTextTemplateName, GetName());
		}

		if( idWinFloat * winVar = GetChildWinVar<idWinFloat*>(defaultChild,AlertTextTemplateName,"letterSpacing") )
		{
			defaultTextSpacing = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertTextTemplateName, GetName());
		}

		if( idWinVec4 * winVar = GetChildWinVar<idWinVec4*>(defaultChild,AlertTextTemplateName,"forecolor") )
		{
			defaultTextColor = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertTextTemplateName, GetName());
		}

		if( idWinVec4 * winVar = GetChildWinVar<idWinVec4*>(defaultChild,AlertBGTemplateName,"matcolor") )
		{
			defaultBGColor = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertBGTemplateName, GetName());
		}

		if( idWinVec4 * winVar = GetChildWinVar<idWinVec4*>(defaultChild,AlertIconTemplateName,"matcolor") )
		{
			defaultIconColor = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertIconTemplateName, GetName());
		}

		if( idWinBackground * winVar = GetChildWinVar<idWinBackground*>(defaultChild,AlertIconTemplateName,"background") )
		{
			defaultIcon = winVar->c_str();
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertIconTemplateName, GetName());
		}

		if( idWinRectangle * winVar = GetChildWinVar<idWinRectangle*>(defaultChild,AlertIconTemplateName,"rect") )
		{
			startIconRect = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertIconTemplateName, GetName());
		}

		if( idWinRectangle * winVar = GetChildWinVar<idWinRectangle*>(defaultChild,AlertBGTemplateName,"rect") )
		{
			startBGRect = *winVar;
		}
		else
		{
			common->Warning("missing window template var %s on feedlist %s",AlertBGTemplateName, GetName());
		}
	}
	else
	{
		common->Warning("missing window template %s on feedlist %s",AlertWindowTemplateName, GetName());
	}

	if(alertQueueSize <= alertWindows.Num())
	{
		alertQueueSize = alertWindows.Num()*2;
	}
	alertQueue.SetNum(alertQueueSize,true);
	alertQueueIterator = 0;
	for (int i = 0; i < alertQueue.Num(); i++)
	{
		ResetAlertState(alertQueue[i]);
	}
	activeAlerts.SetNum(0);
}

// set alert to default state
void idFeedAlertWindow::ResetAlertState(alert_t& item)
{
	item.window = nullptr;
	item.transitioned = false;
	item.ended = true; // ended = true should only happen here
	item.icon = "";
	item.displayText = "";
	item.startTime = 0;
	item.fadeInTime = 0;
	item.fadeOutTime = 0;
	item.endTime = 0;
	item.displayTime = 0;
	item.textColor = vec4_zero;
	item.bgColor = vec4_zero;
	item.iconColor = vec4_zero;
	item.fade = 0.0f;
	item.heightScale = 1.0f;
	item.debugID = -1;
}

// condense the alert queue so active alerts are together
// and update the active alert list
void idFeedAlertWindow::UpdateAlertQueues()
{
	// condense queue so active alerts are together at start
	// there should only be one inactive gap at any given time that will be filled
	// since we index 1 ahead, make for loop range 1 below queue size
	for (int idx = 0; idx < alertQueueSize-1; idx++)
	{
		// trickle fill in gap downwards
		if (GetAlert(idx).ended && !GetAlert(idx+1).ended)
		{
			GetAlert(idx) = GetAlert(idx+1);
			ResetAlertState(GetAlert(idx+1)); // clear next, will get filled next iteration
		}
	}

	activeAlerts.SetNum(0);
	for (int idx = 0; idx < alertQueueSize; idx++)
	{
		alert_t& alertItem = GetAlert(idx);
		if (!alertItem.ended)
		{
			activeAlerts.Append(&alertItem);
		}
	}
}

idFeedAlertWindow::alert_t& idFeedAlertWindow::GetAlert(int slot)
{
	return alertQueue[(alertQueueIterator + slot) % alertQueueSize];
}

idFeedAlertWindow::alert_t * idFeedAlertWindow::GetActiveAlert(int slot)
{
	if(slot >= 0 && slot < activeAlerts.Num())
	{
		return activeAlerts[slot];
	}
	return nullptr;
}

idFeedAlertWindow::alert_t* idFeedAlertWindow::GetOldestAlert()
{
	if( activeAlerts.Num() > 0)
	{
		return activeAlerts[activeAlerts.Num()-1];
	}
	return nullptr;
}

idFeedAlertWindow::alert_t& idFeedAlertWindow::ActivateNewAlert()
{
	// deactivate alert about to be recycled if needed
	alert_t& oldAlert = GetAlert(alertQueueSize-1);
	if(!oldAlert.ended)
	{
		DeactivateAlert(&oldAlert);
	}

	// reverse iterate so new slots are always at the first index position in queue
	alertQueueIterator = alertQueueIterator > 0 ? alertQueueIterator - 1 : alertQueueSize - 1;
	alert_t& newAlert = GetAlert(0);
	assert(newAlert.ended = true);
	newAlert.ended = false;
	newAlert.debugID = debugAlertTotalCount++;
	UpdateAlertQueues();
	return newAlert;
}

void idFeedAlertWindow::DeactivateAlert(alert_t* alertItem)
{
	if(alertItem->window)
	{
		alertItem->window->GetWinVarByName("visible")->Set("0");
		alertWindowsAvailable.Append(alertItem->window);
	}

	ResetAlertState(*alertItem);
	UpdateAlertQueues();
}

/*
====================
idFeedAlertWindow::DisplayAlert()
====================
*/
void idFeedAlertWindow::DisplayAlert(const char * displayText, const char * alertIcon, idVec4 * textColor, idVec4 * bgColor, idVec4 * iconColor, float durationSeconds, bool allowDupes)
{
	if(alertQueue.Num() ==0)
	{
		common->Warning("invalid feed list, likely missing template");
		return;
	}

	// SW 19th March 2025: dupe check now checks all active events, not just the most recent one
	// (this prevents alternating events slipping through when we're spamming a load of interestpoint reactions)
	if (!allowDupes)
	{
		for (int i = 0; i < activeAlerts.Num(); i++)
		{
			alert_t* prevAlert = GetActiveAlert(i);
			if (prevAlert && prevAlert->displayText == displayText)
			{
				return;
			}
		}
	}


	// recycle alert from queue
	alert_t& newAlert = ActivateNewAlert();

	newAlert.displayText = displayText;
	newAlert.icon = alertIcon ? idStr(alertIcon) : defaultIcon;

	float newDisplayTime = (durationSeconds > 0.0f ? SEC2MS(durationSeconds) : displayTime);
	float newTransitionTime = fadeInTime;
	float newFadeTime = fadeOutTime;
	RescaleTime(newDisplayTime, newTransitionTime, newFadeTime);

	newAlert.displayTime = newDisplayTime;
	newAlert.fadeInTime = newTransitionTime;
	newAlert.fadeOutTime = newFadeTime;

	// these are just suggested start and end times, dependent on window availability
	newAlert.startTime = gameLocal.time;
	newAlert.endTime = gameLocal.time + newDisplayTime;

	newAlert.textColor = textColor ? *textColor : defaultTextColor;
	newAlert.bgColor = bgColor ? *bgColor : defaultBGColor;
	newAlert.iconColor = iconColor ? *iconColor : defaultIconColor;

	newAlert.transitioned = false;
	newAlert.ended = false;

	// optional skip transition if no other alerts, from subtitle code
	if(!alwaysTransition) 
	{
		UpdateAlertQueues();
		// deactivate single remaining fading alert, so there's no transition into nearly invisible line
		if(	activeAlerts.Num() == 2 && (GetOldestAlert()->endTime - fadeOutTime < gameLocal.time) )
		{
			DeactivateAlert(GetOldestAlert());
		}

		if(activeAlerts.Num() == 1)
		{
			newAlert.transitioned = true;
		}
	}

}


void idFeedAlertWindow::WriteToSaveGame(idSaveGame* savefile) const
{
	// SM: Intentionally don't save/load this, so it resets on load

	//idWindow::WriteToSaveGame(savefile);

	//savefile->WriteInt(alertQueueIterator); // int alertQueueIterator; // blendo eric: internal index to the newest item in queue
	//
	//// idList<alert_t>	 alertQueue; // queue items, both displayed and waiting
	//savefile->WriteInt(alertQueue.Num());
	//for (int i = 0; i < alertQueue.Num(); i++) {
	//	WriteSaveGameAlert(alertQueue[i], savefile);
	//}

	//// idList<alert_t*> activeAlerts; // alerts currently alive
	//savefile->WriteInt(activeAlerts.Num());
	//for (int i = 0; i < activeAlerts.Num(); i++) {
	//	// Calculate the index into alertQueue this is pointing to
	//	int index = ((char*)activeAlerts[i] - (char*)&alertQueue[0]) / sizeof(alert_t);
	//	savefile->WriteInt(index);
	//}

	//// idList<idWindow*> alertWindows; // display windows for queue items, size = displayCount + 1
	//// idList<idWindow*> alertWindowsAvailable; // display windows available for waiting alerts

	//// specific feed window parsed vars
	//savefile->WriteInt(alertQueueSize); // int		alertQueueSize; // maximum alerts allowed in queue, both displayed and waiting
	//savefile->WriteBool(pushUpwards); // bool	pushUpwards; // untested for downwards
	//savefile->WriteBool(alwaysTransition); // bool	alwaysTransition; // transition new alerts even when no other alerts are present
	//savefile->WriteInt(textBGMargin); // int		textBGMargin; // amount bg width should extend past text
	//savefile->WriteInt(displayTime); // int		displayTime; // total time to display window
	//savefile->WriteInt(fadeOutTime); // int		fadeOutTime; // fade time at end
	//savefile->WriteInt(fadeInTime); // int		fadeInTime; // transition time into starting position
	//savefile->WriteInt(turnOverTime); // int		turnOverTime; // when at display capacity, how long oldest window waits before expiring
	//savefile->WriteFloat(popLerpDistance); // float	popLerpDistance; // distance to pop in poplerp
	//savefile->WriteFloat(fadeEdgeY); // float	fadeEdgeY; // edge pos where alerts will always fade when crossing
	//savefile->WriteRect(fadeInRect); // idRectangle fadeInRect; // the fade in transition rect for new alerts

	//// extracted vars from gui window
	//savefile->WriteRect(startAlertRect); // idRectangle startAlertRect; // the start rect for after transitioned alerts
	//savefile->WriteRect(startTextRect); // idRectangle startTextRect; // the start text rect for after transitioned alerts
	//savefile->WriteRect(startBGRect); // idRectangle startBGRect; // the start bg rect for after transitioned alerts
	//savefile->WriteRect(startIconRect); // idRectangle startIconRect; // the start bg rect for after transitioned alerts
	//savefile->WriteVec4(defaultBGColor); // idVec4 defaultBGColor;
	//savefile->WriteVec4(defaultTextColor); // idVec4 defaultTextColor;
	//savefile->WriteVec4(defaultIconColor); // idVec4 defaultIconColor;
	//savefile->WriteString(defaultIcon); // idStr  defaultIcon;
	//savefile->WriteFloat(defaultTextScale); // float defaultTextScale;
	//savefile->WriteFloat(defaultTextSpacing); // float defaultTextSpacing;

	//savefile->WriteFloat(smoothSpeedUpScale); // float smoothSpeedUpScale;

	//savefile->WriteInt(disposedState); // int disposedState;
	//savefile->WriteInt(debugAlertTotalCount); // int debugAlertTotalCount;
}


void idFeedAlertWindow::ReadFromSaveGame(idRestoreGame* savefile)
{
	// SM: Intentionally don't save/load this, so it resets on load

	//idWindow::ReadFromSaveGame(savefile);

	//savefile->ReadInt(alertQueueIterator); // int alertQueueIterator; // blendo eric: internal index to the newest item in queue

	//// idList<alert_t>	 alertQueue; // queue items, both displayed and waiting
	//int num = 0;
	//savefile->ReadInt(num);
	//alertQueue.SetNum(num);
	//for (int i = 0; i < alertQueue.Num(); i++) {
	//	ReadSaveGameAlert(alertQueue[i], savefile);
	//}

	//// idList<alert_t*> activeAlerts; // alerts currently alive
	//savefile->ReadInt(num);
	//activeAlerts.SetNum(num);
	//int index = 0;
	//for (int i = 0; i < activeAlerts.Num(); i++) {
	//	// Calculate the index into alertQueue this is pointing to
	//	savefile->ReadInt(index);
	//	activeAlerts[i] = &alertQueue[index];
	//}

	//// idList<idWindow*> alertWindows; // display windows for queue items, size = displayCount + 1
	//// idList<idWindow*> alertWindowsAvailable; // display windows available for waiting alerts

	//// specific feed window parsed vars
	//savefile->ReadInt(alertQueueSize); // int		alertQueueSize; // maximum alerts allowed in queue, both displayed and waiting
	//savefile->ReadBool(pushUpwards); // bool	pushUpwards; // untested for downwards
	//savefile->ReadBool(alwaysTransition); // bool	alwaysTransition; // transition new alerts even when no other alerts are present
	//savefile->ReadInt(textBGMargin); // int		textBGMargin; // amount bg width should extend past text
	//savefile->ReadInt(displayTime); // int		displayTime; // total time to display window
	//savefile->ReadInt(fadeOutTime); // int		fadeOutTime; // fade time at end
	//savefile->ReadInt(fadeInTime); // int		fadeInTime; // transition time into starting position
	//savefile->ReadInt(turnOverTime); // int		turnOverTime; // when at display capacity, how long oldest window waits before expiring
	//savefile->ReadFloat(popLerpDistance); // float	popLerpDistance; // distance to pop in poplerp
	//savefile->ReadFloat(fadeEdgeY); // float	fadeEdgeY; // edge pos where alerts will always fade when crossing
	//savefile->ReadRect(fadeInRect); // idRectangle fadeInRect; // the fade in transition rect for new alerts

	//// extracted vars from gui window
	//savefile->ReadRect(startAlertRect); // idRectangle startAlertRect; // the start rect for after transitioned alerts
	//savefile->ReadRect(startTextRect); // idRectangle startTextRect; // the start text rect for after transitioned alerts
	//savefile->ReadRect(startBGRect); // idRectangle startBGRect; // the start bg rect for after transitioned alerts
	//savefile->ReadRect(startIconRect); // idRectangle startIconRect; // the start bg rect for after transitioned alerts
	//savefile->ReadVec4(defaultBGColor); // idVec4 defaultBGColor;
	//savefile->ReadVec4(defaultTextColor); // idVec4 defaultTextColor;
	//savefile->ReadVec4(defaultIconColor); // idVec4 defaultIconColor;
	//savefile->ReadString(defaultIcon); // idStr  defaultIcon;
	//savefile->ReadFloat(defaultTextScale); // float defaultTextScale;
	//savefile->ReadFloat(defaultTextSpacing); // float defaultTextSpacing;

	//savefile->ReadFloat(smoothSpeedUpScale); // float smoothSpeedUpScale;

	//savefile->ReadInt(disposedState); // int disposedState;
	//savefile->ReadInt(debugAlertTotalCount); // int debugAlertTotalCount;
}

//void idFeedAlertWindow::WriteSaveGameAlert(const alert_t& alert, idSaveGame* savefile) const
//{
//	savefile->WriteInt(alert.debugID); // int		debugID;
//	savefile->WriteBool(alert.transitioned); // bool	transitioned; // true after fade in
//	savefile->WriteBool(alert.ended); // bool	ended; // true when reset, and no longer valid
//	savefile->WriteString(alert.icon); // idStr	icon; // asset str
//	savefile->WriteString(alert.displayText); // idStr	displayText;
//	savefile->WriteVec4(alert.textColor); // idVec4	textColor;
//	savefile->WriteVec4(alert.bgColor); // idVec4	bgColor;
//	savefile->WriteVec4(alert.iconColor); // idVec4	iconColor;
//	savefile->WriteInt(alert.startTime); // int		startTime; // ms time of alert creation (start of transition/fade in)
//	savefile->WriteInt(alert.fadeInTime); // int		fadeInTime; // ms
//	savefile->WriteInt(alert.fadeOutTime); // int		fadeOutTime; // ms
//	savefile->WriteInt(alert.endTime); // int		endTime; // ms time when the alert should no longer exist
//	savefile->WriteInt(alert.displayTime); // int		displayTime; // ms unaltered total time to display
//	savefile->WriteFloat(alert.fade); // float	fade;
//	savefile->WriteFloat(alert.heightScale); // float	heightScale; // alters the default rect, currently for double lines
//	savefile->WriteInt(alertWindows.FindIndex(alert.window)); // idWindow* window;	// draw window assigned when window begins transition
//}
//
//
//void idFeedAlertWindow::ReadSaveGameAlert(alert_t& alert, idRestoreGame* savefile)
//{
//	savefile->ReadInt(alert.debugID); // int		debugID;
//	savefile->ReadBool(alert.transitioned); // bool	transitioned; // true after fade in
//	savefile->ReadBool(alert.ended); // bool	ended; // true when reset, and no longer valid
//	savefile->ReadString(alert.icon); // idStr	icon; // asset str
//	savefile->ReadString(alert.displayText); // idStr	displayText;
//	savefile->ReadVec4(alert.textColor); // idVec4	textColor;
//	savefile->ReadVec4(alert.bgColor); // idVec4	bgColor;
//	savefile->ReadVec4(alert.iconColor); // idVec4	iconColor;
//	savefile->ReadInt(alert.startTime); // int		startTime; // ms time of alert creation (start of transition/fade in)
//	savefile->ReadInt(alert.fadeInTime); // int		fadeInTime; // ms
//	savefile->ReadInt(alert.fadeOutTime); // int		fadeOutTime; // ms
//	savefile->ReadInt(alert.endTime); // int		endTime; // ms time when the alert should no longer exist
//	savefile->ReadInt(alert.displayTime); // int		displayTime; // ms unaltered total time to display
//	savefile->ReadFloat(alert.fade); // float	fade;
//	savefile->ReadFloat(alert.heightScale); // float	heightScale; // alters the default rect, currently for double lines
//	
//	int windowIndex = 0;
//	savefile->ReadInt(windowIndex); // idWindow* window;	// draw window assigned when window begins transition
//	alert.window = windowIndex != -1 ? alertWindows[windowIndex] : nullptr;
//}

/*
====================
idFeedAlertWindow::SetupWindow()
====================
* initialize sub windows with alert data
*/
void idFeedAlertWindow::SetupWindow(alert_t& alertItem, idWindow* alertWindow)
{
	alertItem.startTime = gameLocal.time;
	alertItem.endTime = gameLocal.time + alertItem.displayTime;
	alertItem.window = alertWindow;
	alertWindow->GetWinVarByName("visible")->Set("1");

	drawWin_t * iconChild = alertWindow->FindChildByName(AlertIconTemplateName);
	drawWin_t * bgChild = alertWindow->FindChildByName(AlertBGTemplateName);
	drawWin_t * textChild = alertWindow->FindChildByName(AlertTextTemplateName);

	// initialize basic vars
	GetChildWinVar<idWinBackground*>(iconChild, "background")->Set(alertItem.icon);
	GetChildWinVar<idWinStr*>(textChild, "text")->Set(alertItem.displayText);

	// begin fitting window to text and icon size
	idRectangle iconRect = startIconRect;
	float initialIconPad = Min(iconRect.x, startAlertRect.w - (iconRect.x + iconRect.w)); // icon padding away from alert border
	float iconPlusPad = iconRect.w + initialIconPad;

	bool noWrap = true;
	if(textChild->simp)
	{  // need font set for size info
		noWrap = textChild->simp->GetFlags() & WIN_NOWRAP;
		textChild->simp->SetFont();
	}
	else
	{
		noWrap = textChild->win->GetFlags() & WIN_NOWRAP;
		textChild->win->SetFont(); 
	}

	const int textRectCorrection = 2;
	float textErrorMargin = noWrap ? 0.0f : textRectCorrection; // min margin to prevent cropping/wrapping on exact rect sizes
	float textMargin = Max(0.0f, (float)textBGMargin - textErrorMargin);
	float textWidth = textErrorMargin + dc->TextWidth( alertItem.displayText, defaultTextScale, 0, defaultTextSpacing) + textErrorMargin;
	float textPlusMargins = textMargin + textWidth + textMargin;
	float textHeight = dc->MaxCharHeight(defaultTextScale) + textErrorMargin;

	float extraLineHeight = 0.0f;
	bool shouldWrap = !noWrap && ((textPlusMargins - 1) > (startTextRect.w - iconPlusPad));
	if(shouldWrap)
	{
		textWidth = startAlertRect.w - 2.0f*textMargin - iconPlusPad; // use up all the space
		textPlusMargins = textMargin + textWidth + textMargin;
		extraLineHeight = textHeight + 5;
	}

	idRectangle alertRect = startAlertRect;
	alertRect.h = alertRect.h + extraLineHeight;
	alertWindow->GetWinVarByName("rect")->Set(alertRect.ToVec4().ToString());
	alertWindow->CalcClientRect(0.0f,0.0f);

	// move text over for icon
	idRectangle curTextRect = startTextRect;
	curTextRect.w = Min(textWidth,startTextRect.w);
	curTextRect.x = startAlertRect.w - iconPlusPad - textMargin - textWidth;
	curTextRect.h = textHeight + extraLineHeight + textRectCorrection;
	GetChildWinVar<idWinRectangle*>(textChild, "rect")->Set(curTextRect.ToVec4().ToString());

	if(textChild->simp) { textChild->simp->CalcClientRect(0.0f,0.0f); }
	else { textChild->win->CalcClientRect(0.0f,0.0f); }

	// expand bg for text and icon
	// precalc text width
	float bgWidth = Min(textPlusMargins + iconPlusPad, startBGRect.w); // don't exceed initial size
	float initialBGPadX = startAlertRect.w - (startBGRect.x+startBGRect.w); // bg padding derived from init 
	float initialBGPadY = startAlertRect.h - (startBGRect.y+startBGRect.h); // bg padding derived from init 
	idRectangle bgRect = startBGRect;
	bgRect.w = Min(bgWidth,startBGRect.w);
	bgRect.x = Max(0.0f, startAlertRect.w - initialBGPadX - bgWidth); // don't move beyond container
	bgRect.h = startBGRect.h + extraLineHeight;
	bgRect.y = Max(0.0f, startAlertRect.h - initialBGPadY - bgRect.h); // don't move beyond container
	GetChildWinVar<idWinRectangle*>(bgChild, "rect")->Set(bgRect.ToVec4().ToString());

	if(bgChild->simp) { bgChild->simp->CalcClientRect(0.0f,0.0f); }
	else { bgChild->win->CalcClientRect(0.0f,0.0f); }

	// recenter icon
	iconRect.y = alertRect.h*0.5f - iconRect.h*0.5f;
	GetChildWinVar<idWinRectangle*>(iconChild, "rect")->Set(iconRect.ToVec4().ToString());

	if(iconChild->simp) { iconChild->simp->CalcClientRect(0.0f,0.0f); }
	else { iconChild->win->CalcClientRect(0.0f,0.0f); }

	alertItem.heightScale = alertRect.h / startAlertRect.h;
}

/*
====================
idFeedAlertWindow::Update()
====================
	alert life cycle:
	-> alert added, activated/initialized
	-> invisible, waiting for last alert to transition completely
	-> start transition, give alert a visisble window
	-> displace/move against other windows
	-> expiration fadeout
	-> deactivated/recycled, window freed back into pool
*/
void idFeedAlertWindow::Update()
{
	if( activeAlerts.Num() == 0 ){ return; }

	{ // deactive expired alerts that have been displayed, will also condense queue, but should not affect iterator
		for (int idx = activeAlerts.Num()-1; idx >= 0 ; idx--)
		{
			alert_t* curAlert = activeAlerts[idx];
			if(curAlert->window && (curAlert->endTime <= gameLocal.time))
			{
				DeactivateAlert(curAlert);
			}
		}
	}

	// assign alerts a window from unused pool, from oldest to newest
	for (int idx = activeAlerts.Num()-1; idx >= 0 ; idx--)
	{
		if( alertWindowsAvailable.Num() == 0)
		{
			break;
		}

		alert_t* oldestAlert = GetOldestAlert();
		if(alertWindowsAvailable.Num() == 1)
		{
			bool oldestFading = (oldestAlert->endTime - oldestAlert->fadeOutTime) < gameLocal.time;
			// skip until a window is free
			if(!oldestFading)
			{
				break;
			}
		}

		alert_t& alertItem = *activeAlerts[idx];
		assert( !alertItem.ended );

		idWindow* alertWindow = alertItem.window;
		if( !alertWindow )
		{
			// wait for transition of older alert, unless at capacity
			if( !NearCapacity())
			{
				if( alert_t* nextAlert = GetActiveAlert(idx+1) )
				{
					if( !nextAlert->transitioned )
					{
						continue;
					}
				}
			}
			// assign unused alert window
			alertWindow = alertWindowsAvailable[alertWindowsAvailable.Num()-1];
			alertWindowsAvailable.RemoveIndex(alertWindowsAvailable.Num()-1);
			SetupWindow(alertItem, alertWindow);
		}
	}

	// make sure the extra window fades out as fast as newest window fades in
	if( alertWindowsAvailable.Num() == 0 )
	{
		// get the absolute fade in time of the newest window
		int fadingInAlertTime = 0;
		for (int idx = 0; idx < activeAlerts.Num(); idx++)
		{
			alert_t* curAlert = activeAlerts[idx];
			if(curAlert->window && !curAlert->transitioned)
			{
				fadingInAlertTime = activeAlerts[idx]->startTime + activeAlerts[idx]->fadeInTime;
				break;
			}
		}

		// fade oldest window as fast as new one transitions in
		alert_t* oldestAlert = GetOldestAlert();
		oldestAlert->endTime = Min(oldestAlert->endTime, fadingInAlertTime);
	}

	// if there's not enough windows, lower the expiration time
	// of the oldest windows
	bool speedUpExpiration =  activeAlerts.Num() >= alertWindows.Num();

	if(speedUpExpiration && activeAlerts.Num() == alertWindows.Num())
	{
		// don't speed up expiration if the oldest alert is already fading
		alert_t* oldestAlert = GetOldestAlert();
		speedUpExpiration = (oldestAlert->endTime - oldestAlert->fadeOutTime) > gameLocal.time;
	}

	float fadeTimeScale = NearCapacity() ? 0.5f : 1.0f; // shorten the fade time when near capacity

	// displacing/moving "line" which proceeding windows should not cross
	float displacedPos = pushUpwards ? 10000 : -10000; // initial values set to not interfere with first window
	int windowIdx = 0;
	// update visible alert windows, transitions/fading/pushing movement, from newest to oldest
	for (int idx = 0; idx < activeAlerts.Num(); idx++)
	{
		alert_t& alertItem = *activeAlerts[idx];
		idWindow* alertWindow = alertItem.window;
		assert( !alertItem.ended );
		if(!alertWindow) { continue; } // not visible yet

		idRectangle curRect = startAlertRect;

		int fadeInScaled = Max( (int)(alertItem.fadeInTime*fadeTimeScale), 1 );
		int fadeOutScaled = Max( alertItem.fadeOutTime, 1 );

		if( speedUpExpiration )
		{
			// last window(s) should use the turn over time when at capacity
			// to begin fadeout asap
			if(windowIdx >= alertWindows.Num()-2)
			{
				alertItem.endTime = Min(gameLocal.time + turnOverTime, alertItem.endTime);
			}
		}

		// if fade in overlaps with fadeout, use the non overlapped time instead
		fadeInScaled = idMath::ClampInt(1, fadeInScaled, alertItem.endTime - fadeOutScaled - alertItem.startTime); 

		float fadeInInterp = (float)(gameLocal.time - alertItem.startTime) / fadeInScaled;
		float fadeOutInterp = (float)(alertItem.endTime - gameLocal.time) / fadeOutScaled;
		if( !alertItem.transitioned )
		{
			if(fadeInInterp > 0.99f)
			{
				alertItem.transitioned = true;
			}

			curRect.x = idMath::PopLerp(fadeInRect.x, Sign(startAlertRect.x-fadeInRect.x)*popLerpDistance + startAlertRect.x, startAlertRect.x, fadeInInterp);
			curRect.y = idMath::PopLerp(fadeInRect.y, Sign(startAlertRect.y-fadeInRect.y)*popLerpDistance + startAlertRect.y, startAlertRect.y, fadeInInterp);
			curRect.w = idMath::Lerp(fadeInRect.w, startAlertRect.w, fadeInInterp);
			curRect.h = idMath::Lerp(fadeInRect.h, startAlertRect.h, fadeInInterp);
		}

		if( alertItem.transitioned )
		{
			fadeInInterp = Max(1.0f,fadeInInterp);
		}

		// alter initial alert size if doubleline
		curRect.h *= alertItem.heightScale;

		// make sure last y pos actually needs displacing
		float curEdge = static_cast<idWinRectangle*>(alertWindow->GetWinVarByName("rect"))->y() + curRect.h;
		curEdge = pushUpwards ? Min(curRect.Bottom(),curEdge) : Max(curRect.y,curEdge);
		// the preceeding alert window pushes/displaces the current alert to a new position using min/max
		curEdge = pushUpwards ? Min(curEdge,displacedPos) : Max(curEdge,displacedPos);

		curRect.y = curEdge - curRect.h;
		alertWindow->GetWinVarByName("rect")->Set(curRect.ToVec4().ToString());

		// force fade if it starts crossing the border of the window
		float edgeFaded = (pushUpwards ? (curRect.y - fadeEdgeY) : ( rect.h() + fadeEdgeY - curRect.y) ) / curRect.h;
		edgeFaded = idMath::ClampFloat(0.0f, 1.0f, edgeFaded );

		float faded = idMath::ClampFloat(0.0f, 1.0f, min(fadeInInterp,fadeOutInterp) );
		faded = Min(edgeFaded,faded);

		// make sure fade is always moving in proper direction
		faded = alertItem.transitioned ?  Min(alertItem.fade,faded) : Max(alertItem.fade,faded);
		alertItem.fade = faded;

		idVec4 colorFaded = alertItem.textColor;
		colorFaded.w *= faded;
		alertWindow->SetChildWinVarVal(AlertTextTemplateName,"forecolor",colorFaded.ToString());

		idVec4 bgFaded = alertItem.bgColor;
		bgFaded.w *= faded;
		alertWindow->SetChildWinVarVal(AlertBGTemplateName,"matcolor",bgFaded.ToString());

		idVec4 iconFaded = alertItem.iconColor;
		iconFaded.w *= faded;
		alertWindow->SetChildWinVarVal(AlertIconTemplateName,"matcolor",iconFaded.ToString());

		// displacement calcs to progress alerts
		float displaceInterp = 1.0f;
		float displaceDistance = curRect.h;

		// if alerts transition in from the side, use timing instead of actual border to displace
		if( idMath::Fabs(startAlertRect.y-fadeInRect.y) < startAlertRect.w )
		{
			// increase the speed of displacement to prevent overlap while side moving
			float displaceInterpScale = 2.0f;
			// half speed for double line alerts
			displaceInterpScale = displaceInterpScale * ( startAlertRect.h / displaceDistance  );
			displaceInterp = idMath::ClampFloat(0.0f, 1.0f, fadeInInterp*displaceInterpScale);
		}

		if( pushUpwards) 
		{
			// displace pushes next alert upwards (decreasing value of Y)
			displacedPos = curRect.Bottom() - displaceInterp*(displaceDistance);
		}
		else
		{
			displacedPos = curRect.y + displaceInterp*(displaceDistance);
		}

		if(edgeFaded <= 0.001f)
		{ // expire if it exits the border of the list window
			alertItem.endTime = gameLocal.time;
		}

		windowIdx++;
	}
}
