
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"



#define		HEIGHT_MAX		32
#define		FLYRADIUS		45


const idEventDef EV_TremblerReset(	"tremblerreset"  );
const idEventDef EV_TremblerArm(	"<TremblerArm>" );

CLASS_DECLARATION( idAnimatedEntity, idTrembler )
	
	EVENT( EV_Touch,			idTrembler::Event_Touch )
	EVENT( EV_TremblerReset,	idTrembler::Reset)
	EVENT( EV_TremblerArm,		idTrembler::Arm)

END_CLASS


void idTrembler::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteVec3(itemOriginalPosition);
	itemEnt.Save(savefile);
	savefile->WriteInt(ticksoundTime);
}

void idTrembler::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadVec3(itemOriginalPosition);
	itemEnt.Restore(savefile);
	savefile->ReadInt(ticksoundTime);
}

//state
//-1 = waiting for item to settle.
// 0 = armed.
// 1 = alarm tripped.

void idTrembler::Spawn( void )
{
	itemEnt = NULL;

	itemOriginalPosition = idVec3(0,0,0);
	state = -1;
	ticksoundTime = 0;
		
	Reset();

	BecomeActive( TH_THINK );
}

void idTrembler::Reset( void )
{
	StopSound( SND_CHANNEL_ANY, false );

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0.7f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1;
	UpdateVisuals();

	state = -1;

	int resettime = spawnArgs.GetInt("armtime");

	Event_PlayAnim("idle", 1);

	PostEventMS( &EV_TremblerArm, resettime ); //give a little delay to let physics settle the objects down.
}

void idTrembler::Arm( void )
{
	if ( !itemEnt.IsValid() )
	{
		idStr itemName = spawnArgs.GetString( "itemname", "" );
		if ( itemName.Length() )
		{
			itemEnt = gameLocal.FindEntity( itemName );

			if (!itemEnt.IsValid())
			{
				common->Warning("idTrembler %s couldn't find item '%s'.", name.c_str(), itemName.c_str());
			}
		}
		else
		{
			common->Warning("idTrembler %s has no 'itemname' value.", name.c_str());
		}
	}

	if (itemEnt.IsValid())
	{
		itemOriginalPosition = itemEnt.GetEntity()->GetPhysics()->GetOrigin();
	}

	state = 0;

	renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
	UpdateVisuals();
}

void idTrembler::Think( void )
{
	if (state == 0 && itemEnt.IsValid())
	{	
		idVec3 currentPosition = itemEnt.GetEntity()->GetPhysics()->GetOrigin();
		if (currentPosition.x != itemOriginalPosition.x || currentPosition.y != itemOriginalPosition.y
			|| currentPosition.z != itemOriginalPosition.z || itemEnt.GetEntity()->IsHidden())
		{
			Event_Touch(NULL,NULL);
		}
	}

	if (state == 1)
	{
		if (gameLocal.time > ticksoundTime)
		{
			StopSound( SND_CHANNEL_BODY2, false );
			ticksoundTime = gameLocal.time + 1000000;
		}
	}

	//Render the polys.
	idAnimatedEntity::Present();
}

void idTrembler::Event_Touch( idEntity *other, trace_t *trace )
{
	


	if (state == 0)
	{
		idEntity	*entityList[ MAX_GENTITIES ];
		int			i;
		int			flyforce;

		state = 1;

		Event_PlayAnim("bounce", 1);
		StartSound( "snd_trigger", SND_CHANNEL_BODY, 0, false, NULL );
		StartSound( "snd_trigger_idle", SND_CHANNEL_BODY2, 0, false, NULL );


		ticksoundTime = gameLocal.time + (spawnArgs.GetInt("ticktime", "10") * 1000);


		if (g_skill.GetInteger() > 0)
		{
			ActivateTargets(this); //activate alarm.
		}






		int listedEntities = gameLocal.EntitiesWithinRadius( GetPhysics()->GetOrigin(), FLYRADIUS, entityList, MAX_GENTITIES );

		flyforce = spawnArgs.GetInt("force", "256");

		renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
		renderEntity.shaderParms[ SHADERPARM_GREEN ] = 0;
		renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
		UpdateVisuals();



		//KA BOIIIIINNNGGGGGG
		for (i = 0; i < listedEntities; i++)
		{
			idEntity *ent = entityList[ i ];

			if (!ent)
			{
				continue;
			}

			float ent_zpos = ent->GetPhysics()->GetOrigin().z;
			float trembler_zpos = this->GetPhysics()->GetOrigin().z;

			//only affect things above the trembler.
			if (ent_zpos < trembler_zpos)
				continue;

			//only affect things within X height of trembler.
			if (ent_zpos > trembler_zpos + HEIGHT_MAX)
				continue;

			if ( !ent->IsType( idMoveableItem::Type ) && !ent->IsType( idMoveable::Type ) )
				continue;

			ent->GetPhysics()->SetLinearVelocity(idVec3(0,0,1) * flyforce);
		}
	}
}