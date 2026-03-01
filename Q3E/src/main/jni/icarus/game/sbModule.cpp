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

#include "sbShip.h"

const idEventDef EV_ResetBuffAmount( "<resetBuffAmount>", NULL );
const idEventDef EV_LoopEfficiencyCalculation( "<loopEfficiencyCalculation>", NULL );

CLASS_DECLARATION(idEntity, sbModule)

EVENT( EV_ResetBuffAmount,	sbModule::Event_ResetBuffAmount )
EVENT( EV_LoopEfficiencyCalculation,	sbModule::Event_LoopEfficiencyCalculation )

END_CLASS

sbModule::sbModule() {
	team	= 0;
	entity_max_health = 0;
	module_efficiency = 0;
	module_buffed_amount = 0;
	power_allocated = 0;
	max_power = 0;
	max_charge_amount = 0;
	current_charge_amount = 0;
	current_charge_percentage = 0;
	ms_between_charge_cycles = 0;
	charge_added_each_cycle = 0;
	last_charge_cycle_time = 0;
	module_was_just_damaged = false;
	module_was_just_repaired = false;
	IsSelectedModule = false;
	CurrentTargetModule = NULL;

	recieve_damage_fx = NULL;
	projectileDef = NULL;
	projectileCounter = 0;

	ParentConsole = NULL;

	automanage_target_power_level = 0;
	damage_adjusted_max_power = 0;

	module_buffed_amount_modifier = 0;

	gui_minigame_sequence_counter = 0;

	loop_efficiency_calculation = false;

	allow_module_recieve_damage_projectiles = false;
}
void sbModule::Spawn() {
	testNumber = spawnArgs.GetInt("testNumber"); //the amount to regen each entity, each frame
	fl.takedamage = spawnArgs.GetBool("allow_damage","0"); // boyette note - might be a good idea to give this to all entities - but it is not really necessary and that would require putting this flag on all the proper entities.
	spawnArgs.GetInt( "team", "0", team );
// not necessary - same as idEntity begin - idEntity::Spawn gets called for all entities derived from idEntity on gameLocal spawn.
	/*
	if ( spawnArgs.GetInt( "entity_max_health", "100", entity_max_health ) ) {
		spawnArgs.GetInt( "entity_max_health", "100", entity_max_health );
	} else {
		entity_max_health = spawnArgs.GetInt( "health" );
	}
	health = spawnArgs.GetInt( "health" );
	if ( health > entity_max_health ) {
		health = entity_max_health;
	}
	*/
// not necessary - same as idEntity end
	spawnArgs.GetInt( "module_efficiency", "0", module_efficiency );
	spawnArgs.GetInt( "module_buffed_amount", "0", module_buffed_amount );
	power_allocated = 0;
	spawnArgs.GetInt( "max_power", "1", max_power );

	spawnArgs.GetFloat("max_charge_amount", "100", max_charge_amount);
	current_charge_amount = max_charge_amount;
	current_charge_percentage = (current_charge_amount / max_charge_amount);
	spawnArgs.GetInt("ms_between_charge_cycles", "100", ms_between_charge_cycles);
	spawnArgs.GetFloat("charge_added_each_cycle", "0.2", charge_added_each_cycle);
	last_charge_cycle_time = gameLocal.time;

	recieve_damage_fx = spawnArgs.GetString( "recieve_damage_fx", "fx/steve_space_command/module_sparks_default" );
	projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "recieve_damage_projectile_def", "projectile_module_ship_damage_default" ), false );
	projectileCounter = 0;

	allow_module_recieve_damage_projectiles = spawnArgs.GetBool("allow_module_recieve_damage_projectiles","1");

	if ( health <= 0 ) {
		SetRenderEntityGuisBools("module_disabled",true);
	} else {
		SetRenderEntityGuisBools("module_disabled",false);
	}
	if ( health < entity_max_health && health > 0 ) {
		SetRenderEntityGuisBools("module_damaged",true);
	} else {
		SetRenderEntityGuisBools("module_damaged",false);
	}

	// this is temporary until we get the spinners set up on the captain gui.
	automanage_target_power_level = 0;

	module_buffed_amount_modifier = 1.0f;

	RecalculateModuleEfficiency();
}

