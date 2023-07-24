// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#include "DeclPlayerClass.h"
#include "DeclRadialMenu.h"

#include "../structures/TeamManager.h"
#include "../guis/UserInterfaceManager.h"
#include "../proficiency/StatsTracker.h"

#include "../../decllib/declTypeHolder.h"
#include "../../framework/KeyInput.h"
#include "../../framework/DeclParseHelper.h"

/*
===============================================================================

	sdDeclPlayerClass

===============================================================================
*/

/*
================
sdDeclPlayerClass::sdDeclPlayerClass
================
*/
sdDeclPlayerClass::sdDeclPlayerClass( void ) {
	contextCallback.Init( this );
	FreeData();
	proficiencies.SetGranularity( 4 );
}

/*
================
sdDeclPlayerClass::~sdDeclPlayerClass
================
*/
sdDeclPlayerClass::~sdDeclPlayerClass( void ) {
}

/*
================
sdDeclPlayerClass::DefaultDefinition
================
*/
const char* sdDeclPlayerClass::DefaultDefinition( void ) const {
	return 
		"{\n"							\
		"}\n";
}

/*
================
sdDeclPlayerClass::ParseOption
================
*/
bool sdDeclPlayerClass::ParseOption( idParser& src ) {
	if ( !src.ExpectTokenString( "{" ) ) {
		return false;
	}



	optionList_t& list = options.Alloc();

	idToken token;
	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "sdDeclPlayerClass::ParseOption Unexpected End of File" );
			return false;
		}

		if ( !token.Cmp( "}" ) ) {
			break;
		}

		const sdDeclItemPackage* package = gameLocal.declItemPackageType[ token ];
		if ( !package ) {
			src.Error( "sdDeclPlayerClass::ParseOption Invalid Item Package '%s'", token.c_str() );
			return false;
		}
		list.Alloc() = package;
	}

	return true;
}

