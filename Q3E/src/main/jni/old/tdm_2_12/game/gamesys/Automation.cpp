/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#include "Automation_local.h"
#include "../Missions/MissionManager.h"


#ifdef _DEBUG
//allow developers to enable automation persistently
#define AUTOMATION_VARMODE CVAR_ARCHIVE
#else
//make sure all players have automation disabled on TDM releases
//note: they can enable it manually in console, but setting won't be saved
#define AUTOMATION_VARMODE 0
#endif

idCVar com_automation("com_automation", "0", CVAR_BOOL | AUTOMATION_VARMODE, "Enable TDM automation for connection with DarkRadiant");
idCVar com_automation_port("com_automation_port", "3879", CVAR_INTEGER | CVAR_ARCHIVE, "The TCP port number to be listened to");

Automation automationLocal;
Automation *automation = &automationLocal;


void GameplayControlPlan::Clear() {
	Kill();
	for (int i = 0; i < 8; i++)
		buttons[i].Clear();
	for (int i = 0; i < 3; i++)
		moves[i].Clear();
	for (int i = 0; i < 3; i++)
		angles[i].Clear();
}

void GameplayControlPlan::Finalize() {
	//get current view angles
	idAngles initialAngles;
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player)
		initialAngles = player->viewAngles;

	//choose mode for viewangles
	if (angleMode == amNone)
		angleMode = amAbsolute;	//default
	if (angleMode == amRelative) {
		//all angles are set relative to initial values
		for (int i = 0; i < 3; i++)
			for (auto &span : angles[i]) {
				span.startValue += initialAngles[i];
				span.endValue += initialAngles[i];
			}
		angleMode = amAbsolute;
	}
	if (angleMode == amSmooth) {
		//all angles are set absolute, but first start value if modified to initial value
		for (int i = 0; i < 3; i++) {
			int k = angles[i].Num() - 1;
			if (k >= 0)
				angles[i][k].startValue = initialAngles[i];
		}
		angleMode = amAbsolute;
	}

	//if some signal has no spans, add one span with default value
	for (int i = 0; i < 8; i++)
		if (buttons[i].Num() == 0) {
			float value = 0;
			if ((1<<i) == BUTTON_MLOOK)
				value = 1;		//assume in_freeLook = 1
			buttons[i].Append(CubicSpan{0, 0, value, value, 0, 0});
		}
	for (int i = 0; i < 3; i++)
		if (moves[i].Num() == 0)
			moves[i].Append(CubicSpan{0, 0, 0, 0, 0, 0});
	for (int i = 0; i < 3; i++)
		if (angles[i].Num() == 0)
			angles[i].Append(CubicSpan{0, 0, initialAngles[i], initialAngles[i], 0, 0});
}

static bool RemoveExpired(idList<CubicSpan> &plan, float nowTime) {
	assert(plan.Num() > 0);
	while (nowTime > plan[plan.Num() - 1].endParam) {
		if (plan.Num() == 1)
			return true;
		else
			plan.Resize(plan.Num() - 1);
	}
	return false;
}
bool GameplayControlPlan::RemoveExpired(float nowTime) {
	if (!IsAlive())
		return true;	//never set or expired long ago
	//remove all spans which have ended before current time
	int cntExpired = 0;
	for (int i = 0; i < 8; i++)
		cntExpired += ::RemoveExpired(buttons[i], nowTime);
	for (int i = 0; i < 3; i++)
		cntExpired += ::RemoveExpired(moves[i], nowTime);
	for (int i = 0; i < 3; i++)
		cntExpired += ::RemoveExpired(angles[i], nowTime);
	//the whole plan is expired if all plans are expired and all impulses are finished
	if (cntExpired == 14 && impulse.Num() == 0)
		return true;
	return false;
}

