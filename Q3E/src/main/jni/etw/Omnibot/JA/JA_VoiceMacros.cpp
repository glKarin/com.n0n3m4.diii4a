////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompJA.h"
#include "JA_VoiceMacros.h"

const char *strVoiceMacros[] =
{
	NULL,

	"att_attack",
	"att_primary",
	"att_second",

	"def_guns",
	"def_position",
	"def_primary",
	"def_second",

	"reply_coming",
	"reply_go",
	"reply_no",
	"reply_stay",
	"reply_yes",

	"req_assist",
	"req_demo",
	"req_hvy",
	"req_medic",
	"req_sup",
	"req_tech",

	"spot_air",
	"spot_def",
	"spot_emplaced",
	"spot_sniper",
	"spot_troops",

	"tac_cover",
	"tac_fallback",
	"tac_follow",
	"tac_hold",
	"tac_split",
	"tac_together",

	NULL,

	NULL,
};

namespace JA_VoiceChatAssertions 
{
	// Make sure our enum size at least matches the size of our array.
	BOOST_STATIC_ASSERT((sizeof(strVoiceMacros) / sizeof(strVoiceMacros[0])) == NUM_JA_VCHATS);
}

int JA_VoiceMacros::GetVChatId(const char *_string)
{
	// Search for the string and return the index.
	for(int i = VCHAT_NONE; i < NUM_JA_VCHATS; ++i)
	{
		if(strVoiceMacros[i] && !strcmp(strVoiceMacros[i], _string))
			return i;
	}
	return VCHAT_NONE;
}

//FIXME: use vsay_team or voice_cmd (?)
void JA_VoiceMacros::SendVoiceMacro(Client *_bot, int _msg)
{
	static char buffer[512] = {0};
	if((_msg < VCHAT_TEAM_NUMMESSAGES) && (_msg > VCHAT_NONE))
	{
		sprintf(buffer, "vsay_team %s", strVoiceMacros[_msg]);
		g_EngineFuncs->BotCommand(_bot->GetGameID(), buffer);
	}
	/*else if((_msg < VCHAT_GLOBAL_NUMMESSAGES) && (_msg > VCHAT_TEAM_NUMMESSAGES))
	{ // no global chatting for now
		sprintf(buffer, "vsay %s", strVoiceMacros[_msg]);
		g_EngineFuncs->BotCommand(_bot->GetGameID(), buffer);
	}*/
}
