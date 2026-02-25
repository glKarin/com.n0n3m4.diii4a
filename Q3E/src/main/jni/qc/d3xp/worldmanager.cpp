
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define RECORDRATE		300 //record every X ms


#define RECORD_LAUNCHAIM	1
#define RECORD_UNSPAWN		2
#define RECORD_SPAWN		3

const idEventDef EV_recordclear( "recordclear", "d" );
const idEventDef EV_recordstart( "recordstart", "dd" );
const idEventDef EV_recordstop( "recordstop" );
const idEventDef EV_recordplay( "recordplay", "d" );
const idEventDef EV_geteventcount( "geteventcount", "d", 'd' );
const idEventDef EV_getlasteventtimestamp( "getlasteventtimestamp", "d", 'd' );
const idEventDef EV_setrecordstarttime( "setrecordstarttime" );
const idEventDef EV_moveplayertoghost( "moveplayertoghost", "d" );
const idEventDef EV_setcomplete( "setcomplete", "dd" );
const idEventDef EV_getcomplete( "getcomplete", "d", 'd' );
const idEventDef EV_recordspawn( "recordspawn", "sdvf" );
const idEventDef EV_recordunspawn( "recordunspawn", "d" );
const idEventDef EV_recordlaunchaim( "recordlaunchaim", "dvf" );

const idEventDef EV_reset( "worldreset" );

CLASS_DECLARATION( idEntity, idWorldManager )
	EVENT( EV_recordstart,			idWorldManager::Event_recordstart)
	EVENT( EV_recordstop,			idWorldManager::Event_recordstop)
	EVENT( EV_recordplay,			idWorldManager::Event_recordplay)
	EVENT( EV_reset,				idWorldManager::Event_reset)
	EVENT( EV_geteventcount,		idWorldManager::Event_geteventcount)
	EVENT( EV_getlasteventtimestamp, idWorldManager::Event_getlasteventtimestamp)
	EVENT( EV_setrecordstarttime,	idWorldManager::Event_setrecordstarttime)
	EVENT( EV_moveplayertoghost,	idWorldManager::Event_moveplayertoghost)
	EVENT( EV_recordclear,			idWorldManager::Event_recordclear)
	EVENT( EV_recordspawn,			idWorldManager::RecordSpawn)
	EVENT( EV_recordunspawn,		idWorldManager::RecordUnspawn)
	EVENT( EV_recordlaunchaim,		idWorldManager::RecordLaunchAim)

	EVENT( EV_setcomplete,			idWorldManager::Event_setcomplete)
	EVENT( EV_getcomplete,			idWorldManager::Event_getcomplete)
END_CLASS

void idWorldManager::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(index);
	savefile->WriteInt(state);
	savefile->WriteInt(recordStarttime);
	savefile->WriteInt(nextRecordtime);
	savefile->WriteVec3(lastPosition);
	savefile->WriteFloat(lastAngle);

	//RECORDS.
	for (int i = 0; i < 3; i++)
	{
		savefile->WriteObject(records[i].ghost);
		savefile->WriteInt(records[i].index);
		savefile->WriteBool(records[i].playing);
		savefile->WriteBool(records[i].completed);

		int totalEvents = records[i].events.Num();
		savefile->WriteInt(totalEvents); //how many events.

		for (int k = 0; k < totalEvents; k++)
		{
			savefile->WriteInt(records[i].events[k].timestamp);
			savefile->WriteVec3(records[i].events[k].position);
			savefile->WriteFloat(records[i].events[k].yaw);
			savefile->WriteString(records[i].events[k].deckcommand.c_str());
			savefile->WriteString(records[i].events[k].frobcommand.c_str());
			savefile->WriteString(records[i].events[k].spawncommand.c_str());
			savefile->WriteBool(records[i].events[k].done);
			savefile->WriteInt(records[i].events[k].entID);
			savefile->WriteInt(records[i].events[k].recordtype);
		}
	}


	//RECORDLINES
	for (int i = 0; i < 3; i++)
	{
		int totalLines = recordlines[i].lines.Num();
		savefile->WriteInt(totalLines);

		for (int k = 0; k < totalLines; k++)
		{
			savefile->WriteVec3(recordlines[i].lines[k].position1);
			savefile->WriteVec3(recordlines[i].lines[k].position2);
		}
	}
}

