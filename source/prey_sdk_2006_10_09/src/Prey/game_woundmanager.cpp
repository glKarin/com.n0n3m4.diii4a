#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

// mdl:  Only steam this often to keep performance hits when hitting several pipes
#define STEAM_DELAY 1000.0f


hhWoundManagerRenderEntity::hhWoundManagerRenderEntity( const idEntity* self ) {
	this->self = self;
}

hhWoundManagerRenderEntity::~hhWoundManagerRenderEntity() {
}

void hhWoundManagerRenderEntity::DetermineWoundInfo( const trace_t& trace, const idVec3 velocity, jointHandle_t& jointNum, idVec3& origin, idVec3& normal, idVec3& dir ) {
	jointNum = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );

	origin =	trace.c.point;
	normal =	trace.c.normal;
	dir =		velocity;
	dir.Normalize();
}

void hhWoundManagerRenderEntity::AddWounds( const idDeclEntityDef *def, surfTypes_t matterType, jointHandle_t jointNum, const idVec3& origin, const idVec3& normal, const idVec3& dir ) {
	PROFILE_SCOPE("AddWounds", PROFMASK_COMBAT);
	if( !def ) {
		return;
	}

	// Do a check so we don't spawn too many steam effects at once
	if( matterType == SURFTYPE_PIPE ) {
		if ( gameLocal.time > gameLocal.GetSteamTime() ) {
			// Mark the current steam time
			gameLocal.SetSteamTime( gameLocal.time + STEAM_DELAY );
		} else {
			// Don't steam on this hit
			matterType = SURFTYPE_METAL;
		}
	}

	ApplyWound( def->dict, gameLocal.MatterTypeToMatterKey("fx_wound", matterType), jointNum, origin, normal, dir );	//smoke_wound conflicted with smoke
	
	const char* impactMarkKey = gameLocal.MatterTypeToMatterKey("mtr", matterType);
	ApplyMark( def->dict, impactMarkKey, origin, normal, dir );
	ApplySplatExit( def->dict, impactMarkKey, origin, normal, dir );
}

void hhWoundManagerRenderEntity::ApplyMark( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir ) {
	PROFILE_SCOPE("ApplyMark", PROFMASK_COMBAT);
	idList<idStr> strList;

	const idDict* decalDict = gameLocal.FindEntityDefDict( damageDict.GetString("def_entranceMark_decal"), false );
	if( !decalDict || !self->spawnArgs.GetBool("accepts_decals", "1") ) {
		return;
	}

	if( !impactMarkKey || !impactMarkKey[0] ) {
		return;
	}

	idStr impactMark = decalDict->RandomPrefix( impactMarkKey, gameLocal.random );
	if( !impactMark.Length() ) {
		return;
	}

	if( self->IsType(idBrittleFracture::Type) ) {
		static_cast<idBrittleFracture *>( self.GetEntity() )->ProjectDecal( origin, normal, gameLocal.GetTime(), NULL );
	} else {
		hhUtils::SplitString( impactMark, strList );

		//bjk: uses normal now
		float depth = 10.0f;

		for( int ix = strList.Num() - 1; ix >= 0; --ix ) {
			gameLocal.ProjectDecal( origin, -normal, depth, true, hhMath::Lerp(decalDict->GetVec2("mark_size"), gameLocal.random.RandomFloat()), strList[ix] );		//HUMANHEAD bjk: using normal
		}
	}

	//rww - if we were running this code on the server, we could do this:
	/*
	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];
	idVec2		markSize;

	msg.Init(msgBuf, sizeof(msgBuf));
	msg.BeginWriting();
	msg.WriteFloat(origin[0]);
	msg.WriteFloat(origin[1]);
	msg.WriteFloat(origin[2]);
	msg.WriteDir(normal, 24);
	msg.WriteDir(dir, 24);

	//AGH. rwwFIXME: configstring type system?
	msg.WriteString(impactMark);

	if (self->IsType(idBrittleFracture::Type))
	{
		msg.WriteBool(true);
		msg.WriteShort(self.GetEntity()->entityNumber);
	}
	else
	{
		msg.WriteBool(false);
		markSize = damageDict->GetVec2("mark_size");
		msg.WriteFloat(markSize[0]);
		msg.WriteFloat(markSize[1]);
	}

	self.GetEntity()->BroadcastEventReroute(idEntity::EVENT_PROJECT_DECAL, &msg);
	*/
}

