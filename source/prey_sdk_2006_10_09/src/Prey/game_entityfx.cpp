#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

CLASS_DECLARATION( idEntityFx, hhEntityFx )
	EVENT( EV_Activate,	   	hhEntityFx::Event_Trigger )
	EVENT( EV_Fx_KillFx,	hhEntityFx::Event_ClearFx )
END_CLASS

/*
================
hhEntityFx::hhEntityFx
================
*/
hhEntityFx::hhEntityFx() {
	// HUMANHEAD nla
	setFxInfo = false;
	// HUMANHEAD

// HUMANHEAD bg
	restartActive = false;
// HUMANHEAD END
}

/*
================
hhEntityFx::~hhEntityFx
================
*/
hhEntityFx::~hhEntityFx() {
	//HUMANHEAD: aob - need to shut everything down
	Event_ClearFx();
	//HUMANHEAD END
}

void hhEntityFx::Save(idSaveGame *savefile) const {
	savefile->WriteStaticObject( fxInfo );
	savefile->WriteBool( setFxInfo );
	savefile->WriteBool( removeWhenDone );
// HUMANHEAD bg
	savefile->WriteBool( restartActive );
// HUMANHEAD END
}

void hhEntityFx::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( fxInfo );
	savefile->ReadBool( setFxInfo );
	savefile->ReadBool( removeWhenDone );
// HUMANHEAD bg
	savefile->ReadBool( restartActive );
// HUMANHEAD END
}

void hhEntityFx::WriteToSnapshot( idBitMsgDelta &msg ) const {
	idEntityFx::WriteToSnapshot(msg);

	fxInfo.WriteToSnapshot(msg);
}

void hhEntityFx::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	idEntityFx::ReadFromSnapshot(msg);

	fxInfo.ReadFromSnapshot(msg);
}

/*
================
hhEntityFx::CleanUpSingleAction
================
*/
void hhEntityFx::CleanUpSingleAction( const idFXSingleAction& fxaction, idFXLocalAction& laction ) {
	idEntityFx::CleanUpSingleAction( fxaction, laction );

	// HUMANHEAD pdm: sound "duration" support
	if ( fxaction.type == FX_SOUND && fxaction.sibling == -1 && laction.soundStarted ) {
		const idSoundShader *def = declManager->FindSound( fxaction.data );
		refSound.referenceSound->StopSound( SND_CHANNEL_ANY );
	}

	if ( fxaction.type == FX_PARTICLE && fxaction.sibling == -1 ) {
		laction.particleSystem = NULL;
		laction.particleStartTime = -1;
	}
	// HUMANHEAD END
}

/*
================
hhEntityFx::Hide
================
*/
void hhEntityFx::Hide() {
	CleanUp();
}