void idWorldManager::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(index);
	savefile->ReadInt(state);
	savefile->ReadInt(recordStarttime);
	savefile->ReadInt(nextRecordtime);
	savefile->ReadVec3(lastPosition);
	savefile->ReadFloat(lastAngle);

	//RECORDS.
	for (int i = 0; i < 3; i++)
	{
		savefile->ReadObject(reinterpret_cast<idClass *&>( records[i].ghost ) );
		savefile->ReadInt(records[i].index);
		savefile->ReadBool(records[i].playing);
		savefile->ReadBool(records[i].completed);

		int totalEvents;
		savefile->ReadInt(totalEvents);

		for (int k = 0; k < totalEvents; k++)
		{
			record_t newEvent;

			savefile->ReadInt(newEvent.timestamp);
			savefile->ReadVec3(newEvent.position);
			savefile->ReadFloat(newEvent.yaw);
			savefile->ReadString(newEvent.deckcommand);
			savefile->ReadString(newEvent.frobcommand);
			savefile->ReadString(newEvent.spawncommand);
			savefile->ReadBool(newEvent.done);
			savefile->ReadInt(newEvent.entID);
			savefile->ReadInt(newEvent.recordtype);

			records[i].events.Append(newEvent);
		}
	}


	//RECORDLINES	
	for (int i = 0; i < 3; i++)
	{
		recordlines[i].lines.Clear();

		int totalLines;
		savefile->ReadInt(totalLines);

		for (int k = 0; k < totalLines; k++)
		{
			recordline_t newLine;

			savefile->ReadVec3(newLine.position1);
			savefile->ReadVec3(newLine.position2);

			recordlines[i].lines.Append(newLine);
		}
	}
}

void idWorldManager::Spawn( void )
{
	int i;
	for (i = 0; i < 3; i++)
	{
		idDict args;

		records[i].index = 0;
		records[i].playing = false;
		records[i].completed = false;
		
		args.Clear();
		args.Set( "model", "models/monster_npc/tris.ase" );
		args.SetInt( "solid", 0 );
		args.Set("skin", "skins/npc/ghost2");
		records[i].ghost = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
		records[i].ghost->Hide();

		recordlines[i].lines.Clear();
	}

	lastPosition = vec3_zero;
	lastAngle = 0;
	nextRecordtime = 0;
	recordStarttime = 0;
	index = 0;
	state = OFF;
	BecomeActive( TH_THINK );
}



void idWorldManager::Event_getcomplete(int idx)
{
	idThread::ReturnInt(records[idx].completed);
}

void idWorldManager::Event_setcomplete(int idx, bool value)
{
	records[idx].completed = value;
}

void idWorldManager::Event_moveplayertoghost(int idx)
{
	//common->Printf("ghost %f %f %f\n", records[idx].ghost->GetPhysics()->GetOrigin().x, records[idx].ghost->GetPhysics()->GetOrigin().y, records[idx].ghost->GetPhysics()->GetOrigin().z);
	gameLocal.GetLocalPlayer()->SetOrigin( records[idx].ghost->GetPhysics()->GetOrigin() + idVec3(0,0,0) );
}

void idWorldManager::Event_recordclear(int idx)
{
	if (idx < 0)
	{
		return;
	}

	//erase all events.
	records[idx].events.Clear();
}

void idWorldManager::Event_setrecordstarttime()
{
	recordStarttime = gameLocal.time;
}

void idWorldManager::Event_getlasteventtimestamp(int idx)
{
	int indexOfLastEvent = records[idx].events.Num();

	if (indexOfLastEvent <= 0)
	{
		idThread::ReturnInt(0);
		return;
	}

	int time = records[idx].events[indexOfLastEvent - 1].timestamp;

	time += gameLocal.time;

	idThread::ReturnFloat(time / 1000.0f);
}

void idWorldManager::Event_geteventcount(int idx)
{	
	idThread::ReturnInt(	records[idx].events.Num() );
}