void hhWoundManagerRenderEntity::ApplyWound( const idDict& damageDict, const char* woundKey, jointHandle_t jointNum,  const idVec3 &origin, const idVec3 &normal, const idVec3 &dir ) {
	PROFILE_SCOPE("ApplyWound", PROFMASK_COMBAT);

	if ( !g_bloodEffects.GetBool() ) {
		return;
	}
	
	if (! (self->fl.acceptsWounds && woundKey && *woundKey)) {
		return;
	}

	const idDict* woundDict = gameLocal.FindEntityDefDict( damageDict.GetString("def_entranceWound"), false );
	if( !woundDict ) {
		return;
	}

	const char* woundName = woundDict->RandomPrefix( woundKey, gameLocal.random );
	if( !woundName || !woundName[0] ) {
		return;
	}

	// for steam sound
	idStr sndKey = "snd_";
	sndKey += woundKey;
	const char *sndName = woundDict->GetString(sndKey.c_str());
	bool bHasSound = idStr::Length(sndName) > 0;

	/*	// Optimization: works but it could cause problems in some of our non-euclidean spaces if 
		// a portal destination is less than 1024 units away
	if (!bHasSound && !gameLocal.isMultiplayer && gameLocal.GetLocalPlayer()) {
		// Don't spawn purely visual wounds when behind player
		idVec3 playerOrigin = gameLocal.GetLocalPlayer()->GetOrigin();
		idMat3 playerView = gameLocal.GetLocalPlayer()->GetAxis();
		idVec3 toSpot = origin - playerOrigin;
		float dist = toSpot.Normalize();

		// Only do this optimization when within some distance to avoid problems with the effects being on the
		// other side of a portal
		if (dist < 1024.0f) {
			float dot = toSpot * playerView[0];

			// If spot is behind me and not right on the border, throw it out
			if (dot < 0.0f && dist > 64.0f) {
				return;
			}
		}
	}
	*/

	hhFxInfo fxInfo;
	fxInfo.SetNormal( normal );
	if( !self.GetEntity()->IsType(idStaticEntity::Type) ) {
		fxInfo.SetEntity( self.GetEntity() );
	}
	fxInfo.RemoveWhenDone( true );
	//correct for server: 	self->BroadcastFxInfo( woundName, origin, dir.ToMat3(), &fxInfo );
	//however this function should -only- be called on the client.
	if (gameLocal.isServer && !gameLocal.GetLocalPlayer()) {
		gameLocal.Error("hhWoundManagerRenderEntity::ApplyWound called on non-listen server!");
	}
	hhEntityFx *fx = self.GetEntity()->SpawnFxLocal( woundName, origin, normal.ToMat3(), &fxInfo, true );
	if (fx) {
		fx->fl.neverDormant = true;

		if (bHasSound) {
			const idSoundShader *shader = declManager->FindSound( sndName );
			fx->StartSoundShader(shader, SND_CHANNEL_ANY, 0, true);
		}
	}
}

void hhWoundManagerRenderEntity::ApplySplatExit( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir ) {	
	PROFILE_SCOPE("ApplySplatExit", PROFMASK_COMBAT);

	if ( !g_bloodEffects.GetBool() ) {
		return;
	}

	if( gameLocal.random.RandomFloat() > 0.8f ) {
		return;
	}
	
	const idDict *splatDict = gameLocal.FindEntityDefDict( damageDict.GetString("def_exitSplat"), false );
	if( !splatDict || !self->spawnArgs.GetBool("produces_splats") ) {
		return;
	}

	if( !impactMarkKey || !impactMarkKey[0] ) {
		return;
	}

	const char* decal = splatDict->RandomPrefix( impactMarkKey, gameLocal.random );
	if( !decal || !decal[0] ) {
		return;
	}

	trace_t	trace;
	if( gameLocal.clip.TracePoint(trace, origin, origin+dir*200.0f, MASK_SHOT_RENDERMODEL, self.GetEntity() ) ) {

		surfTypes_t matterType = gameLocal.GetMatterType( self.GetEntity(), trace.c.material, "ApplySplatExit" );
		
		gameLocal.ProjectDecal( trace.endpos,
								-trace.c.normal,
								splatDict->GetFloat("splat_dist", "10"),
								true,
								hhMath::Lerp(splatDict->GetVec2("mark_size"),
								gameLocal.random.RandomFloat()), decal );
	}
}

