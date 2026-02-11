//#include "Misc.h"
//#include "Entity.h"
#include "Player.h"
#include "ai/AI.h"
#include "bc_gunner.h"

class idMech : public idAI
{
public:
	CLASS_PROTOTYPE(idMech);

	
	void					Spawn(void);
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	virtual void			Think(void);
	
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	bool					mounted;

	void					Dismount(void);

	void					SetDrawBall(bool enabled); // SW: controls whether we draw the ball part of the mech model (this should be false while the player is inside)

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	bool					Player_IsReloading() const { return playerWeaponReloadFinishTime > 0.0f; }
	bool					Player_NeedsReload() const { return playerClip == 0 && playerAmmo > 0 && !Player_IsReloading(); }
	bool					Player_NeedsRack() const { return !playerChambered && !Player_IsRacking(); }
	bool					Player_IsRacking() const { return playerWeaponRackFinishTime > 0.0f; }
	int						Player_GetClip() const { return playerClip; }
	int						Player_GetAmmo() const { return playerAmmo; }
private:

	int						animState;

	enum
	{
		MECHSTATE_IDLE,
		MECHSTATE_FORWARD,
		MECHSTATE_BACKWARD,
		MECHSTATE_LEFT,
		MECHSTATE_RIGHT
	};

	int strafeState;
	enum
	{
		MECHSTRAFE_NONE,
		MECHSTRAFE_LEFT,
		MECHSTRAFE_LEFTFORWARD,
		MECHSTRAFE_LEFTBACKWARD,
		MECHSTRAFE_RIGHT,
		MECHSTRAFE_RIGHTFORWARD,
		MECHSTRAFE_RIGHTBACKWARD
	};

	enum
	{
		MECHCOUNTDOWN_NONE,
		MECHCOUNTDOWN_ACTIVE,
		MECHCOUNTDOWN_EXPIRED
	};

	int					deathCountdownState;
	int					deathTimer;

	const idDeclParticle *	fireParticles = nullptr;
	int						fireParticlesFlyTime;

	idEntity*				countdownGui = nullptr;

	idLight *				fireLight = nullptr;

	// For player firing
	float					playerWeaponLastFireTime;
	float					playerWeaponFireDelay;
	float					playerWeaponReloadDuration;
	float					playerWeaponReloadFinishTime;
	float					playerWeaponRackDuration;
	float					playerWeaponRackFinishTime;

	const idDict*			playerProjectile = nullptr;

	const idSoundShader*	playerSoundFireWeapon = nullptr;
	const idSoundShader*	playerSoundLastShot = nullptr;
	const idSoundShader*	playerSoundDryFire = nullptr;
	const idSoundShader*	playerSoundReload = nullptr;
	const idSoundShader*	playerSoundRack = nullptr;
	int						playerAmmo;
	int						playerClip;
	int						playerClipSize;
	bool					playerChambered;
};
//#pragma once