void idWorldManager::Event_reset(void)
{
	int i;

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		if (gameLocal.entities[i]->IsType(idMoveableItem::Type))
		{
			gameLocal.entities[i]->GetPhysics()->PutToRest();

			gameLocal.entities[i]->SetOrigin(static_cast<idMoveableItem *>( gameLocal.entities[i] )->originalPosition);
			gameLocal.entities[i]->SetAxis(static_cast<idMoveableItem *>( gameLocal.entities[i] )->originalAngle);


			if (static_cast<idMoveableItem *>( gameLocal.entities[i] )->frozen == 2)
			{
				static_cast<idMoveableItem *>( gameLocal.entities[i] )->frozen = 1;
			}
		}
		else if (gameLocal.entities[i]->IsType(idLever::Type))
		{
			static_cast<idLever *>( gameLocal.entities[i] )->Reset();
		}
		else if (gameLocal.entities[i]->IsType(idMoveable::Type))
		{
			gameLocal.entities[i]->GetPhysics()->PutToRest();

			gameLocal.entities[i]->SetOrigin(static_cast<idMoveable *>( gameLocal.entities[i] )->originalPosition);
			gameLocal.entities[i]->SetAxis(static_cast<idMoveable *>( gameLocal.entities[i] )->originalAngle);				

			gameLocal.entities[i]->GetPhysics()->PutToRest();
		}
		else if (gameLocal.entities[i]->IsType(idMover::Type))
		{
			//gameLocal.entities[i]->GetPhysics()->PutToRest();
			if (gameLocal.entities[i]->spawnArgs.GetInt("resetmover", "0") <= 0)
			{
				continue;
			}

			static_cast<idMover *>( gameLocal.entities[i] )->Event_StopMoving();
			static_cast<idMover *>( gameLocal.entities[i] )->Event_StopRotating();


			if ( gameLocal.entities[i]->GetBindMaster() == NULL )
			{
				gameLocal.entities[i]->SetOrigin( static_cast<idMover *>(gameLocal.entities[i] )->originalPosition );
			}
			else
			{
				if (gameLocal.entities[i]->GetBindMaster()->IsType(idMover::Type))
				{
					idVec3 masterPos = GetLocalCoordinates( static_cast<idMover *>(gameLocal.entities[i]->GetBindMaster())->originalPosition );
					gameLocal.entities[i]->SetOrigin(  static_cast<idMover *>(gameLocal.entities[i] )->originalPosition - masterPos);
				}
				else
				{
					idVec3 orgPos = GetLocalCoordinates( static_cast<idMover *>(gameLocal.entities[i])->originalPosition );
					gameLocal.entities[i]->SetOrigin(  orgPos );
				}
			}


			gameLocal.entities[i]->SetAxis(static_cast<idMover *>( gameLocal.entities[i] )->originalAngle);				
		}
		else if (gameLocal.entities[i]->IsType(idTrembler::Type))
		{
			static_cast<idTrembler *>( gameLocal.entities[i] )->Reset();
		}
	}

	//popcorn reset.

	gameLocal.GetLocalPlayer()->popcornPosition = vec3_zero;
}

void idWorldManager::Event_recordstart( int idx, int continuation )
{
	lastPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();

	if (!continuation)
	{
		recordStarttime = gameLocal.time;

		//reset the record lines.
		if (idx >= 0)
		{
			recordlines[idx].lines.Clear();
		}
	}

	this->index = idx;

	if (idx >= 0)
	{
		state = RECORDING;
		records[index].ghost->Hide();

		if (!continuation)
		{
			records[index].events.Clear();
		}
	}


	//worldmanager_moveable

	int i;
	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		if (!gameLocal.entities[i]->IsType(idWorldManager_Moveable::Type))
			continue;

		int role = gameLocal.entities[i]->spawnArgs.GetInt("role", "-1");

		if (role != index)
			continue;

		static_cast<idWorldManager_Moveable *>( gameLocal.entities[i])->Event_startrecord( continuation );
	}
	
}

void idWorldManager::Event_recordstop( void )
{
	int i;

	state = OFF;

	this->index = 0;

	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		if (!gameLocal.entities[i]->IsType(idWorldManager_Moveable::Type))
			continue;

		static_cast<idWorldManager_Moveable *>( gameLocal.entities[i])->Event_stop();
	}
}

