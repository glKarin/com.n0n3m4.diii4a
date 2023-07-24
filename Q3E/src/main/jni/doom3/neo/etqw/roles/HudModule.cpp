// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "HudModule.h"
#include "../Player.h"
#include "../guis/UserInterfaceLocal.h"
#include "../guis/UIList.h"
#include "../Weapon.h"
#include "../structures/DeployRequest.h"
#include "../structures/DeployMask.h"
#include "../script/Script_Helper.h"
#include "../Atmosphere.h"

#include "../roles/Tasks.h"

#include "../../decllib/declTypeHolder.h"

#include "../../sys/sys_local.h"

/*
===============================================================================

	sdHudModule

===============================================================================
*/

/*
==============
sdHudModule::sdHudModule
==============
*/
sdHudModule::sdHudModule( void ) :
	_enabled( false ),
	_activated( false ),
	_inhibitUserCommands( false ),
	_inhibitControllerMovement( false ),
	_passive( false ),
	_hideWeapon( false ),
	_inhibitGuiFocus( false ),
	_manualDraw( false ),
	_sort( 0 ),
	_allowInhibit( true ) {
	_node.SetOwner( this );
	_drawNode.SetOwner( this );
}

/*
==============
sdHudModule::InitGui
==============
*/
void sdHudModule::InitGui( const char* guiName, bool permanent ) {
	_guiHandle = gameLocal.LoadUserInterface( guiName, false, permanent, "default", this );

	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( _guiHandle );
	if ( ui ) {
		ui->Deactivate();
	}
}


/*
==============
sdHudModule::~sdHudModule
==============
*/
sdHudModule::~sdHudModule( void ) {
	gameLocal.FreeUserInterface( _guiHandle );
}


/*
============
sdHudModule::Draw
============
*/
void sdHudModule::Draw( void ) {
	sdUserInterfaceLocal* ui = GetGui();
	if ( ui ) {
		ui->Draw();
	}
}

