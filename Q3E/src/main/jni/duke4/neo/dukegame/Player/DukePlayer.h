// DukePlayer.h
//

enum dnWeapons {
	DN_WEAPON_FEET = 0,
	DN_WEAPON_PISTOL,
	DN_WEAPON_SHOTGUN
};

//
// DnPlayer
//
class DukePlayer : public idPlayer {
public:
	CLASS_PROTOTYPE(DukePlayer);

	DukePlayer();

	void					Spawn(void);
	void					Think(void);

	virtual void			UpdateHudStats(idUserInterface* hud);
	virtual void			SetStartingInventory(void);

	void					Event_DukeTalk(const char* soundName, bool force = false);

	void					Event_PlayDukeJumpSound(void);
	void					Event_PlayRandomDukeTaunt(void);

	void					GiveEgo(int amount);

	virtual void			BobCycle(const idVec3& pushVelocity) override;

	virtual idVec3			GetVisualOffset();
	virtual idBounds		GetClipBounds();

	virtual	void			Damage(idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location);
private:
	void					SetAnimation(const char* name, bool loop);
	idVec3					ApplyLandDeflect(const idVec3& pos, float scale);

	const idSoundShader*	dukeTauntShader;
	const idSoundShader*	dukePainShader;

	idList<const idSoundShader*> dukeJumpSounds;

	bool					firstSwearTaunt;

	float					bob;
	float					lastAppliedBobCycle;
	idStr					currentAnimation;

	rvmDeclRenderParam*		guiCrosshairColorParam;
};