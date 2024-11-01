////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#include "PrecompTF2.h"
#include "TF2_VoiceMacros.h"

//int TF2_VoiceMacros::GetVChatId(const char *_string)
//{
//	// Search for the string and return the index.
//	for(int i = VCHAT_NONE; i < NUM_ET_VCHATS; ++i)
//	{
//		if(strVoiceMacros[i] && strcmp(strVoiceMacros[i], _string)==0)
//			return i;
//	}
//	return VCHAT_NONE;
//}

void TF2_VoiceMacros::SendVoiceMacro(Client *_bot, int _msg)
{
	if(_msg > VCHAT_NONE && _msg < NUM_TF2_VCHATS)
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