/*
============
sdPlayerProperties::GetProperty
============
*/
sdProperties::sdProperty* sdHudModule::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = _properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && prop->GetValueType() != type ) {
		gameLocal.Error( "sdHudModule::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdHudModule::GetProperty
================
*/
sdProperties::sdProperty* sdHudModule::GetProperty( const char* name ) {
	return _properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
==============
sdHudModule::Enable
==============
*/
void sdHudModule::Enable( bool enable, bool urgent, bool timedOut ) {
	if ( _enabled == enable ) {
		return;
	}

	_enabled = enable;

	if ( _enabled ) {
		_activated = false;
		if ( _passive ) {
			Activate();
			gameLocal.localPlayerProperties.PushPassiveHudModule( *this );
		} else {
			if ( urgent ) {
				gameLocal.localPlayerProperties.PushActiveHudModuleUrgent( *this );				
			} else {
				gameLocal.localPlayerProperties.PushActiveHudModule( *this );
			}
		}
		gameLocal.localPlayerProperties.AddDrawHudModule( *this );
	} else {		
		_activated = false;
		
		if( !timedOut ) {	
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( player ) {
				if ( DoWeaponLock() ) {
					player->SetClientWeaponLock( true );
					player->oldButtons.btn.attack = true;
				}
			}
		}
		
		OnCancel();
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( _guiHandle );
		if ( ui ) {
			ui->Deactivate();
		}
		_node.Remove();
		_drawNode.Remove();
	}
}

/*
==============
sdHudModule::HandleGuiEvent
==============
*/
bool sdHudModule::HandleGuiEvent( const sdSysEvent* event ) {
	if ( event->IsKeyEvent() && event->IsKeyDown() && event->GetKey() == K_ESCAPE ||
			( event->IsGuiEvent() && event->GetGuiAction() == ULI_MENU_EVENT_CANCEL ) ) {
		Enable( false );
		return true;
	}
	return false;
}

/*
==============
sdHudModule::GetGui
==============
*/
sdUserInterfaceLocal* sdHudModule::GetGui( void ) const {
	return gameLocal.GetUserInterface( _guiHandle );
}

/*
==============
sdHudModule::Activate
==============
*/
void sdHudModule::Activate( void ) {
	if ( _activated ) {
		return;
	}

	_activated = true;
	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( _guiHandle );
	if ( ui ) {
		ui->Activate();
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		if ( DoWeaponLock() ) {
			localPlayer->SetClientWeaponLock( true );
			localPlayer->oldButtons.btn.attack = true;
		}
	}

	OnActivate();
}


/*
============
sdHudModule::Update
============
*/
void sdHudModule::Update( void ) {
	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( _guiHandle );
	if ( ui && _enabled && !ui->IsActive() ) {
		Enable( false );
	}
}

/*
===============================================================================

	sdLimboMenu

===============================================================================
*/

/*
==============
sdLimboMenu::sdLimboMenu
==============
*/
sdLimboMenu::sdLimboMenu( void ) {	
	_allowInhibit			= false;
	_inhibitUserCommands	= true;
	_inhibitGuiFocus		= true;
}

/*
==============
sdLimboMenu::HandleGuiEvent
==============
*/
bool sdLimboMenu::HandleGuiEvent( const sdSysEvent* event ) {
	sdUserInterfaceLocal* ui = GetGui();
	bool retVal = ui ? ui->PostEvent( event ) : false;

	if( !retVal ) {
		retVal = sdHudModule::HandleGuiEvent( event );
	}

	return retVal;  

}

/*
==============
sdLimboMenu::OnActivate
==============
*/
void sdLimboMenu::OnActivate( void ) {
}

/*
==============
sdLimboMenu::OnCancel
==============
*/
void sdLimboMenu::OnCancel( void ) {
}

/*
==============
sdLimboMenu::Update
==============
*/
void sdLimboMenu::Update( void ) {
	sdHudModule::Update();
}

/*
===============================================================================

	sdDeployMenu

===============================================================================
*/

/*
==============
sdDeployMenu::sdDeployMenu
==============
*/
sdDeployMenu::sdDeployMenu( void ) {
	memset( &deployableRenderEntity, 0, sizeof( deployableRenderEntity ) );

	deployableRenderEntityHandle		= -1;

	deployableRenderEntity.spawnID	= -1;

	decalMaterial						= declHolder.FindMaterial( "textures/decals/white_decal" );
	decalMaterialOuter					= declHolder.FindMaterial( "textures/decals/white_decal_dark" );
	decalMaterialArrows					= declHolder.FindMaterial( "textures/decals/deploy_arrows" );

	decalHandle							= -1;
	decalHandleOuter					= -1;
	decalHandleArrows					= -1;

	modeToggle							= false;
	lockedAngles[ 0 ]					= 0;
	lockedAngles[ 1 ]					= 0;
	lockedAngles[ 2 ]					= 0;
	rotation							= 0.0f;

	deployableObject					= NULL;

	lastExpandedExtents.minx = -1;
}

/*
==============
sdDeployMenu::SetObject
==============
*/
void sdDeployMenu::SetObject( const sdDeclDeployableObject* object ) {
	if ( !object ) {
		return;
	}

	deployableObject = object;
	gameEdit->ParseSpawnArgsToRenderEntity( deployableObject->GetPlacementInfo()->GetDict(), deployableRenderEntity );
}

/*
==============
sdDeployMenu::OnActivate
==============
*/
void sdDeployMenu::OnActivate( void ) {
	modeToggle = false;
}

/*
==============
sdDeployMenu::HandleGuiEvent
==============
*/
bool sdDeployMenu::HandleGuiEvent( const sdSysEvent* event ) {
/*
	if ( event->IsKeyEvent() && event->IsKeyDown() && event->GetKey() == K_ESCAPE ) {
		if ( modeToggle ) {
			SetDeployMode( false );
			return true;
		}
	}
*/
	return sdHudModule::HandleGuiEvent( event );
}

/*
==============
sdDeployMenu::FreeDecals
==============
*/
void sdDeployMenu::FreeDecals( void ) {
	if ( decalHandle != -1 ) {
		gameLocal.FreeLoggedDecal( decalHandle );
		decalHandle = -1;
	}
	if ( decalHandleOuter != -1 ) {
		gameLocal.FreeLoggedDecal( decalHandleOuter );
		decalHandleOuter = -1;
	}
	if ( decalHandleArrows != -1 ) {
		gameLocal.FreeLoggedDecal( decalHandleArrows );
		decalHandleArrows = -1;
	}
	lastExpandedExtents.minx = -1;
}

/*
==============
sdDeployMenu::OnCancel
==============
*/
void sdDeployMenu::OnCancel( void ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer ) {
		localPlayer->RaiseWeapon();
	}
	if ( deployableRenderEntityHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( deployableRenderEntityHandle );
		deployableRenderEntityHandle = -1;
	}

	FreeDecals();
}

/*
==============
sdDeployMenu::Update
==============
*/
void sdDeployMenu::Update( void ) {
	sdHudModule::Update();

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	if ( !deployableObject || !localPlayer ) {
		return;
	}

	deployResult_t result = localPlayer->GetDeployResult( position, deployableObject );

	if ( result != DR_OUT_OF_RANGE ) {

		if ( deployableRenderEntityHandle < 0 ) {
			deployableRenderEntityHandle = gameRenderWorld->AddEntityDef( &deployableRenderEntity );
		}

		idVec4 color;
		switch ( deployableState ) {
			case DR_CLEAR:
				color = colorGreen;
				break;
			case DR_WARNING:
				color = colorOrange;
				break;
			case DR_CONDITION_FAILED:
				color = colorWhite;
				break;
			default:
			case DR_FAILED:
				color = colorDkRed;
				break;
		}

		if ( decalHandle == -1 ) {
			decalHandle = gameLocal.RegisterLoggedDecal( decalMaterial );
			lastExpandedExtents.minx = -1;
		}
		if ( decalHandleOuter == -1 ) {
			decalHandleOuter = gameLocal.RegisterLoggedDecal( decalMaterialOuter );
			lastExpandedExtents.minx = -1;
		}
		if ( decalHandleArrows == -1 ) {
			decalHandleArrows = gameLocal.RegisterLoggedDecal( decalMaterialArrows );
		}
		gameLocal.ResetLoggedDecal( decalHandleArrows );

		gameDecalInfo_t* decalInfo = gameLocal.GetLoggedDecal( decalHandle );
		gameDecalInfo_t* decalInfoOuter = gameLocal.GetLoggedDecal( decalHandleOuter );
		gameDecalInfo_t* decalInfoArrows = gameLocal.GetLoggedDecal( decalHandleArrows );

		if ( !deployableObject->AllowRotation() ) {
			rotation = 0.0f;
		}

		idMat3 rotationStart;
		idAngles::YawToMat3( rotation, rotationStart );

		const sdPlayZone* playZone = gameLocal.GetPlayZone( position, sdPlayZone::PZF_DEPLOYMENT );

		const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( position, sdPlayZone::PZF_HEIGHTMAP );
		const sdHeightMapInstance* heightMap = NULL;
		if ( playZoneHeight ) {
			heightMap = &playZoneHeight->GetHeightMap();
		}

		const sdDeployMaskInstance* mask = NULL;

		if ( playZone ) {
			mask = playZone->GetMask( deployableObject->GetDeploymentMask() );
		}

		if ( mask && mask->IsValid() && heightMap && heightMap->IsValid() ) {
			idBounds bounds( position );
			bounds.ExpandSelf( deployableObject->GetObjectSize() );

			sdDeployMask::extents_t extents;
			mask->CoordsForBounds( bounds, extents );

			idBounds mainBounds;
			mask->GetBounds( extents, mainBounds, heightMap );

			float depth = 512.0f;
			idVec3 top;

			idBounds moveArrowBounds = mainBounds;
			idBounds rotateArrowBounds = mainBounds;

			// straight arrows
			moveArrowBounds.GetMaxs().y = moveArrowBounds.GetCenter().y;
			moveArrowBounds.GetMins().y += 0.25f * ( mainBounds.GetMaxs().y - mainBounds.GetMins().y );
			moveArrowBounds.GetMaxs().y += 0.25f * ( mainBounds.GetMaxs().y - mainBounds.GetMins().y );
			moveArrowBounds.GetMins().x = moveArrowBounds.GetCenter().x;
			{
				idVec3 center = moveArrowBounds.GetCenter();
				moveArrowBounds.TranslateSelf( -center );
				for ( int index = 0; index < 3; index++ ) {
					moveArrowBounds.GetMins()[ index ] *= 0.25f;
					moveArrowBounds.GetMaxs()[ index ] *= 0.25f;
				}
				moveArrowBounds.TranslateSelf( center );
			}

			// rotate arrows
			rotateArrowBounds.GetMins().x = rotateArrowBounds.GetCenter().x;
			{
				idVec3 center = rotateArrowBounds.GetCenter();
				rotateArrowBounds.TranslateSelf( -center );
				for ( int index = 0; index < 3; index++ ) {
					rotateArrowBounds.GetMins()[ index ] *= 0.2f;
					rotateArrowBounds.GetMaxs()[ index ] *= 0.2f;
				}
				rotateArrowBounds.TranslateSelf( center );
			}

			top = mainBounds.GetCenter();
			top[ 2 ] = mainBounds.GetMaxs()[ 2 ];

			idList< const idMaterial* > megaTextureMaterials;
			const idStrList& megaTextureMaterialNames = gameLocal.GetMapInfo().GetMegatextureMaterials();
			for ( int i = 0; i < megaTextureMaterialNames.Num(); i++ ) {
				megaTextureMaterials.Append( declHolder.FindMaterial( megaTextureMaterialNames[ i ] ) );
			}

			idFixedWinding winding;

			int spawnID = WORLD_SPAWN_ID;

			if ( GetDeployMode() ) {
				// rotate mode

				idMat3 rotationOffset;
				idAngles::YawToMat3( 120.0f, rotationOffset );

				// forward arrow
				winding += idVec5( idVec3( moveArrowBounds.GetMins().x, moveArrowBounds.GetMins().y, moveArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 0.0f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMins().x, moveArrowBounds.GetMaxs().y, moveArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 0.5f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMaxs().x, moveArrowBounds.GetMaxs().y, moveArrowBounds.GetMins().z - depth ), idVec2( 1.0f, 0.5f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMaxs().x, moveArrowBounds.GetMins().y, moveArrowBounds.GetMins().z - depth ), idVec2( 1.0f, 0.0f ) );

				winding.Rotate( mainBounds.GetCenter(), rotationStart );
				idVec3 offset = rotationStart[ 0 ] * ( 16.f * idMath::Sin( ( gameLocal.time / 500.f ) ) );
				for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
					winding[ i ].ToVec3() += offset;
				}
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, idVec4( 0.87f, 0.59f, 0.f, 1.f ), decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );

				// rotate arrows
				winding.Clear();
				winding += idVec5( idVec3( rotateArrowBounds.GetMins().x, rotateArrowBounds.GetMins().y, rotateArrowBounds.GetMins().z - depth ), idVec2( 0.0f, 0.0f ) );
				winding += idVec5( idVec3( rotateArrowBounds.GetMins().x, rotateArrowBounds.GetMaxs().y, rotateArrowBounds.GetMins().z - depth ), idVec2( 0.0f, 1.0f ) );
				winding += idVec5( idVec3( rotateArrowBounds.GetMaxs().x, rotateArrowBounds.GetMaxs().y, rotateArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 1.0f ) );
				winding += idVec5( idVec3( rotateArrowBounds.GetMaxs().x, rotateArrowBounds.GetMins().y, rotateArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 0.0f ) );

				winding.Rotate( mainBounds.GetCenter(), rotationStart * rotationOffset );
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );

				winding.Rotate( mainBounds.GetCenter(), rotationOffset );
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );
			} else {
				// move mode

				idMat3 rotationOffset;
				idAngles::YawToMat3( 90.0f, rotationOffset );

				winding += idVec5( idVec3( moveArrowBounds.GetMins().x, moveArrowBounds.GetMins().y, moveArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 0.0f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMins().x, moveArrowBounds.GetMaxs().y, moveArrowBounds.GetMins().z - depth ), idVec2( 0.5f, 0.5f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMaxs().x, moveArrowBounds.GetMaxs().y, moveArrowBounds.GetMins().z - depth ), idVec2( 1.0f, 0.5f ) );
				winding += idVec5( idVec3( moveArrowBounds.GetMaxs().x, moveArrowBounds.GetMins().y, moveArrowBounds.GetMins().z - depth ), idVec2( 1.0f, 0.0f ) );

				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );

				winding.Rotate( mainBounds.GetCenter(), rotationOffset );
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );

				winding.Rotate( mainBounds.GetCenter(), rotationOffset );
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );

				winding.Rotate( mainBounds.GetCenter(), rotationOffset );
				gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0.0f, 0.0f, 64.0f + depth ), true, colorWhite, decalInfoArrows->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );
			}

			// the grid
			int maxX, maxY;
			mask->GetDimensions( maxX, maxY );

			sdDeployMask::extents_t expandedExtents;

			expandedExtents.minx = Max( 0, extents.minx - 2 );
			expandedExtents.miny = Max( 0, extents.miny - 2 );

			expandedExtents.maxx = Min( maxX, extents.maxx + 2 );
			expandedExtents.maxy = Min( maxY, extents.maxy + 2 );

			int w = expandedExtents.maxx - expandedExtents.minx + 1;
			int h = expandedExtents.maxx - expandedExtents.minx + 1;
			int terrIdx = 0;
			bool regen = false;

			if ( ( w * h ) > sizeof( lastTerritory ) / sizeof( lastTerritory[0] ) ) {
				common->Warning( "test territory size too large" );
				return;
			}

			for ( int i = expandedExtents.minx; i <= expandedExtents.maxx; i++ ) {
				for ( int j = expandedExtents.miny; j <= expandedExtents.maxy; j++ ) {
					gameDecalInfo_t* info = ( ( i >= extents.minx ) && ( i <= extents.maxx ) && ( j >= extents.miny ) && ( j <= extents.maxy ) ) ? decalInfo : decalInfoOuter;
					if ( !info ) {
						continue;
					}

					sdDeployMask::extents_t localExtents;
					localExtents.minx = i;
					localExtents.maxx = i;
					localExtents.miny = j;
					localExtents.maxy = j;

					mask->GetBounds( localExtents, bounds, heightMap );

					top = bounds.GetCenter();
					top[ 2 ] = bounds.GetMaxs()[ 2 ];

					deployResult_t localResult = localPlayer->CheckBoundsForDeployment( localExtents, *mask, deployableObject, playZoneHeight );

					bool hasTerritory;
					if ( localResult == DR_CLEAR ) {
						hasTerritory = gameLocal.TerritoryForPoint( top, localPlayer->GetGameTeam(), true ) != NULL;
					} else {
						hasTerritory = false;
					}
					if ( hasTerritory != lastTerritory[ terrIdx ] ) {
						regen = true;
					}
					lastTerritory[ terrIdx++ ] = hasTerritory;
				}
			}

			if ( expandedExtents.minx != lastExpandedExtents.minx || expandedExtents.maxx != lastExpandedExtents.maxx ||
				expandedExtents.miny != lastExpandedExtents.miny || expandedExtents.maxy != lastExpandedExtents.maxy || regen ) {
				gameLocal.ResetLoggedDecal( decalHandle );
				gameLocal.ResetLoggedDecal( decalHandleOuter );

				lastExpandedExtents = expandedExtents;

				terrIdx = 0;
				for ( int i = expandedExtents.minx; i <= expandedExtents.maxx; i++ ) {
					for ( int j = expandedExtents.miny; j <= expandedExtents.maxy; j++ ) {
						gameDecalInfo_t* info = ( ( i >= extents.minx ) && ( i <= extents.maxx ) && ( j >= extents.miny ) && ( j <= extents.maxy ) ) ? decalInfo : decalInfoOuter;
						if ( !info ) {
							continue;
						}

						sdDeployMask::extents_t localExtents;
						localExtents.minx = i;
						localExtents.maxx = i;
						localExtents.miny = j;
						localExtents.maxy = j;

						mask->GetBounds( localExtents, bounds, heightMap );

						top = bounds.GetCenter();
						top[ 2 ] = bounds.GetMaxs()[ 2 ];

						//bool hasTerritory = gameLocal.TerritoryForPoint( top, localPlayer->GetGameTeam(), true ) != NULL;
						deployResult_t localResult = localPlayer->CheckBoundsForDeployment( localExtents, *mask, deployableObject, playZoneHeight );

						idVec4 localColor;
						switch ( localResult ) {
							case DR_CLEAR:
								if ( !lastTerritory[ terrIdx ] ) {
									localColor = colorOrange;
								} else {
									localColor = colorGreen;
								}
								break;
							case DR_WARNING:
								localColor = colorOrange;
								break;
							case DR_FAILED:
								localColor = colorDkRed;
								break;
						}

						winding.Clear();
						winding += idVec5( idVec3( bounds.GetMins()[ 0 ], bounds.GetMins()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 0.0f, 0.0f ) );
						winding += idVec5( idVec3( bounds.GetMins()[ 0 ], bounds.GetMaxs()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 0.0f, 1.0f ) );
						winding += idVec5( idVec3( bounds.GetMaxs()[ 0 ], bounds.GetMaxs()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 1.0f, 1.0f ) );
						winding += idVec5( idVec3( bounds.GetMaxs()[ 0 ], bounds.GetMins()[ 1 ], bounds.GetMins()[ 2 ] - depth ), idVec2( 1.0f, 0.0f ) );

	//					gameRenderWorld->DebugBounds( localColor, bounds );
	//					gameRenderWorld->DrawText( va( "%i %i", i, j ), top, 0.5, colorWhite, gameLocal.GetLocalPlayer()->GetRenderView()->viewaxis );

						gameRenderWorld->AddToProjectedDecal( winding, top + idVec3( 0, 0, 64.f + depth ), true, localColor, info->renderEntity.hModel, spawnID, megaTextureMaterials.Begin(), megaTextureMaterials.Num() );
						terrIdx++;
					}
				}
			}
		} else {
			gameLocal.ResetLoggedDecal( decalHandle );
			gameLocal.ResetLoggedDecal( decalHandleOuter );
			lastExpandedExtents.minx = -1;
		}

		idVec3 modelPos = position;
		if ( playZoneHeight ) {
			const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
			if ( heightMap.IsValid() ) {
				modelPos.z = heightMap.GetHeight( modelPos );
			}
		}

		sdDeployRequest::UpdateRenderEntity( deployableRenderEntity, color, modelPos );
		deployableRenderEntity.axis = rotationStart;
		gameRenderWorld->UpdateEntityDef( deployableRenderEntityHandle, &deployableRenderEntity );
	} else {
		if ( deployableRenderEntityHandle >= 0 ) {
			gameRenderWorld->FreeEntityDef( deployableRenderEntityHandle );
			deployableRenderEntityHandle = -1;
		}
		FreeDecals();
	}
}