/*
================
sdDeclPlayerClass::Parse
================
*/
bool sdDeclPlayerClass::Parse( const char *text, const int textLength ) {
	idToken token;
	idParser src;

	src.SetFlags( DECL_LEXER_FLAGS );
//	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );	
//	src.AddIncludes( GetFileLevelIncludeDependencies() );
	
	sdDeclParseHelper declHelper( this, text, textLength, src );	

	src.SkipUntilString( "{", &token );

	ammoLimits.Clear();

	int i;
	for ( i = 0; i < gameLocal.declAmmoTypeType.Num(); i++ ) {
		ammoLimits.Append( 0 );
	}

	static int counter = 0;
	bool updatePacifier = !networkSystem->IsDedicated();

	while ( true ) {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "sdDeclPlayerClass::Parse Unexpected End of File" );
			return false;
		}

		if( counter & 15 && updatePacifier ) {
			common->PacifierUpdate();
		}
		counter++;

		if ( !token.Icmp( "info" ) ) {
			if ( !classData.Parse( src ) ) {
				src.Error( "sdDeclPlayerClass::Parse Error Reading Class Info Dictionary" );
				return false;
			}

			ReadFromDict( classData );

			gameLocal.CacheDictionaryMedia( classData );

		} else if ( !token.Icmp( "modelData" ) ) {

			if ( !modelData.Parse( src ) ) {
				src.Error( "sdDeclPlayerClass::Parse Error Reading Class Info Dictionary" );
				return false;
			}

		} else if ( !token.Icmp( "abilities" ) ) {
			if ( !src.ExpectTokenString( "{" ) ) {
				return false;
			}

			while ( true ) {
				if ( !src.ReadToken( &token ) ) {
					src.Error( "sdDeclPlayerClass::Parse Unexpected End of File Whilst Parsing Abilities" );
					return false;
				}

				if ( !token.Cmp( "}" ) ) {
					break;
				}

				abilities.Add( token );
			}
		} else if ( !token.Icmp( "proficiency" ) ) {
			if ( !src.ReadToken( &token ) ) {
				return false;
			}

			const sdDeclProficiencyType* type = gameLocal.declProficiencyTypeType[ token.c_str() ];
			if ( type == NULL ) {
				src.Error( "sdDeclPlayerClass::Parse: Unknown proficiency type '%s'", token.c_str() );
				return false;
			}

			proficiencyCategory_t& category = proficiencies.Alloc();
			category.index = type->Index();
			category.text = type->GetProficiencyText();

			if ( !src.ExpectTokenString( "{" ) ) {
				return false;
			}

			if ( src.CheckTokenString( "text" ) ) {

				if ( !src.ExpectTokenType( TT_STRING, 0, &token ) ) {
					src.Error( "sdDeclPlayerClass::Parse Error Parsing 'text'" );
					return false;
				}

				category.text = declHolder.FindLocStr( token );

			}

			while ( true ) {
				if ( !src.ReadToken( &token ) ) {
					return false;
				}

				if( !token.Icmp( "level" ) ) {
					if( !ParseProficiency( category, src )) {
						src.Error( "sdDeclPlayerClass::Parse: failed to parse proficiency" );
						return false;
					}
					continue;
				}
				if ( !token.Cmp( "}" ) ) {
					break;
				}
			}
		} else if ( !token.Icmp( "ammoLimits" ) ) {

			idDict temp;
			if ( !temp.Parse( src ) ) {
				src.Error( "sdDeclPlayerClass::Parse Error Reading ammoLimits dictionary" );
				return false;
			}

			for ( i = 0; i < temp.GetNumKeyVals(); i++ ) {
				const idKeyValue* kv = temp.GetKeyVal( i );				
				const sdDeclAmmoType* type = gameLocal.declAmmoTypeType[ kv->GetKey() ];
				if ( !type ) {
					src.Error( "sdDeclPlayerClass::Parse Invalid Ammo Type '%s'", kv->GetKey().c_str() );
					return false;
				}
				ammoLimits[ type->GetAmmoType() ] = atoi( kv->GetValue() );
				if ( ammoLimits[ type->GetAmmoType() ] < 0 ) {
					src.Error( "sdDeclPlayerClass::Parse Ammo Limits Must be >= 0" );
					return false;
				}
			}

		} else if ( !token.Icmp( "option" ) ) {

			if ( !ParseOption( src ) ) {
				src.Error( "sdDeclPlayerClass::Parse Error Parsing Options" );
				return false;
			}

		} else if ( !token.Cmp( "}" ) ) {
			break;
		}
	}

	if ( GetState() == DS_DEFAULTED ) {
		package			= gameLocal.declItemPackageType[ "_default" ];
		disguisePackage = gameLocal.declItemPackageType[ "_default" ];
		playerModel		= gameLocal.declModelDefType[ "_default" ];
	} else {
		if ( !package ) {
			src.Error( "sdDeclPlayerClass::Parse Invalid or Empty Item Package Supplied in '%s'", GetName() );
		}

		if ( !playerModel ) {
			src.Error( "sdDeclPlayerClass::Parse Invalid or Empty Item Model Supplied in '%s'", GetName() );
		}

		if ( team ) {
			// precache quickchat
			const sdDeclRadialMenu* radialMenu;
			radialMenu = gameLocal.declRadialMenuType.LocalFind( va( "%s_context", team->GetLookupName() ), false );
			radialMenu = gameLocal.declRadialMenuType.LocalFind( va( "%s_quickchat", team->GetLookupName() ), false );
		}
	}

	for ( i = 0; i < ammoLimits.Num(); i++ ) {
		totalAmmoLimit += ammoLimits[ i ];
	}

	return true;
}

