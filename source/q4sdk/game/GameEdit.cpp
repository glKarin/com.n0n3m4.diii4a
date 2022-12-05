		
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
// RAVEN BEGIN
// bdube: added
#include "Effect.h"
// nmckenzie:
//#include "rvAI/AI.h"
#include "ai/AI.h"
#include "client/ClientEffect.h"
// RAVEN END

/*
===============================================================================

	Ingame cursor.

===============================================================================
*/

CLASS_DECLARATION( idEntity, idCursor3D )
END_CLASS

/*
===============
idCursor3D::idCursor3D
===============
*/
idCursor3D::idCursor3D( void ) {
	draggedPosition.Zero();
}

/*
===============
idCursor3D::~idCursor3D
===============
*/
idCursor3D::~idCursor3D( void ) {
}

/*
===============
idCursor3D::Spawn
===============
*/
void idCursor3D::Spawn( void ) {
}

/*
===============
idCursor3D::Present
===============
*/
void idCursor3D::Present( void ) {
	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	const idVec3 &origin = GetPhysics()->GetOrigin();
	const idMat3 &axis = GetPhysics()->GetAxis();
	gameRenderWorld->DebugArrow( colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2 );
	gameRenderWorld->DebugArrow( colorRed, origin, draggedPosition, 2 );
}

/*
===============
idCursor3D::Think
===============
*/
void idCursor3D::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		drag.Evaluate( gameLocal.time );
	}
	Present();
}


/*
===============================================================================

	Allows entities to be dragged through the world with physics.

===============================================================================
*/

#define MAX_DRAG_TRACE_DISTANCE			2048.0f

/*
==============
idDragEntity::idDragEntity
==============
*/
idDragEntity::idDragEntity( void ) {
	cursor = NULL;
	Clear();
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idDragEntity::~idDragEntity( void ) {
	StopDrag();
	selected = NULL;
	delete cursor;
	cursor = NULL;
}


/*
==============
idDragEntity::Clear
==============
*/
void idDragEntity::Clear() {
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= NULL;
}

/*
==============
idDragEntity::StopDrag
==============
*/
void idDragEntity::StopDrag( void ) {
	dragEnt = NULL;
	if ( cursor ) {
		cursor->BecomeInactive( TH_THINK );
	}
}

/*
==============
idDragEntity::Update
==============
*/
void idDragEntity::Update( idPlayer *player ) {
	idVec3 viewPoint, origin;
	idMat3 viewAxis, axis;
	trace_t trace;
	idEntity *newEnt;
	idAngles angles;
	jointHandle_t newJoint = INVALID_JOINT;
	idStr newBodyName;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
    if ( !dragEnt.GetEntity() ) {

		if ( player->usercmd.buttons & BUTTON_ATTACK ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			gameLocal.TracePoint( player, trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY), player );
// RAVEN END
			if ( trace.fraction < 1.0f ) {

				newEnt = gameLocal.entities[ trace.c.entityNum ];
				if ( newEnt ) {

					if ( newEnt->GetBindMaster() ) {
						if ( newEnt->GetBindJoint() ) {
							trace.c.id = JOINT_HANDLE_TO_CLIPMODEL_ID( newEnt->GetBindJoint() );
						} else {
							trace.c.id = newEnt->GetBindBody();
						}
						newEnt = newEnt->GetBindMaster();
					}

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
					if ( newEnt->IsType( idAFEntity_Base::GetClassType() ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) {
// RAVEN END
						idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
					} else if ( !newEnt->IsType( idWorldspawn::GetClassType() ) ) {
// RAVEN END

						if ( trace.c.id < 0 ) {
							newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						} else {
							newJoint = INVALID_JOINT;
						}
						newBodyName = "";

					} else {

						newJoint = INVALID_JOINT;
						newEnt = NULL;
					}
				}
				if ( newEnt ) {
					dragEnt = newEnt;
					selected = newEnt;
					joint = newJoint;
					id = trace.c.id;
					bodyName = newBodyName;

					if ( !cursor ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
						cursor = ( idCursor3D * )gameLocal.SpawnEntityType( idCursor3D::GetClassType() );
// RAVEN END
					}

					idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat() );
					cursor->drag.SetPhysics( phys, id, localEntityPoint );
					cursor->Show();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
					if ( phys->IsType( idPhysics_AF::GetClassType() ) ||
							phys->IsType( idPhysics_RigidBody::GetClassType() ) ||
								phys->IsType( idPhysics_Monster::GetClassType() ) ) {
// RAVEN END
						cursor->BecomeActive( TH_THINK );
					}
				}
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag ) {

		if ( !( player->usercmd.buttons & BUTTON_ATTACK ) ) {
			StopDrag();
			return;
		}

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
			dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
			cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
			gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		} else {
			cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
			gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
	}

	// if there is a selected entity
	if ( selected.GetEntity() && g_dragShowSelection.GetBool() ) {
		// draw the bbox of the selected entity
		renderEntity_t *renderEntity = selected.GetEntity()->GetRenderEntity();
		if ( renderEntity ) {
			gameRenderWorld->DebugBox( colorYellow, idBox( renderEntity->bounds, renderEntity->origin, renderEntity->axis ) );
		}
	}
}

/*
==============
idDragEntity::SetSelected
==============
*/
void idDragEntity::SetSelected( idEntity *ent ) {
	selected = ent;
	StopDrag();
}

/*
==============
idDragEntity::DeleteSelected
==============
*/
void idDragEntity::DeleteSelected( void ) {
	delete selected.GetEntity();
	selected = NULL;
	StopDrag();
}

/*
==============
idDragEntity::BindSelected
==============
*/
void idDragEntity::BindSelected( void ) {
	int num, largestNum;
	idLexer lexer;
	idToken type, bodyName;
	idStr key, value, bindBodyName;
	const idKeyValue *kv;
	idAFEntity_Base *af;

	af = static_cast<idAFEntity_Base *>(dragEnt.GetEntity());

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !af || !af->IsType( idAFEntity_Base::GetClassType() ) || !af->IsActiveAF() ) {
// RAVEN END
		return;
	}

	bindBodyName = af->GetAFPhysics()->GetBody( id )->GetName();
	largestNum = 1;

	// parse all the bind constraints
	kv = af->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while ( kv ) {
		key = kv->GetKey();
		key.Strip( "bindConstraint " );
		if ( sscanf( key, "bind%d", &num ) ) {
			if ( num >= largestNum ) {
				largestNum = num + 1;
			}
		}

		lexer.LoadMemory( kv->GetValue(), kv->GetValue().Length(), kv->GetKey() );
		lexer.ReadToken( &type );
		lexer.ReadToken( &bodyName );
		lexer.FreeSource();

		// if there already exists a bind constraint for this body
		if ( bodyName.Icmp( bindBodyName ) == 0 ) {
			// delete the bind constraint
			af->spawnArgs.Delete( kv->GetKey() );
			kv = NULL;
		}

		kv = af->spawnArgs.MatchPrefix( "bindConstraint ", kv );
	}

	sprintf( key, "bindConstraint bind%d", largestNum );
	sprintf( value, "ballAndSocket %s %s", bindBodyName.c_str(), af->GetAnimator()->GetJointName( joint ) );

	af->spawnArgs.Set( key, value );
	af->spawnArgs.Set( "bind", "worldspawn" );
	af->Bind( gameLocal.world, true );
}

/*
==============
idDragEntity::UnbindSelected
==============
*/
void idDragEntity::UnbindSelected( void ) {
	const idKeyValue *kv;
	idAFEntity_Base *af;

	af = static_cast<idAFEntity_Base *>(selected.GetEntity());

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !af || !af->IsType( idAFEntity_Base::GetClassType() ) || !af->IsActiveAF() ) {
// RAVEN END
		return;
	}

	// unbind the selected entity
	af->Unbind();

	// delete all the bind constraints
	kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	while ( kv ) {
		selected.GetEntity()->spawnArgs.Delete( kv->GetKey() );
		kv = selected.GetEntity()->spawnArgs.MatchPrefix( "bindConstraint ", NULL );
	}

	// delete any bind information
	af->spawnArgs.Delete( "bind" );
	af->spawnArgs.Delete( "bindToJoint" );
	af->spawnArgs.Delete( "bindToBody" );
}