void idWorldManager::Event_recordplay( int idx )
{
	int i;

	if (records[idx].events.Num() <= 0)
	{
		return;
	}


	records[idx].index = 0;
	records[idx].playing = true;
	records[idx].ghost->Show();
	records[idx].ghost->SetSkin(declManager->FindSkin(  this->spawnArgs.GetString(va("skin%d", idx), "skins/npc/ghost2")  ));

	//flag every event as NOT DONE.
	for (i = 0; i < records[idx].events.Num(); i++)
	{
		records[idx].events[i].done = false;
	}


	//the colored line that the operative poops out.
	if (index == -1)
	{
		recordline_t newLine;
		newLine.position1 = records[idx].events[0].position;
		newLine.position2 = records[idx].events[0].position;
		recordlines[idx].lines.Append(newLine);
	}


	for ( i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		if (!gameLocal.entities[i]->IsType(idWorldManager_Moveable::Type))
			continue;

		int role = gameLocal.entities[i]->spawnArgs.GetInt("role", "-1");

		if (role == index)
			continue;

		static_cast<idWorldManager_Moveable *>( gameLocal.entities[i])->Event_startplay();
	}
}

void idWorldManager::RecordDeck( const char *deckString )
{
	if (state == OFF)
	{
		return;
	}

	record_t newEvent;
	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.deckcommand = deckString;
	
	records[index].events.Append( newEvent );
}

void idWorldManager::RecordFrob( const char *entName )
{
	if (state == OFF)
	{
		return;
	}

	record_t newEvent;
	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.frobcommand = entName;
	
	records[index].events.Append( newEvent );
}

void idWorldManager::RecordUnspawn( int id )
{
	if (state == OFF)
	{
		return;
	}

	record_t newEvent;
	newEvent.recordtype = RECORD_UNSPAWN;
	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.entID = id;
	
	records[index].events.Append( newEvent );
}

void idWorldManager::RecordSpawn( const char *defName, int id, const idVec3 &pos, float yaw )
{
	if (state == OFF)
	{
		return;
	}

	record_t newEvent;
	newEvent.recordtype = RECORD_SPAWN;
	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.spawncommand = defName;
	newEvent.position = pos;
	newEvent.yaw = yaw;
	newEvent.entID = id;
	
	records[index].events.Append( newEvent );

	if (developer.GetBool())
	{
		common->Printf("worldmanager: recording entity spawn ID %d     defname %s  \n", id, defName);
	}
}

void idWorldManager::RecordLaunchAim( int id, const idVec3 &dir, float force )
{
	if (state == OFF)
	{
		return;
	}
	
	if (id < 0)
		return;

	record_t newEvent;
	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.recordtype = RECORD_LAUNCHAIM;
	newEvent.entID = id;		//id #
	newEvent.position = dir;	//direction.
	newEvent.yaw = force;		//force.
	
	records[index].events.Append( newEvent );


	if (developer.GetBool())
	{
		common->Printf("worldmanager: recording launcher aim ID %d  \n", id);
	}
}

