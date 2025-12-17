//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//=============================================================================

// No spaces in event names, max length 32
// All strings are case sensitive
// total game event byte length must be < 1024
//
// valid data key types are:
//   none   : value is not networked
//   string : a zero terminated string
//   bool   : unsigned int, 1 bit
//   byte   : unsigned int, 8 bit
//   short  : signed int, 16 bit
//   long   : signed int, 32 bit
//   float  : float, 32 bit



"gameevents"
{
	"team_info"				// info about team
	{
		"teamid"	"byte"		// unique team id
		"teamname"	"string"	// team name eg "Team Blue"
	}
	
	"team_score"				// team score changed
	{
		"teamid"	"byte"		// team id
		"score"		"short"		// total team score
	}
	
//////////////////////////////////////////////////////////////////////
// Player events
//////////////////////////////////////////////////////////////////////
	
	"player_team"				// player change his team
	{
		"userid"	"short"		// user ID on server
		"team"		"byte"		// team id
		"oldteam" "byte"		// old team id
		"disconnect" "bool"	// team change because player disconnects
	}
	
	"player_class"				// a player changed his class
	{
		"userid"	"short"		// user ID on server
		"class"		"string"	// new player class / model
	}
	
	"player_death"				// a game event, name may be 32 charaters long
	{
		"userid"	"short"   	// user ID who died				
		"attacker"	"short"	 	// user ID who killed
	}
	
	"player_hurt"
	{
		"userid"	"short"   	// player index who was hurt				
		"attacker"	"short"	 	// player index who attacked
		"health"	"byte"		// remaining health points
	}
	
	"player_chat"				// a public player chat
	{
		"teamonly"	"bool"		// true if team only chat
		"userid" 	"short"		// chatting player 
		"text" 	 	"string"	// chat text
	}
	
	"player_score"				// players scores changed
	{
		"userid"	"short"		// user ID on server
		"kills"		"short"		// # of kills
		"deaths"	"short"		// # of deaths
		"score"		"short"		// total game score
	}
	
	"player_spawn"				// player spawned in game
	{
		"userid"	"short"		// user ID on server
	}
	
	"player_shoot"				// player shoot his weapon
	{
		"userid"	"short"		// user ID on server
		"weapon"	"byte"		// weapon ID
		"mode"		"byte"		// weapon mode
	}
	
	"player_use"
	{
		"userid"	"short"		// user ID on server
		"entity"	"short"		// entity used by player
	}

	"player_changename"
	{
		"userid"	"short"		// user ID on server
		"oldname"	"string"	// players old (current) name
		"newname"	"string"	// players new name
	}

//////////////////////////////////////////////////////////////////////
// Game events
//////////////////////////////////////////////////////////////////////
		
	"game_newmap"				// send when new map is completely loaded
	{
		"mapname"	"string"	// map name
	}
	
	"game_start"				// a new game starts
	{
		"roundslimit"	"long"		// max round
		"timelimit"	"long"		// time limit
		"fraglimit"	"long"		// frag limit
		"objective"	"string"	// round objective
	}
	
	"game_end"				// a game ended
	{
		"winner"	"byte"		// winner team/user id
	}
	
	"round_start"
	{
		"timelimit"	"long"		// round time limit in seconds
		"fraglimit"	"long"		// frag limit in seconds
		"objective"	"string"	// round objective
	}
	
	"round_end"
	{
		"winner"	"byte"		// winner team/user i
		"reason"	"byte"		// reson why team won
		"message"	"string"	// end round message 
	}
	
	"game_message"				// a message send by game logic to everyone
	{
		"target"	"byte"		// 0 = console, 1 = HUD
		"text"		"string"	// the message text
	}

	"break_breakable"
	{
		"entindex"	"long"
		"userid"		"short"
		"material"	"byte"	// BREAK_GLASS, BREAK_WOOD, etc
	}

	"break_prop"
	{
		"entindex"	"long"
		"userid"	"short"
	}
}