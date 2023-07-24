// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Vehicle_RigidBody.h"
#include "VehicleView.h"
#include "VehicleControl.h"
#include "TransportComponents.h"
#include "../Actor.h"
#include "../Player.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../../decllib/DeclSurfaceType.h"
#include "../botai/BotThreadData.h"
#include "../botai/Bot.h"

/*
===============================================================================

	sdVehicle_RigidBody

===============================================================================
*/
const idEventDef EV_setDamageDealtScale( "setDamageDealtScale", '\0', DOC_TEXT( "Sets the scale factor applied to damage applied when this vehicle collides into something." ), 1, NULL, "f", "scale", "Scale factor to apply." );

CLASS_DECLARATION( sdTransport_RB, sdVehicle_RigidBody )
	EVENT( EV_setDamageDealtScale,			sdVehicle_RigidBody::Event_SetDamageDealtScale )
END_CLASS

/*
================
sdVehicle_RigidBody::sdVehicle_RigidBody
================
*/
sdVehicle_RigidBody::sdVehicle_RigidBody( void ) {
	collideDamage						= NULL;
	collideFatalDamage					= NULL;
	onCollisionFunc						= NULL;
	onCollisionSideScrapeFunc			= NULL;
	nextCollisionTime					= 0;
	collideDamageDealtScale				= 1.0f;
}

/*
================
sdVehicle_RigidBody::~sdVehicle_RigidBody
================
*/
sdVehicle_RigidBody::~sdVehicle_RigidBody( void ) {
}

/*
================
sdVehicle_RigidBody::DoLoadVehicleScript
================
*/
void sdVehicle_RigidBody::DoLoadVehicleScript( void ) {
	if ( !spawnArgs.GetBool( "disableIK" ) ) {
		ik.Init( this, IK_ANIM, vec3_origin );
	}
	ik.ClearWheels();

	physicsObj.ClearClipModels();

	physicsObj.SetFriction( spawnArgs.GetFloat( "linear_friction" ), spawnArgs.GetFloat( "angular_friction" ) );
	physicsObj.SetWaterFriction( spawnArgs.GetFloat( "linear_friction_water" ), spawnArgs.GetFloat( "angular_friction_water" ) );
	physicsObj.SetBouncyness( spawnArgs.GetFloat( "bouncyness" ) );
	physicsObj.SetWaterRestThreshold( spawnArgs.GetFloat( "water_rest_threshold", "1" ) );

	LoadParts( VPT_PART | VPT_WHEEL | VPT_HOVER | VPT_SIMPLE_PART | VPT_SCRIPTED_PART 
			| VPT_MASS | VPT_TRACK | VPT_ROTOR | VPT_THRUSTER | VPT_SUSPENSION | VPT_VTOL 
			| VPT_ANTIGRAV | VPT_PSEUDO_HOVER | VPT_DRAGPLANE | VPT_RUDDER 
			| VPT_AIRBRAKE | VPT_HURTZONE | VPT_ANTIROLL | VPT_ANTIPITCH );

	physicsObj.CalculateMassProperties();
}