/*
================
sdDeclPlayerClass::ReadFromDict
================
*/
void sdDeclPlayerClass::ReadFromDict( const idDict& info ) {
	const char* text;
	
	if ( info.GetString( "items", "", &text ) ) {
		package			= gameLocal.declItemPackageType[ text ];
	}

	if ( info.GetString( "items_disguised", "", &text ) ) {
		disguisePackage	= gameLocal.declItemPackageType[ text ];
	}

	if ( info.GetString( "model", "", &text ) ) {
		playerModel		= gameLocal.declModelDefType[ text ];
	}

	if ( info.GetString( "mtr_cm_icon", "", &text ) ) {
		cmIcon			= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_cm_icon_unknown", "", &text ) ) {
		cmIconUnknown	= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_cm_icon_class", "", &text ) ) {
		cmIconClass		= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_icon_class", "", &text ) ) {
		iconClass		= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_icon_friendly_arrow", "", &text ) ) {
		iconFriendlyArrow	= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_icon_offscreen", "", &text ) ) {
		iconOffScreen		= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "mtr_icon_enemy_arrow", "", &text ) ) {
		iconEnemyArrow		= gameLocal.declMaterialType[ text ];
	}

	if ( info.GetString( "title", "", &text ) ) {
		title			= declHolder.FindLocStr( text );
	}

	if ( info.GetString( "max_health", "100", &text ) ) {
		maxHealth		= atoi( text );
	}

	if ( info.GetString( "player_class_num", "", &text ) ) {
		playerClassNum = ( playerClassTypes_t ) atoi( text );
	}

	if ( info.GetString( "team", "", &text ) ) {
		team			= &sdTeamManager::GetInstance().GetTeam( text );
	}

	if ( info.GetString( "gui", "", &text ) ) {
		classOverlay	= gameLocal.declGUIType[ text ];
	}

	if ( info.GetString( "class_thread", "", &text ) ) {
		classThreadName = text;
	}

	if ( info.GetString( "class_context_cvar", "", &text ) ) {
		contextCallback.SetCVar( cvarSystem->Find( text ) );
	}

	if ( info.GetString( "climate_skin_key", "", &text ) ) {
		climateSkinKey = text;
	}

	if ( info.GetString( "cvar_limit", "", &text ) ) {
		limitCVar = cvarSystem->Find( text );
		if ( !limitCVar ) {
			cvarSystem->SetCVarString( text, "" );
			limitCVar = cvarSystem->Find( text );

			assert( limitCVar );
		}
	}

	if ( info.GetString( "def_dead_body", "", &text ) ) {
		deadBodyDef		= gameLocal.declEntityDefType[ text ];
	}

	if ( info.GetString( "stat_name", "", &text ) ) {
		sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();

		stats.timePlayed	= tracker.GetStat( tracker.AllocStat( va( "%s_time_used", text ), sdNetStatKeyValue::SVT_INT ) );
		stats.deaths		= tracker.GetStat( tracker.AllocStat( va( "%s_deaths", text ), sdNetStatKeyValue::SVT_INT ) );
		stats.suicides		= tracker.GetStat( tracker.AllocStat( va( "%s_suicides", text ), sdNetStatKeyValue::SVT_INT ) );
		stats.revived		= tracker.GetStat( tracker.AllocStat( va( "%s_revived", text ), sdNetStatKeyValue::SVT_INT ) );
		stats.tapouts		= tracker.GetStat( tracker.AllocStat( va( "%s_tapouts", text ), sdNetStatKeyValue::SVT_INT ) );
		stats.respawns		= tracker.GetStat( tracker.AllocStat( va( "%s_respawns", text ), sdNetStatKeyValue::SVT_INT ) );
	}
}

/*
================
sdDeclPlayerClass::FreeData
================
*/
void sdDeclPlayerClass::FreeData( void ) {
	package				= NULL;
	playerModel			= NULL;
	cmIcon				= NULL;
	cmIconClass			= NULL;
	cmIconUnknown		= NULL;
	iconClass			= NULL;
	iconFriendlyArrow	= NULL;
	iconEnemyArrow		= NULL;
	iconOffScreen		= NULL;
	team				= NULL;
	classOverlay		= NULL;
	classThreadName		= "";
	climateSkinKey		= "";
	limitCVar			= NULL;
	bindContext			= NULL;

	totalAmmoLimit		= 0;
	maxHealth			= 100;

	stats.timePlayed	= NULL;
	stats.deaths		= NULL;
	stats.suicides		= NULL;
	stats.revived		= NULL;
	stats.tapouts		= NULL;
	stats.respawns		= NULL;

	playerClassNum      = NOCLASS;

	deadBodyDef		= NULL;

	abilities.Clear();
	title = NULL;
	modelData.Clear();
	classData.Clear();
	ammoLimits.Clear();
	options.Clear();
	proficiencies.Clear();
	contextCallback.SetCVar( NULL );
}

/*
================
sdDeclPlayerClass::CacheFromDict
================
*/
void sdDeclPlayerClass::CacheFromDict( const idDict& dict ) {
	const idKeyValue* kv = NULL;

	while( kv = dict.MatchPrefix( "pc", kv ) ) {
		if ( kv->GetValue().Length() ) {
			gameLocal.declPlayerClassType[ kv->GetValue() ];
		}
	}
}