void sbModule::Save( idSaveGame *savefile ) const {
	// BOYETTE SAVE BEGIN
	savefile->WriteObject( ParentConsole );

	savefile->WriteObject( CurrentTargetModule );

	savefile->WriteInt( module_efficiency );
	savefile->WriteInt( module_buffed_amount );
	savefile->WriteInt( buff_time );

	savefile->WriteInt( power_allocated );
	savefile->WriteInt( max_power );

	savefile->WriteInt( damage_adjusted_max_power );
	savefile->WriteInt( automanage_target_power_level );

	savefile->WriteFloat( max_charge_amount );
	savefile->WriteFloat( current_charge_amount );
	savefile->WriteFloat( current_charge_percentage );
	savefile->WriteInt( ms_between_charge_cycles );
	savefile->WriteFloat( charge_added_each_cycle );
	savefile->WriteInt( last_charge_cycle_time );

	savefile->WriteBool( module_was_just_damaged );
	savefile->WriteBool( module_was_just_repaired );

	//recieve_damage_fx = spawnArgs.GetString( "recieve_damage_fx", "fx/steve_space_command/module_sparks_default" );
	idStr projectileDefName;
	spawnArgs.GetString("def_spaceship_torpedo", "projectile_spaceship_torpedo_default", projectileDefName );
	savefile->WriteString( projectileDefName );
	for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
		projectilearray[i].Save( savefile );
	}
	savefile->WriteInt( projectileCounter );

	savefile->WriteBool( allow_module_recieve_damage_projectiles );

	savefile->WriteFloat( module_buffed_amount_modifier );

	savefile->WriteInt( gui_minigame_sequence_counter );

	savefile->WriteBool( loop_efficiency_calculation );

	savefile->WriteInt( testNumber );
	// BOYETTE SAVE END
}

void sbModule::Restore( idRestoreGame *savefile ) {
	// BOYETTE RESTORE BEGIN
	savefile->ReadObject( reinterpret_cast<idClass *&>( ParentConsole ) );

	savefile->ReadObject( reinterpret_cast<idClass *&>( CurrentTargetModule ) );

	savefile->ReadInt( module_efficiency );
	savefile->ReadInt( module_buffed_amount );
	savefile->ReadInt( buff_time );

	savefile->ReadInt( power_allocated );
	savefile->ReadInt( max_power );

	savefile->ReadInt( damage_adjusted_max_power );
	savefile->ReadInt( automanage_target_power_level );

	savefile->ReadFloat( max_charge_amount );
	savefile->ReadFloat( current_charge_amount );
	savefile->ReadFloat( current_charge_percentage );
	savefile->ReadInt( ms_between_charge_cycles );
	savefile->ReadFloat( charge_added_each_cycle );
	savefile->ReadInt( last_charge_cycle_time );

	savefile->ReadBool( module_was_just_damaged );
	savefile->ReadBool( module_was_just_repaired );

	recieve_damage_fx = spawnArgs.GetString( "recieve_damage_fx", "fx/steve_space_command/module_sparks_default" );
	idStr projectileDefName;
	savefile->ReadString( projectileDefName );
	if ( projectileDefName.Length() ) {
		projectileDef = gameLocal.FindEntityDefDict( projectileDefName );
	} else {
		projectileDef = NULL;
	}
	for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
		projectilearray[i].Restore( savefile );
	}
	savefile->ReadInt( projectileCounter );

	savefile->ReadBool( allow_module_recieve_damage_projectiles );

	savefile->ReadFloat( module_buffed_amount_modifier );

	savefile->ReadInt( gui_minigame_sequence_counter );

	savefile->ReadBool( loop_efficiency_calculation );

	savefile->ReadInt( testNumber );
	// BOYETTE RESTORE END
}

int sbModule::GetCaptainTestNumber() {
	return captaintestnumber;
}

void sbModule::RecieveVolley() {
	captaintestnumber = 6;
}

