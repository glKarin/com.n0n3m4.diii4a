/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

CLASS_DECLARATION(idEntity, sbConsole)

END_CLASS

sbConsole::sbConsole() {
	ControlledModule = NULL;
	ParentShip = NULL;
	gui_minigame_sequence_counter = 0;

	min_view_angles	= ang_zero;
	max_view_angles	= ang_zero;
}
void sbConsole::Spawn() {
	testNumber = spawnArgs.GetInt("testNumber"); //the amount to regen each entity, each frame
	fl.takedamage = spawnArgs.GetBool("allow_damage","0"); // boyette note - might be a good idea to give this to all entities - but it is not really necessary and that would require putting this flag on all the proper entities.

	if ( health <= 0 ) {
		SetRenderEntityGuisBools("console_disabled",true);
	} else {
		SetRenderEntityGuisBools("console_disabled",false);
	}
	if ( health < entity_max_health && health > 0 ) {
		SetRenderEntityGuisBools("console_damaged",true);
	} else {
		SetRenderEntityGuisBools("console_damaged",false);
	}

	min_view_angles.yaw = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().yaw + spawnArgs.GetFloat("seated_yaw_min", "-40.0");
	max_view_angles.yaw = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().yaw + spawnArgs.GetFloat("seated_yaw_max", "40.0");
	min_view_angles.pitch = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().pitch + spawnArgs.GetFloat("seated_pitch_min", "-89.0");
	max_view_angles.pitch = spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().pitch + spawnArgs.GetFloat("seated_pitch_max", "40.0");

	min_view_angles.Normalize180();
	max_view_angles.Normalize180();

	console_occupied = false;

}

void sbConsole::Save( idSaveGame *savefile ) const {
	// BOYETTE SAVE BEGIN
	savefile->WriteObject( ControlledModule );

	savefile->WriteObject( ParentShip );

	savefile->WriteInt( gui_minigame_sequence_counter );

	savefile->WriteAngles( min_view_angles );
	savefile->WriteAngles( max_view_angles );

	savefile->WriteBool( console_occupied );

	savefile->WriteInt( testNumber );
	// BOYETTE SAVE END
}

void sbConsole::Restore( idRestoreGame *savefile ) {
	// BOYETTE RESTORE BEGIN
	savefile->ReadObject( reinterpret_cast<idClass *&>( ControlledModule ) );

	savefile->ReadObject( reinterpret_cast<idClass *&>( ParentShip ) );

	savefile->ReadInt( gui_minigame_sequence_counter );

	savefile->ReadAngles( min_view_angles );
	savefile->ReadAngles( max_view_angles );

	savefile->ReadBool( console_occupied );

	savefile->ReadInt( testNumber );
	// BOYETTE RESTORE END
}

int sbConsole::GetCaptainTestNumber() {
	return captaintestnumber;
}

void sbConsole::RecieveVolley() {
	captaintestnumber = 6;
}

