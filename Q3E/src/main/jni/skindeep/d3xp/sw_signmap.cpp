#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"

#include "sw_signmap.h"

CLASS_DECLARATION(idLight, idSignMap)
END_CLASS

idSignMap::idSignMap()
{

}

idSignMap::~idSignMap()
{

}

void idSignMap::Save(idSaveGame* savefile) const
{
}

void idSignMap::Restore(idRestoreGame* savefile)
{
}

void idSignMap::Spawn()
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = true;
}

bool idSignMap::DoFrob(int index, idEntity* frobber)
{
	//Only player can frob.
	if (frobber != NULL)
	{
		if (frobber == gameLocal.GetLocalPlayer())
		{
			gameLocal.GetLocalPlayer()->DoZoominspectEntity(this);
		}
	}

	return true;
}