void sbModule::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
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
		health = idMath::ClampInt(-1,entity_max_health,health); // boyette - Modules cannot be killed - will eventually replace 1 with "module_min_health" and 200 with "entity_max_health" - possibly
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

	RecalculateModuleEfficiency();

	if ( ParentConsole && ParentConsole->ParentShip ) {
		//gameLocal.Printf( ParentConsole->ParentShip->name );
		ParentConsole->ParentShip->ReturnToReserveExcessPowerFromDamagedModules(); // I think we only need to run this when the damage is greater than zero. So we can put it in the if statement below.
		if ( ParentConsole->ParentShip->modules_power_automanage_on ) {
			ParentConsole->ParentShip->AutoManageModulePowerlevels();
		}
	}

	if ( damage > 0 ) {
		module_was_just_damaged = true; // for the in-game world module screens
		was_just_damaged = true;
	}
	if ( damage < 0 ) {
		module_was_just_repaired = true;
		was_just_repaired = true;

		// BOYETTE NOTE BEGIN: 10/31/2016 - not sure about whether we should do this since modules could be repaired but hull could still be extremely low
		for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
			if ( projectilearray[i].GetEntity() ) {
				if ( GetPhysics() && GetPhysics()->GetAbsBounds().IntersectsBounds( projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
					projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
					projectilearray[i].GetEntity()->Event_Remove();
					projectilearray[i] = NULL;
					break;
				}
			}
		}
		// BOYETTE NOTE END
	}

	if ( health <= 0 ) {
		SetRenderEntityGuisBools("module_disabled",true);
	} else {
		SetRenderEntityGuisBools("module_disabled",false);
	}
	if ( health < entity_max_health && health > 0 ) {
		SetRenderEntityGuisBools("module_damaged",true);
	} else {
		SetRenderEntityGuisBools("module_damaged",false);
	}

	SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
}

