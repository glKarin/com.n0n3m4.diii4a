
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#define CONNECT_TIME		2000
#define SCREW_DELAY			0.5f
#define CONNECT_DISTANCE	48
#define ATTRACTOR_LIFETIME	1000   //milliseconds for attractor to stay alive.

#define CONNECT_DELAY		400

const idEventDef EV_Panel_OnConnect(	"<Onconnect>" );
const idEventDef EV_Panel_FirstOpen(	"<Panel_FirstOpen>" );
const idEventDef EV_Panel_Disconnect(	"PanelDisconnect" );
const idEventDef EV_Panel_Reset(		"PanelReset" );

CLASS_DECLARATION( idAnimatedEntity, idPanel )
	EVENT( EV_Panel_OnConnect,		idPanel::Event_Panel_OnConnect)
	EVENT( EV_Panel_FirstOpen,		idPanel::Event_Panel_Open)
	EVENT( EV_Panel_Disconnect,		idPanel::Event_Panel_Disconnect)
	EVENT( EV_Panel_Reset,			idPanel::Event_Panel_Reset)
END_CLASS



void idPanel::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(unscrewedCount);
	savefile->WriteObject(relay);
	savefile->WriteObject(frobcube0);
	savefile->WriteObject(frobcube1);
	savefile->WriteObject(frobcube_opened);
	savefile->WriteObject(frobcube_closed);
	savefile->WriteObject(bundle);
	savefile->WriteObject(gatorCable);
	savefile->WriteObject(beamStart);
	savefile->WriteObject(beamEnd);
	savefile->WriteObject(screw0);
	savefile->WriteObject(screw1);
}

void idPanel::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(unscrewedCount);
	savefile->ReadObject(reinterpret_cast<idClass *&>(relay));
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcube0));
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcube1));
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcube_opened));
	savefile->ReadObject(reinterpret_cast<idClass *&>(frobcube_closed));
	savefile->ReadObject(reinterpret_cast<idClass *&>(bundle));
	savefile->ReadObject(reinterpret_cast<idClass *&>(gatorCable));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamStart));
	savefile->ReadObject(reinterpret_cast<idClass *&>(beamEnd));
	savefile->ReadObject(reinterpret_cast<idClass *&>(screw0));
	savefile->ReadObject(reinterpret_cast<idClass *&>(screw1));


	
}

