// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclDamage.h"
#include "DeclDamageFilter.h"
#include "DeclTargetInfo.h"
#include "../Game_local.h"
#include "../proficiency/StatsTracker.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclDamage

===============================================================================
*/

/*
================
sdDeclDamage::sdDeclDamage
================
*/
sdDeclDamage::sdDeclDamage( void ) {
	FreeData();
}

/*
================
sdDeclDamage::~sdDeclDamage
================
*/
sdDeclDamage::~sdDeclDamage( void ) {
	FreeData();
}

/*
================
sdDeclDamage::DefaultDefinition
================
*/
const char* sdDeclDamage::DefaultDefinition( void ) const {
	return		\
		"{\n"	\
		"}\n";
}

/*
================
sdDeclDamage::GetDamage
================
*/
float sdDeclDamage::GetDamage( idEntity* entity, bool& noScale ) const {
	if ( !damage ) {
		noScale = false;
		return 0.f;
	}

	for ( int i = 0; i < damage->GetNumFilters(); i++ ) {
		const damageFilter_t& filter = damage->GetFilter( i );

		if ( !filter.target ) {
			continue;
		}

		if ( filter.target->FilterEntity( entity ) ) {
			noScale = filter.noScale;
			switch ( filter.mode ) {
				default:
				case DFM_NORMAL:
					return filter.damage;
				case DFM_PERCENT:
					return ( filter.damage * 0.01f ) * entity->GetMaxHealth();
			}
		}
	}

	noScale = false;
	return 0.f;
}

