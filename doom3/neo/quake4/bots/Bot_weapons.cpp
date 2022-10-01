// Bot_weapons.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

idCVar bot_weaponsfile( "bot_weaponsfile", "weapons.c", CVAR_GAME | CVAR_CHEAT, "which file to load the weapons weights from" );

idBotWeaponInfoManager botWeaponInfoManager;

/*
=========================
idBotWeaponInfoManager::BotValidWeaponNumber
=========================
*/
int idBotWeaponInfoManager::BotValidWeaponNumber( int weaponnum )
{
	if( weaponnum <= 0 || weaponnum > BOT_MAX_WEAPONS )
	{
		gameLocal.Error( "weapon number out of range\n" );
		return false;
	}
	return true;
}

/*
=========================
idBotWeaponInfoManager::BotWeaponStateFromHandle
=========================
*/
bot_weaponstate_t* idBotWeaponInfoManager::BotWeaponStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS )
	{
		gameLocal.Error( "move state handle %d out of range\n", handle );
	}

	return &botweaponstates[handle];
}

/*
========================
idBotWeaponInfoManager::ParseProjectileInfo
========================
*/
void idBotWeaponInfoManager::ParseProjectileInfo( idParser& parser, projectileinfo_t& newProjectileInfo )
{
	idToken token;
	idToken valueToken;

	parser.ExpectTokenString( "{" );

	while( true )
	{
		if( !parser.ReadToken( &token ) )
		{
			parser.Error( "Unexpected end of file found while parsing weaponinfo!" );
		}

		if( token == "}" )
		{
			break;
		}

		if( !parser.ReadToken( &valueToken ) )
		{
			parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
		}

		if( token == "name" )
		{
			newProjectileInfo.name = valueToken; // name of the projectile
		}
		else if( token == "model" )
		{
			newProjectileInfo.model = valueToken; // model of the projectile
		}
		else if( token == "flags" )
		{
			newProjectileInfo.flags = valueToken.GetIntValue(); // special flags
		}
		else if( token == "gravity" )
		{
			newProjectileInfo.gravity = valueToken.GetFloatValue(); // amount of gravity applied to the projectile [0,1]
		}
		else if( token == "damage" )
		{
			newProjectileInfo.damage = valueToken.GetIntValue(); // damage of the projectile
		}
		else if( token == "radius" )
		{
			newProjectileInfo.radius = valueToken.GetFloatValue(); // radius of damage
		}
		else if( token == "visdamage" )
		{
			newProjectileInfo.visdamage = valueToken.GetIntValue(); // damage of the projectile to visible entities
		}
		else if( token == "damagetype" )
		{
			newProjectileInfo.damagetype = valueToken.GetIntValue(); // type of damage (combination of the DAMAGETYPE_? flags)
		}
		else if( token == "healthinc" )
		{
			newProjectileInfo.healthinc = valueToken.GetIntValue(); // health increase the owner gets
		}
		else if( token == "push" )
		{
			newProjectileInfo.push = valueToken.GetFloatValue(); // amount a player is pushed away from the projectile impact
		}
		else if( token == "detonation" )
		{
			newProjectileInfo.detonation = valueToken.GetFloatValue(); // time before projectile explodes after fire pressed
		}
		else if( token == "bounce" )
		{
			newProjectileInfo.bounce = valueToken.GetFloatValue(); // amount the projectile bounces
		}
		else if( token == "bouncefric" )
		{
			newProjectileInfo.bouncefric = valueToken.GetFloatValue(); // amount the bounce decreases per bounce
		}
		else if( token == "bouncestop" )
		{
			newProjectileInfo.bouncestop = valueToken.GetFloatValue(); // minimum bounce value before bouncing stops
		}
		else
		{
			parser.Error( "ParseProjectileInfo: Unexpected token %s\n", token.c_str() );
		}
	}
}

