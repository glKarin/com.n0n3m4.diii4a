// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#include "DeclRequirement.h"
#include "../structures/TeamManager.h"
#include "../Player.h"
#include "../vehicles/Transport.h"
#include "../../framework/DeclParseHelper.h"
#include "../rules/GameRules.h"










/*
===============================================================================

	Protoypes

===============================================================================
*/

class sdRequirementCheck_Team : public sdRequirementCheck {
public:
												sdRequirementCheck_Team( void ) : index( -1 ) { ; }

	virtual void								Init( const idDict& parms );
	virtual bool								Check( idEntity* main, idEntity* other ) const;

private:
	int											index;
};

class sdRequirementCheck_Allegiance : public sdRequirementCheck {
public:

	virtual void								Init( const idDict& parms );
	virtual bool								Check( idEntity* main, idEntity* other ) const;

private:
	teamAllegiance_t							allegiance;
};

class sdRequirementCheck_Radar : public sdRequirementCheck {
public:
	virtual void								Init( const idDict& parms ) { }
	virtual bool								Check( idEntity* main, idEntity* other ) const;
};

class sdRequirementCheck_RadarOther : public sdRequirementCheck {
public:
	virtual void								Init( const idDict& parms ) { }
	virtual bool								Check( idEntity* main, idEntity* other ) const;
};

class sdRequirementCheck_Ability : public sdRequirementCheck {
public:

	virtual void								Init( const idDict& parms );
	virtual bool								Check( idEntity* main, idEntity* other ) const;

protected:
	qhandle_t									abilityHandle;
};

class sdRequirementCheck_AbilityOther : public sdRequirementCheck_Ability {
public:
	virtual bool								Check( idEntity* main, idEntity* other ) const;
};

class sdRequirementCheck_Property : public sdRequirementCheck {
public:
	typedef enum condition_e {
		CND_EQ,
		CND_NE,
		CND_GT,
		CND_LT,
		CND_GE,
		CND_LE,
		CND_INVALID,
	} condition_t;

	typedef enum propertyType_e {
		PT_HEALTH,
		PT_RANGE,
		PT_XP,
		PT_TOTAL_XP,
		PT_NEEDSREVIVE,
		PT_WANT_REVIVE,
		PT_SAME_ENTITY,
		PT_SPECTATOR,
		PT_PRONE,
		PT_CROUCH,
		PT_NEEDSREPAIR,
		PT_PROFICIENCY,
		PT_SPAWN_PROFICIENCY,
		PT_DRIVER,
		PT_DISGUISED,
		PT_VEHICLEFULL,
		PT_INVEHICLE,
		PT_SAME_FIRETEAM,
		PT_INVALID
	} propertyType_t;

public:
												sdRequirementCheck_Property( void ) : value( 0.f ), property( PT_INVALID ), condition( CND_INVALID ) { ; }

	virtual void								Init( const idDict& parms );
	virtual bool								Check( idEntity* main, idEntity* other ) const;

private:
	float										value;
	propertyType_t								property;
	int											subProperty;
	condition_t									condition;
	bool										useOther;
};

class sdRequirementCheck_EntityBin : public sdRequirementCheck {
public:
									sdRequirementCheck_EntityBin( void ) : type( NULL ) { ; }

	virtual void					Init( const idDict& parms );
	virtual bool					Check( idEntity* main, idEntity* other ) const;

private:
	const idDeclEntityDef*			type;
};





















/*
===============================================================================

	Implementations

===============================================================================
*/

/*
===============================================================================

	sdRequirementCheck

===============================================================================
*/
sdFactory< sdRequirementCheck > sdRequirementCheck::checkerFactory;

/*
================
sdRequirementCheck::InitFactory
================
*/
void sdRequirementCheck::InitFactory( void ) {
	checkerFactory.RegisterType( "allegiance",		sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_Allegiance > );
	checkerFactory.RegisterType( "ability",			sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_Ability > );
	checkerFactory.RegisterType( "ability_other",	sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_AbilityOther > );
	checkerFactory.RegisterType( "team",			sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_Team > );
	checkerFactory.RegisterType( "property",		sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_Property > );
	checkerFactory.RegisterType( "radar",			sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_Radar > );
	checkerFactory.RegisterType( "radar_other",		sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_RadarOther > );
	checkerFactory.RegisterType( "entity_bin",		sdFactory< sdRequirementCheck >::Allocator< sdRequirementCheck_EntityBin > );
}