/*
================
sdVehicle_RigidBody::Spawn
================
*/
void sdVehicle_RigidBody::Spawn( void ) {
	const char* damagename;

	damagename = spawnArgs.GetString( "dmg_collide" );
	if ( *damagename ) {
		collideDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !collideDamage ) {
			gameLocal.Warning( "sdVehicle::Spawn Invalid Damage Type '%s'", damagename );
		}
	} else {
		collideDamage = NULL;
	}

	damagename = spawnArgs.GetString( "dmg_collide_fatal" );
	if ( *damagename ) {
		collideFatalDamage = gameLocal.declDamageType.LocalFind( damagename, false );
		if( !collideFatalDamage ) {
			gameLocal.Warning( "sdVehicle::Spawn Invalid Damage Type '%s'", damagename );
		}
	}
	if ( collideFatalDamage == NULL ) {
		collideFatalDamage = collideDamage;
	}


	onCollisionFunc = scriptObject->GetFunction( "OnCollision" );
	onCollisionSideScrapeFunc = scriptObject->GetFunction( "OnCollisionSideScrape" );

	collideDotLimit = spawnArgs.GetFloat( "collide_dot_limit", "-0.5" );

	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetFastPath( true );

	SetPhysics( &physicsObj );

	LoadVehicleScript();

	BecomeActive( TH_THINK );

	if( !health ) {
		health = 2000;
	}

	float gravityScale;
	if ( spawnArgs.GetFloat( "gravity", DEFAULT_GRAVITY_STRING, gravityScale ) ) {
		physicsObj.SetGravity( physicsObj.GetGravityNormal() * gravityScale );
	}

	BecomeActive( TH_UPDATEVISUALS );
	Present();

	nextSelfCollisionTime	= gameLocal.time + SEC2MS( 5 ); // 5s spawn invulnerability to allow it to drop to the ground
	nextCollisionTime		= 0;
	nextCollisionSound		= 0;
	nextJumpSound			= 0;

	// NOTE: USE THIS ONLY TO LOCK DOWN THE HANDLING OF A VEHICLE!
	//		 Tweak the mass distrubtion in the vscript, and ONLY use this if you
	//		 need it to prevent collision model modifications upsetting the handling
	idVec3 IDiagonal;
	idVec3 IOther;
	bool hasIDiagonal = spawnArgs.GetVector( "do_not_modify_itd", "1 1 1", IDiagonal );
	bool hasIOther = spawnArgs.GetVector( "do_not_modify_ito", "0 0 0", IOther );

	if ( hasIDiagonal || hasIOther ) {
		if ( !hasIDiagonal ) {
			idMat3 originalITT = physicsObj.GetInertiaTensor();
			IDiagonal[0] = originalITT[0][0];
			IDiagonal[1] = originalITT[1][1];
			IDiagonal[2] = originalITT[2][2];
		}
		if ( !hasIOther ) {
			idMat3 originalITT = physicsObj.GetInertiaTensor();
			IOther[0] = originalITT[0][1];
			IOther[1] = originalITT[0][2];
			IOther[2] = originalITT[1][2];
		}

		idMat3 itt;
		itt[0][0] = IDiagonal[0];
		itt[1][1] = IDiagonal[1];
		itt[2][2] = IDiagonal[2];
		itt[0][1] = itt[1][0] = IOther[0];
		itt[0][2] = itt[2][0] = IOther[1];
		itt[1][2] = itt[2][1] = IOther[2];
		physicsObj.SetInertiaTensor( itt );
	}
}

/*
================
sdVehicle_RigidBody::Collide
================
*/
bool sdVehicle_RigidBody::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	// FIXME: This stuff should be done on collisions against us too.
	idVec3 normal = -collision.c.normal;
	float length = ( velocity * normal );
	idVec3 v = length * normal;

	if ( collideDamage ) {
		idEntity* other = gameLocal.entities[ collision.c.entityNum ];
		sdTransport* otherTransport = other->Cast< sdTransport >();
		const sdVehicleControlBase* otherControl = NULL;
		if ( otherTransport != NULL ) {
			otherControl = otherTransport->GetVehicleControl();
		}

		partDamageInfo_t damageInfo;	

		sdVehicleDriveObject* object = PartForCollisionById( collision, PFC_SELF_COLLISION );
		if ( object ) {
			damageInfo = *object->GetDamageInfo();
		} else {
			damageInfo.damageScale			= 1.f;
			damageInfo.collisionScale		= 1.f;
			damageInfo.collisionMinSpeed	= 256.f;
			damageInfo.collisionMaxSpeed	= 1024.f;
		}


		if( ( gameLocal.time > nextCollisionTime ) && ( length > damageInfo.collisionMinSpeed ) ) {

			float damage = ( length - damageInfo.collisionMinSpeed ) / ( damageInfo.collisionMaxSpeed - damageInfo.collisionMinSpeed );
			if ( damage > 1.f ) {
				damage = 1.f;
			}

			bool playerCollide = false;
			if ( other->IsType( idPlayer::Type ) ) {
				playerCollide = true;

				if ( collision.c.normal.z > 0.9f ) {
					// landed on their head, should crush them
					damage *= 4.f;
				}
			}

			bool otherIgnoreDamage = otherControl != NULL && otherControl->IgnoreCollisionDamage( -normal );
			if ( other->fl.takedamage && !otherIgnoreDamage ) {
				idPlayer* driver = positionManager.FindDriver();
				int oldHealth = other->GetHealth();
				other->Damage( this, driver != NULL ? ( idEntity* )driver : this, v, collideDamage, damage * collideDamageDealtScale, &collision );
				if ( driver != NULL ) {
					if ( oldHealth > 0 && other->GetHealth() <= 0 ) {
						idPlayer* otherPlayer = other->Cast< idPlayer >();
						if ( otherPlayer != NULL && driver->GetEntityAllegiance( otherPlayer ) == TA_ENEMY ) {
							IncRoadKillStats( driver );

							if ( driver->IsType( idBot::Type ) && !other->IsType( idBot::Type ) ) { //mal: smartass bot!
								clientInfo_t& driverInfo = botThreadData.GetGameWorldState()->clientInfo[ driver->entityNumber ];
								driverInfo.lastRoadKillTime = gameLocal.time + 3000;
							}
						}
					}
				}
			}

			bool ignoreDamage = GetVehicleControl() != NULL && GetVehicleControl()->IgnoreCollisionDamage( normal );
			if ( !ignoreDamage && !playerCollide && gameLocal.time > nextSelfCollisionTime ) {
				// HACK: take no collision damage from below if the other thing doesn't take any damage
				//		 this basically makes it so that you don't take annoying damage from scraping terrain
				float underneathNess = normal * GetPhysics()->GetAxis()[ 2 ];
				if ( other->fl.takedamage || underneathNess > collideDotLimit ) {
					if ( vehicleControl && IsCareening() ) {
						damage *= vehicleControl->GetCareeningCollideScale();
					}
					Damage( this, this, -v, collideDamage, damage * damageInfo.collisionScale, &collision );
				}
			}
		}

/*		if( g_debugDamage.GetInteger() ) {
			gameRenderWorld->DebugBox( colorGreen, idBox( collision.endpos, idVec3( 8, 8, 8 ), mat3_identity ), 3000 );
			gameRenderWorld->DebugLine( colorBlue, collision.endpos, collision.endpos + collision.c.normal * length, 3000 );

			gameRenderWorld->DebugBox( colorYellow, idBox( collision.c.point, idVec3( 8, 8, 8 ), mat3_identity ), 3000 );
			gameRenderWorld->DebugLine( colorBlue, collision.c.point, collision.c.point + collision.c.normal * length, 3000 );
		}*/
	}

	HandleCollision( collision, length );

	return sdTransport::Collide( collision, velocity, bodyId );
}

