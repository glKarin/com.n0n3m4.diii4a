// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "CommandMapPost.h"
#include "../Player.h"
#include "../CommandMapInfo.h"

#include "../guis/UserInterfaceLocal.h"
#include "../guis/UIWindow.h"

#include "../../sys/sys_local.h"

/*
================
sdCommandMapPost::GetWorldToMapTransform
================
*/
void sdCommandMapPost::WorldToMapTransform( const idVec2& inverseSize, idVec2& coords, const idVec2& cameraPosition, const idVec2& screenExtents ) {
	coords.x -= cameraPosition.x;
	coords.x *= inverseSize.x;
	coords.x *= screenExtents.x;
	coords.x += screenExtents.x * 0.5f;

	coords.y -= cameraPosition.y;
	coords.y *= inverseSize.y;
	coords.y *= -screenExtents.y;
	coords.y += screenExtents.y * 0.5f;
}

/*
================
sdCommandMapPost::ClampMapCamera
================
*/
void sdCommandMapPost::ClampMapCamera( const idVec2& org, const idVec2& size, idVec2& cameraPosition, const sdBounds2D& bounds ) {
	idVec2 scale = size * -0.5f;

	cameraPosition.x = idMath::ClampFloat( bounds[ 0 ].x - scale.x, bounds[ 1 ].x + scale.x, org.x );
	cameraPosition.y = idMath::ClampFloat( bounds[ 0 ].y - scale.y, bounds[ 1 ].y + scale.y, org.y );
}

/*
================
sdCommandMapPost::HandleLocalPlayerCommandMapInput
================
*/
bool sdCommandMapPost::HandleLocalPlayerCommandMapInput( sdUIWindow* window, const sdSysEvent* event ) {

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return false;
	}

	if ( !event->IsMouseButtonEvent() || event->GetMouseButton() != M_MOUSE1 || !event->IsButtonDown() ) {
		return false;
	}

	const idVec4& rect = window->GetWorldRect();
	idVec2 screenPos( rect.x, rect.y );
	idVec2 screenExtents( rect.z, rect.w );

	float scale = 1.f;	

	// jrad - allow local overrides for vehicle cockpits, warroom maps, etc
	sdProperties::sdProperty* prop =  window->GetUI()->GetState().GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
	if( !prop ) {
		sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" );
		if( scope ) {
			prop = scope->GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
		}		
	}
	if( prop ) {
		scale = *prop->value.floatValue;
	}

	idVec3 worldPos = player->GetPhysics()->GetOrigin();

	const sdPlayZone* playZone = GetPlayZone( window->GetUI(), worldPos );
	if ( !playZone ) {
		return false;
	}

	idVec2 size = playZone->GetSize() * scale;
	idVec2 inverseSize;
	inverseSize.x = 1.0f / size.x;
	inverseSize.y = 1.0f / size.y;

	idVec2 worldCamera;
	ClampMapCamera( worldPos.ToVec2(), size, worldCamera, playZone->GetBounds() );

	// Gordon: Draw Map out from camera
	// [
	
	idVec2 worldMins		= playZone->GetBounds().GetMins();
	idVec2 worldMaxs		= playZone->GetBounds().GetMaxs();
	idVec2 worldSpaceBounds = worldMaxs - worldMins;

	WorldToMapTransform( inverseSize, worldMins, worldCamera, screenExtents );
	WorldToMapTransform( inverseSize, worldMaxs, worldCamera, screenExtents );

	// swap the y's as the transforms come out the other way about
	Swap( worldMins.y, worldMaxs.y );

	worldMaxs -= worldMins;
	
	float radius = screenExtents.x * 0.5f;
	idVec2 mapCenter( rect.x + rect.z * 0.5f, rect.y + rect.w * 0.5f );

	bool fullSize = idMath::Fabs( scale - 1.0f ) < idMath::FLT_EPSILON;
	// ]

	idVec2 cursorPos( window->GetUI()->cursorPos );

	sdCommandMapInfo* info;
	for ( info = sdCommandMapInfoManager::GetInstance().GetIcons(); info; info = info->GetActiveNode().Next() ) {
		const char* message = info->GetGuiMessage();
		if ( !*message ) {
			continue;
		}

		bool known = true;

		idEntity* cmEntity = info->GetOwner();
		if ( cmEntity ) {
			known = cmEntity->SendCommandMapInfo( player );
			if ( !known ) {
				continue;
			}
		} else {
			continue;
		}

		idVec2 coords;
		info->GetOrigin( coords );
		WorldToMapTransform( inverseSize, coords, worldCamera, screenExtents );
		coords += screenPos;

		if ( !fullSize ) {
			idVec2 diff = coords - mapCenter;
			if ( diff.LengthSqr() > radius ) {
				continue;
			}
		}

		// Gordon: FIXME: Info should handle this check
		idVec2 halfSize = info->GetSize() * 0.5f;
		if ( cursorPos.x < coords.x - halfSize.x || cursorPos.x > coords.x + halfSize.x ) {
			continue;
		}
		if ( cursorPos.y < coords.y - halfSize.y || cursorPos.y > coords.y + halfSize.y ) {
			continue;
		}

		sdGuiInterface* guiInterface = cmEntity->GetGuiInterface();
		if ( !guiInterface ) {
			gameLocal.Warning( "sdCommandMapPost::HandleLocalPlayerCommandMapInput Gui Message on Command Map Icon on Entity with no Gui Interface" );
			continue;
		}

		if ( gameLocal.isClient ) {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_GUISCRIPT );
			msg.WriteBits( gameLocal.GetSpawnId( cmEntity ), 32 );
			msg.WriteString( message );
			msg.Send();
		}

		gameLocal.HandleGuiScriptMessage( player, cmEntity, message );

		return true;
	}

	return false;
}