/*
================
sdRequirementCheck::ShutdownFactory
================
*/
void sdRequirementCheck::ShutdownFactory( void ) {
	checkerFactory.Shutdown();
}

/*
================
sdRequirementCheck::AllocChecker
================
*/
sdRequirementCheck* sdRequirementCheck::AllocChecker( const char* typeName ) {
	return checkerFactory.CreateType( typeName );
}

/*
===============================================================================

	sdRequirementCheck_Team

===============================================================================
*/

/*
================
sdRequirementCheck_Team::Init
================
*/
void sdRequirementCheck_Team::Init( const idDict& parms ) {
	const char* teamName = parms.GetString( "value" );
	if ( !idStr::Icmp( teamName, "none" ) ) {
		index = -1;
	} else {
		sdTeamInfo& team = sdTeamManager::GetInstance().GetTeam( teamName );
		index = team.GetIndex();
	}
}

/*
================
sdRequirementCheck_Team::Check
================
*/
bool sdRequirementCheck_Team::Check( idEntity* main, idEntity* /* other */ ) const {
	sdTeamInfo* team = main->GetGameTeam();
	if ( team ) {
		return team->GetIndex() == index;
	}
	return index == -1;
}

/*
===============================================================================

	sdRequirementCheck_Property

===============================================================================
*/

/*
================
sdRequirementCheck_Property::Init
================
*/
void sdRequirementCheck_Property::Init( const idDict& parms ) {
	useOther					= parms.GetBool( "use_other" );
	const char* propertyName	= parms.GetString( "property" );
	const char* subPropertyName = parms.GetString( "property_type" );

	if ( !idStr::Icmp( propertyName, "health" ) ) {
		property = PT_HEALTH;
	} else if ( !idStr::Icmp( propertyName, "range" ) ) {
		property = PT_RANGE;
	} else if ( !idStr::Icmp( propertyName, "total_xp" ) ) {
		property = PT_TOTAL_XP;
	} else if ( !idStr::Icmp( propertyName, "xp" ) ) {
		property = PT_XP;
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ subPropertyName ];
		if ( !prof ) {
			gameLocal.Error( "sdRequirementCheck_Property::Init Invalid Proficiency Type '%s'", subPropertyName );
		}
		subProperty = prof->Index();
	} else if ( !idStr::Icmp( propertyName, "need_revive" ) ) {
		property = PT_NEEDSREVIVE;
	} else if ( !idStr::Icmp( propertyName, "want_revive" ) ) {
		property = PT_WANT_REVIVE;
	} else if ( !idStr::Icmp( propertyName, "same_entity" ) ) {
		property = PT_SAME_ENTITY;
	} else if ( !idStr::Icmp( propertyName, "spectator" ) ) {
		property = PT_SPECTATOR;
	} else if ( !idStr::Icmp( propertyName, "prone" ) ) {
		property = PT_PRONE;
	} else if ( !idStr::Icmp( propertyName, "crouch" ) ) {
		property = PT_CROUCH;
	} else if ( !idStr::Icmp( propertyName, "need_repair" ) ) {
		property = PT_NEEDSREPAIR;
	} else if ( !idStr::Icmp( propertyName, "proficiency" ) ) {
		property = PT_PROFICIENCY;
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ subPropertyName ];
		if ( !prof ) {
			gameLocal.Error( "sdRequirementCheck_Property::Init Invalid Proficiency Type '%s'", subPropertyName );
		}
		subProperty = prof->Index();
	} else if ( !idStr::Icmp( propertyName, "spawn_proficiency" ) ) {
		property = PT_SPAWN_PROFICIENCY;
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ subPropertyName ];
		if ( !prof ) {
			gameLocal.Error( "sdRequirementCheck_Property::Init Invalid Proficiency Type '%s'", subPropertyName );
		}
		subProperty = prof->Index();
	} else if ( !idStr::Icmp( propertyName, "driver" ) ) {
		property = PT_DRIVER;
	} else if ( !idStr::Icmp( propertyName, "disguised" ) ) {
		property = PT_DISGUISED;
	} else if ( !idStr::Icmp( propertyName, "vehicle_full" ) ) {
		property = PT_VEHICLEFULL;
	} else if ( !idStr::Icmp( propertyName, "in_vehicle" ) ) {
		property = PT_INVEHICLE;
		subProperty = 0;
		if ( !idStr::Icmp( subPropertyName, "local_player" ) ) {
			subProperty = 1;
		}
	} else if ( !idStr::Icmp( propertyName, "same_fireteam" ) ) {
		property = PT_SAME_FIRETEAM;
	} else {
		gameLocal.Error( "sdRequirementCheck_Property::Init Invalid Property Type '%s'", propertyName );
	}

	value = parms.GetFloat( "value" );

	const char* conditionName = parms.GetString( "condition" );

	if ( !idStr::Icmp( conditionName, "==" ) ) {
		condition = CND_EQ;
	} else if ( !idStr::Icmp( conditionName, "!=" ) ) {
		condition = CND_NE;
	} else if ( !idStr::Icmp( conditionName, ">" ) ) {
		condition = CND_GT;
	} else if ( !idStr::Icmp( conditionName, "<" ) ) {
		condition = CND_LT;
	} else if ( !idStr::Icmp( conditionName, ">=" ) ) {
		condition = CND_GE;
	} else if ( !idStr::Icmp( conditionName, "<=" ) ) {
		condition = CND_LE;
	} else {
		gameLocal.Error( "sdRequirementCheck_Property::Init Invalid Condition Type '%s'", conditionName );
	}

	if ( property == PT_RANGE ) {
		value *= value;
	}
}

