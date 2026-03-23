/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#ifdef STEAM
#include "../steamstats.h"
#endif

CLASS_DECLARATION( idForce, idForce_Field )
END_CLASS

/*
================
idForce_Field::idForce_Field
================
*/
idForce_Field::idForce_Field( void ) {
	type			= FORCEFIELD_UNIFORM;
	applyType		= FORCEFIELD_APPLY_FORCE;
	magnitude		= 0.0f;
	dir.Set( 0, 0, 1 );
	randomTorque	= 0.0f;
	playerOnly		= false;
	monsterOnly		= false;
	clipModel		= NULL;

	//Raven
	lastApplyTime   = -1;

	jumppadTimer = 0;
}

/*
================
idForce_Field::~idForce_Field
================
*/
idForce_Field::~idForce_Field( void ) {
	if ( this->clipModel ) {
		delete this->clipModel;
	}
}

/*
================
idForce_Field::Save
================
*/
void idForce_Field::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( type );
	savefile->WriteInt( applyType);
	savefile->WriteFloat( magnitude );
	savefile->WriteVec3( dir );
	savefile->WriteFloat( randomTorque );
	savefile->WriteBool( playerOnly );
	savefile->WriteBool( monsterOnly );
	savefile->WriteClipModel( clipModel );
	savefile->WriteFloat(forceMultiplier);
	owner.Save(savefile);
}

/*
================
idForce_Field::Restore
================
*/
void idForce_Field::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)type );
	savefile->ReadInt( (int &)applyType);
	savefile->ReadFloat( magnitude );
	savefile->ReadVec3( dir );
	savefile->ReadFloat( randomTorque );
	savefile->ReadBool( playerOnly );
	savefile->ReadBool( monsterOnly );
	savefile->ReadClipModel( clipModel );
	savefile->ReadFloat(forceMultiplier);
	owner.Restore(savefile);
}

/*
================
idForce_Field::SetClipModel
================
*/
void idForce_Field::SetClipModel( idClipModel *clipModel ) {
	if ( this->clipModel && clipModel != this->clipModel ) {
		delete this->clipModel;
	}
	this->clipModel = clipModel;
}

/*
================
idForce_Field::Uniform
================
*/
void idForce_Field::Uniform( const idVec3 &force ) {
	dir = force;
	magnitude = dir.Normalize();
	type = FORCEFIELD_UNIFORM;
}

/*
================
idForce_Field::Explosion
================
*/
void idForce_Field::Explosion( float force ) {
	magnitude = force;
	type = FORCEFIELD_EXPLOSION;
}

/*
================
idForce_Field::Implosion
================
*/
void idForce_Field::Implosion( float force ) {
	magnitude = force;
	type = FORCEFIELD_IMPLOSION;
}

/*
================
idForce_Field::RandomTorque
================
*/
void idForce_Field::RandomTorque( float force ) {
	randomTorque = force;
}