/*
==============
sdDeployMenu::UsercommandCallback
==============
*/
void sdDeployMenu::UsercommandCallback( usercmd_t& cmd ) {
	if ( modeToggle ) {
		cmd.forwardmove = 0;
		cmd.rightmove = 0;
		cmd.upmove = 0;

		for ( int i = 0; i < 3; i++ ) {
			if ( !gameLocal.IsPaused() ) {
				rotation += SHORT2ANGLE( cmd.angles[ i ] - lockedAngles[ i ] );
			}
			cmd.angles[ i ] = lockedAngles[ i ];
		}
	}
}

/*
==============
sdDeployMenu::SetDeployMode
==============
*/
void sdDeployMenu::SetDeployMode( bool mode ) {
	if ( mode ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();

		for ( int i = 0; i < 3; i++ ) {
			lockedAngles[ i ] = localPlayer->usercmd.angles[ i ];
		}
	}

	modeToggle = mode;
}

/*
==============
sdDeployMenu::AllowRotation
==============
*/
bool sdDeployMenu::AllowRotation( void ) const {
	return deployableObject != NULL ? deployableObject->AllowRotation() : true;
}

/*
==============
sdDeployMenu::HandleInput
==============
*/
void sdDeployMenu::HandleInput( void ) {
}

/*
===============================================================================

	sdRadialMenuModule

===============================================================================
*/