void sbModule::RecieveShipToShipDamage( idEntity *attacker, int damageAmount ) {

	if ( !attacker ) {
		attacker = gameLocal.world;
	}
	if ( damageAmount != 0 ) {
	// do the damage
		health -= damageAmount;
		// boyette space command begin
		if (health == 0) { health = -1; }
		health = idMath::ClampInt(-1,entity_max_health,health); // boyette - Modules cannot be killed - will eventually replace 1 with "module_min_health" and 200 with "entity_max_health" - possibly
		// boyette space command end
	}

	RecalculateModuleEfficiency();

	if ( damageAmount > 0 ) {
		module_was_just_damaged = true;
		was_just_damaged = true;

		if ( !recieve_damage_fx ) {
			recieve_damage_fx = spawnArgs.GetString( "recieve_damage_fx", "fx/steve_space_command/module_sparks_default" );
		}
		idVec3 recieve_damage_fx_origin_offset = spawnArgs.GetVector( "recieve_damage_fx_origin_offset", "0 0 40" );
		idVec3 tmp = GetPhysics()->GetOrigin() + recieve_damage_fx_origin_offset;
		idEntityFx::StartFx( recieve_damage_fx, &tmp, 0, this, true ); // the 40 is just to get it off the ground.

		// BOYETTE MODULE RECIEVE DAMAGE PROJECTILES BEGIN
		// BOYETTE NOTE IMPORTANT: could never get this to work correctly. Everything is created correctly, but when the objects' destructors are eventually called, it crashes in the destructor  for an unknow reason. Maybe rule of three related but havn't been able to confirm that.
		
		if ( g_enableModuleRecieveDamageProjectiles.GetBool() && allow_module_recieve_damage_projectiles && ParentConsole && ParentConsole->ParentShip && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx == ParentConsole->ParentShip->stargridpositionx && gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony == ParentConsole->ParentShip->stargridpositiony ) {
			
			idVec3 recieve_damage_projectiles_launch_offset = spawnArgs.GetVector( "recieve_damage_projectiles_launch_offset", "0 0 0" );

			float recieve_damage_projectiles_launch_z_vector_min = spawnArgs.GetFloat( "recieve_damage_projectiles_launch_z_vector_min", "-1.0" ); // limit their downward trajectory
			float recieve_damage_projectiles_launch_z_vector_max = spawnArgs.GetFloat( "recieve_damage_projectiles_launch_z_vector_max", "1.0" ); // limit their upward trajectory
			float recieve_damage_projectiles_launch_z_vector_divisor = idMath::ClampFloat( 1.0f, idMath::INFINITY, spawnArgs.GetFloat( "recieve_damage_projectiles_launch_z_vector_divisor", "5.0" ) ); // this makes them go more horizontel than the original
			
			if ( !projectileDef ) {
				projectileDef = gameLocal.FindEntityDefDict( spawnArgs.GetString( "recieve_damage_projectile_def", "projectile_module_ship_damage_default" ), false );
			}
			if ( projectileDef ) {
				if ( projectileCounter >= MAX_GENERATED_PROJECTILES - 5 ) {
					projectileCounter = 0;
				}
				for ( int i = projectileCounter; i <= (projectileCounter + 5) ; ++i ) { // need to figure out why some of the projectiles move really slow.
					if ( projectilearray[i].GetEntity() ) {
						projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
						projectilearray[i].GetEntity()->Event_Remove();
						projectilearray[i] = NULL;
					}
					float randx;
					float randy;
					float randz;
					randx = gameLocal.random.CRandomFloat();
					randy = gameLocal.random.CRandomFloat();
					randz = idMath::ClampFloat(recieve_damage_projectiles_launch_z_vector_min, recieve_damage_projectiles_launch_z_vector_max, gameLocal.random.CRandomFloat() / recieve_damage_projectiles_launch_z_vector_divisor); // this makes them go more horizontel than the original
					// originally was: randz = (gameLocal.random.CRandomFloat()/5);
					idEntity *ent;
					const char *clsname;
					if ( !projectilearray[i].GetEntity() ) {
						gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
						if ( !ent ) {
							clsname = projectileDef->GetString( "classname" );
							gameLocal.Error( "Could not spawn entityDef '%s'", clsname );
						}
						if ( !ent->IsType( idProjectile::Type ) ) {
							clsname = ent->GetClassname();
							gameLocal.Error( "'%s' is not an idProjectile", clsname );
						}
						projectilearray[i] = ( idProjectile * )ent; // boyette note: this is c-style cast that is converting(or designating) an idEntity into an idProjectile. A dynamic_cast or possibly a static_cast would probably work fine here instead but I haven't tried it yet.
					}
					projectilearray[i].GetEntity()->Create( this, GetPhysics()->GetAbsBounds().GetCenter() + recieve_damage_projectiles_launch_offset, idVec3(randx,randy,randz) ); // the 40 is just to get it off the gorund.
					projectilearray[i].GetEntity()->Launch(GetPhysics()->GetAbsBounds().GetCenter() + recieve_damage_projectiles_launch_offset,idVec3(randx,randy,randz), vec3_origin,0.0f,1.0f,1.0f,gameLocal.random.RandomInt(1200)); // we need a spawnarg here for 1200. // THIS WORKS GREAT. // the 40 is just to get it off the gorund.
					//char buffer [33]; // good for 32 bit systems and above.
					//gameLocal.Printf( (const char*)itoa(i,buffer,10) );
				}
				projectileCounter = projectileCounter + 5;
			}
			if ( ParentConsole && ParentConsole->ParentShip && ParentConsole->ParentShip->shields_raised ) {
				gameLocal.RadiusDamage( GetPhysics()->GetOrigin() + recieve_damage_fx_origin_offset, this, this, this, this, "damage_ship_to_ship_module_explosion", 1.0f - (float)ParentConsole->ParentShip->shieldStrength/(float)ParentConsole->ParentShip->max_shieldStrength );
			} else {
				gameLocal.RadiusDamage( GetPhysics()->GetOrigin() + recieve_damage_fx_origin_offset, this, this, this, this, "damage_ship_to_ship_module_explosion", 1.0f - (float)health/(float)entity_max_health );
			}
		} else {
		// BOYETTE MODULE RECIEVE DAMAGE PROJECTILES END
			if ( ParentConsole && ParentConsole->ParentShip && ParentConsole->ParentShip->shields_raised ) {
				gameLocal.RadiusDamage( GetPhysics()->GetOrigin() + recieve_damage_fx_origin_offset, this, this, this, this, "damage_ship_to_ship_module_explosion", 1.0f - (float)ParentConsole->ParentShip->shieldStrength/(float)ParentConsole->ParentShip->max_shieldStrength );
			} else {
				gameLocal.RadiusDamage( GetPhysics()->GetOrigin() + recieve_damage_fx_origin_offset, this, this, this, this, "damage_ship_to_ship_module_explosion", 1.0f - (float)health/(float)entity_max_health );
			}
		}
	}

	if ( ParentConsole && ParentConsole->ParentShip ) {
		//gameLocal.Printf( ParentConsole->ParentShip->name );
		ParentConsole->ParentShip->ReturnToReserveExcessPowerFromDamagedModules(); // I think we only need to run this when the damage is greater than zero. So we can put it in the if statement below.
		if ( ParentConsole->ParentShip->modules_power_automanage_on ) {
			ParentConsole->ParentShip->AutoManageModulePowerlevels();
		}

		if ( ParentConsole->ParentShip->ViewScreenEntity && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard && gameLocal.GetLocalPlayer()->ShipOnBoard == ParentConsole->ParentShip ) {
			ParentConsole->ParentShip->ViewScreenEntity->HandleNamedEventOnGuis( "ParentShipRecievedDamage" );
		}
	}

	if ( damageAmount < 0 ) {
		module_was_just_repaired = true;
	}

	if ( health <= 0 ) {
		SetRenderEntityGuisBools("module_disabled",true);
	} else {
		SetRenderEntityGuisBools("module_disabled",false);
	}
	if ( health < entity_max_health && health > 0 ) {
		SetRenderEntityGuisBools("module_damaged",true);
	} else {
		SetRenderEntityGuisBools("module_damaged",false);
	}

	SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
}

