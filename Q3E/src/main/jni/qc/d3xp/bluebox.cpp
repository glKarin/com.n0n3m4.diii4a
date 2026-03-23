
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"



#define PLUGMOVETIME			0.2
#define DOOROPENTIME			0.3
#define BLUEBOXTRANSITIONTIME	300
#define UNPLUGMOVETIME			0.3

#define EXECUTETIME				0.3



const idEventDef EV_plugConnected( "<plugConnected>", "d" );
const idEventDef EV_doorOpened( "<dooropened>" );
const idEventDef EV_executed( "<executed>" );
const idEventDef EV_unplugdone( "<unplugdone>" );
const idEventDef EV_datajackreset( "datajackreset", "d" );
const idEventDef EV_weevilopendoor( "weevilopendoor" );
const idEventDef EV_getopenstate( "getdatajackopenstate", NULL, 'd');

//const idEventDef EV_camerapointsnap(		"camerapointsnap" );

CLASS_DECLARATION( idAnimatedEntity, idBluebox )
	//EVENT( EV_camerapointsnap,		idCameraPoint::Event_snap)
	EVENT( EV_plugConnected,		idBluebox::PlugConnected)
	EVENT( EV_doorOpened,			idBluebox::DoPlug)
	EVENT( EV_executed,				idBluebox::OnExecuteDone)
	EVENT( EV_unplugdone,			idBluebox::OnUnplugDone)
	EVENT( EV_datajackreset,		idBluebox::Reset)
	EVENT( EV_weevilopendoor,		idBluebox::WeevilOpen)
	EVENT( EV_getopenstate,			idBluebox::GetOpenState)
END_CLASS

void idBluebox::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(blueboxTransitionTime);
	savefile->WriteVec3(blueboxStartPos);
	savefile->WriteJoint(joint_W);
	savefile->WriteJoint(joint_S);
	savefile->WriteJoint(joint_execute);
	savefile->WriteInt(selectedIndex);
	savefile->WriteInt(maxIndex);
	savefile->WriteBool(isOpen);
	savefile->WriteBool(isPlugged);
	savefile->WriteBool(canSelect);

	savefile->WriteObject(plug);
	savefile->WriteObject(cable);
	savefile->WriteObject(bluebox);
}

void idBluebox::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(blueboxTransitionTime);
	savefile->ReadVec3(blueboxStartPos);
	savefile->ReadJoint(joint_W);
	savefile->ReadJoint(joint_S);
	savefile->ReadJoint(joint_execute);
	savefile->ReadInt(selectedIndex);
	savefile->ReadInt(maxIndex);
	savefile->ReadBool(isOpen);
	savefile->ReadBool(isPlugged);
	savefile->ReadBool(canSelect);

	savefile->ReadObject(reinterpret_cast<idClass *&>(plug));
	savefile->ReadObject(reinterpret_cast<idClass *&>(cable));
	savefile->ReadObject(reinterpret_cast<idClass *&>(bluebox));
}

void idBluebox::Spawn( void )
{
	idDict	args;

	args.Clear();
	args.Set(		"model",	"models/datajack_plug/tris.ase" );
	args.SetBool(	"solid",	0);
	plug = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );
	plug->Hide();
	

	idAngles datajackAng = this->GetPhysics()->GetAxis().ToAngles();
	plug->SetAngles(datajackAng);

	isOpen = false;
	isPlugged = false;
	blueboxTransitionTime = 0;
	blueboxStartPos = vec3_zero;
	selectedIndex = 0;
	maxIndex = 0;
	canSelect = 1;

	bluebox = NULL;
	cable = NULL;

	state = READY;

	//corpse setting.
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	BecomeActive( TH_THINK );
}

void idBluebox::Reset( bool closeDoor )
{
	isPlugged = false;
	canSelect = true;
	state = READY;

	if ( closeDoor )
	{
		Event_PlayAnim("idle", 2);
		isOpen = false;
	}

	plug->Hide();

	if (cable != NULL)
	{
		cable->PostEventMS( &EV_Remove, 0 );
		cable = NULL;
	}

	if (bluebox != NULL)
	{
		bluebox->PostEventMS( &EV_Remove, 0 );
		bluebox = NULL;
	}
}

