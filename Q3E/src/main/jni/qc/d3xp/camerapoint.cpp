
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_setcamerapointvisibility(		"setcamerapointvisibility", "d" );

CLASS_DECLARATION( idEntity, idCameraPoint )
	EVENT( EV_setcamerapointvisibility,		idCameraPoint::Event_setcamerapointvisibility)
	
END_CLASS

void idCameraPoint::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(snapped);
	savefile->WriteObject(arrowModel);
}

void idCameraPoint::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(snapped);
	savefile->ReadObject(reinterpret_cast<idClass *&>(arrowModel));
}

void idCameraPoint::Spawn( void )
{
	this->GetPhysics()->SetContents(0);
	snapped = false;

	renderEntity.weaponDepthHack = true;


	idDict args;
	args.Clear();
	args.SetVector( "origin", this->GetPhysics()->GetOrigin() );
	args.Set( "model", "models/camerapoint_arrow/tris.ase" );
	args.SetInt( "solid", 0 );
	arrowModel = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
	arrowModel->GetRenderEntity()->weaponDepthHack = true;
	arrowModel->Bind(this, false);


	BecomeActive( TH_THINK );
}

void idCameraPoint::SetSnapped()
{
	SetSkin(declManager->FindSkin("skins/camerapoint/dotted"));

	arrowModel->Hide();
	snapped = true;
}

void idCameraPoint::SetHover(int value)
{
	if (value > 0)
	{
		SetSkin(declManager->FindSkin("skins/camerapoint/hover"));
		arrowModel->SetSkin(declManager->FindSkin("skins/camerapoint_arrow/green"));
	}
	else
	{
		SetSkin(declManager->FindSkin("skins/camerapoint/skin"));
		arrowModel->SetSkin(declManager->FindSkin("skins/camerapoint_arrow/skin"));
	}
}

void idCameraPoint::Event_setcamerapointvisibility( int value)
{
	if (value <= 0)
	{
		//hide it.
		arrowModel->Hide();
		this->Hide();
		return;
	}

	//else, show it.

	this->Show();

	if (snapped)
		return;

	arrowModel->Show(); //but only show arrow if pic hasnt been snapped yet.
}

/*
void idCameraPoint::Think( void )
{
	//Present();
}*/

/*
void idCameraPoint::Present( void )
{

	//blink speed.
	if (!snapped && gameLocal.GetLocalPlayer()->isCasing)
	{
		if ((int)(gameLocal.time * 0.004) % 4 != 0)
		{
			int lineLength = 128;
			//idVec3 ang = this->GetPhysics()->GetAxis().ToAngles().ToForward();
			//gameRenderWorld->DebugCircle(idVec4(1,1,1,1), this->GetPhysics()->GetOrigin(), ang, 15, 9);

			float modifier = 8 * idMath::Sin(gameLocal.time * 0.008);
			gameRenderWorld->DebugArrow(idVec4(.8f, .8f, .8f, 1), this->GetPhysics()->GetOrigin()  + idVec3(0,0, lineLength + modifier*4), this->GetPhysics()->GetOrigin() + idVec3(0,0,32 + modifier), 16);
			gameRenderWorld->DebugArrow(idVec4(.8f, .8f, .8f, 1), this->GetPhysics()->GetOrigin()  + idVec3(0,0, -lineLength + modifier*4), this->GetPhysics()->GetOrigin() + idVec3(0,0,-32 - modifier), 16);
		}
	}

	idEntity::Present();
}*/