/*
===============================================================================

	Handles ingame entity editing.

===============================================================================
*/

/*
==============
idEditEntities::idEditEntities
==============
*/
idEditEntities::idEditEntities( void ) {
	selectableEntityClasses.Clear();
	nextSelectTime = 0;
}

// RAVEN BEGIN
// bdube: made this special to edit entities
/*
=============
idEditEntities::FindTraceEntity
=============
*/
idEntity* idEditEntities::FindTraceEntity( idVec3 start, idVec3 end, const idEntity *skip ) {
	idEntity *ent;
	idEntity *bestEnt;
	float scale;
	float bestScale;
	idBounds b;

	bestEnt = NULL;
	bestScale = 1.0f;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent != skip && EntityIsSelectable ( ent ) ) {
			b = ent->GetPhysics()->GetAbsBounds().Expand( 16 );
			if ( b.RayIntersection( start, end-start, scale ) ) {
				if ( scale >= 0.0f && scale < bestScale ) {
					bestEnt = ent;
					bestScale = scale;
				}
			}
		}
	}

	return bestEnt;
}
// RAVEN END

/*
=============
idEditEntities::SelectEntity
=============
*/
bool idEditEntities::SelectEntity( const idVec3 &origin, const idVec3 &dir, const idEntity *skip ) {
	idVec3		end;
	idEntity	*ent;

	if ( !g_editEntityMode.GetInteger() || selectableEntityClasses.Num() == 0 ) {
		return false;
	}

	if ( gameLocal.time < nextSelectTime ) {
		return true;
	}
	nextSelectTime = gameLocal.time + 300;

	end = origin + dir * 4096.0f;

// RAVEN BEGIN
// bdube: more generic
	ent = NULL;
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		ent = FindTraceEntity( origin, end, skip );
		if ( ent ) {
			break;
		}
	}
	if ( !ent ) {
		return false;
	}
	
	ClearSelectedEntities();

	AddSelectedEntity( ent );
	gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );

	if ( gameLocal.editors & EDITOR_ENTVIEW ) {
		common->InitTool ( EDITOR_ENTVIEW, &ent->spawnArgs );
	} else {
		ent->ShowEditingDialog();
	}

	return true;
// RAVEN END
}

/*
=============
idEditEntities::AddSelectedEntity
=============
*/
void idEditEntities::AddSelectedEntity(idEntity *ent) {
	ent->fl.selected = true;
	selectedEntities.AddUnique(ent);
}

/*
==============
idEditEntities::RemoveSelectedEntity
==============
*/
void idEditEntities::RemoveSelectedEntity( idEntity *ent ) {
    if ( selectedEntities.Find( ent ) ) {
		selectedEntities.Remove( ent );
	}
}

/*
=============
idEditEntities::ClearSelectedEntities
=============
*/
void idEditEntities::ClearSelectedEntities() {
	int i, count;

	count = selectedEntities.Num();
	for ( i = 0; i < count; i++ ) {
		selectedEntities[i]->fl.selected = false;
	}
	selectedEntities.Clear();
}


