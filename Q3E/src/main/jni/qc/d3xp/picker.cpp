//bc copied from the Grabber weapon.

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Misc.h"


#ifdef STEAM
#include "steamstats.h"
#endif

#define MAX_DRAG_TRACE_DISTANCE			80.0f
#define DRAG_FAIL_LEN					64.f
#define	THROW_SCALE						384 //throwing speed
#define FIRING_DELAY					100.0f
#define TRACE_BOUNDS_SIZE				3.f
#define MAX_PICKUP_VELOCITY				1500 * 1500
#define MAX_PICKUP_SIZE					96

#define	HOLD_DISTANCE_DEFAULT			32
#define	HOLD_DISTANCE_MIN				20
#define	HOLD_DISTANCE_MAX				50

#define THROW_TIME_THRESHOLD			400 //if player holds left button down for this much milliseconds, then throw the held item.

#define PICKUP_TIME_THRESHOLD			350 //if player holds left button down for this much milliseconds, then throw the held item.




CLASS_DECLARATION( idEntity, idPicker )
END_CLASS

idPicker::idPicker( void )
{
	dragEnt = NULL;
	owner = NULL;
	oldUcmdFlags = 0;
	holdingAF = false;
	dragFailTime = 0;
	startDragTime = 0;
	dragTraceDist = MAX_DRAG_TRACE_DISTANCE;
	holdDistance = HOLD_DISTANCE_DEFAULT;
	pickerHeld = false;

	this->attacktimerStart = 0;
	this->attackHeld = false;

	this->pickuptimerStart = 0;
	this->pickerHeld = false;

	this->holdstate = 0;
	
	rotationIndex = -1;	

	rotationAngleStart = 0;
	rotationAngleEnd = 0;
	rotationTimeStart = 0;
	rotationTimeEnd = 0;
}

idPicker::~idPicker( void ) 
{
	
}

void idPicker::Save( idSaveGame *savefile ) const
{
	owner.Save(savefile);
	dragEnt.Save(savefile);

	savefile->WriteBool(attackHeld);
	savefile->WriteInt(attacktimerStart);	
	savefile->WriteStaticObject( drag );
	savefile->WriteInt( id );	
	savefile->WriteInt(pickuptimerStart);
	savefile->WriteInt(holdstate);
	
	savefile->WriteInt(holdDistance);
	savefile->WriteVec3(saveGravity);
	savefile->WriteInt(oldUcmdFlags);
	savefile->WriteVec3(localPlayerPoint);
	savefile->WriteInt(dragFailTime);
	savefile->WriteInt(startDragTime);
	savefile->WriteBool(holdingAF);
	savefile->WriteBool(pickerHeld);
	savefile->WriteFloat(dragTraceDist);
	savefile->WriteInt(rotationIndex);
	savefile->WriteInt(rotationAngleStart);
	savefile->WriteInt(rotationAngleEnd);
	savefile->WriteInt(rotationTimeStart);
	savefile->WriteInt(rotationTimeEnd);
}

void idPicker::Restore( idRestoreGame *savefile ) 
{
	owner.Restore(savefile);
	dragEnt.Restore(savefile);

	savefile->ReadBool(attackHeld);
	savefile->ReadInt(attacktimerStart);
	savefile->ReadStaticObject( drag );
	savefile->ReadInt(id);	
	savefile->ReadInt(pickuptimerStart);
	savefile->ReadInt(holdstate);

	savefile->ReadInt(holdDistance);
	savefile->ReadVec3(saveGravity);
	savefile->ReadInt(oldUcmdFlags);
	savefile->ReadVec3(localPlayerPoint);
	savefile->ReadInt(dragFailTime);
	savefile->ReadInt(startDragTime);
	savefile->ReadBool(holdingAF);
	savefile->ReadBool(pickerHeld);
	savefile->ReadFloat(dragTraceDist);
	savefile->ReadInt(rotationIndex);
	savefile->ReadInt(rotationAngleStart);
	savefile->ReadInt(rotationAngleEnd);
	savefile->ReadInt(rotationTimeStart);
	savefile->ReadInt(rotationTimeEnd);
}

void idPicker::SetDragDistance( float dist ) {
	dragTraceDist = dist;
}