static float GetSignal(const idList<CubicSpan> &plan, float nowTime) {
	//find where we are in the current span
	const auto &c = plan[plan.Num() - 1];
	float len = (c.endParam - c.startParam);
	float ratio = (nowTime - c.startParam) / idMath::Fmax(len, 1e-10f);
	ratio = idMath::ClampFloat(0.0f, 1.0f, ratio);

	//evaluate cubic function via some Bezier-like formulas
	float t = ratio, r = 1.0f - ratio;
	float p0 = c.startValue, p1 = c.startValue + c.startDeriv * (len / 3.0f);
	float p3 = c.endValue, p2 = c.endValue - c.endDeriv * (len / 3.0f);
	float value = 0.0f;
	value += p0 * (r * r * r);
	value += p1 * (r * r * t) * 3.0f;
	value += p2 * (r * t * t) * 3.0f;
	value += p3 * (t * t * t);

	return value;
}
static int GetImpulse(idList<ImpulseMoment> &plan, float nowTime) {
	if (plan.Num() == 0)
		return -1;	//no more impulses
	const auto &last = plan[plan.Num() - 1];
	if (nowTime < last.param)
		return -1;	//next impulse not yet ready
	int res = last.impulse;
	plan.Resize(plan.Num() - 1);
	return res;
}
bool GameplayControlPlan::GetUsercmd(float nowTime, usercmd_t &cmd) {
	//remove past pieces of the plan, check if the whole plan has ended
	if (RemoveExpired(nowTime))
		Kill();			//finished
	if (!IsAlive())
		return false;	//no active plan now

	//calculate values for all buttons
	cmd.buttons = 0;
	for (int b = 0; b < 8; b++) {
		float level = GetSignal(buttons[b], nowTime);
		cmd.buttons ^= (level >= 0.5f ? 1 : 0) << b;
	}

	//calculate values for various moves
	//note: see KEY_MOVESPEED in UsercmdGen.cpp
	//this is how much "move" you do when you press a key
	const int KEY_MOVESPEED = 127;
	cmd.forwardmove = byte(KEY_MOVESPEED * GetSignal(moves[0], nowTime));
	cmd.rightmove   = byte(KEY_MOVESPEED * GetSignal(moves[1], nowTime));
	cmd.upmove      = byte(KEY_MOVESPEED * GetSignal(moves[2], nowTime));

	//calculate values for view angles
	idAngles deltaViewAngles;
	idPlayer *player = gameLocal.GetLocalPlayer();
	if (player) {
		deltaViewAngles = player->GetDeltaViewAngles();
	}
	assert(angleMode == amAbsolute);
	for (int a = 0; a < 3; a++) {
		//see formula for TestAngles in idPlayer::UpdateViewAngles
		float effectiveAngle = GetSignal(angles[a], nowTime);
		float relativeAngle = idMath::AngleNormalize180(effectiveAngle - deltaViewAngles[a]);
		cmd.angles[a] = ANGLE2SHORT(relativeAngle);
	}

	//check if we have impulse ready
	int imp = GetImpulse(impulse, nowTime);
	if (imp >= 0) {
		cmd.impulse = imp;
		cmd.flags ^= UCF_IMPULSE_SEQUENCE;
	}

	return true;
}

int GameplayControlPlan::GetTimeNow() const {
	static uint64_t astroTimeStart = Sys_GetTimeMicroseconds();
	if (timeMode == tmAstronomical)
		return (Sys_GetTimeMicroseconds() - astroTimeStart) / 1000;
	if (timeMode == tmGamePhysics)
		return gameLocal.time;
	assert(0);
	return -1000000000;
}




void Automation::PushInputEvents(int key, int value) {
	sysEvent_t ev;
	memset(&ev, 0, sizeof(ev));
	ev.evType = SE_KEY;
	ev.evValue = key;
	ev.evValue2 = value;
	eventLoop->PushEvent(&ev);
}

bool Automation::GetUsercmd(usercmd_t &cmd) {
	if (!gameControlPlan.IsAlive())
		return false;

	//fetch current time in the specified time measurement system
	float nowTime = (gameControlPlan.GetTimeNow() - gameControlPlan_time) * 1e-3;

	//ask our plan for the data
	cmd.flags = usercmdGen->hack_Flags();
	bool ok = gameControlPlan.GetUsercmd(nowTime, cmd);
	usercmdGen->hack_Flags() = cmd.flags;

	if (!ok) {
		//make sure view angles do not jump back immediately afterwards
		idPlayer *player = gameLocal.GetLocalPlayer();
		if (player) {
			idAngles angles;
			for (int a = 0; a < 3; a++)
				angles[a] = gameControlPlan.angles[a][0].endValue;
			usercmdGen->ClearAngles();
			player->SetDeltaViewAngles(player->viewAngles = angles);
			player->cmdAngles = idAngles();
		}

		WriteGameControlResponse("finished");
		return false;	//no valid plan now: return control to human
	}

	cmd.duplicateCount = 0;
	cmd.mx = cmd.my = 0;
	cmd.sequence = 0;
	cmd.gameFrame = cmd.gameTime = 0;
	return true;
}