/*
=============
idEditEntities::EntityIsSelectable
=============
*/
bool idEditEntities::EntityIsSelectable( idEntity *ent, idVec4 *color, idStr *text ) {
// RAVEN BEGIN
// bdube: no matter what dont let the player or its entities be selectable
	idPlayer* player;
	if ( 0 != (player = gameLocal.GetLocalPlayer() )) {
		if ( ent == player || ent == player->GetWeaponViewModel ( ) || ent == player->GetWeaponWorldModel ( ) ) {
			return false;
		}
	}
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		if ( ent->IsType ( *selectableEntityClasses[i].typeInfo ) ) {		
// RAVEN END
			if ( text ) {
				*text = selectableEntityClasses[i].textKey;
			}
			if ( color ) {
				if ( ent->fl.selected ) {
					*color = colorRed;
				} else {
					switch( i ) {
					case 1 :
						*color = colorYellow;
						break;
					case 2 :
						*color = colorBlue;
						break;
					default:
						*color = colorGreen;
					}
				}
			}
			return true;
		}
	}
	return false;
}

/*
=============
idEditEntities::DisplayEntities
=============
*/
void idEditEntities::DisplayEntities( void ) {
	idEntity *ent;

	if ( !gameLocal.GetLocalPlayer() ) {
		return;
	}

	selectableEntityClasses.Clear();
	selectedTypeInfo_t sit;

	switch( g_editEntityMode.GetInteger() ) {
		case 1:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idLight::GetClassType();
// RAVEN END
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 2:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idSound::GetClassType();
// scork: added secondary "name" field as well (Zack request)
			sit.textKey = "s_shader|name";
			selectableEntityClasses.Append( sit );
// scork: Zack (reasonably enough) doesn't want the lights displayed when editing sounds
//			sit.typeInfo = &idLight::GetClassType();
//			sit.textKey = "texture";
//			selectableEntityClasses.Append( sit );
// RAVEN END
			break;
		case 3:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idAFEntity_Base::GetClassType();
// RAVEN END
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 4:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idFuncEmitter::GetClassType();
// RAVEN END
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idAI::GetClassType();
// RAVEN END
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 6:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idEntity::GetClassType();
// RAVEN END
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 7:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &idEntity::GetClassType();
// RAVEN END
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
// RAVEN BEGIN
// bdube: added fx entities
		case 8:
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			sit.typeInfo = &rvEffect::GetClassType();
// RAVEN END
			sit.textKey = "fx";
			selectableEntityClasses.Append ( sit );
			break;
// RAVEN END						
		default:
			return;
	}

	idBounds viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	viewBounds.ExpandSelf( g_editEntityDistance.GetFloat() );
// RAVEN BEGIN
// scork: changed from 128 to 256 so we can see speaker ent descriptions before getting right up to them
// rhummer: Added cvar to adjust the distance for the text too.
	viewTextBounds.ExpandSelf( g_editEntityTextDistance.GetFloat() );
// RAVEN END

	idStr textKey;
// RAVEN BEGIN
// scork: secondary field
	idStr textKey2;
	idStr strOutput;
// RAVEN END

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		idVec4 color;

		textKey = "";
		if ( !EntityIsSelectable( ent, &color, &textKey ) ) {
			continue;
		}

// RAVEN BEGIN
// scork: handle optional secondary field
		textKey2 = "";
		int iIndex = textKey.Find('|');
		if (iIndex >= 0)
		{
			textKey2 = textKey.Mid ( iIndex+1, textKey.Length()-(iIndex+1) );	// hmmm, they emulate 99% of MS CString but don't have a single-param Mid() func?
			textKey  = textKey.Left( iIndex );
		}
// RAVEN END


		bool drawArrows = false;
// RAVEN BEGIN
// bdube: added		
		bool drawDirection = false;
// RAVEN END
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->GetType() == &idAFEntity_Base::GetClassType() ) {
// RAVEN END
			if ( !static_cast<idAFEntity_Base *>(ent)->IsActiveAF() ) {
				continue;
			}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		} else if ( ent->GetType() == &idSound::GetClassType() ) {
// RAVEN END
			if ( ent->fl.selected ) 
			{
				drawArrows = true;
				int iFlag = ent->GetRefSoundShaderFlags();
				iFlag |= SSF_HILITE;
				ent->SetRefSoundShaderFlags( iFlag );
			}
			else
			{
				int iFlag = ent->GetRefSoundShaderFlags();
				iFlag &= ~SSF_HILITE;
				ent->SetRefSoundShaderFlags( iFlag );
			}
			ent->UpdateSound();
			const idSoundShader * ss = declManager->FindSound( ent->spawnArgs.GetString( textKey ) );
			if ( ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED ) {
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
			}
// RAVEN BEGIN
// bdube: added			
// jnewquist: Use accessor for static class type 
		} else if ( ent->GetType() == &rvEffect::GetClassType() ) {
			drawDirection = true;
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
		} else if ( ent->GetType() == &idFuncEmitter::GetClassType() ) {
// RAVEN END
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
		}

		if ( !viewBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
			continue;
		}

		gameRenderWorld->DebugBounds( color, idBounds( ent->GetPhysics()->GetOrigin() ).Expand( 8 ) );
		if ( drawArrows ) {
			idVec3 start = ent->GetPhysics()->GetOrigin();
			idVec3 end = start + idVec3( 1, 0, 0 ) * 20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x+", end + idVec3( 4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 1, 0, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			gameRenderWorld->DrawText( "x-", end + idVec3( -4, 0, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y+", end + idVec3( 0, 4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 1, 0 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			gameRenderWorld->DrawText( "y-", end + idVec3( 0, -4, 0 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * +20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z+", end + idVec3( 0, 0, 4 ), 0.15f, colorWhite, axis );
			end = start + idVec3( 0, 0, 1 ) * -20.0f;
			gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			gameRenderWorld->DrawText( "z-", end + idVec3( 0, 0, -4 ), 0.15f, colorWhite, axis );
		}

// RAVEN BEGIN
// bdube: added
		if ( drawDirection ) {
			idVec3 start = ent->GetPhysics()->GetOrigin ( );
			idVec3 end   = start + ent->GetPhysics()->GetAxis()[0] * 35.0f;
			gameRenderWorld->DebugArrow ( colorYellow, start, end, 6 );
		}
// RAVEN END

		if ( textKey.Length() ) {
// RAVEN BEGIN
// scork: handle optional secondary field, plus only call GetString when bounds are within view
			if ( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
				strOutput = ent->spawnArgs.GetString( textKey );
				if (!textKey2.IsEmpty())
				{
					strOutput += " ( ";
					strOutput += ent->spawnArgs.GetString( textKey2 );
					strOutput += " )";
				}
				gameRenderWorld->DrawText( strOutput.c_str(), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 12), 0.25, colorWhite, axis, 1 );
			}
// RAVEN END
		}
	}
}


