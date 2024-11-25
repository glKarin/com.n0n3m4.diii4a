#include "PrecompET.h"
#include "ET_VoiceMacros.h"

const char *strVoiceMacros[] =
{
	NULL,

	// Execute these using VoiceTeamChat
	"PathCleared",
	"EnemyWeak",
	"AllClear",
	"Incoming",
	"FireInTheHole",
	"OnDefense",
	"OnOffense",
	"TakingFire",
	"MinesCleared",
	"EnemyDisguised",

	"Medic",
	"NeedAmmo",
	"NeedBackup",
	"NeedEngineer",
	"CoverMe",
	"HoldFire",
	"WhereTo",
	"NeedOps",

	"FollowMe",
	"LetsGo",
	"Move",
	"ClearPath",
	"DefendObjective",
	"DisarmDynamite",
	"ClearMines",
	"ReinforceOffense",
	"ReinforceDefense",

	"Affirmative",
	"Negative",
	"Thanks",
	"Welcome",
	"Sorry",
	"Oops",

	"CommandAcknowledged",
	"CommandDeclined",
	"CommandCompleted",
	"DestroyPrimary",
	"DestroySecondary",
	"DestroyConstruction",
	"ConstructionCommencing",
	"RepairVehicle",
	"DestroyVehicle",
	"EscortVehicle",

	"IamSoldier",
	"IamMedic",
	"IamEngineer",
	"IamFieldOps",
	"IamCovertOps",

	NULL,

	// Execute with VoiceChat
	"Affirmative",
	"Negative",
	"EnemyWeak",
	"Hi",
	"Bye",
	"GreatShot",
	"Cheer",
	"Thanks",
	"Welcome",
	"Oops",
	"Sorry",
	"HoldFire",
	"GoodGame",

	NULL,
	
	"FTMortar",
	"FTHealSquad",
	"FTHealMe",
	"FTReviveTeamMate",
	"FTReviveMe",
	"FTDestroyObjective",
	"FTRepairObjective",
	"FTConstructObjective",
	"FTDeployLandmines",
	"FTDisarmLandmines",
	"FTCallAirStrike",
	"FTCallArtillery",
	"FTResupplySquad",
	"FTResupplyMe",
	"FTExploreArea",
	"FTCheckLandmines",
	"FTSatchelObjective",
	"FTInfiltrate",
	"FTGoUndercover",
	"FTProvideSniperCover",
	"FTAttack",
	"FTFallBack",

	NULL
	//"wm_sayPlayerClass",
};

namespace ET_VoiceChatAssertions 
{
	// Make sure our enum size at least matches the size of our array.
	BOOST_STATIC_ASSERT((sizeof(strVoiceMacros) / sizeof(strVoiceMacros[0])) == NUM_ET_VCHATS);
}

int ET_VoiceMacros::GetVChatId(const char *_string)
{
	// Search for the string and return the index.
	for(int i = VCHAT_NONE; i < NUM_ET_VCHATS; ++i)
	{
		if(strVoiceMacros[i] && strcmp(strVoiceMacros[i], _string)==0)
			return i;
	}
	return VCHAT_NONE;
}

void ET_VoiceMacros::SendVoiceMacro(Client *_bot, int _msg)
{
	static char buffer[512];
	if((_msg < VCHAT_TEAM_NUMMESSAGES) && (_msg > VCHAT_NONE))
	{
		sprintf(buffer, "vsay_team %s", strVoiceMacros[_msg]);
		g_EngineFuncs->BotCommand(_bot->GetGameID(), buffer);
	}
	else if((_msg < VCHAT_GLOBAL_NUMMESSAGES) && (_msg > VCHAT_TEAM_NUMMESSAGES))
	{
		sprintf(buffer, "vsay %s", strVoiceMacros[_msg]);
		g_EngineFuncs->BotCommand(_bot->GetGameID(), buffer);
	}
}

