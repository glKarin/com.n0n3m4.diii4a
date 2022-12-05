// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../../game/Projectile.h"
#include "../ai/AI.h"

static const char *channelNames[ ANIM_NumAnimChannels ] = {
	"all", "torso", "legs", "head", "eyelids"
};

/***********************************************************************

	idAnim

***********************************************************************/

/*
=====================
idAnim::idAnim
=====================
*/
idAnim::idAnim() {
	modelDef = NULL;
// RAVEN BEGIN
// bdube: added speed
	rate = 1.0f;
// RAVEN END
	numAnims = 0;
	memset( anims, 0, sizeof( anims ) );
	memset( &flags, 0, sizeof( flags ) );
}

/*
=====================
idAnim::idAnim
=====================
*/
idAnim::idAnim( const idDeclModelDef *modelDef, const idAnim *anim ) {
	int i;

	this->modelDef = modelDef;
	numAnims = anim->numAnims;
	name = anim->name;
	realname = anim->realname;
	flags = anim->flags;
// RAVEN BEGIN
// bdube: copy speed info
	rate = anim->rate;
// RAVEN END	

	memset( anims, 0, sizeof( anims ) );
	for( i = 0; i < numAnims; i++ ) {
		anims[ i ] = anim->anims[ i ];
		anims[ i ]->IncreaseRefs();
	}

	frameLookup.SetNum( anim->frameLookup.Num() );
// RAVEN BEGIN
// JSinger: Changed to call optimized memcpy
	SIMDProcessor->Memcpy( frameLookup.Ptr(), anim->frameLookup.Ptr(), frameLookup.MemoryUsed() );
// RAVEN END

	frameCommands.SetNum( anim->frameCommands.Num() );
	for( i = 0; i < frameCommands.Num(); i++ ) {
		frameCommands[ i ] = anim->frameCommands[ i ];
		if ( anim->frameCommands[ i ].string ) {
			frameCommands[ i ].string = new idStr( *anim->frameCommands[ i ].string );
		}
// RAVEN BEGIN
// bdube: copy joint information
		if ( anim->frameCommands[ i ].joint ) {
			frameCommands[ i ].joint = new idStr ( *anim->frameCommands[i].joint );
		}
		if ( anim->frameCommands[ i ].joint2 ) {
			frameCommands[ i ].joint2 = new idStr ( *anim->frameCommands[i].joint2 );
		}
// abahr:
		if ( anim->frameCommands[ i ].parmList ) {
			frameCommands[ i ].parmList = new idList<idStr>( *anim->frameCommands[i].parmList );
		}
// RAVEN END
	}
}

/*
=====================
idAnim::~idAnim
=====================
*/
idAnim::~idAnim() {
	int i;

	for( i = 0; i < numAnims; i++ ) {
		anims[ i ]->DecreaseRefs();
	}

	for( i = 0; i < frameCommands.Num(); i++ ) {
		delete frameCommands[ i ].string;

// RAVEN BEGIN
// bdube: joint support
		delete frameCommands[i].joint;
		delete frameCommands[i].joint2;
// abahr:
		SAFE_DELETE_PTR( frameCommands[i].parmList );
// RAVEN END
	}
}

/*
=====================
idAnim::SetAnim
=====================
*/
void idAnim::SetAnim( const idDeclModelDef *modelDef, const char *sourcename, const char *animname, int num, const idMD5Anim *md5anims[ ANIM_MaxSyncedAnims ] ) {
	int i;

	this->modelDef = modelDef;

	for( i = 0; i < numAnims; i++ ) {
		anims[ i ]->DecreaseRefs();
		anims[ i ] = NULL;
	}

	assert( ( num > 0 ) && ( num <= ANIM_MaxSyncedAnims ) );
	numAnims	= num;
	realname	= sourcename;
	name		= animname;

	for( i = 0; i < num; i++ ) {
		anims[ i ] = md5anims[ i ];
		anims[ i ]->IncreaseRefs();
	}

	memset( &flags, 0, sizeof( flags ) );

	for( i = 0; i < frameCommands.Num(); i++ ) {
		delete frameCommands[ i ].string;

// RAVEN BEGIN
// bdube: joints as their own string
		delete frameCommands[i].joint;
		delete frameCommands[i].joint2;
// abahr:
		SAFE_DELETE_PTR( frameCommands[i].parmList );
// RAVEN END
	}

	frameLookup.Clear();
	frameCommands.Clear();
}

/*
=====================
idAnim::Name
=====================
*/
const char *idAnim::Name( void ) const {
	return name;
}

/*
=====================
idAnim::FullName
=====================
*/
const char *idAnim::FullName( void ) const {
	return realname;
}

/*
=====================
idAnim::MD5Anim

index 0 will never be NULL.  Any anim >= NumAnims will return NULL.
=====================
*/
const idMD5Anim *idAnim::MD5Anim( int num ) const {
	if ( anims == NULL || anims[0] == NULL ) { 
		return NULL;
	}
	return anims[ num ];
}

/*
=====================
idAnim::ModelDef
=====================
*/
const idDeclModelDef *idAnim::ModelDef( void ) const {
	return modelDef;
}

/*
=====================
idAnim::Length
=====================
*/
int idAnim::Length( void ) const {
	if ( !anims[ 0 ] ) {
		return 0;
	}

	return anims[ 0 ]->Length();
}

/*
=====================
idAnim::NumFrames
=====================
*/
int	idAnim::NumFrames( void ) const { 
	if ( !anims[ 0 ] ) {
		return 0;
	}
	
	return anims[ 0 ]->NumFrames();
}

/*
=====================
idAnim::NumAnims
=====================
*/
int	idAnim::NumAnims( void ) const { 
	return numAnims;
}

/*
=====================
idAnim::TotalMovementDelta
=====================
*/
const idVec3 &idAnim::TotalMovementDelta( void ) const {
	if ( !anims[ 0 ] ) {
		return vec3_zero;
	}
	
	return anims[ 0 ]->TotalMovementDelta();
}

/*
=====================
idAnim::GetOrigin
=====================
*/
bool idAnim::GetOrigin( idVec3 &offset, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		offset.Zero();
		return false;
	}

	anims[ animNum ]->GetOrigin( offset, currentTime, cyclecount );
	return true;
}

/*
=====================
idAnim::GetOriginRotation
=====================
*/
bool idAnim::GetOriginRotation( idQuat &rotation, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		rotation.Set( 0.0f, 0.0f, 0.0f, 1.0f );
		return false;
	}

	anims[ animNum ]->GetOriginRotation( rotation, currentTime, cyclecount );
	return true;
}

/*
=====================
idAnim::GetBounds
=====================
*/
ID_INLINE bool idAnim::GetBounds( idBounds &bounds, int animNum, int currentTime, int cyclecount ) const {
	if ( !anims[ animNum ] ) {
		return false;
	}

	anims[ animNum ]->GetBounds( bounds, currentTime, cyclecount );
	return true;
}

/*
=====================
idAnim::AddFrameCommand

Returns NULL if no error.
=====================
*/
const char *idAnim::AddFrameCommand( const idDeclModelDef *modelDef, const idList<int>& frames, idLexer &src, const idDict *def ) {
	int					i;
	int					index;
	idStr				text;
	idStr				funcname;
	frameCommand_t		fc;
	idToken				token;

	memset( &fc, 0, sizeof( fc ) );

	if( !src.ReadTokenOnLine( &token ) ) {
		return "Unexpected end of line";
	}

	if ( token == "call" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTION;
		fc.function = gameLocal.program.FindFunction( token );
		if ( !fc.function ) {
			return va( "Function '%s' not found", token.c_str() );
		}
	} else if ( token == "object_call" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SCRIPTFUNCTIONOBJECT;
		fc.string = new idStr( token );
	} else if ( token == "event" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_EVENTFUNCTION;
		const idEventDef *ev = idEventDef::FindEvent( token );
		if ( !ev ) {
			return va( "Event '%s' not found", token.c_str() );
		}
		if ( ev->GetNumArgs() != 0 ) {
			return va( "Event '%s' has arguments", token.c_str() );
		}
		fc.string = new idStr( token );
	}
// RAVEN BEGIN
// abahr:
	else if( token == "eventArgs" ) {
		src.ParseRestOfLine( token );
		if( token.Length() <= 0 ) {
			return "Unexpected end of line";
		}

		fc.type = FC_EVENTFUNCTION_ARGS;
		fc.parmList = new idList<idStr>();
		token.Split( *fc.parmList, ' ' );
		fc.event = idEventDef::FindEvent( (*fc.parmList)[0] );
		if( !fc.event ) {
			SAFE_DELETE_PTR( fc.parmList );
			return va( "Event '%s' not found", (*fc.parmList)[0].c_str() );
		}
		
		fc.parmList->RemoveIndex( 0 );
	}
// RAVEN END
	else if ( token == "sound" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_voice2" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_VOICE2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body2" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY2;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_body3" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_BODY3;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_weapon" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_WEAPON;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_global" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_GLOBAL;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_item" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_ITEM;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "sound_chatter" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SOUND_CHATTER;
		if ( !token.Cmpn( "snd_", 4 ) ) {
			fc.string = new idStr( token );
		} else {
			fc.soundShader = declManager->FindSound( token );
			if ( fc.soundShader->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "Sound '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "skin" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_SKIN;
		if ( token == "none" ) {
			fc.skin = NULL;
		} else {
			fc.skin = declManager->FindSkin( token );
			if ( !fc.skin ) {
				return va( "Skin '%s' not found", token.c_str() );
			}
		}
	} else if ( token == "fx" ) {
// RAVEN BEGIN
// bdube: use Raven effect system
		fc.type = FC_FX;		

		// Get the effect name
		if ( !src.ReadTokenOnLine( &token ) ) {
			return va( "missing effect name" );
		}
		
		// Effect is indirect if it starts with fx_
		if ( !idStr::Icmpn ( token, "fx_", 3 ) ) {
			fc.string = new idStr ( token );
		} else {
			fc.effect = ( const idDecl * )declManager->FindEffect( token );
		}
		
		// Joint specified?
		if ( src.ReadTokenOnLine ( &token ) ) {
			fc.joint = new idStr ( token );
		}
		
		// End joint specified?
		if ( src.ReadTokenOnLine ( &token ) ) {
			fc.joint2 = new idStr ( token );
		}
// RAVEN END
	} else if ( token == "trigger" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER;
		fc.string = new idStr( token );
// RAVEN BEGIN
// bdube: not using
/*
	} else if ( token == "triggerSmokeParticle" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_TRIGGER_SMOKE_PARTICLE;
		fc.string = new idStr( token );
*/
// RAVEN END
	} else if ( token == "direct_damage" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DIRECTDAMAGE;
		if ( !gameLocal.FindEntityDef( token.c_str(), false ) ) {
			return va( "Unknown entityDef '%s'", token.c_str() );
		}
		fc.string = new idStr( token );
	} else if ( token == "muzzle_flash" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		if ( ( token != "" ) && !modelDef->FindJoint( token ) ) {
			return va( "Joint '%s' not found", token.c_str() );
		}
		fc.type = FC_MUZZLEFLASH;
		fc.string = new idStr( token );
	} else if ( token == "muzzle_flash" ) {
		fc.type = FC_MUZZLEFLASH;
		fc.string = new idStr( "" );
	} else if ( token == "footstep" ) {
		fc.type = FC_FOOTSTEP;
	} else if ( token == "leftfoot" ) {
		fc.type = FC_LEFTFOOT;
	} else if ( token == "rightfoot" ) {
		fc.type = FC_RIGHTFOOT;
	} else if ( token == "enableEyeFocus" ) {
		fc.type = FC_ENABLE_EYE_FOCUS;
	} else if ( token == "disableEyeFocus" ) {
		fc.type = FC_DISABLE_EYE_FOCUS;
	} else if ( token == "enableBlinking" ) {
		fc.type = FC_ENABLE_BLINKING;
	} else if ( token == "disableBlinking" ) {
		fc.type = FC_DISABLE_BLINKING;
	} else if ( token == "enableAutoBlink" ) {
		fc.type = FC_ENABLE_AUTOBLINK;
	} else if ( token == "disableAutoBlink" ) {
		fc.type = FC_DISABLE_AUTOBLINK;
	} else if ( token == "disableGravity" ) {
		fc.type = FC_DISABLE_GRAVITY;
	} else if ( token == "enableGravity" ) {
		fc.type = FC_ENABLE_GRAVITY;
	} else if ( token == "jump" ) {
		fc.type = FC_JUMP;
	} else if ( token == "enableClip" ) {
		fc.type = FC_ENABLE_CLIP;
	} else if ( token == "disableClip" ) {
		fc.type = FC_DISABLE_CLIP;
	} else if ( token == "enableWalkIK" ) {
		fc.type = FC_ENABLE_WALK_IK;
	} else if ( token == "disableWalkIK" ) {
		fc.type = FC_DISABLE_WALK_IK;
	} else if ( token == "enableLegIK" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_ENABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "disableLegIK" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line";
		}
		fc.type = FC_DISABLE_LEG_IK;
		fc.index = atoi( token );
	} else if ( token == "recordDemo" ) {
		fc.type = FC_RECORDDEMO;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
	} else if ( token == "aviGame" ) {
		fc.type = FC_AVIGAME;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
// RAVEN BEGIN
// bdube: added script commands
	} else if ( token == "ai_enablePain" ) {
		fc.type = FC_AI_ENABLE_PAIN;
	} else if ( token == "ai_disablePain" ) {
		fc.type = FC_AI_DISABLE_PAIN;
	} else if ( token == "ai_enableDamage" ) {
		fc.type = FC_AI_ENABLE_DAMAGE;
	} else if ( token == "ai_disableDamage" ) {
		fc.type = FC_AI_DISABLE_DAMAGE;
	} else if ( token == "ai_lockEnemyOrigin" ) {
		fc.type = FC_AI_LOCKENEMYORIGIN;
	} else if ( token == "ai_attack" ) {
		fc.type = FC_AI_ATTACK;

		// Name of attack
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for attack Name";
		}
		fc.string = new idStr( token );

		// Joint to attack from
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for attack joint";
		}
		fc.joint = new idStr( token );		
	} else if ( token == "ai_attack_melee" ) {
		if( !src.ReadTokenOnLine( &token ) ) {
			return "Unexpected end of line while looking for melee attack name";
		}
		fc.type = FC_AI_ATTACK_MELEE;
		fc.string = new idStr( token );
	} else if ( token == "guievent" )  {
		fc.type = FC_GUIEVENT;
		if( src.ReadTokenOnLine( &token ) ) 
		{
			fc.string = new idStr( token );
		}
	} else if ( token == "speak" )  {
		fc.type = FC_AI_SPEAK;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
	} else if ( token == "speak_random" )  {
		fc.type = FC_AI_SPEAK_RANDOM;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
//MCG - added attachment frame commands
	} else if ( token == "attachment_hide" )  {
		fc.type = FC_ACT_ATTACH_HIDE;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
	} else if ( token == "attachment_show" )  {
		fc.type = FC_ACT_ATTACH_SHOW;
		if( src.ReadTokenOnLine( &token ) ) {
			fc.string = new idStr( token );
		}
// RAVEN END
	} else {
		return va( "Unknown command '%s'", token.c_str() );
	}

	// check if we've initialized the frame loopup table
	if ( !frameLookup.Num() ) {
		// we haven't, so allocate the table and initialize it
		frameLookup.SetGranularity( 1 );
		frameLookup.SetNum( anims[ 0 ]->NumFrames() );
		for( i = 0; i < frameLookup.Num(); i++ ) {
			frameLookup[ i ].num = 0;
			frameLookup[ i ].firstCommand = 0;
		}
	}

// RAVEN BEGIN
// bdube: support multiple frames
	for ( int ii = 0; ii < frames.Num(); ii ++ ) {
		int framenum = frames[ii];	

// mekberg: error out of frame command is out of range.
//			-1 because we don't want commands on the loop frame.
//			If the anim doesn't loop they won't get handled.
		if ( ( framenum < 1 ) || ( framenum > anims[ 0 ]->NumFrames() -1 ) ) {
			gameLocal.Error("Frame command out of range: %d on  anim '%s'. Max %d.", framenum, anims[ 0 ]->Name(), anims[ 0 ]->NumFrames() -1 );
		}

		// Duplicate the frame info
		if ( ii != 0 ) {
			if ( fc.string ) {
				fc.string = new idStr ( fc.string->c_str() );
			}
			if ( fc.joint ) {
				fc.joint = new idStr ( fc.joint->c_str() );
			}				
			if ( fc.joint2 ) {
				fc.joint2 = new idStr ( fc.joint2->c_str() );
			}
			if ( fc.parmList ) {
				fc.parmList = new idList<idStr>( *fc.parmList );
			}
		}

		// frame numbers are 1 based in .def files, but 0 based internally
		framenum--;
// RAVEN END

	// allocate space for a new command
	frameCommands.Alloc();

	// calculate the index of the new command
	index = frameLookup[ framenum ].firstCommand + frameLookup[ framenum ].num;

	// move all commands from our index onward up one to give us space for our new command
	for( i = frameCommands.Num() - 1; i > index; i-- ) {
		frameCommands[ i ] = frameCommands[ i - 1 ];
	}

	// fix the indices of any later frames to account for the inserted command
	for( i = framenum + 1; i < frameLookup.Num(); i++ ) {
		frameLookup[ i ].firstCommand++;
	}

	// store the new command 
	frameCommands[ index ] = fc;

	// increase the number of commands on this frame
	frameLookup[ framenum ].num++;

// RAVEN BEGIN
// bdube: loop frame commands
	}
// RAVEN END

	// return with no error
	return NULL;
}

// RAVEN BEGIN
// bdube: added for debugging
struct frameCommandInfo_t frameCommandInfo[FC_COUNT] = {
	{ "call",					false	},
	{ "object_call",			false	},
	{ "event",					false	},
	{ "eventArgs",				false	},
	
	{ "sound",					true	},
	{ "sound_voice",			true	},
	{ "sound_voice2",			true	},
	{ "sound_body",				true	},		
	{ "sound_body2",			true	},		
	{ "sound_body3",			true	},		
	{ "sound_weapon",			true	},
	{ "sound_item",				true	},		
	{ "sound_global",			true	},		
	{ "sound_chatter",			true	},
	
