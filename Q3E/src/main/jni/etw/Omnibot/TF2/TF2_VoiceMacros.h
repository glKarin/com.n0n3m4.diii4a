////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF2_VOICEMACROS_H__
#define __TF2_VOICEMACROS_H__

class Client;

typedef enum
{
	VCHAT_NONE,

	// menu 1
	VCHAT_MENU0_START,
	VCHAT_MEDIC,
	VCHAT_THANKS,
	VCHAT_GO,
	VCHAT_MOVEUP,
	VCHAT_FLANK_LEFT,
	VCHAT_FLANK_RIGHT,
	VCHAT_YES,
	VCHAT_NO,
	

	// menu 2
	VCHAT_MENU1_START,
	VCHAT_INCOMING,
	VCHAT_SPY,
	VCHAT_SENTRY_AHEAD,
	VCHAT_TELEPORTER_HERE,
	VCHAT_DISPENSER_HERE,
	VCHAT_SENTRY_HERE,
	VCHAT_ACTIVATE_UBERCHARGE,
	VCHAT_UBERCHARGE_READY,

	// menu 3
	VCHAT_MENU2_START,
	VCHAT_HELP,
	VCHAT_BATTLECRY,
	VCHAT_CHEERS,
	VCHAT_JEERS,
	VCHAT_POSITIVE,
	VCHAT_NEGATIVE,
	VCHAT_NICESHOT,
	VCHAT_GOODJOB,

	// This must stay last.
	NUM_TF2_VCHATS
} eVChat;

// class: TF2_VoiceMacros
class TF2_VoiceMacros
{
public:

	//static int GetVChatId(const char *_string);
	static void SendVoiceMacro(Client *_bot, int _msg);

protected:
};

#endif