/*
================
hhEntityFx::Run
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
================
*/
void hhEntityFx::Run( int time ) {
	int ieff, j;
	idEntity *ent = NULL;
	const idDict *projectileDef = NULL;
	idProjectile *projectile = NULL;

	if ( !fxEffect || IsHidden() ) {
		return;
	}

	for( ieff = 0; ieff < fxEffect->events.Num(); ieff++ ) {
		const idFXSingleAction& fxaction = fxEffect->events[ieff];
		idFXLocalAction& laction = actions[ieff];

		//
		// if we're currently done with this one
		//
		if ( laction.start == -1 ) {
			continue;
		}

		//
		// see if it's delayed
		//
		if ( laction.delay ) {
			if ( laction.start + (time - laction.start) < laction.start + (laction.delay * 1000) ) {
				continue;
			}
		}

		//
		// each event can have it's own delay and restart
		//
		int actualStart = laction.delay ? laction.start + (int)( laction.delay * 1000 ) : laction.start;
		float pct = (float)( time - actualStart ) / (1000 * fxaction.duration );
		if ( pct >= 1.0f ) {
			laction.start = -1;
			float totalDelay = 0.0f;
			if ( fxaction.restart ) {
				if ( fxaction.random1 || fxaction.random2 ) {
					totalDelay = fxaction.random1 + gameLocal.random.RandomFloat() * (fxaction.random2 - fxaction.random1);
				} else {
					totalDelay = fxaction.delay;
				}
				laction.delay = totalDelay;
				laction.start = time;
			} 
			continue;
		}

		if ( fxaction.fire.Length() ) {
			for( j = 0; j < fxEffect->events.Num(); j++ ) {
				if ( fxEffect->events[j].name.Icmp( fxaction.fire ) == 0 ) {
					actions[j].delay = 0;
				}
			}
		}

		idFXLocalAction *useAction = NULL;
		if ( fxaction.sibling == -1 ) {
			useAction = &laction;
		} else {
			useAction = &actions[fxaction.sibling];
		}
		assert( useAction );

		switch( fxaction.type ) {
			case FX_ATTACHLIGHT:
			case FX_LIGHT: {
				if ( useAction->lightDefHandle == -1 ) {
					if ( fxaction.type == FX_LIGHT ) {
						memset( &useAction->renderLight, 0, sizeof( renderLight_t ) );
						//HUMANHEAD: bjk once again fixing aob code
						useAction->renderLight.axis = DetermineAxis( fxaction );
						useAction->renderLight.origin = GetOrigin() + fxaction.offset * useAction->renderLight.axis;
						useAction->renderLight.axis = hhUtils::SwapXZ( useAction->renderLight.axis );
						//HUMANHEAD END
						useAction->renderLight.lightRadius[0] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[1] = fxaction.lightRadius;
						useAction->renderLight.lightRadius[2] = fxaction.lightRadius;
						useAction->renderLight.shader = declManager->FindMaterial( fxaction.data, false );
						useAction->renderLight.shaderParms[ SHADERPARM_RED ]	= fxaction.lightColor.x;
						useAction->renderLight.shaderParms[ SHADERPARM_GREEN ]	= fxaction.lightColor.y;
						useAction->renderLight.shaderParms[ SHADERPARM_BLUE ]	= fxaction.lightColor.z;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;
						useAction->renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( time );
						useAction->renderLight.referenceSound = refSound.referenceSound;
						useAction->renderLight.pointLight = true;
						if ( fxaction.noshadows ) {
							useAction->renderLight.noShadows = true;
						}
						useAction->lightDefHandle = gameRenderWorld->AddLightDef( &useAction->renderLight );
					}
					if ( fxaction.noshadows ) {
						for( j = 0; j < fxEffect->events.Num(); j++ ) {
							idFXLocalAction& laction2 = actions[j];
							if ( laction2.modelDefHandle != -1 ) {
								laction2.renderEntity.noShadow = true;
							}
						}
					}
				} else if ( fxaction.trackOrigin ) {
					//HUMANHEAD: bjk
					useAction->renderLight.axis = DetermineAxis( fxaction );
					useAction->renderLight.origin = GetOrigin() + fxaction.offset * useAction->renderLight.axis;
					useAction->renderLight.axis = hhUtils::SwapXZ( useAction->renderLight.axis );
					gameRenderWorld->UpdateLightDef( useAction->lightDefHandle, &useAction->renderLight );
					//HUMANHEAD END
				}
				ApplyFade( fxaction, *useAction, time, actualStart );
				break;
		   }
			case FX_SOUND: {
				if ( !useAction->soundStarted ) {
					useAction->soundStarted = true;
					const idSoundShader *shader = declManager->FindSound(fxaction.data);
					StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, NULL );
					for( j = 0; j < fxEffect->events.Num(); j++ ) {
						idFXLocalAction& laction2 = actions[j];
						if ( laction2.lightDefHandle != -1 ) {
							laction2.renderLight.referenceSound = refSound.referenceSound;
							gameRenderWorld->UpdateLightDef( laction2.lightDefHandle, &laction2.renderLight );
						}
					}
				}
				break;
			}
			case FX_DECAL: {
				if ( !useAction->decalDropped ) {
					useAction->decalDropped = true;
					// HUMANHEAD pdm: Increased depth to from 8 to 25
					//TODO: Expose depth and parrallel to the FX files
					gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetGravity(), 25.0f, true, fxaction.size, fxaction.data ); 
				}
				break;
			}
			case FX_SHAKE: {
				if ( !useAction->shakeStarted ) {
					idDict args;
					args.Clear();
					args.SetFloat( "kick_time", fxaction.shakeTime );
					args.SetFloat( "kick_amplitude", fxaction.shakeAmplitude );
					for ( j = 0; j < gameLocal.numClients; j++ ) {
						idPlayer *player = gameLocal.GetClientByNum( j );
						if ( player && ( player->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() ).LengthSqr() < Square( fxaction.shakeDistance ) ) {
							if ( !gameLocal.isMultiplayer || !fxaction.shakeIgnoreMaster || GetBindMaster() != player ) {
								player->playerView.DamageImpulse( fxaction.offset, &args );
							}
						}
					}
					if ( fxaction.shakeImpulse != 0.0f && fxaction.shakeDistance != 0.0f ) {
						idEntity *ignore_ent = NULL;
						if ( gameLocal.isMultiplayer ) {
							ignore_ent = this;
							if ( fxaction.shakeIgnoreMaster ) {
								ignore_ent = GetBindMaster();
							}
						}
						// lookup the ent we are bound to?
						gameLocal.RadiusPush( GetPhysics()->GetOrigin(), fxaction.shakeDistance, fxaction.shakeImpulse, this, ignore_ent, 1.0f, true );
					}
					useAction->shakeStarted = true;
				}
				break;
			}
			case FX_ATTACHENTITY:
			case FX_MODEL: {
				if ( useAction->modelDefHandle == -1 ) {
					memset( &useAction->renderEntity, 0, sizeof( renderEntity_t ) );
					//HUMANHEAD: aob - rotated offset by axis
					useAction->renderEntity.axis = DetermineAxis( fxaction );
					useAction->renderEntity.origin = GetOrigin() + fxaction.offset * useAction->renderEntity.axis;
					useAction->renderEntity.axis = hhUtils::SwapXZ( useAction->renderEntity.axis );
					useAction->renderEntity.weaponDepthHack = renderEntity.weaponDepthHack;
					useAction->renderEntity.onlyVisibleInSpirit = renderEntity.onlyVisibleInSpirit;
					useAction->renderEntity.onlyInvisibleInSpirit = renderEntity.onlyInvisibleInSpirit;	// tmj
					useAction->renderEntity.allowSurfaceInViewID = renderEntity.allowSurfaceInViewID;	// bjk
					//HUMANHEAD END
					useAction->renderEntity.hModel = renderModelManager->FindModel( fxaction.data );
					// HUMANHEAD pdm: allow color on fx, but they'll be the same for all particles
					if ( renderEntity.shaderParms[SHADERPARM_RED] != 0.0f ||
						 renderEntity.shaderParms[SHADERPARM_GREEN] != 0.0f ||
						 renderEntity.shaderParms[SHADERPARM_BLUE] != 0.0f) {
						useAction->renderEntity.shaderParms[ SHADERPARM_RED ]		= renderEntity.shaderParms[SHADERPARM_RED];
						useAction->renderEntity.shaderParms[ SHADERPARM_GREEN ]		= renderEntity.shaderParms[SHADERPARM_GREEN];
						useAction->renderEntity.shaderParms[ SHADERPARM_BLUE ]		= renderEntity.shaderParms[SHADERPARM_BLUE];
					}
					else {
						useAction->renderEntity.shaderParms[ SHADERPARM_RED ]		= 1.0f;
						useAction->renderEntity.shaderParms[ SHADERPARM_GREEN ]		= 1.0f;
						useAction->renderEntity.shaderParms[ SHADERPARM_BLUE ]		= 1.0f;
					}
					useAction->renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( time );
					useAction->renderEntity.shaderParms[3] = 1.0f;
					useAction->renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] = gameLocal.random.RandomFloat();	//HUMANHEAD bjk
					if ( useAction->renderEntity.hModel ) {
						useAction->renderEntity.bounds = useAction->renderEntity.hModel->Bounds( &useAction->renderEntity );
					}
					useAction->modelDefHandle = gameRenderWorld->AddEntityDef( &useAction->renderEntity );
				} else if ( fxaction.trackOrigin ) {
					//HUMANHEAD: aob - rotated offset by axis
					useAction->renderEntity.axis = DetermineAxis( fxaction );
					if (fxaction.offset.x || fxaction.offset.y || fxaction.offset.z) {
						useAction->renderEntity.origin = GetOrigin() + fxaction.offset * useAction->renderEntity.axis;
					}
					else {
						useAction->renderEntity.origin = GetOrigin();
					}
					useAction->renderEntity.axis = hhUtils::SwapXZ( useAction->renderEntity.axis );
					gameRenderWorld->UpdateEntityDef( useAction->modelDefHandle, &useAction->renderEntity );
					//HUMANHEAD END
				}
				ApplyFade( fxaction, *useAction, time, actualStart );
				break;
			}
			case FX_PARTICLE: {
				//HUMANHEAD: aob
				//rww - kind of a hack, but otherwise the timing is off, which causes issues in mp.
				if (useAction->particleStartTime == -2) {
					useAction->particleStartTime = gameLocal.time;
				}

				if( !useAction->particleSystem && fxaction.data.Length() ) {
					useAction->particleSystem = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, fxaction.data, false ) );
				} else if( useAction->particleStartTime >= 0 && useAction->particleSystem ) {
					idMat3 axis = DetermineAxis( fxaction );
					if( !gameLocal.smokeParticles->EmitSmoke(useAction->particleSystem, useAction->particleStartTime, gameLocal.random.RandomFloat(), GetOrigin() + fxaction.offset * axis, hhUtils::SwapXZ(axis)) ) {
						useAction->particleStartTime = -1;
					}
				}
				// HUMANHEAD END
				break;
			}
			case FX_LAUNCH: {
				//HUMANHEAD rww - if this is a client ent then assert.
				assert(!fl.clientEntity);

				if ( gameLocal.isClient ) {
					// client never spawns entities outside of ClientReadSnapshot
					useAction->launched = true;
					break;
				}
				if ( !useAction->launched ) {
					useAction->launched = true;
					projectile = NULL;
					// FIXME: may need to cache this if it is slow
					projectileDef = gameLocal.FindEntityDefDict( fxaction.data, false );
					if ( !projectileDef ) {
						gameLocal.Warning( "projectile \'%s\' not found", fxaction.data.c_str() );
					} else {
						gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
						if ( ent && ent->IsType( idProjectile::Type ) ) {
							projectile = ( idProjectile * )ent;
							projectile->Create( this, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0] );
							projectile->Launch( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[0], vec3_origin );
						}
					}
				}
				break;
			}
		}
	}
}