	{ "skin",					true	},				
	{ "trigger",				false	},	
	{ "direct_damage",			false	},
	{ "muzzle_flash",			false	},		
	{ "footstep",				false	},			
	{ "leftfoot",				false	},			
	{ "rightfoot",				false	},		
	{ "enableEyeFocus",			false	},	
	{ "disableEyeFocus",		false	},	
	{ "effect",					true	},	
	{ "disable_gravity",		false	},
	{ "enable_gravity",			false	},
	{ "jump",					false	},				
	{ "enableClip",				false	},		
	{ "disableClip",			false	},	
	{ "enableWalkIK",			false   },
	{ "disableWalkIK",			false	},
	{ "enableLegIK",			false	},
	{ "disableLegID",			false	},
	{ "recordDemo",				false	},
	{ "aviGame",				false	},	
	{ "guievent",				false	},
	
	{ "ai_enablePain",			false	},
	{ "ai_disablePain",			false   },
	{ "ai_enableDamage",		false	},
	{ "ai_disableDamage",		false   },
	{ "ai_lockEnemyOrigin",		false	},
	{ "ai_attack",				false	},
	{ "ai_attack_melee",		false	},
	{ "speak",					true	},
};
// RAVEN END

// RAVEN BEGIN
// bdube: added frame command methods
/*
=====================
idAnim::CallFrameCommandSound
=====================
*/
void idAnim::CallFrameCommandSound ( const frameCommand_t& command, idEntity* ent, const s_channelType channel ) const {

	int flags = 0;
	if( channel == ( FC_SOUND_GLOBAL - FC_SOUND ) ) {
		flags = SSF_PRIVATE_SOUND;
	}
	
	if ( command.string ) {
		ent->StartSound ( command.string->c_str(), channel, flags, false, NULL );
	} else {
		ent->StartSoundShader( command.soundShader, channel, flags, false, NULL );
	}
}
// RAVEN END

/*
=====================
idAnim::CallFrameCommands
=====================
*/
void idAnim::CallFrameCommands( idEntity *ent, int from, int to ) const {
	int index;
	int end;
	int frame;
	int numframes;

	numframes = anims[ 0 ]->NumFrames();

	frame = from;
	while( frame != to ) {
		frame++;
		if ( frame >= numframes ) {
			frame = 0;
		}

		index = frameLookup[ frame ].firstCommand;
		end = index + frameLookup[ frame ].num;
		while( index < end ) {
			const frameCommand_t &command = frameCommands[ index++ ];

// RAVEN BEGIN
// bdube: frame command debugging			
			if ( g_showFrameCmds.GetBool() ) {
				idStr shortName;
				shortName = name;
				shortName.StripPath();
				shortName.StripFileExtension ( );
				gameLocal.Printf ( "framecmd: anim=%s frame=%d cmd=%s parm=%s\n",
							 shortName.c_str(),
							 frame + 1,
							 frameCommandInfo[command.type].name,
							 command.string?command.string->c_str():"???" );
			}

			if ( ( gameLocal.editors & EDITOR_MODVIEW ) && !frameCommandInfo[command.type].modview ) {
				continue;
			}
// RAVEN END			

			switch( command.type ) {
				case FC_SCRIPTFUNCTION: {
					gameLocal.CallFrameCommand( ent, command.function );
					break;
				}
// RAVEN BEGIN
// bdube: rewrote
				case FC_SCRIPTFUNCTIONOBJECT: {
					ent->ProcessEvent ( &EV_CallFunction, command.string->c_str() );
					break;
				}
// RAVEN END				
				case FC_EVENTFUNCTION: {
					const idEventDef *ev = idEventDef::FindEvent( command.string->c_str() );
					ent->ProcessEvent( ev );
					break;
				}
// RAVEN BEGIN
// abahr:
				case FC_EVENTFUNCTION_ARGS: {
					assert( command.event );
					ent->ProcessEvent( command.event, (int)command.parmList );
					break;
				}
// bdube: support indirection and simplify
				case FC_SOUND:
				case FC_SOUND_VOICE:
				case FC_SOUND_VOICE2:
				case FC_SOUND_BODY:
				case FC_SOUND_BODY2:
				case FC_SOUND_BODY3:
				case FC_SOUND_WEAPON:
				case FC_SOUND_ITEM:
				case FC_SOUND_GLOBAL:
				case FC_SOUND_CHATTER:
					CallFrameCommandSound ( command, ent, (const s_channelType)(command.type - FC_SOUND) );
					break;
// RAVEN END

				case FC_FX: {

					if ( gameLocal.localClientNum == -1 ) {
						// early ret on dedicated server
						break;
					}
// RAVEN BEGIN
// bdube: use raven effect system
					rvClientEffect* cent;
					if ( command.string ) {
						if ( command.joint ) {
							cent = ent->PlayEffect( command.string->c_str(), ent->GetAnimator()->GetJointHandle ( *command.joint ) );
						} else {
							cent = gameLocal.PlayEffect( ent->spawnArgs, command.string->c_str(), ent->GetRenderEntity()->origin, ent->GetRenderEntity()->axis );
						}
					} else {
						if ( command.joint ) {
							cent = ent->PlayEffect( command.effect, ent->GetAnimator()->GetJointHandle ( *command.joint ), vec3_zero, mat3_identity );
						} else {
							cent = gameLocal.PlayEffect(  command.effect, ent->GetRenderEntity()->origin, ent->GetRenderEntity()->axis );
						}
					}
					// End origin bone specified?
					if ( cent && command.joint2 && ent->IsType( idAnimatedEntity::GetClassType() ) ) {
						cent->SetEndOrigin( ent->GetAnimator()->GetJointHandle( *command.joint2 ) );
					}

					// Error print should the effect fail to play
					if ( !cent ) {
						idStr error = "Failed to play effect";

						if( command.string ) {
							error += va(  " \'%s\'", command.string->c_str() );
						}
						if ( command.effect ) {
							error += va( " \'%s\'", command.effect->GetName() );
						}
						if( command.joint ) {
							error += va( " on bone \'%s\'", command.joint->c_str() );
						}
						common->Warning( error.c_str() );
					}
// RAVEN END
					break;
				}
				case FC_SKIN:
					ent->SetSkin( command.skin );
					break;

				case FC_TRIGGER: {
					idEntity *target;

					target = gameLocal.FindEntity( command.string->c_str() );
					if ( target ) {
						target->Signal( SIG_TRIGGER );
						target->ProcessEvent( &EV_Activate, ent );
						target->TriggerGuis();
					} else {
						gameLocal.Warning( "Framecommand 'trigger' on entity '%s', anim '%s', frame %d: Could not find entity '%s'",
							ent->name.c_str(), FullName(), frame + 1, command.string->c_str() );
					}
					break;
				}

				case FC_DIRECTDAMAGE: {
					ent->ProcessEvent( &AI_DirectDamage, command.string->c_str() );
					break;
				}
				case FC_MUZZLEFLASH: {
// RAVEN BEGIN
// nmckenzie: We're not using this.
//					ent->ProcessEvent( &AI_MuzzleFlash, command.string->c_str() );
// RAVEN END
					break;
				}
				case FC_FOOTSTEP : {
					ent->ProcessEvent( &EV_Footstep );
					break;
				}
				case FC_LEFTFOOT: {
					ent->ProcessEvent( &EV_FootstepLeft );
					break;
				}
				case FC_RIGHTFOOT: {
					ent->ProcessEvent( &EV_FootstepRight );
					break;
				}
				case FC_ENABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_EnableEyeFocus );
					break;
				}
				case FC_DISABLE_EYE_FOCUS: {
					ent->ProcessEvent( &AI_DisableEyeFocus );
					break;
				}
				case FC_ENABLE_BLINKING: {
					ent->ProcessEvent( &AI_EnableBlink );
					break;
				}
				case FC_DISABLE_BLINKING: {
					ent->ProcessEvent( &AI_DisableBlink );
					break;
				}
				case FC_ENABLE_AUTOBLINK: {
					ent->ProcessEvent( &AI_EnableAutoBlink );
					break;
				}
				case FC_DISABLE_AUTOBLINK: {
					ent->ProcessEvent( &AI_DisableAutoBlink );
					break;
				}
				case FC_DISABLE_GRAVITY: {
					ent->ProcessEvent( &AI_DisableGravity );
					break;
				}
				case FC_ENABLE_GRAVITY: {
					ent->ProcessEvent( &AI_EnableGravity );
					break;
				}
				case FC_JUMP: {
					ent->ProcessEvent( &AI_JumpFrame );
					break;
				}
				case FC_ENABLE_CLIP: {
					ent->ProcessEvent( &AI_EnableClip );
					break;
				}
				case FC_DISABLE_CLIP: {
					ent->ProcessEvent( &AI_DisableClip );
					break;
				}
				case FC_ENABLE_WALK_IK: {
					ent->ProcessEvent( &EV_EnableWalkIK );
					break;
				}
				case FC_DISABLE_WALK_IK: {
					ent->ProcessEvent( &EV_DisableWalkIK );
					break;
				}
				case FC_ENABLE_LEG_IK: {
					ent->ProcessEvent( &EV_EnableLegIK, command.index );
					break;
				}
				case FC_DISABLE_LEG_IK: {
					ent->ProcessEvent( &EV_DisableLegIK, command.index );
					break;
				}
				case FC_RECORDDEMO: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "recordDemo %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "stoprecording" );
					}
					break;
				}
				case FC_AVIGAME: {
					if ( command.string ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "aviGame %s", command.string->c_str() ) );
					} else {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "aviGame" );
					}
					break;
				}

				case FC_AI_ENABLE_PAIN:
					ent->ProcessEvent ( &AI_EnablePain );				
					break;

				case FC_AI_DISABLE_PAIN:
					ent->ProcessEvent ( &AI_DisablePain );
					break;

				case FC_AI_ENABLE_DAMAGE:
					ent->ProcessEvent ( &AI_EnableDamage );				
					break;

				case FC_AI_LOCKENEMYORIGIN:
					ent->ProcessEvent ( &AI_LockEnemyOrigin );
					break;
					
				case FC_AI_ATTACK:					
					ent->ProcessEvent ( &AI_Attack, command.string->c_str(), command.joint->c_str() );
					break;

				case FC_AI_DISABLE_DAMAGE:
					ent->ProcessEvent ( &AI_DisableDamage );
					break;
					
				case FC_AI_SPEAK:
					ent->ProcessEvent( &AI_Speak, command.string->c_str() );
					break;

				case FC_AI_SPEAK_RANDOM:
					ent->ProcessEvent( &AI_SpeakRandom, command.string->c_str() );
					break;

				case FC_ACT_ATTACH_HIDE:
					if ( ent->IsType(idActor::GetClassType()) )
					{
						static_cast<idActor*>(ent)->HideAttachment( command.string->c_str() );
					}
					break;

				case FC_ACT_ATTACH_SHOW:
					if ( ent->IsType(idActor::GetClassType()) )
					{
						static_cast<idActor*>(ent)->ShowAttachment( command.string->c_str() );
					}
					break;

				case FC_AI_ATTACK_MELEE:
					ent->ProcessEvent( &AI_AttackMelee, command.string->c_str() );
					break;
			}
		}
	}
}

/*
=====================
idAnim::FindFrameForFrameCommand
=====================
*/
int	idAnim::FindFrameForFrameCommand( frameCommandType_t framecommand, const frameCommand_t **command ) const {
	int frame;
	int index;
	int numframes;
	int end;

	if ( !frameCommands.Num() ) {
		return -1;
	}

	numframes = anims[ 0 ]->NumFrames();
	for( frame = 0; frame < numframes; frame++ ) {
		end = frameLookup[ frame ].firstCommand + frameLookup[ frame ].num;
		for( index = frameLookup[ frame ].firstCommand; index < end; index++ ) {
			if ( frameCommands[ index ].type == framecommand ) {
				if ( command ) {
					*command = &frameCommands[ index ];
				}
				return frame;
			}
		}
	}

	if ( command ) {
		*command = NULL;
	}

	return -1;
}

/*
=====================
idAnim::HasFrameCommands
=====================
*/
bool idAnim::HasFrameCommands( void ) const {
	if ( !frameCommands.Num() ) {
		return false;
	}
	return true;
}

/*
=====================
idAnim::SetAnimFlags
=====================
*/
void idAnim::SetAnimFlags( const animFlags_t &animflags ) {
	flags = animflags;
}

/*
=====================
idAnim::GetAnimFlags
=====================
*/
const animFlags_t &idAnim::GetAnimFlags( void ) const {
	return flags;
}

/***********************************************************************

	idAnimBlend

***********************************************************************/

/*
=====================
idAnimBlend::idAnimBlend
=====================
*/
idAnimBlend::idAnimBlend( void ) {
	Reset( NULL );
}

/*
=====================
idAnimBlend::Save

archives object for save game file
=====================
*/
void idAnimBlend::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( starttime );
	savefile->WriteInt( endtime );
	savefile->WriteInt( timeOffset );
	savefile->WriteFloat( rate );

	savefile->WriteInt( blendStartTime );
	savefile->WriteInt( blendDuration );
	savefile->WriteFloat( blendStartValue );
	savefile->WriteFloat( blendEndValue );

	for( i = 0; i < ANIM_MaxSyncedAnims; i++ ) {
		savefile->WriteFloat( animWeights[ i ] );
	}
	savefile->WriteShort( cycle );
	savefile->WriteShort( animNum );
	savefile->WriteBool( allowMove );
	savefile->WriteBool( allowFrameCommands );
	savefile->WriteBool( useFrameBlend );
}

/*
=====================
idAnimBlend::Restore

unarchives object from save game file
=====================
*/
void idAnimBlend::Restore( idRestoreGame *savefile, const idDeclModelDef *modelDef ) {
	int	i;

	this->modelDef = modelDef;

	savefile->ReadInt( starttime );
	savefile->ReadInt( endtime );
	savefile->ReadInt( timeOffset );
	savefile->ReadFloat( rate );

	savefile->ReadInt( blendStartTime );
	savefile->ReadInt( blendDuration );
	savefile->ReadFloat( blendStartValue );
	savefile->ReadFloat( blendEndValue );

	for( i = 0; i < ANIM_MaxSyncedAnims; i++ ) {
		savefile->ReadFloat( animWeights[ i ] );
	}
	savefile->ReadShort( cycle );
	savefile->ReadShort( animNum );
	if ( !modelDef ) {
		animNum = 0;
	} else if ( ( animNum < 0 ) || ( animNum > modelDef->NumAnims() ) ) {
		gameLocal.Warning( "Anim number %d out of range for model '%s' during save game", animNum, modelDef->GetModelName() );
		animNum = 0;
	}
	savefile->ReadBool( allowMove );
	savefile->ReadBool( allowFrameCommands );
	savefile->ReadBool( useFrameBlend );
}

/*
=====================
idAnimBlend::Reset
=====================
*/
void idAnimBlend::Reset( const idDeclModelDef *_modelDef ) {
	modelDef	= _modelDef;
	cycle		= 1;
	starttime	= 0;
	endtime		= 0;
	timeOffset	= 0;
	rate		= 1.0f;
	allowMove	= true;
	allowFrameCommands = true;
	animNum		= 0;

	memset( animWeights, 0, sizeof( animWeights ) );

	blendStartValue = 0.0f;
	blendEndValue	= 0.0f;
    blendStartTime	= 0;
	blendDuration	= 0;
	useFrameBlend = false;

	memset( &frameBlend, 0, sizeof( frameBlend ) );
}

/*
=====================
idAnimBlend::FullName
=====================
*/
const char *idAnimBlend::AnimFullName( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->FullName();
}

/*
=====================
idAnimBlend::AnimName
=====================
*/
const char *idAnimBlend::AnimName( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return "";
	}

	return anim->Name();
}

/*
=====================
idAnimBlend::NumFrames
=====================
*/
int idAnimBlend::NumFrames( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumFrames();
}

/*
=====================
idAnimBlend::Length
=====================
*/
int	idAnimBlend::Length( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->Length();
}

/*
=====================
idAnimBlend::GetWeight
=====================
*/
float idAnimBlend::GetWeight( int currentTime ) const {
	int		timeDelta;
	float	frac;
	float	w;

	timeDelta = currentTime - blendStartTime;
	if ( timeDelta <= 0 ) {
		w = blendStartValue;
	} else if ( timeDelta >= blendDuration ) {
		w = blendEndValue;
	} else {
		frac = ( float )timeDelta / ( float )blendDuration;
		w = blendStartValue + ( blendEndValue - blendStartValue ) * frac;
	}

	return w;
}

/*
=====================
idAnimBlend::GetFinalWeight
=====================
*/
float idAnimBlend::GetFinalWeight( void ) const {
	return blendEndValue;
}

/*
=====================
idAnimBlend::SetWeight
=====================
*/
void idAnimBlend::SetWeight( float newweight, int currentTime, int blendTime ) {
	blendStartValue = GetWeight( currentTime );
	blendEndValue = newweight;
    blendStartTime = currentTime - 1;
	blendDuration = blendTime;

	if ( !newweight ) {
		endtime = currentTime + blendTime;
	}
}

/*
=====================
idAnimBlend::NumSyncedAnims
=====================
*/
int idAnimBlend::NumSyncedAnims( void ) const {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return 0;
	}

	return anim->NumAnims();
}

/*
=====================
idAnimBlend::SetSyncedAnimWeight
=====================
*/
bool idAnimBlend::SetSyncedAnimWeight( int num, float weight ) {
	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	if ( ( num < 0 ) || ( num > anim->NumAnims() ) ) {
		return false;
	}

	animWeights[ num ] = weight;
	return true;
}

/*
=====================
idAnimBlend::SetFrame
=====================
*/
void idAnimBlend::SetFrame( const idDeclModelDef *modelDef, int _animNum, const frameBlend_t & _frameBlend ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
// RAVEN BEGIN
// scork: reports the actual joint # mismatch as well
		gameLocal.Warning( "Model '%s' has different # of joints (%d) than anim '%s' (%d)", modelDef->GetModelName(), modelDef->Joints().Num(), md5anim->Name(), md5anim->NumJoints() );
// RAVEN END
		return;
	}

	frameBlend = _frameBlend;
	animNum = _animNum;
	useFrameBlend = true;

	// a frame of 0 means it's not a single frame blend, so we set it to frame + 1
	if ( frameBlend.frame1 < 0 ) {
		frameBlend.frame1 = 0;
	} else if ( frameBlend.frame1 >= _anim->NumFrames() ) {
		frameBlend.frame1 = _anim->NumFrames() - 1;
	}

	if ( frameBlend.frame2 < 0 ) {
		frameBlend.frame2 = 0;
	} else if ( frameBlend.frame2 >= _anim->NumFrames() ) {
		frameBlend.frame2 = _anim->NumFrames() - 1;
	}
}