void idBluebox::HandleSelection(int delta)
{
	if (canSelect == 0)
	{
		return;
	}

	canSelect = 0;

	selectedIndex += delta;

	if (selectedIndex > maxIndex)
		selectedIndex = 0;
	else if (selectedIndex < 0)
		selectedIndex = maxIndex;

	//gameLocal.GetLocalPlayer()->DebugMessage(va("selectedindex %d   maxindex %d\n", selectedIndex, maxIndex));
	

	bluebox->GetRenderEntity()->gui[0]->HandleNamedEvent( va("select%d", selectedIndex) );

	StartSound( "snd_button", SND_CHANNEL_ANY, 0, false, NULL );
	
}

void idBluebox::OnUnplugDone()
{
	state = READY;
	isPlugged = 0;

	plug->Hide();
	cable->PostEventMS( &EV_Remove, 0 );
	bluebox->PostEventMS( &EV_Remove, 0 );

	cable = NULL;
	bluebox = NULL;
}

void idBluebox::OnExecuteDone()
{
	bluebox->Hide();

	idEntity *targetEnt = gameLocal.FindEntity(spawnArgs.GetString(va("target%d", selectedIndex)));

	if (targetEnt)
	{
		if ( targetEnt->RespondsTo( EV_Activate ) || targetEnt->HasSignal( SIG_TRIGGER ) )
		{
			targetEnt->Signal( SIG_TRIGGER );
			targetEnt->ProcessEvent( &EV_Activate, this );
		}
	}

	state = PLUGTRANSITION;
	AttachBlueboxToCable();

	idDict args;
	args.Clear();
	args.SetVector( "origin",		plug->GetPhysics()->GetOrigin() );
	args.Set(		"model",	    spawnArgs.GetString("smoke_unplug", "smokeburst12.prt") );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );

	StartSound( "snd_plugin", SND_CHANNEL_ANY, 0, false, NULL );

	//Unplug the plug.
	plug->Event_SetMoveTime(UNPLUGMOVETIME);
	idVec3 forward, up;
	idVec3 eyePos = gameLocal.GetLocalPlayer()->GetEyePosition();
	idAngles eyeAng = gameLocal.GetLocalPlayer()->viewAngles;
	eyeAng.ToVectors(&forward, NULL, &up);	

	idVec3 plugEndPos = eyePos + (forward * 5) + (up * -2);

	idVec3 plugForward, plugUp;
	idAngles selfAng = this->GetPhysics()->GetAxis().ToAngles();	
	selfAng.ToVectors(&plugForward, NULL, &plugUp);
	
	plug->Event_MoveToPos(plugEndPos);
	PostEventSec( &EV_unplugdone, UNPLUGMOVETIME );
}

void idBluebox::OnExecute()
{
	StartSound( "snd_confirm", SND_CHANNEL_ANY, 0, false, NULL );
	bluebox->GetRenderEntity()->gui[0]->HandleNamedEvent( va("blink%d", selectedIndex) );
	bluebox->GetAnimator()->SetJointPos(joint_execute, JOINTMOD_LOCAL, idVec3(-0.6f, 0, 0));

	PostEventSec( &EV_executed, EXECUTETIME );	
}

void idBluebox::OnDrop()
{
	state = READY;

	AttachBlueboxToCable();
}

void idBluebox::OnDown()
{
	if (state != BLUEBOXHELD)
	{
		return;
	}

	bluebox->GetAnimator()->SetJointPos(joint_S, JOINTMOD_LOCAL, idVec3(-0.2f, 0, 0));

	HandleSelection(1);
}

void idBluebox::OnUp()
{
	if (state != BLUEBOXHELD)
	{
		return;
	}

	bluebox->GetAnimator()->SetJointPos(joint_W, JOINTMOD_LOCAL, idVec3(-0.2f, 0, 0));

	HandleSelection(-1);
}

void idBluebox::OnNone()
{
	bluebox->GetAnimator()->SetJointPos(joint_W, JOINTMOD_LOCAL, idVec3(0, 0, 0));
	bluebox->GetAnimator()->SetJointPos(joint_S, JOINTMOD_LOCAL, idVec3(0, 0, 0));

	canSelect = 1;
}

void idBluebox::PlugConnected(bool playEffects)
{	
	if (playEffects )
	{
		idDict args;

		args.Clear();
		args.SetVector( "origin",		plug->GetPhysics()->GetOrigin() );
		args.Set(		"model",	    spawnArgs.GetString("smoke_plug", "plugconnect.prt") );
		args.SetBool(	"start_off",	false );
		gameLocal.SpawnEntityType( idExplodable::Type, &args );

		StartSound( "snd_plugin", SND_CHANNEL_ANY, 0, false, NULL );
	}
	

	gameLocal.GetLocalPlayer()->Event_usePicker(cable);

	state = BLUEBOXTRANSITION;	
	blueboxTransitionTime = gameLocal.time;
	blueboxStartPos = bluebox->GetPhysics()->GetOrigin();
	bluebox->Unbind();
}