void sbModule::UpdateChargeAmount() {
	// if module is disabled(health is less than or equal to 0) the module charge will reduce every cycle by the charge_added_each_cycle.
	if ( health >= 0 ) {
		if ( current_charge_amount < max_charge_amount ) {
			if ( (last_charge_cycle_time + ms_between_charge_cycles) < gameLocal.time ) {
				current_charge_amount = current_charge_amount + (((float)module_efficiency/100.f) * charge_added_each_cycle);
				last_charge_cycle_time = gameLocal.time;
				// clamp it down.
				if ( current_charge_amount > max_charge_amount ) {
					current_charge_amount = idMath::ClampInt(0,max_charge_amount,current_charge_amount);
				}
				// this is just for display on the CaptainGui - it is not used for anything.
				current_charge_percentage = ( current_charge_amount / max_charge_amount );
			}
		}
	} else {
		if ( current_charge_amount > 0 ) {
			if ( (last_charge_cycle_time + ms_between_charge_cycles) < gameLocal.time ) {
				current_charge_amount = current_charge_amount - charge_added_each_cycle;
				last_charge_cycle_time = gameLocal.time;
				// clamp it down.
				if ( current_charge_amount > max_charge_amount ) {
					current_charge_amount = idMath::ClampInt(0,max_charge_amount,current_charge_amount);
				}
				// this is just for display on the CaptainGui - it is not used for anything.
				current_charge_percentage = ( current_charge_amount / max_charge_amount );
			}
		}
	}
}