/*
=====================
idAnimBlend::CycleAnim
=====================
*/
void idAnimBlend::CycleAnim( const idDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime, float _rate ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
// RAVEN BEGIN
// scork: reports the actual joint # mismatch as well
		gameLocal.Warning( "Model '%s' has different # of joints (%d) than anim '%s' (%d)", modelDef->GetModelName(), modelDef->Joints().Num(), md5anim->Name(), md5anim->NumJoints() );
// RAVEN END
		return;
	}

	animNum				= _animNum;
	animWeights[ 0 ]	= 1.0f;
	endtime				= -1;
	cycle				= -1;
	if ( _anim->GetAnimFlags().random_cycle_start ) {
		// start the animation at a random time so that characters don't walk in sync
		starttime = currentTime - gameLocal.random.RandomFloat() * _anim->Length();
	} else {
		starttime = currentTime;
	}

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;
	
// RAVEN BEGIN
// bdube: configurable playback rate
	_rate *= _anim->GetPlaybackRate ( );
	if ( _rate != 1.0f ) {
		SetPlaybackRate ( currentTime, _rate );
	}			
// RAVEN END
}

/*
=====================
idAnimBlend::PlayAnim
=====================
*/
void idAnimBlend::PlayAnim( const idDeclModelDef *modelDef, int _animNum, int currentTime, int blendTime, float _rate ) {
	Reset( modelDef );
	if ( !modelDef ) {
		return;
	}
	
	const idAnim *_anim = modelDef->GetAnim( _animNum );
	if ( !_anim ) {
		return;
	}

	const idMD5Anim *md5anim = _anim->MD5Anim( 0 );
	if ( modelDef->Joints().Num() != md5anim->NumJoints() ) {
// RAVEN BEGIN
// scork: reports the actual joint # mismatch as well
		gameLocal.Warning( "Model '%s' has different # of joints (%d) than anim '%s' (%d)", modelDef->GetModelName(), modelDef->Joints().Num(), md5anim->Name(), md5anim->NumJoints() );
// RAVEN END
		return;
	}

	animNum				= _animNum;
	starttime			= currentTime;
	endtime				= starttime + _anim->Length();
	cycle				= 1;
	animWeights[ 0 ]	= 1.0f;

	// set up blend
	blendEndValue		= 1.0f;
	blendStartTime		= currentTime - 1;
	blendDuration		= blendTime;
	blendStartValue		= 0.0f;

// RAVEN BEGIN
// bdube: configurable playback rate
	
	_rate *= _anim->GetPlaybackRate ( );
	if ( _rate != 1.0f ) {
		SetPlaybackRate ( currentTime, _rate );
	}

// RAVEN END
}

/*
=====================
idAnimBlend::Clear
=====================
*/
void idAnimBlend::Clear( int currentTime, int clearTime ) {
	if ( !clearTime ) {
		Reset( modelDef );
	} else {
		SetWeight( 0.0f, currentTime, clearTime );
	}
}

/*
=====================
idAnimBlend::IsDone
=====================
*/
bool idAnimBlend::IsDone( int currentTime ) const {
	if ( !useFrameBlend && ( endtime > 0 ) && ( currentTime >= endtime ) ) {
		return true;
	}

	if ( ( blendEndValue <= 0.0f ) && ( currentTime >= ( blendStartTime + blendDuration ) ) ) {
		return true;
	}

	return false;
}

/*
=====================
idAnimBlend::FrameHasChanged
=====================
*/
bool idAnimBlend::FrameHasChanged( int currentTime ) const {
	// if we don't have an anim, no change
	if ( !animNum ) {
		return false;
	}

	// if anim is done playing, no change
	if ( ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	// if our blend weight changes, we need to update
	if ( ( currentTime < ( blendStartTime + blendDuration ) && ( blendStartValue != blendEndValue ) ) ) {
		return true;
	}

	// if we're a single frame anim and this isn't the frame we started on, we don't need to update
	if ( ( useFrameBlend || ( NumFrames() == 1 ) ) && ( currentTime != starttime ) ) {
		return false;
	}

	return true;
}

/*
=====================
idAnimBlend::GetCycleCount
=====================
*/
int idAnimBlend::GetCycleCount( void ) const {
	return cycle;
}

/*
=====================
idAnimBlend::SetCycleCount
=====================
*/
void idAnimBlend::SetCycleCount( int count ) {
	const idAnim *anim = Anim();

	if ( !anim ) {
		cycle = -1;
		endtime = 0;
	} else {
		cycle = count;
// RAVEN BEGIN
// jnewquist: Xenon compiler bug generated bad code.  Used count instead of cycle for test.
		if ( count < 0 ) {
// RAVEN END
			cycle = -1;
			endtime	= -1;
// RAVEN BEGIN
// jnewquist: Xenon compiler bug generated bad code.  Used count instead of cycle for test.
		} else if ( count == 0 ) {
// RAVEN END
			cycle = 1;

			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length();
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + anim->Length() / rate;
			} else {
				endtime = -1;
			}
		} else {
			// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
			if ( rate == 1.0f ) {
				endtime	= starttime - timeOffset + anim->Length() * cycle;
			} else if ( rate != 0.0f ) {
				endtime	= starttime - timeOffset + ( anim->Length() * cycle ) / rate;
			} else {
				endtime = -1;
			}
		}
	}
}