/*
===============================================================================

	idGameEdit

===============================================================================
*/

idGameEdit			gameEditLocal;
idGameEdit *		gameEdit = &gameEditLocal;


/*
=============
idGameEdit::GetSelectedEntities
=============
*/
int idGameEdit::GetSelectedEntities( idEntity *list[], int max ) {
	int num = 0;
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			list[num++] = ent;
			if ( num >= max ) {
				break;
			}
		}
	}
	return num;
}

/*
=============
idGameEdit::TriggerSelected
=============
*/
void idGameEdit::TriggerSelected() {
	idEntity *ent;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		}
	}
}

/*
================
idGameEdit::ClearEntitySelection
================
*/
void idGameEdit::ClearEntitySelection() {
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		ent->fl.selected = false;
	}
// RAVEN BEGIN
// bdube: fixed potential crash	
	if ( gameLocal.editEntities ) {
		gameLocal.editEntities->ClearSelectedEntities();
	}
// RAVEN END
}

/*
================
idGameEdit::AddSelectedEntity
================
*/
void idGameEdit::AddSelectedEntity( idEntity *ent ) {
// RAVEN BEGIN
// mekberg: fixed crash
	if ( ent && gameLocal.editEntities ) {
		gameLocal.editEntities->AddSelectedEntity( ent );
	}
// RAVEN END
}

/*
================
idGameEdit::FindEntityDefDict
================
*/
const idDict *idGameEdit::FindEntityDefDict( const char *name, bool makeDefault ) const {
	return gameLocal.FindEntityDefDict( name, makeDefault );
}

/*
================
idGameEdit::SpawnEntityDef
================
*/
void idGameEdit::SpawnEntityDef( const idDict &args, idEntity **ent ) {
	gameLocal.SpawnEntityDef( args, ent );
}

/*
================
idGameEdit::FindEntity
================
*/
idEntity *idGameEdit::FindEntity( const char *name ) const {
	return gameLocal.FindEntity( name ); 
}

/*
=============
idGameEdit::GetUniqueEntityName

generates a unique name for a given classname
=============
*/
const char *idGameEdit::GetUniqueEntityName( const char *classname ) const {
	int			id;
	static char	name[1024];

	// can only have MAX_GENTITIES, so if we have a spot available, we're guaranteed to find one
	for( id = 0; id < MAX_GENTITIES; id++ ) {
		idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
		if ( !gameLocal.FindEntity( name ) ) {
			return name;
		}
	}

	// id == MAX_GENTITIES + 1, which can't be in use if we get here
	idStr::snPrintf( name, sizeof( name ), "%s_%d", classname, id );
	return name;
}

/*
================
idGameEdit::EntityGetOrigin
================
*/
void  idGameEdit::EntityGetOrigin( idEntity *ent, idVec3 &org ) const {
	if ( ent ) {
		org = ent->GetPhysics()->GetOrigin();
	}
}

/*
================
idGameEdit::EntityGetAxis
================
*/
void idGameEdit::EntityGetAxis( idEntity *ent, idMat3 &axis ) const {
	if ( ent ) {
		axis = ent->GetPhysics()->GetAxis();
	}
}

/*
================
idGameEdit::EntitySetOrigin
================
*/
void idGameEdit::EntitySetOrigin( idEntity *ent, const idVec3 &org ) {
	if ( ent ) {
		ent->SetOrigin( org );
	}
}

/*
================
idGameEdit::EntitySetAxis
================
*/
void idGameEdit::EntitySetAxis( idEntity *ent, const idMat3 &axis ) {
	if ( ent ) {
		ent->SetAxis( axis );
	}
}

/*
================
idGameEdit::EntitySetColor
================
*/
void idGameEdit::EntitySetColor( idEntity *ent, const idVec3 color ) {
	if ( ent ) {
		ent->SetColor( color );
	}
}

/*
================
idGameEdit::EntityTranslate
================
*/
void idGameEdit::EntityTranslate( idEntity *ent, const idVec3 &org ) {
	if ( ent ) {
		ent->GetPhysics()->Translate( org );
	}
}

/*
================
idGameEdit::EntityGetSpawnArgs
================
*/
// RAVEN BEGIN
// scork: const-qualified 'ent' so other things would compile
const idDict *idGameEdit::EntityGetSpawnArgs( const idEntity *ent ) const {
// RAVEN END
	if ( ent ) {
		return &ent->spawnArgs;
	}
	return NULL;
}