idCVar g_radialMenuMouseInput( "g_radialMenuMouseInput", "2", CVAR_GAME | CVAR_INTEGER | CVAR_ARCHIVE | CVAR_PROFILE, "0 - no mouse input\n1 - mouse input, no view movement\n2 - mouse input, view movement" );

/*
==============
sdRadialMenuModule::HandleGuiEvent
==============
*/
bool sdRadialMenuModule::HandleGuiEvent( const sdSysEvent* event ) {
	// always eat mouse events that come from the controller
	bool controllerMouse = event->IsControllerMouseEvent();
	if ( g_radialMenuMouseInput.GetInteger() == 0 && 
		!controllerMouse &&
		( event->IsMouseEvent() ||
		( event->IsMouseButtonEvent() ) ) ) {
		return false;
	}

	// ignore mouse movement if it's a vertical quick chat
	if ( g_radialMenuStyle.GetInteger() == 1 && event->IsMouseEvent() ) {
		return false;
	}

	sdUserInterfaceLocal* ui = GetGui();
	bool retVal = ui ? ui->PostEvent( event ) : false;

	if ( !retVal ) {
		retVal |= sdHudModule::HandleGuiEvent( event );
	}

	if( controllerMouse ) {
		return true;
	}

	return ( g_radialMenuMouseInput.GetInteger() == 2 && event->IsMouseEvent() ) ? false : retVal;
}

