


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"



CLASS_DECLARATION( idStaticEntity, idCommentaryNode )
	
END_CLASS


void idCommentaryNode::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(frobbed);
	savefile->WriteVec3(orgOrigin);
}

void idCommentaryNode::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(frobbed);
	savefile->ReadVec3(orgOrigin);
}

void idCommentaryNode::Spawn( void )
{
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );



	this->isFrobbable = true;
	this->noGrab = true;

	frobbed = false;	
	orgOrigin = GetPhysics()->GetOrigin();

	if (g_commentary.GetBool())
	{
		StartSound( "snd_idle", SND_CHANNEL_ANY, 0, false, NULL );
	}

	BecomeActive( TH_THINK );
}

void idCommentaryNode::Think( void )
{
	if (!frobbed)
	{
		idAngles	ang;
		idVec3		org;

		ang.pitch = ang.roll = 0.0f;
		ang.yaw = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
		SetAngles( ang );

		float scale = 0.005f + entityNumber * 0.00001f;
			
		org = orgOrigin;
		org.z += 2.0f + cos( ( gameLocal.time + 2000 ) * scale ) * 2.0f;
		SetOrigin( org );
	}

	Present();
}

void idCommentaryNode::OnFrob( idEntity* activator )
{
	StopSound( SND_CHANNEL_ANY, false );

	frobbed = true;	
	BecomeInactive( TH_THINK );

	void *buffer;
	idStr rawtext;
	if (fileSystem->ReadFile( va("text/%s.txt", spawnArgs.GetString( "message" )), &buffer) > 0)
	{
		rawtext = (char *) buffer;
		fileSystem->FreeFile( buffer );
	}
	else
	{
		//failed to load text.
		gameLocal.GetLocalPlayer()->Event_hudMessage(va("Failed to find commentary text: %s\n", spawnArgs.GetString( "message" )));
		return;
	}

	
	if (gameLocal.GetLocalPlayer()->toggleCommentary(1, rawtext))
	{
		StartSound( "snd_press", SND_CHANNEL_ANY, 0, false, NULL );
		SetSkin(declManager->FindSkin( "skins/commentary/noglow" ));
	}
}
	