/*
================
idEntity::HandleSingleGuiCommand
================
*/
bool sbConsole::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "hide_module" ) == 0 ) {
		if ( ControlledModule ) {
			ControlledModule->Hide();
			return true;
		}
	}

	if ( token.Icmp( "initiate_transport_to_the_targetship_of_parentship" ) == 0 ) {
		if ( ParentShip && ParentShip->TransporterBounds && ParentShip->TargetEntityInSpace && ParentShip->TargetEntityInSpace->TransporterBounds ) {
			if ( ParentShip->consoles[COMPUTERMODULEID] && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount >= ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount && ParentShip->TargetEntityInSpace->shieldStrength >= ( ParentShip->TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters * ParentShip->TargetEntityInSpace->max_shieldStrength ) ) {
				HandleNamedEventOnGui0("TransportAttemptedButTargetShipShieldsAreTooHigh");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			} else if ( ParentShip->consoles[COMPUTERMODULEID] && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule && ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->current_charge_amount < ParentShip->consoles[COMPUTERMODULEID]->ControlledModule->max_charge_amount ) {
				HandleNamedEventOnGui0("TransportAttemptedButNotSequenced");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			} else {
				HandleNamedEventOnGui0("TransportInitiatedSuccessfully");
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
			ParentShip->BeginTransporterSequence();
			return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
		}
	}

	// GUI MINIGAME BEGIN
	if ( token.Icmp( "increase_gui_minigame_repair_sequence" ) == 0 ) {
		gui_minigame_sequence_counter++;
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetRenderEntityGui0Int( "random_x_position", gameLocal.random.RandomInt(796) ); // get random x position within the designated gui area;
		SetRenderEntityGui0Float( "random_x_position_float", gameLocal.random.RandomFloat() ); // get random x position within the designated gui area;
		SetRenderEntityGui0Int( "random_y_position", gameLocal.random.RandomInt(840) ); // get random y position within the designated gui area;
		SetRenderEntityGui0Float( "random_y_position_float", gameLocal.random.RandomFloat() ); // get random y position within the designated gui area;
		return true;
	}
	if ( token.Icmp( "increase_gui_minigame_sabotage_sequence" ) == 0 ) {
		gui_minigame_sequence_counter++;
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetRenderEntityGui0Int( "random_x_position", gameLocal.random.RandomInt(796) );
		SetRenderEntityGui0Float( "random_x_position_float", gameLocal.random.RandomFloat() );
		SetRenderEntityGui0Int( "random_y_position", gameLocal.random.RandomInt(840) );
		SetRenderEntityGui0Float( "random_y_position_float", gameLocal.random.RandomFloat() );
		return true;
	}
	if ( token.Icmp( "reset_gui_minigame_sequence" ) == 0 ) {
		gui_minigame_sequence_counter = 0;
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		return true;
	}
	if ( token.Icmp( "repair_the_console_and_module" ) == 0 ) {
		// BOYETTE NOTE BEGIN: 10/31/2016 - not sure about whether we should do this since modules could be repaired but hull could still be extremely low
		if ( ControlledModule ) {
			for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
				if ( ControlledModule->projectilearray[i].GetEntity() ) {
					if ( GetPhysics() && GetPhysics()->GetAbsBounds().IntersectsBounds( ControlledModule->projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
						ControlledModule->projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
						ControlledModule->projectilearray[i].GetEntity()->Event_Remove();
						ControlledModule->projectilearray[i] = NULL;
					} else if ( ControlledModule && ControlledModule->GetPhysics() && ControlledModule->GetPhysics()->GetAbsBounds().IntersectsBounds( ControlledModule->projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
						ControlledModule->projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
						ControlledModule->projectilearray[i].GetEntity()->Event_Remove();
						ControlledModule->projectilearray[i] = NULL;
					}
				}
			}
		}
		// BOYETTE NOTE END

		health = entity_max_health;
		SetRenderEntityGui0Bool("console_disabled",false);
		SetRenderEntityGui0Bool("console_damaged",false);
		if ( ControlledModule ) {
			ControlledModule->health = ControlledModule->entity_max_health;
			ControlledModule->SetRenderEntityGui0Bool("module_disabled",false);
			ControlledModule->SetRenderEntityGui0Bool("module_damaged",false);
			ControlledModule->RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
			// not necessary, happens in ControlledModule->RecieveShipToShipDamage .. ControlledModule->SetShaderParm( 11, 1.0f -( (float)ControlledModule->health / (float)ControlledModule->entity_max_health ) ); // set the controlled module's damage decal alpha
		}
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "repair_the_console" ) == 0 ) {
		health = entity_max_health;
		SetRenderEntityGui0Bool("console_disabled",false);
		SetRenderEntityGui0Bool("console_damaged",false);
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "sabotage_the_console_and_module" ) == 0 ) {
		health = -1;
		SetRenderEntityGui0Bool("console_disabled",true);
		if ( ControlledModule ) {
			ControlledModule->health = -1;
			ControlledModule->SetRenderEntityGui0Bool("module_disabled",true);
			ControlledModule->RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
			// not necessary, happens in ControlledModule->RecieveShipToShipDamage .. ControlledModule->SetShaderParm( 11, 1.0f -( (float)ControlledModule->health / (float)ControlledModule->entity_max_health ) ); // set the controlled module's damage decal alpha
		}
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "sabotage_the_console" ) == 0 ) {
		health = -1;
		SetRenderEntityGui0Bool("console_disabled",true);
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	// GUI MINIGAME END

	if ( token.Icmp( "player_sit_in_console" ) == 0 ) {
		if ( !console_occupied || (ControlledModule && ControlledModule->module_buffed_amount <= 0) ) {
			// gameLocal.GetLocalPlayer()->SetOrigin( GetPhysics()->GetOrigin() ); - might not be the correct location - need to cadd an adjust.
			// BOYETTE NOTE TODO: need to make a spawnarg on consoles called "player_sitting_origin_fwd_adjust" so that we are sitting in the correct spot when we sit in the chair.
			gameLocal.GetLocalPlayer()->SetOrigin(GetPhysics()->GetOrigin() + ( spawnArgs.GetMatrix("rotation", "1 0 0 0 1 0 0 0 1").ToAngles().ToForward() * spawnArgs.GetFloat("sit_down_distance","40") ) ); // PERFECT - if 40 is not enough we can always increase and if necessary make a spawnarg so it can vary.
			gameLocal.GetLocalPlayer()->Bind( this , true );
			if ( spawnArgs.GetBool("player_can_sit_in_this","1") ) {
				cvarSystem->SetCVarFloat( "pm_crouchviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
				cvarSystem->SetCVarFloat( "pm_normalviewheight", spawnArgs.GetFloat("player_view_sit_height","55") );
			}
			gameLocal.GetLocalPlayer()->ConsoleSeatedIn = this;
			gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage("Press -" + idStr(common->KeysFromBinding("togglemenu")) + "- to exit the console");
			gameLocal.GetLocalPlayer()->DisableWeapon();
			gameLocal.GetLocalPlayer()->disable_crosshairs = true;
			console_occupied = true;
			if ( renderEntity.gui[0] ) {
				renderEntity.gui[0]->Activate(true,gameLocal.time);
			}
			// BOYETTE NOTE TODO - need to make a spawnarg for the player buff amount. also need to look at efficiency calculation frequency in this situation - buffing and efficency recalculation happens continuously with AI's but the player does not do this the same way.
			if ( ControlledModule && ControlledModule->team == gameLocal.GetLocalPlayer()->team ) {
				ControlledModule->module_buffed_amount = 20;
				ControlledModule->RecalculateModuleEfficiency();

				ControlledModule->loop_efficiency_calculation = true;
				ControlledModule->Event_LoopEfficiencyCalculation();
			}
		}
		return true;
	}

	src->UnreadToken( &token );
	return false;
}

void sbConsole::ReleasePlayerCaptain() {
	gameLocal.GetLocalPlayer()->Unbind();
	// gameLocal.GetLocalPlayer()->SetOrigin(GetPhysics()->GetOrigin() + idVec3(0,40,0)); // this will just move it 40 units forward but will not take into account the rotation of the chair.
	gameLocal.GetLocalPlayer()->SetOrigin( GetPhysics()->GetOrigin() ); // need to make sure the origin is far enough behind the console so we don't get stuck inside of it.
	cvarSystem->SetCVarFloat( "pm_crouchviewheight", 32 );
	cvarSystem->SetCVarFloat( "pm_normalviewheight", 68 );
	gameLocal.GetLocalPlayer()->ConsoleSeatedIn = NULL;
	gameLocal.GetLocalPlayer()->disable_crosshairs = false;
	gameLocal.GetLocalPlayer()->EnableWeapon();
	gameLocal.GetLocalPlayer()->RaiseWeapon();
	console_occupied = false;
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->Activate(false,gameLocal.time + 1000);
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->Activate(false,gameLocal.time);
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->Activate(false,gameLocal.time);
	}
	// BOYETTE NOTE TODO - need to make a spawnarg for the player buff amount. also need to look efficiency calculation frequency in this situation - buffing and efficency recalculation happens continuously with AI's but the player does not do this the same way.
	if ( ControlledModule ) {
		ControlledModule->module_buffed_amount = 0;
		ControlledModule->RecalculateModuleEfficiency();

		ControlledModule->loop_efficiency_calculation = false;
	}
}