/*
========================
idBotWeaponInfoManager::ParseWeaponInfo
========================
*/
void idBotWeaponInfoManager::ParseWeaponInfo( idParser& parser, weaponinfo_t& newWeaponInfo )
{
	idToken token;
	idToken valueToken;

	parser.ExpectTokenString( "{" );

	while( true )
	{
		if( !parser.ReadToken( &token ) )
		{
			parser.Error( "Unexpected end of file found while parsing weaponinfo!" );
		}

		if( token == "}" )
		{
			break;
		}

		if( !parser.ReadToken( &valueToken ) )
		{
			parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
		}

		if( token == "number" )
		{
			newWeaponInfo.number = valueToken.GetIntValue(); //weapon number
		}
		else if( token == "name" )
		{
			newWeaponInfo.name = valueToken; //name of the weapon
		}
		else if( token == "level" )
		{
			newWeaponInfo.level = valueToken.GetIntValue();
		}
		else if( token == "model" )
		{
			newWeaponInfo.model = valueToken; //model of the weapon
		}
		else if( token == "weaponindex" )
		{
			newWeaponInfo.weaponindex = valueToken.GetIntValue(); //index of weapon in inventory
		}
		else if( token == "flags" )
		{
			newWeaponInfo.flags = valueToken.GetIntValue(); //special flags
		}
		else if( token == "projectile" )
		{
			newWeaponInfo.projectile = valueToken; //projectile used by the weapon
		}
		else if( token == "numprojectiles" )
		{
			newWeaponInfo.numprojectiles = valueToken.GetIntValue(); //number of projectiles
		}
		else if( token == "hspread" )
		{
			newWeaponInfo.hspread = valueToken.GetFloatValue(); //horizontal spread of projectiles (degrees from middle)
		}
		else if( token == "vspread" )
		{
			newWeaponInfo.vspread = valueToken.GetFloatValue(); //vertical spread of projectiles (degrees from middle)
		}
		else if( token == "speed" )
		{
			newWeaponInfo.speed = valueToken.GetFloatValue(); //speed of the projectile (0 = instant hit)
		}
		else
		{
			parser.Error( "ParseWeaponInfo: Unexpected token %s\n", token.c_str() );
		}
	}
}

/*
========================
idBotWeaponInfoManager::LoadWeaponConfig
========================
*/
void idBotWeaponInfoManager::LoadWeaponConfig( char* filename )
{
	idParser parser;
	idToken token;
	int j;

	if( !parser.LoadFile( filename ) )
	{
		gameLocal.Error( "Failed to load bot weapon config %s\n", filename );
	}

	while( parser.ReadToken( &token ) )
	{
		if( token == "weaponinfo" )
		{
			weaponinfo_t newWeaponInfo;
			ParseWeaponInfo( parser, newWeaponInfo );

			if( newWeaponInfo.number < 0 || newWeaponInfo.number >= BOT_MAX_WEAPONS )
			{
				parser.Error( "weapon info number %d out of range\n", newWeaponInfo.number );
				return;
			}

			weaponinfo[newWeaponInfo.number] = newWeaponInfo;
			weaponinfo[newWeaponInfo.number].valid = true;
		}
		else if( token == "projectileinfo" )
		{
			projectileinfo_t newProjectileInfo;

			ParseProjectileInfo( parser, newProjectileInfo );

			projectileinfo.Append( newProjectileInfo );
		}
		else
		{
			parser.Error( "LoadWeaponConfig: Unknown definintion %s", token.c_str() );
		}
	}

	// fix up weapons.
	for( int i = 0; i < BOT_MAX_WEAPONS; i++ )
	{
		if( !weaponinfo[i].valid )
		{
			continue;
		}

		if( weaponinfo[i].name.Length() <= 0 )
		{
			parser.Error( "weapon %d has no name\n", i );
			return;
		}

		if( weaponinfo[i].projectile.Length() <= 0 )
		{
#if 0 //k
			parser.Error( "weapon %s has no projectile\n", weaponinfo[i].name );
#else
			parser.Error( "weapon %s has no projectile\n", weaponinfo[i].name.c_str() );
#endif
			return;
		}

		//find the projectile info and copy it to the weapon info
		for( j = 0; j < projectileinfo.Num(); j++ )
		{
			if( projectileinfo[j].name == weaponinfo[i].projectile )
			{
				memcpy( &weaponinfo[i].proj, &projectileinfo[j], sizeof( projectileinfo_t ) );
				break;
			}
		}
		if( j == projectileinfo.Num() )
		{
			parser.Error( "weapon %s uses undefined projectile\n", weaponinfo[i].name.c_str() );
			return;
		}
	}
}

/*
========================
idBotWeaponInfoManager::Init
========================
*/
void idBotWeaponInfoManager::Init( void )
{
	rvmScopedLexerBaseFolder scopedBaseFolder( BOTFILESBASEFOLDER );

	LoadWeaponConfig( ( char* )bot_weaponsfile.GetString() );
}