void idWorldManager::UpdatePlay( int idx )
{
	int currentIndex = records[idx].index;

	if (currentIndex > records[idx].events.Num() - 1)
	{
		return;
	}

	if (records[idx].events[currentIndex].timestamp < gameLocal.time - recordStarttime)
	{
		if (!records[idx].events[currentIndex].frobcommand.IsEmpty() && !records[idx].events[currentIndex].done)
		{
			//FROB EVENT.
			idEntity *frobEnt;
			frobEnt = gameLocal.FindEntity( records[idx].events[currentIndex].frobcommand );
			if (frobEnt)
			{

				if (frobEnt->IsType(idTrigger_Multi::Type))
				{
					static_cast<idTrigger_Multi *>( frobEnt )->TriggerAction(NULL);
				}
				else
				{
					gameLocal.GetLocalPlayer()->DoFrob(frobEnt);
					//common->Printf("frobbing! %d\n", gameLocal.time);
				}
			}
			else
			{
				gameLocal.Warning("WorldManager couldn't find frob ent: %s\n", gameLocal.FindEntity( records[idx].events[currentIndex].frobcommand.c_str()));
			}

			records[idx].events[currentIndex].done = true;
		}
		else if (records[idx].events[currentIndex].recordtype == RECORD_LAUNCHAIM && !records[idx].events[currentIndex].done)
		{
			//event: change launcher aim.
			
			//find the launcher with this id.
			int i;
			int targetEnt = -1;

			for ( i = 0; i < gameLocal.num_entities; i++ )
			{
				if ( !gameLocal.entities[ i ] )
					continue;

				int kk = records[idx].events[currentIndex].entID;

				if (gameLocal.entities[i]->spawnArgs.GetInt("id") != records[idx].events[currentIndex].entID)
				{
					continue;
				}

				targetEnt = i;
				break;
			}

			if (developer.GetBool())
			{
				common->Printf("worldmanager: aiming launcher entity ID %d  \n", records[idx].events[currentIndex].entID);
			}

			if (targetEnt >= 0)
			{
				//legacy.
				gameLocal.entities[targetEnt]->spawnArgs.SetVector("new_launchdir", records[idx].events[currentIndex].position);
				gameLocal.entities[targetEnt]->spawnArgs.SetInt("new_force", records[idx].events[currentIndex].yaw);
				gameLocal.entities[targetEnt]->spawnArgs.SetInt("new_update", 1);

				static_cast<idLauncher *>( gameLocal.entities[i] )->SetLaunchDir(records[idx].events[currentIndex].position, records[idx].events[currentIndex].yaw);
			}

			records[idx].events[currentIndex].done = true;
		}
		else if (!records[idx].events[currentIndex].deckcommand.IsEmpty() && !records[idx].events[currentIndex].done)
		{
			//DECK EVENT.
			gameLocal.RunDeckCommand(records[idx].events[currentIndex].deckcommand.c_str());

			records[idx].events[currentIndex].done = true;
		}
		else if (records[idx].events[currentIndex].recordtype == RECORD_UNSPAWN && !records[idx].events[currentIndex].done)
		{
			//UNSPAWN EVENT.
			//find the ent with this id.
			int i;
			int targetEnt = -1;

			for ( i = 0; i < gameLocal.num_entities; i++ )
			{
				if ( !gameLocal.entities[ i ] )
					continue;

				int kk = records[idx].events[currentIndex].entID;

				if (gameLocal.entities[i]->spawnArgs.GetInt("id") != records[idx].events[currentIndex].entID)
				{
					continue;
				}

				targetEnt = i;
				break;
			}

			if (targetEnt >= 0)
			{
				gameLocal.entities[targetEnt]->PostEventMS( &EV_Remove, 0 );
				//gameLocal.entities[targetEnt]->Event_CallFunction("onPick");
			}

			records[idx].events[currentIndex].done = true;
		}
		else if (records[idx].events[currentIndex].recordtype == RECORD_SPAWN && !records[idx].events[currentIndex].done)
		{
			
			//SPAWN EVENT.

			idEntity	*ent;
			idDict		args;
			idAngles	ang;

			int id = gameLocal.time - recordStarttime;

			if (developer.GetBool())
			{
				common->Printf("worldmanager: spawning entity ID %d  classname %s \n", records[idx].events[currentIndex].entID,
					records[idx].events[currentIndex].spawncommand.c_str());
			}

			args.Set( "classname", records[idx].events[currentIndex].spawncommand );
			args.SetVector( "origin", records[idx].events[currentIndex].position );
			args.SetInt( "id", records[idx].events[currentIndex].entID);
			bool bSpawn = gameLocal.SpawnEntityDef( args, &ent );

			if (ent)
			{
				ent->spawnArgs.SetInt("id", records[idx].events[currentIndex].entID);  //BC 7-21-2016 was crashing here.
			
				ang.pitch = 0;
				ang.yaw = records[idx].events[currentIndex].yaw;
				ang.roll = 0;
				ent->SetAngles( ang );
			}

			if (developer.GetBool())
			{
				common->Printf("worldmanager: spawning success:%d\n",  bSpawn);
				
			}

			records[idx].events[currentIndex].done = true;
		}
		else
		{
			//MOVE EVENT.
			float moveTime = (float)RECORDRATE / 1000;
		
			records[idx].ghost->Event_SetMoveTime(moveTime);
			records[idx].ghost->Event_MoveToPos(records[idx].events[currentIndex].position);
			
			idAngles newAng = idAngles(0, records[idx].events[currentIndex].yaw, 0);
			records[idx].ghost->SetAxis(newAng.ToMat3());

			if (index == -1)
			{
				int lastLineIndex = recordlines[idx].lines.Num() - 1;
				recordline_t newLine;
				newLine.position1 = recordlines[idx].lines[lastLineIndex].position2;
				newLine.position2 = records[idx].events[currentIndex].position;

				recordlines[idx].lines.Append(newLine);
			}

			records[idx].events[currentIndex].done = true;
		}

		
		records[idx].index++;
	}
}

