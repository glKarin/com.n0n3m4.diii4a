/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

/*
   Copyright (C) 2004 Id Software, Inc.
   Copyright (C) 2011 The Dark Mod

func_emitters - have one or more particle models

*/

#include "precompiled.h"
#pragma hdrstop



#include "Emitter.h"

const idEventDef EV_EmitterAddModel( "emitterAddModel", EventArgs('s', "modelName", "", 'v', "modelOffset", ""), EV_RETURNS_VOID, 
	"Adds a new particle (or regular, if you wish) model to the emitter,\n" \
	"located at modelOffset units away from the emitter's origin." );
const idEventDef EV_EmitterGetNumModels( "emitterGetNumModels", EventArgs(), 'f', "Returns the number of models/particles this emitter has. Always >= 1." );
const idEventDef EV_On( "On", EventArgs(), EV_RETURNS_VOID, "Switches the emitter on." );
const idEventDef EV_Off( "Off", EventArgs(), EV_RETURNS_VOID, "Switches the emitter off." );

CLASS_DECLARATION( idStaticEntity, idFuncEmitter )
	EVENT( EV_Activate,				idFuncEmitter::Event_Activate )
	EVENT( EV_On,					idFuncEmitter::Event_On )
	EVENT( EV_Off,					idFuncEmitter::Event_Off )
	EVENT( EV_EmitterAddModel,		idFuncEmitter::Event_EmitterAddModel )
	EVENT( EV_EmitterGetNumModels,	idFuncEmitter::Event_EmitterGetNumModels )
END_CLASS

/*
===============
idFuncEmitter::idFuncEmitter
===============
*/
idFuncEmitter::idFuncEmitter( void ) {
	
	hidden = false;
	m_models.Clear();
}

/*
================
idFuncEmitter::~idFuncEmitter
================
*/
idFuncEmitter::~idFuncEmitter( void ) {

	const int num = m_models.Num();
	for (int i = 0; i < num; i++ )
	{
		if ( m_models[i].defHandle != -1 )
		{
			gameRenderWorld->FreeEntityDef( m_models[i].defHandle );
			m_models[i].defHandle = -1;
		}
	}
	m_models.Clear();
}

