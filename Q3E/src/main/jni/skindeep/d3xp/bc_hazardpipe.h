#include "Misc.h"
#include "Entity.h"

const int PIPETYPE_ELECTRICAL = 0;
const int PIPETYPE_FIRE = 1;

class idHazardPipe : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idHazardPipe);

	void					Spawn(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual void			AddDamageEffect(const trace_t &collision, const idVec3 &velocity, const char *damageDefName);

	virtual void			Think( void );
	//virtual void			Present(void);

	//int						state;

	//float					baseAngle;
	void					SetControlbox(idEntity *ent);

	void					ResetPipe();

private:

	virtual void			Event_PostSpawn(void);

	//idStr					fxActivate;

	int						pipetype;

	idEntityPtr<idEntity>	controlBox;

	bool					pipeActive;

	void					SetGlowLights(bool value);

	bool					sabotageEnabled;


	enum					{CD_IDLE, CD_PRECOOLDOWN, CD_COOLINGDOWN};
	int						cooldownState;
	int						cooldownTimer;

};