void Automation::ExecuteInGameConsole(const char *command, bool blocking) {
	cmdSystem->BufferCommandText(blocking ? CMD_EXEC_NOW : CMD_EXEC_APPEND, command);
}

void Automation::ParseMessage(const char *message, int len) {
	ParseIn parseIn = { message, len, idLexer{message, len, "<automation input>"}, -1 };
	ParseMessage(parseIn);
}

void Automation::ParseMessage(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("seqno");
	parseIn.lexer.ExpectTokenType(TT_NUMBER, TT_INTEGER, &token);
	parseIn.seqno = token.GetIntValue();

	parseIn.lexer.ExpectTokenString("message");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);
	if (token == "action")
		ParseAction(parseIn);
	else if (token == "query")
		ParseQuery(parseIn);
	else
		parseIn.lexer.Error("Unknown message type '%s'", token.c_str());
}

void Automation::ParseAction(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("action");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);

	parseIn.lexer.ExpectTokenString("content");
	parseIn.lexer.CheckTokenString(":");

	int pos = parseIn.lexer.GetFileOffset();
	const char *rest = strchr(parseIn.message + pos, '\n');


	if (token == "conexec") {
		int begMarker = common->GetConsoleMarker();
		while (const char *eol = strchr(rest, '\n')) {
			idStr command(rest, 0, eol - rest);
			if (!command.IsEmpty())
				ExecuteInGameConsole(command.c_str());
			rest = eol + 1;
		}
		int endMarker = common->GetConsoleMarker();
		idStr consoleUpdates = common->GetConsoleContents(begMarker, endMarker);
		WriteResponse(parseIn.seqno, consoleUpdates.c_str());
		return;
	}

	if (token == "guiscript") {
		bool ok = false;
		char name[256];
		int scriptNum;
		if (sscanf(rest, "%s%d", name, &scriptNum) == 2)
			ok = session->RunGuiScript(name, scriptNum);
		WriteResponse(parseIn.seqno, (ok ? "done" : "error"));
	}

	if (token == "installfm") {
		int modsNum = gameLocal.m_MissionManager->GetNumMods();

		char name[256];
		int argsNum = sscanf(rest, "%s", name);
		if (argsNum <= 0) {
			idStr modList;
			for (int i = 0; i < modsNum; i++) {
				modList += gameLocal.m_MissionManager->GetModInfo(i)->modName;
				modList += "\n";
			}
			WriteResponse(parseIn.seqno, modList.c_str());
		}
		else {
			int idx = -1;
			for (int i = 0; i < modsNum; i++)
				if (gameLocal.m_MissionManager->GetModInfo(i)->modName == idStr(name))
					idx = i;
			bool ok = false;
			if (idx >= 0) {
				auto res = gameLocal.m_MissionManager->InstallMod(idx);
				ok = (res == CMissionManager::INSTALLED_OK);
				if (ok)
					cmdSystem->SetupReloadEngine(idCmdArgs());
			}
			else if (idStr::Cmp(name, "0") == 0) {
				gameLocal.m_MissionManager->UninstallMod();
				ok = true;
				cmdSystem->SetupReloadEngine(idCmdArgs());
			}
			WriteResponse(parseIn.seqno, (ok ? "done" : "error"));
		}
	}

	if (token == "sysctrl") {
		int key, value;
		if (sscanf(rest, "%d%d", &key, &value) == 2)
			PushInputEvents(key, value);
		WriteResponse(parseIn.seqno, "");
		return;
	}

	if (token == "gamectrl") {
		if (gameControlPlan_seqno >= 0)
			WriteGameControlResponse("interrupted");
		gameControlPlan_seqno = parseIn.seqno;
		gameControlPlan_time = -1;
		GameplayControlPlan &plan = gameControlPlan;
		plan.Clear();

		while (!parseIn.lexer.EndOfFile()) {
			parseIn.lexer.ExpectTokenType(TT_NAME, 0, &token);

			if (token == "timemode") {
				parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);
				if (token.Left(5) == "astro")
					plan.timeMode = GameplayControlPlan::tmAstronomical;
				else if (token.Left(4) == "game")
					plan.timeMode = GameplayControlPlan::tmGamePhysics;
				else
					parseIn.lexer.Error("Expected time mode, found '%s'", token.c_str());
				continue;
			}

			if (token == "anglemode") {
				parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);
				if (token.Left(3) == "abs")
					plan.angleMode = GameplayControlPlan::amAbsolute;
				else if (token.Left(3) == "rel")
					plan.angleMode = GameplayControlPlan::amRelative;
				else if (token == "smooth")
					plan.angleMode = GameplayControlPlan::amSmooth;
				else
					parseIn.lexer.Error("Expected angle mode, found '%s'", token.c_str());
				continue;
			}

			if (token == "impulse") {
				while (parseIn.lexer.PeekTokenType(TT_PUNCTUATION, 0, &token) && token == "(") {
					parseIn.lexer.ExpectTokenString("(");
					ImpulseMoment imp;
					imp.impulse = parseIn.lexer.ParseInt();
					imp.param = parseIn.lexer.ParseFloat();
					plan.impulse.Append(imp);
					parseIn.lexer.ExpectTokenString(")");
				}
				plan.impulse.Reverse();
				continue;
			}

			idList<CubicSpan> *series = 0;
			int len = -1;
			idStr suffix;
			if (token.Left(6) == "button") {
				series = plan.buttons;  len = 8;
				suffix = token.Right(token.Length() - 6);
			}
			if (token.Left(4) == "move") {
				series = plan.moves;  len = 3;
				suffix = token.Right(token.Length() - 4);
			}
			if (token.Left(5) == "angle") {
				series = plan.angles;  len = 3;
				suffix = token.Right(token.Length() - 5);
			}

			if (series == 0)
				parseIn.lexer.Error("Expected signal or impulses, found '%s'", token.c_str());
			int index = -1;
			sscanf(suffix.c_str(), "%d", &index);
			if (index < 0 || index >= len)
				parseIn.lexer.Error("Signal index is too large: %d", index);
			idList<CubicSpan> &signal = series[index];

			while (parseIn.lexer.PeekTokenType(TT_PUNCTUATION, 0, &token) && token == "(") {
				parseIn.lexer.ExpectTokenString("(");
				CubicSpan span;
				span.startValue = parseIn.lexer.ParseFloat();
				span.endValue = parseIn.lexer.ParseFloat();
				span.startDeriv = parseIn.lexer.ParseFloat();
				span.endDeriv = parseIn.lexer.ParseFloat();
				span.startParam = parseIn.lexer.ParseFloat();
				span.endParam = parseIn.lexer.ParseFloat();
				signal.Append(span);
				parseIn.lexer.ExpectTokenString(")");
			}
			signal.Reverse();
		}

		plan.Finalize();
		gameControlPlan_time = plan.GetTimeNow();

		return;
	}

	if (token == "reloadmap-diff") {
		//on-the-fly HotReload from DarkRadiant
		int begMarker = common->GetConsoleMarker();
		gameLocal.HotReloadMap(rest);
		int endMarker = common->GetConsoleMarker();
		idStr consoleUpdates = common->GetConsoleContents(begMarker, endMarker);
		WriteResponse(parseIn.seqno, consoleUpdates.c_str());
		return;
	}
}