/*
================
hhEntityFx::DetermineAxis

HUMANHEAD: aob
================
*/
idMat3 hhEntityFx::DetermineAxis( const idFXSingleAction& fxaction ) {
	idVec3 fxDir;

	if( fxaction.explicitAxis ) {
		if (fxaction.useAxis == AXIS_CUSTOMLOCAL) {
			// When customlocal is set, axis is the specifed axis, untransformed by the entities axis
			return fxaction.dir.ToMat3();
		}
		return (fxaction.dir[0] * GetAxis()[0] + fxaction.dir[1] * GetAxis()[1] + fxaction.dir[2] * GetAxis()[2]).ToMat3();
	} else if( fxInfo.GetAxisFor(fxaction.useAxis, fxDir) ) {
		return fxDir.ToMat3();
	}

	return GetAxis();
}

/*
================
hhEntityFx::Nozzle

HUMANHEAD: aob - used to toggle particle systems
================
*/
void hhEntityFx::Nozzle( bool bOn ) {
	if ( !fxEffect ) {
		return;
	}

	if( bOn && !IsActive( TH_THINK ) ) {
		Event_Trigger( NULL );
	}

	if( !bOn ) {
		//Canceling events incase we are toggled before our actvate event has fired.
		CancelEvents( &EV_Activate );

		if( IsActive( TH_THINK ) ) {
			Event_ClearFx();
		}
	}
}

