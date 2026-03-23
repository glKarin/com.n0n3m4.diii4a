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

#ifndef __SBMODULE_H__
#define __SBMODULE_H__

class sbShip;
class sbConsole;

const int MAX_GENERATED_PROJECTILES = 21;//31;//15;//63;

// boyette space command begin
const float MAX_POSSIBLE_MODULE_POWER = 8.0f;
const int MAX_MAX_MODULE_POWER = 8;
// boyette space command end

class sbModule : public idEntity {

public:
			CLASS_PROTOTYPE( sbModule ); //the necessary idClass prototypes

						sbModule();

	virtual void		Spawn( void );
	virtual void		Save( idSaveGame *savefile ) const;
	virtual void		Restore( idRestoreGame *savefile );

	sbConsole*			ParentConsole;

	sbModule*			CurrentTargetModule;		// boyette mod

	int					GetCaptainTestNumber(); // boyette mod
	virtual void		RecieveVolley();		// boyette mod

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
							// adds a damage effect like overlays, blood, sparks, debris etc.



	int				module_efficiency;
	int				module_buffed_amount;
	int				buff_time;
	virtual void	RecalculateModuleEfficiency();		// boyette mod

	void			Event_ResetBuffAmount( void );
	virtual void	BuffModule(int amount_to_buff);

	int				power_allocated;
	int				max_power;

	int				damage_adjusted_max_power;
	int				automanage_target_power_level;

	float			max_charge_amount;
	float			current_charge_amount;
	float			current_charge_percentage; // current_charge_amount / max_charge_amount
	int				ms_between_charge_cycles;
	float			charge_added_each_cycle;
	int				last_charge_cycle_time;
	virtual void	UpdateChargeAmount();

	virtual void	DoStuffAfterAllMapEntitiesHaveSpawned();

	bool			module_was_just_damaged;
	bool			module_was_just_repaired;

	bool			IsSelectedModule;

					// applies damage to this entity
	virtual	void	RecieveShipToShipDamage( idEntity *attacker, int damageAmount );

	const const char *	recieve_damage_fx;
	const idDict *		projectileDef;
	idEntityPtr<idProjectile> projectilearray[MAX_GENERATED_PROJECTILES];
	int					projectileCounter;

	bool				allow_module_recieve_damage_projectiles;

	//idEntityFx*			shipToShipDamageFX; // not necessary

	float			module_buffed_amount_modifier;

	virtual	void		SetRenderEntityGui0String( const char* varName, const char* value );
	virtual	void		SetRenderEntityGui0Bool( const char* varName, bool value );
	virtual	void		SetRenderEntityGui0Int( const char* varName, int value );
	virtual	void		SetRenderEntityGui0Float( const char* varName, float value );
	virtual	void		HandleNamedEventOnGui0( const char* eventName );

	// gui
	virtual bool		HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );
	int					gui_minigame_sequence_counter;

	void				Event_LoopEfficiencyCalculation( void );
	bool				loop_efficiency_calculation;

protected:
	int					testNumber;


};


#endif /* __GAME_SBMODULE_H__ */