void sbConsole::SetRenderEntityGuisStrings( const char* varName, const char* value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateString( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateString( varName , value );
	}
}

void sbConsole::SetRenderEntityGuisBools( const char* varName, bool value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateBool( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateBool( varName , value );
	}
}

void sbConsole::SetRenderEntityGuisInts( const char* varName, int value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateInt( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateInt( varName , value );
	}
}

void sbConsole::SetRenderEntityGuisFloats( const char* varName, float value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->SetStateFloat( varName , value );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->SetStateFloat( varName , value );
	}
}

void sbConsole::HandleNamedEventOnGuis( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[1] ) {
		renderEntity.gui[1]->HandleNamedEvent( eventName );
	}
	if ( renderEntity.gui[2] ) {
		renderEntity.gui[2]->HandleNamedEvent( eventName );
	}
}

void sbConsole::SetRenderEntityGui0String( const char* varName, const char* value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString( varName , value );
	}
}

void sbConsole::SetRenderEntityGui0Bool( const char* varName, bool value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateBool( varName , value );
	}
}

void sbConsole::SetRenderEntityGui0Int( const char* varName, int value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( varName , value );
	}
}

void sbConsole::SetRenderEntityGui0Float( const char* varName, float value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateFloat( varName , value );
	}
}

