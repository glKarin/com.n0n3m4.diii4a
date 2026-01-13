/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#include "sys/platform.h"
#include "physics/Physics_Player.h"
#include "physics/Physics_Monster.h"
#include "physics/Physics_Parametric.h"
#include "WorldSpawn.h"
#include "Projectile.h"

#include "physics/Force_Field.h"

CLASS_DECLARATION( idForce, idForce_Field )
END_CLASS

const float PLAYER_HINT_PCT = 0.1f;

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
	mode2D			= false;
	clipModel		= NULL;

	//ivan start
	magnitudeType			= FORCEFIELD_MAGNITUDE_FIXED;
	magnitude_sphere_max	= 0.0f;
	magnitude_sphere_min	= 0.0f;
	magnitude_cylinder_max	= 0.0f;
	magnitude_cylinder_min	= 0.0f;
	oldVelocityPct			= 0.0f;
	oldVelocityProjPct		= 0.0f;
	swingMagnitude			= 0.0f;
	swingPeriod				= 0.0f;
	velocityCompensationPct	= 0.0f;
	parentLinearVelocity	= vec3_zero;
	useWhitelist			= false;
	exclusiveMode			= false;
	ignoreInactiveRagdolls	= false;
	windMode				= false;
	whiteList.Clear();
	//ivan end
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
	//ivan start
	int i;
	savefile->WriteInt( magnitudeType);
	savefile->WriteFloat( magnitude_sphere_max );
	savefile->WriteFloat( magnitude_sphere_min );
	savefile->WriteFloat( magnitude_cylinder_max );
	savefile->WriteFloat( magnitude_cylinder_min );
	savefile->WriteFloat( oldVelocityPct );
	savefile->WriteFloat( oldVelocityProjPct );
	savefile->WriteFloat( swingMagnitude );
	savefile->WriteFloat( swingPeriod );
	savefile->WriteFloat( velocityCompensationPct );
	savefile->WriteVec3( parentLinearVelocity );
	savefile->WriteBool( useWhitelist );
	savefile->WriteBool( exclusiveMode );
	savefile->WriteBool( ignoreInactiveRagdolls );
	savefile->WriteBool( windMode );

	savefile->WriteInt( whiteList.Num() );
	for( i = 0; i < whiteList.Num(); i++ ) {
		whiteList[ i ].Save( savefile );
	}
	//ivan end
	savefile->WriteInt( type );
	savefile->WriteInt( applyType);
	savefile->WriteFloat( magnitude );
	savefile->WriteVec3( dir );
	savefile->WriteFloat( randomTorque );
	savefile->WriteBool( playerOnly );
	savefile->WriteBool( monsterOnly );
	savefile->WriteBool( mode2D );
	savefile->WriteClipModel( clipModel );
}