void idPanel::Spawn( void )
{
	//SPAWN SCREWS & SCREW FROB CUBES.	
	idVec3 screwPos0, screwPos1;
	idVec3 forward, right, up;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	screwPos0 = GetPhysics()->GetOrigin()  + (right * -6.8f) + (up * 6.8f);
	idDict args;
	args.Clear();
	args.SetVector( "origin", screwPos0 );
	args.Set( "model", "models/frob/cube_8x8.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "noGrab", 1 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "index", IDX_SCREW0 );
	args.SetInt( "role", this->spawnArgs.GetInt("role"));
	args.SetInt( "recordable", this->spawnArgs.GetInt("recordable") );
	this->frobcube0 = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );

	screwPos1 = GetPhysics()->GetOrigin() +  (right * 6.8f) + (up * 6.8f);
	args.Clear();
	args.SetVector( "origin", screwPos1 );
	args.Set( "model", "models/frob/cube_8x8.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "noGrab", 1 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "index", IDX_SCREW1 );
	args.SetInt( "role", this->spawnArgs.GetInt("role"));
	args.SetInt( "recordable", this->spawnArgs.GetInt("recordable") );
	this->frobcube1 = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );



	args.Clear();
	args.SetVector( "origin", screwPos0 );
	args.Set( "classname", "env_screwanim" );
	gameLocal.SpawnEntityDef( args, &this->screw0 );
	this->screw0->SetAngles( GetPhysics()->GetAxis().ToAngles() );

	args.Clear();
	args.SetVector( "origin", screwPos1 );
	args.Set( "classname", "env_screwanim" );
	gameLocal.SpawnEntityDef( args, &this->screw1 );
	this->screw1->SetAngles( GetPhysics()->GetAxis().ToAngles() );



	args.Clear();
	args.SetVector( "origin", GetPhysics()->GetOrigin() + (forward * -3) + (up * 2) );
	args.Set( "model",		"models/wirebundle/wirebundle_16x3.ase" );
	args.Set( "clipmodel",	"models/wirebundle/wirebundle_16x3_cm.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "noGrab", 1 );
	args.SetInt( "solid", 0 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "index", IDX_BUNDLE );
	args.SetInt( "role", this->spawnArgs.GetInt("role1"));
	//args.SetInt( "recordable", this->spawnArgs.GetInt("recordable") );
	this->bundle = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	this->bundle->SetAngles( GetPhysics()->GetAxis().ToAngles() );
	this->bundle->GetPhysics()->SetContents( 0 );





	args.Clear();
	args.SetVector( "origin", GetPhysics()->GetOrigin() );
	args.Set( "model", "models/panel_16x16/tris_closed_cm.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "solid", 0 );
	args.SetInt( "noGrab", 1 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "index", IDX_DOOR_CLOSED );
	this->frobcube_closed = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	this->frobcube_closed->SetAngles( GetPhysics()->GetAxis().ToAngles() );
	this->frobcube_closed->GetPhysics()->SetContents( 0 );

	args.Clear();
	args.SetVector( "origin", GetPhysics()->GetOrigin() );
	args.Set( "model", "models/panel_16x16/tris_open_cm.ase" );
	args.SetInt( "frobbable", 1 );
	args.SetInt( "corpse", 1 );
	args.SetInt( "solid", 0 );
	args.SetInt( "noGrab", 1 );
	args.Set( "owner", this->GetName() );
	args.SetInt( "index", IDX_DOOR_OPENED );
	this->frobcube_opened = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	this->frobcube_opened->SetAngles( GetPhysics()->GetAxis().ToAngles() );
	this->frobcube_opened->GetPhysics()->SetContents( 0 );


	args.Clear();
	args.Set("classname", "trigger_relay" );
	args.Set("target", this->spawnArgs.GetString("target"));
	args.Set("target0", this->spawnArgs.GetString("target0"));
	gameLocal.SpawnEntityDef(args, &relay);




	//INITIAL SETTINGS.
	unscrewedCount = 0;

	BecomeActive( TH_THINK );
	this->state = IDLE;

	this->gatorCable = NULL;
}





void idPanel::Event_Panel_Reset( void )
{
	//Reset.
	CancelEvents( &EV_Panel_OnConnect );
	CancelEvents( &EV_Panel_FirstOpen );

	this->frobcube_closed->GetPhysics()->SetContents( 0 );
	this->frobcube_opened->GetPhysics()->SetContents( 0 );

	Event_Panel_Disconnect();

	//delete cable.
	if (this->gatorCable != NULL)
	{
		this->gatorCable->PostEventMS( &EV_Remove, 0 );
		this->gatorCable = NULL;
	}

	//reset screws.
	gameLocal.GetLocalPlayer()->UseFrob( screw0, "reset" );
	gameLocal.GetLocalPlayer()->UseFrob( screw1, "reset" );

	this->frobcube0->GetPhysics()->SetContents( CONTENTS_CORPSE );
	this->frobcube1->GetPhysics()->SetContents( CONTENTS_CORPSE );

	//reset anim.
	Event_PlayAnim("closed", 4);

	//settings.
	unscrewedCount = 0;
	this->bundle->GetPhysics()->SetContents( 0 );
}

void idPanel::Event_Panel_OnConnect( void )
{
	if (spawnArgs.GetBool("recordable") > 0)
	{
		static_cast<idTrigger_Multi *>( relay )->Event_Trigger(this);
	}
	else
	{
		ActivateTargets( this );
	}
}

void idPanel::Event_Panel_Disconnect( void )
{
	CancelEvents( &EV_Panel_OnConnect );

	if (this->gatorCable != NULL)
	{
		//frobbed the cable. If connected to deck, then disconnect.
		this->gatorCable->SetSkin(declManager->FindSkin("skins/cable/phone"));
	}

	if (this->state == CONNECTED)
	{
		//disconnect the deck.
		this->state = IDLE;

		idEntity* deck = gameLocal.FindEntityUsingDef(NULL, "moveable_item_deck");

		if (deck)
		{
			deck->spawnArgs.SetInt("phone", 0);
			gameLocal.GetLocalPlayer()->UseFrob( deck, "PhoneOff" );

			deck->StopSound( SND_CHANNEL_VOICE, false );
			deck->StopSound( SND_CHANNEL_VOICE2, false );
		}

		if (this->gatorCable != NULL)
		{
			//make the cable jump a little.
			this->gatorCable->ApplyImpulse( this->gatorCable, 13 , gatorCable->GetPhysics()->GetOrigin(), idVec3(0, 0, 4096));
		}
	}
}


void idPanel::Event_Panel_Open( void )
{
	this->frobcube_opened->GetPhysics()->SetContents( CONTENTS_CORPSE );
	this->frobcube_closed->GetPhysics()->SetContents( 0 );

	this->bundle->GetPhysics()->SetContents( CONTENTS_CORPSE );
	Event_PlayAnim("opening", 4);

	this->StartSound(  "snd_open" , SND_CHANNEL_BODY, 0, false, NULL );
}

void idPanel::Event_Panel_Close( void )
{
	Event_Panel_Disconnect();

	if (this->gatorCable != NULL)
	{
		this->gatorCable->PostEventMS( &EV_Remove, 0 );
		this->gatorCable = NULL;
		this->state = IDLE;
	}

	this->frobcube_opened->GetPhysics()->SetContents( 0 );
	this->frobcube_closed->GetPhysics()->SetContents( CONTENTS_CORPSE );

	this->bundle->GetPhysics()->SetContents( 0 );
	Event_PlayAnim("closing", 4);

	this->StartSound(  "snd_close" , SND_CHANNEL_BODY, 0, false, NULL );
}

void idPanel::OnFrob( idEntity* activator )
{
	int index;
	
	if (!activator->spawnArgs.GetInt( "index", "", index))
	{
		return;
	}

	if (index == IDX_DOOR_OPENED)
	{
		//is opened. Close it.
		Event_Panel_Close();
		return;
	}

	if (index == IDX_DOOR_CLOSED)
	{
		//is opened. Close it.
		Event_Panel_Open();
		return;
	}
		
	//frob screws.
	if (index == IDX_SCREW0 || index == IDX_SCREW1)
	{
		activator->Hide();
		unscrewedCount++;

		if (index == IDX_SCREW0)
		{
			gameLocal.GetLocalPlayer()->UseFrob( this->screw0, "playanim" );
		}
		else
		{
			gameLocal.GetLocalPlayer()->UseFrob( this->screw1, "playanim" );
		}

		if (unscrewedCount >= 2)
		{
			PostEventSec( &EV_Panel_FirstOpen, SCREW_DELAY );
		}

		return;
	}

	if (index == IDX_CABLE)
	{
		Event_Panel_Disconnect();

		return;
	}

	if (index == IDX_BUNDLE)
	{
		//frobbed the wire bundle.
		if (this->gatorCable == NULL)
		{
			idAngles cableAngle;
			idVec3 forward, right, up;
			idDict args;
			idVec3 cablePos;


			//this->connectTime = gameLocal.time;



			this->StartSound(  "snd_plug" , SND_CHANNEL_VOICE, 0, false, NULL );

			//turn off bundle frobbability.
			this->bundle->GetPhysics()->SetContents( 0 );

			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

			//Spawn the cable.
			cablePos = this->bundle->GetPhysics()->GetOrigin() + (forward * 6) + (up * 2);
			args.Clear();
			args.SetVector( "origin", cablePos );
			args.Set( "classname", "env_cable_gator" );
			args.Set( "owner", this->GetName() );
			args.SetInt( "index", IDX_CABLE );
			gameLocal.SpawnEntityDef( args, &this->gatorCable );

			cableAngle = this->GetPhysics()->GetAxis().ToAngles();
			cableAngle.pitch += 100;
			this->gatorCable->SetAngles(cableAngle);




			args.Clear();
			args.SetVector( "origin",		this->bundle->GetPhysics()->GetOrigin() );
			args.Set(		"model",		"sparkburst2.prt" );
			args.SetBool(	"start_off",	false );
			gameLocal.SpawnEntityType( idExplodable::Type, &args );


			/*
			const idDeclParticle *smokeFly = NULL;
			smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, "sparkburst2.prt" ) );
			if (smokeFly)
			{
				gameLocal.smokeParticles->EmitSmoke( smokeFly, gameLocal.time, 0, cablePos, mat3_zero, timeGroup );
			}
			*/


			idVec3 offset;
			idMat3 axis;
			jointHandle_t jointHandle;

			jointHandle = this->gatorCable->GetAnimator()->GetJointHandle( "bone16" );
			if ( static_cast< idAnimatedEntity * >(gatorCable)->GetJointWorldTransform(jointHandle, gameLocal.time, offset, axis ) )
			{
				args.Clear();
				args.SetVector( "origin",		offset );
				args.Set(		"target",		"deck1_beamend" );
				args.SetBool(	"start_off",	false );
				args.Set(		"skin",			"skins/beam_cable" );
				this->beamStart = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
				this->beamStart->BindToJoint(this->gatorCable, "bone16", false);


			}

			gameLocal.GetLocalPlayer()->Event_usePicker(this->gatorCable);
		}



		return;
	}


}

void idPanel::Think( void )
{
	idEntity* deck = gameLocal.FindEntityUsingDef(NULL, "moveable_item_deck");
	if (this->gatorCable != NULL && deck != NULL && state == IDLE)
	{
		idVec3 deckForward, deckRight, deckUp;
		idVec3 connectPos, phonePos;
		float distance;
		idMat3 axis;
		jointHandle_t jointHandle;
		trace_t tr;

		if (deck->spawnArgs.GetInt("phone", "0") > 0)
		{
			return;
		}

		//ignore if player is holding the deck.
		if (gameLocal.GetLocalPlayer()->Event_isHolding(deck))
		{
			return;
		}

		jointHandle = this->gatorCable->GetAnimator()->GetJointHandle( "bone16" );		

		if ( !static_cast< idAnimatedEntity * >(gatorCable)->GetJointWorldTransform(jointHandle, gameLocal.time, phonePos, axis ) )
		{
			gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointHandle, name.c_str() );
			return;
		}
		//gameRenderWorld->DebugCircle( colorWhite,  offset, idVec3(0,0,1), 2, 5, 200);

		
		
		deck->GetPhysics()->GetAxis().ToAngles().ToVectors(&deckForward, &deckRight, &deckUp);
		connectPos = deck->GetPhysics()->GetOrigin() + (deckRight * 8) + (deckUp * 3);

		

		//Check LOS between deck and coupler.
		gameLocal.clip.TracePoint( tr, phonePos, connectPos, MASK_SOLID, this );

		if (gameLocal.GetLocalPlayer()->Event_isHolding(this->gatorCable))
		{
			if (tr.fraction < 1)
			{
				this->beamStart->Hide();
			}
			else if (this->beamStart->IsHidden())
			{
				this->beamStart->Show();
			}

			return;
		}

		if (!this->beamStart->IsHidden())
		{
			this->beamStart->Hide();
		}



		//if (gameLocal.time > this->connectTime + CONNECT_DELAY)
		{
			//Check distance between coupler and deck.
			distance = (phonePos - connectPos).LengthFast();
			//gameRenderWorld->DebugCircle( colorGreen,  connectPos, idVec3(0,0,1), 2, 5, 200);
			if (CONNECT_DISTANCE >= distance)
			{
				deck->StartSound(  "snd_plug" , SND_CHANNEL_VOICE, 0, false, NULL );
				deck->StartSound( "snd_phonedial" , SND_CHANNEL_VOICE2, 0, false, NULL );

				gameLocal.GetLocalPlayer()->UseFrob( deck, "PhoneOn" );
				this->gatorCable->SetSkin(declManager->FindSkin("skins/cable/nophone"));

				this->state = CONNECTED;

				//Spawn attractor for cable.
				idDict args;
				args.Clear();
				args.SetVector( "origin",		deck->GetPhysics()->GetOrigin() + (deckRight * 10) + (deckUp * 1) + (deckForward * -4));
				args.Set(		"joint",		"bone14" );
				args.Set(		"entity",		this->gatorCable->GetName() );
				idEntity *attractor = gameLocal.SpawnEntityType( idAttractor::Type, &args );
				attractor->Bind(deck, false);
				attractor->PostEventMS( &EV_Remove, ATTRACTOR_LIFETIME );

				this->PostEventMS( &EV_Panel_OnConnect, CONNECT_TIME );


			
				const char *smokeName = "tinyburst.prt";
				if ( *smokeName != '\0' )
				{
					const idDeclParticle *smokeFly = NULL;
					smokeFly = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );

					if (smokeFly)
					{
						gameLocal.smokeParticles->EmitSmoke( smokeFly, gameLocal.time, 0,
							connectPos, mat3_zero, timeGroup );
					}
				}
			}
		}

	}

	//
	//delete this;				

	Present();
}



/*
================
Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/

void idPanel::Present( void )
{
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) )
	{
		return;
	}

	BecomeInactive( TH_UPDATEVISUALS );
	
	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() )
	{
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 )
	{
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}
}
