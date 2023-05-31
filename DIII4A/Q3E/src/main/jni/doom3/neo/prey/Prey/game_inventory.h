
#ifndef __PREY_GAME_INVENTORY_H__
#define __PREY_GAME_INVENTORY_H__


class hhInventory : public idInventory {

public:
	virtual void		GetPersistantData( idDict &dict );
	virtual void		RestoreInventory( idPlayer *owner, const idDict &dict );
	virtual bool		Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud );
	virtual void		Clear();
	virtual int			MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const;

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	bool				GiveItem( const idDict& spawnArgs, const idDict* item );
	int					HasAmmo( ammo_t type, int amount );
	bool				UseAmmo( ammo_t type, int amount );
	int					HasAmmo( const char *weapon_classname );
	int					HasAltAmmo( const char *weapon_classname );
	float				AmmoPercentage(idPlayer *player, ammo_t ammoType);

	idDict*				FindItem( const idDict* dict );
	idDict*				FindItem( const char *name );
	void				EvaluateRequirements( idPlayer *p );
	void				AddPickupName( const char *name, const char *icon, bool bIsWeapon );

	// Requirements are precomputed after any inventory item change, for faster access
	struct {
		bool			bCanWallwalk		: 1;
		bool			bCanSpiritWalk		: 1;
		bool			bCanSummonTalon		: 1;
		bool			bCanUseBowVision	: 1;
		bool			bCanUseLighter		: 1;
		bool			bHunterHand			: 1;
		bool			bCanDeathWalk		: 1;
	} requirements;
	int					maxSpirit;
	bool				bHasDeathwalked;
	int					storedHealth;

	//bjk: persistent weapons
	idStr				energyType;
	bool				altMode[ MAX_WEAPONS ];
	bool				weaponRaised[ MAX_WEAPONS ];
	int					lastShot[ MAX_WEAPONS ];		//HUMANHEAD bjk PATCH 7-27-06
	int					zoomFov;
};


#endif /* __PREY_INVENTORY_H__ */