/*
================
sdRequirementCheck_Property::Check
================
*/
bool sdRequirementCheck_Property::Check( idEntity* main, idEntity* other ) const {
	if ( useOther ) {
		if ( !other ) {
			return false;
		}

		Swap( main, other );
	}

	float checkValue = 0.f;

	switch ( property ) {
		case PT_RANGE:
			if ( !other ) {
				return false;
			} else {
				checkValue = ( main->GetPhysics()->GetOrigin() - other->GetPhysics()->GetOrigin() ).LengthSqr();
			}
			break;
		case PT_HEALTH:
			checkValue = main->GetHealth();
			break;
		case PT_XP: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->GetProficiencyTable().GetPoints( subProperty );
			break;
		}
		case PT_TOTAL_XP: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->GetProficiencyTable().GetXP();
			break;
		}
		case PT_NEEDSREVIVE: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->NeedsRevive() ? 1.f : 0.f;
			break;
		}
		case PT_WANT_REVIVE: {
			idPlayer* player = other->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->WantRespawn() ? 0.f : 1.f;
			break;
		}
		case PT_SAME_ENTITY: {
			checkValue = main == other ? 1.f : 0.f;
			break;
		}
		case PT_SPECTATOR: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->IsSpectator() ? 1.f : 0.f;
			break;
		}
		case PT_PRONE: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->IsProne() || ( player->GetPlayerPhysics().GetProneChangeEndTime() > gameLocal.time );
			break;
		}
		case PT_CROUCH: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->IsCrouching();
			break;
		}
		case PT_NEEDSREPAIR: {
			if ( !main ) {
				return false;
			}
			checkValue = main->NeedsRepair() ? 1.0f : 0.0f;
			break;
		}
		case PT_PROFICIENCY: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->GetProficiencyTable().GetLevel( subProperty );
			break;
		}
		case PT_SPAWN_PROFICIENCY: {
			idPlayer* player = main->Cast< idPlayer >();
			if ( !player ) {
				return false;
			}
			checkValue = player->GetProficiencyTable().GetSpawnLevel( subProperty );
			break;
		}
		case PT_DRIVER: {
			idPlayer* driver = main->Cast< idPlayer >();
			idPlayer* passenger = other->Cast< idPlayer >();
			if ( !driver || !passenger ) {
				return false;
			}

			sdTransport* transport = driver->GetProxyEntity()->Cast< sdTransport >();
			if ( !transport || transport != passenger->GetProxyEntity() ) {
				checkValue = false;
			} else {
				checkValue = transport->GetPositionManager().FindDriver() == driver;
			}
			break;
		}
		case PT_DISGUISED: {
			idPlayer* player = main->Cast< idPlayer >();
			if( !player ) {
				return false;
			}

			checkValue = player->IsDisguised() ? 1.f : 0.f;
			break;
		}
		case PT_VEHICLEFULL: {
			idPlayer* player = main->Cast< idPlayer >();
			if( player == NULL ) {
				return false;
			}

			sdTransport* transport = player->GetProxyEntity()->Cast< sdTransport >();
			if ( transport == NULL ) {
				checkValue = false;
			} else {
				sdTransportPositionManager &positionManager = transport->GetPositionManager();
				int numAvailable = 0;
				for ( int i = positionManager.NumPositions() - 1; i >= 0; i-- ) {
					if ( positionManager.PositionForId( i )->GetPlayer() == NULL ) {
						numAvailable++;
					}
				}

				if ( numAvailable >= 1 ) {
					checkValue = false;
				} else {
					checkValue = true;
				}
			}
			break;
		}
		case PT_INVEHICLE: {
			idPlayer* player;
			if ( subProperty ) {
				player = gameLocal.GetLocalViewPlayer();
			} else {
				player = useOther ? other->Cast< idPlayer >() : main->Cast< idPlayer >();
			}
			 
			if( player == NULL ) {
				return false;
			}

			sdTransport* transport = player->GetProxyEntity()->Cast< sdTransport >();

			if ( ( subProperty && transport != other ) || ( transport == NULL ) ) {
				checkValue = false;
			} else {
				checkValue = true;
			}
			break;
		}
		case PT_SAME_FIRETEAM: {
			idPlayer* playerMain = main->Cast< idPlayer >();
			idPlayer* playerOther = other->Cast< idPlayer >();
			if ( playerMain == NULL || other == NULL ) {
				return false;
			}

			sdFireTeam* ft1 = gameLocal.rules->GetPlayerFireTeam( playerMain->entityNumber );
			sdFireTeam* ft2 = gameLocal.rules->GetPlayerFireTeam( playerOther->entityNumber );

			if ( ft1 != NULL || ft2 != NULL ) {
				if ( ft1 == ft2 ) {
					checkValue = true;
					break;
				}
			}

			checkValue = false;
			break;
		}
	}

	switch ( condition ) {
		case CND_EQ:
			return value == checkValue;
		case CND_NE:
			return value != checkValue;
		case CND_GT:
			return checkValue > value;
		case CND_LT:
			return checkValue < value;
		case CND_GE:
			return checkValue >= value;
		case CND_LE:
			return checkValue <= value;
	}

	return false;
}