/*
==============
sdRadialMenuModule::DoWeaponLock
==============
*/
bool sdRadialMenuModule::DoWeaponLock( void ) const {
	return g_radialMenuMouseInput.GetInteger() != 0;
}

/*
============
sdRadialMenuModule::GetSensitivity
============
*/
bool sdRadialMenuModule::GetSensitivity( float& x, float& y ) {
	if( g_radialMenuMouseInput.GetInteger() == 0 ) {
		return false;
	}

	if( g_radialMenuMouseInput.GetInteger() == 2 ) {
		return false;		
	}

	x = 0.0f;
	y = 0.0f;
	return true;
}

/*
============
sdRadialMenuModule::UsercommandCallback
============
*/
void sdRadialMenuModule::UsercommandCallback( usercmd_t& cmd ) {
	/*
	if( g_radialMenuMouseInput.GetInteger() > 0 ) {
		cmd.buttons.btn.attack		= false;
		cmd.buttons.btn.altAttack	= false;
	}
	*/
}

/*
===============================================================================

	sdChatMenu

===============================================================================
*/

/*
==============
sdChatMenu::HandleGuiEvent
==============
*/
bool sdChatMenu::HandleGuiEvent( const sdSysEvent* event ) {
	if ( sdHudModule::HandleGuiEvent( event ) ) {
		return true;
	}

	sdUserInterfaceLocal* ui = GetGui();
	return ui ? ui->PostEvent( event ) : false;
}