/*
============
sdCommandMapPost::DrawLocalPlayerCommandMapInfo
============
*/
void sdCommandMapPost::DrawLocalPlayerCommandMapInfo( sdUserInterfaceLocal* ui, float x, float y, float w, float h ) {
	DrawLocalPlayerCommandMapInfo( ui, x, y, w, h, vec2_zero );
}

/*
============
sdCommandMapPost::GetPlayZone
============
*/
const sdPlayZone* sdCommandMapPost::GetPlayZone( sdUserInterfaceLocal* ui, const idVec3& worldPos ) {
	int playzoneId = -1;

	if ( sdProperties::sdProperty* prop = ui->GetState().GetProperty( "playZone", sdProperties::PT_FLOAT )) {
		playzoneId = idMath::Ftoi( *prop->value.floatValue );
	}

	const sdPlayZone* zone = NULL;
	if( playzoneId != -1 ) {
		zone = gameLocal.GetChoosablePlayZone( playzoneId );
	}
	if( zone == NULL ) {
		zone = gameLocal.GetPlayZone( worldPos, sdPlayZone::PZF_COMMANDMAP );
	}
	return zone;
}

/*
================
sdCommandMapPost::DrawLocalPlayerCommandMapInfo
================
*/
void sdCommandMapPost::DrawLocalPlayerCommandMapInfo( sdUserInterfaceLocal* ui, float x, float y, float w, float h, const idVec2& offset ) {

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( player == NULL && !gameLocal.serverIsRepeater ) {
		return;
	}

	sdTeamInfo* team = player == NULL ? NULL : player->GetGameTeam();

	idVec2	screenPos( x, y );
	idVec2	screenExtents( w, h );

	float	scale = 1.f;
	bool	drawSquare = false;	

	// jrad - allow local overrides for vehicle cockpits, warroom maps, etc
	sdProperties::sdProperty* prop = ui->GetState().GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
	if( !prop ) {
		sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" );
		if( scope ) {
			prop = scope->GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
		}		
	}
	if( prop != NULL ) {
		scale = *prop->value.floatValue;
	}



	bool fullSize = idMath::Fabs( scale - 1.0f ) < idMath::FLT_EPSILON;

	if ( sdProperties::sdProperty* prop = ui->GetState().GetProperty( "drawSquare", sdProperties::PT_FLOAT )) {
		drawSquare = *prop->value.floatValue == 1.0f;		
	}
	if( drawSquare ) {
		fullSize = true;
	}

	renderView_t repeaterView;
	if ( gameLocal.serverIsRepeater ) {
		gameLocal.playerView.CalculateRepeaterView( repeaterView );
	}

	idVec3 worldPos;
	if ( gameLocal.serverIsRepeater ) {
		worldPos = repeaterView.vieworg;
	} else {
		if ( player->GetRemoteCamera() != NULL ) {
			worldPos = player->GetRenderView()->vieworg;
		} else {
			worldPos = player->GetPhysics()->GetOrigin();
		}
	}

	const sdPlayZone* playZone = GetPlayZone( ui, worldPos );
	
	if ( playZone == NULL ) {
		return;
	}

	idVec2 size = playZone->GetSize() * scale;

	idVec2 inverseSize;
	inverseSize.x = 1.0f / size.x;
	inverseSize.y = 1.0f / size.y;

	idVec2 worldCamera;
	ClampMapCamera( worldPos.ToVec2(), size, worldCamera, playZone->GetBounds() );

	// Gordon: Draw Map out from camera
	// [
	idVec2 worldMins		= playZone->GetBounds().GetMins();
	idVec2 worldMaxs		= playZone->GetBounds().GetMaxs();
	idVec2 worldSpaceBounds	= worldMaxs - worldMins;

	float xOffset = ( ( worldCamera.x - worldMins.x ) / worldSpaceBounds.x );
	float yOffset = -( ( worldCamera.y - worldMins.y ) / worldSpaceBounds.y );

	float radius = screenExtents.x * 0.5f;

	float rotAngle = 0.0f;
	if( !fullSize ) {
		if( sdCommandMapInfo::g_rotateCommandMap.GetBool() ) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( gameLocal.serverIsRepeater ) {
				rotAngle = repeaterView.viewaxis.ToAngles().yaw - 90.0f;
			} else if ( player != NULL ) {
				idMat3 pAxis;
				player->GetRenderViewAxis( pAxis );
				rotAngle = pAxis.ToAngles().yaw - 90.0f;
			}
		}
	}

	const idMaterial* cmMaterial = playZone->GetCommandMapMaterial();

	if ( fullSize && !drawSquare ) {
		deviceContext->DrawMaterial( x, y, w, h, cmMaterial, colorWhite );
	} else if( drawSquare ) {
		deviceContext->DrawMaterial( x, y, w, h, cmMaterial, colorWhite, scale, scale, xOffset - 0.5f * scale, yOffset - 0.5f * scale );
	} else {
		idVec2 radii( w * 0.5f, h * 0.5f );
		deviceContext->DrawCircleMaterial( x + radii.x, y + radii.y, radii, idWinding2D::MAX_POINTS, idVec4( xOffset, yOffset, 0.5f * scale, 0.5f * scale ), cmMaterial, colorWhite, rotAngle );
	}
	// ]
}

