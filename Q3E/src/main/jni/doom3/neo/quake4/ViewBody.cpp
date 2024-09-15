#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#include "Weapon.h"
#include "Projectile.h"
#include "ai/AI.h"
#include "ai/AI_Manager.h"
#include "client/ClientEffect.h"
#include "ViewBody.h"
#include "Player.h"
//#include "../renderer/tr_local.h"

#define viewAnimator GetAnimator()
#define PostAnimState PostState

#define ANIMCHANNEL_BODY (allChannel ? ANIMCHANNEL_ALL : ANIMCHANNEL_LEGS)
#define CHECK_MODEL(); if ( !animator.ModelHandle() ) return;

CLASS_STATES_DECLARATION ( idViewBody )

    // Wait States
    STATE ( "Wait_Alive",					idViewBody::State_Wait_Alive )

    // Leg States
    STATE ( "Legs_Idle",					idViewBody::State_Legs_Idle )
    STATE ( "Legs_Crouch",					idViewBody::State_Legs_Crouch )
    STATE ( "Legs_Uncrouch",				idViewBody::State_Legs_Uncrouch )
    STATE ( "Legs_Run_Forward",				idViewBody::State_Legs_Run_Forward )
    STATE ( "Legs_Run_Backward",			idViewBody::State_Legs_Run_Backward )
    STATE ( "Legs_Run_Left",				idViewBody::State_Legs_Run_Left )
    STATE ( "Legs_Run_Right",				idViewBody::State_Legs_Run_Right )
    STATE ( "Legs_Walk_Forward",			idViewBody::State_Legs_Walk_Forward )
    STATE ( "Legs_Walk_Backward",			idViewBody::State_Legs_Walk_Backward )
    STATE ( "Legs_Walk_Left",				idViewBody::State_Legs_Walk_Left )
    STATE ( "Legs_Walk_Right",				idViewBody::State_Legs_Walk_Right )
    STATE ( "Legs_Crouch_Idle",				idViewBody::State_Legs_Crouch_Idle )
    STATE ( "Legs_Crouch_Forward",			idViewBody::State_Legs_Crouch_Forward )
    STATE ( "Legs_Crouch_Backward",			idViewBody::State_Legs_Crouch_Backward )
    STATE ( "Legs_Fall",					idViewBody::State_Legs_Fall )
    STATE ( "Legs_Jump",					idViewBody::State_Legs_Jump )
    STATE ( "Legs_Fall",					idViewBody::State_Legs_Fall )
    STATE ( "Legs_Land",					idViewBody::State_Legs_Land )
    STATE ( "Legs_Dead",					idViewBody::State_Legs_Dead )

END_CLASS_STATES

/***********************************************************************

  idViewBody

***********************************************************************/

// class def
CLASS_DECLARATION( idAnimatedEntity, idViewBody )
END_CLASS

/***********************************************************************

	init

***********************************************************************/

/*
================
idViewBody::idViewBody()
================
*/
idViewBody::idViewBody() {
    modelDefHandle		= -1;
    owner				= NULL;
	usePlayerModel		= false;
    bodyOffset.Zero();
	allChannel			= true;
    Clear();

    memset ( &animDoneTime, 0, sizeof(animDoneTime) );

    fl.networkSync = true;
}

/*
================
idViewBody::~idViewBody()
================
*/
idViewBody::~idViewBody() {
    Clear();
}

/*
================
idViewBody::Spawn
================
*/
void idViewBody::Spawn( void ) {
    GetPhysics()->SetContents( 0 );
    GetPhysics()->SetClipMask( 0 );
    GetPhysics()->SetClipModel( NULL, 1.0f );

    stateThread.SetName( va("%s_%s", GetName ( ), spawnArgs.GetString("classname") ) );
    stateThread.SetOwner( this );

	idStr surfaces;
	hideSurfaces.Clear();
	spawnArgs.GetString("body_hidesurfaces", "", surfaces);
	if(surfaces.Length() > 0)
		SplitSurfaces(surfaces, hideSurfaces);
	const idKeyValue* kv;
	for ( kv = spawnArgs.MatchPrefix ( "hidesurface", NULL );
		  kv;
		  kv = spawnArgs.MatchPrefix ( "hidesurface", kv ) ) {
		hideSurfaces.Append ( kv->GetValue() );
	}

	usePlayerModel = spawnArgs.GetBool("body_usePlayerModel", "0");
	if(!usePlayerModel)
	{
		const char *modelName = spawnArgs.GetString("body_model");
		SetViewModel(modelName);
	}
    spawnArgs.GetVector("body_offset", "0 0 0", bodyOffset);
    allChannel = spawnArgs.GetBool("body_allChannel", "1");
}