void sbModule::RecalculateModuleEfficiency() {
	float captain_chair_buff_amount = 0;
	if ( ParentConsole && ParentConsole->ParentShip && power_allocated > 0 ) {
		if ( ParentConsole->ParentShip->CaptainChair && ParentConsole->ParentShip->CaptainChair->SeatedEntity.GetEntity() && ParentConsole->ParentShip->CaptainChair->SeatedEntity.GetEntity()->team == team ) {
			if ( module_buffed_amount > 0 ) {
				captain_chair_buff_amount = 15.0f * module_buffed_amount_modifier;
				if ( captain_chair_buff_amount < 5.0 ) {
					captain_chair_buff_amount = 5.0f;
				}
			} else {
				captain_chair_buff_amount = 5.0f;
			}
		} else if ( ParentConsole->ParentShip->ReadyRoomCaptainChair && ParentConsole->ParentShip->ReadyRoomCaptainChair->SeatedEntity.GetEntity() && ParentConsole->ParentShip->ReadyRoomCaptainChair->SeatedEntity.GetEntity()->team == team ) {
			if ( module_buffed_amount > 0 ) {
				captain_chair_buff_amount = 15.0f * module_buffed_amount_modifier;
				if ( captain_chair_buff_amount < 5.0 ) {
					captain_chair_buff_amount = 5.0f;
				}
			} else {
				captain_chair_buff_amount = 5.0f;
			}
		}
	}
	//module_efficiency = idMath::Rint(((health + module_buffed_amount) / (float)entity_max_health) * ((float)power_allocated/(float)max_power) * 100.f); 
	// original // module_efficiency = idMath::Rint((health / (float)entity_max_health) * ((float)power_allocated/(float)max_power) * (1.f + (float)module_buffed_amount/100.f) * 100.f); // the guis can not display floats currently - so we need to round the to an int. boyette - will eventually replace 200 with "entity_max_health"
	module_efficiency = idMath::Rint(((float)power_allocated/(float)max_power)* 100.f); // need to round to an int for gui
	module_efficiency = idMath::ClampInt(0,idMath::Rint(((float)health / (float)entity_max_health) * 100.f),module_efficiency) + idMath::Rint( (float)module_buffed_amount * module_buffed_amount_modifier ) + idMath::Rint(((float)power_allocated/(float)max_power) * captain_chair_buff_amount);

	// damage_adjusted_max_power = idMath::Rint((((float)health / (float)entity_max_health)) * (float)max_power); // ROUNDDOWN TO INT VERSION
	damage_adjusted_max_power = idMath::Ceil( ((float)health / (float)entity_max_health) * (float)max_power ); // ROUNDUP TO INT VERSION

	if ( health <= 0 ) {
		module_efficiency = 0;
	}
	SetShaderParm( 10, ((float)module_efficiency/100.f) ); // set the module efficiency parm
}

void sbModule::BuffModule(int amount_to_buff) {
	buff_time = gameLocal.time;
	module_buffed_amount = amount_to_buff;
	PostEventMS( &EV_ResetBuffAmount, 3002 ); // boyette note - still need to figure this out - but it works. - figured it out - this amount(and the 3000 in ResetBuffAmount) needs to be greater than the interval between how often idAI::BuffConsolesModule is called in the idAI script - The BuffModule amount needs be slightly higher - hence 3000 + 2.
	RecalculateModuleEfficiency();
}

void sbModule::Event_ResetBuffAmount( void ) {
	if ( (buff_time + 3000) < gameLocal.time ) { // boyette note - might want to have a spawn arg called "buff_lost_delay" or "buff_time_delay" in place of 3000(which is 3 seconds).
		module_buffed_amount = 0;
	}
	RecalculateModuleEfficiency();
}

void sbModule::Event_LoopEfficiencyCalculation( void ) {
	if ( loop_efficiency_calculation ) {
		PostEventMS( &EV_LoopEfficiencyCalculation, 2000 ); // recalculates every 2 seconds
	}
	RecalculateModuleEfficiency();
}

void sbModule::SetRenderEntityGui0String( const char* varName, const char* value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateString( varName , value );
	}
}

void sbModule::SetRenderEntityGui0Bool( const char* varName, bool value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateBool( varName , value );
	}
}

void sbModule::SetRenderEntityGui0Int( const char* varName, int value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateInt( varName , value );
	}
}

void sbModule::SetRenderEntityGui0Float( const char* varName, float value ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->SetStateFloat( varName , value );
	}
}

void sbModule::HandleNamedEventOnGui0( const char* eventName ) {
	if ( renderEntity.gui[0] ) {
		renderEntity.gui[0]->HandleNamedEvent( eventName );
	}
}

