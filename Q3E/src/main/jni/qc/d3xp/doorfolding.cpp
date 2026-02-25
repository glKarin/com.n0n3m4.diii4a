
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"



const idEventDef EV_doorfoldingactivate(		"doorfoldingactivate", "d" );

CLASS_DECLARATION( idEntity, idDoorFolding )
	EVENT( EV_doorfoldingactivate,		idDoorFolding::Event_activate)
END_CLASS



void idDoorFolding::Think( void )
{

	

	idEntity::Present();
}


void idDoorFolding::Event_activate( int value )
{
	if (value <= 0)
	{
		int i;

		//close.
		for (i = 0; i < BARCOUNT; i++)
		{
			idVec3 origin = GetPhysics()->GetOrigin();
			movers[i]->Event_MoveToPos( origin );
		}
	}
	else
	{
		int i;
		//open.
		for (i = 0; i < BARCOUNT; i++)
		{
			movers[i]->Event_MoveToPos( movers[i]->originalPosition );
		}
	}
}

void idDoorFolding::Spawn( void )
{
	int i;
	idVec3 forward;
	int doorHeight = 112;
	int doorWidth = spawnArgs.GetInt("width", "128");

	float movetime = spawnArgs.GetFloat("movetime", "1" );

	int segmentWidth = doorWidth / BARCOUNT;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward);

	int barheight0 = doorHeight * 0.3f;
	int barheight1 = doorHeight * 0.7f;

	for (i = 0; i < BARCOUNT; i++)
	{
		idDict args;
		args.Clear();

		args.SetVector( "origin", this->GetPhysics()->GetOrigin() + (forward * (segmentWidth * (i+1))));
		args.Set( "model", "models/doorfolding/tris.ase" );

		movers[i] = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
		movers[i]->Event_SetMoveTime( movetime );
	}

	//diagonal bars.
	for (i = 1; i < BARCOUNT; i++)
	{
		idDict args;

		args.Clear();
		args.SetVector( "origin", movers[i]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight0+16) );
		this->beam[(i * 8) + 0] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 0]->Bind(movers[i], false);

		args.Clear();		
		args.SetVector(	"origin", movers[i - 1]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight0-16) );
		args.Set(		"width", "2" );
		args.Set(		"skin", "skins/beam_black" );
		args.Set(		"target", beam[(i * 8) + 0]->name.c_str() );
		this->beam[(i * 8) + 1] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 1]->Bind(movers[i - 1], false);

		args.Clear();
		args.SetVector( "origin", movers[i]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight0-16) );
		this->beam[(i * 8) + 2] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 2]->Bind(movers[i], false);

		args.Clear();		
		args.SetVector(	"origin", movers[i - 1]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight0+16) );
		args.Set(		"width", "2" );
		args.Set(		"skin", "skins/beam_black" );
		args.Set(		"target", beam[(i * 8) + 2]->name.c_str() );
		this->beam[(i * 8) + 3] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 3]->Bind(movers[i - 1], false);




		//second set.
		args.Clear();
		args.SetVector( "origin", movers[i]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight1+16) );
		this->beam[(i * 8) + 4] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 4]->Bind(movers[i], false);

		args.Clear();		
		args.SetVector(	"origin", movers[i - 1]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight1-16) );
		args.Set(		"width", "2" );
		args.Set(		"skin", "skins/beam_black" );
		args.Set(		"target", beam[(i * 8) + 4]->name.c_str() );
		this->beam[(i * 8) + 5] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 5]->Bind(movers[i - 1], false);

		args.Clear();
		args.SetVector( "origin", movers[i]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight1-16) );
		this->beam[(i * 8) + 6] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 6]->Bind(movers[i], false);

		args.Clear();		
		args.SetVector(	"origin", movers[i - 1]->GetPhysics()->GetOrigin() + idVec3(0,0,barheight1+16) );
		args.Set(		"width", "2" );
		args.Set(		"skin", "skins/beam_black" );
		args.Set(		"target", beam[(i * 8) + 6]->name.c_str() );
		this->beam[(i * 8) + 7] = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		this->beam[(i * 8) + 7]->Bind(movers[i - 1], false);


	}
}

