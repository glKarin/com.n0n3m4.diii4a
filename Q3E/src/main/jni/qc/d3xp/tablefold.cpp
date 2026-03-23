
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

const idEventDef EV_tablefoldreset(		"tablefoldreset"  );

CLASS_DECLARATION( idAnimatedEntity, idTableFold )
	EVENT( EV_tablefoldreset,		idTableFold::Event_reset)
END_CLASS

void idTableFold::Spawn( void )
{
	idDict args;

	args.Clear();
	args.SetVector( "origin",	this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"models/tablefold/tris_cm.ase" );
	args.SetInt( "frobbable",	1 );
	args.SetInt( "corpse",		1 );
	args.SetInt( "noGrab",		1 );
	args.Set( "owner",			this->GetName() );
	args.Set( "recordable",		this->spawnArgs.GetString("recordable", "0") );
	this->frobcube = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	this->frobcube->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );

	args.Clear();
	args.SetVector( "origin",	this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"models/tablefold/tris_handle_cm.ase" );
	args.SetInt( "frobbable",	1 );
	args.SetInt( "corpse",		1 );
	args.SetInt( "noGrab",		1 );
	args.Set( "owner",			this->GetName() );
	args.Set( "recordable",		this->spawnArgs.GetString("recordable", "0") );
	this->frobcube_handle = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	this->frobcube_handle->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );

	args.Clear();
	args.SetVector( "origin",	this->GetPhysics()->GetOrigin() );
	args.Set( "model",			"models/tablefold/tris_table_cm.ase" );
	mover = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
	mover->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );
	mover->Event_SetMoveTime( 0.5 );

	Event_open(0);
	state	= CLOSED;
}

void idTableFold::Event_open( bool value )
{
	if (value)
	{
		//open.
		Event_PlayAnim("open", 4);
		this->frobcube->GetPhysics()->SetContents(0);
		this->frobcube_handle->GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
		this->frobcube_handle->GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
		state	= OPENED;

		idVec3 origPos = this->GetPhysics()->GetOrigin();
		mover->Event_MoveToPos( origPos );

		
	}
	else
	{
		//close.
		Event_PlayAnim("close", 4);
		this->frobcube_handle->GetPhysics()->SetContents(0);
		this->frobcube->GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
		this->frobcube->GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
		state	= CLOSED;

		idVec3	forward;
		idVec3 movePos;		
		GetPhysics()->GetAxis().ToAngles().ToVectors( &forward );
		movePos = this->GetPhysics()->GetOrigin() + (forward * -20);
		mover->Event_MoveToPos( movePos );
	}
}

void idTableFold::Event_reset( void )
{
	Event_open(0);
}

void idTableFold::OnFrob( idEntity* activator )
{
	if (state == CLOSED)
	{
		Event_open(true);
	}
	else
	{
		Event_open(false);
	}
}