//adjust how far/close the object is to the player.
void idPicker::SetHoldDistance( int delta )
{
	idPlayer *thePlayer = owner.GetEntity();

	int minDistance = HOLD_DISTANCE_MIN;
		
	if (dragEnt.GetEntity() != NULL)
	{
		int holdDistanceMin = dragEnt.GetEntity()->spawnArgs.GetInt("holddistancemin");

		if (holdDistanceMin > 0)
		{
			minDistance = holdDistanceMin;
		}
	}

	
	this->holdDistance += delta;
	this->holdDistance = idMath::ClampInt(minDistance, HOLD_DISTANCE_MAX, this->holdDistance);

	localPlayerPoint = ( thePlayer->firstPersonViewAxis[0] * holdDistance ) * thePlayer->firstPersonViewAxis.Transpose();
}

void idPicker::StartDrag( idEntity *grabEnt, int id ) {
	int clipModelId = id;
	idPlayer *thePlayer = owner.GetEntity();

	this->rotationIndex = -1;
	holdingAF = false;
	dragFailTime = gameLocal.slow.time;
	startDragTime = gameLocal.slow.time;

	oldUcmdFlags = thePlayer->usercmd.flags;

	// set grabbed state for networking
	grabEnt->SetGrabbedState( true );

	// This is the new object to drag around
	dragEnt = grabEnt;

	


	// Handle specific class types
	if ( grabEnt->IsType( idAI::Type ) )
	{
		idAI *aiEnt = static_cast<idAI*>(grabEnt);
		//aiEnt->Damage( thePlayer, thePlayer, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
		aiEnt->StartRagdoll();

		holdingAF = true;
		clipModelId = 0;
	}
	else if ( grabEnt->IsType( idAFEntity_Base::Type ) )
	{
		holdingAF = true;
		//clipModelId = 0;
	}
	else if ( grabEnt->IsType( idMoveableItem::Type ) )
	{
		if (static_cast<idMoveableItem*>(grabEnt)->frozen > 0)
		{
			if (static_cast<idMoveableItem*>(grabEnt)->frozen == 1)
			{
				grabEnt->StartSound( "snd_unfreeze", SND_CHANNEL_ANY, 0, false, NULL );
			}


			static_cast<idMoveableItem*>(grabEnt)->frozen = 2;

			grabEnt->Unbind();

			
		}

		// flibit: 64 bit fixes, changed NULL to 0
		grabEnt->PostEventMS( &EV_Touch, 250, thePlayer, 0 );
		// flibit end

	}
	
	
	// Get the current physics object to manipulate
	idPhysics *phys = grabEnt->GetPhysics();

	// Turn off gravity on object
	//if ( !holdingAF )
	{
		//saveGravity = phys->GetGravity();	
		//saveGravity = gameLocal.GetGravity();
		//phys->SetGravity( vec3_origin );
		//phys->
	}

	//find distance between object and player eyeball.
	float distanceToObject = (	thePlayer->firstPersonViewOrigin - grabEnt->GetPhysics()->GetOrigin() ).Normalize();

	if ( grabEnt->spawnArgs.GetInt( "isremote", "0" ) )
	{
		holdDistance = dragEnt.GetEntity()->spawnArgs.GetInt( "remote_x", "12" );
	}
	else if (distanceToObject < HOLD_DISTANCE_DEFAULT)
	{
		holdDistance = distanceToObject - 1;
	}	
	else
	{
		//hold distance default override.
		int holddistOverride = grabEnt->spawnArgs.GetInt( "holddistance", "32" );

		holdDistance = holddistOverride;
	}


	// hold it directly in front of player
	localPlayerPoint = ( thePlayer->firstPersonViewAxis[0] * holdDistance ) * thePlayer->firstPersonViewAxis.Transpose();

	// Start up the Force_Drag to bring it in
	drag.Init( g_grabberDamping.GetFloat() );
	drag.SetPhysics( phys, clipModelId, thePlayer->firstPersonViewOrigin + localPlayerPoint * thePlayer->firstPersonViewAxis);

	attackHeld = true;
	pickerHeld = true;
	
	thePlayer->StartSound("snd_grab", SND_CHANNEL_WEAPON, 0, false, NULL);

	//bc 8-13-2015
	grabEnt->StartSound( "snd_onpick", SND_CHANNEL_ANY, 0, false, NULL );

}