void Automation::WriteGameControlResponse(const char *message) {
	WriteResponse(gameControlPlan_seqno, message);
	gameControlPlan_seqno = -1;
}

extern int showFPS_currentValue;	//for "fps" query
void Automation::ParseQuery(ParseIn &parseIn) {
	idToken token;

	parseIn.lexer.ExpectTokenString("query");
	parseIn.lexer.ExpectTokenType(TT_STRING, 0, &token);

	parseIn.lexer.ExpectTokenString("content");
	parseIn.lexer.CheckTokenString(":");

	int pos = parseIn.lexer.GetFileOffset();
	const char *rest = strchr(parseIn.message + pos, '\n');

	if (token == "fps") {
		char buff[256];
		sprintf(buff, "%d\n", showFPS_currentValue);
		WriteResponse(parseIn.seqno, buff);
	}

	if (token == "console") {
		int arg0, arg1;
		int argsNum = sscanf(rest, "%d%d", &arg0, &arg1);
		int consoleEnd = common->GetConsoleMarker();

		if (argsNum <= 0) {
			//return size of console in chars
			char buff[256];
			sprintf(buff, "%d\n", consoleEnd);
			WriteResponse(parseIn.seqno, buff);
		}
		else if (argsNum == 2) {
			auto decodePos = [consoleEnd](int x) -> int {	//python-style indexing
				x = (x < 0 ? consoleEnd : 0) + x;
				return idMath::ClampInt(0, consoleEnd, x);
			};
			//fetch console chars
			int beg = decodePos(arg0);
			int end = decodePos(arg1);
			if (end < beg) end = beg;
			idStr consoleUpdates = common->GetConsoleContents(beg, end);
			WriteResponse(parseIn.seqno, consoleUpdates.c_str());
		}
		else 
			WriteResponse(parseIn.seqno, "");
	}

	if (token == "status") {
		CModInfoPtr mod = gameLocal.m_MissionManager->GetCurrentModInfo();
		const char *modName = (mod ? mod->modName.c_str() : "0");
		const char *mapName = gameLocal.GetMapName();
		if (idStr::Cmpn(mapName, "maps/", 5) == 0)
			mapName = mapName + 5;
		const char *guiActiveName = "";
		if (idUserInterface *guiActive = session->GetGui(idSession::gtActive)) {
			if (guiActive == session->GetGui(idSession::gtMainMenu))
				guiActiveName = "mainmenu";
			else if (guiActive == session->GetGui(idSession::gtLoading))
				guiActiveName = "loading";
			else
				guiActiveName = "?unknown?";
		}

		idStr text;
		char buff[256];
		sprintf(buff, "currentfm %s\n", modName);
		text += buff;
		sprintf(buff, "mapname %s\n", mapName);
		text += buff;
		sprintf(buff, "guiactive %s\n", guiActiveName);
		text += buff;
		WriteResponse(parseIn.seqno, text.c_str());
	}
}

