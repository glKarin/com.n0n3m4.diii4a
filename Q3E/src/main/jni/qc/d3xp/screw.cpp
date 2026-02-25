
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#ifdef STEAM
#include "steamstats.h"
#endif

const idEventDef EV_screw_out( "<screw_out>" );
const idEventDef EV_screw_reset( "screwreset" );

CLASS_DECLARATION( idMoveableItem, idScrew )
	EVENT( EV_screw_out,		idScrew::ScrewOut)
	EVENT( EV_screw_reset,		idScrew::Reset)
END_CLASS



void idScrew::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(unscrewed);
	savefile->WriteObject(moverItem);
}

void idScrew::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(unscrewed);
	savefile->ReadObject(reinterpret_cast<idClass *&>(moverItem));
}

void idScrew::Spawn( void )
{
	unscrewed = false;
	//this->GetPhysics()->SetContents(0);
	this->isFrobbable = 1;

	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	//args.Set( "model", spawnArgs.GetString("model", "models/bolt/tris.ase") );
	//args.Set( "clipmodel", spawnArgs.GetString("clipmodel2", "models/bolt/tris_cm.ase") );
	//args.Set("skin", "skins/bolt/noglow");
	//physicsItem = ( idMoveableItem * )gameLocal.SpawnEntityType( idMoveableItem::Type, &args );
	//physicsItem->Hide();

	idDict args;
	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() );
	args.SetInt( "solid", 0 );
	args.SetInt( "frobbable", 0 );
	moverItem = ( idMover * )gameLocal.SpawnEntityType( idMover::Type, &args );

	this->Bind(moverItem, true);
}

void idScrew::Frob( void )
{
	this->isFrobbable = 0;
	//this->GetPhysics()->SetContents(0);


	this->Unbind();
	moverItem->SetOrigin(this->GetPhysics()->GetOrigin());
	moverItem->SetAxis(this->GetPhysics()->GetAxis());
	this->Bind(moverItem, true);


	StartSound( "snd_unscrew", SND_CHANNEL_ANY, 0, false, NULL );

	float forward = this->GetPhysics()->GetAxis().ToAngles().ToForward().ToYaw();

	/*
	common->Printf("%f %f %f\n", this->GetPhysics()->GetAxis().ToAngles().ToForward().x,
		this->GetPhysics()->GetAxis().ToAngles().ToForward().y,
		this->GetPhysics()->GetAxis().ToAngles().ToForward().z);
		*/


	if ((int)this->GetPhysics()->GetAxis().ToAngles().ToForward().x == 0
		&& (int)this->GetPhysics()->GetAxis().ToAngles().ToForward().y == 0
		&& (int)this->GetPhysics()->GetAxis().ToAngles().ToForward().z == 1)
	{
		forward = -1;
	}
	else if ((int)this->GetPhysics()->GetAxis().ToAngles().ToForward().x == 0
		&& (int)this->GetPhysics()->GetAxis().ToAngles().ToForward().y == 0
		&& (int)this->GetPhysics()->GetAxis().ToAngles().ToForward().z == -1)
	{
		forward = -2;
	}
	
	float movetime = spawnArgs.GetFloat("movetime", "0.4");
	float movedist = spawnArgs.GetFloat("movedist", "5");
	float rolldist = spawnArgs.GetFloat("rolldist", "16");

	rolldist = rolldist * -1.0f;
	
	moverItem->Event_SetMoveTime(movetime);
	moverItem->Event_SetAccellerationTime(16);
	idAngles rotateAngle(0, 0, rolldist);
	moverItem->Event_Rotate(rotateAngle);
	moverItem->Event_MoveDir( forward, movedist );

	SetSkin(declManager->FindSkin( "skins/bolt/noglow" ));
	unscrewed = true;



	PostEventSec( &EV_screw_out, movetime + 0.02f );

#ifdef STEAM
	g_SteamStats->SetStatDelta("stat_screws", 1);
#endif
}


void idScrew::ScrewOut( void )
{
	//screw has reached the Out position.
	//ActivateTargets( this );   //CRASH.

	//hack
	idStr targetName = spawnArgs.GetString( "target0" );

	if ( !targetName.Length() )
	{
		targetName = spawnArgs.GetString( "target" );
	}

	if ( targetName.Length() )
	{
		idEntity *ent = gameLocal.FindEntity( targetName );

		if (ent)
		{
			if (ent->IsType(idPowerScrewGeneric::Type))
			{
				idPowerScrewGeneric *gate = static_cast<idPowerScrewGeneric *>( ent );
				gate->OnActivate();
			}
			else
			{
				ActivateTargets(this);
			}
		}
	}


	//physicsItem->SetOrigin( this->GetPhysics()->GetOrigin() );
	//physicsItem->SetAngles( this->GetPhysics()->GetAxis().ToAngles() );
	this->Unbind();

	int popamount = spawnArgs.GetInt("popamount");
	
	if (popamount > 0)
	{
		this->GetPhysics()->SetLinearVelocity( (this->GetPhysics()->GetAxis().ToAngles().ToForward() + idVec3(0.1f,0.1f,0.1f)) * popamount );
	}
	else
		this->GetPhysics()->SetLinearVelocity( idVec3(0,0,0) );
	
	this->isFrobbable = 1;
	//physicsItem->Show();
	//this->Hide();

	//GetPhysics()->SetContents( CONTENTS_SOLID );

	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

}






void idScrew::Reset( void )
{
	this->Unbind();

	unscrewed = false;
	moverItem->SetOrigin(this->originalPosition);
	moverItem->SetAxis(this->originalAngle);

	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->GetPhysics()->SetOrigin( this->originalPosition);
	this->GetPhysics()->SetAxis( this->originalAngle);
	this->UpdateVisuals();

	this->Bind(moverItem, true);
	//physicsItem->Hide();
	//this->Show();

	SetSkin(declManager->FindSkin( "skins/bolt/glow" ));

	this->isFrobbable = 1;

	moverItem->Event_StopMoving();
	moverItem->Event_StopRotating();
}