void idPicker::StopDrag( bool dropOnly, bool isRightClick )
{
	idPlayer *thePlayer = owner.GetEntity();

	if ( dragEnt.IsValid() )
	{
		idEntity *ent = dragEnt.GetEntity();

		// set grabbed state for networking
		ent->SetGrabbedState( false );

		// If a cinematic has started, let dropped object to think in cinematics
		if ( gameLocal.inCinematic ) {
			ent->cinematic = true;
		}

		// Restore Gravity
		if ( !holdingAF )
		{
			//ent->GetPhysics()->SetGravity( saveGravity );
		}

		// Move the object back to the slow group (helltime)
		ent->timeGroup = TIME_GROUP1;

		if ( holdingAF )
		{
			idAFEntity_Base *af = static_cast<idAFEntity_Base *>(ent);
			idPhysics_AF	*af_Phys = static_cast<idPhysics_AF*>(af->GetPhysics());

			// Reset timers so that it isn't forcibly put to rest in mid-air
			af_Phys->PutToRest();
			af_Phys->Activate();

			af_Phys->SetTimeScaleRamp( MS2SEC(gameLocal.slow.time) - 1.5f, MS2SEC(gameLocal.slow.time) + 1.0f );
		}

		
		ent->lastThrowTime = gameLocal.time; //for the security turret. if player throws or drops an item, mark it as a turret target.

		// If the object isn't near its goal, just drop it in place.
		if ( !ent->IsType( idProjectile::Type ) && ( dropOnly || drag.GetDistanceToGoal() > DRAG_FAIL_LEN ) )
		{
			ent->GetPhysics()->SetLinearVelocity( vec3_origin );
			
			thePlayer->StartSound("snd_maindrop", SND_CHANNEL_WEAPON, 0, false, NULL);

			ent->StartSound( "snd_onunpick", SND_CHANNEL_ANY, 0, false, NULL );
		}
		else
		{
			// Shoot the object forward
			ent->ApplyImpulse( thePlayer, 0, ent->GetPhysics()->GetOrigin(), (thePlayer->firstPersonViewAxis[0] + idVec3(0, 0, 0.6f)) * THROW_SCALE * ent->GetPhysics()->GetMass() );
			
			thePlayer->StartSound("snd_release", SND_CHANNEL_WEAPON, 0, false, NULL);

			ent->StartSound( "snd_onunpick", SND_CHANNEL_ANY, 0, false, NULL );


			

			// Orient projectiles away from the player
			if ( ent->IsType( idMoveable::Type ) )
			{
				// Turn on damage for this object
				//idMoveable *obj = static_cast<idMoveable*>(ent);
				//obj->EnableDamage( true, 2.5f );
				//obj->SetAttacker( thePlayer );
			}
			/*
			else if ( ent->IsType( idMoveableItem::Type ) )
			{

			}
			*/

#ifdef STEAM
					g_SteamStats->SetStatDelta("stat_throws", 1);
#endif
		}

		// Remove the Force_Drag's control of the entity
		drag.RemovePhysics( ent->GetPhysics() );

		//if it's a remote, then call the onDrop call. onDrop triggers the selected item on the BlueBox.
		if ( dragEnt.GetEntity()->spawnArgs.GetInt( "isremote", "0" ))
		{
			if (thePlayer->launcherEnt.IsValid())
			{
				if (isRightClick)
				{
					//Execute the line.
					thePlayer->UseFrob( thePlayer->launcherEnt.GetEntity(), "onDrop");					
					thePlayer->pickerHeld = true;

					if (thePlayer->launcherEnt.GetEntity()->IsType(idBluebox::Type))
					{
						static_cast<idBluebox *>( thePlayer->launcherEnt.GetEntity() )->OnExecute();
					}
				}
				else
				{
					//I just right clicked!!
					if (thePlayer->launcherEnt.GetEntity()->IsType(idBluebox::Type))
					{
						static_cast<idBluebox *>( thePlayer->launcherEnt.GetEntity() )->OnDrop();
					}
				}
			}
		}
	}

	dragEnt = NULL;
	
	this->holdstate = 2;
	
	
	/*
	if ( !(thePlayer->usercmd.buttons & BUTTON_PICKER) )
	{
		pickerHeld = false;
	}
	else
	{
		pickerHeld = true;
	}

	if ( !(thePlayer->usercmd.buttons & BUTTON_ATTACK) )
	{
		attackHeld = false;
	}
	else
	{
		attackHeld = true;
	}*/
}