/*
================
sdDeclDamage::Parse
================
*/
bool sdDeclDamage::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if( !token.Icmp( "damage" ) ) {

			if ( !src.ReadToken( &token ) ) {
				src.Error( "sdDeclDamageFilter::ParseLevel Missing Parm for 'damage'" );
				return false;
			}

			damage = gameLocal.declDamageFilterType[ token ];

		} else if( !token.Icmp( "radius" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'radius' keyword in damage def '%s'", base->GetName() );
				return false;
			}
			radius = token.GetFloatValue();

		} else if( !token.Icmp( "noTrace" ) ) {

			flags.noTrace = true;

		} else if( !token.Icmp( "noGod" ) ) {

			flags.noGod = true;

		} else if( !token.Icmp( "noArmor" ) ) {

			flags.noArmor = true;

		} else if( !token.Icmp( "gib" ) ) {

			flags.gib = true;

		} else if( !token.Icmp( "noAir" ) ) {

			flags.noAir = true;

		} else if( !token.Icmp( "noPain" ) ) {

			flags.noPain = true;			

		} else if( !token.Icmp( "noTeam" ) ) {

			flags.noTeam = true;

		} else if( !token.Icmp( "forcePassengerKill" ) ) {

			flags.forcePassengerKill = true;

		} else if( !token.Icmp( "noHeadShot" ) ) {

			flags.canHeadShot = false;

		} else if( !token.Icmp( "melee" ) ) {

			flags.melee = true;

		} else if( !token.Icmp( "noDirection" ) ) {

			flags.noDirection = true;

		} else if( !token.Icmp( "kickDir" ) ) {

			if( !src.Parse1DMatrix( 3, kickDir.ToFloatPtr() ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'kickDir' keyword in damage def '%s'", base->GetName() );
				return false;
			}

		} else if( !token.Icmp( "mtr_blob" ) ) {

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'mtr_blob' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobMaterial = token;

		} else if( !token.Icmpn( "snd_", 4 ) ) {
			idStr key;
			token.Right( token.Length() - 4, key );

			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'snd_hit' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			sounds.Set( key, token );

		} else if( !token.Icmp( "kick_time" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'kick_time' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			kickTime = token.GetFloatValue();

		} else if( !token.Icmp( "kick_amplitude" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'kick_amplitude' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			kickAmplitude = token.GetFloatValue();
		} else if( !token.Icmp( "selfDamageScale" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'selfDamageScale' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			selfDamageScale = token.GetFloatValue();

		} else if( !token.Icmp( "blob_time" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'blob_time' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobTime = token.GetFloatValue();

		} else if( !token.Icmp( "blob_offset_x" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'blob_x' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobRect[ 0 ] = token.GetFloatValue();

		} else if( !token.Icmp( "blob_offset_y" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'blob_y' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobRect[ 1 ] = token.GetFloatValue();

		} else if( !token.Icmp( "blob_width" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'blob_width' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobRect[ 2 ] = token.GetFloatValue();

		} else if( !token.Icmp( "blob_height" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'blob_height' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			blobRect[ 3 ] = token.GetFloatValue();

		} else if( !token.Icmp( "knockback" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'knockback' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			knockback = token.GetFloatValue();

		} else if( !token.Icmp( "knockback_damage" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'knockback_damage' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			damageKnockback = token.GetFloatValue();

		} else if( !token.Icmp( "push" ) ) {
			if( !src.ExpectTokenType( TT_NUMBER, 0, &token )) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'push' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			push = token.GetFloatValue();

		} else if( !token.Icmp( "tt_obituary" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'tt_obituary' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			obituary = gameLocal.declToolTipType[ token ];

		} else if( !token.Icmp( "tt_obituary_self" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'tt_obituary_self' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			selfObituary = gameLocal.declToolTipType[ token ];

		} else if( !token.Icmp( "tt_obituary_team_kill" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'tt_obituary_team_kill' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			teamKillObituary = gameLocal.declToolTipType[ token ];			

		} else if( !token.Icmp( "tt_obituary_unknown" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'tt_obituary_unknown' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			unknownObituary = gameLocal.declToolTipType[ token ];			

		} else if( !token.Icmp( "tt_obituary_unknown_friendly" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'tt_obituary_unknown' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			unknownFriendlyObituary = gameLocal.declToolTipType[ token ];			

		} else if( !token.Icmp( "prof_damage" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'prof_damage' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			damageBonus		= gameLocal.declProficiencyItemType[ token ];

		} else if( !token.Icmp( "stat_name" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'stat_name' keyword in damage def '%s'", base->GetName() );
				return false;
			}

			sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

			stats.name				= token.c_str();

			stats.damage			= tracker.GetStat( tracker.AllocStat( va( "%s_damage", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.shotsHit			= tracker.GetStat( tracker.AllocStat( va( "%s_shots_hit", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.shotsHitHead		= tracker.GetStat( tracker.AllocStat( va( "%s_shots_hit_head", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.shotsHitTorso		= tracker.GetStat( tracker.AllocStat( va( "%s_shots_hit_torso", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.teamKills			= tracker.GetStat( tracker.AllocStat( va( "%s_teamkills", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.kills				= tracker.GetStat( tracker.AllocStat( va( "%s_kills", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.deaths			= tracker.GetStat( tracker.AllocStat( va( "%s_deaths", token.c_str() ), sdNetStatKeyValue::SVT_INT ) );
			stats.xp				= tracker.GetStat( tracker.AllocStat( va( "%s_xp", token.c_str() ), sdNetStatKeyValue::SVT_FLOAT ) );

			stats.totalKills		= tracker.GetStat( tracker.AllocStat( "total_kills", sdNetStatKeyValue::SVT_INT ) );
			stats.totalHeadshotKills= tracker.GetStat( tracker.AllocStat( "total_headshot_kills", sdNetStatKeyValue::SVT_INT ) );
			stats.totalDeaths		= tracker.GetStat( tracker.AllocStat( "total_deaths", sdNetStatKeyValue::SVT_INT ) );
			stats.totalTeamKills	= tracker.GetStat( tracker.AllocStat( "total_team_kills", sdNetStatKeyValue::SVT_INT ) );
			stats.totalDamage		= tracker.GetStat( tracker.AllocStat( "total_damage", sdNetStatKeyValue::SVT_INT ) );

		} else if( !token.Icmp( "team_kill_cvar" ) ) {
			if( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
				src.Error( "sdDeclDamage::Parse Missing or Bad Parameter for 'team_kill_cvar' keyword in damage def '%s'", base->GetName() );
				return false;
			}
			
			teamKillCVar = cvarSystem->Find( token.c_str() );
			if ( teamKillCVar == NULL ) {
				gameLocal.Warning( "Unknown CVar '%s' in damage def '%s'", token.c_str(), base->GetName() );
			}

		} else if( !token.Icmp( "no_complaint" ) ) {

			flags.noComplaint = true;

		} else if ( token.Icmp( "record_hit_stats" ) == 0 ) {

			flags.recordHitStats = true;

		} else if ( token.Icmp( "team_damage" ) == 0 ) {

			flags.isTeamDamage = true;

		} else if( !token.Cmp( "}" ) ) {

			break;
			
		} else {

			src.Error( "sdDeclDamage::Parse Invalid Token '%s' in damage def '%s'", token.c_str(), base->GetName() );
			return false;

		}
	}

	return true;
}

/*
================
sdDeclDamage::FreeData
================
*/
void sdDeclDamage::FreeData( void ) {
	damage				= NULL;
	push				= 0.f;

	teamKillCVar		= NULL;

	obituary				= NULL;
	selfObituary			= NULL;
	teamKillObituary		= NULL;
	unknownObituary			= NULL;
	unknownFriendlyObituary = NULL;

	damageBonus			= NULL;

	radius				= 50.f;
	knockback			= 0.f;
	damageKnockback		= 0.f;
	kickTime			= 0.f;
	kickAmplitude		= 0.f;

	selfDamageScale		= 1.f;

	sounds.Clear();

	blobTime			= 0.f;
	blobRect.Zero();
	blobMaterial		= "";

	kickDir				= idVec3( 0, 0, 0 );

	flags.gib					= false;
	flags.noArmor				= false;
	flags.noGod					= false;
	flags.noTrace				= false;
	flags.noAir					= false;
	flags.noTeam				= false;
	flags.noPain				= false;
	flags.forcePassengerKill	= false;
	flags.canHeadShot			= true;
	flags.melee					= false;
	flags.noComplaint			= false;
	flags.recordHitStats		= false;
	flags.isTeamDamage			= false;
	flags.noDirection			= false;

	stats.damage		= NULL;
	stats.shotsHit		= NULL;
	stats.shotsHitTorso	= NULL;
	stats.shotsHitHead	= NULL;
	stats.kills			= NULL;
	stats.teamKills		= NULL;
	stats.deaths		= NULL;
	stats.xp			= NULL;

	stats.totalHeadshotKills= NULL;
	stats.totalKills		= NULL;
	stats.totalDeaths		= NULL;
	stats.totalTeamKills	= NULL;
	stats.totalDamage		= NULL;
}

/*
================
sdDeclDamage::DamageForName
================
*/
const sdDeclDamage* sdDeclDamage::DamageForName( const char* name, bool makeDefault ) {
	return gameLocal.declDamageType.LocalFind( name, makeDefault );
};

/*
================
sdDeclDamage::GetSound
================
*/
const char* sdDeclDamage::GetSound( const char* key ) const {
	return sounds.GetString( key );
}

/*
================
sdDeclDamage::CacheFromDict
================
*/
void sdDeclDamage::CacheFromDict( const idDict& dict ) {
	const idKeyValue *kv;

	kv = NULL;
	while( kv = dict.MatchPrefix( "dmg_", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declDamageType[ kv->GetValue() ];
		}
	}
}