void idBluebox::AttachBlueboxToCable()
{
	idVec3 cableEndPos;
	jointHandle_t			cableEndJoint;		
	idMat3 cableAxis;
	cableEndJoint = cable->GetAnimator()->GetJointHandle( "bone15" );

	cable->GetAnimator()->GetJointTransform( cableEndJoint, gameLocal.time, cableEndPos, cableAxis );
	cableEndPos = cable->GetRenderEntity()->origin + cableEndPos * cable->GetRenderEntity()->axis;

	idAngles blueboxAng = idAngles(0,0,0);
	blueboxAng.roll = 180;
	blueboxAng.yaw = this->GetPhysics()->GetAxis().ToAngles().yaw;
	bluebox->SetAngles(blueboxAng);

	bluebox->SetOrigin( cableEndPos);		
	bluebox->BindToJoint( cable, cableEndJoint, false );
}

void idBluebox::DoPlug()
{
	if (!isPlugged)
	{
		//Not plugged in. Plug in.
		isPlugged = true;

		idVec3 forward, up;
		idVec3 eyePos = gameLocal.GetLocalPlayer()->GetEyePosition();
		idAngles eyeAng = gameLocal.GetLocalPlayer()->viewAngles;
		eyeAng.ToVectors(&forward, NULL, &up);
	

		idVec3 plugStartPos = eyePos + (forward * 5) + (up * -2);

		plug->GetPhysics()->SetOrigin(plugStartPos);
		plug->Show();


		idVec3 plugForward, plugUp;
		idAngles selfAng = this->GetPhysics()->GetAxis().ToAngles();	
		selfAng.ToVectors(&plugForward, NULL, &plugUp);
		idVec3 plugEndPos = this->GetPhysics()->GetOrigin() + (plugForward * 0.4)  + (plugUp * 1.1);


		plug->Event_SetMoveTime( PLUGMOVETIME );
		plug->Event_MoveToPos(plugEndPos);
		PostEventSec( &EV_plugConnected, PLUGMOVETIME,  true);
		

		
		//spawn the cable.
		idDict args;
		args.Clear();
		args.SetBool(	"solid", 0);
		args.SetBool(	"isremote", 1);
		args.Set(		"classname", "env_cable_60");
		args.Set(		"master", this->GetName() );
		gameLocal.SpawnEntityDef(args, &cable);
		cable->SetOrigin(plug->GetPhysics()->GetOrigin());
		cable->Bind(plug, false);

		
		//spawn the bluebox.
		args.Clear();
		args.SetBool(	"solid", 0);
		args.Set(		"gui", "guis/bluebox.gui");
		args.Set(		"classname", "moveable_bluebox");
		gameLocal.SpawnEntityDef(args, &bluebox);


		//set the gui parms.
		bluebox->GetRenderEntity()->gui[0]->SetStateString("gui_parm0", spawnArgs.GetString("string0", ""));
		bluebox->GetRenderEntity()->gui[0]->SetStateString("gui_parm1", spawnArgs.GetString("string1", ""));
		bluebox->GetRenderEntity()->gui[0]->SetStateString("gui_parm2", spawnArgs.GetString("string2", ""));
		bluebox->GetRenderEntity()->gui[0]->SetStateString("gui_parm3", spawnArgs.GetString("string3", ""));

		joint_W = bluebox->GetAnimator()->GetJointHandle( "button1" );
		joint_S = bluebox->GetAnimator()->GetJointHandle( "button2" );
		joint_execute = bluebox->GetAnimator()->GetJointHandle( "execute_bone" );


		//determine the max amount of options in the bluebox.
		idStr str3 = spawnArgs.GetString("string3", "");
		idStr str2 = spawnArgs.GetString("string2", "");
		idStr str1 = spawnArgs.GetString("string1", "");


		if (str3.Length() > 0)
			maxIndex = 3;	
		else if (str2.Length() > 0)
			maxIndex = 2;		
		else if (str1.Length() > 0)
			maxIndex = 1;


		//select index 0.
		selectedIndex = 0;
		bluebox->GetRenderEntity()->gui[0]->HandleNamedEvent( "select0" );
		
			

		AttachBlueboxToCable();
		
	}
	else
	{
		//Already plugged in. Pick up the plugged in bluebox.

		PlugConnected(false);
	}
}