/*
===============================================================================

	sdRequirementCheck_Allegiance

===============================================================================
*/

/*
================
sdRequirementCheck_Allegiance::Check
================
*/
void sdRequirementCheck_Allegiance::Init( const idDict& parms ) {
	const char* value = parms.GetString( "value" );
	if ( !idStr::Icmp( value, "friend" ) ) {
		allegiance = TA_FRIEND;
	} else if ( !idStr::Icmp( value, "enemy" ) ) {
		allegiance = TA_ENEMY;
	} else if ( !idStr::Icmp( value, "neutral" ) ) {
		allegiance = TA_NEUTRAL;
	} else {
		gameLocal.Error( "sdRequirementCheck_Allegiance::Init Invalid Allegiance Type '%s'", value );
	}
}

/*
================
sdRequirementCheck_Allegiance::Check
================
*/
bool sdRequirementCheck_Allegiance::Check( idEntity* main, idEntity* other ) const {
	return main->GetEntityAllegiance( other ) == allegiance;
}


/*
===============================================================================

	sdRequirementCheck_Radar

===============================================================================
*/

/*
================
sdRequirementCheck_Radar::Check
================
*/
bool sdRequirementCheck_Radar::Check( idEntity* main, idEntity* other ) const {
	if ( !other ) {
		return false;
	}
	sdTeamInfo* team = main->GetGameTeam();
	if ( !team ) {
		return false;
	}
	return team->PointInRadar( other->GetPhysics()->GetOrigin(), RM_RADAR ) != NULL;
}

/*
===============================================================================

	sdRequirementCheck_RadarOther

===============================================================================
*/