/*
================
idGameEdit::EntityUpdateChangeableSpawnArgs
================
*/
void idGameEdit::EntityUpdateChangeableSpawnArgs( idEntity *ent, const idDict *dict ) {
	if ( ent ) {
		ent->UpdateChangeableSpawnArgs( dict );
	}
}

/*
================
idGameEdit::EntityChangeSpawnArgs
================
*/
void idGameEdit::EntityChangeSpawnArgs( idEntity *ent, const idDict *newArgs ) {
	if ( ent ) {
		for ( int i = 0 ; i < newArgs->GetNumKeyVals () ; i ++ ) {
			const idKeyValue *kv = newArgs->GetKeyVal( i );
	        
			if ( kv->GetValue().Length() > 0 ) {
				ent->spawnArgs.Set ( kv->GetKey() ,kv->GetValue() );
			} else {
				ent->spawnArgs.Delete ( kv->GetKey() );
			}
		}
	}
}

/*
================
idGameEdit::EntityUpdateVisuals
================
*/
void idGameEdit::EntityUpdateVisuals( idEntity *ent ) {
	if ( ent ) {
		ent->UpdateVisuals();
	}
}

/*
================
idGameEdit::EntitySetModel
================
*/
void idGameEdit::EntitySetModel( idEntity *ent, const char *val ) {
	if ( ent ) {
		ent->spawnArgs.Set( "model", val );
		ent->SetModel( val );
	}
}

/*
================
idGameEdit::EntityStopSound
================
*/
void idGameEdit::EntityStopSound( idEntity *ent ) {
	if ( ent ) {
		ent->StopSound( SND_CHANNEL_ANY, false );
	}
}

/*
================
idGameEdit::EntityDelete
================
*/
void idGameEdit::EntityDelete( idEntity *ent ) {
	delete ent;
}

// RAVEN BEGIN
// bdube: added
/*
================
idGameEdit::EntityGetRenderEntity
================
*/
renderEntity_t* idGameEdit::EntityGetRenderEntity ( idEntity* ent ) {
	return ent->GetRenderEntity();
}

/*
================
idGameEdit::EntityGetName
================
*/
const char* idGameEdit::EntityGetName ( idEntity* ent ) const {
	if ( !ent ) {
		return "";
	}
	return ent->GetName();
}

/*
================
idGameEdit::EntityGetClassname
================
*/
const char* idGameEdit::EntityGetClassname ( idEntity* ent ) const {
	return ent->GetType()->classname;
}

/*
================
idGameEdit::EntityIsDerivedFrom
================
*/
bool idGameEdit::EntityIsDerivedFrom ( idEntity* ent, const char* classname ) const {
	idTypeInfo* type;
	type = idClass::GetClass ( classname );
	if ( !type ) {
		return false;
	}
	
	return ent->IsType ( *type );
}

/*
================
idGameEdit::EntityIsValid
================
*/
int idGameEdit::EntityToSafeId ( idEntity* ent ) const {
	if ( !ent ) {
		return 0;
	}
	return ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
}

/*
================
idGameEdit::EntityFromSafeId
================
*/
idEntity *idGameEdit::EntityFromSafeId( int safeID ) const {
	int entityNum = safeID & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.spawnIds[ entityNum ] == ( safeID >> GENTITYNUM_BITS ) ) ) {
		return gameLocal.entities[ entityNum ];
	}
	return NULL;
}

/*
================
idGameEdit::EntitySetSkin
================
*/
void idGameEdit::EntitySetSkin ( idEntity* ent, const char* temp ) const {
	ent->SetSkin ( declManager->FindSkin ( temp ) );
}

/*
================
idGameEdit::EntityClearSkin
================
*/
void idGameEdit::EntityClearSkin ( idEntity* ent ) const {
	ent->ClearSkin ( );
}

/*
================
idGameEdit::EntityClearSkin
================
*/
void idGameEdit::EntityShow ( idEntity* ent ) const {
	ent->Show ( );
}

/*
================
idGameEdit::EntityClearSkin
================
*/
void idGameEdit::EntityHide ( idEntity* ent ) const {
	ent->Hide ( );
}

/*
================
idGameEdit::EntityGetBounds
================
*/
void idGameEdit::EntityGetBounds ( idEntity* ent, idBounds &bounds ) const {
	bounds = ent->GetRenderEntity()->bounds;
}

/*
================
idGameEdit::EntityPlayAnim
================
*/
int idGameEdit::EntityPlayAnim ( idEntity* ent, int animNum, int time, int blendtime ) {
	if ( !ent->GetAnimator ( ) ) {
		return 0;
	}
	ent->GetAnimator()->PlayAnim ( ANIMCHANNEL_ALL, animNum, time, blendtime );
	ent->GetAnimator()->ServiceAnims ( time, time );
	return ent->GetAnimator()->CurrentAnim( ANIMCHANNEL_ALL )->GetEndTime ( );
}

/*
================
idGameEdit::EntitySetFrame
================
*/
void idGameEdit::EntitySetFrame ( idEntity* ent, int animNum, int frame, int time, int blendtime ) {
	idAnimator* animator;
		
	animator = ent->GetAnimator ( );	
	if ( !animator ) {
		return;
	}

	animator->ClearAllAnims ( time, time );
	
	// Move to the first frame of the animation
// RAVEN BEGIN
	frameBlend_t frameBlend = { 0, frame, frame, 1.0f, 0 };
	animator->SetFrame ( ANIMCHANNEL_ALL, animNum, frameBlend );
// RAVEN END
	animator->ForceUpdate ( );
}

/*
================
idGameEdit::EntityGetDelta
================
*/
void idGameEdit::EntityGetDelta ( idEntity* ent, int fromTime, int toTime, idVec3& delta ) {
	ent->GetAnimator()->GetDelta ( fromTime, toTime, delta );
}

