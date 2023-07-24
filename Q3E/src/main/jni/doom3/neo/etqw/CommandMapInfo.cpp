// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "CommandMapInfo.h"
#include "Player.h"
#include "vehicles/Transport.h"
#include "rules/GameRules.h"
#include "../renderer/DeviceContext.h"

idCVar sdCommandMapInfo::g_rotateCommandMap( "g_rotateCommandMap", "1", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE | CVAR_PROFILE, "Rotate the command map around the player" );

/*
===============================================================================

	sdCommandMapInfo

===============================================================================
*/

/*
================
sdCommandMapInfo::sdCommandMapInfo
================
*/
sdCommandMapInfo::sdCommandMapInfo( idEntity* owner, int sort ) {
	_owner = owner;
	_activeNode.SetOwner( this );

	_drawMode			= DM_MATERIAL;
	_colorMode			= CM_NORMAL;
	_scaleMode			= SM_FIXED;
	_positionMode		= PM_ENTITY;

	_material			= NULL;
	_unknownMaterial	= NULL;
	_fireteamMaterial	= NULL;
	_flashMaterial		= NULL;

	_sort				= sort;
	_size				= idVec2( 24, 24 );
	_unknownSize		= idVec2( 10, 10 );
	_color				= colorWhite;
	_angle				= 0.f;
	_sides				= 16;
	_flags				= 0;
	_flagsBackup		= -1;
	_arcAngle			= 0.0f;

	_font				= -1;
	_textScale			= 16.0f;

	_flashEndTime		= 0;

	_shaderParms.SetNum( MAX_ENTITY_SHADER_PARMS - 4 );
	for ( int i = 0; i < MAX_ENTITY_SHADER_PARMS - 4; i++ ) {
		_shaderParms[i] = 0.0f;
	}



	_origin.Zero();

	Show();
}

/*
================
sdCommandMapInfo::~sdCommandMapInfo
================
*/
sdCommandMapInfo::~sdCommandMapInfo( void ) {
	FreeFont();
}

/*
============
sdCommandMapInfo::SetFont
============
*/
void sdCommandMapInfo::SetFont( const char* fontName ) {
	FreeFont();
	_font = deviceContext->FindFont( fontName );
}

/*
============
sdCommandMapInfo::FreeFont
============
*/
void sdCommandMapInfo::FreeFont( void ) {
	if ( _font == -1 ) {
		return;
	}
	deviceContext->FreeFont( _font );
	_font = -1;
}

/*
============
sdCommandMapInfo::SetShaderParm
============
*/
void sdCommandMapInfo::SetShaderParm( int index, float value ) {	
	_shaderParms[ index - 4 ] = value;
}

/*
================
sdCommandMapInfo::SetSort
================
*/
void sdCommandMapInfo::SetSort( int sort ) {
	_sort = sort;
}

/*
================
sdCommandMapInfo::Show
================
*/
void sdCommandMapInfo::Show( void ) {
	sdCommandMapInfoManager::GetInstance().SortIntoList( this );
}

/*
================
sdCommandMapInfo::Hide
================
*/
void sdCommandMapInfo::Hide( void ) {
	_activeNode.Remove();
}

/*
================
sdCommandMapInfo::Flash
================
*/
void sdCommandMapInfo::Flash( const idMaterial* material, int msec, int setFlags ) {
	if ( material != NULL ) {
		_flashMaterial = material;
	} else {
		_flashMaterial = NULL;
	}

	if ( setFlags != -1 ) {
		if ( gameLocal.ToGuiTime( gameLocal.time ) > _flashEndTime ) {
			_flagsBackup = _flags;
		}
		_flags = setFlags;
	}

	_flashEndTime = gameLocal.ToGuiTime( gameLocal.time ) + msec;
}