int idPicker::UpdateHeld( idPlayer *player )
{
	//if not holding anything.
	if (!dragEnt.GetEntity())
	{
		if ( !(player->usercmd.buttons & BUTTON_ATTACK) && attackHeld )
		{
			attackHeld = false;
		}

		return 0;
	}

	idPhysics *entPhys = dragEnt.GetEntity()->GetPhysics();
	idVec3 goalPos;

	if (this->holdstate == 0 && dragEnt.GetEntity() && pickerHeld == false && attackHeld == false )
	{
		this->holdstate = 1;
	}

	//holding an item.
	if (this->holdstate == 1)
	{
		if (player->usercmd.buttons & BUTTON_ATTACK)
		{
			if (!attackHeld)
			{			
				this->attackHeld = true;
				this->attacktimerStart = gameLocal.time;
				//player->DebugMessage(va("uheld %d %d", this->attacktimerStart, gameLocal.time));
			}

			if (attacktimerStart + 400 < gameLocal.time)
			{
				//throw.
				player->StartSound("snd_exhale", SND_CHANNEL_ANY, 0, false, NULL);
				
				StopDrag( false, false );
				this->holdstate = 2;
				return 3;
			}
		}
		else if ( !(player->usercmd.buttons & BUTTON_ATTACK) && attackHeld)
		{
			this->attackHeld = false;
			if (attacktimerStart + 400 >= gameLocal.time)
			{
				//tap.
				//drop.

				StopDrag( true, true );
				this->holdstate = 2;
				return 3;
			}
		}

		int rotationSteps[8] = {180, 225, 270, 315, 0, 45, 90, 135 };

		if (dragEnt.IsValid())
		{
			if (dragEnt.GetEntity()->spawnArgs.GetInt("alt_rotation", "0") > 0)
			{
				rotationSteps[0] = 0;
				rotationSteps[1] = 45;
				rotationSteps[2] = 90;
				rotationSteps[3] = 135;
				rotationSteps[4] = 180;
				rotationSteps[5] = 225;
				rotationSteps[6] = 270;
				rotationSteps[7] = 315;
			}
		}
		
		if (player->usercmd.buttons & BUTTON_PICKER && !pickerHeld)
		{
			//pickerWeapon.dragEnt.GetEntity()->spawnArgs.GetInt( "isremote", "0" ) )

			//Remote (bluebox) special case.
			if (dragEnt.GetEntity()->spawnArgs.GetInt( "isremote", "0"))
			{
				idPlayer *thePlayer = owner.GetEntity();

				StopDrag(true, false); //right click. drop.
				thePlayer->pickerHeld = true;
				return 0;
			}

			pickerHeld = true;

			//rotate
			

			
			player->StartSound("snd_rotate", SND_CHANNEL_ANY, 0, false, NULL );


			rotationAngleStart = this->rotationIndex;

			if (rotationAngleStart < 0)
				rotationAngleStart = 0;


			int rotationSlots = sizeof(rotationSteps) / sizeof(int);
			this->rotationIndex++;
		
			if (this->rotationIndex >= rotationSlots)
			{
				this->rotationIndex = 0;
			}

			rotationAngleEnd = this->rotationIndex;

			rotationTimeStart = gameLocal.time;
			rotationTimeEnd = gameLocal.time + 100;
		}

		if (this->rotationIndex >= 0 && !holdingAF)
		{
			float lerpAmount;

			if (gameLocal.time >= rotationTimeEnd)
			{
				lerpAmount = 1;
			}
			else
			{
				lerpAmount = (gameLocal.time - rotationTimeStart) / 100.0f;
			}

			float lerpAng = idMath::Lerp(rotationSteps[rotationAngleStart], rotationSteps[rotationAngleEnd], lerpAmount);


			if (rotationSteps[rotationAngleStart] > rotationSteps[rotationAngleEnd])
			{
				//hack to resolve lerp weirdness when going from a big number to a small number.
				lerpAng = idMath::Lerp(rotationSteps[rotationAngleEnd] - 45, rotationSteps[rotationAngleEnd], lerpAmount);
			}


			
			//if (gameLocal.time < rotationTimeEnd)
			//common->Printf("%f   %f  \n", lerpAmount, lerpAng);


			idAngles ang = player->firstPersonViewAxis[0].ToAngles();
			ang.roll = 0.f;
			ang.pitch = 0.f;
			ang.yaw += lerpAng;
			entPhys->SetAxis( ang.ToMat3() );

			
			/*
			int angleToAdd = rotationSteps[this->rotationIndex];
			//orient the object.
			idAngles ang = player->firstPersonViewAxis[0].ToAngles();
			ang.roll = 0.f;
			ang.pitch = 0.f;
			ang.yaw += angleToAdd;
			entPhys->SetAxis( ang.ToMat3() );
			*/
		}

		/*
		if (player->usercmd.buttons & BUTTON_PICKER)
		{
			if (!pickerHeld)
			{
				this->pickerHeld = true;
				this->pickuptimerStart = gameLocal.time;
			}

			if (pickuptimerStart + 400 < gameLocal.time)
			{
				//drop.
				StopDrag( true, false );
				this->holdstate = 2;
				return 3;
			}
		}
		else if ( !(player->usercmd.buttons & BUTTON_PICKER) && pickerHeld)
		{
			this->pickerHeld = false;

			if (pickuptimerStart + 400 >= gameLocal.time)
			{
				//Get.
				player->PerformImpulse(41);
				this->holdstate = 2;
				return 0;
			}
		}*/
	}



	

	//Handle object physics
	//=================================================================
	// Check if the player is standing on the object.
	if ( !holdingAF && dragEnt.GetEntity()->IsType( idMoveable::Type ))
	{
		idBounds	playerBounds;
		idBounds	objectBounds = entPhys->GetAbsBounds();
		idVec3		newPoint = player->GetPhysics()->GetOrigin();

		// create a bounds at the players feet
		playerBounds.Clear();
		playerBounds.AddPoint( newPoint );
		newPoint.z -= 1.f;
		playerBounds.AddPoint( newPoint );
		playerBounds.ExpandSelf( 8.f );

		// If it intersects the object bounds, then drop it
		if ( playerBounds.IntersectsBounds( objectBounds ) )
		{
			StopDrag( true, false );
			player->StartSound( "snd_error", SND_CHANNEL_ANY, 0, false, NULL );
			player->Event_hudMessage( common->GetLanguageDict()->GetString( "#str_02116" )  );
			return 3;
		}
	}

	// Set and evaluate drag force
	goalPos = player->firstPersonViewOrigin + localPlayerPoint * player->firstPersonViewAxis;

	if (holdingAF)
	{
		if ( dragEnt.GetEntity()->spawnArgs.GetInt( "isremote", "0" ) )
		{
			goalPos.z += dragEnt.GetEntity()->spawnArgs.GetInt( "remote_z", "2" );
		}
		else
		{
			int holdheight = dragEnt.GetEntity()->spawnArgs.GetInt( "holdheight", "4" );
			goalPos.z += holdheight; //held AFs look better when they're elevated up a bit.
		}
	}

	drag.SetGoalPosition( goalPos );
	drag.Evaluate( gameLocal.time );

	// If an object is flying too fast toward the player, stop it hard
	if ( g_grabberHardStop.GetBool() )
	{
		idPlane theWall;
		idVec3 toPlayerVelocity, objectCenter;
		float toPlayerSpeed;

		toPlayerVelocity = -player->firstPersonViewAxis[0];
		toPlayerSpeed = entPhys->GetLinearVelocity() * toPlayerVelocity;

		if ( toPlayerSpeed > 64.f )
		{
			objectCenter = entPhys->GetAbsBounds().GetCenter();

			theWall.SetNormal( player->firstPersonViewAxis[0] );
			theWall.FitThroughPoint( goalPos );

			if ( theWall.Side( objectCenter, 0.1f ) == PLANESIDE_BACK ) {
				int i, num;

				num = entPhys->GetNumClipModels();
				for ( i=0; i<num; i++ ) {
					entPhys->SetLinearVelocity( vec3_origin, i );
				}
			}
		}

		idVec3 angVel;
		angVel.x = 0;
		angVel.y = 0;
		angVel.z = 0;
		entPhys->SetAngularVelocity( angVel );
	}
	//=================================================================

	if ( !(player->usercmd.buttons & BUTTON_PICKER) && pickerHeld )
	{
		pickerHeld = false;
	}

	if ( !(player->usercmd.buttons & BUTTON_ATTACK) && attackHeld )
	{
		attackHeld = false;
	}

	if (this->holdstate > 0 && dragEnt.GetEntity() && dragEnt.GetEntity()->spawnArgs.GetInt("showhand", "1") <= 0 )
	{
		//BC 3-13-2016 add flag to hide finger. so that you can read letters.
		return 4;
	}
	else if (this->holdstate > 0 && dragEnt.GetEntity())
	{
		return 2;
	}

	return 0;
}