/*
================
idForce_Field::Restore
================
*/
void idForce_Field::Restore( idRestoreGame *savefile ) {
	//ivan start
	int	i, num;
	savefile->ReadInt( (int &)magnitudeType);
	savefile->ReadFloat( magnitude_sphere_max );
	savefile->ReadFloat( magnitude_sphere_min );
	savefile->ReadFloat( magnitude_cylinder_max );
	savefile->ReadFloat( magnitude_cylinder_min );
	savefile->ReadFloat( oldVelocityPct );
	savefile->ReadFloat( oldVelocityProjPct );
	savefile->ReadFloat( swingMagnitude );
	savefile->ReadFloat( swingPeriod );
	savefile->ReadFloat( velocityCompensationPct );
	savefile->ReadVec3( parentLinearVelocity );
	savefile->ReadBool( useWhitelist );
	savefile->ReadBool( exclusiveMode );
	savefile->ReadBool( ignoreInactiveRagdolls );
	savefile->ReadBool( windMode );

	whiteList.Clear();
	savefile->ReadInt( num );
	whiteList.SetNum( num );
	for( i = 0; i < num; i++ ) {
		whiteList[ i ].Restore( savefile );
	}
	//ivan end
	savefile->ReadInt( (int &)type );
	savefile->ReadInt( (int &)applyType);
	savefile->ReadFloat( magnitude );
	savefile->ReadVec3( dir );
	savefile->ReadFloat( randomTorque );
	savefile->ReadBool( playerOnly );
	savefile->ReadBool( monsterOnly );
	savefile->ReadBool( mode2D );
	savefile->ReadClipModel( clipModel );
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
idForce_Field::Swing
================
*/
void idForce_Field::Swing( float magnitude, float period ) {
	swingMagnitude = magnitude;
	swingPeriod = period;
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

//ivan start
/*
================
idForce_Field::SetMagnitudeSphere
================
*/
void idForce_Field::SetMagnitudeSphere( float radius_min, float radius_max ) {
	magnitude_sphere_min = radius_min;
	magnitude_sphere_max = ( radius_max < radius_min ) ? radius_min : radius_max;
}

/*
================
idForce_Field::SetMagnitudeCylinder
================
*/
void idForce_Field::SetMagnitudeCylinder( float radius_min, float radius_max ) {
	magnitude_cylinder_min = radius_min;
	magnitude_cylinder_max = ( radius_max < radius_min ) ? radius_min : radius_max;
}

/*
================
idForce_Field::GetDistancePct
================
*/
float idForce_Field::GetDistancePct( float distance, float radius_inner, float radius_outer ) {
	float resultPct;
	if ( radius_outer > 0.0f ) {
		resultPct = (distance - radius_inner)/radius_outer;
		if ( resultPct < 0.0f ) { resultPct = 0.0f; } //inside inner radius
		if ( resultPct > 1.0f ) { resultPct = 1.0f; } //outside outer radius
		return resultPct;
	}
	return 1.0f; //don't use pct
}

/*
================
idForce_Field::SetPosition
================
*/
void idForce_Field::SetPosition( const idVec3 &newOrigin, const idMat3 &newAxis ) {
	assert( clipModel );
	clipModel->SetPosition( newOrigin, newAxis );
}

/*
================
idForce_Field::AddToWhiteList
================
*/
void idForce_Field::AddToWhiteList( idEntity *ent ) {
	if ( ent ) {
		idEntityPtr<idEntity> &entityPtr = whiteList.Alloc();
		entityPtr = ent;
	}
}

/*
================
idForce_Field::IsWhiteListed
================
*/
bool idForce_Field::IsWhiteListed( idEntity *ent ) {
	if ( ent ) {
		int j;
		for( j = 0; j < whiteList.Num(); j++ ) {
			if ( whiteList[ j ].GetEntity() == ent ) {
				return true;
			}
		}
	}
	return false;
}

/*
================
idForce_Field::RemoveFromWhiteList
================
*/
void idForce_Field::RemoveFromWhiteList( idEntity *ent ) {
	if ( ent ) {
		int j;
		for( j = 0; j < whiteList.Num(); j++ ) {
			if ( whiteList[ j ].GetEntity() == ent ) {
				whiteList.RemoveIndex(j);
				return;
			}
		}
	}
}

/*
================
idForce_Field::ClearWhiteList
================
*/
void idForce_Field::ClearWhiteList( void ) {
	whiteList.Clear();
}

//ivan end

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
	//ivan start
	idVec3 distanceVec;
	float realMagnitude;
	idVec3 oldVelocity;
	//ivan end

	assert( clipModel );

	bounds.FromTransformedBounds( clipModel->GetBounds(), clipModel->GetOrigin(), clipModel->GetAxis() );
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	torque.Zero();

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModelList[ i ];

		if ( !cm->IsTraceModel() ) {
			continue;
		}

		idEntity *entity = cm->GetEntity();

		if ( !entity ) {
			continue;
		}

		//ff1.3 start

		//entities whitelist
		if ( useWhitelist ) {
			int j;
			for( j = 0; j < whiteList.Num(); j++ ) {
				if ( whiteList[ j ].GetEntity() == entity ) {
					break;
				}
			}
			if ( j >= whiteList.Num() ) { //not found
				//gameLocal.Printf("ignore not in whitelist %s\n", entity->GetName());
				continue;
			}
		}

		//ignore some projectiles (beams, ...)
		if ( entity->IsType( idProjectile::Type ) && static_cast<idProjectile *>( entity )->IgnoresForceField() ){
			//gameLocal.Printf("ignore projectile %s\n", entity->GetName());
			continue;
		}

		//ignore inactive ragdolls
		if ( ignoreInactiveRagdolls
			&& entity->IsType( idAFEntity_Base::Type )
			&& !static_cast<idAFEntity_Base *>( entity )->IsActiveAF()
			/*&& !entity->IsType( idPlayer::Type )*/ ){
			//gameLocal.Printf("ignore inactive AF %s\n", entity->GetName());
			continue;
		}
		//ff1.3 end

		idPhysics *physics = entity->GetPhysics();

		if ( playerOnly ) {
			if ( !physics->IsType( idPhysics_Player::Type ) ) {
				continue;
			}
		} else if ( monsterOnly ) {
			if ( !physics->IsType( idPhysics_Monster::Type ) ) {
				continue;
			}
		} else if ( physics->IsType( idPhysics_Parametric::Type ) ) { //ff1.3 - ignore movers, doors, ...
			continue;
		}

		if ( !gameLocal.clip.ContentsModel( cm->GetOrigin(), cm, cm->GetAxis(), -1, clipModel->Handle(), clipModel->GetOrigin(), clipModel->GetAxis() ) ) {
			continue;
		}

		//ivan start
		distanceVec = cm->GetOrigin() - clipModel->GetOrigin();
		if ( mode2D ) {
			distanceVec.z = 0;
		}
		//ivan end

		switch( type ) {
			case FORCEFIELD_UNIFORM: {
				force = dir;
				break;
			}
			case FORCEFIELD_EXPLOSION: {
				//ivan - was: force = cm->GetOrigin() - clipModel->GetOrigin();
				force = distanceVec;
				force.Normalize();
				break;
			}
			case FORCEFIELD_IMPLOSION: {
				//ivan - was: force = clipModel->GetOrigin() - cm->GetOrigin();
				force = -distanceVec;
				force.Normalize();
				break;
			}
			default: {
				gameLocal.Error( "idForce_Field: invalid type" );
				return;
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

		realMagnitude = magnitude; //ivan

		//sinusoidal
		if( swingMagnitude > 0.0f && swingPeriod > 0.0f ){
			//gameLocal.Printf("sinusoidal at %d\n", gameLocal.time);
			realMagnitude += (idMath::Sin( (float)(gameLocal.time)/swingPeriod )) * swingMagnitude;
		}

		//ivan start - magnitude
		switch( magnitudeType ) {
			case FORCEFIELD_MAGNITUDE_FIXED: {
				//realMagnitude = magnitude; //do nothing, just use the default magnitude
				break;
			}
			case FORCEFIELD_MAGNITUDE_DISTANCE: {
				if ( magnitude_sphere_max > 0.0f ) {
					realMagnitude = realMagnitude * GetDistancePct( distanceVec.LengthFast(), magnitude_sphere_min, magnitude_sphere_max );
				}
				if ( magnitude_cylinder_max > 0.0f ) {
					float dist2d = ( cm->GetOrigin().ToVec2() - clipModel->GetOrigin().ToVec2() ).LengthFast();
					realMagnitude = realMagnitude * GetDistancePct( dist2d, magnitude_cylinder_min, magnitude_cylinder_max );
				}
				break;
			}
			case FORCEFIELD_MAGNITUDE_DISTANCE_INV: {
				if ( magnitude_sphere_max > 0.0f ) {
					realMagnitude = realMagnitude * (1 - GetDistancePct( distanceVec.LengthFast(), magnitude_sphere_min, magnitude_sphere_max ));
				}
				if ( magnitude_cylinder_max > 0.0f ) {
					float dist2d = ( cm->GetOrigin().ToVec2() - clipModel->GetOrigin().ToVec2() ).LengthFast();
					realMagnitude = realMagnitude * (1 - GetDistancePct( dist2d, magnitude_cylinder_min, magnitude_cylinder_max ));
				}
				break;
			}
			default: {
				gameLocal.Error( "idForce_Field: invalid magnitude type" );
				break;
			}
		}

		/*
		if ( fftest1.GetFloat() > 0 ) {
			idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();
			idVec3 center = entity->GetPhysics()->GetAbsBounds().GetCenter();
			gameRenderWorld->DebugArrow( colorMagenta, clipModel->GetOrigin(), cm->GetOrigin(), 1 );
			gameRenderWorld->DrawText( va( "%f pct", realMagnitude/magnitude ), center, 0.1f, colorWhite, axis, 1 );
			gameRenderWorld->DebugBounds( colorMagenta, clipModel->GetAbsBounds() );
		}
		*/

		//player hints
		if ( physics->IsType( idPhysics_Player::Type ) ) {
			int playerHint = static_cast<idPhysics_Player *>( physics )->GetHintForForceFields();
			if ( playerHint > 0 ) {
				realMagnitude += realMagnitude * ( force.z > 0.0f ? PLAYER_HINT_PCT : -PLAYER_HINT_PCT );  //increase upward forces, decrease downward ones
			} else if ( playerHint < 0 ) {
				realMagnitude += realMagnitude * ( force.z > 0.0f ? -PLAYER_HINT_PCT : PLAYER_HINT_PCT );  //decrease upward forces, increase downward ones
			}
		}
		//lower force for projectiles in wind
		else if ( windMode && entity->IsType( idProjectile::Type ) ){
			realMagnitude = realMagnitude * entity->spawnArgs.GetFloat("windMagnitudePct", "0.2");
		}

		//ivan end

		switch( applyType ) {
			case FORCEFIELD_APPLY_FORCE: {
				if ( randomTorque != 0.0f ) {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * realMagnitude );
				}
				else {
					entity->AddForce( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * realMagnitude );
				}
				break;
			}
			case FORCEFIELD_APPLY_VELOCITY: {
				//ivan start
				if ( entity->IsType( idProjectile::Type ) ) {
					oldVelocity = oldVelocityProjPct * physics->GetLinearVelocity();
				} else if ( oldVelocityPct > 0.0f ) {
					oldVelocity = oldVelocityPct * physics->GetLinearVelocity();
				} else {
					oldVelocity.Zero();
				}

				//increase magnitude for items 'following' the forcefield (useful for BFG)
				if ( velocityCompensationPct > 0.0f && parentLinearVelocity != vec3_zero ) {
					idVec3 velDir = parentLinearVelocity;
					velDir.NormalizeFast();
					float projection = force * velDir;
					//gameLocal.Printf("proj %f\n", proj);
					if ( projection > 0.9f ) { //following the proj
						realMagnitude += realMagnitude * projection * projection * projection * projection * velocityCompensationPct;
					}
				}

				if ( !windMode && entity->IsType( idGuidedProjectile::Type ) ) {
					static_cast<idGuidedProjectile *>( entity )->SetForceFieldVelocity( force * realMagnitude );
				} else {
					physics->SetLinearVelocity( oldVelocity + force * realMagnitude, cm->GetId() );
				}
				//was: physics->SetLinearVelocity( force * realMagnitude, cm->GetId() );
				//ivan end
				if ( randomTorque != 0.0f ) {
					angularVelocity = physics->GetAngularVelocity( cm->GetId() );
					physics->SetAngularVelocity( 0.5f * (angularVelocity + torque * randomTorque), cm->GetId() );
				}
				break;
			}
			case FORCEFIELD_APPLY_IMPULSE: {
				if ( randomTorque != 0.0f ) {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin() + torque.Cross( dir ) * randomTorque, dir * realMagnitude );
				}
				else {
					entity->ApplyImpulse( gameLocal.world, cm->GetId(), cm->GetOrigin(), force * realMagnitude );
				}
				break;
			}
			default: {
				gameLocal.Error( "idForce_Field: invalid apply type" );
				return;
			}
		}

		if( exclusiveMode ){
			break;
		}
	}
}
