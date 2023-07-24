// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "GameEdit.h"
#include "Player.h"
#include "vehicles/Transport.h"
#include "WorldSpawn.h"
#include "physics/Physics_Monster.h"
#include "Light.h"
#include "Sound.h"
#include "Misc.h"
//#include "ai/AI.h"
#include "client/ClientEffect.h"

#include "script/Script_Interpreter.h"
#include "script/Script_Compiler.h"

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

	if ( g_dragEntity.GetBool() ) {
		gameRenderWorld->DebugArrow( colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2 );
		gameRenderWorld->DebugArrow( colorRed, origin, draggedPosition, 2 );
	}
}

/*
===============
idCursor3D::Think
===============
*/
void idCursor3D::Think( void ) {
	drag.Evaluate( gameLocal.time );
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
	StopDrag();

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
	jointHandle_t newJoint;
	idStr newBodyName;

	player->GetViewPos( viewPoint, viewAxis );

	// if no entity selected for dragging
    if ( !dragEnt.GetEntity() ) {

		if ( player->usercmd.buttons.btn.attack ) {

			gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY|CONTENTS_SLIDEMOVER), player );
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

					if ( newEnt->IsType( idAFEntity_Base::Type ) && static_cast<idAFEntity_Base *>(newEnt)->IsActiveAF() ) {
						idAFEntity_Base *af = static_cast<idAFEntity_Base *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();

					} else if ( newEnt->IsType( sdTransport_AF::Type ) ) {
						sdTransport_AF *af = static_cast<sdTransport_AF *>(newEnt);

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = af->BodyForClipModelId( trace.c.id );
						// get the name of the body being dragged
						newBodyName = af->GetAFPhysics()->GetBody( trace.c.id )->GetName();
						// always drag the center of the body for stability
						trace.c.point = af->GetAFPhysics()->GetOrigin( trace.c.id );
						
					} else if ( newEnt->IsType( sdTransport::Type ) ) {

						// joint being dragged
						newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						// get the body id from the trace model id which might be a joint handle
						trace.c.id = 0;
						// get the name of the body being dragged
						newBodyName = "";

					} else if ( !newEnt->IsType( idWorldspawn::Type ) ) {

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
				if ( newEnt && !gameLocal.isClient ) {
					dragEnt = newEnt;
					selected = newEnt;
					joint = newJoint;
					id = trace.c.id;
					bodyName = newBodyName;

					if ( cursor == NULL ) {
						cursor = static_cast< idCursor3D* >( gameLocal.SpawnEntityType( idCursor3D::Type, true ) );
					}

					idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat(), g_dragMaxforce.GetFloat() );
					cursor->drag.SetPhysics( phys, id, localEntityPoint );
					cursor->Show();

					if ( phys->IsType( idPhysics_AF::Type ) ||
							phys->IsType( idPhysics_RigidBody::Type ) ||
								phys->IsType( idPhysics_Monster::Type ) ) {
						cursor->BecomeActive( TH_THINK );
					}
				}
			}
		}
	}

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag ) {

		if ( !player->usercmd.buttons.btn.attack ) {
			StopDrag();
			return;
		}

		if ( cursor ) {

			cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
			cursor->SetAxis( viewAxis );

			cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

			renderEntity_t *renderEntity = drag->GetRenderEntity();
			idAnimator *dragAnimator = drag->GetAnimator();

			if ( joint != INVALID_JOINT && renderEntity && dragAnimator ) {
				dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
				cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
				if ( g_dragEntity.GetBool() ) {
					gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
				}
			} else {
				cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
				if ( g_dragEntity.GetBool() ) {
					gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
				}
			}
		}
	}

	// if there is a selected entity
	if ( g_dragEntity.GetBool() && selected.GetEntity() && g_dragShowSelection.GetBool() ) {
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

	if ( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() ) {
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

	if ( !af || !af->IsType( idAFEntity_Base::Type ) || !af->IsActiveAF() ) {
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

	ent = NULL;
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		ent = gameLocal.FindTraceEntity( origin, end, *selectableEntityClasses[i].typeInfo, skip );
		if ( ent ) {
			break;
		}
	}
	if ( ent ) {
		ClearSelectedEntities();
		if ( EntityIsSelectable( ent ) ) {
			AddSelectedEntity( ent );
			gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );
			ent->ShowEditingDialog();
			return true;
		}
	}
	return false;
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
    if ( selectedEntities.FindElement( ent ) ) {
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
idEditEntities::GetSelectedEntities
=============
*/
idList<idEntity *>& idEditEntities::GetSelectedEntities( void ) {
	return selectedEntities;
}

/*
=============
idEditEntities::EntityIsSelectable
=============
*/
bool idEditEntities::EntityIsSelectable( idEntity *ent, idVec4 *color, idStr *text ) {
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		if ( ent->GetType() == selectableEntityClasses[i].typeInfo ) {
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
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 2:
			sit.typeInfo = &idSound::Type;
			sit.textKey = "s_shader";
			selectableEntityClasses.Append( sit );
			sit.typeInfo = &idLight::Type;
			sit.textKey = "texture";
			selectableEntityClasses.Append( sit );
			break;
		case 3:
			sit.typeInfo = &idAFEntity_Base::Type;
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
			/*	jrad - idAI removal
			sit.typeInfo = &idAI::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			*/
			break;
		case 6:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
			break;
		case 7:
			sit.typeInfo = &idEntity::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		default:
			return;
	}

	idBounds viewBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idBounds viewTextBounds( gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() );
	idMat3 axis = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	viewTextBounds.ExpandSelf( 128 );
	viewBounds.ExpandSelf( g_maxShowDistance.GetFloat() );

	idStr textKey;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {

		idVec4 color;

		textKey = "";
		if ( !EntityIsSelectable( ent, &color, &textKey ) ) {
			continue;
		}

		bool drawArrows = false;
		if ( ent->GetType() == &idAFEntity_Base::Type ) {
			if ( !static_cast<idAFEntity_Base *>(ent)->IsActiveAF() ) {
				continue;
			}
		} else if ( ent->GetType() == &idSound::Type ) {
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
			const char* string = ent->spawnArgs.GetString( textKey );

			const idSoundShader *ss = NULL;
			
			if( idStr::Length( string ) > 0 ) {
				ss = declHolder.declSoundShaderType.LocalFind( string );
			}
			if ( !ss || ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED ) {
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
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

		if ( textKey.Length() ) {
			const char *text = ent->spawnArgs.GetString( textKey );
			if ( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
				gameRenderWorld->DrawText( text, ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 12), 0.25, colorWhite, axis, 1 );
			}
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
idGameEdit::GetSelectedEntitiesByName
=============
*/
int idGameEdit::GetSelectedEntitiesByName( idStr *list[], int max ) {
	int num = 0;
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->fl.selected ) {
			list[num++] = &ent->name;
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
	if( gameLocal.editEntities == NULL ) {
		return;
	}
	idEntity *ent;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		ent->fl.selected = false;
	}
	gameLocal.editEntities->ClearSelectedEntities();
}

/*
================
idGameEdit::AddSelectedEntity
================
*/
void idGameEdit::AddSelectedEntity( idEntity *ent ) {
	if( gameLocal.editEntities == NULL ) {
		return;
	}
	if ( ent ) {
		gameLocal.editEntities->AddSelectedEntity( ent );
	}
}

/*
================
idGameEdit::RemoveSelectedEntity
================
*/
void idGameEdit::RemoveSelectedEntity( idEntity *ent ) {
	if( gameLocal.editEntities == NULL ) {
		return;
	}
	if ( ent ) {
		gameLocal.editEntities->RemoveSelectedEntity( ent );
	}
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
	gameLocal.SpawnEntityDef( args, true, ent );
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
const idDict *idGameEdit::EntityGetSpawnArgs( idEntity *ent ) const {
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
				ent->spawnArgs.Set ( kv->GetKey(), kv->GetValue() );
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
		ent->StopSound( SND_ANY );
	}
}

/*
================
idGameEdit::EntityDelete
================
*/
void idGameEdit::EntityDelete( idEntity *ent ) {
	ent->ProcessEvent( &EV_Remove );
}

/*
================
idGameEdit::EntityToSafeId
================
*/
int idGameEdit::EntityToSafeId ( idEntity* ent ) const {
	return gameLocal.GetSpawnId( ent );
}

/*
================
idGameEdit::EntityFromSafeId
================
*/
idEntity *idGameEdit::EntityFromSafeId( int safeID ) const {
	return gameLocal.EntityForSpawnId( safeID );
}

/*
================
idGameEdit::EntityFromIndex
================
*/
idEntity *idGameEdit::EntityFromIndex( int index ) const {
	if ( index < 0 || index >= MAX_GENTITIES ) {
		return NULL;
	}

	return gameLocal.entities[ index ];
}

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
	if( gameLocal.GetLocalPlayer() != NULL )  {
		org = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	}
}

/*
================
idGameEdit::PlayerGetAxis
================
*/
void idGameEdit::PlayerGetAxis( idMat3 &axis ) const {
	if( gameLocal.GetLocalPlayer() != NULL )  {
		axis = gameLocal.GetLocalPlayer()->GetPhysics()->GetAxis();
	}
}

/*
================
idGameEdit::PlayerGetViewAngles
================
*/
void idGameEdit::PlayerGetViewAngles( idAngles &angles ) const {
	if( gameLocal.GetLocalPlayer() != NULL )  {
		angles = gameLocal.GetLocalPlayer()->viewAngles;
	}	
}

/*
================
idGameEdit::PlayerGetEyePosition
================
*/
void idGameEdit::PlayerGetEyePosition( idVec3 &org ) const {
	if( gameLocal.GetLocalPlayer() != NULL )  {
		org = gameLocal.GetLocalPlayer()->GetEyePosition();
	}
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
	if ( mapFile != NULL ) {
		mapFile->Write( ( path != NULL && path[0] != '\0' ) ? path : mapFile->GetName(), ".entities" );
	}
}

/*
================
idGameEdit::MapSaveClass
================
*/
void idGameEdit::MapSaveClass( const char *path, const char* classname ) const {

	idMapFile *mapFile = gameLocal.GetLevelMap();	
	if ( mapFile != NULL ) {
		idFile* f = fileSystem->OpenFileWrite( ( path != NULL && path[0] != '\0' ) ? path : va( "%s.entities", mapFile->GetName()) );
		if ( f != NULL ) {
			int num = 0;
			for( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
				idMapEntity* ent = mapFile->GetEntity( i ); 
				if( idStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 ) {
					ent->Write( f, num );
					num++;
				}
			}
			fileSystem->CloseFile( f );
		}
	}
}

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
int idGameEdit::MapGetEntitiesMatchingClassWithString( const char *classname, const char *list[], const int max, const char* matchKey, const char *matchValue ) const {
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int count = 0;
	if ( mapFile ) {
		int entCount = mapFile->GetNumEntities();
		for ( int i = 0 ; i < entCount; i++ ) {
			idMapEntity *ent = mapFile->GetEntity(i);
			if ( ent != NULL ) {
				idStr work = ent->epairs.GetString( "classname" );
				if ( work.Icmp( classname ) == 0 ) {
					if ( matchKey && *matchKey && matchValue && *matchValue ) { 
						work = ent->epairs.GetString( matchKey );
						if ( count < max && work.Icmp( matchValue ) == 0 ) {
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
	
	return gameLocal.clip.TracePoint ( CLIP_DEBUG_PARMS results, start, end, contentMask, gameLocal.GetLocalPlayer() );
}

/*
================
idGameEdit::EffectRefreshTemplate
================
*/
void idGameEdit::EffectRefreshTemplate ( int effectIndex ) const {
	rvClientEntity* cent;	

	// Restart all effects
	for ( cent = gameLocal.clientSpawnedEntities.Next(); cent; cent = cent->spawnNode.Next() ) {
		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect && effect->GetEffectIndex() == effectIndex ) {
			effect->Restart();
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
	return gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS results, start, end, contentMask, NULL );
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

/*
================
idGameEdit::GetRenderWorld
================
*/
idRenderWorld* idGameEdit::GetRenderWorld() const {
	return gameRenderWorld;
}

/*
================
idGameEdit::GetRenderWorld
================
*/
const char* idGameEdit::MapGetName() const {
	return gameLocal.GetMapName();
}

/*
================
idGameEdit::KillClass
================
*/
void KillEntities( const idCmdArgs &args, const idTypeInfo &superClass, bool delayed );

void idGameEdit::KillClass( const char* classname ) {
	if ( classname == NULL || classname[0] == '\0' ) {
		return;
	}

	idTypeInfo* info = idClass::GetClass( classname );
	if ( info == NULL ) {
		gameLocal.Warning( "Unknown class '%s'", classname );
		return;
	}

	idCmdArgs dummyArgs;

	KillEntities( dummyArgs, *info, false );
}

/*
============
idGameEdit::NumUserInterfaceScopes
============
*/
int	idGameEdit::NumUserInterfaceScopes() const {
	return gameLocal.GetUIScopes().Num();
}

/*
============
idGameEdit::GetScope
============
*/
const guiScope_t& idGameEdit::GetScope( int index ) {
	return gameLocal.GetUIScopes()[ index ];
}
