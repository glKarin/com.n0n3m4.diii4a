#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Player.h"

#include "bc_emailflag.h"

CLASS_DECLARATION(idAnimatedEntity, idEmailflag)
END_CLASS

#define CHECKTIME 500

#define FLAGTYPE_CRITICAL	1
#define FLAGTYPE_UNREAD		2



idEmailflag::idEmailflag(void)
{
	state = UNDEPLOYED;
	checkTimer = gameLocal.time + 400;
	idleTimer = 0;
	locbox = nullptr;
}

idEmailflag::~idEmailflag(void)
{
}

void idEmailflag::Spawn(void)
{
	flagType = spawnArgs.GetInt("flagtype");

	if (flagType != FLAGTYPE_CRITICAL && flagType != FLAGTYPE_UNREAD)
	{
		gameLocal.Error("emailflag: invalid flag type '%d'", flagType);
		return;
	}

	Event_PlayAnim("undeployed_idle", 0);

	fl.takedamage = false;
	BecomeActive(TH_THINK);

	idleTimer = gameLocal.time + 2000;

	//BC 3-22-2025: locbox for emailflag.
	idVec3 forward, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	idVec3 locboxPos = GetPhysics()->GetOrigin() + (forward * 10) + (up * -2);
	gameRenderWorld->DebugArrowSimple(locboxPos, 9000000);

	#define LOCBOXRADIUS 2.5f
	idDict args;
	args.Clear();
	args.Set("text", spawnArgs.GetString("displayname"));
	args.SetVector("origin", locboxPos);
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	locbox = static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));
	locbox->Hide();
}

void idEmailflag::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state

	savefile->WriteInt( checkTimer ); //  int checkTimer
	savefile->WriteInt( flagType ); //  int flagType

	savefile->WriteInt( idleTimer ); //  int idleTimer

	savefile->WriteObject( locbox ); // idEntity* locbox 
}

void idEmailflag::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state

	savefile->ReadInt( checkTimer ); //  int checkTimer
	savefile->ReadInt( flagType ); //  int flagType

	savefile->ReadInt( idleTimer ); //  int idleTimer

	savefile->ReadObject( locbox ); // idEntity* locbox 
}

void idEmailflag::Think(void)
{
	if (gameLocal.time > checkTimer)
	{
		//how often to poll email status
		checkTimer = gameLocal.time + CHECKTIME;

		//for debug text...
		idVec3 forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL,NULL);
		idVec3 textPos = GetPhysics()->GetOrigin() + forward * 8;

		//check the email status (how many emails are unread/critical)
		int emailCount = 0;		
		if (flagType == FLAGTYPE_CRITICAL)
		{
			//gameRenderWorld->DrawText(idStr::Format("critical emails = %d", gameLocal.GetLocalPlayer()->HasEmailsCritical()).c_str(), textPos, .05f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 500);
			emailCount = gameLocal.GetLocalPlayer()->HasEmailsCritical();
		}
		else if (flagType == FLAGTYPE_UNREAD)
		{
			//gameRenderWorld->DrawText(idStr::Format("unread emails = %d", gameLocal.GetLocalPlayer()->HasEmailsUnread()).c_str(), textPos, .05f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 500);
			emailCount = gameLocal.GetLocalPlayer()->HasEmailsUnread();
		}

		//If the state has changed, then play the appropriate animation.
		int desiredState = (emailCount > 0) ? DEPLOYED : UNDEPLOYED;
		if (desiredState != state)
		{
			state = desiredState;
			Event_PlayAnim((state == DEPLOYED) ? "deploy" : "undeploy", 1, false);

			if (desiredState == DEPLOYED)
			{
				locbox->Show();
			}
			else
			{
				locbox->Hide();
			}
		}		
	}

	if (state == DEPLOYED && gameLocal.time > idleTimer)
	{
		idleTimer = gameLocal.time + gameLocal.random.RandomInt(3000, 7000);

		Event_PlayAnim("deployed_idle", 1); //Do a little idle wiggle.
	}

	idAnimatedEntity::Think();
}