/*
===============================================================================

	sdTakeViewNoteMenu

===============================================================================
*/

/*
==============
sdTakeViewNoteMenu::HandleGuiEvent
==============
*/
bool sdTakeViewNoteMenu::HandleGuiEvent( const sdSysEvent* event ) {
	if ( sdHudModule::HandleGuiEvent( event ) ) {
		return true;
	}

	sdUserInterfaceLocal* ui = GetGui();
	return ui ? ui->PostEvent( event ) : false;
}

/*
===============================================================================

	sdWeaponSelectionMenu

===============================================================================
*/

/*
============
sdWeaponSelectionMenu::UpdateGui
update here to avoid needless lookups
============
*/
void sdWeaponSelectionMenu::UpdateGui( void ) {
}

/*
============
sdWeaponSelectionMenu::UsercommandCallback
============
*/
void sdWeaponSelectionMenu::UsercommandCallback( usercmd_t& cmd ) {
	if( switchActive == AT_ENABLED ) {
		cmd.buttons.btn.attack = false;
		cmd.buttons.btn.altAttack = false;
	}
}

/*
============
sdWeaponSelectionMenu::DoWeaponLock
============
*/
bool sdWeaponSelectionMenu::DoWeaponLock( void ) const {
	return switchActive == AT_ENABLED;
}

