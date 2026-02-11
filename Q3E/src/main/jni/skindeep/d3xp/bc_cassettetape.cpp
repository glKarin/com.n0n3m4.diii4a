#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_cassettetape.h"


CLASS_DECLARATION(idStaticEntity, idCassetteTape)
END_CLASS

idCassetteTape::idCassetteTape(void)
{
}

idCassetteTape::~idCassetteTape(void)
{
}

void idCassetteTape::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);

	isFrobbable = true;
	fl.takedamage = false; //let's make our lives easier and make cassette tapes invincible

	tapeIndex = spawnArgs.GetInt("tapeindex");
}

void idCassetteTape::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( tapeIndex ); //  int tapeIndex
}

void idCassetteTape::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( tapeIndex ); //  int tapeIndex
}

void idCassetteTape::Think(void)
{
	idStaticEntity::Think();
}

bool idCassetteTape::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
	{
		return false;
	}

	if (frobber != gameLocal.GetLocalPlayer()) //Only player can frob.
	{
		return false;
	}

	//Give this cassette tape to player's persistent inventory.

	idStr scriptName;
	if (spawnArgs.GetString("call", "", scriptName))
	{
		gameLocal.RunMapScript(scriptName);
	}

	//common->Printf("PICKED UP TAPE INDEX: '%d'\n", tapeIndex);
	gameLocal.GetLocalPlayer()->PickupTape(tapeIndex);

	gameLocal.AddEventLog("#str_def_gameplay_foundcassette", GetPhysics()->GetOrigin());

	//BC 3-6-2025: added pickup effect.
	idEntityFx::StartFx(spawnArgs.GetString("fx_pickup"), GetPhysics()->GetOrigin(), mat3_identity);	

	Hide();
	PostEventMS(&EV_Remove, 0);
	return true;
}