/*
================
sdCommandMapPost::DrawLocalPlayerCommandMapInfo_Icons
================
*/
void sdCommandMapPost::DrawLocalPlayerCommandMapInfo_Icons( sdUserInterfaceLocal* ui, float x, float y, float w, float h ) {

	idPlayer* player = gameLocal.GetLocalViewPlayer();
	if ( player == NULL && !gameLocal.serverIsRepeater ) {
		return;
	}

	idVec2 screenPos( x, y );
	idVec2 screenExtents( w, h );

	float scale = 1.f;
	bool drawSquare = false;	

	// jrad - allow local overrides for vehicle cockpits, warroom maps, etc
	sdProperties::sdProperty* prop = ui->GetState().GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
	if( prop == NULL ) {
		sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" );
		if( scope != NULL ) {
			prop = scope->GetProperty( "mapZoomLevel", sdProperties::PT_FLOAT );
		}		
	}
	if( prop ) {
		scale = *prop->value.floatValue;
	}

	bool fullSize = idMath::Fabs( scale - 1.0f ) < idMath::FLT_EPSILON;

	if ( sdProperties::sdProperty* prop = ui->GetState().GetProperty( "drawSquare", sdProperties::PT_FLOAT )) {
		drawSquare = *prop->value.floatValue == 1.0f;		
	}

	if( drawSquare ) {
		fullSize = true;
	}

	renderView_t repeaterView;
	if ( gameLocal.serverIsRepeater ) {
		gameLocal.playerView.CalculateRepeaterView( repeaterView );
	}

	idMat2 rotation( mat2_identity );
	float rotAngle = 0.0f;
	if( !fullSize) {
		if( sdCommandMapInfo::g_rotateCommandMap.GetBool() ) {
			idPlayer* player = gameLocal.GetLocalPlayer();
			if ( gameLocal.serverIsRepeater ) {
				rotAngle = repeaterView.viewaxis.ToAngles().yaw - 90.0f;
				rotation.Rotation( DEG2RAD( rotAngle ) );
				rotation.TransposeSelf();
			} else if ( player != NULL ) {
				idMat3 pAxis;
				player->GetRenderViewAxis( pAxis );
				rotAngle = pAxis.ToAngles().yaw - 90.0f;
				rotation.Rotation( DEG2RAD( rotAngle ));
				rotation.TransposeSelf();
			}
		}
	}

	idVec3 worldPos;
	if ( gameLocal.serverIsRepeater ) {
		worldPos = repeaterView.vieworg;
	} else {
		if ( player->GetRemoteCamera() != NULL ) {
			worldPos = player->GetRenderView()->vieworg;
		} else {
			worldPos = player->GetPhysics()->GetOrigin();
		}
	}

	const sdPlayZone* playZone = GetPlayZone( ui, worldPos );
	if ( playZone == NULL ) {
		return;
	}

	idVec2 size = playZone->GetSize() * scale;

	idVec2 inverseSize;
	inverseSize.x = 1.0f / size.x;
	inverseSize.y = 1.0f / size.y;

	idVec2 worldCamera;
	ClampMapCamera( worldPos.ToVec2(), size, worldCamera, playZone->GetBounds() );

	// Gordon: Draw Map out from camera
	// [
	idVec2 worldMins		= playZone->GetBounds().GetMins();
	idVec2 worldMaxs		= playZone->GetBounds().GetMaxs();
	idVec2 worldSpaceBounds	= worldMaxs - worldMins;

	idVec2 mapCamera = worldCamera;
	WorldToMapTransform( inverseSize, worldMins, worldCamera, screenExtents );
	WorldToMapTransform( inverseSize, worldMaxs, worldCamera, screenExtents );
	WorldToMapTransform( inverseSize, mapCamera, worldCamera, screenExtents );

	// swap the y's as the transforms come out the other way about
	Swap( worldMins.y, worldMaxs.y );

	worldMaxs -= worldMins;

	idVec2 radius = screenExtents * 0.5f;
	idVec2 center( w * 0.5f, h * 0.5f );
	idVec2 mapCenter( x + center.x, y + center.y );
	
	float sizeAdjustment = 1.0f;
	if ( sdProperties::sdProperty* prop = ui->GetState().GetProperty( "iconScaleAdjustment", sdProperties::PT_FLOAT )) {
		sizeAdjustment = *prop->value.floatValue;
	}

	sdCommandMapInfo* info;
	for ( info = sdCommandMapInfoManager::GetInstance().GetIcons(); info; info = info->GetActiveNode().Next() ) {
		bool known = true;

		const sdRequirementContainer& requirements = info->GetRequirements();
		if ( requirements.HasRequirements() ) {
			if ( player == NULL ) {
				continue;
			}
			if ( !requirements.Check( player, info->GetOwner() ) ) {
				continue;
			}
		}
		
		if ( info->OnlyInFullView() && scale < 1.0f ) {
			continue;
		}

		if ( !info->IsAlwaysKnown() ) {
			idEntity* cmEntity = info->GetOwner();
			if ( cmEntity ) {
				bool sameTeam = player->GetEntityAllegiance( cmEntity ) == TA_FRIEND;
				bool enemyAlwaysKnown = info->EnemyAlwaysKnown();
				if ( enemyAlwaysKnown ) {
					if ( !sameTeam ) {
						known = true;
					} else {
						known = cmEntity->IsInRadar( player );
					}
				} else {
					known = cmEntity->SendCommandMapInfo( player );
				}

				if ( !known ) {
					continue;
				}

				if ( known && !enemyAlwaysKnown ) {
					known = info->EnemyOnly() ^ sameTeam;
				}
			}
		}		

		idVec2 coords;
		info->GetOrigin( coords );
		WorldToMapTransform( inverseSize, coords, worldCamera, screenExtents );

		if( coords.x < worldMins.x || coords.x > worldMaxs.x ) {
			continue;
		}

		if( coords.y < worldMins.y || coords.y > worldMaxs.y ) {
			continue;
		}

		coords -= center;
		coords *= rotation;
		coords += center;

		coords += screenPos;

		if ( !fullSize && !drawSquare ) {
			idVec2 diff = coords - mapCenter;
			float length = diff.Normalize();

			if ( length >= radius.x || length >= radius.y ) {
				if ( info->CanAdjustPosition() ) {
					coords = idVec2( diff.x * radius.x, diff.y * radius.y );
					coords += mapCenter;

					known = false;
				}
			}
		}

		if ( !info->IsAlwaysKnown() ) {
			if ( drawSquare && ( coords.x < x || coords.y < y || coords.x > x + w || coords.y > y + w ) ) {
				continue;
			}
		}
		for ( int i = 6; i < 12; i++ ) {
			info->SetShaderParm( i, ui->GetShaderParms( i ) );
		}
		info->Draw( player, coords, screenPos, screenExtents, inverseSize, known, sizeAdjustment, rotation, rotAngle, fullSize );
	}
}