/*
================
idForce_Field::Evaluate
================
*/
void idForce_Field::Evaluate( int time ) {
	int numClipModels, i;
	idBounds bounds;
	idVec3 force, torque, angularVelocity;
	idClipModel *cm, *clipModelList[ MAX_GENTITIES ];

	assert( clipModel );

	bounds.FromTransformedBounds( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[ i ];

		if ( !cm->IsTraceModel() ) {
			continue;
		}

		idEntity *entity = cm->GetEntity();

		if ( !entity ) {
			continue;
		}
		
		//bc ignore things.
		if (entity->IsType( idMoveableItem::Type ))
		{
			//int ignoreNum = static_cast<idMoveableItem *>( entity)->forcefieldIgnore.GetEntityNum();
			//int myOwnerNum = this->owner.GetEntityNum();

			if (static_cast<idMoveableItem *>( entity)->forcefieldIgnore.GetEntityNum() == this->owner.GetEntityNum())
			{
				continue;
			}
		}
		


		idPhysics *physics = entity->GetPhysics();

		//bc
		/*
		if (this->owner.IsValid())
		{
			if (gameLocal.GetLocalPlayer()->pickerState >= 2)
			{
				if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.IsValid())
				{
					if( gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.GetEntity()->IsGrabbed() && physics->IsType( idPhysics_Player::Type ))
					{
						continue;
					}
				}
			}			
		}
		*/


		if ( playerOnly ) {
			if ( !physics->IsType( idPhysics_Player::Type ) ) {
				continue;
			}
		} else if ( monsterOnly ) {
			if ( !physics->IsType( idPhysics_Monster::Type ) ) {
				continue;
			}
		}

		if ( !gameLocal.clip.ContentsModel( cm->GetOrigin(), cm, cm->GetAxis(), -1,
									clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis() ) ) {
			continue;
		}

		switch( type ) {
			case FORCEFIELD_UNIFORM: {
				force = dir;
				break;
			}
			case FORCEFIELD_EXPLOSION: {
				force = cm->GetOrigin() - clipModel->GetOrigin();
				force.Normalize();
				break;
			}
			case FORCEFIELD_IMPLOSION: {
				force = clipModel->GetOrigin() - cm->GetOrigin();
				force.Normalize();
				break;
			}
			default: {
				gameLocal.Error( "idForce_Field: invalid type" );
				break;
			}
		}
		
		if ( randomTorque != 0.0f ) {
			torque[0] = gameLocal.random.CRandomFloat();
			torque[1] = gameLocal.random.CRandomFloat();
			torque[2] = gameLocal.random.CRandomFloat();
			if ( torque.Normalize() == 0.0f ) {
				torque[2] = 1.0f;
			}
		}

		switch( applyType ) {
			case FORCEFIELD_APPLY_FORCE: {
				if ( randomTorque != 0.0f ) {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * magnitude );
				}
				else {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * magnitude );
				}
				break;
			}
			case FORCEFIELD_APPLY_VELOCITY:
			{
				//triggered by jumppad. LAUNCHER.


				//ignore objects that are being held by the player.

				if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.IsValid())
				{
					//player is holding onto an object.
					if (gameLocal.GetLocalPlayer()->pickerWeapon.dragEnt.GetEntity() == entity)
					{
						//the thrown object equals the carried object. abort.
						continue;
					}
				}

				if ( entity->IsType( idDoor::Type ) )
					continue;

				if ( entity->IsType( idAFEntity_Generic::Type ))
				{

					physics->SetLinearVelocity( force * magnitude, cm->GetId() );

					continue;
				}

				//BC hack. only launch players when they're reeeaally close, to lessen accidental launches when
				//launch is plopped down near player's feet.
				if ( entity->IsType( idPlayer::Type ) )
				{
					idVec3 playerPos = entity->GetPhysics()->GetOrigin();
					playerPos.z = clipModel->GetOrigin().z;
					idVec3 rawDist = playerPos - clipModel->GetOrigin();
					float finalDist = rawDist.LengthFast();
					

					if (finalDist > 24)
						continue;

					//force *= 0.98f; //BC hack. reduce player boost.

					static_cast<idPlayer *>( entity )->physicsObj.inJump = true;
					

#ifdef STEAM
					g_SteamStats->SetStatDelta("stat_launches_player", 1);
#endif

				}				
				else
				{
					
					force *= g_launchermultiplier.GetFloat(); //BC hack. give non-players an extra boost.

					//TODO: figure out why/how physics treats players and objects differently.
					//force *= 1.1f; //was 1.43
					//force *= 1.9f; //was 1.43
					//force *= forceMultiplier;
				}



				//common->Printf("thrown mass is %f\n", 					entity->GetPhysics()->GetMass());

				


				//bc hack: snap the object to the center of the jumppad so jump arc is more consistent.
				if (jumppadTimer < time)
				{
					jumppadTimer = time + 400;

					idVec3 jumppadOrigin = clipModel->GetOrigin();
					//jumppadOrigin.z = physics->GetOrigin().z;  //retain objects' original vertical (z) axis, so that things don't get stuck in the ground.

					//jumppadOrigin.z
					//gameRenderWorld->DebugCircle( colorWhite, jumppadOrigin, idVec3(0,0,1), 32, 12, 5000 );

					//physics->GetAbsBounds()					

					jumppadOrigin.z += 5;
					physics->SetOrigin( jumppadOrigin );



					//BC 10-13-2015 reset object angle to 0,0,0.
					idAngles snapAngle = idAngles(0,0,0);
					physics->SetAxis(snapAngle.ToMat3());



					//common->Printf("tele hack\n");

					//gameRenderWorld->DebugBounds( colorMagenta, physics->GetBounds(), physics->GetOrigin(), 5000 );
					//gameRenderWorld->DebugCircle( colorOrange, physics->GetOrigin(), idVec3(0,0,1), 16, 12, 5000 );
					//gameRenderWorld->DebugCircle( colorWhite, clipModel->GetOrigin(), idVec3(0,0,1), 32, 12, 5000 );

					physics->SetLinearVelocity( force * magnitude, cm->GetId() );
				}				
				
				
				/*
				if ( randomTorque != 0.0f )
				{
					angularVelocity = physics->GetAngularVelocity( cm->GetId() );
					physics->SetAngularVelocity( 0.5f * (angularVelocity + torque * randomTorque), cm->GetId() );
				}
				*/

				//Raven
				if ( !physics->IsType( idPhysics_AF::Type ))					
					lastApplyTime = time;

				break;
			}
			case FORCEFIELD_APPLY_IMPULSE: {
				if ( randomTorque != 0.0f ) {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * magnitude );
				}
				else {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * magnitude );
				}
				break;
			}
			default: {
				gameLocal.Error( "idForce_Field: invalid apply type" );
				break;
			}
		}
	}
}