/*
================
sdCommandMapInfo::GetOrigin
================
*/
void sdCommandMapInfo::GetOrigin( idVec2& out ) const {
	switch ( _positionMode ) {
		case PM_ENTITY: {
			idEntity* owner = _owner;
			if ( owner ) {
				idPlayer *player = owner->Cast<idPlayer>();
				idEntity *remote = player != NULL ? player->GetRemoteCamera() : NULL;
				if ( player != NULL && remote != NULL && _flags & CMF_FOLLOWREMOTECAMERAORIGIN ) {
					out = player->GetRenderView()->vieworg.ToVec2();
				} else {
					out = owner->GetPhysics()->GetOrigin().ToVec2();		
				}
			} else {
				out = vec2_origin;
			}
			return;
		}
	}

	out = _origin;

}

/*
================
sdCommandMapInfo::Draw
================
*/
void sdCommandMapInfo::Draw( idPlayer* player, const idVec2& position, const idVec2& screenPos, const idVec2& screenSize, const idVec2& mapScale, bool known, float sizeScale, const idMat2& rotation, float angle_, bool fullSize ) {

	bool flashing = gameLocal.ToGuiTime( gameLocal.time ) > _flashEndTime ? false : true;

	// restore original flags after we're done with flashing
	if ( !flashing && _flagsBackup != -1 ) {
		_flags = _flagsBackup;
		_flagsBackup = -1;

		// avoid brief flash of known material.
		if ( !( _flags & CMF_ENEMYONLY ) ) {
			known = false;
		}
	}

	if ( _flags & CMF_TEAMONLY ) {
		if ( player->GetEntityAllegiance( _owner ) != TA_FRIEND ) {
			return;
		}
	}

	bool sameFireTeam = false;
	if ( _owner.IsValid() ) {
		if ( player != NULL ) {
			idPlayer* playerOwner = GetOwner()->Cast< idPlayer >();
			if ( playerOwner != NULL ) {
				sdFireTeam* localFireTeam = gameLocal.rules->GetPlayerFireTeam( player->entityNumber );

				sdFireTeam *myFireTeam;
				if ( playerOwner->GetGameTeam() == player->GetGameTeam() || !playerOwner->IsDisguised() || playerOwner->GetDisguiseEntity() == NULL ) {
					myFireTeam = gameLocal.rules->GetPlayerFireTeam( playerOwner->entityNumber );
				} else {
					myFireTeam = gameLocal.rules->GetPlayerFireTeam( playerOwner->GetDisguiseEntity()->entityNumber );
				}
				
				if ( localFireTeam != NULL && myFireTeam == localFireTeam ) {
					sameFireTeam = true;
				}
			}
		}
	}


	if ( !sameFireTeam && IsFireTeamOnly() ) {
		return;
	}

	if ( sameFireTeam && IsFireTeamKnown() ) {
		known = true;
	}

	if ( _flags & CMF_ONLYSHOWKNOWN ) {
		if ( !known ) {
			return;
		}
	}

	const idMaterial* material;
	if ( !flashing || _flashMaterial == NULL ) {
		if ( known || _flags & CMF_ALWAYSKNOWN ) {
			material = sameFireTeam && _fireteamMaterial != NULL ? _fireteamMaterial : _material;
		} else {
			material = _unknownMaterial;
		}
	} else {
		material = _flashMaterial;
	}

	static idVec4 drawColor;
	switch ( _colorMode ) {
		case CM_FRIENDLY:
			if ( player == NULL || !player->GetGameTeam() ) {
				drawColor = idEntity::GetColorForAllegiance( TA_NEUTRAL );
			} else {
				if ( _flags & CMF_FIRETEAMCOLORING && sameFireTeam ) {
					drawColor = _owner->GetFireteamColor();
				} else {
					drawColor = idEntity::GetColorForAllegiance( TA_FRIEND );
				}
			}
			break;
		default:
		case CM_NORMAL:
			drawColor = _color;
			break;
		case CM_ALLEGIANCE:
			if ( _flags & CMF_FIRETEAMCOLORING && sameFireTeam ) {
				drawColor = _owner->GetFireteamColor();
			} else {
				drawColor = _owner->GetColorForAllegiance( player );
			}
			break;
	}

	if ( flashing ) { 
		drawColor.w = idMath::Cos( gameLocal.ToGuiTime( gameLocal.time ) * 0.02f ) * 0.5f + 0.5f;
	}

	idVec2 drawSize = known ? _size : _unknownSize;

	switch ( _scaleMode ) {
		case SM_WORLD:
			drawSize.x *= ( screenSize.x * mapScale.x );
			drawSize.y *= ( screenSize.y * mapScale.y );
			break;
		default:
			drawSize *= sizeScale;
			break;
	}

	idVec2 drawPos = position;

	switch ( _positionMode ) {
		case PM_ENTITY: {
			drawPos += _origin;
			break;
		}
	}

	deviceContext->SetRegisters( _shaderParms.Begin() );

	switch ( _drawMode ) {
		case DM_CIRCLE:
			if ( material != NULL ) {
				deviceContext->DrawCircleMaterialMasked( drawPos[ 0 ], drawPos[ 1 ], drawSize, _sides, vec4_zero, material, drawColor, 0.0f, screenPos[ 0 ], screenPos[ 1 ], screenSize.x, screenSize.y );
			}
			break;
		case DM_MATERIAL: {
			if ( material != NULL ) {
				idVec2 _drawPos = drawPos - idVec2( drawSize.x * 0.5f, drawSize.y * 0.5f );

				float u0 = (_drawPos[ 0 ] - screenPos[ 0 ]) / screenSize.x;
				float v0 = (_drawPos[ 1 ] - screenPos[ 1 ]) / screenSize.y;
				float u1 = (_drawPos[ 0 ] + drawSize.x - screenPos[ 0 ]) / screenSize.x;
				float v1 = (_drawPos[ 1 ] + drawSize.y - screenPos[ 1 ]) / screenSize.y;
				if( sdCommandMapInfo::g_rotateCommandMap.GetBool() && !fullSize && _scaleMode == SM_WORLD ) {
//					deviceContext->DrawRotatedMaterial( angle_, _drawPos, drawSize, material, drawColor );
					deviceContext->DrawMaskedMaterial(_drawPos[ 0 ], _drawPos[ 1 ], drawSize.x, drawSize.y, u0, v0, u1, v1, material, drawColor, 1.f, 1.f, 0.f, 0.f, angle_ );
				} else {
					deviceContext->DrawMaskedMaterial( _drawPos[ 0 ], _drawPos[ 1 ], drawSize.x, drawSize.y, u0, v0, u1, v1, material, drawColor );
//					deviceContext->DrawMaterial( screenPos[ 0 ], screenPos[ 1 ], screenSize.x, screenSize.y, material, drawColor, 120.f*1.f/2048.f, 120.f*1.f/2048.f, wp.x/worldSpaceBounds.x, wp.y/worldSpaceBounds.y );
				}
				if ( _flags & CMF_DROPSHADOW ) {
					deviceContext->DrawMaterial( _drawPos[ 0 ] + 1.0f, _drawPos[ 1 ] + 1.0f, drawSize.x, drawSize.y, material, colorBlack );
				}
			}
			break;
		}
		case DM_ROTATED_MATERIAL: {
			if ( material != NULL ) {
				float angle = 270.f;
				if ( _flags & CMF_FOLLOWROTATION ) {
					idPlayer* localPlayer = gameLocal.GetLocalPlayer();
					if ( localPlayer != NULL ) {
						if ( localPlayer == _owner ) {
							if( !sdCommandMapInfo::g_rotateCommandMap.GetBool() || fullSize ) {
								idMat3 pAxis;
								if ( _flags & CMF_PLAYERROTATIONONLY && localPlayer->GetRemoteCamera() != NULL ) {
									pAxis = player->viewAxis;
								} else {
									localPlayer->GetRenderViewAxis( pAxis );
								}
								angle = -pAxis.ToAngles().yaw;
							} else {
								angle = 270.0f;
							}
						} else {
							if( !sdCommandMapInfo::g_rotateCommandMap.GetBool() || fullSize ) {
								angle = -_owner->GetRenderEntity()->axis.ToAngles().yaw;
							} else {
								idMat3 pAxis;
								if ( _flags & CMF_PLAYERROTATIONONLY && localPlayer->GetRemoteCamera() != NULL ) {
									pAxis = localPlayer->viewAxis;
								} else {
									localPlayer->GetRenderViewAxis( pAxis );
								}
								angle = -_owner->GetRenderEntity()->axis.ToAngles().yaw + pAxis.ToAngles().yaw - 90.0f;
							}
						}
					} else if ( gameLocal.serverIsRepeater ) {
						// FIXME: ETQWTV
					}
				} else {
					angle = _angle;
				}
				float u0 = (drawPos[ 0 ] - screenPos[ 0 ]) / screenSize.x;
				float v0 = (drawPos[ 1 ] - screenPos[ 1 ]) / screenSize.y;
				float u1 = (drawPos[ 0 ] + drawSize.x - screenPos[ 0 ]) / screenSize.x;
				float v1 = (drawPos[ 1 ] + drawSize.y - screenPos[ 1 ]) / screenSize.y;
				idVec2 _drawPos = drawPos - idVec2( drawSize.x * 0.5f, drawSize.y * 0.5f );
				deviceContext->DrawMaskedMaterial(_drawPos[ 0 ], _drawPos[ 1 ], drawSize.x, drawSize.y, u0, v0, u1, v1, material, drawColor, 1.f, 1.f, 0.f, 0.f, angle );
				//deviceContext->DrawRotatedMaterial( angle, _drawPos, idVec2( drawSize.x, drawSize.y ), material, drawColor );
			}
			break;
		}
		case DM_ARC:
			if ( material != NULL ) {
				float angle = 270.f;
				if ( _flags & CMF_FOLLOWROTATION ) {
					idPlayer* localPlayer = gameLocal.GetLocalPlayer();
					if ( localPlayer != NULL ) {
						if ( _owner == localPlayer ) {
							if( !sdCommandMapInfo::g_rotateCommandMap.GetBool() || fullSize ) {
								idMat3 pAxis;
								if ( _flags & CMF_PLAYERROTATIONONLY && localPlayer->GetRemoteCamera() != NULL ) {
									pAxis = localPlayer->viewAxis;
								} else {
									localPlayer->GetRenderViewAxis( pAxis );
								}
								angle = -pAxis.ToAngles().yaw;
							} else {
								angle = 270.0f;
							}
						} else {
							if( !sdCommandMapInfo::g_rotateCommandMap.GetBool() || fullSize ) {
								angle = -_owner->GetRenderEntity()->axis.ToAngles().yaw;
							} else {
								idMat3 pAxis;
								if ( _flags & CMF_PLAYERROTATIONONLY && localPlayer->GetRemoteCamera() != NULL ) {
									pAxis = player->viewAxis;
								} else {
									localPlayer->GetRenderViewAxis( pAxis );
								}
								angle = -_owner->GetRenderEntity()->axis.ToAngles().yaw + pAxis.ToAngles().yaw - 90.0f;
							}
						}
					} else {
						// FIXME: ETQWTV
					}
				} else {
					angle = _angle;
				}
//				deviceContext->DrawFilledArc( drawPos[ 0 ], drawPos[ 1 ], drawSize.x, _sides, _arcAngle / 360.0f, 
//					drawColor, angle + ( _arcAngle / 2.0f ), material );
				deviceContext->DrawFilledArcMasked( drawPos[ 0 ], drawPos[ 1 ], drawSize.x, _sides, _arcAngle / 360.0f, 
					drawColor, 
					screenPos[ 0 ], screenPos[ 1 ], screenSize.x, screenSize.y, 
					angle + ( _arcAngle / 2.0f ), material );
			}
			break;
		case DM_TEXT:
			deviceContext->SetColor( drawColor );
			deviceContext->SetFont( _font );
			deviceContext->SetFontSize( _textScale );
			deviceContext->DrawText( _text.c_str(), sdBounds2D( drawPos, drawPos ), DTF_CENTER | DTF_VCENTER | DTF_SINGLELINE | DTF_NOCLIPPING );
			break;
		case DM_CROSSHAIR:
			if ( material != NULL ) {
				float scale = 2.0f;		
				idVec2 stPos( ( drawPos.x - ( drawSize.x * 0.5f * 1.0f / scale ) - screenPos.x ) / screenSize.x, ( drawPos.y - ( drawSize.y * 0.5f * 1.0f / scale ) - screenPos.y ) / screenSize.y );


				float totalX = screenSize.x / drawSize.x * scale;
				float totalY = screenSize.y / drawSize.y * scale;

				// we divide by 10 to bring the coordinates into the 32 byte vertex format's ST range
				// we scale back up in the material
				float minX = ( totalX * -stPos.x ) / 10.0f;
				float maxX = ( totalX * ( 1.0f - stPos.x ) )  / 10.0f;

				float minY = ( totalY * -stPos.y )  / 10.0f;
				float maxY = ( totalY * ( 1.0f - stPos.y ) )  / 10.0f;

				idWinding2D w; 
				w.AddPoint( screenPos.x,					screenPos.y,				minX, minY );
				w.AddPoint( screenPos.x + screenSize.x,		screenPos.y,				maxX, minY );
				w.AddPoint( screenPos.x + screenSize.x,		screenPos.y + screenSize.y, maxX, maxY );
				w.AddPoint( screenPos.x,					screenPos.y + screenSize.y, minX, maxY );

				deviceContext->SetColor( drawColor );
				deviceContext->DrawClippedWinding( w, _material );
			}
	}

}