//////////////////////////////////////////////////////////////////////////
//QM_MENU_START( "wm_quickstatements" )
//QM_MENU_ITEM_TEAM( "P. Path Cleared.", 	exec "VoiceTeamChat PathCleared"; 		close wm_quickstatements,	"p", 0 )
//QM_MENU_ITEM_TEAM( "W. Enemy Weak!", 		exec "VoiceTeamChat EnemyWeak"; 		close wm_quickstatements, 	"w", 1 )
//QM_MENU_ITEM_TEAM( "C. All Clear", 		exec "VoiceTeamChat AllClear"; 			close wm_quickstatements,	"c", 2 )
//QM_MENU_ITEM_TEAM( "I. Incoming", 		exec "VoiceTeamChat Incoming"; 			close wm_quickstatements,	"i", 3 )
//QM_MENU_ITEM_TEAM( "F. Fire In The Hole!",exec "VoiceTeamChat FireInTheHole";		 close wm_quickstatements,	"f", 4 )
//QM_MENU_ITEM_TEAM( "D. I'm Defending.", 	exec "VoiceTeamChat OnDefense"; 		close wm_quickstatements,	"d", 5 )
//QM_MENU_ITEM_TEAM( "A. I'm Attacking.", 	exec "VoiceTeamChat OnOffense"; 		close wm_quickstatements,	"a", 6 )
//QM_MENU_ITEM_TEAM( "T. Taking Fire!", 	exec "VoiceTeamChat TakingFire"; 		close wm_quickstatements,	"t", 7 )
//QM_MENU_ITEM_TEAM( "M. Mines Cleared", 	exec "VoiceTeamChat MinesCleared"; 		close wm_quickstatements,	"m", 8 )
//QM_MENU_ITEM_TEAM( "E. Enemy Disguised", 	exec "VoiceTeamChat EnemyDisguised";	close wm_quickstatements,	"e", 9 )
//QM_MENU_END
//QM_MENU_START( "wm_quickrequests" )
//QM_MENU_ITEM_TEAM( "M. Need Medic!", 		exec "VoiceTeamChat Medic"; 		close wm_quickrequests,	"m", 0 )
//QM_MENU_ITEM_TEAM( "A. Need Ammo!", 		exec "VoiceTeamChat NeedAmmo"; 		close wm_quickrequests,	"a", 1 )
//QM_MENU_ITEM_TEAM( "B. Need Backup!", 	exec "VoiceTeamChat NeedBackup"; 	close wm_quickrequests,	"b", 2 )
//QM_MENU_ITEM_TEAM( "E. Need Engineer!", 	exec "VoiceTeamChat NeedEngineer"; 	close wm_quickrequests,	"e", 3 )
//QM_MENU_ITEM_TEAM( "C. Cover Me!", 		exec "VoiceTeamChat CoverMe"; 		close wm_quickrequests,	"c", 4 )
//QM_MENU_ITEM_TEAM( "H. Hold Fire!",		exec "VoiceTeamChat HoldFire";	 	close wm_quickrequests, "h", 5 )
//QM_MENU_ITEM_TEAM( "W. Where To?", 		exec "VoiceTeamChat WhereTo"; 		close wm_quickrequests,	"w", 6 )
//QM_MENU_ITEM_TEAM( "O. Need Covert Ops!", exec "VoiceTeamChat NeedOps"; 		close wm_quickrequests,	"o", 7 )
//QM_MENU_END
//QM_MENU_START( "wm_quickcommand" )
//QM_MENU_ITEM_TEAM( "F. Follow Me!", 		exec "VoiceTeamChat FollowMe"; 			close wm_quickcommand, 	"f", 0 )
//QM_MENU_ITEM_TEAM( "G. Let's Go!", 		exec "VoiceTeamChat LetsGo"; 			close wm_quickcommand, 	"g", 1 )
//QM_MENU_ITEM_TEAM( "M. Move!", 			exec "VoiceTeamChat Move"; 				close wm_quickcommand, 	"m", 2 )
//QM_MENU_ITEM_TEAM( "C. Clear The Path!", 	exec "VoiceTeamChat ClearPath"; 		close wm_quickcommand, 	"c", 3 )
//QM_MENU_ITEM_TEAM( "O. Defend Objective!",exec "VoiceTeamChat DefendObjective"; 	close wm_quickcommand, 	"o", 4 )
//QM_MENU_ITEM_TEAM( "D. Disarm Dynamite!", exec "VoiceTeamChat DisarmDynamite"; 	close wm_quickcommand, 	"d", 5 )
//QM_MENU_ITEM_TEAM( "N. Clear Mines!", 	exec "VoiceTeamChat ClearMines"; 		close wm_quickcommand, 	"n", 6 )
//QM_MENU_ITEM_TEAM( "R. Reinforce Offense",exec "VoiceTeamChat ReinforceOffense"; 	close wm_quickcommand,	"r", 7 )
//QM_MENU_ITEM_TEAM( "E. Reinforce Defense",exec "VoiceTeamChat ReinforceDefense"; 	close wm_quickcommand,	"e", 8 )
//QM_MENU_END
//QM_MENU_START( "wm_quickmisc" )
//QM_MENU_ITEM_TEAM( "Y. Yes",			exec "VoiceTeamChat Affirmative"; 	close wm_quickmisc, "y", 0 )
//QM_MENU_ITEM_TEAM( "N. No",				exec "VoiceTeamChat Negative"; 		close wm_quickmisc, "n", 1 )
//QM_MENU_ITEM_TEAM( "T. Thanks",			exec "VoiceTeamChat Thanks"; 		close wm_quickmisc, "t", 2 )
//QM_MENU_ITEM_TEAM( "W. Welcome",		exec "VoiceTeamChat Welcome"; 		close wm_quickmisc, "w", 3 )
//QM_MENU_ITEM_TEAM( "S. Sorry",			exec "VoiceTeamChat Sorry"; 		close wm_quickmisc, "s", 4 )
//QM_MENU_ITEM_TEAM( "O. Oops", 			exec "VoiceTeamChat Oops"; 			close wm_quickmisc, "o", 5 )
//QM_MENU_END
//QM_MENU_START( "wm_quickglobal" )
//QM_MENU_ITEM( "Y. Yes",				exec "VoiceChat Affirmative"; 	close wm_quickglobal, 	"y", 0 )
//QM_MENU_ITEM( "N. No",				exec "VoiceChat Negative"; 		close wm_quickglobal, 	"n", 1 )
//QM_MENU_ITEM( "W. Enemy Weak",		exec "VoiceChat EnemyWeak";		close wm_quickglobal, 	"w", 2 )
//QM_MENU_ITEM( "H. Hi",				exec "VoiceChat Hi"; 			close wm_quickglobal, 	"h", 3 )
//QM_MENU_ITEM( "B. Bye",				exec "VoiceChat Bye"; 			close wm_quickglobal, 	"b", 4 )
//QM_MENU_ITEM( "S. Great Shot",		exec "VoiceChat GreatShot"; 	close wm_quickglobal, 	"s", 5 )
//QM_MENU_ITEM( "C. Cheer",				exec "VoiceChat Cheer"; 		close wm_quickglobal, 	"c", 6 )
//QM_MENU_ITEM( "G. More Globals",		close wm_quickglobal;			open wm_quickglobal2, 	"g", 7 )
//QM_MENU_END
//QM_MENU_START( "wm_quickglobal2" )
//QM_MENU_ITEM( "T. Thanks",		exec "VoiceChat Thanks";	close wm_quickglobal2, 	"t", 0 )
//QM_MENU_ITEM( "W. Welcome",		exec "VoiceChat Welcome"; 	close wm_quickglobal2, 	"w", 1 )
//QM_MENU_ITEM( "O. Oops",		exec "VoiceChat Oops"; 		close wm_quickglobal2, 	"o", 2 )
//QM_MENU_ITEM( "S. Sorry",		exec "VoiceChat Sorry"; 	close wm_quickglobal2, 	"s", 3 )
//QM_MENU_ITEM( "H. Hold Fire!",	exec "VoiceChat HoldFire";	close wm_quickglobal2, 	"h", 4 )
//QM_MENU_ITEM( "G. Good Game",	exec "VoiceChat GoodGame";	close wm_quickglobal2, 	"g", 5 )
//QM_MENU_END
//QM_MENU_START( "wm_quickobjectives" )
//QM_MENU_ITEM_TEAM( "A. Command Acknowledged",		exec "VoiceTeamChat CommandAcknowledged"; 		close wm_quickobjectives,	"a", 0 )
//QM_MENU_ITEM_TEAM( "D. Command Declined",			exec "VoiceTeamChat CommandDeclined";			close wm_quickobjectives,	"d", 1 )
//QM_MENU_ITEM_TEAM( "C. Command Completed",			exec "VoiceTeamChat CommandCompleted";			close wm_quickobjectives,	"c", 2 )
//QM_MENU_ITEM_TEAM( "P. Destroy Primary Objective",	exec "VoiceTeamChat DestroyPrimary"; 			close wm_quickobjectives,	"p", 3 )
//QM_MENU_ITEM_TEAM( "S. Destroy Secondary Objective",	exec "VoiceTeamChat DestroySecondary";		close wm_quickobjectives,	"s", 4 )
//QM_MENU_ITEM_TEAM( "X. Destroy Construction",		exec "VoiceTeamChat DestroyConstruction";		close wm_quickobjectives,	"x", 5 )
//QM_MENU_ITEM_TEAM( "M. Commencing Construction",	exec "VoiceTeamChat ConstructionCommencing";	close wm_quickobjectives, 	"m", 6 )
//QM_MENU_ITEM_TEAM( "R. Repair Vehicle",				exec "VoiceTeamChat RepairVehicle";				close wm_quickobjectives,	"r", 7 )
//QM_MENU_ITEM_TEAM( "V. Disable Vehicle",			exec "VoiceTeamChat DestroyVehicle";			close wm_quickobjectives,	"v", 8 )
//QM_MENU_ITEM_TEAM( "E. Escort Vehicle",				exec "VoiceTeamChat EscortVehicle";				close wm_quickobjectives,	"e", 9 )
//QM_MENU_END