/*
================
idViewBody::Init
================
*/
void idViewBody::Init( idPlayer* _owner, bool setstate )
{
    owner = _owner;
    stateThread.SetName( va("%s_%s_%s", owner->GetName(), GetName ( ), spawnArgs.GetString("classname") ) );
    stateThread.SetOwner( this );

    renderEntity.suppressSurfaceInViewID = owner->entityNumber + 1;
    renderEntity.suppressSurfaceInViewID = 0;
    renderEntity.noShadow = true;
    renderEntity.noSelfShadow = true;

	if(usePlayerModel)
	{
		const char *modelName = owner->spawnArgs.GetString("model");
		SetViewModel(modelName);
	}
    if(setstate)
		SetState ( "Legs_Idle", 0 );
}

/*
================
idViewBody::Save
================
*/
void idViewBody::Save( idSaveGame *savefile ) const {
    int						i;

	savefile->WriteObject	( owner );
	savefile->WriteInt	    ( hideSurfaces.Num() );
    for ( i = 0; i < hideSurfaces.Num(); i++ ) {
        savefile->WriteString( hideSurfaces[i] );
    }
	savefile->WriteBool	    ( usePlayerModel );
	savefile->WriteString	( spawnArgs.GetString("body_model") );
    stateThread.Save( savefile );

    for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
        savefile->WriteInt( animDoneTime[i] );
    }

	savefile->WriteVec3	    ( bodyOffset );
	savefile->WriteBool	    ( allChannel );
}

/*
================
idViewBody::Restore
================
*/
void idViewBody::Restore( idRestoreGame *savefile ) {
    int						i;
	int num;
	idStr str;

	savefile->ReadObject( reinterpret_cast<idClass *&>( owner ) );
	if(owner)
	{
        // for keep old savegame compatibility, idPlayer not save idViewBody, so setup idPlayer by this
		owner->viewBody = this;
	}
	savefile->ReadInt(num);
	hideSurfaces.Clear();
    for ( i = 0; i < num; i++ ) {
        savefile->ReadString( str );
		hideSurfaces.Append(str);
    }
	savefile->ReadBool( usePlayerModel );
	savefile->ReadString(str);
	if(!usePlayerModel)
		SetViewModel(str.c_str());

    GetPhysics()->SetContents( 0 );
    GetPhysics()->SetClipMask( 0 );
    GetPhysics()->SetClipModel( NULL, 1.0f );
	Init(owner, false);

    stateThread.Restore( savefile, this );

    for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
        savefile->ReadInt( animDoneTime[i] );
    }

    savefile->ReadVec3( bodyOffset );
	savefile->ReadBool( allChannel );
}

/*
===============
idViewBody::ClientPredictionThink
===============
*/
void idViewBody::ClientPredictionThink( void ) {
    UpdateAnimation();
}

/*
================
idViewBody::Clear
================
*/
void idViewBody::Clear( void ) {
    DeconstructScriptObject();
    scriptObject.Free();

    StopAllEffects( );

    // TTimo - the weapon doesn't get a proper Event_DisableWeapon sometimes, so the sound sticks
    // typically, client side instance join in tourney mode just wipes all ents
    StopSound( SND_CHANNEL_ANY, false );

    memset( &renderEntity, 0, sizeof( renderEntity ) );
    renderEntity.entityNum	= entityNumber;

    renderEntity.noShadow		= true;
    renderEntity.noSelfShadow	= true;
    renderEntity.customSkin		= NULL;

    // set default shader parms
    renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
    renderEntity.shaderParms[ SHADERPARM_GREEN ]= 1.0f;
    renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
    renderEntity.shaderParms[3] = 1.0f;
    renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = 0.0f;
    renderEntity.shaderParms[5] = 0.0f;
    renderEntity.shaderParms[6] = 0.0f;
    renderEntity.shaderParms[7] = 0.0f;

    memset( &refSound, 0, sizeof( refSound_t ) );
    refSound.referenceSoundHandle = -1;

    // setting diversity to 0 results in no random sound.  -1 indicates random.
    refSound.diversity = -1.0f;

    animator.ClearAllAnims( gameLocal.time, 0 );

    FreeModelDef();
}

