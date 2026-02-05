#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"

#include "bc_frobcube.h"

CLASS_DECLARATION(idStaticEntity, idFrobcube)
END_CLASS

idFrobcube::idFrobcube(void)
{
}

idFrobcube::~idFrobcube(void)
{
}

void idFrobcube::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = spawnArgs.GetBool("frobbable", "1");

	index = 0;
}

void idFrobcube::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(index); // int index
}

void idFrobcube::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt(index); // int index
}

void idFrobcube::SetIndex(int value)
{
	index = value;
}

//When the frob cube is frobbed.....
bool idFrobcube::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL)
	{
		return false;
	}

	if (this->GetPhysics()->GetClipModel()->GetOwner() != NULL)
	{
		this->GetPhysics()->GetClipModel()->GetOwner()->DoFrob(this->index, frobber);
	}
	else
	{
		idStr scriptName = spawnArgs.GetString("call", "");
		if (scriptName.Length() > 0)
		{
			const function_t	*scriptFunction;
			scriptFunction = gameLocal.program.FindFunction(scriptName);
			if (scriptFunction)
			{
				assert(scriptFunction->parmSize.Num() <= 2);
				idThread* thread = new idThread();

				// 1 or 0 args always pushes activator (frobber)
				thread->PushArg(frobber);
				// 2 args pushes self as well
				if (scriptFunction->parmSize.Num() == 2)
				{
					thread->PushArg(this);
				}

				thread->CallFunction(scriptFunction, false);
				thread->DelayedStart(0);
			}
			else
			{
				common->Printf("frobcube unable to find function '%s'", scriptName.c_str());
			}
		}
	}


	StartSound("snd_frob", SND_CHANNEL_ANY, 0, false, NULL);	
	return true;
}

void idFrobcube::Show(void)
{
	idEntity::Show();

	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
}

void idFrobcube::SetPostFlag(customPostFlag_t flag, bool enable)
{
	idEntity::SetPostFlag(flag, enable);

	// Also set the post flag on the owner
	if (GetPhysics()->GetClipModel()->GetOwner())
	{
		GetPhysics()->GetClipModel()->GetOwner()->SetPostFlag(flag, enable);
	}
}