/*
================
idGameEdit::EntityRemoveOriginOffset
================
*/
void idGameEdit::EntityRemoveOriginOffset ( idEntity* ent, bool remove ) {
	ent->GetAnimator()->RemoveOriginOffset ( remove );
}

/*
================
idGameEdit::EntityStopAllEffects
================
*/
void idGameEdit::EntityStopAllEffects ( idEntity* ent ) {
	ent->StopAllEffects ( );
	ent->StopSound ( SND_CHANNEL_ANY, false );
}

// RAVEN BEGIN
// scork: some accessor functions for various utils
idEntity *idGameEdit::EntityGetNextTeamEntity( idEntity *pEnt ) const {
	return pEnt->GetNextTeamEntity();
}
void idGameEdit::GetPlayerInfo( idVec3 &v3Origin, idMat3 &mat3Axis, int PlayerNum, idAngles *deltaViewAngles ) const
{
	game->GetPlayerInfo( v3Origin, mat3Axis, PlayerNum, deltaViewAngles );
}

void idGameEdit::SetPlayerInfo( idVec3 &v3Origin, idMat3 &mat3Axis, int PlayerNum ) const
{
	game->SetPlayerInfo( v3Origin, mat3Axis, PlayerNum );
}
void idGameEdit::EntitySetName( idEntity* pEnt, const char *psName )
{
	pEnt->SetName( psName );
}
// RAVEN END

/*
================
idGameEdit::LightSetParms
================
*/
void idGameEdit::LightSetParms ( idEntity* ent, int maxLevel, int currentLevel, float radius ) {	
	int		 data;
	idLight* light;
	
	// Switch to a light entity
	light = dynamic_cast<idLight*>(ent);
	if ( !light )
	{
		return;
	}

	light->ProcessEvent ( &EV_Light_SetMaxLightLevel, maxLevel );
	light->ProcessEvent ( &EV_Light_SetCurrentLightLevel, (int)currentLevel );

	(*(float*)&data) = radius;
	light->ProcessEventArgPtr ( &EV_Light_SetRadius, &data );		
	
	light->SetLightLevel();
}
// RAVEN END

/*
================
idGameEdit::PlayerIsValid
================
*/
bool idGameEdit::PlayerIsValid() const {
	return ( gameLocal.GetLocalPlayer() != NULL );
}

/*
================
idGameEdit::PlayerGetOrigin
================
*/
void idGameEdit::PlayerGetOrigin( idVec3 &org ) const {
	org = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
}

/*
================
idGameEdit::PlayerGetAxis
================
*/
void idGameEdit::PlayerGetAxis( idMat3 &axis ) const {
	axis = gameLocal.GetLocalPlayer()->GetPhysics()->GetAxis();
}

/*
================
idGameEdit::PlayerGetViewAngles
================
*/
void idGameEdit::PlayerGetViewAngles( idAngles &angles ) const {
	angles = gameLocal.GetLocalPlayer()->viewAngles;
}

/*
================
idGameEdit::PlayerGetEyePosition
================
*/
void idGameEdit::PlayerGetEyePosition( idVec3 &org ) const {
	org = gameLocal.GetLocalPlayer()->GetEyePosition();
}


/*
================
idGameEdit::MapGetEntityDict
================
*/
const idDict *idGameEdit::MapGetEntityDict( const char *name ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			return &mapent->epairs;
		}
	}
	return NULL;
}

/*
================
idGameEdit::MapSave
================
*/
void idGameEdit::MapSave( const char *path ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if (mapFile) {
// RAVEN BEGIN
// rjohnson: added entity export
		if (mapFile->HasExportEntities()) {
			mapFile->WriteExport( (path) ? path : mapFile->GetName() );
		} else {
			if ( path ) {
				mapFile->Write( path, ".map");
			} else {
				idStr osPath;
				osPath = mapFile->GetName ( );
				osPath.DefaultFileExtension ( ".map" );
				idFile* file = fileSystem->OpenFileRead ( osPath );
				if ( file ) {
					osPath = file->GetFullPath ( );
					fileSystem->CloseFile ( file );
					mapFile->Write ( osPath, ".map", false );
				} else {
					mapFile->Write ( file->GetName(), ".map" );
				}
			}				
		}
// RAVEN END
	}
}

// RAVEN BEGIN
// rjohnson: added entity export
bool idGameEdit::MapHasExportEntities( void ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if (mapFile) {
		return mapFile->HasExportEntities();
	}
	return false;
}
// scork: simple query function for the sound editor
// cdr: changed to also return the full string name of the map file (still compatable as a bool test)
const char* idGameEdit::MapLoaded( void ) const {

	const char *psMapName = gameLocal.GetMapName();
	if (psMapName && psMapName[0]) {
		return psMapName;
	}
	return 0;
}

// cdr: AASTactical
idAASFile* idGameEdit::GetAASFile( int i ) {
	if (gameLocal.GetAAS( i )) {
		return gameLocal.GetAAS( i )->GetFile();
	}
	return 0;
}
// RAVEN END

/*
================
idGameEdit::MapSetEntityKeyVal
================
*/
void idGameEdit::MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			mapent->epairs.Set( key, val );
		}
	}
}

/*
================
idGameEdit::MapCopyDictToEntity
================
*/
void idGameEdit::MapCopyDictToEntity( const char *name, const idDict *dict ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			for ( int i = 0; i < dict->GetNumKeyVals(); i++ ) {
				const idKeyValue *kv = dict->GetKeyVal( i );
				const char *key = kv->GetKey();
				const char *val = kv->GetValue();
				mapent->epairs.Set( key, val );
			}
		}
	}
}