int idViewBody::SplitSurfaces(const char *surfaces, idStrList &ret) const
{
	if(!surfaces || !surfaces[0])
		return 0;

	int start = 0;
	int index;
	int counter = 0;
	idStr str(surfaces);
	str.Strip(',');
	while((index = str.Find(',', start)) != -1)
	{
		if(index - start > 0)
		{
			idStr s = str.Mid(start, index - start);
			ret.AddUnique(s);
			counter++;
		}
		start = index + 1;
		if(index == str.Length() - 1)
			break;
	}
	if(start < str.Length() - 1)
	{
		idStr s = str.Mid(start, str.Length() - start);
		ret.AddUnique(s);
		counter++;
	}

	return counter;
}

/*
================
idViewBody::SetModel
================
*/
void idViewBody::SetViewModel( const char *modelname ) {
    assert( modelname );

    if ( modelDefHandle >= 0 ) {
        gameRenderWorld->RemoveDecals( modelDefHandle );
    }

    renderEntity.hModel = animator.SetModel( modelname );
    if ( renderEntity.hModel ) {
        renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
        animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );

		for(int i = 0; i < hideSurfaces.Num(); i++)
			HideSurface(hideSurfaces[i]);
    } else {
        renderEntity.customSkin = NULL;
        renderEntity.callback = NULL;
        renderEntity.numJoints = 0;
        renderEntity.joints = NULL;
    }

    // hide the model until an animation is played
    Hide();
}

/***********************************************************************

	State control/player interface

***********************************************************************/

/*
================
idViewBody::Think
================
*/
void idViewBody::Think( void ) {
	if(!owner)
		return;

    // set the physics position and orientation
    GetPhysics()->SetOrigin( owner->GetPhysics()->GetOrigin() + bodyOffset * owner->viewAxis );
    GetPhysics()->SetAxis(owner->viewAxis);

	CHECK_MODEL();

    UpdateVisuals();

    if ( gameLocal.isNewFrame ) {
        stateThread.Execute( );
    }

    UpdateAnimation( );
}

/*
=====================
idViewBody::ConvertLocalToWorldTransform
=====================
*/
void idViewBody::ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis ) {
    offset = GetPhysics()->GetOrigin()/* + offset * weapon->ForeshortenAxis( GetPhysics()->GetAxis() )*/;
    axis *= GetPhysics()->GetAxis();
}

/*
================
idViewBody::UpdateModelTransform
================
*/
void idViewBody::UpdateModelTransform( void ) {
    idVec3 origin;
    idMat3 axis;

    if ( GetPhysicsToVisualTransform( origin, axis ) ) {
        renderEntity.axis = axis * /*weapon->ForeshortenAxis*/( GetPhysics()->GetAxis() );
        renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
    } else {
        renderEntity.axis = /*weapon->ForeshortenAxis*/( GetPhysics()->GetAxis() );
        renderEntity.origin = GetPhysics()->GetOrigin();
    }
}

/*
================
idViewBody::PresentBody
================
*/
void idViewBody::PresentBody( bool showViewModel ) {
    // Dont do anything with the weapon while its stale
    if ( fl.networkStale ) {
        return;
    }

// RAVEN BEGIN
// rjohnson: cinematics should never be done from the player's perspective, so don't think the weapon ( and their sounds! )
    if ( gameLocal.inCinematic ) {
        return;
    }
// RAVEN END

    // only show the surface in player view
    renderEntity.allowSurfaceInViewID = owner->entityNumber + 1;

    // crunch the depth range so it never pokes into walls this breaks the machine gun gui
    renderEntity.weaponDepthHackInViewID = owner->entityNumber + 1;

    //weapon->Think();

    // present the model
    if ( showViewModel ) {
        Present();
    } else {
        FreeModelDef();
    }

    UpdateSound();
}