/*
=====================
WeaponWeightIndex
=====================
*/
void idBotWeaponInfoManager::WeaponWeightIndex( weightconfig_t* wwc, bot_weaponstate_t* weaponState )
{
	for( int i = 0; i < BOT_MAX_WEAPONS; i++ )
	{
		weaponState->weaponweightindex[i] = botFuzzyWeightManager.FindFuzzyWeight( wwc, ( char* )weaponinfo[i].name.c_str() );
	}
}

/*
=====================
idBotWeaponInfoManager::WeaponWeightIndex
=====================
*/
void idBotWeaponInfoManager::BotFreeWeaponWeights( int weaponstate )
{
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if( !ws )
	{
		return;
	}
//	if (ws->weaponweightconfig)
//		FreeWeightConfig(ws->weaponweightconfig);
	//	if (ws->weaponweightindex) FreeMemory(ws->weaponweightindex);
}

/*
=====================
BotLoadWeaponWeights
=====================
*/
int idBotWeaponInfoManager::BotLoadWeaponWeights( int weaponstate, char* filename )
{
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if( !ws )
	{
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	}

#if !defined(_QUAKE4) //k: free not in state alloc
	BotFreeWeaponWeights( weaponstate );
#endif

	ws->weaponweightconfig = botFuzzyWeightManager.ReadWeightConfig( filename );
	if( !ws->weaponweightconfig )
	{
		gameLocal.Error( "couldn't load weapon config %s\n", filename );
		return BLERR_CANNOTLOADWEAPONWEIGHTS;
	}

	WeaponWeightIndex( ws->weaponweightconfig, ws );
	return BLERR_NOERROR;
}

/*
=====================
idBotWeaponInfoManager::BotGetWeaponInfo
=====================
*/
void idBotWeaponInfoManager::BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t* weaponinfo )
{
	bot_weaponstate_t* ws;

	if( !BotValidWeaponNumber( weapon ) )
	{
		return;
	}
	ws = BotWeaponStateFromHandle( weaponstate );
	if( !ws )
	{
		return;
	}

	*weaponinfo = this->weaponinfo[weapon];
}

/*
=====================
idBotWeaponInfoManager::BotChooseBestFightWeapon
=====================
*/
int idBotWeaponInfoManager::BotChooseBestFightWeapon( int weaponstate, int* inventory )
{
	int i, index, bestweapon;
	float weight, bestweight;
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if( !ws )
	{
		return 0;
	}

	//if the bot has no weapon weight configuration
	if( !ws->weaponweightconfig )
	{
		return 0;
	}

	bestweight = 0;
	bestweapon = 0;
	for( i = 0; i < BOT_MAX_WEAPONS; i++ )
	{
		if( !weaponinfo[i].valid )
		{
			continue;
		}
		index = ws->weaponweightindex[i];
		if( index < 0 )
		{
			continue;
		}
		weight = botFuzzyWeightManager.FuzzyWeight( inventory, ws->weaponweightconfig, index );
		if( weight > bestweight )
		{
			bestweight = weight;
			bestweapon = i;
		}
	}
	return bestweapon;
}

/*
=====================
idBotWeaponInfoManager::BotResetWeaponState
=====================
*/
void idBotWeaponInfoManager::BotResetWeaponState( int weaponstate )
{
	weightconfig_t* weaponweightconfig;
	int* weaponweightindex;
	bot_weaponstate_t* ws;

	ws = BotWeaponStateFromHandle( weaponstate );
	if( !ws )
	{
		return;
	}
	weaponweightconfig = ws->weaponweightconfig;
	weaponweightindex = ws->weaponweightindex;

	//Com_Memset(ws, 0, sizeof(bot_weaponstate_t));
	ws->weaponweightconfig = weaponweightconfig;
	memcpy( ws->weaponweightindex, weaponweightindex, sizeof( int ) * BOT_MAX_WEAPONS );
}

/*
=====================
idBotWeaponInfoManager::BotAllocWeaponState
=====================
*/
int idBotWeaponInfoManager::BotAllocWeaponState( void )
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ )
	{
		if( !botweaponstates[i].inUse )
		{
			botweaponstates[i].Reset();
			botweaponstates[i].inUse = true;
			return i;
		}
	}
	return 0;
}

/*
=====================
idBotWeaponInfoManager::BotAllocWeaponState
=====================
*/
void idBotWeaponInfoManager::BotFreeWeaponState( int ws )
{
	botweaponstates[ws].inUse = false;
	botweaponstates[ws].Reset();

#ifdef _QUAKE4 //k: free in state free
	BotFreeWeaponWeights( ws );
#endif
}
