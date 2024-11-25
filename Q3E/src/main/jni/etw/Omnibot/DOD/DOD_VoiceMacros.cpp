////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompDOD.h"
#include "DOD_VoiceMacros.h"

//int DOD_VoiceMacros::GetVChatId(const char *_string)
//{
//	// Search for the string and return the index.
//	for(int i = VCHAT_NONE; i < NUM_ET_VCHATS; ++i)
//	{
//		if(strVoiceMacros[i] && strcmp(strVoiceMacros[i], _string)==0)
//			return i;
//	}
//	return VCHAT_NONE;
//}

void DOD_VoiceMacros::SendVoiceMacro(Client *_bot, int _msg)
{
	if(_msg > VCHAT_NONE && _msg < NUM_DOD_VCHATS)
	{
		int menu = 0;
		int macro = 0;
		if(_msg > VCHAT_MENU1_START)
		{
			macro = _msg - VCHAT_MENU1_START;
			menu = 1;
		}
		if(_msg > VCHAT_MENU2_START)
		{
			macro = _msg - VCHAT_MENU2_START;
			menu = 2;
		}
		g_EngineFuncs->BotCommand(_bot->GetGameID(), va("voicemenu %d %d",menu,macro));
	}
}