/*
================
sdRequirementCheck_RadarOther::Check
================
*/
bool sdRequirementCheck_RadarOther::Check( idEntity* main, idEntity* other ) const {
	if ( !other ) {
		return false;
	}
	sdTeamInfo* team = other->GetGameTeam();
	if ( !team ) {
		return false;
	}
	return team->PointInRadar( main->GetPhysics()->GetOrigin(), RM_RADAR ) != NULL;
}

/*
===============================================================================

	sdRequirementCheck_Ability

===============================================================================
*/

/*
================
sdRequirementCheck_Ability::Check
================
*/
void sdRequirementCheck_Ability::Init( const idDict& parms ) {
	const char* abilityValue = parms.GetString( "value" );
	if ( !*abilityValue ) {
		gameLocal.Error( "sdRequirementCheck_Ability::Init No 'value' key specified" );
	}

	abilityHandle = sdRequirementManager::GetInstance().RegisterAbility( abilityValue );
}

/*
================
sdRequirementCheck_Ability::Check
================
*/
bool sdRequirementCheck_Ability::Check( idEntity* main, idEntity* /* other */ ) const {
	return main->HasAbility( abilityHandle );
}

/*
===============================================================================

	sdRequirementCheck_AbilityOther

===============================================================================
*/

/*
================
sdRequirementCheck_AbilityOther::Check
================
*/
bool sdRequirementCheck_AbilityOther::Check( idEntity* /* main */, idEntity* other ) const {
	return other && other->HasAbility( abilityHandle );
}


/*
===============================================================================

	sdRequirementCheck_EntityBin

===============================================================================
*/

/*
================
sdRequirementCheck_EntityBin::Init
================
*/
void sdRequirementCheck_EntityBin::Init( const idDict& parms ) {
	const char* typeName = parms.GetString( "type" );
	if ( !*typeName ) {
		gameLocal.Error( "sdRequirementCheck_EntityBin::Init - No 'type' key specified" );
	}

	type = gameLocal.declEntityDefType[ typeName ];
	if ( type == NULL ) {
		gameLocal.Error( "sdRequirementCheck_EntityBin::Init - Couldn't find entityDef for '%s'", typeName );
	}
}

/*
================
sdRequirementCheck_EntityBin::Check
================
*/
bool sdRequirementCheck_EntityBin::Check( idEntity* main, idEntity* other ) const {
	idPlayer* player = main->Cast< idPlayer >();
	if ( !player ) {
		return false;
	}

	if ( player->BinFindEntityOfType( type ) != NULL ) {
		return true;
	}

	return false;
}

/*
===============================================================================

	sdDeclRequirement

===============================================================================
*/

/*
================
sdDeclRequirement::sdDeclRequirement
================
*/
sdDeclRequirement::sdDeclRequirement( void ) : checker( NULL ) {
}

/*
================
sdDeclRequirement::~sdDeclRequirement
================
*/
sdDeclRequirement::~sdDeclRequirement( void ) {
	delete checker;
}

/*
================
sdDeclRequirement::DefaultDefinition
================
*/
const char* sdDeclRequirement::DefaultDefinition( void ) const {
	return						\
		"{\n"					\
		"}\n";
}

/*
================
sdDeclRequirement::Parse
================
*/
bool sdDeclRequirement::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
	//src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	sdDeclParseHelper declHelper( this, text, textLength, src );

	src.SkipUntilString( "{", &token );

	if ( GetState() == DS_DEFAULTED ) {
		return true;
	}

	if( !src.ReadToken( &token ) ) {
		src.Error( "sdDeclRequirement::Unexpected end of file" );
		return false;
	}

	checker = sdRequirementCheck::AllocChecker( token );
	if ( !checker ) {
		src.Error( "sdDeclRequirement::Parse Invalid Requirement Type '%s'", token.c_str() );
		return false;
	}

	idDict parms;

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if( !token.Icmp( "parms" ) ) {
			
			if ( !parms.Parse( src ) ) {
				return false;
			}

		} else if( !token.Cmp( "}" ) ) {

			break;

		} else {

			src.Error( "sdDeclRequirement::Parse Invalid Token '%s'", token.c_str() );
			return false;

		}
	}

	checker->Init( parms );

	return true;
}

/*
================
sdDeclRequirement::FreeData
================
*/
void sdDeclRequirement::FreeData( void ) {
	delete checker;
	checker = NULL;
}