/*
================
idGameEdit::MapGetUniqueMatchingKeyVals
================
*/
int idGameEdit::MapGetUniqueMatchingKeyVals( const char *key, const char *list[], int max ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
			idMapEntity *ent = mapFile->GetEntity( i );
			if ( ent ) {
				const char *k = ent->epairs.GetString( key );
				if ( k && *k && count < max ) {
					list[count++] = k;
				}
			}
		}
	}
	return count;
}

/*
================
idGameEdit::MapAddEntity
================
*/
void idGameEdit::MapAddEntity( const idDict *dict ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		idMapEntity *ent = new idMapEntity();
		ent->epairs = *dict;
		mapFile->AddEntity( ent );
	}
}

/*
================
idGameEdit::MapRemoveEntity
================
*/
void idGameEdit::MapRemoveEntity( const char *name ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile ) {
		idMapEntity *ent = mapFile->FindEntity( name );
		if ( ent ) {
			mapFile->RemoveEntity( ent );
		}
	}
}


/*
================
idGameEdit::MapGetEntitiesMatchignClassWithString
================
*/
int idGameEdit::MapGetEntitiesMatchingClassWithString( const char *classname, const char *match, const char *list[], const int max ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		int entCount = mapFile->GetNumEntities();
		for ( int i = 0 ; i < entCount; i++ ) {
			idMapEntity *ent = mapFile->GetEntity(i);
			if (ent) {
				idStr work = ent->epairs.GetString("classname");
				if ( work.Icmp( classname ) == 0 ) {
					if ( match && *match ) { 
						work = ent->epairs.GetString( "soundgroup" );
						if ( count < max && work.Icmp( match ) == 0 ) {
							list[count++] = ent->epairs.GetString( "name" );
						}
					} else if ( count < max ) {
						list[count++] = ent->epairs.GetString( "name" );
					}
				}
			}
		}
	}
	return count;
}


/*
================
idGameEdit::MapEntityTranslate
================
*/
void idGameEdit::MapEntityTranslate( const char *name, const idVec3 &v ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	if ( mapFile && name && *name ) {
		idMapEntity *mapent = mapFile->FindEntity( name );
		if ( mapent ) {
			idVec3 origin;
			mapent->epairs.GetVector( "origin", "", origin );
			origin += v;
			mapent->epairs.SetVector( "origin", origin );
		}
	}
}

// RAVEN BEGIN
// bdube: new game edit stuff
/*
================
idGameEdit::PlayerTraceFromEye
================
*/
bool idGameEdit::PlayerTraceFromEye ( trace_t &results, float length, int contentMask ) {
	idVec3		start;
	idVec3		end;
	idAngles	angles;
		
	PlayerGetEyePosition( start );
	PlayerGetEyePosition( end );
	PlayerGetViewAngles ( angles );
	
	end += angles.ToForward() * length;
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	return gameLocal.TracePoint ( gameLocal.GetLocalPlayer(), results, start, end, contentMask, gameLocal.GetLocalPlayer() );
// RAVEN END
}

/*
================
idGameEdit::EffectRefreshTemplate
================
*/
void idGameEdit::EffectRefreshTemplate ( const idDecl *effect ) const {
	rvClientEntity* cent;	

	// Restart all effects
	for ( cent = gameLocal.clientSpawnedEntities.Next(); cent; cent = cent->spawnNode.Next() ) {
		if ( cent->IsType ( rvClientEffect::GetClassType() ) ) {
			rvClientEffect* clientEffect;
			clientEffect = static_cast<rvClientEffect*>( cent );
			if ( clientEffect->GetEffectIndex ( ) == effect->Index() ) {
				clientEffect->Restart ( );
			}
		}
	}
}

/*
================
idGameEdit::GetGameTime
================
*/
int idGameEdit::GetGameTime ( int *previous ) const {
	if ( previous ) {
		*previous = gameLocal.previousTime;
	}
	
	return gameLocal.time;
}

/*
================
idGameEdit::SetGameTime
================
*/
void idGameEdit::SetGameTime ( int time ) const {
	gameLocal.time = time;
	gameLocal.previousTime = time;
}

/*
================
idGameEdit::TracePoint
================
*/
bool idGameEdit::TracePoint ( trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask ) const {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	return gameLocal.TracePoint( gameLocal.GetLocalPlayer(), results, start, end, contentMask, NULL );
// RAVEN END
}

/*
================
idGameEdit::CacheDictionaryMedia
================
*/
void idGameEdit::CacheDictionaryMedia ( const idDict* dict ) const {
	gameLocal.CacheDictionaryMedia ( dict );
}

/*
================
idGameEdit::SetCamera
================
*/
void idGameEdit::SetCamera ( idEntity* camera ) const {
	gameLocal.SetCamera ( dynamic_cast<idCamera*>(camera) );
}

/*
================
idGameEdit::ScriptGetStatementLineNumber
================
*/
int idGameEdit::ScriptGetStatementLineNumber ( idProgram* program, int instructionPointer ) const {
	return program->GetStatement ( instructionPointer ).linenumber;
}

/*
================
idGameEdit::ScriptGetStatementFileName
================
*/
const char* idGameEdit::ScriptGetStatementFileName ( idProgram* program, int instructionPointer ) const {
	return program->GetFilename ( program->GetStatement ( instructionPointer ).file );
}

/*
================
idGameEdit::ScriptGetStatementOperator
================
*/
int idGameEdit::ScriptGetStatementOperator ( idProgram* program, int instructionPointer ) const {
	return program->GetStatement ( instructionPointer ).op;
}

/*
================
idGameEdit::ScriptGetCurrentFunction
================
*/
void* idGameEdit::ScriptGetCurrentFunction ( idInterpreter* interpreter ) const {
	return (void*)interpreter->GetCurrentFunction ( );
}

