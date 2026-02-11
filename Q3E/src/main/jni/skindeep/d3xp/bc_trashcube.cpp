#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Fx.h"

#include "bc_trashcube.h"

CLASS_DECLARATION(idMover, idTrashcube)

END_CLASS

idTrashcube::idTrashcube(void)
{
	//fl.takedamage = false;
	isDead = false;
}

idTrashcube::~idTrashcube(void)
{
}

void idTrashcube::Spawn(void)
{
}

void idTrashcube::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( isDead ); // bool isDead
}

void idTrashcube::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( isDead ); // bool isDead
}
	
void idTrashcube::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idVec3 fxPos;

	if (isDead)
		return;

	isDead = true;
	//Play a low rumble sound of the cube disintegrating into nothingness.
	StartSound("snd_shatter", SND_CHANNEL_BODY, 0, false, NULL);	

	fxPos = GetPhysics()->GetOrigin();
	idEntityFx::StartFx("fx/trash_shatter", &fxPos, &mat3_identity, NULL, false);

	//Burnaway fx.
	renderEntity.noShadow = true;
	renderEntity.shaderParms[SHADERPARM_TIME_OF_DEATH] = gameLocal.time * 0.001f;
	UpdateVisuals();

	PostEventMS(&EV_Remove, 700); //Not a great solution, but give it a delay so the fish have time to unbind from cube and swim away. If the delay isn't long enough, the fish get removed :(
}

