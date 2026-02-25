
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


#ifdef STEAM
#include "steamstats.h"
#endif

//#define ROCKETSPEED			256

const idEventDef EV_glassreset(	"glassreset" );

CLASS_DECLARATION( idEntity, idQGlass )
	EVENT( EV_glassreset,			idQGlass::Event_glassreset)
END_CLASS


void idQGlass::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(state);
	savefile->WriteInt(count);
	savefile->WriteString(fxFracture);
}

void idQGlass::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(state);
	savefile->ReadInt(count);
	savefile->ReadString(fxFracture);
}

void idQGlass::Spawn( void )
{
	fxFracture = spawnArgs.GetString( "fx" );
	count = spawnArgs.GetInt("count", "3");

	this->GetPhysics()->SetContents( CONTENTS_SOLID );

	renderEntity.noShadow = true;
	renderEntity.noSelfShadow = true;


	//precache.
	common->Printf("\n------------- Caching glass debris -------------\n");
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_debris" );	
	while ( kv )
	{
		const idDict *debris_args = gameLocal.FindEntityDefDict( kv->GetValue(), false );
		if ( debris_args )
		{
			const idDeclEntityDef *debrisDef = gameLocal.FindEntityDef( kv->GetValue(), false );
			if (debrisDef)
			{
				const char *clipModelName;				
				debrisDef->dict.GetString( "clipmodel", "", &clipModelName );

				if ( !clipModelName[0] )
				{
					clipModelName = debrisDef->dict.GetString( "model" );		// use the visual model
				}

				// load the trace model
				idTraceModel trm;
				collisionModelManager->TrmFromModel( clipModelName, trm );
			}
		}

		kv = spawnArgs.MatchPrefix( "def_debris", kv );
	}
}

void idQGlass::Event_glassreset( void )
{
	this->state = IDLE;
	this->Show();
	GetPhysics()->SetContents(CONTENTS_SOLID);
}

void idQGlass::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force )
{
	if (state == DESTROYED)
		return;

	//hide the brush.
	this->Hide();


	state = DESTROYED;
	GetPhysics()->SetContents(0);

	//audio.
	StartSound( "snd_shatter", SND_CHANNEL_ANY, 0, false, NULL );

	//particles.
	if ( fxFracture.Length() )
	{
		idEntityFx::StartFx( fxFracture, &point, &GetPhysics()->GetAxis(), this, true );
	}


	idBounds bounds = this->GetPhysics()->GetAbsBounds();
	idVec3			tmin;
	idVec3			tmax;

	tmin[0] = bounds[0][0] ;
	tmin[1] = bounds[0][1] ;
	tmin[2] = bounds[0][2] ;

	tmax[0] = bounds[1][0] ;
	tmax[1] = bounds[1][1] ;
	tmax[2] = bounds[1][2] ;

	int i;
	//GetPhysics()->GetOrigin()

	//gameRenderWorld->DebugSphere(idVec4(1,1,1,1), idSphere(tmin + GetPhysics()->GetOrigin(), 4), 10000);
	//gameRenderWorld->DebugSphere(idVec4(1,0,0,1), idSphere(tmax + GetPhysics()->GetOrigin(), 4), 10000);


	//facingAxis 0, 1 = wall panes. facingAxis 2 = ceiling/floor pane.
	int facingAxis = 0;
	for ( i = 1 ; i < 3 ; i++ )
	{
		if ( bounds[1][ i ] - bounds[0][ i ] < bounds[1][ facingAxis ] - bounds[0][ facingAxis ] )
		{
			facingAxis = i;
		}
	}


	


	
	//debris shower.
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_debris" );
	while ( kv )
	{
		const idDict *debris_args = gameLocal.FindEntityDefDict( kv->GetValue(), false );
		if ( debris_args )
		{
			
			idAngles dir = idAngles(0,0,0);

			if (facingAxis == 0)
			{
				dir.yaw = 0;
			}
			else if (facingAxis == 1)
			{
				dir.yaw = 90;				
			}
			else if (facingAxis == 2)
			{
				dir.pitch = 90;
			}

			

			for (i = 0; i < count; i++)
			{
				idVec3 pos;
				idDebris *debris;
				idEntity *ent;

				dir.roll = gameLocal.random.RandomInt(360);

				gameLocal.SpawnEntityDef( *debris_args, &ent, false );
				if ( !ent || !ent->IsType( idDebris::Type ) )
				{
					gameLocal.Error( "'projectile_debris' is not an idDebris" );
				}

				pos.x =  (gameLocal.random.CRandomFloat() * (tmax[0] - tmin[0])) / 2;
				pos.y =  (gameLocal.random.CRandomFloat() * (tmax[1] - tmin[1])) / 2;
				pos.z =  (gameLocal.random.CRandomFloat() * (tmax[2] - tmin[2])) / 2;
				pos += GetPhysics()->GetOrigin();
				
				debris = static_cast<idDebris *>(ent);
				debris->Create( this, pos, dir.ToMat3() );
				debris->Launch();
				//debris->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = ( gameLocal.time + 1500 ) * 0.001f;
				debris->UpdateVisuals();				
			}			
		}
		kv = spawnArgs.MatchPrefix( "def_debris", kv );
	}

	ActivateTargets( gameLocal.GetLocalPlayer() );

	
#ifdef STEAM
	g_SteamStats->SetStatDelta("stat_glassbreaks", 1);
#endif
}