/*
================
idViewBody::ClientStale
================
*/
bool idViewBody::ClientStale ( void ) {
    StopSound( SND_CHANNEL_ANY, false );

    idEntity::ClientStale( );

    return false;
}

/*
================
idViewBody::ClientReceiveEvent
================
*/
bool idViewBody::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
    if ( idEntity::ClientReceiveEvent( event, time, msg ) ) {
        return true;
    }
    return false;
}

/*
================
idViewBody::GetPosition
================
*/
void idViewBody::GetPosition( idVec3& origin, idMat3& axis ) const {
    origin = GetPhysics()->GetOrigin();
    axis = GetPhysics()->GetAxis();
}

/*
=====================
idViewBody::GetDebugInfo
=====================
*/
void idViewBody::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
    idAnimatedEntity::GetDebugInfo ( proc, userData );
    proc ( "idViewBody", "state",	stateThread.GetState()?stateThread.GetState()->state->name : "<none>", userData );
}

/*
=====================
idViewBody::GetBodyAnim
=====================
*/
int idViewBody::GetBodyAnim( const char *animname ) {
	int			anim;
	const char *temp;
	idAnimator *animatorPtr;

	animatorPtr = viewAnimator;

	// Allow for anim substitution
	animname = spawnArgs.GetString ( va("anim %s", animname ), animname );

	if ( owner->animPrefix.Length() ) {
		temp = va( "%s_%s", owner->animPrefix.c_str(), animname );
		anim = animatorPtr->GetAnim( temp );
		if ( anim ) {
			return anim;
		}
	}

	anim = animatorPtr->GetAnim( animname );

	return anim;
}

/*
=====================
idViewBody::UpdateWeapon
=====================
*/
void idViewBody::UpdateWeapon(void)
{
	if ( owner->IsLegsIdle ( false ) )
		SetState ( "Legs_Idle", 0 );
}

/*
===============
idViewBody::PlayAnim
===============
*/
void idViewBody::PlayAnim( int channel, const char *animname, int blendFrames ) {
    int	anim;

	CHECK_MODEL();

    anim = GetBodyAnim( animname );
    if ( !anim ) {
        gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, GetName(), GetEntityDefName() );
        viewAnimator->Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
        animDoneTime[channel] = 0;
    } else {
        Show();
        viewAnimator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
        animDoneTime[channel] = viewAnimator->CurrentAnim( channel )->GetEndTime();
    }
}

/*
===============
idViewBody::PlayCycle
===============
*/
void idViewBody::PlayCycle( int channel, const char *animname, int blendFrames ) {
    int anim;

	CHECK_MODEL();

    anim = GetBodyAnim( animname );
    if ( !anim ) {
        gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, GetName(), GetEntityDefName() );
        viewAnimator->Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
        animDoneTime[channel] = 0;
    } else {
        Show();
        viewAnimator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
        animDoneTime[channel] = viewAnimator->CurrentAnim( channel )->GetEndTime();
    }
}

/*
===============
idViewBody::AnimDone
===============
*/
bool idViewBody::AnimDone( int channel, int blendFrames ) {
    if ( animDoneTime[channel] - FRAME2MS( blendFrames ) <= gameLocal.time ) {
        return true;
    }
    return false;
}
/*
================
idViewBody::SetState
================
*/
void idViewBody::SetState( const char *statename, int blendFrames ) {
	CHECK_MODEL();

    stateThread.SetState( statename, blendFrames );
}

/*
================
idViewBody::PostState
================
*/
void idViewBody::PostState( const char* statename, int blendFrames ) {
	CHECK_MODEL();

    stateThread.PostState( statename, blendFrames );
}

/*
=====================
idViewBody::ExecuteState
=====================
*/
void idViewBody::ExecuteState ( const char* statename ) {
	CHECK_MODEL();

    SetState ( statename, 0 );
    stateThread.Execute ( );
}