void idBluebox::OnFrob()
{
	if (state != READY)
	{
		return;
	}

	state = PLUGTRANSITION;



	//check all other datajacks. If another datajack has a bluebox attached to it, then detach it.
	for ( int i = 0; i < gameLocal.num_entities; i++ )
	{
		if ( !gameLocal.entities[ i ] )
			continue;

		if (gameLocal.entities[i]->entityNumber == this->entityNumber)
			continue;

		if (!gameLocal.entities[i]->IsType(idBluebox::Type))
		{
			continue;
		}

		static_cast<idBluebox *>( gameLocal.entities[i] )->Reset(false);
	}



	if (!isOpen)
	{
		Event_PlayAnim("open", 2);
		isOpen = true;

		StartSound( "snd_opendoor", SND_CHANNEL_ANY, 0, false, NULL );
		PostEventSec( &EV_doorOpened, DOOROPENTIME );
		return;
	}
	else
	{
		DoPlug();
	}

	bluebox->GetAnimator()->SetJointPos(joint_execute, JOINTMOD_LOCAL, idVec3(0, 0, 0));
}

void idBluebox::GetOpenState( void )
{
	idThread::ReturnInt(isOpen);
}

void idBluebox::WeevilOpen( void )
{
	Event_PlayAnim("open", 2);
	isOpen = true;
	StartSound( "snd_opendoor", SND_CHANNEL_ANY, 0, false, NULL );

	idVec3 plugForward, plugUp;
	idAngles selfAng = this->GetPhysics()->GetAxis().ToAngles();	
	selfAng.ToVectors(&plugForward, NULL, &plugUp);
	idVec3 plugEndPos = this->GetPhysics()->GetOrigin() + (plugForward * 0.4)  + (plugUp * 1.1);

	idDict args;
	args.Clear();
	args.SetVector( "origin",		plugEndPos );
	args.Set(		"model",	    spawnArgs.GetString("smoke_unplug", "smokeburst12.prt") );
	args.SetBool(	"start_off",	false );
	gameLocal.SpawnEntityType( idExplodable::Type, &args );
}




void idBluebox::Think( void )
{
	if (state == BLUEBOXTRANSITION || state == BLUEBOXHELD)
	{
		idVec3 forward, up, right;
		idVec3 eyePos = gameLocal.GetLocalPlayer()->GetEyePosition();
		idAngles eyeAng = gameLocal.GetLocalPlayer()->viewAngles;
		eyeAng.ToVectors(&forward, &right, &up);

		idVec3 offset;
		spawnArgs.GetVector("blueboxoffset", "0 0 0", offset);
		idVec3 blueboxEndPos = eyePos + (forward * offset.x) +  (right * offset.y) + (up * offset.z);	


		idAngles blueboxAng = eyeAng;	
		blueboxAng.yaw += 180;
		blueboxAng.pitch = -blueboxAng.pitch;


		

		if (state == BLUEBOXTRANSITION)
		{
	
			int elapsedTime = gameLocal.time - blueboxTransitionTime;
			float lerp = (float)elapsedTime / BLUEBOXTRANSITIONTIME;

			if (lerp > 1 )
				lerp = 1;
		

			idVec3 lerpPos;

			lerpPos.x = idMath::Lerp(blueboxStartPos.x, blueboxEndPos.x, lerp);	
			lerpPos.y = idMath::Lerp(blueboxStartPos.y, blueboxEndPos.y, lerp);	
			lerpPos.z = idMath::Lerp(blueboxStartPos.z, blueboxEndPos.z, lerp);	


			//common->Printf("lerp %f    pos %f %f %f\n", lerp,lerpPos.x,lerpPos.y,lerpPos.z);


			bluebox->SetOrigin(lerpPos);
			bluebox->SetAngles(blueboxAng);

			if (gameLocal.time >= blueboxTransitionTime + BLUEBOXTRANSITIONTIME)
			{
				state = BLUEBOXHELD;
			}
		}
		else if (state == BLUEBOXHELD)
		{
			bluebox->SetOrigin(blueboxEndPos);
			bluebox->SetAngles(blueboxAng);
		}

		/*
		idVec3 cableEndPos;
		jointHandle_t			cableEndJoint;		
		idMat3 cableAxis;
		cableEndJoint = cable->GetAnimator()->GetJointHandle( "bone15" );
		cable->GetAnimator()->GetJointTransform( cableEndJoint, gameLocal.time, cableEndPos, cableAxis );



		gameRenderWorld->DebugLine(idVec4(0,1,0,1), cableEndPos, bluebox->GetPhysics()->GetOrigin(), 10);
		*/
	}

	idAnimatedEntity::Present();
}