/*
================
sdVehicle_RigidBody::CollideFatal
================
*/
void sdVehicle_RigidBody::CollideFatal( idEntity* other ) {
	idEntity* driver = positionManager.FindDriver();
	idPlayer* playerDriver = NULL;
	
	if ( driver != NULL ) {
		playerDriver = driver->Cast< idPlayer >();
		assert( playerDriver != NULL );
	}

	if ( collideFatalDamage != NULL ) {
		if ( other->fl.takedamage ) {
			int oldHealth = other->GetHealth();
			idVec3 dir = GetPhysics()->GetOrigin() - other->GetPhysics()->GetOrigin();
			dir.Normalize();
			other->Damage( this, driver ? driver : this, dir, collideFatalDamage, -1.0f, NULL, true );

			if ( playerDriver != NULL ) {
				idPlayer* otherPlayer = other->Cast< idPlayer >();
				if ( otherPlayer != NULL && otherPlayer->GetEntityAllegiance( playerDriver ) == TA_ENEMY ) {
					if ( oldHealth > 0 && otherPlayer->GetHealth() <= 0 ) {
						IncRoadKillStats( playerDriver );
					}
				}
			}
		}
	}
}

/*
================
sdVehicle_RigidBody::IncRoadKillStats
================
*/
void sdVehicle_RigidBody::IncRoadKillStats( idPlayer* player ) {
	sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();
	sdPlayerStatEntry* entry = tracker.GetStat( tracker.AllocStat( "total_roadkills", sdNetStatKeyValue::SVT_INT ) );
	entry->IncreaseValue( player->entityNumber, 1 );
}

/*
================
sdVehicle_RigidBody::UpdateAnimationControllers
================
*/
bool sdVehicle_RigidBody::UpdateAnimationControllers( void ) {
	if ( !gameLocal.isNewFrame ) {
		return false;
	}

	if ( ik.IsInitialized() ) {
		if ( !ik.IsInhibited() ) {
			return ik.Evaluate();
		}
		return false;
	} else {
		ik.ClearJointMods();
	}

	return false;
}