void sbConsole::HandleNamedEventOnGui0( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent( eventName );
	}
}

void sbConsole::DoStuffAfterAllMapEntitiesHaveSpawned() {
	idStr ControlledModuleName = spawnArgs.GetString("controlled_module",NULL);
	ControlledModule = dynamic_cast<sbModule*>( gameLocal.FindEntity( ControlledModuleName ) );
	if ( ControlledModule ) {
		ControlledModule->ParentConsole = this;
	}
}

void sbConsole::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location ) {
 //idEntity::Damage begin
	if ( !fl.takedamage ) {
		return;
	}

	SetTimeState ts( timeGroup );

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage != 0 ) {
	// do the damage
	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}
	int	damage = damageDef->GetInt( "damage" );
		health -= damage;
		// boyette space command begin
		if (health == 0) { health = -1; }
		health = idMath::ClampInt(-1,100,health); // Modules cannot be killed - will eventually replace 1 with "console_min_health" and 200 with "console_max_health" - possibly

		if ( damage > 0 ) {
			was_just_damaged = true;
		}
		if ( damage < 0 ) {
			was_just_repaired = true;

			// BOYETTE NOTE BEGIN: 10/31/2016 - not sure about whether we should do this since modules could be repaired but hull could still be extremely low
			if ( ControlledModule ) {
				for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
					if ( ControlledModule->projectilearray[i].GetEntity() ) {
						if ( GetPhysics() && GetPhysics()->GetAbsBounds().IntersectsBounds( ControlledModule->projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
							ControlledModule->projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
							ControlledModule->projectilearray[i].GetEntity()->Event_Remove();
							ControlledModule->projectilearray[i] = NULL;
							break;
						}
					}
				}
			}
			// BOYETTE NOTE END
		}
		// boyette space command end
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( inflictor, attacker, damage, dir, location );
		} else {
			Pain( inflictor, attacker, damage, dir, location );
		}
	}
// idEntity::Damage end

	if ( health <= 0 ) {
		SetRenderEntityGui0Bool("console_disabled",true);
	} else {
		SetRenderEntityGui0Bool("console_disabled",false);
	}
	if ( health < entity_max_health && health > 0 ) {
		SetRenderEntityGui0Bool("console_damaged",true);
	} else {
		SetRenderEntityGui0Bool("console_damaged",false);
	}

	SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha

}