void idWorldManager::UpdateRecord( void )
{
	if (state == OFF)
	{
		return;
	}

	if (gameLocal.time < nextRecordtime)
	{
		return;
	}

	nextRecordtime = gameLocal.time + RECORDRATE;

	idVec3 currentPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	int currentAngle = (int)gameLocal.GetLocalPlayer()->viewAngles.yaw;	

	if ((int)lastPosition.x == (int)currentPosition.x && (int)lastPosition.y == (int)currentPosition.y && (int)lastPosition.z == (int)currentPosition.z && (int)lastAngle == (int)currentAngle)
	{
		return;
	}

	lastPosition = currentPosition;
	lastAngle = gameLocal.GetLocalPlayer()->viewAngles.yaw;	
	
	record_t newEvent;
	newEvent.recordtype = 0;
	newEvent.done = false;

	newEvent.timestamp = gameLocal.time - recordStarttime;
	newEvent.position = currentPosition;
	newEvent.yaw = currentAngle;
	newEvent.deckcommand = NULL;
	newEvent.frobcommand = NULL;
	
	records[index].events.Append( newEvent );
}

void idWorldManager::Think( void )
{
	int i;
	UpdateRecord();

	for (i = 0; i < 3; i++)
	{
		idVec4 markerColor;
		idVec3 markerPos;
		int markerRadius = 24;

		if (records[i].playing)
		{
			UpdatePlay(i);
		}
		else
		{
			continue;
		}

		if (state == OFF && index != -1 /*caser*/)
		{
			continue;
		}

		if (this->index == i)
		{
			continue;
		}

		if (!records[i].ghost->IsHidden())
		{
			if ((int)(gameLocal.time * 0.004) % 3 == 0)
			{
				markerRadius = 12;
			}

			switch (i)
			{
				case 0:		markerColor = idVec4(.25, .75, 1, 1); break; //blue.
				case 1:		markerColor = idVec4(.74, .9, .26, 1); break;  //green.
				default:	markerColor = idVec4(1, .6, 0, 1); break; //orange.
			}

			//blue .25, .75, 1
			//yellow 1, .8, 0, 1
			//red .9, .25, .25

			markerPos = records[i].ghost->GetPhysics()->GetOrigin() + idVec3(0,0,85);
			gameRenderWorld->DebugLine( markerColor, markerPos + idVec3(0,0,-markerRadius), markerPos + idVec3(0,0,markerRadius) );
			gameRenderWorld->DebugLine( markerColor, markerPos + idVec3(0,-markerRadius,0), markerPos + idVec3(0,markerRadius,0) );
			gameRenderWorld->DebugLine( markerColor, markerPos + idVec3(-markerRadius,0,0), markerPos + idVec3(markerRadius,0,0) );

			if (index == -1)
			{
				//draw lines.
				int k;

				if (recordlines[i].lines.Num() >= 3)
				{
					for (k = 0; k < recordlines[i].lines.Num() - 1; k++)
					{
						if (k > 2) //BC hack. The initial lines are garbage data. So, just skip them.
						{
							gameRenderWorld->DebugLine( markerColor, 
								recordlines[i].lines[k].position1, recordlines[i].lines[k].position2 );
						}
					}


					//This is the dangly bit that connects the last recordline position to the character model's feet.
					if (records[i].index < recordlines[i].lines.Num() && i > 1)
					{
						gameRenderWorld->DebugLine( markerColor, 
							recordlines[i].lines[records[i].index].position1,
							records[i].ghost->GetPhysics()->GetOrigin() );
					}
				}
			}
		}
	}
}




