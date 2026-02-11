#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "Moveable.h"

#include "bc_tutorialstation.h"



const int BLINKTIME = 2500;

CLASS_DECLARATION(idStaticEntity, idTutorialStation)
	EVENT(EV_Activate, idTutorialStation::Event_Activate)
END_CLASS

idTutorialStation::idTutorialStation(void)
{
}

idTutorialStation::~idTutorialStation(void)
{
}

void idTutorialStation::Spawn(void)
{
	tutState = TUTSTAT_IDLE;
	stateTimer = 0;
	idleSmoke = nullptr;

	StartSound("snd_ambient", SND_CHANNEL_BODY, 0, false, NULL);
	SetSkin(declManager->FindSkin("skins/objects/tutorialstation/default"));

	fl.takedamage = true;
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	BecomeActive(TH_THINK);
}

void idTutorialStation::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( tutState ); // bool tutState
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteObject( idleSmoke ); // idFuncEmitter * idleSmoke
}

void idTutorialStation::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( tutState ); // bool tutState
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadObject( CastClassPtrRef(idleSmoke) ); // idFuncEmitter * idleSmoke
}

void idTutorialStation::Think(void)
{	
	if (tutState == TUTSTAT_BLINKING)
	{
		if (gameLocal.time > stateTimer)
		{

			if (fl.takedamage)
			{
				SetSkin(declManager->FindSkin("skins/objects/tutorialstation/default"));
			}

			BecomeInactive(TH_THINK);
		}
	}

	idStaticEntity::Think();
}

void idTutorialStation::Event_Activate(idEntity *activator)
{
	idVec3 forward, right, up, sirenPos;

	if (tutState != TUTSTAT_IDLE)
		return;

	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	sirenPos = GetPhysics()->GetOrigin() + (right * -10) + (up * 128);

	
	tutState = TUTSTAT_BLINKING;
	StartSound("snd_jingle", SND_CHANNEL_VOICE, 0, false, NULL);
	gameLocal.DoParticle("music_longburst.prt", sirenPos);

	if (fl.takedamage)
	{
		SetSkin(declManager->FindSkin("skins/objects/tutorialstation/blink"));
	}

	SetColor(idVec4(0, .8f, 0, 1));

	Event_GuiNamedEvent(1, "ondone");

	stateTimer = gameLocal.time + BLINKTIME;
}

void idTutorialStation::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	idDict args;

	if (!fl.takedamage)
		return;

	fl.takedamage = false;

	//blow it up.
	SetSkin(declManager->FindSkin("skins/objects/tutorialstation/broken"));

	StartSound("snd_broken", SND_CHANNEL_BODY, 0, false, NULL);

	args.Clear();
	args.Set("model", "machine_damaged_smoke.prt");
	args.Set("start_off", "0");
	idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	idleSmoke->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0,0,60));
}