/*
================
hhEntityFx::DormantBegin

//HUMANHEAD: aob
================
*/
void hhEntityFx::DormantBegin() {
	idEntityFx::DormantBegin();

	//HUMANHEAD: aob - used CleanUp directly so 'started' doesn't get reset
	//Stop();
	CleanUp();
	//HUMANHEAD END

	//want to make sure we can trigger these back on
	nextTriggerTime = -1;
}

/*
================
hhEntityFx::DormantEnd

//HUMANHEAD: aob
================
*/
void hhEntityFx::DormantEnd() {
	idEntityFx::DormantEnd();

	if ( started >= 0 ) {
		CreateFx( this );
	}
}

/*
================
hhEntityFx::Event_Trigger
================
*/
void hhEntityFx::Event_Trigger( idEntity *activator ) {
	if ( gameLocal.time < nextTriggerTime ) {
		return;
	}

// HUMANHEAD bg: Special handling for "restart" fx enable/disable.
	if ( restartActive && (activator != this) ) {
		CancelEvents( &EV_Fx_Action );
		CancelEvents( &EV_Activate );
		CancelEvents( &EV_Fx_KillFx );
		Stop();
		CleanUp();
		BecomeInactive( TH_THINK );
		restartActive = false;
		return;
	}
	if ( !restartActive && spawnArgs.GetFloat( "restart" ) ) {
		restartActive = true;
	}
// HUMANHEAD END

	//HUMANHEAD: aob
	if( spawnArgs.GetBool("toggle") && IsActive(TH_THINK) ) {
		ProcessEvent( &EV_Fx_KillFx );
		return;
	}
	//HUMANHEAD END

	//HUMANHEAD: aob - moved logic to helper function so I can call code specifically
	CreateFx( activator );
	//HUMANHEAD END
}