/*
=====================
idAnimBlend::SetPlaybackRate
=====================
*/
void idAnimBlend::SetPlaybackRate( int currentTime, float newRate ) {
	int animTime;

	if ( rate == newRate ) {
		return;
	}

	animTime = AnimTime( currentTime );
	if ( newRate == 1.0f ) {
		timeOffset = animTime - ( currentTime - starttime );
	} else {
		timeOffset = animTime - ( currentTime - starttime ) * newRate;
	}

	rate = newRate;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
idAnimBlend::GetPlaybackRate
=====================
*/
float idAnimBlend::GetPlaybackRate( void ) const {
	return rate;
}

/*
=====================
idAnimBlend::SetStartTime
=====================
*/
void idAnimBlend::SetStartTime( int _startTime ) {
	starttime = _startTime;

	// update the anim endtime
	SetCycleCount( cycle );
}

/*
=====================
idAnimBlend::GetStartTime
=====================
*/
int idAnimBlend::GetStartTime( void ) const {
	if ( !animNum ) {
		return 0;
	}

	return starttime;
}

/*
=====================
idAnimBlend::GetEndTime
=====================
*/
int idAnimBlend::GetEndTime( void ) const {
	if ( !animNum ) {
		return 0;
	}

	return endtime;
}

/*
=====================
idAnimBlend::PlayLength
=====================
*/
int idAnimBlend::PlayLength( void ) const {
	if ( !animNum ) {
		return 0;
	}

	if ( endtime < 0 ) {
		return -1;
	}

	return endtime - starttime + timeOffset;
}

/*
=====================
idAnimBlend::AllowMovement
=====================
*/
void idAnimBlend::AllowMovement( bool allow ) {
	allowMove = allow;
}

/*
=====================
idAnimBlend::AllowFrameCommands
=====================
*/
void idAnimBlend::AllowFrameCommands( bool allow ) {
	allowFrameCommands = allow;
}

/*
=====================
idAnimBlend::AnimNum
=====================
*/
int idAnimBlend::AnimNum( void ) const {
	return animNum;
}

/*
=====================
idAnimBlend::AnimTime
=====================
*/
int idAnimBlend::AnimTime( int currentTime ) const {
	int time;
	int length;
	const idAnim *anim = Anim();

	if ( anim ) {
		if ( useFrameBlend ) {
			return FRAME2MS( frameBlend.frame1 );
		}

		// most of the time we're running at the original frame rate, so avoid the int-to-float-to-int conversion
		if ( rate == 1.0f ) {
			time = currentTime - starttime + timeOffset;
		} else {
			time = static_cast<int>( ( currentTime - starttime ) * rate ) + timeOffset;
		}

		// given enough time, we can easily wrap time around in our frame calculations, so
		// keep cycling animations' time within the length of the anim.
		length = anim->Length();
		if ( ( cycle < 0 ) && ( length > 0 ) ) {
			time %= length;

			// time will wrap after 24 days (oh no!), resulting in negative results for the %.
			// adding the length gives us the proper result.
			if ( time < 0 ) {
				time += length;
			}
		}
		return time;
	} else {
		return 0;
	}
}

/*
=====================
idAnimBlend::GetFrameNumber
=====================
*/
int idAnimBlend::GetFrameNumber( int currentTime ) const {
	const idMD5Anim	*md5anim;
	frameBlend_t	frameinfo;
	int				animTime;

	const idAnim *anim = Anim();
	if ( !anim ) {
		return 1;
	}

	if ( useFrameBlend ) {
		return frameBlend.frame1;
	}

	md5anim = anim->MD5Anim( 0 );
	animTime = AnimTime( currentTime );
	md5anim->ConvertTimeToFrame( animTime, cycle, frameinfo );

	return frameinfo.frame1 + 1;
}

/*
=====================
idAnimBlend::CallFrameCommands
=====================
*/
void idAnimBlend::CallFrameCommands( idEntity *ent, int fromtime, int totime ) const {
	const idMD5Anim	*md5anim;
	frameBlend_t	frame1;
	frameBlend_t	frame2;
	int				fromFrameTime;
	int				toFrameTime;

	if ( !allowFrameCommands || !ent || useFrameBlend || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim || !anim->HasFrameCommands() ) {
		return;
	}

	if ( totime <= starttime ) {
		// don't play until next frame or we'll play commands twice.
		// this happens on the player sometimes.
		return;
	}

	fromFrameTime	= AnimTime( fromtime );
	toFrameTime		= AnimTime( totime );
	if ( toFrameTime < fromFrameTime ) {
		toFrameTime += anim->Length();
	}

	md5anim = anim->MD5Anim( 0 );
	md5anim->ConvertTimeToFrame( fromFrameTime, cycle, frame1 );
	md5anim->ConvertTimeToFrame( toFrameTime, cycle, frame2 );

	if ( fromFrameTime <= 0 ) {
		// make sure first frame is called
		anim->CallFrameCommands( ent, -1, frame2.frame1 );
	} else {
		anim->CallFrameCommands( ent, frame1.frame1, frame2.frame1 );
	}
}

/*
=====================
idAnimBlend::BlendAnim
=====================
*/
bool idAnimBlend::BlendAnim( int currentTime, int channel, int numJoints, idJointQuat *blendFrame, float &blendWeight, bool removeOriginOffset, bool overrideBlend, bool printInfo ) const {
	int				i;
	float			lerp;
	float			mixWeight;
	const idMD5Anim	*md5anim;
	idJointQuat		*ptr;
	frameBlend_t	frametime = {0};
	idJointQuat		*jointFrame;
	idJointQuat		*mixFrame;
	int				numAnims;
	int				time;

	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( blendWeight > 0.0f ) {
		if ( ( endtime >= 0 ) && ( currentTime >= endtime ) ) {
			return false;
		}
		if ( !weight ) {
			return false;
		}
		if ( overrideBlend ) {
			blendWeight = 1.0f - weight;
		}
	}

	if ( ( channel == ANIMCHANNEL_ALL ) && !blendWeight ) {
		// we don't need a temporary buffer, so just store it directly in the blend frame
		jointFrame = blendFrame;
	} else {
		// allocate a temporary buffer to copy the joints from
		jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	}

	time = AnimTime( currentTime );

	numAnims = anim->NumAnims();
	if ( numAnims == 1 ) {
		md5anim = anim->MD5Anim( 0 );
		if ( useFrameBlend ) {
			md5anim->GetInterpolatedFrame( frameBlend, jointFrame, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
		} else {
			md5anim->ConvertTimeToFrame( time, cycle, frametime );
			md5anim->GetInterpolatedFrame( frametime, jointFrame, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
		}
	} else {
		//
		// need to mix the multipoint anim together first
		//
		// allocate a temporary buffer to copy the joints to
		mixFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );

		if ( !useFrameBlend ) {
			anim->MD5Anim( 0 )->ConvertTimeToFrame( time, cycle, frametime );
		}

		ptr = jointFrame;
		mixWeight = 0.0f;
		for( i = 0; i < numAnims; i++ ) {
			if ( animWeights[ i ] > 0.0f ) {
				mixWeight += animWeights[ i ];
				lerp = animWeights[ i ] / mixWeight;
				md5anim = anim->MD5Anim( i );
				if ( useFrameBlend ) {
					md5anim->GetInterpolatedFrame( frameBlend, ptr, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				} else {
					md5anim->GetInterpolatedFrame( frametime, ptr, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				}

				// only blend after the first anim is mixed in
				if ( ptr != jointFrame ) {
					SIMDProcessor->BlendJoints( jointFrame, ptr, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
				}

				ptr = mixFrame;
			}
		}

		if ( !mixWeight ) {
			return false;
		}
	}

	if ( removeOriginOffset ) {
		if ( allowMove ) {
#ifdef VELOCITY_MOVE
			jointFrame[ 0 ].t.x = 0.0f;
#else
			jointFrame[ 0 ].t.Zero();
#endif
		}

		if ( anim->GetAnimFlags().anim_turn ) {
			jointFrame[ 0 ].q.Set( -0.70710677f, 0.0f, 0.0f, 0.70710677f );
		}
	}

	if ( !blendWeight ) {
		blendWeight = weight;
		if ( channel != ANIMCHANNEL_ALL ) {
			const int *index = modelDef->GetChannelJoints( channel );
			const int num = modelDef->NumJointsOnChannel( channel );
			for( i = 0; i < num; i++ ) {
				int j = index[i];
				blendFrame[j].t = jointFrame[j].t;
				blendFrame[j].q = jointFrame[j].q;
			}
		}
    } else {
		blendWeight += weight;
		lerp = weight / blendWeight;
		SIMDProcessor->BlendJoints( blendFrame, jointFrame, lerp, modelDef->GetChannelJoints( channel ), modelDef->NumJointsOnChannel( channel ) );
	}

	if ( printInfo ) {
		if ( useFrameBlend ) {
			gameLocal.Printf( "  %s: '%s', %d, %.2f%%\n", channelNames[ channel ], anim->FullName(), frameBlend.frame1, weight * 100.0f );
		} else {
			gameLocal.Printf( "  %s: '%s', %.3f, %.2f%%\n", channelNames[ channel ], anim->FullName(), ( float )frametime.frame1 + frametime.backlerp, weight * 100.0f );
		}
	}

	return true;
}

/*
=====================
idAnimBlend::BlendOrigin
=====================
*/
void idAnimBlend::BlendOrigin( int currentTime, idVec3 &blendPos, float &blendWeight, bool removeOriginOffset ) const {
	float	lerp;
	idVec3	animpos;
	idVec3	pos;
	int		time;
	int		num;
	int		i;

	if ( useFrameBlend || ( ( endtime > 0 ) && ( currentTime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	if ( allowMove && removeOriginOffset ) {
		return;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return;
	}

	time = AnimTime( currentTime );

	pos.Zero();
	num = anim->NumAnims();
	for( i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time, cycle );
		pos += animpos * animWeights[ i ];
	}

	if ( !blendWeight ) {
		blendPos = pos;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendPos += lerp * ( pos - blendPos );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::BlendDelta
=====================
*/
void idAnimBlend::BlendDelta( int fromtime, int totime, idVec3 &blendDelta, float &blendWeight ) const {
	idVec3	pos1;
	idVec3	pos2;
	idVec3	animpos;
	idVec3	delta;
	int		time1;
	int		time2;
	float	lerp;
	int		num;
	int		i;
	
	if ( useFrameBlend || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	time1 = AnimTime( fromtime );
	time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	num = anim->NumAnims();

	pos1.Zero();
	pos2.Zero();
	for( i = 0; i < num; i++ ) {
		anim->GetOrigin( animpos, i, time1, cycle );
		pos1 += animpos * animWeights[ i ];

		anim->GetOrigin( animpos, i, time2, cycle );
		pos2 += animpos * animWeights[ i ];
	}

	delta = pos2 - pos1;
	if ( !blendWeight ) {
		blendDelta = delta;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendDelta += lerp * ( delta - blendDelta );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::BlendDeltaRotation
=====================
*/
void idAnimBlend::BlendDeltaRotation( int fromtime, int totime, idQuat &blendDelta, float &blendWeight ) const {
	idQuat	q1;
	idQuat	q2;
	idQuat	q3;
	int		time1;
	int		time2;
	float	lerp;
	float	mixWeight;
	int		num;
	int		i;
	
	if ( useFrameBlend || !allowMove || ( ( endtime > 0 ) && ( fromtime > endtime ) ) ) {
		return;
	}

	const idAnim *anim = Anim();
	if ( !anim || !anim->GetAnimFlags().anim_turn ) {
		return;
	}

	float weight = GetWeight( totime );
	if ( !weight ) {
		return;
	}

	time1 = AnimTime( fromtime );
	time2 = AnimTime( totime );
	if ( time2 < time1 ) {
		time2 += anim->Length();
	}

	q1.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	q2.Set( 0.0f, 0.0f, 0.0f, 1.0f );

	mixWeight = 0.0f;
	num = anim->NumAnims();
	for( i = 0; i < num; i++ ) {
		if ( animWeights[ i ] > 0.0f ) {
			mixWeight += animWeights[ i ];
			if ( animWeights[ i ] == mixWeight ) {
				anim->GetOriginRotation( q1, i, time1, cycle );
				anim->GetOriginRotation( q2, i, time2, cycle );
			} else {
				lerp = animWeights[ i ] / mixWeight;
				anim->GetOriginRotation( q3, i, time1, cycle );
				q1.Slerp( q1, q3, lerp );

				anim->GetOriginRotation( q3, i, time2, cycle );
				q2.Slerp( q1, q3, lerp );
			}
		}
	}

	q3 = q1.Inverse() * q2;
	if ( !blendWeight ) {
		blendDelta = q3;
		blendWeight = weight;
	} else {
		lerp = weight / ( blendWeight + weight );
		blendDelta.Slerp( blendDelta, q3, lerp );
		blendWeight += weight;
	}
}

/*
=====================
idAnimBlend::AddBounds
=====================
*/
bool idAnimBlend::AddBounds( int currentTime, idBounds &bounds, bool removeOriginOffset ) const {
	int			i;
	int			num;
	idBounds	b;
	int			time;
	idVec3		pos;
	bool		addorigin;

	if ( ( endtime > 0 ) && ( currentTime > endtime ) ) {
		return false;
	}

	const idAnim *anim = Anim();
	if ( !anim ) {
		return false;
	}

	float weight = GetWeight( currentTime );
	if ( !weight ) {
		return false;
	}

	time = AnimTime( currentTime );
	num = anim->NumAnims();
	
	addorigin = !allowMove || !removeOriginOffset;
	for( i = 0; i < num; i++ ) {
		if ( anim->GetBounds( b, i, time, cycle ) ) {
			if ( addorigin ) {
				anim->GetOrigin( pos, i, time, cycle );
				b.TranslateSelf( pos );
			}
			bounds.AddBounds( b );
		}
	}

	return true;
}

/***********************************************************************

	idDeclModelDef

***********************************************************************/

/*
=====================
idDeclModelDef::idDeclModelDef
=====================
*/
idDeclModelDef::idDeclModelDef() {
	modelHandle	= NULL;
	skin		= NULL;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
// RAVEN BEGIN
// jsinger: I have to track this for binary decl support
#ifdef RV_BINARYDECLS
	mNumChannels=0;
#endif
// RAVEN END
}

/*
=====================
idDeclModelDef::~idDeclModelDef
=====================
*/
idDeclModelDef::~idDeclModelDef() {
	FreeData();
}

/*
=================
idDeclModelDef::Size
=================
*/
// RAVEN BEGIN
// jscott: made more accurate
size_t idDeclModelDef::Size( void ) const {

	int		i;
	size_t	size;

	size = sizeof( idDeclModelDef );
	size += joints.Allocated();
	size += jointParents.Allocated();
	size += anims.Allocated();

	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {

		size += channelJoints[i].Allocated();
	}

	return( size );
}
// RAVEN END

/*
=====================
idDeclModelDef::CopyDecl
=====================
*/
void idDeclModelDef::CopyDecl( const idDeclModelDef *decl ) {
	int i;

	FreeData();

	offset = decl->offset;
	modelHandle = decl->modelHandle;
	skin = decl->skin;

	anims.SetNum( decl->anims.Num() );
	for( i = 0; i < anims.Num(); i++ ) {
		anims[ i ] = new idAnim( this, decl->anims[ i ] );
	}

	joints.SetNum( decl->joints.Num() );
// RAVEN BEGIN
// JSinger: Changed to call optimized memcpy
	SIMDProcessor->Memcpy( joints.Ptr(), decl->joints.Ptr(), decl->joints.Num() * sizeof( joints[0] ) );
// RAVEN END
	jointParents.SetNum( decl->jointParents.Num() );
// RAVEN BEGIN
// JSinger: Changed to call optimized memcpy
	SIMDProcessor->Memcpy( jointParents.Ptr(), decl->jointParents.Ptr(), decl->jointParents.Num() * sizeof( jointParents[0] ) );
// RAVEN END
	for ( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i] = decl->channelJoints[i];
	}
}

/*
=====================
idDeclModelDef::FreeData
=====================
*/
void idDeclModelDef::FreeData( void ) {
	anims.DeleteContents( true );
	joints.Clear();
	jointParents.Clear();
	modelHandle	= NULL;
	skin = NULL;
	offset.Zero();
	for ( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		channelJoints[i].Clear();
	}
}

/*
================
idDeclModelDef::DefaultDefinition
================
*/
const char *idDeclModelDef::DefaultDefinition( void ) const {
	return "{ }";
}

/*
====================
idDeclModelDef::FindJoint
====================
*/
const jointInfo_t *idDeclModelDef::FindJoint( const char *name ) const {
	int					i;
	const idMD5Joint	*joint;

	if ( !modelHandle ) {
		return NULL;
	}
	
	joint = modelHandle->GetJoints();
	for( i = 0; i < joints.Num(); i++, joint++ ) {
		if ( !joint->name.Icmp( name ) ) {
			return &joints[ i ];
		}
	}

	return NULL;
}

/*
=====================
idDeclModelDef::ModelHandle
=====================
*/
idRenderModel *idDeclModelDef::ModelHandle( void ) const {
	return ( idRenderModel * )modelHandle;
}

/*
=====================
idDeclModelDef::GetJointList
=====================
*/
void idDeclModelDef::GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const {
	const char			*pos;
	idStr				jointname;
	const jointInfo_t	*joint;
	const jointInfo_t	*child;
	int					i;
	int					num;
	bool				getChildren;
	bool				subtract;

	if ( !modelHandle ) {
		return;
	}

	jointList.Clear();

	num = modelHandle->NumJoints();

	// scan through list of joints and add each to the joint list
	pos = jointnames;
	while( *pos ) {
		// skip over whitespace
		while( ( *pos != 0 ) && isspace( *pos ) ) {
			pos++;
		}

		if ( !*pos ) {
			// no more names
			break;
		}

		// copy joint name
		jointname = "";

		if ( *pos == '-' ) {
			subtract = true;
			pos++;
		} else {
			subtract = false;
		}

		if ( *pos == '*' ) {
			getChildren = true;
			pos++;
		} else {
			getChildren = false;
		}

		while( ( *pos != 0 ) && !isspace( *pos ) ) {
			jointname += *pos;
			pos++;
		}

		joint = FindJoint( jointname );
		if ( !joint ) {
			gameLocal.Warning( "Unknown joint '%s' in '%s' for model '%s'", jointname.c_str(), jointnames, GetName() );
			continue;
		}

		if ( !subtract ) {
			jointList.AddUnique( joint->num );
		} else {
			jointList.Remove( joint->num );
		}

		if ( getChildren ) {
			// include all joint's children
			child = joint + 1;
			for( i = joint->num + 1; i < num; i++, child++ ) {
				// all children of the joint should follow it in the list.
				// once we reach a joint without a parent or with a parent
				// who is earlier in the list than the specified joint, then
				// we've gone through all it's children.
				if ( child->parentNum < joint->num ) {
					break;
				}

				if ( !subtract ) {
					jointList.AddUnique( child->num );
				} else {
					jointList.Remove( child->num );
				}
			}
		}
	}
}

/*
=====================
idDeclModelDef::Touch
=====================
*/
void idDeclModelDef::Touch( void ) const {
	if ( modelHandle ) {
		renderModelManager->FindModel( modelHandle->Name() );
	}
}

/*
=====================
idDeclModelDef::GetDefaultSkin
=====================
*/
const idDeclSkin *idDeclModelDef::GetDefaultSkin( void ) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetDefaultPose
=====================
*/
const idJointQuat *idDeclModelDef::GetDefaultPose( void ) const {
	return modelHandle->GetDefaultPose();
}

/*
=====================
idDeclModelDef::SetupJoints
=====================
*/
void idDeclModelDef::SetupJoints( int *numJoints, idJointMat **jointList, idBounds &frameBounds, bool removeOriginOffset ) const {
	int					num;
	const idJointQuat	*pose;
	idJointMat			*list;

	if ( !modelHandle || modelHandle->IsDefaultModel() ) {
		Mem_Free16( (*jointList) );
		(*jointList) = NULL;
		frameBounds.Clear();
		return;
	}

	// get the number of joints
	num = modelHandle->NumJoints();

	if ( !num ) {
		gameLocal.Error( "model '%s' has no joints", modelHandle->Name() );
	}

	// set up initial pose for model (with no pose, model is just a jumbled mess)
//RAVEN BEGIN
//amccarthy: Added allocation tag
	list = (idJointMat *) Mem_Alloc16( num * sizeof( list[0] ), MA_ANIM );
//RAVEN END
	pose = GetDefaultPose();

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( list, pose, joints.Num() );

	// check if we offset the model by the origin joint
	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		list[ 0 ].SetTranslation( idVec3( offset.x, offset.y + pose[0].t.y, offset.z + pose[0].t.z ) );
#else
		list[ 0 ].SetTranslation( offset );
#endif
	} else {
		list[ 0 ].SetTranslation( pose[0].t + offset );
	}

	// transform the joint hierarchy
	SIMDProcessor->TransformJoints( list, jointParents.Ptr(), 1, joints.Num() - 1 );

	*numJoints = num;
	*jointList = list;

	// get the bounds of the default pose
	frameBounds = modelHandle->Bounds( NULL );
}

/*
=====================
idDeclModelDef::ParseAnim
=====================
*/
bool idDeclModelDef::ParseAnim( idLexer &src, int numDefaultAnims ) {
	int				i;
	int				len;
	idAnim			*anim;
	const idMD5Anim	*md5anims[ ANIM_MaxSyncedAnims ];
	const idMD5Anim	*md5anim;
	idStr			alias;
	idToken			realname;
	idToken			token;
	int				numAnims;
	animFlags_t		flags;

	numAnims = 0;
	memset( md5anims, 0, sizeof( md5anims ) );

	if( !src.ReadToken( &realname ) ) {
		src.Warning( "Unexpected end of file" );
		MakeDefault();
		return false;
	}
	alias = realname;

	for( i = 0; i < anims.Num(); i++ ) {
		if ( !idStr::Cmp( anims[ i ]->FullName(), realname ) ) {
			break;
		}
	}

	if ( ( i < anims.Num() ) && ( i >= numDefaultAnims ) ) {
		src.Warning( "Duplicate anim '%s'", realname.c_str() );
		MakeDefault();
		return false;
	}

	if ( i < numDefaultAnims ) {
		anim = anims[ i ];
	} else {
		// create the alias associated with this animation
		anim = new idAnim();
		anims.Append( anim );
	}

	// random anims end with a number.  find the numeric suffix of the animation.
	len = alias.Length();
	for( i = len - 1; i > 0; i-- ) {
		if ( !isdigit( alias[ i ] ) ) {
			break;
		}
	}

	// check for zero length name, or a purely numeric name
	if ( i <= 0 ) {
		src.Warning( "Invalid animation name '%s'", alias.c_str() );
		MakeDefault();
		return false;
	}

	// remove the numeric suffix
	alias.CapLength( i + 1 );

	// parse the anims from the string
	do {
		if( !src.ReadToken( &token ) ) {
			src.Warning( "Unexpected end of file" );
			MakeDefault();
			return false;
		}

		// lookup the animation
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
		md5anim = animationLib->GetAnim( token );
// RAVEN END
		if ( !md5anim ) {
			src.Warning( "Couldn't load anim '%s'", token.c_str() );
			MakeDefault();
			return false;
		}

		md5anim->CheckModelHierarchy( modelHandle );

		if ( numAnims > 0 ) {
			// make sure it's the same length as the other anims
			if ( md5anim->Length() != md5anims[ 0 ]->Length() ) {
				src.Warning( "Anim '%s' does not match length of anim '%s'", md5anim->Name(), md5anims[ 0 ]->Name() );
				MakeDefault();
				return false;
			}
		}

		if ( numAnims >= ANIM_MaxSyncedAnims ) {
			src.Warning( "Exceeded max synced anims (%d)", ANIM_MaxSyncedAnims );
			MakeDefault();
			return false;
		}

		// add it to our list
		md5anims[ numAnims ] = md5anim;
		numAnims++;
	} while ( src.CheckTokenString( "," ) );

	if ( !numAnims ) {
		src.Warning( "No animation specified" );
		MakeDefault();
		return false;
	}

	anim->SetAnim( this, realname, alias, numAnims, md5anims );
	memset( &flags, 0, sizeof( flags ) );

	// parse any frame commands or animflags
	if ( src.CheckTokenString( "{" ) ) {
		while( 1 ) {
			if( !src.ReadToken( &token ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			if ( token == "}" ) {
				break;
			}else if ( token == "prevent_idle_override" ) {
				flags.prevent_idle_override = true;
			} else if ( token == "random_cycle_start" ) {
				flags.random_cycle_start = true;			
// RAVEN BEGIN
// bdube: added speed
			} else if ( token == "sync_cycle" ) {
				flags.sync_cycle = true;
			} else if ( token == "rate" ) {
				anim->SetPlaybackRate ( src.ParseFloat ( ) );
			} else if ( token == "ai_no_look" ) {
				flags.ai_no_look = true;
			} else if ( token == "ai_look_head_only" ) {
				flags.ai_look_head_only = true;
// RAVEN END												
			} else if ( token == "ai_no_turn" ) {
				flags.ai_no_turn = true;
			} else if ( token == "anim_turn" ) {
				flags.anim_turn = true;
			} else if ( token == "frame" ) {
				// create a frame command
// RAVEN BEGIN
// bdube: Support a list of frame numbers
//				int			framenum;
				const char	*err;
				idList<int> frameList;

				do
				{						
// RAVEN END				
				// make sure we don't have any line breaks while reading the frame command so the error line # will be correct
				if ( !src.ReadTokenOnLine( &token ) ) {
					src.Warning( "Missing frame # after 'frame'" );
					MakeDefault();
					return false;
				}
				if ( token.type == TT_PUNCTUATION && token == "-" ) {
					src.Warning( "Invalid frame # after 'frame'" );
					MakeDefault();
					return false;
				} else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
					src.Error( "expected integer value, found '%s'", token.c_str() );
				}

// RAVEN BEGIN
// bdube: multiple frames
					frameList.Append ( token.GetIntValue() );
				
				} while ( src.CheckTokenString ( "," ) );
// RAVEN END				

				// put the command on the specified frame of the animation
// RAVEN BEGIN
// bdube: Support a list of frame numbers
				err = anim->AddFrameCommand( this, frameList, src, NULL );
// RAVEN END
				if ( err ) {
					src.Warning( "%s", err );
					MakeDefault();
					return false;
				}
			} else {
				src.Warning( "Unknown command '%s'", token.c_str() );
				MakeDefault();
				return false;
			}
		}
	}

	// set the flags
	anim->SetAnimFlags( flags );
	return true;
}

/*
================
idDeclModelDef::Parse
================
*/
bool idDeclModelDef::Parse( const char *text, const int textLength, bool noCaching ) {
	int					i;
	int					num;
	idStr				filename;
	idStr				extension;
	const idMD5Joint	*md5joint;
	const idMD5Joint	*md5joints;
	idLexer				src;
	idToken				token;
	idToken				token2;
	idStr				jointnames;
	int					channel;
	jointHandle_t		jointnum;
	idList<jointHandle_t> jointList;
	int					numDefaultAnims;
// RAVEN BEGIN
// bdube: attachments
	idList<idStr>		attachJoints;
// RAVEN END

	TIME_THIS_SCOPE( __FUNCLINE__);

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );

	numDefaultAnims = 0;
	while( 1 ) {
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		if ( !token.Icmp( "}" ) ) {
			break;
		}

		if ( token == "inherit" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			
			const idDeclModelDef *copy = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, token2, false ) );
			if ( !copy ) {
				gameLocal.Warning( "Unknown model definition '%s'", token2.c_str() );
			} else if ( copy->GetState() == DS_DEFAULTED ) {
				gameLocal.Warning( "inherited model definition '%s' defaulted", token2.c_str() );
				MakeDefault();
				return false;
			} else {
				CopyDecl( copy );
				numDefaultAnims = anims.Num();
			}
		} else if ( token == "skin" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			skin = declManager->FindSkin( token2 );
			if ( !skin ) {
				src.Warning( "Skin '%s' not found", token2.c_str() );
				MakeDefault();
				return false;
			}
		} else if ( token == "mesh" ) {
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			filename = token2;
			filename.ExtractFileExtension( extension );
			if ( extension != MD5_MESH_EXT ) {
				src.Warning( "Invalid model for MD5 mesh" );
				MakeDefault();
				return false;
			}
			modelHandle = renderModelManager->FindModel( filename );
			if ( !modelHandle ) {
				src.Warning( "Model '%s' not found", filename.c_str() );
				MakeDefault();
				return false;
			}

			if ( modelHandle->IsDefaultModel() ) {
				src.Warning( "Model '%s' defaulted", filename.c_str() );
				MakeDefault();
				return false;
			}

			// get the number of joints
			num = modelHandle->NumJoints();
			if ( !num ) {
				src.Warning( "Model '%s' has no joints", filename.c_str() );
			}

			// set up the joint hierarchy
			joints.SetGranularity( 1 );
			joints.SetNum( num );
			jointParents.SetNum( num );
			channelJoints[0].SetNum( num );
			md5joints = modelHandle->GetJoints();
			md5joint = md5joints;
			for( i = 0; i < num; i++, md5joint++ ) {
				joints[i].channel = ANIMCHANNEL_ALL;
				joints[i].num = static_cast<jointHandle_t>( i );
				if ( md5joint->parent ) {
					joints[i].parentNum = static_cast<jointHandle_t>( md5joint->parent - md5joints );
				} else {
					joints[i].parentNum = INVALID_JOINT;
				}
				jointParents[i] = joints[i].parentNum;
				channelJoints[0][i] = i;
			}
		} else if ( token == "remove" ) {
			// removes any anims whos name matches
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			num = 0;
			for( i = 0; i < anims.Num(); i++ ) {
				if ( ( token2 == anims[ i ]->Name() ) || ( token2 == anims[ i ]->FullName() ) ) {
					delete anims[ i ];
					anims.RemoveIndex( i );
					if ( i >= numDefaultAnims ) {
						src.Warning( "Anim '%s' was not inherited.  Anim should be removed from the model def.", token2.c_str() );
						MakeDefault();
						return false;
					}
					i--;
					numDefaultAnims--;
					num++;
					continue;
				}
			}
			if ( !num ) {
				src.Warning( "Couldn't find anim '%s' to remove", token2.c_str() );
				MakeDefault();
				return false;
			}
		} else if ( token == "anim" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining anims" );
				MakeDefault();
				return false;
			}
			if ( !ParseAnim( src, numDefaultAnims ) ) {
				MakeDefault();
				return false;
			}
		} else if ( token == "offset" ) {
			if ( !src.Parse1DMatrix( 3, offset.ToFloatPtr() ) ) {
				src.Warning( "Expected vector following 'offset'" );
				MakeDefault();
				return false;
			}
		} else if ( token == "channel" ) {
			if ( !modelHandle ) {
				src.Warning( "Must specify mesh before defining channels" );
				MakeDefault();
				return false;
			}

			// set the channel for a group of joints
			if( !src.ReadToken( &token2 ) ) {
				src.Warning( "Unexpected end of file" );
				MakeDefault();
				return false;
			}
			if ( !src.CheckTokenString( "(" ) ) {
				src.Warning( "Expected { after '%s'\n", token2.c_str() );
				MakeDefault();
				return false;
			}

			for( i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
				if ( !stricmp( channelNames[ i ], token2 ) ) {
					break;
				}
			}

			if ( i >= ANIM_NumAnimChannels ) {
				src.Warning( "Unknown channel '%s'", token2.c_str() );
				MakeDefault();
				return false;
			}

			channel = i;
			jointnames = "";

			while( !src.CheckTokenString( ")" ) ) {
				if( !src.ReadToken( &token2 ) ) {
					src.Warning( "Unexpected end of file" );
					MakeDefault();
					return false;
				}
				jointnames += token2;
				if ( ( token2 != "*" ) && ( token2 != "-" ) ) {
					jointnames += " ";
				}
			}

			GetJointList( jointnames, jointList );

			channelJoints[ channel ].SetNum( jointList.Num() );
			for( num = i = 0; i < jointList.Num(); i++ ) {
				jointnum = jointList[ i ];
				if ( joints[ jointnum ].channel != ANIMCHANNEL_ALL ) {
					src.Warning( "Joint '%s' assigned to multiple channels", modelHandle->GetJointName( jointnum ) );
					continue;
				}
				joints[ jointnum ].channel = channel;
				channelJoints[ channel ][ num++ ] = jointnum;
			}
			channelJoints[ channel ].SetNum( num );
// RAVEN BEGIN
// jsinger: I have to track this for binary decl support
#ifdef RV_BINARYDECLS
			mNumChannels++;
#endif
// RAVEN END
		} else {
			src.Warning( "unknown token '%s'", token.c_str() );
			MakeDefault();
			return false;
		}
	}

	// shrink the anim list down to save space
	anims.SetGranularity( 1 );
	anims.SetNum( anims.Num() );

	return true;
}

/*
=====================
idDeclModelDef::Validate
=====================
*/
bool idDeclModelDef::Validate( const char *psText, int iTextLength, idStr &strReportTo ) const {
	idDeclModelDef *pSelf = (idDeclModelDef*) declManager->AllocateDecl( DECL_MODELDEF );
	bool bOk = pSelf->Parse( psText, iTextLength, false );
	pSelf->FreeData();
	delete pSelf->base;
	delete pSelf;

	return bOk;
}

/*
=====================
idDeclModelDef::HasAnim
=====================
*/
bool idDeclModelDef::HasAnim( const char *name ) const {
	int	i;

	// find any animations with same name
	for( i = 0; i < anims.Num(); i++ ) {
		if ( !idStr::Cmp( anims[ i ]->Name(), name ) ) {
			return true;
		}
	}
	
	return false;
}

/*
=====================
idDeclModelDef::NumAnims
=====================
*/
int idDeclModelDef::NumAnims( void ) const {
	return anims.Num() + 1;
}

/*
=====================
idDeclModelDef::GetSpecificAnim

Gets the exact anim for the name, without randomization.
=====================
*/
int idDeclModelDef::GetSpecificAnim( const char *name ) const {
	int	i;

	// find a specific animation
	for( i = 0; i < anims.Num(); i++ ) {
		if ( !idStr::Cmp( anims[ i ]->FullName(), name ) ) {
			return i + 1;
		}
	}

	// didn't find it
	return 0;
}

/*
=====================
idDeclModelDef::GetAnim
=====================
*/
int idDeclModelDef::GetAnim( const char *name ) const {
	int				i;
	int				which;
	const int		MAX_ANIMS = 64;
	int				animList[ MAX_ANIMS ];
	int				numAnims;
	int				len;

	len = strlen( name );
	if ( len && idStr::CharIsNumeric( name[ len - 1 ] ) ) {
		// find a specific animation
		return GetSpecificAnim( name );
	}

	// find all animations with same name
	numAnims = 0;
	for( i = 0; i < anims.Num(); i++ ) {
		if ( !idStr::Cmp( anims[ i ]->Name(), name ) ) {
			animList[ numAnims++ ] = i;
			if ( numAnims >= MAX_ANIMS ) {
				break;
			}
		}
	}

	if ( !numAnims ) {
		return 0;
	}

	// get a random anim
	//FIXME: don't access gameLocal here?
	which = gameLocal.random.RandomInt( numAnims );
	return animList[ which ] + 1;
}

/*
=====================
idDeclModelDef::GetSkin
=====================
*/
const idDeclSkin *idDeclModelDef::GetSkin( void ) const {
	return skin;
}

/*
=====================
idDeclModelDef::GetModelName
=====================
*/
const char *idDeclModelDef::GetModelName( void ) const {
	if ( modelHandle ) {
		return modelHandle->Name();
	} else {
		return "";
	}
}

/*
=====================
idDeclModelDef::Joints
=====================
*/
const idList<jointInfo_t> &idDeclModelDef::Joints( void ) const {
	return joints;
}

/*
=====================
idDeclModelDef::JointParents
=====================
*/
const int * idDeclModelDef::JointParents( void ) const {
	return jointParents.Ptr();
}

/*
=====================
idDeclModelDef::NumJoints
=====================
*/
int idDeclModelDef::NumJoints( void ) const {
	return joints.Num();
}

/*
=====================
idDeclModelDef::GetJoint
=====================
*/
const jointInfo_t *idDeclModelDef::GetJoint( int jointHandle ) const {
	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "idDeclModelDef::GetJoint : joint handle out of range" );
	}
	return &joints[ jointHandle ];
}

/*
====================
idDeclModelDef::GetJointName
====================
*/
const char *idDeclModelDef::GetJointName( int jointHandle ) const {
	const idMD5Joint *joint;

	if ( !modelHandle ) {
		return NULL;
	}
	
	if ( ( jointHandle < 0 ) || ( jointHandle > joints.Num() ) ) {
		gameLocal.Error( "idDeclModelDef::GetJointName : joint handle out of range" );
	}

	joint = modelHandle->GetJoints();
	return joint[ jointHandle ].name.c_str();
}

/*
=====================
idDeclModelDef::NumJointsOnChannel
=====================
*/
int idDeclModelDef::NumJointsOnChannel( int channel ) const {
	if ( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idDeclModelDef::NumJointsOnChannel : channel out of range" );
	}
	return channelJoints[ channel ].Num();
}

/*
=====================
idDeclModelDef::GetChannelJoints
=====================
*/
const int * idDeclModelDef::GetChannelJoints( int channel ) const {
	if ( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idDeclModelDef::GetChannelJoints : channel out of range" );
	}
	return channelJoints[ channel ].Ptr();
}

/*
=====================
idDeclModelDef::GetVisualOffset
=====================
*/
const idVec3 &idDeclModelDef::GetVisualOffset( void ) const {
	return offset;
}

// RAVEN BEGIN
// jsinger: allow exporting of this decl type in a preparsed form
#ifdef RV_BINARYDECLS
void idDeclModelDef::Write( SerialOutputStream &stream ) const
{
	WriteValue(offset[0], stream);
	WriteValue(offset[1], stream);
	WriteValue(offset[2], stream);
	WriteValue(joints.Num(), stream);
	for(int i=0; i<joints.Num(); i++)
	{
        WriteValue(joints[i].num, stream);
		WriteValue(joints[i].parentNum, stream);
		WriteValue(joints[i].channel, stream);
	}

	WriteValue(jointParents.Num(), stream);
	for(int i=0; i<jointParents.Num(); i++)
	{
		WriteValue(jointParents[i], stream);
	}

	WriteValue(mNumChannels, stream);
	for(int i=0; i<mNumChannels; i++)
	{
		WriteValue(channelJoints[i].Num(), stream);
		for(int j=0; j<channelJoints[i].Num(); j++)
		{
			WriteValue(channelJoints[i][j], stream);
		}
	}

	WriteValue(idStr(modelHandle->Name()), stream);
	WriteValue(anims.Num(), stream);
	for(int i=0; i<anims.Num(); i++)
	{
		anims[i]->Write(stream);
	}

	if(skin)
	{
		WriteValue(idStr(skin->GetName()), stream);
	}
	else
	{
		WriteValue(idStr(""), stream);
	}
}

void idDeclModelDef::AddReferences() const
{
}

idDeclModelDef::idDeclModelDef( SerialInputStream &stream )
{
	offset[0] = stream.ReadFloatValue();
	offset[1] = stream.ReadFloatValue();
	offset[2] = stream.ReadFloatValue();
	int numJoints = stream.ReadIntValue();
	joints.AssureSize(numJoints);
	for(int i=0; i<numJoints; i++)
	{
		joints[i].num = (jointHandle_t)stream.ReadIntValue();
		joints[i].parentNum = (jointHandle_t)stream.ReadIntValue();
		joints[i].channel = stream.ReadIntValue();
	}

	int numJointParents = stream.ReadIntValue();
	jointParents.AssureSize(numJointParents);
	for(int i=0; i<numJointParents; i++)
	{
		jointParents[i] = stream.ReadIntValue();
	}

	mNumChannels = stream.ReadIntValue();
	for(int i=0; i<mNumChannels; i++)
	{
		int numChannelJoints = stream.ReadIntValue();
		channelJoints[i].AssureSize(numChannelJoints);
		for(int j=0; j<numChannelJoints; j++)
		{
			channelJoints[i][j] = stream.ReadIntValue();
		}
	}
	idStr modelName = stream.ReadStringValue();
	if(!modelName.Icmp(""))
	{
		modelHandle = NULL;
	}
	else
	{
		modelHandle = renderModelManager->FindModel( modelName.c_str() );
	}
	int numAnims = stream.ReadIntValue();
	anims.AssureSize(numAnims);
	for(int i=0; i<numAnims; i++)
	{
		// sorry that I have to cast away const, but it was that way
		// in memory once before anyway
		anims[i] = new idAnim( this, stream );
		//anims[i] = (idAnim *)GetAnim(stream.ReadStringValue().c_str());
	}
	idStr skinName = stream.ReadStringValue();
	if(skinName.Icmp(""))
	{
		skin = declManager->FindSkin(skinName.c_str());
	}
	else
	{
		skin = NULL;
	}
}
#endif
// RAVEN END
/***********************************************************************

	idAnimator

***********************************************************************/

/*
=====================
idAnimator::idAnimator
=====================
*/
idAnimator::idAnimator() {
	int	i, j;

	modelDef				= NULL;
	entity					= NULL;
	numJoints				= 0;
	joints					= NULL;
	lastTransformTime		= -1;
	stoppedAnimatingUpdate	= false;
	removeOriginOffset		= false;
	forceUpdate				= false;

// RAVEN BEGIN
// jshepard:
	rateMultiplier			= 1;
// RAVEN END

	frameBounds.Clear();

	AFPoseJoints.SetGranularity( 1 );
	AFPoseJointMods.SetGranularity( 1 );
	AFPoseJointFrame = NULL;
	AFPoseJointFrameSize = 0;

	ClearAFPose();

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( NULL );
		}
	}
}

/*
=====================
idAnimator::~idAnimator
=====================
*/
idAnimator::~idAnimator() {
	FreeData();
}

/*
=====================
idAnimator::Allocated
=====================
*/
size_t idAnimator::Allocated( void ) const {
	size_t	size;

	size = jointMods.Allocated() + numJoints * sizeof( joints[0] ) + jointMods.Num() * sizeof( jointMods[ 0 ] ) + AFPoseJointMods.Allocated() + AFPoseJointFrameSize * sizeof( AFPoseJointFrame[0] ) + AFPoseJoints.Allocated();

	return size;
}

/*
=====================
idAnimator::Save

archives object for save game file
=====================
*/
void idAnimator::Save( idSaveGame *savefile ) const {
	int i;
	int j;

	savefile->WriteModelDef( modelDef );
	savefile->WriteObject( entity );

	savefile->WriteInt( jointMods.Num() );
	for( i = 0; i < jointMods.Num(); i++ ) {
		savefile->Write( jointMods[ i ], sizeof( *jointMods[ i ] ) );
	}
	
	savefile->WriteInt( numJoints );
	savefile->Write( joints, numJoints * sizeof( joints[0] ) );

	savefile->WriteInt( lastTransformTime );
	savefile->WriteBool( stoppedAnimatingUpdate );
	savefile->WriteBool( forceUpdate );
	savefile->WriteBounds( frameBounds );

	savefile->WriteFloat( AFPoseBlendWeight );

	savefile->WriteInt( AFPoseJoints.Num() );
	savefile->Write( AFPoseJoints.Ptr(), AFPoseJoints.MemoryUsed() );

	savefile->WriteInt( AFPoseJointMods.Num() );
	savefile->Write( AFPoseJointMods.Ptr(), AFPoseJointMods.MemoryUsed() );

	savefile->WriteInt( AFPoseJointFrameSize );
	savefile->Write( AFPoseJointFrame, AFPoseJointFrameSize * sizeof( AFPoseJointFrame[0] ) );

	savefile->WriteBounds( AFPoseBounds );
	savefile->WriteInt( AFPoseTime );

	savefile->WriteBool( removeOriginOffset );

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Save( savefile );
		}
	}
}

/*
=====================
idAnimator::Restore

unarchives object from save game file
=====================
*/
void idAnimator::Restore( idRestoreGame *savefile ) {
	int i;
	int j;
	int num;

	savefile->ReadModelDef( modelDef );
	savefile->ReadObject( reinterpret_cast<idClass *&>( entity ) );

	savefile->ReadInt( num );
	jointMods.SetNum( num );
	for( i = 0; i < num; i++ ) {
		jointMods[ i ] = new jointMod_t;
		savefile->Read( jointMods[ i ], sizeof( *jointMods[ i ] ) );
	}
	
	savefile->ReadInt( numJoints );
//RAVEN BEGIN
//amccarthy: Added allocation tag
	joints = (idJointMat *) Mem_Alloc16( numJoints * sizeof( joints[0] ), MA_ANIM );
//RAVEN END
	savefile->Read( joints, numJoints * sizeof( joints[0] ) );

	savefile->ReadInt( lastTransformTime );
	savefile->ReadBool( stoppedAnimatingUpdate );
	savefile->ReadBool( forceUpdate );
	savefile->ReadBounds( frameBounds );

	savefile->ReadFloat( AFPoseBlendWeight );

	savefile->ReadInt( num );
	AFPoseJoints.SetGranularity( 1 );
	AFPoseJoints.SetNum( num );
	savefile->Read( AFPoseJoints.Ptr(), AFPoseJoints.MemoryUsed() );

	savefile->ReadInt( num );
	AFPoseJointMods.SetGranularity( 1 );
	AFPoseJointMods.SetNum( num );
	savefile->Read( AFPoseJointMods.Ptr(), AFPoseJointMods.MemoryUsed() );

	savefile->ReadInt( AFPoseJointFrameSize );
	AFPoseJointFrame = (idJointQuat *)Mem_Alloc16( AFPoseJointFrameSize * sizeof( AFPoseJointFrame[0] ), MA_ANIM );
	savefile->Read( AFPoseJointFrame, AFPoseJointFrameSize * sizeof( AFPoseJointFrame[0] ) );

	savefile->ReadBounds( AFPoseBounds );
	savefile->ReadInt( AFPoseTime );

	savefile->ReadBool( removeOriginOffset );

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Restore( savefile, modelDef );
		}
	}
}