/*
================
sdDeclPlayerClass::BuildQuickChatDeclName
================
*/
const char* sdDeclPlayerClass::BuildQuickChatDeclName( const char* qc ) const {
	if ( team == NULL ) {
		return "";
	}
	return va( "%s/%s", team->GetLookupName(), qc );
}


/*
============
sdDeclPlayerClass::ParseProficiency
============
*/
bool sdDeclPlayerClass::ParseProficiency( proficiencyCategory_t& category, idParser& src ) {
	idToken token;

	category.upgrades.SetGranularity( 4 );
	proficiencyUpgrade_t& upgrade = category.upgrades.Alloc();
	upgrade.text = NULL;
	upgrade.toolTip = NULL;

	if( !src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
		return false;
	}
	upgrade.level = token.GetIntValue();

	if( !src.ExpectTokenString( "{" ) ) {
		return false;
	}

	while( true ) {
		if( !src.ReadToken( &token ) ) {
			return false;
		}

		if( token.Icmp( "text" ) == 0 ) {
			if( !src.ReadToken( &token ) ) {
				return false;
			}

			upgrade.text = declHolder.FindLocStr( token );

			if ( upgrade.text->GetState() == DS_DEFAULTED ) {
				src.Warning( "Defaulted text string for proficiency" );
			}

			continue;
		}

		if( token.Icmp( "title" ) == 0 ) {
			if( !src.ReadToken( &token ) ) {
				return false;
			}

			upgrade.title = declHolder.FindLocStr( token );

			if ( upgrade.text->GetState() == DS_DEFAULTED ) {
				src.Warning( "Defaulted title string for proficiency" );
			}

			continue;
		}

		if( token.Icmp( "icon" ) == 0 ) {
			if( !src.ReadToken( &token ) ) {
				return false;
			}

			upgrade.materialInfo = token;
			continue;
		}

		if( token.Icmp( "tooltip" ) == 0 ) {
			if( !src.ExpectTokenType( TT_STRING, TT_STRING, &token ) ) {
				return false;
			}

			upgrade.toolTip = gameLocal.declToolTipType[ token.c_str() ];
			continue;
		}

		if( token.Icmp( "sound" ) == 0 ) {
			if( !src.ExpectTokenType( TT_STRING, TT_STRING, &token ) ) {
				return false;
			}

			// precache sounds
			const idSoundShader* shader = gameLocal.declSoundShaderType[ token.c_str() ];
			if ( shader == NULL ) {
				gameLocal.Warning( "sdDeclPlayerClass: Sound shader not found '%s'", token.c_str() );
			}

			upgrade.sound = token.c_str();
			continue;
		}

		if( token.Cmp( "}" ) == 0 ) {
			break;
		}
		
	}
	return true;
}

/*
============
sdDeclPlayerClass::OnContextCVarChanged
============
*/
void sdDeclPlayerClass::OnContextCVarChanged( void ) const {
	const char* contextName = contextCallback.GetValue();
	if ( *contextName == '\0' ) {
		bindContext = NULL;
		return;
	}

	bindContext = keyInputManager->AllocBindContext( contextName );
}

/*
============
sdDeclPlayerClass::OnInputInit
============
*/
void sdDeclPlayerClass::OnInputInit( void ) const {
	OnContextCVarChanged();
}

/*
============
sdDeclPlayerClass::OnInputShutdown
============
*/
void sdDeclPlayerClass::OnInputShutdown( void ) const {
	bindContext = NULL;
}


/*
============
sdPlayerClassContextCallback::SetCVar
============
*/
void sdPlayerClassContextCallback::SetCVar( idCVar* _cvar ) {
	if ( cvar != NULL ) {
		cvar->UnRegisterCallback( this );
		cvar = NULL;
	}

	cvar = _cvar;
	if ( cvar != NULL ) {
		cvar->RegisterCallback( this );
	}

	playerClass->OnContextCVarChanged();
}

/*
============
sdPlayerClassContextCallback::OnChanged
============
*/
void sdPlayerClassContextCallback::OnChanged( void ) {
	playerClass->OnContextCVarChanged();
}