void Automation::WriteResponse(int seqno, const char *response) {
	char buff[256];
	sprintf(buff, "response %d\n", seqno);
	idStr text = idStr(buff) + response;
	connection.WriteMessage(text.c_str(), text.Length());
}

bool Automation::TryListen() {
	if (listenTcp.IsAlive())
		return true;

	int port = com_automation_port.GetInteger();

	static int64 lastAttemptAtTime = -int64(1e+18);
	static int retriesCount = 0;

	//if fails, retry every second
	int64 nowTime = Sys_Microseconds();
	if (nowTime - lastAttemptAtTime > 1000000) {
		lastAttemptAtTime = nowTime;

		listenTcp.Listen(port);
		if (listenTcp.IsAlive()) {
			common->Printf("Automation now listens on port %d\n", port);
			retriesCount = 0;
		}
		else {
			common->Warning("Automation cannot listen on port %d", port);
			if (++retriesCount >= 30) {
				retriesCount = 0;
				com_automation.SetBool(false);
				common->Printf("com_automation disabled\n");
			}
		}
	}

	//still not working -> skip automation
	if (listenTcp.IsAlive())
		return true;
	return false;
}

bool Automation::TryConnect() {
	if (connection.IsAlive())
		return true;

	std::unique_ptr<idTCP> clientTcp(listenTcp.Accept());
	if (!clientTcp)
		return false;

	const auto &addr = clientTcp->GetAddress();
	common->Printf("Automation received incoming connection from %d.%d.%d.%d:%d\n",
		int(addr.ip[0]), int(addr.ip[1]), int(addr.ip[2]), int(addr.ip[3]), int(addr.port)
	);

	connection.Init(std::move(clientTcp));
	return true;
}

void Automation::Think() {
	//make sure log file is enabled, so that automation can fetch console messages
	if (com_logFile.GetInteger() == 0) {
		common->Printf("Forcing com_logFile to 1 for automation to work properly");
		com_logFile.SetInteger(1);
	}

	//if not yet listening, open tcp port and start listening
	if (!TryListen())
		return;

	//if not connected yet, check for incoming client
	if (!TryConnect())
		return;

	//push data from output buffer, pull into input buffer
	connection.Think();

	//handle any messages send from client
	idList<char> message;
	while (connection.ReadMessage(message))
		ParseMessage(message.Ptr(), message.Num());
}

void Auto_Think() {
	automation->Think();
}

bool Auto_GetUsercmd(usercmd_t &cmd) {
	return automation->GetUsercmd(cmd);
}