/*
================
sdVehicle_RigidBody::HandleCollision
================
*/
void sdVehicle_RigidBody::HandleCollision( const trace_t &collision, const float velocity ) {
	idVec3 bodyOrigin;
	idVec3 localCollisionPoint;

	physicsObj.GetBodyOrigin( bodyOrigin, collision.c.selfId );

	localCollisionPoint = ( collision.c.point - bodyOrigin ) * physicsObj.GetAxis().Transpose();

	// get dot with forward facing direction
	idVec3 localCollisionNormal = collision.c.normal * physicsObj.GetAxis().Transpose();
	idVec3 v( 1, 0, 0 );
	float dot = v * localCollisionNormal;

	sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( collision );

	const idBounds& bodyBounds = physicsObj.GetBounds( collision.c.selfId );
	const idVec3 bodySize = bodyBounds.Size();

#if 0
	bool foundCollisionPoint = false;

	// sides
	if ( localCollisionPoint.x > 0 && localCollisionPoint.y > 0 ) {
		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			gameLocal.Printf( "Collision with front left side. (dot = %f)\n", idMath::Fabs( dot ) );

/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.y = ( .5f * bodySize.y ) - 2.f;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( idMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( idMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = .5f * bodySize.x - collisionExtents.x;
				} else {
					collisionCenter.x = .25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			idVec3 mins, maxs;
			mins.y = .5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.x = .5f * bodySize.x;
			maxs.y = .5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			idVec3 mins, maxs;
			mins.y = -.5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.x = .5f * bodySize.x;
			maxs.y = -.5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x > 0 && localCollisionPoint.y < 0 ) {
		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			gameLocal.Printf( "Collision with front right side. (dot = %f)\n", idMath::Fabs( dot ) );

/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.y = ( -.5f * bodySize.y ) + 2.f;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( idMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( idMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = .5f * bodySize.x - collisionExtents.x;
				} else {
					collisionCenter.x = .25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			idVec3 mins, maxs;
			mins.x = -.5f * bodySize.x;
			mins.y = .5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.y = .5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x < 0 && localCollisionPoint.y > 0 ) {
		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			gameLocal.Printf( "Collision with rear left side. (dot = %f)\n", idMath::Fabs( dot ) );

/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.y = ( .5f * bodySize.y ) - 2.f;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( idMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( idMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = -.5f * bodySize.x + collisionExtents.x;
				} else {
					collisionCenter.x = -.25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			idVec3 mins, maxs;
			mins.x = -.5f * bodySize.x;
			mins.y = -.5f * bodySize.y;
			mins.z = -.5f * bodySize.z;

			maxs.y = -.5f * bodySize.y;
			maxs.z = .5f * bodySize.z;

			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		}
	}
	if ( localCollisionPoint.x < 0 && localCollisionPoint.y < 0 ) {
		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			gameLocal.Printf( "Collision with rear right side. (dot = %f)\n", idMath::Fabs( dot ) );

/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.y = ( -.5f * bodySize.y ) + 2.f;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				if ( idMath::Fabs( dot ) > .06f ) {
					float fraction = 1.f - ( ( idMath::Fabs( dot ) - .06f ) / .09f );

					collisionExtents.x = .0625f * bodySize.x + fraction * .1875f * bodySize.x;
					collisionCenter.x = -.5f * bodySize.x + collisionExtents.x;
				} else {
					collisionCenter.x = -.25f * bodySize.x;
					collisionExtents.x = .25f * bodySize.x;
				}

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorRed, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		} else {
			// just generate a small box around the collision point
/*			if ( g_debugDamage.GetInteger() ) {
				idVec3 collisionCenter;
				collisionCenter.x = localCollisionPoint.x;
				collisionCenter.y = localCollisionPoint.y;
				collisionCenter.z = 0.f;

				idVec3 collisionExtents;
				collisionExtents.x = 2.f;
				collisionExtents.y = 2.f;
				collisionExtents.z = .5f * bodySize.z;

				idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

				// move into world
				collisionBox.RotateSelf( physicsObj.GetAxis() );
				collisionBox.TranslateSelf( bodyOrigin );

				gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
			}*/

			foundCollisionPoint = true;
		}
	}

	// top
	if ( !foundCollisionPoint && localCollisionPoint.z > ( .5f * bodySize.z ) - 2.f ) {
		// just generate a small box around the collision point
/*		if ( g_debugDamage.GetInteger() ) {
			idVec3 collisionCenter;
			collisionCenter.x = localCollisionPoint.x;
			collisionCenter.y = localCollisionPoint.y;
			collisionCenter.z = localCollisionPoint.z;

			idVec3 collisionExtents;
			collisionExtents.x = 2.f;
			collisionExtents.y = 2.f;
			collisionExtents.z = localCollisionPoint.z;

			idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

			// move into world
			collisionBox.RotateSelf( physicsObj.GetAxis() );
			collisionBox.TranslateSelf( bodyOrigin );

			gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
		}*/

		foundCollisionPoint = true;
	}

	// bottom
	if ( !foundCollisionPoint && localCollisionPoint.z < ( -.5f * bodySize.z ) + 2.f ) {
		// just generate a small box around the collision point
/*		if ( g_debugDamage.GetInteger() ) {
			idVec3 collisionCenter;
			collisionCenter.x = localCollisionPoint.x;
			collisionCenter.y = localCollisionPoint.y;
			collisionCenter.z = localCollisionPoint.z;

			idVec3 collisionExtents;
			collisionExtents.x = 2.f;
			collisionExtents.y = 2.f;
			collisionExtents.z = localCollisionPoint.z;

			idBox collisionBox( collisionCenter, collisionExtents, mat3_identity );

			// move into world
			collisionBox.RotateSelf( physicsObj.GetAxis() );
			collisionBox.TranslateSelf( bodyOrigin );

			gameRenderWorld->DebugBox( colorCyan, collisionBox, 3000 );
		}*/

		foundCollisionPoint = true;
	}
#endif

	bool scrape = false;
	idVec3 mins, maxs;

	// scraping
	bool front;

	if ( localCollisionPoint.x > 0 ) {
		front = true;

		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ) {
			scrape = true;
		}
	} else {
		front = false;

		if ( idMath::Fabs( dot ) < .15f && localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ) {
			scrape = true;
		}
	}

	if ( scrape ) {
		mins.y = maxs.y = ( localCollisionPoint.y > 0 ? 1 : -1 ) * .5f * bodySize.y;

		if ( front ) {
			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				mins.x = fraction * ( .4375f * bodySize.x );
			} else {
				mins.x = 0.f;
			}

			maxs.x = .5f * bodySize.x;
		} else {
			mins.x = -.5f * bodySize.x;

			if ( idMath::Fabs( dot ) > .06f ) {
				float fraction = ( idMath::Fabs( dot ) - .06f ) / .09f;
				maxs.x = -fraction * ( .4375f * bodySize.x );
			} else {
				maxs.x = 0.f;
			}
		}

		mins.z = -.5f * bodySize.z;
		maxs.z = .5f * bodySize.z;
	} else {
		// not scraping
        mins.x = maxs.x = localCollisionPoint.x;
		mins.y = maxs.y = localCollisionPoint.y;

		if ( localCollisionPoint.x > ( .5f * bodySize.x ) - 2.f ||
			 localCollisionPoint.x < ( -.5f * bodySize.x ) + 2.f ||
			 localCollisionPoint.y > ( .5f * bodySize.y ) - 2.f ||
			 localCollisionPoint.x < ( -.5f * bodySize.y ) + 2.f ) {
			 // sides
			mins.z = -.5f * bodySize.z;
			maxs.z = .5f * bodySize.z;
		} else {
		//if ( ( localCollisionPoint.z > ( .5f * bodySize.z ) - 2.f ) || ( localCollisionPoint.z < ( -.5f * bodySize.z ) + 2.f ) ) {
			// top/bottom
			mins.z = maxs.z = localCollisionPoint.z;
		}
	}

	// transform to world
	mins *= physicsObj.GetAxis();
	mins += bodyOrigin;

	maxs *= physicsObj.GetAxis();
	maxs += bodyOrigin;

	if ( onCollisionFunc ) {
		sdScriptHelper helper;
		helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
		helper.Push( velocity );
		helper.Push( mins );
		helper.Push( maxs );
		CallNonBlockingScriptEvent( onCollisionFunc, helper );
	}

	if ( scrape ) {
		if ( onCollisionSideScrapeFunc ) {
			sdScriptHelper helper;
			helper.Push( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
			helper.Push( velocity );
			helper.Push( mins );
			helper.Push( maxs );
			CallNonBlockingScriptEvent( onCollisionSideScrapeFunc, helper );
		}
	}

	gameLocal.FreeLoggedTrace( loggedTrace );
}

/*
================
sdVehicle_RigidBody::FreezePhysics
================
*/
void sdVehicle_RigidBody::FreezePhysics( bool freeze ) {
	physicsObj.SetFrozen( freeze );
	physicsObj.SetLinearVelocity( vec3_origin );
	physicsObj.SetAngularVelocity( vec3_origin );
}

/*
================
sdVehicle_RigidBody::Event_SetDamageDealtScale
================
*/
void sdVehicle_RigidBody::Event_SetDamageDealtScale( float scale ) {
	SetDamageDealtScale( scale );
}

/*
================
sdVehicle_RigidBody::SetDamageDealtScale
================
*/
void sdVehicle_RigidBody::SetDamageDealtScale( float scale ) {
	collideDamageDealtScale = scale;
}