/*
================
idFuncEmitter::SetModel for additional models
================
*/
void idFuncEmitter::SetModel( int id, const idStr &modelName, const idVec3 &offset ) {

	m_models.AssureSize( id+1 );
	m_models[id].defHandle = -1;
	m_models[id].offset = offset;
	m_models[id].flags = 0;
	m_models[id].handle = renderModelManager->FindModel( modelName );
	if (spawnArgs.GetString("model") != modelName)
	{
		// differs
		m_models[id].name = modelName;
	}
	else
	{
		// store "" to flag as "same as the original model"
		m_models[id].name = "";
	}

	// need to call Present() the next time we Think():
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idFuncEmitter::SetModel for LOD change
================
*/
void idFuncEmitter::SetModel( const char* modelName ) 
{
	if ( modelDefHandle > 0 )
	{
		gameRenderWorld->FreeEntityDef( modelDefHandle );
	}
	modelDefHandle = -1;
	spawnArgs.Set("model", modelName);

	idRenderModel * modelHandle = renderModelManager->FindModel( modelName );

	// go through all other models and change them
	const int num = m_models.Num();
	for (int i = 0; i < num; i++ ) {
		if ( m_models[i].name.IsEmpty())
		{
			// same as original model, so change it, too
			if ( m_models[i].defHandle != -1 )
			{
				gameRenderWorld->FreeEntityDef( m_models[0].defHandle );
			}
			m_models[i].defHandle = -1;
			m_models[i].handle = modelHandle;
		}
		// else: leave it alone
	}

	// need to call Present() the next time we Think():
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idFuncEmitter::Present
================
*/
void idFuncEmitter::Present( void ) 
{
	if( m_bFrobable )
	{
		UpdateFrobState();
		UpdateFrobDisplay();
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) 
	{
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	// present the first model
    renderEntity.origin = GetPhysics()->GetOrigin();
	renderEntity.axis = GetPhysics()->GetAxis();
	renderEntity.hModel = renderModelManager->FindModel( spawnArgs.GetString("model") );

	renderEntity.bodyId = 0;
	
	// give each instance of the particle effect a unique seed -- SteveL #3945
	if ( !renderEntity.shaderParms[SHADERPARM_DIVERSITY] ) // 2.10: but not when it moves bound to another entity, otherwise animation breaks
		renderEntity.shaderParms[SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat();

	if ( !renderEntity.hModel || IsHidden() ) { // copy the default behaviour in idEntity::Present - don't re-create render entities when hidden by LOD triggers
		return;
	}

	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
		// add to refresh list
		if ( modelDefHandle == -1 ) {
			modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
		} else {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
	else {
		// don't present the model if it is non-existant
		renderEntity.bounds.Zero();
	}
	// TODO: add to the bounds above the bounds of the other models

	// present additional models to the renderer
	const int num = m_models.Num();
//		gameLocal.Printf("%s: Have %i models\n", GetName(), num + 1);
	for (int i = 0; i < num; i++ ) {

		if ( !m_models[i].handle ) {
			continue;
		}

		if ( (m_models[i].flags & 1) != 0)
		{
			// is invisible, ignore it
			continue;
		}
//		gameLocal.Printf("%s: Presenting model %i\n", GetName(), i);
		renderEntity.origin = GetPhysics()->GetOrigin() + m_models[i].offset;
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.hModel = m_models[i].handle;
		renderEntity.bodyId = i + 1;
		// give each instance of the particle effect a unique seed
		renderEntity.shaderParms[ SHADERPARM_DIVERSITY ] = gameLocal.random.CRandomFloat();

		if ( renderEntity.hModel ) {
			renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
			// add to refresh list
			if ( m_models[i].defHandle == -1 ) {
				m_models[i].defHandle = gameRenderWorld->AddEntityDef( &renderEntity );
			} else {
				gameRenderWorld->UpdateEntityDef( m_models[i].defHandle, &renderEntity );
			}
		}
		else {
			renderEntity.bounds.Zero();
		}
	}
}

/*
===============
idFuncEmitter::Spawn
===============
*/
void idFuncEmitter::Spawn( void ) {
	const idKeyValue *kv;

	if ( spawnArgs.GetBool( "start_off" ) ) {
		hidden = true;
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( 1 );
		UpdateVisuals();
	} else {
		hidden = false;
	}
	
	// check if we have additional models
   	kv = spawnArgs.MatchPrefix( "model", NULL );
	int model = 0;
	while( kv )
	{
		idStr suffix = kv->GetKey();
		idStr modelName = kv->GetValue();
		if ((suffix.Length() >= 9 && suffix.Cmpn("model_lod",9) == 0) || suffix == "model")
		{
			// ignore "model_lod_2" as well as "model"
			kv = spawnArgs.MatchPrefix( "model", kv );
			continue;
		}
		suffix = suffix.Right( suffix.Length() - 5);	// model_2 => "_2"
		if (!spawnArgs.FindKey("offset" + suffix))
		{
			gameLocal.Warning("FuncEmitter %s: 'offset%s' is undefined and will clash at origin.", GetName(), suffix.c_str() );
		}
		idVec3 modelOffset = spawnArgs.GetVector( "offset" + suffix, "0,0,0" );
//		gameLocal.Printf( "FuncEmitter %s: Adding model %i ('%s') at offset %s (suffix %s).\n", GetName(), model, modelName.c_str(), modelOffset.ToString(), suffix.c_str() );
		SetModel( model++, modelName, modelOffset );
		kv = spawnArgs.MatchPrefix( "model", kv );
	}
	if (model > 0)
	{
		gameLocal.Printf( "FuncEmitter %s: Added %i additional emitters.\n", GetName(), model );
	}
}

/*
===============
idFuncEmitter::Save
===============
*/
void idFuncEmitter::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( hidden );

	const int num = m_models.Num();
	savefile->WriteInt( num );
	for (int i = 0; i < num; i++ ) {
		savefile->WriteInt( m_models[i].defHandle );
		savefile->WriteVec3( m_models[i].offset );
		savefile->WriteString( m_models[i].name );
		savefile->WriteInt( m_models[i].flags );
	}
}

/*
================
idFuncEmitter::Think
================
*/
void idFuncEmitter::Think( void ) 
{
	// will also do LOD thinking:
	idEntity::Think();

	// extra models? Do LOD thinking for them:
	//const int num = m_models.Num();
	// start with 1
/*	for (int i = 1; i < num; i++)
	{
		// TODO:
	}
*/
	// did our model(s) change?
	Present();
}

/*
===============
idFuncEmitter::Restore
===============
*/
void idFuncEmitter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( hidden );
	int num;
	savefile->ReadInt( num );

	idStr defaultModelName = spawnArgs.GetString("model");

	m_models.Clear();
	m_models.SetNum(num);
	for (int i = 0; i < num; i++ ) {
		savefile->ReadInt( m_models[i].defHandle );
		savefile->ReadVec3( m_models[i].offset );
		savefile->ReadString( m_models[i].name );
		savefile->ReadInt( m_models[i].flags );

		// find the modelHandle
		idStr modelName = m_models[i].name;
		if (modelName.IsEmpty())
		{
			modelName = defaultModelName;
		}
		m_models[i].handle = renderModelManager->FindModel( modelName );
	}
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idFuncEmitter::On
================
*/
void idFuncEmitter::On( void ) {
	//if this is a looping emitter that's already on, dont call On() to avoid refreshing it
	if ( !hidden && !spawnArgs.GetBool("cycleTrigger") )
		return;

	renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = 0;
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
	hidden = false;
	UpdateVisuals();
}

/*
================
idFuncEmitter::Off
================
*/
void idFuncEmitter::Off( void ) {
	//if this emitter is already off, don't call Off() to avoid refreshing it
	if ( hidden )
		return;

	renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC(gameLocal.time);
	hidden = true;
	UpdateVisuals();
}

/*
  ****************   Events   ****************************************
*/

/*
================
idFuncEmitter::Event_Activate
================
*/
void idFuncEmitter::Event_Activate( idEntity *activator ) {
	if ( hidden || spawnArgs.GetBool( "cycleTrigger" ) ) {
		On();
	} else {
		Off();
	}

	UpdateVisuals();
}

/*
================
idFuncEmitter::Event_On
================
*/
void idFuncEmitter::Event_On( void ) {
	On();
}

/*
================
idFuncEmitter::Event_Off
================
*/
void idFuncEmitter::Event_Off( void ) {
	Off();
}

/*
================
idFuncEmitter::Event_EmitterGetNumModels
================
*/
void idFuncEmitter::Event_EmitterGetNumModels( void ) const {
	idThread::ReturnFloat( m_models.Num() + 1 );
}

/*
================
idFuncEmitter::Event_EmitterAddModel
================
*/
void idFuncEmitter::Event_EmitterAddModel( idStr const &modelName, idVec3 const &modelOffset ) {

	SetModel( m_models.Num(), modelName, modelOffset ); 
}



/*
================
idFuncEmitter::WriteToSnapshot
================
*/
void idFuncEmitter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( hidden ? 1 : 0, 1 );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
	// TODO: additinal models
}

/*
================
idFuncEmitter::ReadFromSnapshot
================
*/
void idFuncEmitter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hidden = msg.ReadBits( 1 ) != 0;
	renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] = msg.ReadFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = msg.ReadFloat();
	// TODO: additinal models
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

idFuncSplat

===============================================================================
*/


const idEventDef EV_Splat( "<Splat>", EventArgs(), EV_RETURNS_VOID, "internal" );
CLASS_DECLARATION( idFuncEmitter, idFuncSplat )
EVENT( EV_Activate,		idFuncSplat::Event_Activate )
EVENT( EV_Splat,		idFuncSplat::Event_Splat )
END_CLASS

/*
===============
idFuncSplat::idFuncSplat
===============
*/
idFuncSplat::idFuncSplat( void ) {
}

/*
===============
idFuncSplat::Spawn
===============
*/
void idFuncSplat::Spawn( void ) {
}

/*
================
idFuncSplat::Event_Splat
================
*/
void idFuncSplat::Event_Splat( void ) {
	const char *splat = NULL;
	int count = spawnArgs.GetInt( "splatCount", "1" );
	for ( int i = 0; i < count; i++ ) {
		splat = spawnArgs.RandomPrefix( "mtr_splat", gameLocal.random );
		if ( splat && *splat ) {
			float size = spawnArgs.GetFloat( "splatSize", "128" );
			float dist = spawnArgs.GetFloat( "splatDistance", "128" );
			float angle = spawnArgs.GetFloat( "splatAngle", "0" );
			gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[2], dist, true, size, splat, angle );
		}
	}
	StartSound( "snd_splat", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
idFuncSplat::Event_Activate
================
*/
void idFuncSplat::Event_Activate( idEntity *activator ) {
	idFuncEmitter::Event_Activate( activator );
	PostEventSec( &EV_Splat, spawnArgs.GetFloat( "splatDelay", "0.25" ) );
	StartSound( "snd_spurt", SND_CHANNEL_ANY, 0, false, NULL );
}