/*
============
sdWeaponSelectionMenu::SetSwitchActive
============
*/
void sdWeaponSelectionMenu::SetSwitchActive( eActivationType set ) {
	if ( switchActive == set ) {
		return;
	}

	// jrad - if the menu is deactivated by the fire key, keep the attack from 
	// "leaking" through - otherwise, don't interrupt the player's firing
	if( switchActive != AT_DIRECT_SELECTION && set == AT_DISABLED ) {
		idPlayer* player = gameLocal.GetLocalPlayer();
		player->SetClientWeaponLock( true );
		player->oldButtons.btn.attack = true;
		player->oldButtons.btn.altAttack = true;
	}

	switchActive = set;
	UpdateGui();

}

/*
============
sdPostProcess::DrawPost
============
*/
void sdPostProcess::DrawPost( void ) {
	if ( g_skipPostProcess.GetBool() || sdAtmosphere::currentAtmosphere == NULL ) {
		return;
	}
	sdHudModule::Draw();
}

/*
============
sdQuickChatMenu::OnActivate
============
*/
void sdQuickChatMenu::OnActivate( void ) {
	sdRadialMenuModule::OnActivate();
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL ) {
		const sdCrosshairInfo& info = player->GetCrosshairInfoDirect();
		if( info.IsValid() ) {
			gameLocal.localPlayerProperties.SetContextEntity( info.GetEntity() );
		}		
	}
}

/*
============
sdQuickChatMenu::OnCancel
============
*/
void sdQuickChatMenu::OnCancel( void ) {
	sdRadialMenuModule::OnCancel();
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player != NULL ) {
		gameLocal.localPlayerProperties.SetContextEntity( NULL );
	}
}


/*
===============================================================================

sdFireTeamMenu

===============================================================================
*/

/*
==============
sdFireTeamMenu::HandleGuiEvent
==============
*/
bool sdFireTeamMenu::HandleGuiEvent( const sdSysEvent* event ) {
	sdUserInterfaceLocal* ui = GetGui();
	if( ui && ui->PostEvent( event ) ) {
		return true;
	}

	return sdHudModule::HandleGuiEvent( event );		
}