/*
================
idViewBody::State_Legs_Idle
================
*/
stateResult_t idViewBody::State_Legs_Idle ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            if ( owner->pfl.crouch ) {
                PostAnimState ( "Legs_Crouch", parms.blendFrames );
                return SRESULT_DONE;
            }
            PlayCycle( ANIMCHANNEL_BODY, "idle", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            // If now crouching go back to idle so we can transition to crouch 
            if ( owner->pfl.crouch ) {
                PostAnimState ( "Legs_Crouch", 4 );
                return SRESULT_DONE;
            } else if ( owner->pfl.jump ) {
                PostAnimState ( "Legs_Jump", 4 );
                return SRESULT_DONE;
            } else if ( !owner->pfl.onGround ) {
                PostAnimState ( "Legs_Fall", 4 );
                return SRESULT_DONE;
            }else if ( owner->pfl.forward && !owner->pfl.backward ) {
                if( owner->usercmd.buttons & BUTTON_RUN ) {
                    PlayCycle( ANIMCHANNEL_BODY,  "run_forward", parms.blendFrames );
                    PostAnimState ( "Legs_Run_Forward", parms.blendFrames );
                } else {
                    PlayCycle( ANIMCHANNEL_BODY,  "walk_forward", parms.blendFrames );
                    PostAnimState ( "Legs_Walk_Forward", parms.blendFrames );
                }

                return SRESULT_DONE;
            } else if ( owner->pfl.backward && !owner->pfl.forward ) {
                if( owner->usercmd.buttons & BUTTON_RUN ) {
                    PlayCycle( ANIMCHANNEL_BODY,  "run_backwards", parms.blendFrames );
                    PostAnimState ( "Legs_Run_Backward", parms.blendFrames );
                } else {
                    PlayCycle( ANIMCHANNEL_BODY,  "walk_backwards", parms.blendFrames );
                    PostAnimState ( "Legs_Walk_Backward", parms.blendFrames );
                }

                return SRESULT_DONE;
            } else if ( owner->pfl.strafeLeft && !owner->pfl.strafeRight ) {
                if( owner->usercmd.buttons & BUTTON_RUN ) {
                    PlayCycle( ANIMCHANNEL_BODY,  "run_strafe_left", parms.blendFrames );
                    PostAnimState ( "Legs_Run_Left", parms.blendFrames );
                } else {
                    PlayCycle( ANIMCHANNEL_BODY,  "walk_left", parms.blendFrames );
                    PostAnimState ( "Legs_Walk_Left", parms.blendFrames );
                }

                return SRESULT_DONE;
            } else if ( owner->pfl.strafeRight && !owner->pfl.strafeLeft ) {
                if( owner->usercmd.buttons & BUTTON_RUN ) {
                    PlayCycle( ANIMCHANNEL_BODY,  "run_strafe_right", parms.blendFrames );
                    PostAnimState ( "Legs_Run_Right", parms.blendFrames );
                } else {
                    PlayCycle( ANIMCHANNEL_BODY,  "walk_right", parms.blendFrames );
                    PostAnimState ( "Legs_Walk_Right", parms.blendFrames );
                }

                return SRESULT_DONE;
            }
            return SRESULT_WAIT;

    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Crouch_Idle
================
*/
stateResult_t idViewBody::State_Legs_Crouch_Idle ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            if ( !owner->pfl.crouch ) {
                PostAnimState ( "Legs_Uncrouch", parms.blendFrames );
                return SRESULT_DONE;
            }
            PlayCycle ( ANIMCHANNEL_BODY,  "crouch", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( !owner->pfl.crouch || owner->pfl.jump ) {
                PostAnimState ( "Legs_Uncrouch", 4 );
                return SRESULT_DONE;
            } else if ( (owner->pfl.forward && !owner->pfl.backward) || (owner->pfl.strafeLeft != owner->pfl.strafeRight) ) {
                PostAnimState ( "Legs_Crouch_Forward", parms.blendFrames );
                return SRESULT_DONE;
            } else if ( owner->pfl.backward && !owner->pfl.forward ) {
                PostAnimState ( "Legs_Crouch_Backward", parms.blendFrames );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Crouch
================
*/
stateResult_t idViewBody::State_Legs_Crouch ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            PlayAnim( ANIMCHANNEL_BODY,  "crouch_down", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( !owner->IsLegsIdle ( true ) || AnimDone ( ANIMCHANNEL_BODY,  4 ) ) {
                PostAnimState ( "Legs_Crouch_Idle", parms.blendFrames );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Uncrouch
================
*/
stateResult_t idViewBody::State_Legs_Uncrouch ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            PlayAnim( ANIMCHANNEL_BODY,  "crouch_up", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( !owner->IsLegsIdle ( false ) || AnimDone ( ANIMCHANNEL_BODY,  4 ) ) {
                PostAnimState ( "Legs_Idle", 4 );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Run_Forward
================
*/
stateResult_t idViewBody::State_Legs_Run_Forward ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && !owner->pfl.backward && owner->pfl.forward ) {
        if( owner->usercmd.buttons & BUTTON_RUN ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "walk_forward", parms.blendFrames );
            PostAnimState ( "Legs_Walk_Forward", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Run_Backward
================
*/
stateResult_t idViewBody::State_Legs_Run_Backward ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && !owner->pfl.forward && owner->pfl.backward ) {
        if( owner->usercmd.buttons & BUTTON_RUN ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "walk_backwards", parms.blendFrames );
            PostAnimState ( "Legs_Walk_Backward", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Run_Left
================
*/
stateResult_t idViewBody::State_Legs_Run_Left ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && (owner->pfl.forward == owner->pfl.backward) && owner->pfl.strafeLeft && !owner->pfl.strafeRight ) {
        if( owner->usercmd.buttons & BUTTON_RUN ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "walk_left", parms.blendFrames );
            PostAnimState ( "Legs_Walk_Left", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Run_Right
================
*/
stateResult_t idViewBody::State_Legs_Run_Right ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && (owner->pfl.forward == owner->pfl.backward) && owner->pfl.strafeRight && !owner->pfl.strafeLeft ) {
        if( owner->usercmd.buttons & BUTTON_RUN ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "walk_right", parms.blendFrames );
            PostAnimState ( "Legs_Walk_Right", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Walk_Forward
================
*/
stateResult_t idViewBody::State_Legs_Walk_Forward ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && !owner->pfl.backward && owner->pfl.forward ) {
        if( !(owner->usercmd.buttons & BUTTON_RUN) ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "run_forward", parms.blendFrames );
            PostAnimState ( "Legs_Run_Forward", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Walk_Backward
================
*/
stateResult_t idViewBody::State_Legs_Walk_Backward ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && !owner->pfl.forward && owner->pfl.backward ) {
        if( !(owner->usercmd.buttons & BUTTON_RUN) ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "run_backwards", parms.blendFrames );
            PostAnimState ( "Legs_Run_Backward", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Walk_Left
================
*/
stateResult_t idViewBody::State_Legs_Walk_Left ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && (owner->pfl.forward == owner->pfl.backward) && owner->pfl.strafeLeft && !owner->pfl.strafeRight ) {
        if( !(owner->usercmd.buttons & BUTTON_RUN) ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "run_strafe_left", parms.blendFrames );
            PostAnimState ( "Legs_Run_Left", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Walk_Right
================
*/
stateResult_t idViewBody::State_Legs_Walk_Right ( const stateParms_t& parms ) {
    if ( !owner->pfl.jump && owner->pfl.onGround && !owner->pfl.crouch && (owner->pfl.forward == owner->pfl.backward) && owner->pfl.strafeRight && !owner->pfl.strafeLeft ) {
        if( !(owner->usercmd.buttons & BUTTON_RUN) ) {
            return SRESULT_WAIT;
        } else {
            PlayCycle( ANIMCHANNEL_BODY,  "run_strafe_right", parms.blendFrames );
            PostAnimState ( "Legs_Run_Right", parms.blendFrames );
            return SRESULT_DONE;
        }
    }
    PostAnimState ( "Legs_Idle", parms.blendFrames );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Crouch_Forward
================
*/
stateResult_t idViewBody::State_Legs_Crouch_Forward ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            PlayCycle( ANIMCHANNEL_BODY,  "crouch_walk", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( !owner->pfl.jump && owner->pfl.onGround && owner->pfl.crouch && ((!owner->pfl.backward && owner->pfl.forward) || (owner->pfl.strafeLeft != owner->pfl.strafeRight)) ) {
                return SRESULT_WAIT;
            }
            PostAnimState ( "Legs_Crouch_Idle", 2 );
            return SRESULT_DONE;
    }
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Crouch_Backward
================
*/
stateResult_t idViewBody::State_Legs_Crouch_Backward ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            PlayCycle( ANIMCHANNEL_BODY,  "crouch_walk_backward", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( !owner->pfl.jump && owner->pfl.onGround && owner->pfl.crouch && !owner->pfl.forward && owner->pfl.backward ) {
                return SRESULT_WAIT;
            }
            PostAnimState ( "Legs_Crouch_Idle", parms.blendFrames );
            return SRESULT_DONE;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Jump
================
*/
stateResult_t idViewBody::State_Legs_Jump ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            // prevent infinite recursion
            owner->pfl.jump = false;
            if ( owner->pfl.run ) {
                PlayAnim( ANIMCHANNEL_BODY,  "run_jump", parms.blendFrames );
            } else {
                PlayAnim( ANIMCHANNEL_BODY,  "jump", parms.blendFrames );
            }
            return SRESULT_STAGE ( STAGE_WAIT );

        case STAGE_WAIT:
            if ( owner->pfl.onGround ) {
                PostAnimState ( "Legs_Land", 4 );
                return SRESULT_DONE;
            }
            if ( AnimDone ( ANIMCHANNEL_BODY,  4 ) ) {
                PostAnimState ( "Legs_Fall", 4 );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Fall
================
*/
stateResult_t idViewBody::State_Legs_Fall ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            if ( owner->pfl.onGround ) {
                PostAnimState ( "Legs_Land", 2 );
                return SRESULT_DONE;
            }
            PlayCycle ( ANIMCHANNEL_BODY,  "fall", parms.blendFrames );
            return SRESULT_STAGE ( STAGE_WAIT );
        case STAGE_WAIT:
            if ( owner->pfl.onGround ) {
                PostAnimState ( "Legs_Land", 2 );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Legs_Land
================
*/
stateResult_t idViewBody::State_Legs_Land ( const stateParms_t& parms ) {
    enum {
        STAGE_INIT,
        STAGE_WAIT
    };
    switch ( parms.stage ) {
        case STAGE_INIT:
            if ( owner->IsLegsIdle ( false ) && ( owner->pfl.hardLanding || owner->pfl.softLanding ) ) {
                if ( owner->pfl.hardLanding ) {
                    PlayAnim( ANIMCHANNEL_BODY,  "hard_land", parms.blendFrames );
                } else {
                    PlayAnim( ANIMCHANNEL_BODY,  "soft_land", parms.blendFrames );
                }
                return SRESULT_STAGE ( STAGE_WAIT );
            }
            PostAnimState ( "Legs_Idle", 4 );
            return SRESULT_DONE;

        case STAGE_WAIT:
            if ( !owner->IsLegsIdle ( false ) || AnimDone ( ANIMCHANNEL_BODY,  parms.blendFrames ) ) {
                PostAnimState ( "Legs_Idle", parms.blendFrames );
                return SRESULT_DONE;
            }
            return SRESULT_WAIT;
    }
    return SRESULT_ERROR;
}

/*
================
idViewBody::State_Legs_Dead
================
*/
stateResult_t idViewBody::State_Legs_Dead ( const stateParms_t& parms ) {
    PostAnimState ( "Wait_Alive", 0 );
    PostAnimState ( "Legs_Idle", 0 );
    return SRESULT_DONE;
}

/*
================
idViewBody::State_Wait_Alive

Waits until the player is alive again.
================
*/
stateResult_t idViewBody::State_Wait_Alive ( const stateParms_t& parms ) {
    if ( owner->pfl.dead ) {
        return SRESULT_WAIT;
    }
    return SRESULT_DONE;
}