/*
=====================
idAnimator::FreeData
=====================
*/
void idAnimator::FreeData( void ) {
	int	i, j;

	if ( entity ) {
		entity->BecomeInactive( TH_ANIMATE );
	}

	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( NULL );
		}
	}

	jointMods.DeleteContents( true );

	Mem_Free16( joints );
	joints = NULL;
	numJoints = 0;

	modelDef = NULL;

	Mem_Free16( AFPoseJointFrame );
	AFPoseJointFrame = NULL;
	AFPoseJointFrameSize = 0;

	ForceUpdate();
}

/*
=====================
idAnimator::PushAnims
=====================
*/
void idAnimator::PushAnims( int channelNum, int currentTime, int blendTime ) {
	int			i;
	idAnimBlend *channel;

	channel = channels[ channelNum ];
	if ( !channel[ 0 ].GetWeight( currentTime ) || ( channel[ 0 ].starttime == currentTime ) ) {
		return;
	}

	for( i = ANIM_MaxAnimsPerChannel - 1; i > 0; i-- ) {
		channel[ i ] = channel[ i - 1 ];
	}

	channel[ 0 ].Reset( modelDef );
	channel[ 1 ].Clear( currentTime, blendTime );
	ForceUpdate();
}

/*
=====================
idAnimator::SetModel
=====================
*/
idRenderModel *idAnimator::SetModel( const char *modelname ) {
	int i, j;

	FreeData();

	// check if we're just clearing the model
	if ( !modelname || !*modelname ) {
		return NULL;
	}

	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( !modelDef ) {
		return NULL;
	}
	
	idRenderModel *renderModel = modelDef->ModelHandle();
	if ( !renderModel ) {
		modelDef = NULL;
		return NULL;
	}

	// make sure model hasn't been purged
	modelDef->Touch();
	
// RAVEN BEGIN
// bdube: make sure models dont get purged
	if ( modelDef->ModelHandle() ) {
		modelDef->ModelHandle()->SetLevelLoadReferenced ( true );
	}
// RAVEN END	

	modelDef->SetupJoints( &numJoints, &joints, frameBounds, removeOriginOffset );
	modelDef->ModelHandle()->Reset();

	// set the modelDef on all channels
	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++ ) {
			channels[ i ][ j ].Reset( modelDef );
		}
	}

	return modelDef->ModelHandle();
}

/*
=====================
idAnimator::Size
=====================
*/
size_t idAnimator::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

/*
=====================
idAnimator::SetEntity
=====================
*/
void idAnimator::SetEntity( idEntity *ent ) {
	entity = ent;
}

/*
=====================
idAnimator::GetEntity
=====================
*/
idEntity *idAnimator::GetEntity( void ) const {
	return entity;
}

/*
=====================
idAnimator::RemoveOriginOffset
=====================
*/
void idAnimator::RemoveOriginOffset( bool remove ) {
	removeOriginOffset = remove;
}

/*
=====================
idAnimator::RemoveOrigin
=====================
*/
bool idAnimator::RemoveOrigin( void ) const {
	return removeOriginOffset;
}

/*
=====================
idAnimator::GetJointList
=====================
*/
void idAnimator::GetJointList( const char *jointnames, idList<jointHandle_t> &jointList ) const {
	if ( modelDef ) {
		modelDef->GetJointList( jointnames, jointList );
	}
}

/*
=====================
idAnimator::NumAnims
=====================
*/
int	idAnimator::NumAnims( void ) const {
	if ( !modelDef ) {
		return 0;
	}
	
	return modelDef->NumAnims();
}

/*
=====================
idAnimator::GetAnim
=====================
*/
const idAnim *idAnimator::GetAnim( int index ) const {
	if ( !modelDef ) {
		return NULL;
	}
	
	return modelDef->GetAnim( index );
}

/*
=====================
idAnimator::GetAnim
=====================
*/
int idAnimator::GetAnim( const char *name ) const {
	if ( !modelDef ) {
		return 0;
	}
	
	return modelDef->GetAnim( name );
}