hhWoundManagerAnimatedEntity::hhWoundManagerAnimatedEntity( const idEntity* self ) : hhWoundManagerRenderEntity(self) {
}

hhWoundManagerAnimatedEntity::~hhWoundManagerAnimatedEntity() {
}

void hhWoundManagerAnimatedEntity::ApplyMark( const idDict& damageDict, const char* impactMarkKey, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir ) {
	idList<idStr> strList;

	const idDict* overlayDict = gameLocal.FindEntityDefDict( damageDict.GetString("def_entranceMark_overlay"), false );
	if( !overlayDict || !self->spawnArgs.GetBool("accepts_decals", "1") ) {
		return;
	}

	if( !impactMarkKey && !impactMarkKey[0] ) {
		return;
	}

	idStr impactMark = overlayDict->RandomPrefix( impactMarkKey, gameLocal.random );
	if( !impactMark.Length() ) {
		return;
	}

	hhUtils::SplitString( impactMark, strList );
	for( int ix = strList.Num() - 1; ix >= 0; --ix ) {
		self->ProjectOverlay( origin, dir, overlayDict->GetFloat("mark_size"), strList[ix] );
	}
}

void hhWoundManagerAnimatedEntity::ApplyWound( const idDict& damageDict, const char* woundKey, jointHandle_t jointNum, const idVec3 &origin, const idVec3 &normal, const idVec3 &dir ) {

	if (! (self->fl.acceptsWounds && woundKey && *woundKey && jointNum != INVALID_JOINT)) {
		return;
	}

	const idDict* woundDict = gameLocal.FindEntityDefDict( damageDict.GetString("def_entranceWound"), false );
	if( !woundDict ) {
		return;
	}

	const char* woundName = woundDict->RandomPrefix( woundKey, gameLocal.random );
	if( !woundName || !woundName[0] ) {
		return;
	}

#if !GOLD
	//correct for server: 	self->BroadcastFxInfo( woundName, origin, dir.ToMat3(), &fxInfo );
	//however this function should -only- be called on the client.
	if (gameLocal.isServer && !gameLocal.GetLocalPlayer()) {
		gameLocal.Error("hhWoundManagerRenderEntity::ApplyWound called on non-listen server!");
	}
#endif

#if 0
	hhFxInfo fxInfo;
	fxInfo.SetNormal( normal );
	fxInfo.SetEntity( self.GetEntity() );
	fxInfo.RemoveWhenDone( true );

	hhEntityFx* fx = self.GetEntity()->SpawnFxLocal( woundName, origin, normal.ToMat3(), &fxInfo, true );
	if (fx) {
		fx->fl.neverDormant = true;
		fx->BindToJoint( self.GetEntity(), jointNum, true);
	}
#else //rww - do this in some way that is less hideous and slow (specialcased here, cause this function gets a lot of use)
		//(avoids binding twice and some other needless logic)
	if ( !g_skipFX.GetBool() ) {
		idDict fxArgs;

		fxArgs.Set( "fx", woundName );
		fxArgs.SetBool( "start", true );
		fxArgs.SetBool( "removeWhenDone", true );
		fxArgs.SetVector( "origin", origin );
		fxArgs.SetMatrix( "rotation", normal.ToMat3() );

		hhEntityFx *fx = (hhEntityFx *)gameLocal.SpawnClientObject( "func_fx", &fxArgs );
		if (fx) {
			hhFxInfo fxInfo;
			fxInfo.SetNormal( normal );
			fxInfo.SetEntity( self.GetEntity() );
			fxInfo.RemoveWhenDone( true );
			fx->SetFxInfo( fxInfo );

			fx->fl.neverDormant = true;
			fx->BindToJoint( self.GetEntity(), jointNum, true );
			fx->Show();
		}
	}
#endif
}
