#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"


#include "bc_notewall.h"

const idEventDef EV_GetNoteRead("getNoteRead", NULL, 'd');

CLASS_DECLARATION(idStaticEntity, idNoteWall)
	EVENT(EV_GetNoteRead, idNoteWall::Event_GetNoteRead)
END_CLASS

idNoteWall::idNoteWall(void)
{
	isRead = false;
	isMemorypalaceClone = false;
	markedDone = false;

	memorypalaceNode.SetOwner(this);
	memorypalaceNode.AddToEnd(gameLocal.memorypalaceEntities);
}

idNoteWall::~idNoteWall(void)
{
	memorypalaceNode.Remove();
}

void idNoteWall::Spawn(void)
{
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
	isFrobbable = spawnArgs.GetBool("frobbable", "1");
	BecomeInactive(TH_THINK);
	fl.takedamage = false;
	SetColor(1, 1, 1);
}

void idNoteWall::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( isRead ); // bool isRead
	savefile->WriteBool( isMemorypalaceClone ); // bool isMemorypalaceClone

	savefile->WriteBool( markedDone ); // bool markedDone
}

void idNoteWall::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( isRead ); // bool isRead
	savefile->ReadBool( isMemorypalaceClone ); // bool isMemorypalaceClone

	savefile->ReadBool( markedDone ); // bool markedDone
}


bool idNoteWall::DoFrob(int index, idEntity * frobber)
{
	//Only player can frob.
	if (frobber != NULL)
	{
		if (frobber == gameLocal.GetLocalPlayer())
		{
			gameLocal.GetLocalPlayer()->DoZoominspectEntity(this);
			SetRead(); //BC 5-8-2025: setread() now happens AFTER the zoom, so that the "memory palace notification" works properly.
		}
	}

	return true;
}

void idNoteWall::SetRead()
{
	if (!isRead)
	{

		if (spawnArgs.GetBool("do_memorypalace", "1")) //BC 2-10-2025: make some notes ignore the memory palace.
		{
			gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("noteAddedToLog");

			if (!isMemorypalaceClone)
			{
				//Create the memory palace note clone.
				idEntity* noteClone = NULL;
				idDict args;
				args.Clear();
				args.Set("classname", spawnArgs.GetString("classname"));
				args.Set("gui_parm0", spawnArgs.GetString("gui_parm0"));
				args.SetFloat("gui_parm1", spawnArgs.GetFloat("gui_parm1"));
				args.Set("gui_parm2", spawnArgs.GetString("gui_parm2"));
				args.Set("model", spawnArgs.GetString("model"));

				// SW 3rd April 2025: To avoid the camera clipping into the wall, 
				// zoom-inspecting memory palace notes is done exclusively via FOV reduction.
				// Please ensure that the below `zoominspect_campos` offset is equivalent to 
				// the FORWARD_DISTANCE defined in idPlayer::DoMemoryPalace().
				// The goal is to prevent the camera from physically moving in space.
				args.SetVector("zoominspect_campos", idVec3(30, 0, 0));
				args.SetFloat("zoominspect_fov", spawnArgs.GetFloat("zoominspect_memory_fov", "70"));

				gameLocal.SpawnEntityDef(args, &noteClone);
				if (noteClone)
				{
					if (noteClone->IsType(idNoteWall::Type))
					{
						static_cast<idNoteWall*>(noteClone)->SetMemorypalaceClone();
					}
				}
			}
		}

		isRead = true;
		SetColor(0, 0, 0);
	}
}

void idNoteWall::SetMemorypalaceClone()
{
	isMemorypalaceClone = true;
	Hide();
	renderEntity.weaponDepthHack = true;
	SetColor(0, 0, 0);
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_memory")));
}

bool idNoteWall::IsMemorypalaceClone()
{
	return isMemorypalaceClone;
}

//Called when the note is summoned/dismissed via memory palace.
void idNoteWall::ShowMemoryPalace(bool value)
{
	if (value)
	{
		//Show.
		Show();
		GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
		GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);
		fl.drawGlobally = true;
		UpdateVisuals();
	}
	else
	{
		//hide.
		Hide();
	}

	gameLocal.DoParticle(spawnArgs.GetString("model_memoryparticle"), GetPhysics()->GetOrigin());
}

void idNoteWall::Event_GetNoteRead()
{
	idThread::ReturnFloat(GetRead());
}

bool idNoteWall::GetRead()
{
	return isRead;
}

void idNoteWall::SetMarkDoneToggle()
{
	markedDone = !markedDone;

	StartSound("snd_markdone", SND_CHANNEL_ANY);
	
	SetSkin(declManager->FindSkin(spawnArgs.GetString(markedDone ? "skin_memory_done" : "skin_memory")));

	// SW 3rd April 2025: darkens gui for self-illuminated notes (e.g. pirate/crew tablets)
	Event_GuiNamedEvent(1, markedDone ? "lowerShade" : "raiseShade");

	UpdateVisuals();
	Think(); //since we're in gamelocal menupause, we force it to think in order to update the skin change.
}

bool idNoteWall::GetMarkedDone()
{
	return markedDone;
}