//////////////////////////////////////////////////////////////////////////

//QM_MENU_START( "wm_nq_alt3" )	
//QM_MENU_ITEM_TEAM( "1. Deploy Mortar",		exec "VoiceFireteamChat FTMortar";		close wm_nq_alt3, "1", 0)
//QM_MENU_ITEM_TEAM( "2. Heal The Squad",		exec "VoiceFireteamChat FTHealSquad";		close wm_nq_alt3, "2", 1)
//QM_MENU_ITEM_TEAM( "3. Heal Me",			exec "VoiceFireteamChat FTHealMe";		close wm_nq_alt3, "3", 2)
//QM_MENU_ITEM_TEAM( "4. Revive Team Mate",	exec "VoiceFireteamChat FTReviveTeamMate";	close wm_nq_alt3, "4", 3)
//QM_MENU_ITEM_TEAM( "5. Revive Me",			exec "VoiceFireteamChat FTReviveMe";		close wm_nq_alt3, "5", 4)
//QM_MENU_ITEM_TEAM( "6. Destroy The Objective",	exec "VoiceFireteamChat FTDestroyObjective";	close wm_nq_alt3, "6", 5)
//QM_MENU_ITEM_TEAM( "7. Repair Objective",	exec "VoiceFireteamChat FTRepairObjective";	close wm_nq_alt3, "7", 6)
//QM_MENU_ITEM_TEAM( "8. Construct Objective",exec "VoiceFireteamChat FTConstructObjective";	close wm_nq_alt3, "8", 7)
//QM_MENU_ITEM_TEAM( "9. Deploy Landmines",	exec "VoiceFireteamChat FTDeployLandmines";	close wm_nq_alt3, "9", 8)
//QM_MENU_ITEM_TEAM( "0. Disarm Landmines",	exec "VoiceFireteamChat FTDisarmLandmines";	close wm_nq_alt3, "0", 9)
//QM_MENU_END
//
//QM_MENU_START( "wm_nq_alt4" )
//QM_MENU_ITEM_TEAM( "1. Call an Airstrike",		exec "VoiceFireteamChat FTCallAirStrike";	close wm_nq_alt4, "1", 0)
//QM_MENU_ITEM_TEAM( "2. Call in Artillery",		exec "VoiceFireteamChat FTCallArtillery";	close wm_nq_alt4, "2", 1)
//QM_MENU_ITEM_TEAM( "3. Resupply The Squad",		exec "VoiceFireteamChat FTResupplySquad";	close wm_nq_alt4, "3", 2)
//QM_MENU_ITEM_TEAM( "4. Resupply Me",			exec "VoiceFireteamChat FTResupplyMe";		close wm_nq_alt4, "4", 3)
//QM_MENU_ITEM_TEAM( "5. Explore The Area",		exec "VoiceFireteamChat FTExploreArea";		close wm_nq_alt4, "5", 4)
//QM_MENU_ITEM_TEAM( "6. Check for Landmines",		exec "VoiceFireteamChat FTCheckLandmines";	close wm_nq_alt4, "6", 5)
//QM_MENU_ITEM_TEAM( "7. Destroy The Objective",		exec "VoiceFireteamChat FTSatchelObjective";	close wm_nq_alt4, "7", 6)
//QM_MENU_ITEM_TEAM( "8. Infiltrate",			exec "VoiceFireteamChat FTInfiltrate";		close wm_nq_alt4, "8", 7)
//QM_MENU_ITEM_TEAM( "9. Go Undercover",			exec "VoiceFireteamChat FTGoUndercover";	close wm_nq_alt4, "9", 8)
//QM_MENU_ITEM_TEAM( "0. Provide Sniper Cover",		exec "VoiceFireteamChat FTProvideSniperCover";	close wm_nq_alt4, "0", 9

//////////////////////////////////////////////////////////////////////////