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

#ifndef AUTOMATION_LOCAL_H
#define AUTOMATION_LOCAL_H

#include "Automation.h"
#include "../Http/MessageTcp.h"


//represents single-valued cubic function defined on span [startParam .. endParam]
//and having specified values and derivatives at the ends of the span
struct CubicSpan {
	float startParam, endParam;
	float startValue, endValue;
	float startDeriv, endDeriv;
};
//represents time moment when specified impulse should be triggered
struct ImpulseMoment {
	float param;
	int impulse;
};
//represents a detailed plan about how player must behave in the nearest future
//it defines value of usercmd_t at any time moment in the range of the plan
//almost every option possible in usercmd_t can behave as piecewise-cubic function of time
struct GameplayControlPlan {
	enum TimeMode {
		tmNone = 0,			//plan is invalid / dead currently
		tmAstronomical,		//all time moments are specified in real astronomical time
		tmGamePhysics,		//all time moments are specified in gameplay time (used by physics)
	};
	TimeMode timeMode = tmNone;

	enum AngleMode {
		amNone = 0,			//not specified
		amAbsolute,			//viewangles are set as absolute values
		amRelative,			//viewangles are set relative to initial values
		amSmooth,			//viewangles are set as absolute, but first span is modified for smooth joining
	};
	AngleMode angleMode = amNone;

	//in each of the arrays below, stuff is sorted by time !decreasing!

	idList<CubicSpan> buttons[8];	//[attack, run, zoom, scores, mlook, ...]
	idList<CubicSpan> moves[3];		//[forward, right, up]
	idList<CubicSpan> angles[3];	//[pitch(up/down), yaw(right/left), roll]
	idList<ImpulseMoment> impulse;
	//short mx, my;					//in my opinion, no gameplay should rely on raw values

	bool IsAlive() const { return timeMode != tmNone; }
	void Kill() { timeMode = tmNone; }
	void Clear();
	void Finalize();
	bool RemoveExpired(float nowTime);
	bool GetUsercmd(float nowTime, usercmd_t &cmd);
	int GetTimeNow() const;			//in msec
};


/**
 * Allows controlling TDM game from external tool.
 * Used in automated testing framework.
 */
class Automation {
public:
	void PushInputEvents(int key, int value);
	void ExecuteInGameConsole(const char *command, bool blocking = true);
	bool GetUsercmd(usercmd_t &cmd);

	void Think();

private:
	//low-level TCP socket connection with automation script
	//we can get/put bytes from/to it =)
	idTCP listenTcp;
	//we can receive messages and send messages using this thing =)
	MessageTcp connection;

	//the plan controlling player's actions in the near future
	//usually completely overrides user's input
	GameplayControlPlan gameControlPlan;
	//seqno of request which posted the plan (used to send request back)
	int gameControlPlan_seqno = -1;
	//time moment when the current plan was started (used to understand where we are in the plan)
	int gameControlPlan_time = -1;

	//info about current input message being parsed
	struct ParseIn {
		const char *message;
		int len;
		idLexer lexer;
		int seqno;
	};

	bool TryListen();
	bool TryConnect();
	void ParseMessage(const char *message, int len);
	void ParseMessage(ParseIn &parseIn);
	void ParseAction(ParseIn &parseIn);
	void ParseQuery(ParseIn &parseIn);
	void WriteResponse(int seqno, const char *response);
	void WriteGameControlResponse(const char *message);
};

#endif
