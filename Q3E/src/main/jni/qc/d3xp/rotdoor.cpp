
//Rotating door.
//Based on The Dark Mod's CBinaryFrobMover.


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

const idEventDef EV_rotdoor_open( "open", NULL );
const idEventDef EV_rotdoor_close( "close", NULL );
const idEventDef EV_rotdoor_isOpen( "isOpen", NULL, 'f' );

CLASS_DECLARATION( idMover, idRotDoor )
	EVENT( EV_rotdoor_open,			idRotDoor::Event_Open )
	EVENT( EV_rotdoor_close,		idRotDoor::Event_Close )
	EVENT( EV_rotdoor_isOpen,		idRotDoor::Event_isOpen )
END_CLASS


void idRotDoor::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(shouldOpen);
	savefile->WriteAngles(m_Rotate);
	savefile->WriteVec3(m_ClosedPos);
	savefile->WriteVec3(m_OpenPos);
	savefile->WriteVec3(m_OpenDir);
	savefile->WriteAngles(m_ClosedAngles);
	savefile->WriteAngles(m_OpenAngles);
}

void idRotDoor::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(shouldOpen);
	savefile->ReadAngles(m_Rotate);
	savefile->ReadVec3(m_ClosedPos);
	savefile->ReadVec3(m_OpenPos);
	savefile->ReadVec3(m_OpenDir);
	savefile->ReadAngles(m_ClosedAngles);
	savefile->ReadAngles(m_OpenAngles);
}

void idRotDoor::Spawn( void )
{	
	m_Rotate = spawnArgs.GetAngles("rotate", "0 90 0");

	m_ClosedAngles = physicsObj.GetLocalAngles();
	m_ClosedAngles.Normalize180();
	m_OpenAngles = (m_ClosedAngles + m_Rotate).Normalize180();

	idRotation rot = m_Rotate.ToRotation();
	idVec3 rotationAxis = rot.GetVec();
	idVec3 normal = rotationAxis.Cross(m_ClosedPos);

	m_OpenDir = (m_OpenPos * normal) * normal;
	m_OpenDir.Normalize();

	this->shouldOpen = true;
}

void idRotDoor::Event_isOpen( void )
{
	idThread::ReturnInt( IsOpen() );
}

bool idRotDoor::IsOpen( void )
{
	return ( !this->shouldOpen );
}

void idRotDoor::Event_Activate( idEntity *activator )
{
	idAngles targetAngles = this->shouldOpen ? m_OpenAngles : m_ClosedAngles;

	idAngles angleDelta = (targetAngles - physicsObj.GetLocalAngles()).Normalize180();

	if (!angleDelta.Compare(idAngles(0,0,0)))
	{
		Event_RotateOnce(angleDelta);

		if (this->shouldOpen)
		{
			StartSound( "snd_open", SND_CHANNEL_ANY, 0, false, NULL );
			Signal( SIG_MOVER_1TO2 );
		}
		else
			StartSound( "snd_close", SND_CHANNEL_ANY, 0, false, NULL );
	}

	this->shouldOpen = !this->shouldOpen;
}

void idRotDoor::Event_Open( void )
{
	this->shouldOpen = true;
	this->Event_Activate( NULL ); 
}

void idRotDoor::Event_Close( void )
{
	this->shouldOpen = false;
	this->Event_Activate( NULL ); 
}