/*
=====================
idAnimator::HasAnim
=====================
*/
bool idAnimator::HasAnim( const char *name ) const {
	if ( !modelDef ) {
		return false;
	}
	
	return modelDef->HasAnim( name );
}

/*
=====================
idAnimator::NumJoints
=====================
*/
int	idAnimator::NumJoints( void ) const {
	return numJoints;
}

/*
=====================
idAnimator::ModelHandle
=====================
*/
idRenderModel *idAnimator::ModelHandle( void ) const {
	if ( !modelDef ) {
		return NULL;
	}
	
	return modelDef->ModelHandle();
}

/*
=====================
idAnimator::ModelDef
=====================
*/
const idDeclModelDef *idAnimator::ModelDef( void ) const {
	return modelDef;
}

/*
=====================
idAnimator::CurrentAnim
=====================
*/
idAnimBlend *idAnimator::CurrentAnim( int channelNum ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::CurrentAnim : channel out of range" );
	}

	return &channels[ channelNum ][ 0 ];
}

/*
=====================
idAnimator::Clear
=====================
*/
void idAnimator::Clear( int channelNum, int currentTime, int cleartime ) {
	int			i;
	idAnimBlend	*blend;

	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::Clear : channel out of range" );
	}

	blend = channels[ channelNum ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->Clear( currentTime, cleartime );
	}
	ForceUpdate();
}

/*
=====================
idAnimator::SetFrame
=====================
*/
void idAnimator::SetFrame( int channelNum, int animNum, const frameBlend_t & frameBlend ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::SetFrame : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}

	PushAnims( channelNum, gameLocal.time, 0 );
	channels[ channelNum ][ 0 ].SetFrame( modelDef, animNum, frameBlend );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::CycleAnim
=====================
*/
void idAnimator::CycleAnim( int channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::CycleAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}
	
	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][ 0 ].CycleAnim( modelDef, animNum, currentTime, blendTime, rateMultiplier );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}

	// If the old animation and new animation both sync cycles then do so now
	idAnimBlend* oldChannel = &channels[channelNum][1];
	idAnimBlend* newChannel = &channels[channelNum][0];
	if ( oldChannel->AnimNum ( ) && newChannel->AnimNum ( ) ) {		
		if ( oldChannel->Anim ( )->GetAnimFlags ( ).sync_cycle && newChannel->Anim ( )->GetAnimFlags ( ).sync_cycle ) {
			// Calculate the percentage through the animation the old channel was 
			float f = (float)oldChannel->GetFrameNumber ( currentTime ) / (float)oldChannel->NumFrames ( );
			// Move the new channel back in time to start it mid cycle
			newChannel->SetStartTime ( gameLocal.time - f * (newChannel->Length ( ) / newChannel->GetPlaybackRate()) );
		}
	}
}

/*
=====================
idAnimator::PlayAnim
=====================
*/
void idAnimator::PlayAnim( int channelNum, int animNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::PlayAnim : channel out of range" );
	}

	if ( !modelDef || !modelDef->GetAnim( animNum ) ) {
		return;
	}
	
	PushAnims( channelNum, currentTime, blendTime );
	channels[ channelNum ][ 0 ].PlayAnim( modelDef, animNum, currentTime, blendTime, rateMultiplier );
	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

/*
=====================
idAnimator::SyncAnimChannels
=====================
*/
void idAnimator::SyncAnimChannels( int channelNum, int fromChannelNum, int currentTime, int blendTime ) {
	if ( ( channelNum < 0 ) || ( channelNum >= ANIM_NumAnimChannels ) || ( fromChannelNum < 0 ) || ( fromChannelNum >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::SyncToChannel : channel out of range" );
	}

	idAnimBlend &fromBlend = channels[ fromChannelNum ][ 0 ];
	idAnimBlend &toBlend = channels[ channelNum ][ 0 ];

	float weight = fromBlend.blendEndValue;
	if ( ( fromBlend.Anim() != toBlend.Anim() ) || ( fromBlend.GetStartTime() != toBlend.GetStartTime() ) || ( fromBlend.GetEndTime() != toBlend.GetEndTime() ) ) {
		PushAnims( channelNum, currentTime, blendTime );
		toBlend = fromBlend;
		toBlend.blendStartValue = 0.0f;
		toBlend.blendEndValue = 0.0f;
	}
    toBlend.SetWeight( weight, currentTime - 1, blendTime );

	// disable framecommands on the current channel so that commands aren't called twice
	toBlend.AllowFrameCommands( false );

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
}

// RAVEN BEGIN
/*
=====================
idAnimator::FindJointMod
=====================
*/
jointMod_t* idAnimator::FindExistingJointMod( jointHandle_t jointnum, int *index ) {
	jointMod_t* jointMod;
	int			i;
	
	jointMod = NULL;
	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			jointMod = jointMods[ i ];
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}

	if ( index )
	{
		*index = i;
	}
	return jointMod;
}

// bdube: added methods
/*
=====================
idAnimator::FindJointMod
=====================
*/
jointMod_t* idAnimator::FindJointMod ( jointHandle_t jointnum ) {
	jointMod_t* jointMod;
	int			i;
	
	jointMod = FindExistingJointMod( jointnum, &i );

	if ( !jointMod )  {
		jointMod = new jointMod_t;
		jointMod->jointnum = jointnum;		
		jointMod->mat.Identity();
		jointMod->transform_axis = JOINTMOD_NONE;
		jointMod->transform_pos  = JOINTMOD_NONE;
		jointMod->angularVelocity.Init ( 0, 0, 0, 0, idAngles(0,0,0), idAngles(0,0,0) );
		jointMod->lastTime = 0;
		jointMods.Insert( jointMod, i );
	}
	
	return jointMod;
}

// abahr:
/*
=====================
idAnimator::SetPlaybackRate
=====================
*/
void idAnimator::SetPlaybackRate( const char* animName, float rate ) {
	SetPlaybackRate( GetAnim(animName), rate );
}

/*
=====================
idAnimator::SetPlaybackRate
=====================
*/
void idAnimator::SetPlaybackRate( int animHandle, float rate ) {
	idAnim* anim = const_cast<idAnim*>( GetAnim(animHandle) );
	if( anim ) {
		anim->SetPlaybackRate( rate );
	}
}
// RAVEN END

/*
=====================
idAnimator::SetJointPos
=====================
*/
void idAnimator::SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
// RAVEN BEGIN
// bdube: moved old code to FindJointMod
/*
	int i;
*/
//
	jointMod_t *jointMod;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

// RAVEN BEGIN
// bdube: moved old code to FindJointMod
	jointMod = FindJointMod ( jointnum );

/*
	jointMod = NULL;
	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			jointMod = jointMods[ i ];
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new jointMod_t;
		jointMod->jointnum = jointnum;
		jointMod->mat.Identity();
		jointMod->transform_axis = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}
*/
// RAVEN END

	jointMod->pos = pos;
	jointMod->transform_pos = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

/*
=====================
idAnimator::SetJointAxis
=====================
*/
void idAnimator::SetJointAxis( jointHandle_t jointnum, jointModTransform_t transform_type, const idMat3 &mat ) {
// RAVEN BEGIN
// bdube: moved old code to FindJointMod
/*
	int i;
*/
// RAVEN END
	jointMod_t *jointMod;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

// RAVEN BEGIN
// bdube: moved old code to FindJointMod
	jointMod = FindJointMod ( jointnum );
/*
	jointMod = NULL;
	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			jointMod = jointMods[ i ];
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}

	if ( !jointMod ) {
		jointMod = new jointMod_t;
		jointMod->jointnum = jointnum;
		jointMod->pos.Zero();
		jointMod->transform_pos = JOINTMOD_NONE;
		jointMods.Insert( jointMod, i );
	}
*/
// RAVEN END

	jointMod->mat = mat;
	jointMod->transform_axis = transform_type;

	if ( entity ) {
		entity->BecomeActive( TH_ANIMATE );
	}
	ForceUpdate();
}

// RAVEN BEGIN
// twhitaker: just added this for uniformity
/*
=====================
idAnimator::GetJointAxis
=====================
*/
void idAnimator::GetJointAxis( jointHandle_t jointnum, idMat3 &mat ) {
	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	mat = FindJointMod ( jointnum )->mat;
}
// RAVEN END

/*
=====================
idAnimator::CollapseJoint

Add a joint modification for the given joint to collapse it to another joint
=====================
*/
void idAnimator::CollapseJoint ( jointHandle_t jointnum, jointHandle_t collapseTo ) {
	jointMod_t *jointMod;
	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	jointMod = FindJointMod ( jointnum );
		
	jointMod->transform_pos = JOINTMOD_COLLAPSE;
	jointMod->transform_axis = JOINTMOD_WORLD_OVERRIDE;
	jointMod->mat.Zero ( );
	jointMod->collapseJoint = collapseTo;
}

/*
=====================
idAnimator::CollapseJoints

Add a collapse joint modification for each joint listed in "jointnames" to the given joint "coppaseJoint"
=====================
*/
void idAnimator::CollapseJoints ( const char *jointnames, jointHandle_t collpaseJoint ) {
	idList<jointHandle_t> jointList;
	int					  i;

	GetJointList( jointnames, jointList );
	
	for ( i = 0; i < jointList.Num(); i ++ ) {
		CollapseJoint ( jointList[i], collpaseJoint );
	}
}

// RAVEN BEGIN
// bdube: added more programmer controlled joint methods
/*
=====================
idAnimator::AimJointAt
=====================
*/
void idAnimator::AimJointAt ( jointHandle_t jointnum, const idVec3& pos, const int currentTime ) {
	jointMod_t *jointMod;

	jointMod = FindJointMod ( jointnum );	

	idVec3	offset;
	idVec3	origin;
	idMat3	axis;
	idVec3	dir;
	idVec3	localDir;

	GetJointTransform ( jointnum, currentTime, offset, axis );

	origin = entity->GetPhysics()->GetOrigin() + offset * entity->GetPhysics()->GetAxis();
			
	dir = pos - origin;
	dir.NormalizeFast ( );
	entity->GetPhysics()->GetAxis().ProjectVector ( dir, localDir );
		
	jointMod->transform_axis = JOINTMOD_WORLD_OVERRIDE;
	jointMod->mat = localDir.ToMat3();

	entity->BecomeActive( TH_ANIMATE );
	ForceUpdate();
}

/*
=====================
idAnimator::SetJointAngularVelocity
=====================
*/
void idAnimator::SetJointAngularVelocity ( jointHandle_t jointnum, const idAngles& vel, const int currentTime, const int blendTime ) {
	jointMod_t *jointMod;

	jointMod = FindJointMod ( jointnum );

	if ( !jointMod->lastTime ) {
		jointMod->lastTime = currentTime;
		jointMod->mat = idAngles(0,0,0).ToMat3();
	}

	jointMod->transform_axis = JOINTMOD_LOCAL;

	if ( blendTime <= 0 ) {
		jointMod->angularVelocity.Init ( currentTime, 0, 0, 1, jointMod->angularVelocity.GetCurrentValue(currentTime), vel );	
	} else {
		jointMod->angularVelocity.Init ( currentTime, blendTime/2, blendTime/2, blendTime, jointMod->angularVelocity.GetCurrentValue(currentTime), vel );	
	}

	entity->BecomeActive( TH_ANIMATE );
	ForceUpdate();
}

/*
=====================
idAnimator::GetJointAngularVelocity
=====================
*/
idAngles idAnimator::GetJointAngularVelocity ( jointHandle_t jointnum, const int currentTime ) {
	jointMod_t *jointMod = FindJointMod ( jointnum );

	return jointMod->angularVelocity.GetCurrentValue( currentTime );
}

/*
=====================
idAnimator::GetJointAngularVelocity
=====================
*/
void idAnimator::ClearJointAngularVelocity ( jointHandle_t jointnum ) {
	jointMod_t *jointMod = FindJointMod ( jointnum );

	jointMod->angularVelocity.Init( 0, 0, 0, 0, jointMod->angularVelocity.GetStartValue() - jointMod->angularVelocity.GetStartValue(), jointMod->angularVelocity.GetStartValue() - jointMod->angularVelocity.GetStartValue() );
}
// RAVEN END

/*
=====================
idAnimator::ClearJoint
=====================
*/
void idAnimator::ClearJoint( jointHandle_t jointnum ) {
	int i;

	if ( !modelDef || !modelDef->ModelHandle() || ( jointnum < 0 ) || ( jointnum >= numJoints ) ) {
		return;
	}

	for( i = 0; i < jointMods.Num(); i++ ) {
		if ( jointMods[ i ]->jointnum == jointnum ) {
			delete jointMods[ i ];
			jointMods.RemoveIndex( i );
			ForceUpdate();
			break;
		} else if ( jointMods[ i ]->jointnum > jointnum ) {
			break;
		}
	}
}

/*
=====================
idAnimator::ClearAllJoints
=====================
*/
void idAnimator::ClearAllJoints( void ) {
	if ( jointMods.Num() ) {
		ForceUpdate();
	}
	jointMods.DeleteContents( true );
}

/*
=====================
idAnimator::ClearAllAnims
=====================
*/
void idAnimator::ClearAllAnims( int currentTime, int cleartime ) {
	int	i;

	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		Clear( i, currentTime, cleartime );
	}

	ClearAFPose();
	ForceUpdate();
}

/*
====================
idAnimator::GetDelta
====================
*/
void idAnimator::GetDelta( int fromtime, int totime, idVec3 &delta ) const {
	int					i;
	const idAnimBlend	*blend;
	float				blendWeight;

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		delta.Zero();
		return;
	}

	delta.Zero();
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendDelta( fromtime, totime, delta, blendWeight );
	}

	if ( modelDef->Joints()[ 0 ].channel ) {
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendDelta( fromtime, totime, delta, blendWeight );
		}
	}
}

/*
====================
idAnimator::GetDeltaRotation
====================
*/
bool idAnimator::GetDeltaRotation( int fromtime, int totime, idMat3 &delta ) const {
	int					i;
	const idAnimBlend	*blend;
	float				blendWeight;
	idQuat				q;

	if ( !modelDef || !modelDef->ModelHandle() || ( fromtime == totime ) ) {
		delta.Identity();
		return false;
	}

	q.Set( 0.0f, 0.0f, 0.0f, 1.0f );
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendDeltaRotation( blend->starttime + fromtime, totime, q, blendWeight );
	}

	if ( modelDef->Joints()[ 0 ].channel ) {
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendDeltaRotation( blend->starttime + fromtime, totime, q, blendWeight );
		}
	}

	if ( blendWeight > 0.0f ) {
		delta = q.ToMat3();
		return true;
	} else {
		delta.Identity();
		return false;
	}
}

/*
====================
idAnimator::GetOrigin
====================
*/
void idAnimator::GetOrigin( int currentTime, idVec3 &pos ) const {
	int					i;
	const idAnimBlend	*blend;
	float				blendWeight;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		pos.Zero();
		return;
	}

	pos.Zero();
	blendWeight = 0.0f;

	blend = channels[ ANIMCHANNEL_ALL ];
	for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
		blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
	}

	if ( modelDef->Joints()[ 0 ].channel ) {
		blend = channels[ modelDef->Joints()[ 0 ].channel ];
		for( i = 0; i < ANIM_MaxAnimsPerChannel; i++, blend++ ) {
			blend->BlendOrigin( currentTime, pos, blendWeight, removeOriginOffset );
		}
	}

	pos += modelDef->GetVisualOffset();
}

/*
====================
idAnimator::GetBounds
====================
*/
bool idAnimator::GetBounds( int currentTime, idBounds &bounds ) {
	int					i, j;
	const idAnimBlend	*blend;
	int					count;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	if ( AFPoseJoints.Num() ) {
		bounds = AFPoseBounds;
		count = 1;
	} else {
		bounds.Clear();
		count = 0;
	}

	blend = channels[ 0 ];
	for( i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->AddBounds( currentTime, bounds, removeOriginOffset ) ) {
				count++;
			}
		}
	}

	if ( !count ) {
		if ( !frameBounds.IsCleared() ) {
			bounds = frameBounds;
			return true;
		} else {
			bounds.Zero();
			return false;
		}
	}

	bounds.TranslateSelf( modelDef->GetVisualOffset() );
// RAVEN BEGIN
// jscott: HACK! The bounds only get the range of the skeleton, so add in the distance between the highest bone and the top of the head
	bounds[1][2] += 4.0f;
// RAVEN END

	if ( g_debugBounds.GetBool() ) {
		if ( bounds[1][0] - bounds[0][0] > 2048 || bounds[1][1] - bounds[0][1] > 2048 ) {
			if ( entity ) {
				gameLocal.Warning( "big frameBounds on entity '%s' with model '%s': %f,%f", entity->name.c_str(), modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			} else {
				gameLocal.Warning( "big frameBounds on model '%s': %f,%f", modelDef->ModelHandle()->Name(), bounds[1][0] - bounds[0][0], bounds[1][1] - bounds[0][1] );
			}
		}
	}

	frameBounds = bounds;

	return true;
}

/*
=====================
idAnimator::InitAFPose
=====================
*/
void idAnimator::InitAFPose( void ) {

	if ( !modelDef ) {
		return;
	}

	AFPoseJoints.SetNum( modelDef->Joints().Num(), false );
	AFPoseJoints.SetNum( 0, false );
	AFPoseJointMods.SetNum( modelDef->Joints().Num(), false );

	if ( AFPoseJointFrame == NULL || AFPoseJointFrameSize != modelDef->Joints().Num() ) {
		Mem_Free16( AFPoseJointFrame );
		AFPoseJointFrame = (idJointQuat *) Mem_Alloc16( modelDef->Joints().Num() * sizeof( AFPoseJointFrame[0] ), MA_ANIM );
		AFPoseJointFrameSize = modelDef->Joints().Num();
	}
}

/*
=====================
idAnimator::SetAFPoseJointMod
=====================
*/
void idAnimator::SetAFPoseJointMod( const jointHandle_t jointNum, const AFJointModType_t mod, const idMat3 &axis, const idVec3 &origin ) {
	AFPoseJointMods[jointNum].mod = mod;
	AFPoseJointMods[jointNum].axis = axis;
	AFPoseJointMods[jointNum].origin = origin;

	int index = idBinSearch_GreaterEqual<int>( AFPoseJoints.Ptr(), AFPoseJoints.Num(), jointNum );
	if ( index >= AFPoseJoints.Num() || jointNum != AFPoseJoints[index] ) {
		AFPoseJoints.Insert( jointNum, index );
	}
}