/*
================
idGameEdit::ScriptGetCurrentFunctionName
================
*/
const char* idGameEdit::ScriptGetCurrentFunctionName ( idInterpreter* interpreter ) const {
	if ( interpreter->GetCurrentFunction ( ) ) {
		return interpreter->GetCurrentFunction ( )->Name();
	}
	return "";
}

/*
================
idGameEdit::ScriptGetStatementOperator
================
*/
int idGameEdit::ScriptGetCallstackDepth ( idInterpreter* interpreter ) const {
	return interpreter->GetCallstackDepth ( );
}

/*
================
idGameEdit::ScriptGetCallstackFunction
================
*/
void* idGameEdit::ScriptGetCallstackFunction ( idInterpreter* interpreter, int depth ) const {
	return (void*)interpreter->GetCallstack ( )[depth].f;
}

/*
================
idGameEdit::ScriptGetCallstackFunctionName
================
*/
const char* idGameEdit::ScriptGetCallstackFunctionName ( idInterpreter* interpreter, int depth ) const {
	return interpreter->GetCallstack()[depth].f->Name();
}

/*
================
idGameEdit::ScriptGetCallstackStatement
================
*/
int idGameEdit::ScriptGetCallstackStatement ( idInterpreter* interpreter, int depth ) const {
	return interpreter->GetCallstack()[depth].s;
}

/*
================
idGameEdit::ScriptIsReturnOperator
================
*/
bool idGameEdit::ScriptIsReturnOperator ( int op ) const {
	return op == OP_RETURN;
}

/*
================
idGameEdit::ScriptGetRegisterValue
================
*/
const char* idGameEdit::ScriptGetRegisterValue ( idInterpreter* interpreter, const char* varname, int callstackDepth ) const {
	static char	value[4096];
	idStr		out;
	
	value[0] = '\0';
	if ( interpreter->GetRegisterValue ( varname, out, callstackDepth ) ) {	
		idStr::snPrintf ( value, 4095, out.c_str() );	
	}
	
	return value;
}

/*
================
idGameEdit::ScriptGetThread
================
*/
idThread* idGameEdit::ScriptGetThread ( idInterpreter* interpreter ) const {
	return interpreter->GetThread();
}

/*
================
idGameEdit::ThreadGetCount
================
*/
int idGameEdit::ThreadGetCount ( void ) {
	return idThread::GetThreads().Num();
}

/*
================
idGameEdit::ThreadGetThread
================
*/
idThread* idGameEdit::ThreadGetThread ( int index ) {
	return idThread::GetThreads()[index];
}

/*
================
idGameEdit::ThreadGetName
================
*/
const char* idGameEdit::ThreadGetName ( idThread* thread ) {
	return thread->GetThreadName ( );
}

/*
================
idGameEdit::ThreadGetNumber
================
*/
int idGameEdit::ThreadGetNumber ( idThread* thread ) {
	return thread->GetThreadNum ( );
}

/*
================
idGameEdit::ThreadGetState
================
*/
const char* idGameEdit::ThreadGetState ( idThread* thread ) {
	if ( thread->IsDying() ) {
		return "Dying";
	} else if ( thread->IsWaiting() ) {
		return "Waiting";
	} else if ( thread->IsDoneProcessing() ) {
		return "Stopped";
	}
	
	return "Running";
}

/*
================
idGameEdit::GetClassDebugInfo
================
*/
void idGameEdit::GetClassDebugInfo ( const idEntity* entity, debugInfoProc_t proc, void* userdata ) {
	const_cast<idEntity *>( entity )->GetDebugInfo ( proc, userdata  );
}

/*
================
idGameEdit::GetGameEntityRegisterTime
================
*/
int idGameEdit::GetGameEntityRegisterTime ( void ) const {
	return gameLocal.entityRegisterTime;
}

/*
================
idGameEdit::GetFirstSpawnedEntity
================
*/
idEntity* idGameEdit::GetFirstSpawnedEntity ( void ) const {
	return gameLocal.spawnedEntities.Next();
}

/*
================
idGameEdit::GetNextSpawnedEntity
================
*/
idEntity* idGameEdit::GetNextSpawnedEntity ( idEntity* from ) const {
	if ( !from ) {
		return NULL;
	}
	return from->spawnNode.Next();
}


// RAVEN END

// RAVEN BEGIN
// mekberg: access to animationlib functions for radiant
void idGameEdit::FlushUnusedAnims ( void ) {

// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
	animationLib->FlushUnusedAnims();
// RAVEN END
}

/*
===============================================================================

  rvModviewModel

  Actor model for modview

===============================================================================
*/

class rvModviewModel : public idActor {
public:
	CLASS_PROTOTYPE( rvModviewModel );
	
	rvModviewModel ( void );
		
private:
	
	void					Event_Speak		( const char* lipsync );
};

CLASS_DECLARATION( idActor, rvModviewModel )
	EVENT( AI_Speak,				rvModviewModel::Event_Speak )	
END_CLASS

/*
=====================
rvModviewModel::rvModviewModel
=====================
*/
rvModviewModel::rvModviewModel ( void ) {
}

/*
=====================
rvModviewModel::Event_Speak
=====================
*/
void rvModviewModel::Event_Speak ( const char* lipsync ) {
	assert( idStr::Icmpn( lipsync, "lipsync_", 7 ) == 0 );
	
	lipsync = spawnArgs.GetString ( lipsync );
	if ( !lipsync || !*lipsync ) {
		return;
	}
	
	if ( head ) {
		head->StartLipSyncing( lipsync );
	} else {
		StartSoundShader (declManager->FindSound ( lipsync ), SND_CHANNEL_VOICE, 0, false, NULL );
	}
}	

// RAVEN END

