/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_Monster.h"
#include "ai/AI.h"
#include "Player.h"
#include "Light.h"
#include "WorldSpawn.h"
#include "Sound.h"
#include "Misc.h"
#include "Fx.h"

#include "Mover.h"

#include "GameEdit.h"

const int BODYDRAG_PUSHFORCE = 3072;
const int MOVETIME_MAX = 300;
const int LIGHTEDIT_GRIDSIZE = 8;

//BC
idCVar slider_red("slider_red", "0", CVAR_FLOAT, "");
idCVar slider_green("slider_green", "0", CVAR_FLOAT, "");
idCVar slider_blue("slider_blue", "0", CVAR_FLOAT, "");

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
	
	

	if (drawDebuglines && g_dragEntity.GetInteger() <= 1)
	{
		gameRenderWorld->DebugArrow(colorYellow, origin + axis[1] * -5.0f + axis[2] * 5.0f, origin, 2);
		gameRenderWorld->DebugArrow(colorRed, origin, draggedPosition, 2);
	}
	else
	{
		//gameRenderWorld->DebugLine(colorWhite, origin , draggedPosition);
	}

	//gameRenderWorld->DebugArrowSimple(draggedPosition, 10);
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
	if ( !dragEnt.GetEntity() )
	{
		if ( player->usercmd.buttons & BUTTON_ATTACK )
		{
			gameLocal.clip.TracePoint( trace, viewPoint, viewPoint + viewAxis[0] * MAX_DRAG_TRACE_DISTANCE, (CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY), player );
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

					} else if ( !newEnt->IsType( idWorldspawn::Type ) ) {

						if ( trace.c.id < 0 ) {
							newJoint = CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id );
						} else {
							newJoint = INVALID_JOINT;
						}
						newBodyName = "";

						//BC
						if (newEnt->IsType(idDoor::Type))
						{
							//force door open...
							static_cast<idDoor *>(newEnt)->wait = -1;
							static_cast<idDoor *>(newEnt)->Open();
						}

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
						cursor = ( idCursor3D * )gameLocal.SpawnEntityType( idCursor3D::Type );
					}

					idPhysics *phys = dragEnt.GetEntity()->GetPhysics();
					localPlayerPoint = ( trace.c.point - viewPoint ) * viewAxis.Transpose();
					origin = phys->GetOrigin( id );
					axis = phys->GetAxis( id );
					localEntityPoint = ( trace.c.point - origin ) * axis.Transpose();

					cursor->drag.Init( g_dragDamping.GetFloat() );
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
	if ( drag )
	{
		if ( !( player->usercmd.buttons & BUTTON_ATTACK ) ) {
			StopDrag();
			return;
		}

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		if (g_dragEntity.GetInteger() <= 1)
		{
			renderEntity_t *renderEntity = drag->GetRenderEntity();
			idAnimator *dragAnimator = drag->GetAnimator();

			if (joint != INVALID_JOINT && renderEntity && dragAnimator) {
				dragAnimator->GetJointTransform(joint, gameLocal.time, cursor->draggedPosition, axis);
				cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
				gameRenderWorld->DrawText(va("%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName(joint), bodyName.c_str()), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1);
			}
			else {
				cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
				gameRenderWorld->DrawText(va("%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str()), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1);
			}
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

	
	moveState = EDITENT_IDLE;
}

bool idEditEntities::IsLookingAtEnt(idPlayer *skip, idEntity *skip2, const idVec3 &origin, const idVec3 &dir)
{
	idVec3		end;
	idEntity	*ent;

	if (!g_editEntityMode.GetInteger() || selectableEntityClasses.Num() == 0)
	{
		return false;
	}

	end = origin + dir * 2048.0f;
	
	ent = NULL;
	for (int i = 0; i < selectableEntityClasses.Num(); i++) {

		ent = gameLocal.FindTraceEntity(origin, end, *selectableEntityClasses[i].typeInfo, skip);

		if (ent == skip2)
			continue;

		if (ent) {
			return true;
		}
	}

	return false;
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

	end = origin + dir * 2048.0f;

	ent = NULL;
	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		

		ent = gameLocal.FindTraceEntity( origin, end, *selectableEntityClasses[i].typeInfo, skip );

		if ( ent ) {
			break;
		}
	}

	//PLAYER LEFT CLICK ON AN ENTITY.
	if ( ent )
	{
		ClearSelectedEntities();
		if ( EntityIsSelectable( ent ) )
		{
			AddSelectedEntity( ent );
			//gameLocal.Printf( "entity #%d: %s '%s'\n", ent->entityNumber, ent->GetClassname(), ent->name.c_str() );
			ent->ShowEditingDialog();

			selectionOffset = (origin - ent->GetPhysics()->GetOrigin()).Length();


			//BC update the editmenu.
			if (ent)
			{
				idLight *light;				
				light = static_cast<idLight *>(ent);
				
				if (light)
				{
					idVec3 lightColor;
					idVec3 lightRadius;

					//Propagate all of the light's properties to the editmenu gui. This...... is a pretty bad hack
					gameEdit->lighteditMenu->SetStateString("entName", ent->GetName());
				
					if (light->GetNoShadow())
						gameEdit->lighteditMenu->HandleNamedEvent("init_shadowsIsOff");
					else
						gameEdit->lighteditMenu->HandleNamedEvent("init_shadowsIsOn");

					if (light->Event_IsOn())
						gameEdit->lighteditMenu->HandleNamedEvent("init_lightIsOn");
					else
						gameEdit->lighteditMenu->HandleNamedEvent("init_lightIsOff");

					if (light->Event_IsAmbient())
						gameEdit->lighteditMenu->HandleNamedEvent("init_ambientIsOn");
					else
						gameEdit->lighteditMenu->HandleNamedEvent("init_ambientIsOff");

					//the light color
					light->GetBaseColor(lightColor);
					SetRGBSliders(lightColor);

					//the light radius
					lightRadius = light->GetRadius();
					gameEdit->lighteditMenu->SetStateInt("lightsize_x", lightRadius.x);
					gameEdit->lighteditMenu->SetStateInt("lightsize_y", lightRadius.y);
					gameEdit->lighteditMenu->SetStateInt("lightsize_z", lightRadius.z);
				}

				moveState = EDITENT_IDLE;

				if (ent)
				{
					idEntityFx::StartFx("fx/edit_select", &ent->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
				}
			}

			//New entity selected. Open the edit menu immediately.
			gameLocal.GetLocalPlayer()->LightEdit_OpenMenu(true);

			return true;
		}
	}

	//ClearSelectedEntities(); //BC if nothing in crosshair, then deselect.
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

	for ( int i = 0; i < selectableEntityClasses.Num(); i++ ) {
		if ( ent->GetType() == selectableEntityClasses[i].typeInfo )
		{
			if (g_editEntityMode.GetInteger() == 1)
			{
				idEntity *parent;

				//BC light editing mode. Ignore ents with double underscore. We want to ignore lights created during runtime.
				idStr entName = ent->GetName();
				if (entName.Find("__") >= 0)
				{
					return false;
				}

				//Ignore lights bound to player.
				parent = ent->GetBindMaster();
				if (parent == gameLocal.GetLocalPlayer())
				{
					return false;
				}
			}
			else if (g_editEntityMode.GetInteger() == 2)
			{
				//BC sound editing mode.
				idStr entName = ent->GetName();
				if (entName.Find("__") >= 0)
				{
					return false;
				}
			}

			if ( text )
			{
				*text = selectableEntityClasses[i].textKey;
			}

			if ( color )
			{
				if ( ent->fl.selected )
				{
					*color = colorRed;
				}
				else
				{
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

	if (g_editEntityMode.GetInteger() == 1 && gameEdit->editmenuActive)
	{
		//BC HACK to make color sliders work.
		float redValue = gameEdit->lighteditMenu->GetStateFloat("slider_red_valueFloat2");
		float greenValue = gameEdit->lighteditMenu->GetStateFloat("slider_green_valueFloat2");
		float blueValue = gameEdit->lighteditMenu->GetStateFloat("slider_blue_valueFloat2");
		
		//Change light color.
		idEntity * selectedLight = gameEdit->GetFirstSelectedEntity();

		if (selectedLight)
		{
			idLight	*lightEnt = static_cast<idLight*>(selectedLight);

			if (lightEnt)
			{
				lightEnt->SetColor(idVec4(redValue, greenValue, blueValue, 1));
				gameEdit->lighteditMenu->SetStateFloat("preview_r", redValue);
				gameEdit->lighteditMenu->SetStateFloat("preview_g", greenValue);
				gameEdit->lighteditMenu->SetStateFloat("preview_b", blueValue);
			}
		}
	}
	else if (g_editEntityMode.GetInteger() == 1)
	{
		idEntity * selectedLight = gameEdit->GetFirstSelectedEntity();
		if (selectedLight)
		{
			idLight	*lightEnt = static_cast<idLight*>(selectedLight);
			if (lightEnt)
			{
				idBox lightBox;
				idVec3 lightRadius = lightEnt->GetRadius();

				lightBox = idBox(lightEnt->GetPhysics()->GetOrigin(), lightRadius, lightEnt->GetPhysics()->GetAxis());

				gameRenderWorld->DebugBox(colorGreen, lightBox, 0, true);
			}
		}
	}
	

	switch( g_editEntityMode.GetInteger() ) {
		case 1:
			sit.typeInfo = &idLight::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append(sit);

			break;
		case 2:
			sit.typeInfo = &idSound::Type;
			sit.textKey = "s_shader";
			selectableEntityClasses.Append( sit );
			//sit.typeInfo = &idLight::Type; //BC ignore lights.
			//sit.textKey = "texture";
			//selectableEntityClasses.Append( sit );
			break;
		case 3:
			sit.typeInfo = &idAFEntity_Base::Type;
			sit.textKey = "articulatedFigure";
			selectableEntityClasses.Append( sit );
			break;
		case 4:
			sit.typeInfo = &idFuncEmitter::Type;
			sit.textKey = "model";
			selectableEntityClasses.Append( sit );
			break;
		case 5:
			sit.typeInfo = &idAI::Type;
			sit.textKey = "name";
			selectableEntityClasses.Append( sit );
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

	viewBounds.ExpandSelf( 1024 );
	viewTextBounds.ExpandSelf( 512 );

	idStr textKey;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
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
		}
		else if ( ent->GetType() == &idSound::Type )
		{
			//if ( ent->fl.selected ) //BC don't draw the axis widget on selected sounds.
			//{
			//	drawArrows = true;
			//}
			const idSoundShader * ss = declManager->FindSound( ent->spawnArgs.GetString( textKey ) );
			if ( ss->HasDefaultSound() || ss->base->GetState() == DS_DEFAULTED ) {
				color.Set( 1.0f, 0.0f, 1.0f, 1.0f );
			}
		}
		else if ( ent->GetType() == &idFuncEmitter::Type ) {
			if ( ent->fl.selected ) {
				drawArrows = true;
			}
		}
		else if (ent->GetType() == &idLight::Type)
		{
			if (ent->fl.selected)
			{
				drawArrows = true;
			}
		}

		if ( !viewBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) ) {
			continue;
		}

		gameRenderWorld->DebugBounds( color, idBounds( ent->GetPhysics()->GetOrigin() ).Expand( 8 ) );
		if ( drawArrows )
		{
			//idVec3 start = ent->GetPhysics()->GetOrigin();
			//idVec3 end = start + idVec3( 1, 0, 0 ) * 20.0f;
			//gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			//gameRenderWorld->DrawText( "x+", end + idVec3( 4, 0, 0 ), 0.15f, colorWhite, axis );
			//end = start + idVec3( 1, 0, 0 ) * -20.0f;
			//gameRenderWorld->DebugArrow( colorWhite, start, end, 2 );
			//gameRenderWorld->DrawText( "x-", end + idVec3( -4, 0, 0 ), 0.15f, colorWhite, axis );
			//end = start + idVec3( 0, 1, 0 ) * +20.0f;
			//gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			//gameRenderWorld->DrawText( "y+", end + idVec3( 0, 4, 0 ), 0.15f, colorWhite, axis );
			//end = start + idVec3( 0, 1, 0 ) * -20.0f;
			//gameRenderWorld->DebugArrow( colorGreen, start, end, 2 );
			//gameRenderWorld->DrawText( "y-", end + idVec3( 0, -4, 0 ), 0.15f, colorWhite, axis );
			//end = start + idVec3( 0, 0, 1 ) * +20.0f;
			//gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			//gameRenderWorld->DrawText( "z+", end + idVec3( 0, 0, 4 ), 0.15f, colorWhite, axis );
			//end = start + idVec3( 0, 0, 1 ) * -20.0f;
			//gameRenderWorld->DebugArrow( colorBlue, start, end, 2 );
			//gameRenderWorld->DrawText( "z-", end + idVec3( 0, 0, -4 ), 0.15f, colorWhite, axis );


			idVec3 start = ent->GetPhysics()->GetOrigin() + idVec3(1, 0, 0) * 32.0f;
			idVec3 end = ent->GetPhysics()->GetOrigin() + idVec3(1, 0, 0) * -32.0f;
			gameRenderWorld->DebugLine(colorRed, start, end, 50, true);

			start = ent->GetPhysics()->GetOrigin() + idVec3(0, 1, 0) * 32.0f;
			end = ent->GetPhysics()->GetOrigin() + idVec3(0, 1, 0) * -32.0f;
			gameRenderWorld->DebugLine(colorGreen, start, end, 50, true);

			start = ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1) * 32.0f;
			end = ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 1) * -32.0f;
			gameRenderWorld->DebugLine(colorBlue, start, end, 50, true);

		}

		if ( textKey.Length() )
		{
			const char *text = ent->spawnArgs.GetString( textKey );
			if ( viewTextBounds.ContainsPoint( ent->GetPhysics()->GetOrigin() ) )
			{
				if (text[0] == '\0')
				{
					if (g_editEntityMode.GetInteger() == 1)
					{
						//BC If no texture name, then just say Light.
						gameRenderWorld->DrawText("Light", ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 12), 0.25, colorWhite, axis, 1);
					}
				}
				else
				{
					gameRenderWorld->DrawText(text, ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 12), 0.25, colorWhite, axis, 1);
				}
			}
		}
	}
}



bool idEditEntities::UpdateDrag(idPlayer *player, const idVec3 &origin, const idVec3 &dir)
{
	

	if (selectedEntities.Num() > 0)
	{
		//Dragging an ent.
		idVec3 viewPoint;
		idMat3 viewAxis;
		int i;

		if (IsLookingAtEnt(player, selectedEntities[0], origin, dir))
			return false;

		player->GetViewPos(viewPoint, viewAxis);

		for (i = 0; i < selectedEntities.Num(); i++)
		{
			//BC handle dragging edit entities around the world.
			
			idVec3 desiredPos = viewPoint + viewAxis[0] * selectionOffset;

			

			if (moveState == EDITENT_IDLE)
			{
				//do distance check. Only enter move mode if player moves the entity farther than XX units.

				//BC disable entity moving.

				//float dist = (desiredPos - selectedEntities[i]->GetPhysics()->GetOrigin()).Length();
				//
				//if (dist >= 24)
				//{
				//	moveState = EDITENT_MOVING;
				//}
			}
			else if (moveState == EDITENT_MOVING)
			{
				idVec3 snapPos;
				//snap to the closest gridsnap location.
				snapPos.x = round(desiredPos.x / LIGHTEDIT_GRIDSIZE) * LIGHTEDIT_GRIDSIZE;
				snapPos.y = round(desiredPos.y / LIGHTEDIT_GRIDSIZE) * LIGHTEDIT_GRIDSIZE;
				snapPos.z = round(desiredPos.z / LIGHTEDIT_GRIDSIZE) * LIGHTEDIT_GRIDSIZE;

				selectedEntities[i]->SetOrigin(snapPos);
			}

			//BC original code:
			//selectedEntities[i]->SetOrigin(viewPoint + viewAxis[0] * selectionOffset);
		}

		return true;
	}

	return false;
}

//BC
void idEditEntities::SetRGBSliders(idVec3 color)
{
	slider_red.SetFloat(color.x);
	slider_green.SetFloat(color.y);
	slider_blue.SetFloat(color.z);
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
	gameLocal.editEntities->ClearSelectedEntities();
}

/*
================
idGameEdit::AddSelectedEntity
================
*/
void idGameEdit::AddSelectedEntity( idEntity *ent ) {
	if ( ent ) {
		gameLocal.editEntities->AddSelectedEntity( ent );
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
		mapFile->Write( (path) ? path : mapFile->GetName(), ".map");
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

//BC shortcut to get the first selected item.
idEntity * idGameEdit::GetFirstSelectedEntity()
{
	int selectCount;
	idEntity *entityList[MAX_GENTITIES];

	selectCount = GetSelectedEntities(entityList, MAX_GENTITIES);

	if (selectCount <= 0)
	{
		return NULL;
	}

	return entityList[0];
}








//BC the bodydragger system. This should uhh probably not be in GameEdit but this place will work well enough....

#define MAX_DRAG_TRACE_DISTANCE			2048.0f

/*
==============
idDragEntity::idDragEntity
==============
*/
idBodyDragger::idBodyDragger( void ) {
	cursor = NULL;
	Clear();
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idBodyDragger::~idBodyDragger( void ) {
	StopDrag(false);
	selected = NULL;
	delete cursor;
	cursor = NULL;
}


/*
==============
idDragEntity::Clear
==============
*/
void idBodyDragger::Clear() {
	dragEnt			= NULL;
	joint			= INVALID_JOINT;
	id				= 0;
	localEntityPoint.Zero();
	localPlayerPoint.Zero();
	bodyName.Clear();
	selected		= NULL;
	isDragging		= false;
}

/*
==============
idDragEntity::StopDrag
==============
*/
void idBodyDragger::StopDrag( bool isThrow ) 
{
	if (!isDragging)
		return;

	//Revert hud to display name of currently equipped weapon.
	gameLocal.GetLocalPlayer()->UpdateHudWeapon();
	gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("weaponChangeForce");

	if (dragEnt.IsValid())
	{
		if (dragEnt.GetEntity()->IsType(idAI::Type))
		{
			static_cast<idAI *>(dragEnt.GetEntity())->SetDragFrobbable(true);//This re-enables the frob dragpoints on each limb.
		}

		if (isThrow)
		{
			//Throw it forward a little.
			idVec3 viewPoint;
			idMat3 viewAxis;
			idVec3 pushForce;

			gameLocal.GetLocalPlayer()->GetViewPos(viewPoint, viewAxis);
			pushForce = viewAxis.ToAngles().ToForward() * BODYDRAG_PUSHFORCE;

			//dragEnt.GetEntity()->ApplyImpulse(dragEnt.GetEntity(), 0, dragEnt.GetEntity()->GetPhysics()->GetOrigin(), pushForce);
			//dragEnt.GetEntity()->GetPhysics()->ApplyImpulse(0, dragEnt.GetEntity()->GetPhysics()->GetOrigin(), pushForce);
			dragEnt.GetEntity()->GetPhysics()->SetLinearVelocity(pushForce);
		}
	}	

	isDragging = false;
	dragEnt = NULL;

	if ( cursor )
	{
		cursor->BecomeInactive( TH_THINK );
	}
}

//When player starts dragging a body.
bool idBodyDragger::StartDrag(idEntity *ent, jointHandle_t dragJoint, idVec3 jointPos, int _id)
{
	idVec3 viewPoint, origin;
	idMat3 viewAxis, axis;	

	joint = dragJoint;
	id = _id;

	if (!cursor) {
		cursor = (idCursor3D *)gameLocal.SpawnEntityType(idCursor3D::Type);
	}

	gameLocal.GetLocalPlayer()->GetViewPos(viewPoint, viewAxis);

	idPhysics *phys = ent->GetPhysics();
	localPlayerPoint = (jointPos - viewPoint) * viewAxis.Transpose();
	origin = phys->GetOrigin(id);
	axis = phys->GetAxis(id);
	localEntityPoint = (jointPos - origin) * axis.Transpose();

	cursor->drag.Init(g_dragDamping.GetFloat());
	cursor->drag.SetPhysics(phys, id, localEntityPoint);
	cursor->Show();
	cursor->drawDebuglines = false;

	if (phys->IsType(idPhysics_AF::Type) || phys->IsType(idPhysics_RigidBody::Type) || phys->IsType(idPhysics_Monster::Type))
	{
		gameLocal.GetLocalPlayer()->DropCurrentCarryable(); //Drop any current carryables, if applicable.
		
		if (isDragging) //if carrying a body, then drop it.
		{
			StopDrag(false);
		}

		dragEnt = ent;

		cursor->SetOrigin(jointPos);
		cursor->drag.SetDragPosition(jointPos);
		cursor->SetAxis(viewAxis);

		cursor->BecomeActive(TH_THINK);
		isDragging = true;

		//update hud to display name of person you're dragging.
		gameLocal.GetLocalPlayer()->hud->SetStateString("equippedweapon", ent->displayName);
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("weaponChangeForce");

		//ent->GetPhysics()->GetClipModel()->SetOwner(gameLocal.GetLocalPlayer()); //make player the owner so that the player frob check goes through this corpse

		return true;
	}

	return false;
}

void idBodyDragger::Update( idPlayer *player )
{
	idVec3 viewPoint;
	idMat3 viewAxis, axis;

	player->GetViewPos(viewPoint, viewAxis);

	// if there is an entity selected for dragging
	idEntity *drag = dragEnt.GetEntity();
	if ( drag )
	{
		//if ( !( player->usercmd.buttons & BUTTON_ATTACK ) ) {
		//	StopDrag();
		//	return;
		//}

		//gameRenderWorld->DebugArrowSimple(localEntityPoint, 10);

		cursor->SetOrigin( viewPoint + localPlayerPoint * viewAxis );
		cursor->SetAxis( viewAxis );

		cursor->drag.SetDragPosition( cursor->GetPhysics()->GetOrigin() );

		renderEntity_t *renderEntity = drag->GetRenderEntity();
		idAnimator *dragAnimator = drag->GetAnimator();

		if ( joint != INVALID_JOINT && renderEntity && dragAnimator )
		{
			dragAnimator->GetJointTransform( joint, gameLocal.time, cursor->draggedPosition, axis );
			cursor->draggedPosition = renderEntity->origin + cursor->draggedPosition * renderEntity->axis;
			//gameRenderWorld->DrawText( va( "%s\n%s\n%s, %s", drag->GetName(), drag->GetType()->classname, dragAnimator->GetJointName( joint ), bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
		else
		{
			cursor->draggedPosition = cursor->GetPhysics()->GetOrigin();
			//gameRenderWorld->DrawText( va( "%s\n%s\n%s", drag->GetName(), drag->GetType()->classname, bodyName.c_str() ), cursor->GetPhysics()->GetOrigin(), 0.1f, colorWhite, viewAxis, 1 );
		}
	}
	else if (isDragging)
	{
		StopDrag(false);
	}
}





//Ruler

idRuler::idRuler(void)
{
}

/*
==============
idDragEntity::~idDragEntity
==============
*/
idRuler::~idRuler(void)
{
}

void idRuler::Update(idPlayer *player)
{
	trace_t tr;
	gameLocal.clip.TracePoint(tr, player->GetEyePosition(), player->GetEyePosition() + player->viewAngles.ToForward() * 2047, MASK_SOLID, player);

	if (g_ruler.GetInteger() >= 2)
	{
		//arbitrary		
		gameRenderWorld->DebugArrow(colorGreen, startingPoint, tr.endpos, 4, 10);

		int dist = (startingPoint - tr.endpos).Length();
		dist = idMath::Abs(dist);
		gameRenderWorld->DrawText(va("%d", dist), tr.endpos + idVec3(0, 0, 8), 0.4f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		return;
	}

	//orthogonal axis-aligned ruler.

	//Determine which axis the player wants to measure.
	int rulerAxis;
	int xAxis = idMath::Fabs(tr.endpos.x - startingPoint.x);
	int yAxis = idMath::Fabs(tr.endpos.y - startingPoint.y);
	int zAxis = idMath::Fabs(tr.endpos.z - startingPoint.z);
	if (xAxis > yAxis && xAxis > zAxis)
		rulerAxis = 0;
	else if (yAxis > xAxis && yAxis > zAxis)
		rulerAxis = 1;
	else
		rulerAxis = 2;

	idVec3 circleAxis;
	idVec3 endPoint;
	if (rulerAxis == 0)
	{
		//x axis.
		endPoint = idVec3(tr.endpos.x, startingPoint.y, startingPoint.z);
		circleAxis = idVec3(1, 0, 0);
	}
	else if (rulerAxis == 1)
	{
		//y axis.
		endPoint = idVec3(startingPoint.x, tr.endpos.y, startingPoint.z);
		circleAxis = idVec3(0, 1, 0);
	}
	else
	{
		//z axis.
		endPoint = idVec3(startingPoint.x, startingPoint.y, tr.endpos.z);
		circleAxis = idVec3(0, 0, 1);		
	}

	gameRenderWorld->DebugCircle(colorGreen, startingPoint, circleAxis, 3, 6, 10);

	gameRenderWorld->DebugArrow(colorGreen, startingPoint, endPoint, 4, 10);

	int dist = (startingPoint - endPoint).Length();
	dist = idMath::Abs(dist);
	gameRenderWorld->DrawText(va("%d", dist), endPoint + idVec3(0,0,8), 0.4f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());	

}

void idRuler::SetStartingPoint(idPlayer *player)
{
	//Get the starting point for the ruler.
	trace_t tr;
	gameLocal.clip.TracePoint(tr, player->GetEyePosition(), player->GetEyePosition() + player->viewAngles.ToForward() * 1024, MASK_SOLID, player);
	startingPoint = tr.endpos + tr.c.normal * 0.1f; //bump it out of the wall a little bit.
}