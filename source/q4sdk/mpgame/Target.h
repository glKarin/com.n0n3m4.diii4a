
#ifndef __GAME_TARGET_H__
#define __GAME_TARGET_H__

//Used to compare two ammoData structs and see who has more.
int					CompareAmmoData( const void* ammo1, const void* ammo2);


/*
===============================================================================

idTarget

===============================================================================
*/

class idTarget : public idEntity {
public:
	CLASS_PROTOTYPE( idTarget );
};
 

/*
===============================================================================

idTarget_Remove

===============================================================================
*/

class idTarget_Remove : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Remove );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Show

===============================================================================
*/

class idTarget_Show : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Show );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Damage

===============================================================================
*/

class idTarget_Damage : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Damage );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SessionCommand

===============================================================================
*/

class idTarget_SessionCommand : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SessionCommand );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_EndLevel

===============================================================================
*/

class idTarget_EndLevel : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_EndLevel );

private:
	void				Event_Activate( idEntity *activator );

};


/*
===============================================================================

idTarget_WaitForButton

===============================================================================
*/

class idTarget_WaitForButton : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_WaitForButton );

	void				Think( void );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_SetGlobalShaderTime

===============================================================================
*/

class idTarget_SetGlobalShaderTime : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetGlobalShaderTime );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetShaderParm

===============================================================================
*/

class idTarget_SetShaderParm : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetShaderParm );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetShaderTime

===============================================================================
*/

class idTarget_SetShaderTime : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetShaderTime );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_FadeEntity

===============================================================================
*/

class idTarget_FadeEntity : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_FadeEntity );

						idTarget_FadeEntity( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Think( void );

private:
	idVec4				fadeFrom;
	int					fadeStart;
	int					fadeEnd;

	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_LightFadeIn

===============================================================================
*/

class idTarget_LightFadeIn : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LightFadeIn );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_LightFadeOut

===============================================================================
*/

class idTarget_LightFadeOut : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LightFadeOut );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_Give

===============================================================================
*/

class idTarget_Give : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Give );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
// RAVEN BEGIN
// abahr: fixing issue with EV_Activate not taking NULL ptrs
	void				Event_PostSpawn();
// RAVEN END
};


/*
===============================================================================

idTarget_GiveEmail

===============================================================================
*/

class idTarget_GiveEmail : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_GiveEmail );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_SetModel

===============================================================================
*/

class idTarget_SetModel : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetModel );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetInfluence

===============================================================================
*/

class idTarget_SetInfluence : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetInfluence );

						idTarget_SetInfluence( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_RestoreInfluence();
	void				Event_GatherEntities();
	void				Event_Flash( float flash, int out );
	void				Event_ClearFlash( float flash );
	void				Think( void );

	idList<int>			lightList;
	idList<int>			guiList;
	idList<int>			soundList;
	idList<int>			genericList;
	float				flashIn;
	float				flashOut;
	float				delay;
	idStr				flashInSound;
	idStr				flashOutSound;
	idEntity *			switchToCamera;
	idInterpolate<float>fovSetting;
	bool				soundFaded;
	bool				restoreOnTrigger;
};


/*
===============================================================================

idTarget_SetKeyVal

===============================================================================
*/

class idTarget_SetKeyVal : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetKeyVal );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetFov

===============================================================================
*/

class idTarget_SetFov : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetFov );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Think( void );

private:
	idInterpolate<int>	fovSetting;

	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetPrimaryObjective

===============================================================================
*/

class idTarget_SetPrimaryObjective : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetPrimaryObjective );

private:
	void				Event_Activate( idEntity *activator );
};

// RAVEN BEGIN
// bdube: added player database
/*
===============================================================================

idTarget_AddDatabaseEntry

===============================================================================
*/

class rvTarget_AddDatabaseEntry : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_AddDatabaseEntry );

private:
	void				Event_Activate( idEntity *activator );
};

// jshepard: secret area discovery

/*
===============================================================================

idTarget_SecretArea

===============================================================================
*/

class rvTarget_SecretArea : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_SecretArea );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

rvTarget_BossBattle

===============================================================================
*/

class rvTarget_BossBattle : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_BossBattle );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_SetShieldPercent( float percent );
	void				Event_SetBossMaxHealth( float f );
	void				Event_AllowShieldBar( float activate );
	void				Event_AllowShieldWarningBar( float activate );

};

/*
===============================================================================

rvTarget_TetherAI

===============================================================================
*/

class rvTarget_TetherAI : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_TetherAI );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

rvTarget_Nailable

===============================================================================
*/

class rvTarget_Nailable : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_Nailable );

private:
	void				Spawn( void );
};


// RAVEN END

/*
===============================================================================

idTarget_LockDoor

===============================================================================
*/

class idTarget_LockDoor: public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LockDoor );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_CallObjectFunction

===============================================================================
*/

class idTarget_CallObjectFunction : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_CallObjectFunction );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_LockDoor

===============================================================================
*/

class idTarget_EnableLevelWeapons : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_EnableLevelWeapons );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Tip

===============================================================================
*/

class idTarget_Tip : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Tip );

						idTarget_Tip( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	idVec3				playerPos;

	void				Event_Activate( idEntity *activator );
	void				Event_TipOff( void );
	void				Event_GetPlayerPos( void );
};

/*
===============================================================================

idTarget_GiveSecurity

===============================================================================
*/
class idTarget_GiveSecurity : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_GiveSecurity );
private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_RemoveWeapons

===============================================================================
*/
class idTarget_RemoveWeapons : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_RemoveWeapons );
private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_LevelTrigger

===============================================================================
*/
class idTarget_LevelTrigger : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LevelTrigger );
private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_EnableStamina

===============================================================================
*/
class idTarget_EnableStamina : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_EnableStamina );
private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_FadeSoundClass

===============================================================================
*/
class idTarget_FadeSoundClass : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_FadeSoundClass );
private:
	void				Event_Activate( idEntity *activator );
	void				Event_RestoreVolume();
};


/*
===============================================================================

rvTarget_LaunchProjectile

===============================================================================
*/

class rvTarget_LaunchProjectile : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_LaunchProjectile );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_LaunchProjectile( idEntity *activator );
};

/*
===============================================================================

rvTarget_ExitAreaAlert

===============================================================================
*/

class rvTarget_ExitAreaAlert : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_ExitAreaAlert );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

rvTarget_AmmoStash

===============================================================================
*/
typedef struct	ammodata_s	{

	int			ammoIndex;
	idStr		ammoName;
	int			ammoCount;
	int			ammoMax;
	float		percentFull;

} ammodata_t;

#define AMMO_ARRAY_SIZE 10

class rvTarget_AmmoStash : public idTarget {
public:
	CLASS_PROTOTYPE( rvTarget_AmmoStash );

	//used to detect which weapons need ammo. The values stored are 0 to 1, with -1 meaning don't check this out.
	ammodata_t			AmmoArray[ AMMO_ARRAY_SIZE];

private:
	void				Event_Activate( idEntity *activator );
};
#endif /* !__GAME_TARGET_H__ */