/*
================
hhEntityFx::CreateFx

HUMANHEAD: aob
================
*/
void hhEntityFx::CreateFx( idEntity *activator ) {
	if ( g_skipFX.GetBool() ) {
		return;
	}

	float		fxActionDelay;
	const char *fx;

	if ( spawnArgs.GetString( "fx", "", &fx) ) {
		Setup( fx );
		Start( gameLocal.time );
		//HUMANHEAD: aob - added so fx can be deleted at end of duration
		if( RemoveWhenDone() ) {
			PostEventMS( &EV_Remove, Duration() );
		} else {
			PostEventMS( &EV_Fx_KillFx, Duration() );
		}
		//HUMANHEAD END
		BecomeActive( TH_THINK );
	}

	fxActionDelay = spawnArgs.GetFloat( "fxActionDelay" );
	if ( fxActionDelay != 0.0f ) {
		nextTriggerTime = gameLocal.time + SEC2MS( fxActionDelay );
	} else {
		// prevent multiple triggers on same frame
		nextTriggerTime = gameLocal.time + 1;
	}
	PostEventSec( &EV_Fx_Action, fxActionDelay, activator );
}

/*
================
hhEntityFx::StartFx

creates an fx entity, ONLY CALL THIS ON THE SERVER. -rww
================
*/
hhEntityFx *hhEntityFx::StartFx( const char *fx, const idVec3 *useOrigin, const idMat3 *useAxis, idEntity *ent, bool bind )
{

	if ( g_skipFX.GetBool() || !fx || !*fx ) {
		return NULL;
	}

	idDict args;
	args.SetBool( "start", true );
	args.Set( "fx", fx );
	hhEntityFx *nfx = static_cast<hhEntityFx *>( gameLocal.SpawnEntityType( hhEntityFx::Type, &args ) );
	if ( nfx->Joint() && *nfx->Joint() ) {
		nfx->BindToJoint( ent, nfx->Joint(), true );
		nfx->SetOrigin( vec3_origin );
	} else {
		nfx->SetOrigin( (useOrigin) ? *useOrigin : ent->GetPhysics()->GetOrigin() );
		nfx->SetAxis( (useAxis) ? *useAxis : ent->GetPhysics()->GetAxis() );
	}

	if ( bind ) {
		// never bind to world spawn
		if ( ent != gameLocal.world ) {
			nfx->Bind( ent, true );
		}
	}
	nfx->Show();
	return nfx;
}

/*
================
hhEntityFx::SetParticleShaderParm
  Allow setting shader parms directly to particle renderentities
  Note that this functionality isn't set to the default SetShaderParm()
  to avoid breaking any existing code
================
*/
void hhEntityFx::SetParticleShaderParm( int parmnum, float value ) {

	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	for( int ieff = 0; ieff < fxEffect->events.Num(); ieff++ ) {
		const idFXSingleAction& fxaction = fxEffect->events[ieff];
		idFXLocalAction *useAction = NULL;
		idFXLocalAction& laction = actions[ieff];
		if ( fxaction.sibling == -1 ) {
			useAction = &laction;
		} else {
			useAction = &actions[fxaction.sibling];
		}
		if ( !useAction ) {
			continue;
		}
		switch( fxaction.type ) {
			case FX_MODEL: {
				useAction->renderEntity.shaderParms[parmnum] = value;
				break;
			}
		}
	}
}

/*
================
hhEntityFx::Event_ClearFx

  Clears any visual fx started when item(mob) was spawned
================
*/
void hhEntityFx::Event_ClearFx( void ) {

	if ( g_skipFX.GetBool() ) {
		return;
	}

	Stop();
	CleanUp();
	BecomeInactive( TH_THINK );

	if ( spawnArgs.GetBool("test") ) {
		PostEventMS( &EV_Activate, 0, this );
	} else {
		if (!spawnArgs.GetBool("triggered")) {
			float rest = spawnArgs.GetInt( "restart", "0" );
			if ( rest == 0.0f ) {
				//HUMANHEAD: aob - added RemoveWhenDone and CancelEvents call
				if( RemoveWhenDone() ) {
					PostEventSec( &EV_Remove, 0.1f );
				} else {
					CancelEvents( &EV_Fx_Action );
				}
				//HUMANHEAD END
			} else {
				rest *= gameLocal.random.RandomFloat();
// HUMANHEAD bg: Give ability for minimum restart time by adding offset.
				rest += spawnArgs.GetFloat( "restartDelay", "0" );
// HUMANHEAD END
				PostEventSec( &EV_Activate, rest, this );
			}
		}
	}
}