/*
=====================
idAnimator::FinishAFPose
=====================
*/
void idAnimator::FinishAFPose( int animNum, const idBounds &bounds, const int time ) {
	int					i, j;
	int					numJoints;
	int					parentNum;
	int					jointMod;
	int					jointNum;
	const int *			jointParent;

	if ( !modelDef ) {
		return;
	}
	
	const idAnim *anim = modelDef->GetAnim( animNum );
	if ( !anim ) {
		return;
	}

	numJoints = modelDef->Joints().Num();
	if ( !numJoints ) {
		return;
	}

	idRenderModel		*md5 = modelDef->ModelHandle();
	const idMD5Anim		*md5anim = anim->MD5Anim( 0 );

	if ( numJoints != md5anim->NumJoints() ) {
// RAVEN BEGIN
// scork: reports the actual joint # mismatch as well
		gameLocal.Warning( "Model '%s' has different # of joints (%d) than anim '%s' (%d)", md5->Name(), numJoints, md5anim->Name(), md5anim->NumJoints() );
// RAVEN END
		return;
	}

	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	md5anim->GetSingleFrame( 0, jointFrame, modelDef->GetChannelJoints( ANIMCHANNEL_ALL ), modelDef->NumJointsOnChannel( ANIMCHANNEL_ALL ) );

	if ( removeOriginOffset ) {
#ifdef VELOCITY_MOVE
		jointFrame[ 0 ].t.x = 0.0f;
#else
		jointFrame[ 0 ].t.Zero();
#endif
	}

	idJointMat *joints = ( idJointMat * )_alloca16( numJoints * sizeof( *joints ) );

	// convert the joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( AFPoseJoints.Num() && AFPoseJoints[0] == 0 ) {
		switch( AFPoseJointMods[0].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[0].SetRotation( AFPoseJointMods[0].axis );
				joints[0].SetTranslation( AFPoseJointMods[0].origin );
				break;
			}
		}
		j = 1;
	} else {
		j = 0;
	}

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// transform the child joints
	for( i = 1; j < AFPoseJoints.Num(); j++, i++ ) {
		jointMod = AFPoseJoints[j];

		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod - 1 );
		i = jointMod;

		parentNum = jointParent[i];

		switch( AFPoseJointMods[jointMod].mod ) {
			case AF_JOINTMOD_AXIS: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;
			}
			case AF_JOINTMOD_ORIGIN: {
				joints[i].SetRotation( joints[i].ToMat3() * joints[parentNum].ToMat3() );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
			case AF_JOINTMOD_BOTH: {
				joints[i].SetRotation( AFPoseJointMods[jointMod].axis );
				joints[i].SetTranslation( AFPoseJointMods[jointMod].origin );
				break;
			}
		}
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	// untransform hierarchy
	SIMDProcessor->UntransformJoints( joints, jointParent, 1, numJoints - 1 );

	// convert joint matrices back to joint quaternions
	SIMDProcessor->ConvertJointMatsToJointQuats( AFPoseJointFrame, joints, numJoints );

	// find all modified joints and their parents
	bool *blendJoints = (bool *) _alloca16( numJoints * sizeof( bool ) );
	memset( blendJoints, 0, numJoints * sizeof( bool ) );

	// mark all modified joints and their parents
	for( i = 0; i < AFPoseJoints.Num(); i++ ) {
		for( jointNum = AFPoseJoints[i]; jointNum != INVALID_JOINT; jointNum = jointParent[jointNum] ) {
			blendJoints[jointNum] = true;
		}
	}

	// lock all parents of modified joints
	AFPoseJoints.SetNum( 0, false );
	for ( i = 0; i < numJoints; i++ ) {
		if ( blendJoints[i] ) {
			AFPoseJoints.Append( i );
		}
	}

	AFPoseBounds = bounds;
	AFPoseTime = time;

	ForceUpdate();
}

/*
=====================
idAnimator::SetAFPoseBlendWeight
=====================
*/
void idAnimator::SetAFPoseBlendWeight( float blendWeight ) {
	AFPoseBlendWeight = blendWeight;
}

/*
=====================
idAnimator::BlendAFPose
=====================
*/
bool idAnimator::BlendAFPose( idJointQuat *blendFrame ) const {

	if ( !AFPoseJoints.Num() ) {
		return false;
	}

	SIMDProcessor->BlendJoints( blendFrame, AFPoseJointFrame, AFPoseBlendWeight, AFPoseJoints.Ptr(), AFPoseJoints.Num() );

	return true;
}

/*
=====================
idAnimator::ClearAFPose
=====================
*/
void idAnimator::ClearAFPose( void ) {
	if ( AFPoseJoints.Num() ) {
		ForceUpdate();
	}
	AFPoseBlendWeight = 1.0f;
	AFPoseJoints.SetNum( 0, false );
	AFPoseBounds.Clear();
	AFPoseTime = 0;
}

/*
=====================
idAnimator::ServiceAnims
=====================
*/
void idAnimator::ServiceAnims( int fromtime, int totime ) {
	int			i, j;
	idAnimBlend	*blend;

	if ( !modelDef ) {
		return;
	}

	if ( modelDef->ModelHandle() ) {
		blend = channels[ 0 ];
		for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
			for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
				blend->CallFrameCommands( entity, fromtime, totime );
			}
		}
	}

	if ( !IsAnimating( totime ) ) {
		stoppedAnimatingUpdate = true;
		if ( entity ) {
			entity->BecomeInactive( TH_ANIMATE );

			// present one more time with stopped animations so the renderer can properly recreate interactions
			entity->BecomeActive( TH_UPDATEVISUALS );
		}
	}
}

// RAVEN BEGIN
// rjohnson: added flag to ignore AF when checking for animation
/*
=====================
idAnimator::IsAnimating
=====================
*/
bool idAnimator::IsAnimating( int currentTime, bool IgnoreAF ) const {
	int					i, j;
	const idAnimBlend	*blend;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( !IgnoreAF && AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( !blend->IsDone( currentTime ) ) {
				return true;
			}
		}
	}

	return false;
}
// RAVEN END

/*
=====================
idAnimator::FrameHasChanged
=====================
*/
bool idAnimator::FrameHasChanged( int currentTime ) const {
	int					i, j;
	const idAnimBlend	*blend;

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

	// if animating with an articulated figure
	if ( AFPoseJoints.Num() && currentTime <= AFPoseTime ) {
		return true;
	}

	blend = channels[ 0 ];
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->FrameHasChanged( currentTime ) ) {
				return true;
			}
		}
	}

	if ( forceUpdate && IsAnimating( currentTime ) ) {
		return true;
	}

	return false;
}

/*
=====================
idAnimator::CreateFrame
=====================
*/
bool idAnimator::CreateFrame( int currentTime, bool force ) {
	int					i, j;
	int					numJoints;
	int					parentNum;
	bool				hasAnim;
	bool				debugInfo;
	float				baseBlend;
	float				blendWeight;
	const idAnimBlend *	blend;
	const int *			jointParent;
// RAVEN BEGIN
// bdube: removed const
	jointMod_t *		jointMod;
// RAVEN END	
	const idJointQuat *	defaultPose;

	static idCVar		r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only, 3 = draw joints only", 0, 3, idCmdSystem::ArgCompletion_Integer<0,3> );

	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	if ( !modelDef || !modelDef->ModelHandle() ) {
		return false;
	}

// RAVEN BEGIN
// rjohnson: always check the time transform except force.  Even when skeleton is on
	if ( !force ) {
		if ( lastTransformTime == currentTime ) {
			return false;
		}
	}

	if ( !force && !r_showSkel.GetInteger() ) {
		if ( lastTransformTime != -1 && !stoppedAnimatingUpdate && !IsAnimating( currentTime ) ) {
			return false;
		}
	}

#if 0
	if ( !gameLocal.isLastPredictFrame ) {
		return false;
	}
#endif
// RAVEN END

	lastTransformTime = currentTime;
	stoppedAnimatingUpdate = false;

	if ( entity && ( ( g_debugAnim.GetInteger() == entity->entityNumber ) || ( g_debugAnim.GetInteger() == -2 ) ) ) {
		debugInfo = true;
		gameLocal.Printf( "---------------\n%d: entity '%s':\n", gameLocal.time, entity->GetName() );
 		gameLocal.Printf( "model '%s':\n", modelDef->GetModelName() );
	} else {
		debugInfo = false;
	}

	// init the joint buffer
	if ( AFPoseJoints.Num() ) {
		// initialize with AF pose anim for the case where there are no other animations and no AF pose joint modifications
		defaultPose = AFPoseJointFrame;
	} else {
		defaultPose = modelDef->GetDefaultPose();
	}

	if ( !defaultPose ) {
		//gameLocal.Warning( "idAnimator::CreateFrame: no defaultPose on '%s'", modelDef->Name() );
		return false;
	}

	numJoints = modelDef->Joints().Num();
	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( jointFrame[0] ) );
	SIMDProcessor->Memcpy( jointFrame, defaultPose, numJoints * sizeof( jointFrame[0] ) );

	hasAnim = false;

	// blend the all channel
	baseBlend = 0.0f;
	blend = channels[ ANIMCHANNEL_ALL ];
	for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
		if ( blend->BlendAnim( currentTime, ANIMCHANNEL_ALL, numJoints, jointFrame, baseBlend, removeOriginOffset, false, debugInfo ) ) {
			hasAnim = true;
			if ( baseBlend >= 1.0f ) {
				break;
			}
		}
	}

	// only blend other channels if there's enough space to blend into
	if ( baseBlend < 1.0f ) {
		for( i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
			if ( !modelDef->NumJointsOnChannel( i ) ) {
				continue;
			}
			if ( i == ANIMCHANNEL_EYELIDS ) {
				// eyelids blend over any previous anims, so skip it and blend it later
				continue;
			}
			blendWeight = baseBlend;
			blend = channels[ i ];
			for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
				if ( blend->BlendAnim( currentTime, i, numJoints, jointFrame, blendWeight, removeOriginOffset, false, debugInfo ) ) {
					hasAnim = true;
					if ( blendWeight >= 1.0f ) {
						// fully blended
						break;
					}
				}
			}

			if ( debugInfo && !AFPoseJoints.Num() && !blendWeight ) {
				gameLocal.Printf( "%d: %s using default pose in model '%s'\n", gameLocal.time, channelNames[ i ], modelDef->GetModelName() );
			}
		}
	}

	// blend in the eyelids
	if ( modelDef->NumJointsOnChannel( ANIMCHANNEL_EYELIDS ) ) {
		blend = channels[ ANIMCHANNEL_EYELIDS ];
		blendWeight = baseBlend;
		for( j = 0; j < ANIM_MaxAnimsPerChannel; j++, blend++ ) {
			if ( blend->BlendAnim( currentTime, ANIMCHANNEL_EYELIDS, numJoints, jointFrame, blendWeight, removeOriginOffset, true, debugInfo ) ) {
				hasAnim = true;
				if ( blendWeight >= 1.0f ) {
					// fully blended
					break;
				}
			}
		}
	}

	// blend the articulated figure pose
	if ( BlendAFPose( jointFrame ) ) {
		hasAnim = true;
	}

	if ( !hasAnim && !jointMods.Num() ) {
		// no animations were updated
		return false;
	}

	// convert the joint quaternions to rotation matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// check if we need to modify the origin
	if ( jointMods.Num() && ( jointMods[0]->jointnum == 0 ) ) {
		jointMod = jointMods[0];

		switch( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetRotation( jointMod->mat * joints[0].ToMat3() );
				break;
			
			case JOINTMOD_WORLD:
				joints[0].SetRotation( joints[0].ToMat3() * jointMod->mat );
				break;

			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetRotation( jointMod->mat );
				break;				
		}

		switch( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				break;

			case JOINTMOD_LOCAL:
				joints[0].SetTranslation( joints[0].ToVec3() + jointMod->pos );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
			case JOINTMOD_WORLD:
			case JOINTMOD_WORLD_OVERRIDE:
				joints[0].SetTranslation( jointMod->pos );
				break;			
		}
		j = 1;
	} else {
		j = 0;
	}

	// add in the model offset
	joints[0].SetTranslation( joints[0].ToVec3() + modelDef->GetVisualOffset() );

	// pointer to joint info
	jointParent = modelDef->JointParents();

	// add in any joint modifications
	for( i = 1; j < jointMods.Num(); j++, i++ ) {
		jointMod = jointMods[j];

		// transform any joints preceding the joint modifier
		SIMDProcessor->TransformJoints( joints, jointParent, i, jointMod->jointnum - 1 );
		i = jointMod->jointnum;

		parentNum = jointParent[i];

// RAVEN BEGIN
// bdube: angular velocity on joints
		if ( jointMod->angularVelocity.GetStartTime ( ) ) {		
			idAngles angles;
			angles = jointMod->angularVelocity.GetCurrentValue ( currentTime ) * MS2SEC(currentTime-jointMod->lastTime);
			angles.Normalize360 ( );						
			jointMod->mat *= angles.ToMat3 ( );
		}
// RAVEN END

		// modify the axis
		switch( jointMod->transform_axis ) {
			case JOINTMOD_NONE:
				joints[i].SetRotation( joints[i].ToMat3() * joints[ parentNum ].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetRotation( jointMod->mat * ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetRotation( jointMod->mat * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetRotation( ( joints[i].ToMat3() * joints[parentNum].ToMat3() ) * jointMod->mat );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetRotation( jointMod->mat );
				break;		
		}

		// modify the position
		switch( jointMod->transform_pos ) {
			case JOINTMOD_NONE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_LOCAL:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + ( joints[i].ToVec3() + jointMod->pos ) * joints[parentNum].ToMat3() );
				break;
			
			case JOINTMOD_LOCAL_OVERRIDE:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + jointMod->pos * joints[parentNum].ToMat3() );
				break;

			case JOINTMOD_WORLD:
				joints[i].SetTranslation( joints[parentNum].ToVec3() + joints[i].ToVec3() * joints[parentNum].ToMat3() + jointMod->pos );
				break;

			case JOINTMOD_WORLD_OVERRIDE:
				joints[i].SetTranslation( jointMod->pos );
				break;

			case JOINTMOD_COLLAPSE:
				joints[i].SetTranslation( joints[jointMod->collapseJoint].ToVec3() );
				break;
		}

// RAVEN BEGIN
// bdube: more joint control
		jointMod->lastTime = currentTime;
// RAVEN END		
	}

	// transform the rest of the hierarchy
	SIMDProcessor->TransformJoints( joints, jointParent, i, numJoints - 1 );

	return true;
}

/*
=====================
idAnimator::ForceUpdate
=====================
*/
void idAnimator::ForceUpdate( void ) {
	lastTransformTime = -1;
	forceUpdate = true;
}

/*
=====================
idAnimator::ClearForceUpdate
=====================
*/
void idAnimator::ClearForceUpdate( void ) {
	forceUpdate = false;
}

/*
=====================
idAnimator::GetJointTransform

=====================
*/
bool idAnimator::GetJointTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !modelDef || ( jointHandle < 0 ) || ( jointHandle >= modelDef->NumJoints() ) ) {
		return false;
	}
	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = entity->GetPhysics()->GetCenterMass()-entity->GetPhysics()->GetOrigin();
		axis = entity->GetRenderEntity()->axis;
		return true;
	}

	CreateFrame( currentTime, false );

	offset = joints[ jointHandle ].ToVec3();
	axis = joints[ jointHandle ].ToMat3();

	return true;
}

/*
=====================
idAnimator::GetJointLocalTransform
=====================
*/
bool idAnimator::GetJointLocalTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !modelDef ) {
		return false;
	}

	const idList<jointInfo_t> &modelJoints = modelDef->Joints();

	if ( ( jointHandle < 0 ) || ( jointHandle >= modelJoints.Num() ) ) {
		return false;
	}

	if ( g_perfTest_noJointTransform.GetBool() ) {
		offset = entity->GetPhysics()->GetCenterMass()-entity->GetPhysics()->GetOrigin();
		axis = entity->GetRenderEntity()->axis;
		return true;
	}

	// FIXME: overkill
	CreateFrame( currentTime, false );

	if ( jointHandle > 0 ) {
		idJointMat m = joints[ jointHandle ];
		m /= joints[ modelJoints[ jointHandle ].parentNum ];
		offset = m.ToVec3();
		axis = m.ToMat3();
	} else {
		offset = joints[ jointHandle ].ToVec3();
		axis = joints[ jointHandle ].ToMat3();
	}

	return true;
}

/*
=====================
idAnimator::GetJointHandle
=====================
*/
jointHandle_t idAnimator::GetJointHandle( const char *name ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return INVALID_JOINT;
	}

	return modelDef->ModelHandle()->GetJointHandle( name );
}

/*
=====================
idAnimator::GetJointName
=====================
*/
const char *idAnimator::GetJointName( jointHandle_t handle ) const {
	if ( !modelDef || !modelDef->ModelHandle() ) {
		return "";
	}

	return modelDef->ModelHandle()->GetJointName( handle );
}

/*
=====================
idAnimator::GetChannelForJoint
=====================
*/
int idAnimator::GetChannelForJoint( jointHandle_t joint ) const {
	if ( !modelDef ) {
		gameLocal.Error( "idAnimator::GetChannelForJoint: NULL model" );
	}

	if ( ( joint < 0 ) || ( joint >= numJoints ) ) {
		gameLocal.Error( "idAnimator::GetChannelForJoint: invalid joint num (%d)", joint );
	}

	return modelDef->GetJoint( joint )->channel;
}

/*
=====================
idAnimator::GetFirstChild
=====================
*/
jointHandle_t idAnimator::GetFirstChild( const char *name ) const {
	return GetFirstChild( GetJointHandle( name ) );
}

/*
=====================
idAnimator::GetFirstChild
=====================
*/
jointHandle_t idAnimator::GetFirstChild( jointHandle_t jointnum ) const {
	int					i;
	int					num;
	const jointInfo_t	*joint;

	if ( !modelDef ) {
		return INVALID_JOINT;
	}

	num = modelDef->NumJoints();
	if ( !num ) {
		return jointnum;
	}
	joint = modelDef->GetJoint( 0 );
	for( i = 0; i < num; i++, joint++ ) {
		if ( joint->parentNum == jointnum ) {
			return ( jointHandle_t )joint->num;
		}
	}
	return jointnum;
}

/*
=====================
idAnimator::GetJoints
=====================
*/
void idAnimator::GetJoints( int *numJoints, idJointMat **jointsPtr ) {
	*numJoints	= this->numJoints;
	*jointsPtr	= this->joints;
}