/*
===============================================================================

	sdCommandMapInfoManagerLocal

===============================================================================
*/

/*
================
sdCommandMapInfoManagerLocal::Init
================
*/
void sdCommandMapInfoManagerLocal::Init( void ) {
	commandMapIcons.AssureSize( MAX_ICONS, NULL );
}

/*
================
sdCommandMapInfoManagerLocal::Shutdown
================
*/
void sdCommandMapInfoManagerLocal::Shutdown( void ) {
	commandMapIcons.DeleteContents( true );
}

/*
================
sdCommandMapInfoManagerLocal::Alloc
================
*/
qhandle_t sdCommandMapInfoManagerLocal::Alloc( idEntity* owner, int sort ) {
	for ( int i = 0; i < MAX_ICONS; i++ ) {
		if ( commandMapIcons[ i ] == NULL ) {
			commandMapIcons[ i ] = new sdCommandMapInfo( owner, sort );
			return i;
		}
	}

	return -1;
}

/*
================
sdCommandMapInfoManagerLocal::OnEntityDeleted
================
*/
void sdCommandMapInfoManagerLocal::OnEntityDeleted( idEntity* ent ) {
	for ( sdCommandMapInfo* icon = activeIcons.Next(); icon != NULL; icon = icon->GetActiveNode().Next() ) {
		if ( icon->GetOwner() == ent ) {
			assert( false );
		}
	}
}

/*
================
sdCommandMapInfoManagerLocal::SortIntoList
smaller values are drawn after larger values
================
*/
void sdCommandMapInfoManagerLocal::SortIntoList( sdCommandMapInfo* info ) {
	int localSort = info->GetSort();

	info->GetActiveNode().Remove();

	sdCommandMapInfo* other;
	for ( other = activeIcons.Next(); other; other = other->GetActiveNode().Next() ) {
		if ( localSort >= other->GetSort() ) {
			info->GetActiveNode().InsertBefore( other->GetActiveNode() );
			return;
		}
	}

	info->GetActiveNode().AddToEnd( activeIcons );
}

/*
================
sdCommandMapInfoManagerLocal::Free
================
*/
void sdCommandMapInfoManagerLocal::Free( qhandle_t handle ) {
	if ( handle == -1 ) {
		return;
	}

	delete commandMapIcons[ handle ];
	commandMapIcons[ handle ] = NULL;
}

/*
================
sdCommandMapInfoManagerLocal::Clear
================
*/
void sdCommandMapInfoManagerLocal::Clear( void ) {
	commandMapIcons.DeleteContents( false );
}