//Update return values:
//   0 = holding nothing.
//   1 = something valid in crosshair.
//   2 = holding something.
//   3 = dropped the object.

int idPicker::Update( idPlayer *player, bool hide, idEntity *newEnt, int newEntID, bool forcePick )
{
	// Dead players release the trigger
	if ( hide || player->health <= 0 )
	{
		StopDrag( true, false );
		return 3;
	}



	//player is HOLDING something. Do some checks to ensure it should still be held.
	if ( dragEnt.GetEntity() )
	{
		return 2;
	}
	
	owner = player;

	if (this->holdstate == 2 && !(player->usercmd.buttons & BUTTON_PICKER)
		&& !(player->usercmd.buttons & BUTTON_ATTACK))
	{
		this->holdstate = 0;
	}

	// if no entity selected for dragging. search for stuff that's grabbable.
    if ( !dragEnt.GetEntity() )
	{		
		// If the trace hit something
		if ( newEnt )
		{
			// if entity is already being grabbed then bypass
			if ( gameLocal.isMultiplayer && newEnt->IsGrabbed() )
			{
				return 0;
			}

			// Check if this is a valid entity to hold
			if ( newEnt && 
				
					( newEnt->IsType( idMoveable::Type )
					|| newEnt->IsType( idMoveableItem::Type )
					|| newEnt->IsType( idProjectile::Type )
					|| newEnt->IsType( idStaticEntity::Type )
					|| newEnt->IsType( idScrew::Type )
					|| newEnt->IsType( idFrobCube::Type )
					|| newEnt->IsType( idAFEntity_Base::Type ))

					&& newEnt->noGrab == false
					//&& newEnt->GetPhysics()->GetBounds().GetRadius() < MAX_PICKUP_SIZE
					&& newEnt->GetPhysics()->GetLinearVelocity().LengthSqr() < MAX_PICKUP_VELOCITY)
			{

				//bc 9-26-2015 gettable frobcubes can be right clicked
				if (newEnt->IsType(idFrobCube::Type)   && player->usercmd.buttons & BUTTON_PICKER)
				{
					if (newEnt->spawnArgs.GetBool("gettable"))
					{
						static_cast<idFrobCube *>( newEnt )->OnFrob(this);
					}

					return 0;
				}


				//BC 4-15-2016 moveables can be gotten
				if (newEnt->IsType(idMoveableItem::Type) && player->usercmd.buttons & BUTTON_PICKER)
				{
					if (newEnt->spawnArgs.GetBool("stashable"))
					{
						gameLocal.GetLocalPlayer()->StartSound( "snd_get", SND_CHANNEL_ANY, 0, false, NULL );

						idDict args;
						args.Clear();
						args.SetVector( "origin",		newEnt->GetPhysics()->GetOrigin() );
						args.Set(		"model",	    "pickupdust.prt" );
						args.SetBool(	"start_off",	false );
						gameLocal.SpawnEntityType( idExplodable::Type, &args );

						newEnt->PostEventMS(&EV_Remove, 0 );

						return 0;
					}
				}


				bool validAF = true;



				

				if (player->usercmd.buttons & BUTTON_PICKER)
				{
					player->PerformImpulse(41); //GET item.
				}

				if ( !dragEnt.GetEntity() && this->holdstate == 0 && forcePick )
				{
						//hold the button. hold the item.
						StartDrag(newEnt, newEntID);
						player->UseFrob( newEnt, "onPick");
				}



				/*
				if ( validAF && (( (player->usercmd.buttons & BUTTON_PICKER) && (this->pickuptimerStart + pickupThreshold < gameLocal.time)  && !dragEnt.GetEntity() ) || forcePick) && !attackHeld && !pickerHeld)
				{
					// Grab this entity and start dragging it around.
					StartDrag( newEnt, newEntID );
					player->UseFrob( newEnt, "onPick");

					this->pickuptimerStart = gameLocal.time;
					this->pickerHeld = true;
				}
				else if (!(player->usercmd.buttons & BUTTON_PICKER) && this->pickerHeld && (this->pickuptimerStart + pickupThreshold > gameLocal.time))
				{
					//if player taps the right click, then GET the item.
					this->pickerHeld = false;
					player->PerformImpulse(41); //GET item.
				}*/
				else if ( validAF )
				{
					// A holdable object is ready to be grabbed
					if (pickerHeld && !(player->usercmd.buttons & BUTTON_PICKER))
					{
						pickerHeld = false;
					}

					return 1;
				}




			}
		}

		if (pickerHeld && !(player->usercmd.buttons & BUTTON_PICKER))
			pickerHeld = false;

		if (attackHeld && !(player->usercmd.buttons & BUTTON_ATTACK))
			attackHeld = false;
	}



	if (dragEnt.GetEntity())
		return 2;

	// Not holding, nothing to hold
	return 0;
}