/*
=====================
idAnimator::GetAnimFlags
=====================
*/
const animFlags_t idAnimator::GetAnimFlags( int animNum ) const {
	animFlags_t result;

	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->GetAnimFlags();
	}

	memset( &result, 0, sizeof( result ) );
	return result;
}

/*
=====================
idAnimator::IsBlending

Returns true if the given channel number is currently blending more than one animation
=====================
*/
bool idAnimator::IsBlending ( int channel, int currentTime ) const {
	if ( ( channel < 0 ) || ( channel >= ANIM_NumAnimChannels ) ) {
		gameLocal.Error( "idAnimator::GetAnimCount : channel (%d) out of range", channel );
	}

	int i;
	for( i = 1; i < ANIM_MaxAnimsPerChannel; i++ ) {
		if ( !channels[ channel ][ i ].IsDone ( currentTime ) ) {
			return true;
		}
	}
	
	return false;
}	

/*
=====================
idAnimator::NumFrames
=====================
*/
int	idAnimator::NumFrames( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumFrames();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::NumSyncedAnims
=====================
*/
int	idAnimator::NumSyncedAnims( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->NumAnims();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::AnimName
=====================
*/
const char *idAnimator::AnimName( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Name();
	} else {
		return "";
	}
}

/*
=====================
idAnimator::AnimFullName
=====================
*/
const char *idAnimator::AnimFullName( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->FullName();
	} else {
		return "";
	}
}

// RAVEN BEGIN
// rjohnson: more output for animators
/*
=====================
idAnimator::AnimMD5Name
=====================
*/
const char *idAnimator::AnimMD5Name( int animNum, int index ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		const idMD5Anim	*md5Anim = anim->MD5Anim( index );
		
		if ( md5Anim ) {
			return md5Anim->Name();
		} else {
			return "";
		}
	} else {
		return "";
	}
}
// RAVEN END

/*
=====================
idAnimator::AnimLength
=====================
*/
int	idAnimator::AnimLength( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->Length();
	} else {
		return 0;
	}
}

/*
=====================
idAnimator::TotalMovementDelta
=====================
*/
const idVec3 &idAnimator::TotalMovementDelta( int animNum ) const {
	const idAnim *anim = GetAnim( animNum );
	if ( anim ) {
		return anim->TotalMovementDelta();
	} else {
		return vec3_origin;
	}
}

// RAVEN BEGIN
// nrausch: added func
/*
=====================
idAnimator::GetNearestJoint
=====================
*/
jointHandle_t idAnimator::GetNearestJoint( const idVec3 &start, const idVec3 &end, int currentTime, jointHandle_t *jointList, int cnt ) {
	
	int numJoints = 0;
	idJointMat *joints = 0;
	GetJoints( &numJoints, &joints );
	
	jointHandle_t closestJoint = INVALID_JOINT;
	float closest = 0.0f;
	
	idVec3 a = end - start;
	a.Normalize();

	for ( int i = 0; i < cnt; ++i ) {
		
		// If we specified a null jointlist, just run through each joint
		jointHandle_t jointNum = (jointHandle_t)i;
		if ( jointList ) {
			jointNum = jointList[i];
		}
		
		if ( jointNum != INVALID_JOINT ) {
			idVec3 jointPos;
			idMat3 jointMat;
			
			GetJointTransform( jointNum, currentTime, jointPos, jointMat );
			
			idVec3 b = jointPos - start;
			b.Normalize();
			
			float newDot = DotProduct(a, b);
			
			if ( newDot > closest ) {
				closest = newDot;
				closestJoint = jointNum;
			}
		}
	}

	return closestJoint;
}
// RAVEN END

/***********************************************************************

	Util functions

***********************************************************************/

/*
=====================
ANIM_GetModelDefFromEntityDef
=====================
*/
const idDeclModelDef *ANIM_GetModelDefFromEntityDef( const idDict *args ) {
	const idDeclModelDef *modelDef;

	idStr name = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef && modelDef->ModelHandle() ) {
		return modelDef;
	}

	return NULL;
}

/*
=====================
idGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromEntityDef( const idDict *args ) {
	idRenderModel *model;
	const idDeclModelDef *modelDef;

	model = NULL;

	idStr name = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}

	if ( !model ) {
		model = renderModelManager->FindModel( name );
	}

	if ( model && model->IsDefaultModel() ) {
		return NULL;
	}

	return model;
}

/*
=====================
idGameEdit::ANIM_GetModelFromEntityDef
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromEntityDef( const char *classname ) {
	const idDict *args;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	return ANIM_GetModelFromEntityDef( args );
}

/*
=====================
idGameEdit::ANIM_GetModelOffsetFromEntityDef
=====================
*/
const idVec3 &idGameEdit::ANIM_GetModelOffsetFromEntityDef( const char *classname ) {
	const idDict *args;
	const idDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return vec3_origin;
	}

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( !modelDef ) {
		return vec3_origin;
	}

	return modelDef->GetVisualOffset();
}

/*
=====================
idGameEdit::ANIM_GetModelFromName
=====================
*/
idRenderModel *idGameEdit::ANIM_GetModelFromName( const char *modelName ) {
	const idDeclModelDef *modelDef;
	idRenderModel *model;

	model = NULL;
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelName, false ) );
	if ( modelDef ) {
		model = modelDef->ModelHandle();
	}
	if ( !model ) {
		model = renderModelManager->FindModel( modelName );
	}
	return model;
}

/*
=====================
idGameEdit::ANIM_GetAnimFromEntityDef
=====================
*/
const idMD5Anim *idGameEdit::ANIM_GetAnimFromEntityDef( const char *classname, const char *animname ) {
	const idDict *args;
	const idMD5Anim *md5anim;
	const idAnim *anim;
	int	animNum;
	const char	*modelname;
	const idDeclModelDef *modelDef;

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	md5anim = NULL;
	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( animNum ) {
			anim = modelDef->GetAnim( animNum );
			if ( anim ) {
				md5anim = anim->MD5Anim( 0 );
			}
		}
	}
	return md5anim;
}

// RAVEN BEGIN
// bdube: added
/*
=====================
idGameEdit::ANIM_GetAnimFromEntity
=====================
*/
// scork: const-qualified 'ent'
const idMD5Anim *idGameEdit::ANIM_GetAnimFromEntity( const idEntity* ent, int animNum ) {
	const idAnim * anim;

	anim = ent->GetAnimator ( )->GetAnim ( animNum );
	if ( !anim ) {
		return NULL;
	}
	
	return anim->MD5Anim(0);
}

/*
=====================
idGameEdit::ANIM_GetAnimPlaybackRateFromEntity
=====================
*/
float idGameEdit::ANIM_GetAnimPlaybackRateFromEntity ( idEntity* ent, int animNum ) {
	const idAnim * anim;

	anim = ent->GetAnimator ( )->GetAnim ( animNum );
	if ( !anim ) {
		return 1.0f;
	}
	
	return anim->GetPlaybackRate();	
}

/*
=====================
idGameEdit::ANIM_GetAnimNameFromEntity
=====================
*/
// scork: const-qualified 'ent'
const char* idGameEdit::ANIM_GetAnimNameFromEntity ( const idEntity* ent, int animNum ) {
	const idAnim * anim;

	anim = ent->GetAnimator ( )->GetAnim ( animNum );
	if ( !anim ) {
		return "";
	}
	
	return anim->FullName();
}
// RAVEN END

/*
=====================
idGameEdit::ANIM_GetNumAnimsFromEntityDef
=====================
*/
int idGameEdit::ANIM_GetNumAnimsFromEntityDef( const idDict *args ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		return modelDef->NumAnims();
	}
	return 0;
}

/*
=====================
idGameEdit::ANIM_GetAnimNameFromEntityDef
=====================
*/
const char *idGameEdit::ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum ) {
	const char *modelname;
	const idDeclModelDef *modelDef;

	modelname = args->GetString( "model" );
	modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, modelname, false ) );
	if ( modelDef ) {
		const idAnim* anim = modelDef->GetAnim( animNum );
		if ( anim ) {
			return anim->FullName();
		}
	}
	return "";
}

/*
=====================
idGameEdit::ANIM_GetAnim
=====================
*/
const idMD5Anim *idGameEdit::ANIM_GetAnim( const char *fileName ) {
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
	return animationLib->GetAnim( fileName );
// RAVEN END
}

/*
=====================
idGameEdit::ANIM_GetLength
=====================
*/
int	idGameEdit::ANIM_GetLength( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->Length();
}

/*
=====================
idGameEdit::ANIM_GetNumFrames
=====================
*/
int idGameEdit::ANIM_GetNumFrames( const idMD5Anim *anim ) {
	if ( !anim ) {
		return 0;
	}
	return anim->NumFrames();
}

// RAVEN BEGIN
// bdube: added
/*
=====================
idGameEdit::ANIM_GetFilename
=====================
*/
const char * idGameEdit::ANIM_GetFilename( const idMD5Anim* anim ) {
	return anim->Name();
}

/*
=====================
idGameEdit::ANIM_ConvertFrameToTime
=====================
*/
int idGameEdit::ANIM_ConvertFrameToTime ( const idMD5Anim* anim, int frame ) {
	frameBlend_t fb;
	fb.frame1 = frame;
	fb.frame2 = frame;
	return anim->ConvertFrameToTime ( fb );
}

/*
=====================
idGameEdit::ANIM_ConvertTimeToFrame
=====================
*/
int idGameEdit::ANIM_ConvertTimeToFrame ( const idMD5Anim* anim, int time ) {
	frameBlend_t fb;
	anim->ConvertTimeToFrame ( time, 0, fb );
	return fb.frame1;
}

// RAVEN END

/*
=====================
idGameEdit::ANIM_CreateAnimFrame
=====================
*/
void idGameEdit::ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *joints, int time, const idVec3 &offset, bool remove_origin_offset ) {
	int					i;
	frameBlend_t		frame;
	const idMD5Joint	*md5joints;
	int					*index;

	if ( !model || model->IsDefaultModel() || !anim ) {
		return;
	}

	if ( numJoints != model->NumJoints() ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: different # of joints in renderEntity_t (%d) than in model (%s)(%d)", model->NumJoints(), model->Name(), numJoints );
	}

	if ( !model->NumJoints() ) {
		// FIXME: Print out a warning?
		return;
	}

	if ( !joints ) {
		gameLocal.Error( "ANIM_CreateAnimFrame: NULL joint frame pointer on model (%s)", model->Name() );
	}

	if ( numJoints != anim->NumJoints() ) {
// RAVEN BEGIN
// scork: reports the actual joint # mismatch as well
		gameLocal.Warning( "Model '%s' has different # of joints (%d) than anim '%s' (%d)", model->Name(), numJoints, anim->Name(), anim->NumJoints() );
// RAVEN END
		for( i = 0; i < numJoints; i++ ) {
			joints[i].SetRotation( mat3_identity );
			joints[i].SetTranslation( offset );
		}
		return;
	}

	// create index for all joints
	index = ( int * )_alloca16( numJoints * sizeof( int ) );
	for ( i = 0; i < numJoints; i++ ) {
		index[i] = i;
	}

	// create the frame
	anim->ConvertTimeToFrame( time, 1, frame );
	idJointQuat *jointFrame = ( idJointQuat * )_alloca16( numJoints * sizeof( *jointFrame ) );
	anim->GetInterpolatedFrame( frame, jointFrame, index, numJoints );

	// convert joint quaternions to joint matrices
	SIMDProcessor->ConvertJointQuatsToJointMats( joints, jointFrame, numJoints );

	// first joint is always root of entire hierarchy
	if ( remove_origin_offset ) {
		joints[0].SetTranslation( offset );
	} else {
		joints[0].SetTranslation( joints[0].ToVec3() + offset );
	}

	// transform the children
	md5joints = model->GetJoints();
	for( i = 1; i < numJoints; i++ ) {
		joints[i] *= joints[ md5joints[i].parent - md5joints ];
	}
}

/*
=====================
idGameEdit::ANIM_CreateMeshForAnim
=====================
*/
idRenderModel *idGameEdit::ANIM_CreateMeshForAnim( idRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset ) {
	renderEntity_t			ent;
	const idDict			*args;
	const char				*temp;
	idRenderModel			*newmodel;
	const idMD5Anim 		*md5anim;
	idStr					filename;
	idStr					extension;
	const idAnim			*anim;
	int						animNum;
	idVec3					offset;
	const idDeclModelDef	*modelDef;

	if ( !model || model->IsDefaultModel() ) {
		return NULL;
	}

	args = gameLocal.FindEntityDefDict( classname, false );
	if ( !args ) {
		return NULL;
	}

	memset( &ent, 0, sizeof( ent ) );

	ent.bounds.Clear();
	ent.suppressSurfaceInViewID = 0;

	modelDef = ANIM_GetModelDefFromEntityDef( args );
	if ( modelDef ) {
		animNum = modelDef->GetAnim( animname );
		if ( !animNum ) {
			return NULL;
		}
		anim = modelDef->GetAnim( animNum );
		if ( !anim ) {
			return NULL;
		}
		md5anim = anim->MD5Anim( 0 );
		ent.customSkin = modelDef->GetDefaultSkin();
		offset = modelDef->GetVisualOffset();
	} else {
		filename = animname;
		filename.ExtractFileExtension( extension );
		if ( !extension.Length() ) {
			animname = args->GetString( va( "anim %s", animname ) );
		}

// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
		md5anim = animationLib->GetAnim( animname );
// RAVEN END
		offset.Zero();
	}

	if ( !md5anim ) {
		return NULL;
	}

	temp = args->GetString( "skin", "" );
	if ( temp[ 0 ] ) {
		ent.customSkin = declManager->FindSkin( temp );
	}

	ent.numJoints = model->NumJoints();
//RAVEN BEGIN
//amccarthy: Added allocation tag
	ent.joints = ( idJointMat * )Mem_Alloc16( ent.numJoints * sizeof( *ent.joints ), MA_ANIM );
//RAVEN END

	ANIM_CreateAnimFrame( model, md5anim, ent.numJoints, ent.joints, FRAME2MS( frame ), offset, remove_origin_offset );

	newmodel = model->InstantiateDynamicModel( &ent, NULL, NULL, ~SURF_COLLISION );

	Mem_Free16( ent.joints );
	ent.joints = NULL;

	return newmodel;
}

// RAVEN BEGIN
// jsinger: allow for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
idAnim::idAnim( idDeclModelDef const *def, SerialInputStream &stream )
{
	modelDef = def;
	numAnims = stream.ReadIntValue();
	for(int i=0; i<numAnims; i++)
	{
		idStr fname = stream.ReadStringValue();
		idMD5Anim *testAnim = animationLib.GetAnim(fname);
		anims[i] = testAnim;
	}
	name = stream.ReadStringValue();
	realname = stream.ReadStringValue();
	int numFrameLookups = stream.ReadIntValue();
	for(int i=0; i<numFrameLookups; i++)
	{
		frameLookup_t lookup;
		lookup.num = stream.ReadIntValue();
		lookup.firstCommand = stream.ReadIntValue();
		frameLookup.Append(lookup);
	}
	int numFrameCommands = stream.ReadIntValue();
	frameCommands.AssureSize(numFrameCommands);
	for(int i=0; i<numFrameCommands; i++)
	{
		frameCommand_t &com(frameCommands[i]);
		if(stream.ReadBoolValue())	// has string?
		{
			com.string = new idStr(stream.ReadStringValue());
		}
		else 
		{
			com.string = NULL;
		}
		if(stream.ReadBoolValue())	// has joint
		{
			com.joint = new idStr(stream.ReadStringValue());
		}
		else
		{
			com.joint = NULL;
		}
		if(stream.ReadBoolValue())	// has joint2
		{
			com.joint2 = new idStr(stream.ReadStringValue());
		}
		else
		{
			com.joint2 = NULL;
		}
		if(stream.ReadBoolValue())	// has parmList
		{
			int numParms = stream.ReadIntValue();
			com.parmList = new idList<idStr>();
			for(int i=0; i<numParms; i++)
			{
				com.parmList->Append(stream.ReadStringValue());
			}
		}
		else
		{
			com.parmList = NULL;
		}
	}
	flags.prevent_idle_override = stream.ReadBoolValue();
	flags.random_cycle_start = stream.ReadBoolValue();
	flags.ai_no_turn = stream.ReadBoolValue();
	flags.anim_turn = stream.ReadBoolValue();
	flags.sync_cycle = stream.ReadBoolValue();
    flags.ai_no_look = stream.ReadBoolValue();
    flags.ai_look_head_only = stream.ReadBoolValue();
	rate = stream.ReadFloatValue();
}

void idAnim::Write( SerialOutputStream &stream ) const
{
	stream.Write(numAnims);
	for(int i=0; i<numAnims; i++)
	{
		stream.Write(anims[i]->Name());
	}
	stream.Write(name);
	stream.Write(realname);
	stream.Write(frameLookup.Num());
	for(int i=0; i<frameLookup.Num(); i++)
	{
		stream.Write(frameLookup[i].num);
		stream.Write(frameLookup[i].firstCommand);
	}
	stream.Write(frameCommands.Num());
	for(int i=0; i<frameCommands.Num(); i++)
	{
		stream.Write(frameCommands[i].type);
		stream.Write(frameCommands[i].string != NULL);
		if(frameCommands[i].string != NULL)
		{
			stream.Write(*frameCommands[i].string);
		}
		stream.Write(frameCommands[i].joint != NULL);
		if(frameCommands[i].joint != NULL)
		{
			stream.Write(*frameCommands[i].joint);
		}
		stream.Write(frameCommands[i].joint2 != NULL);
		if(frameCommands[i].joint2 != NULL)
		{
			stream.Write(*frameCommands[i].joint2);
		}
		stream.Write(frameCommands[i].parmList != NULL);
		if(frameCommands[i].parmList != NULL)
		{
            stream.Write(frameCommands[i].parmList->Num());
			for(int j=0; j<frameCommands[i].parmList->Num(); j++)
			{
				stream.Write((*frameCommands[i].parmList)[j]);
			}
		}
	}
	stream.Write((bool)flags.prevent_idle_override);
	stream.Write((bool)flags.random_cycle_start);
	stream.Write((bool)flags.ai_no_turn);
	stream.Write((bool)flags.anim_turn);
	stream.Write((bool)flags.sync_cycle);
	stream.Write((bool)flags.ai_no_look);
	stream.Write((bool)flags.ai_look_head_only);
	stream.Write(rate);
}
#endif
// RAVEN END
