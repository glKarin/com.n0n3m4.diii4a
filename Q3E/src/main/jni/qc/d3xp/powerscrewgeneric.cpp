
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_powerscrew_reset( "powerscrewreset" );

CLASS_DECLARATION( idMover, idPowerScrewGeneric )
	
	EVENT( EV_Activate,					idPowerScrewGeneric::OnActivate )
	EVENT( EV_powerscrew_reset,			idPowerScrewGeneric::Reset)
END_CLASS


void idPowerScrewGeneric::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(countMax);
	savefile->WriteInt(count);
}

void idPowerScrewGeneric::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(countMax);
	savefile->ReadInt(count);
}

void idPowerScrewGeneric::OnActivate( void )
{
	count--;

	if (count >= 1)
		return;

	idStr scriptName = spawnArgs.GetString( "call" );
	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction( scriptName );
	if ( scriptFunction )
	{
		idThread *thread;
		thread = new idThread( scriptFunction );
		thread->DelayedStart( 0 );
	}
}

void idPowerScrewGeneric::Spawn( void )
{
	count = spawnArgs.GetInt( "count", "2" );
	countMax = count;
}

void idPowerScrewGeneric::Reset( void )
{
	this->count = spawnArgs.GetInt( "count", "2" );

	this->Event_StopMoving();
	this->Event_StopRotating();

	this->GetPhysics()->SetOrigin( this->originalPosition);
	this->GetPhysics()->SetAxis( this->originalAngle);
	this->UpdateVisuals();
}