/*
================
sbModule::HandleSingleGuiCommand
================
*/
bool sbModule::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	// GUI MINIGAME BEGIN
	if ( token.Icmp( "increase_gui_minigame_repair_sequence" ) == 0 ) {
		gui_minigame_sequence_counter++;
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetRenderEntityGui0Int( "random_x_position", gameLocal.random.RandomInt(796) ); // get random x position within the designated gui area;
		SetRenderEntityGui0Float( "random_x_position_float", gameLocal.random.RandomFloat() ); // get random x position within the designated gui area;
		SetRenderEntityGui0Int( "random_y_position", gameLocal.random.RandomInt(840) ); // get random y position within the designated gui area;
		SetRenderEntityGui0Float( "random_y_position_float", gameLocal.random.RandomFloat() ); // get random y position within the designated gui area;
		// BOYETTE NOTE: this was just for testing: gameLocal.GetLocalPlayer()->DisplayNonInteractiveAlertMessage( "^1The " + idStr(gui_minigame_sequence_counter) + "^1entity \n^1" + name + "\n ^1is ^1damaging ^1random ^1modules ^1of ^1entities ^1in ^1local ^1space\n and that is great." );
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
		for ( int i = 0; i < MAX_GENERATED_PROJECTILES; i++ ) {
			if ( projectilearray[i].GetEntity() ) {
				if ( GetPhysics() && GetPhysics()->GetAbsBounds().IntersectsBounds( projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
					projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
					projectilearray[i].GetEntity()->Event_Remove();
					projectilearray[i] = NULL;
				} else if ( ParentConsole && ParentConsole->GetPhysics() && ParentConsole->GetPhysics()->GetAbsBounds().IntersectsBounds( projectilearray[i].GetEntity()->GetPhysics()->GetAbsBounds() ) ) {
					projectilearray[i].GetEntity()->CancelEvents( &EV_Remove );
					projectilearray[i].GetEntity()->Event_Remove();
					projectilearray[i] = NULL;
				}
			}
		}
		// BOYETTE NOTE END
		
		health = entity_max_health;
		SetRenderEntityGui0Bool("module_disabled",false);
		SetRenderEntityGui0Bool("module_damaged",false);
		RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
		if ( ParentConsole ) {
			ParentConsole->health = ParentConsole->entity_max_health;
			ParentConsole->SetRenderEntityGui0Bool("console_disabled",false);
			ParentConsole->SetRenderEntityGui0Bool("console_damaged",false);
			ParentConsole->SetShaderParm( 11, 1.0f -( (float)ParentConsole->health / (float)ParentConsole->entity_max_health ) ); // set the parent console's damage decal alpha
		}
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "repair_the_module" ) == 0 ) {
		health = entity_max_health;
		SetRenderEntityGui0Bool("module_disabled",false);
		SetRenderEntityGui0Bool("module_damaged",false);
		RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "sabotage_the_console_and_module" ) == 0 ) {
		health = -1;
		SetRenderEntityGui0Bool("module_disabled",true);
		RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
		if ( ParentConsole ) {
			ParentConsole->health = -1;
			ParentConsole->SetRenderEntityGui0Bool("console_disabled",true);
			ParentConsole->SetShaderParm( 11, 1.0f -( (float)ParentConsole->health / (float)ParentConsole->entity_max_health ) ); // set the parent console's damage decal alpha
		}
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	if ( token.Icmp( "sabotage_the_module" ) == 0 ) {
		health = -1;
		SetRenderEntityGui0Bool("module_disabled",true);
		RecieveShipToShipDamage(this,0); // just to update all the things that would normally happen when a module is damaged.
		SetRenderEntityGui0Int( "gui_minigame_sequence_counter", gui_minigame_sequence_counter );
		SetShaderParm( 11, 1.0f -( (float)health / (float)entity_max_health ) ); // set the damage decal alpha
		ActivateGuisIfNotActive( false, gameLocal.time + 5000 );
		return true;
	}
	// GUI MINIGAME END

	src->UnreadToken( &token );
	return false;
}

void sbModule::DoStuffAfterAllMapEntitiesHaveSpawned() {

}
