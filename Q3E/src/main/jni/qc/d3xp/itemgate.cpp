
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_staticactivate(		"staticactivate", "d" );

CLASS_DECLARATION( idTrigger, idItemGate )
	
	EVENT( EV_Touch,			idItemGate::Event_Touch )
	EVENT( EV_staticactivate,	idItemGate::Event_staticactivate)

END_CLASS

void idItemGate::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(nextTriggerTime);
	savefile->WriteVec3(throwDir);
	savefile->WriteString(className);
	savefile->WriteString(weaponName);
	savefile->WriteString(callName);
	savefile->WriteInt(force);
	savefile->WriteBool(hasFoundTarget);
	savefile->WriteBool(isOn);
	savefile->WriteVec3(throwDestination);
	savefile->WriteInt(nextHitSoundTime);
}

void idItemGate::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(nextTriggerTime);
	savefile->ReadVec3(throwDir);
	savefile->ReadString(className);
	savefile->ReadString(weaponName);
	savefile->ReadString(callName);
	savefile->ReadInt(force);
	savefile->ReadBool(hasFoundTarget);
	savefile->ReadBool(isOn);
	savefile->ReadVec3(throwDestination);
	savefile->ReadInt(nextHitSoundTime);
}

void idItemGate::Spawn( void )
{
	className = spawnArgs.GetString( "class" );
	if ( !className.Length() )
	{
		gameLocal.Error( "idItemGate '%s' at (%s) doesn't have 'classname' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	weaponName = spawnArgs.GetString( "weapon" );
	if ( !weaponName.Length() )
	{
		gameLocal.Error( "idItemGate '%s' at (%s) doesn't have 'weapon' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
	}

	callName = spawnArgs.GetString( "callname" );

	force = spawnArgs.GetInt( "force", "160" );

	nextTriggerTime = 0;

	GetPhysics()->SetContents( CONTENTS_TRIGGER );

	StartSound( "snd_hum", SND_CHANNEL_BODY, 0, false, NULL );

	isOn = true;
	hasFoundTarget = false;
	nextHitSoundTime = 0;

	BecomeActive( TH_THINK );
}

void idItemGate::Think( void )
{
	//11-4-2015
	//why does the trigger spin in circles sometimes?
	//why does adding this empty function fix this bug?
	//??????????
}

void idItemGate::Event_staticactivate( int value )
{
	isOn = value;

	if (value >= 1)
	{
		StartSound( "snd_hum", SND_CHANNEL_BODY, 0, false, NULL );
	}
	else
	{
		StopSound( SND_CHANNEL_BODY, false );
	}
}

void idItemGate::repel( idEntity *ent )
{
	idDict args;
	ent->GetPhysics()->SetLinearVelocity(throwDir * force);

	if (gameLocal.time > nextHitSoundTime)
	{
		StartSound( "snd_hit", SND_CHANNEL_ANY, 0, false, NULL );
		nextHitSoundTime = gameLocal.time + 1500;
	}


	args.Clear();
	args.SetVector( "origin",		ent->GetPhysics()->GetOrigin() );
	args.Set(		"model",		"magnetfire.prt" );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );
}

void idItemGate::Event_Touch( idEntity *other, trace_t *trace )
{
	if (!isOn || g_skill.GetInteger() <= 0)
	{
		return;
	}

	if (!hasFoundTarget)
	{
		FindTargets();
		RemoveNullTargets();
		if ( !targets.Num() )
		{
			gameLocal.Error( "idItemGate '%s' at (%s) doesn't have 'target' key specified", name.c_str(), GetPhysics()->GetOrigin().ToString(0) );
		}
		throwDir = targets[0].GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
		throwDir.Normalize();

		throwDestination = targets[0].GetEntity()->GetPhysics()->GetOrigin();

		hasFoundTarget = true;
	}


	if ( nextTriggerTime > gameLocal.time )
	{
		// can't retrigger until the wait is over
		return;
	}
	
	if ( !other )
	{
		return;
	}

	nextTriggerTime = gameLocal.time + 1;

	//if player is holding item, then force player to drop the item.
	if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.IsValid())
	{
		if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.GetEntity()->spawnArgs.GetString("classname") == className)
		{
			idEntity *dragee;
			dragee = gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.GetEntity();

			gameLocal.GetLocalPlayer()->pickerWeapon.StopDrag(true, false);

			repel(dragee);
			return;
		}
	}

	//if in inventory, then remove item from inventory and spawn into world.
	if (gameLocal.GetLocalPlayer()->Event_hasWeapon(weaponName))
	{
		if (gameLocal.GetLocalPlayer()->RemoveWeaponItem(weaponName))
		{
			idVec3 spawnPos;
			idAngles forward;
			trace_t	tr;



			//BC 8-20-2015 if player has the weapon currently selected, then switch to a different weapon.
			if (idStr::Icmp(  weaponName, gameLocal.GetLocalPlayer()->GetCurrentWeapon()) == 0)
			{	
				gameLocal.GetLocalPlayer()->SelectWeapon(0, true); //select unarmed weapon.
			}



			//BC 10-20-2015 simplify this to just spawn the item in the middle of the trigger.
			//BC 11-4-2015 allow for manually-placed spawnpoint for the spawned item.

			idStr spawnpointname = spawnArgs.GetString( "spawnpoint", "" );
			if (spawnpointname.Length())
			{
				idEntity *spawnEnt = gameLocal.FindEntity(spawnpointname.c_str());

				if (spawnEnt)
				{
					spawnPos = spawnEnt->GetPhysics()->GetOrigin();
				}
			}
			else
			{
				//center of the bounding box.
				spawnPos = this->GetPhysics()->GetAbsBounds().GetCenter();
			}

			


			//weapon removed. now spawn it.
			//first, determine whether the item already exists in the world. (is this case specific to the deck?)

			gameLocal.GetLocalPlayer()->Event_hudMessage( common->GetLanguageDict()->GetString( "#str_02118" ) );

		
			idEntity *launchee = gameLocal.FindEntityUsingDef(NULL, className);
			if (launchee)
			{
				launchee->SetOrigin( spawnPos );

				if (callName)
				{
					gameLocal.GetLocalPlayer()->UseFrob(  launchee,  callName );
				}

				repel(launchee);			
				return;
			}
			else
			{
				//doesn't exist. spawn it.
				idDict args;
				idEntity *spawnee;

				args.Clear();
				args.SetVector( "origin", spawnPos );
				gameLocal.SpawnEntityDef( args, &spawnee );

				if (callName)
				{
					gameLocal.GetLocalPlayer()->UseFrob(  spawnee,  callName );
				}

				repel(spawnee);
				return;
			}
		}
	}
	

	if ( other->spawnArgs.GetString("classname") == className )
	{
		//launch the item.
		repel(other);
	}
}