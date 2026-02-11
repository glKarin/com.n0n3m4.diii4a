#include "Misc.h"
#include "Entity.h"
//#include "Mover.h"
//#include "Item.h"
//#include "Light.h"


class idZena : public idAnimatedEntity
{
public:
	CLASS_PROTOTYPE(idZena);

							idZena(void);
	virtual					~idZena(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	//virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);	

	virtual void			Think(void);

private:
	
	enum					{ZENA_IDLE};
	int						state;

	enum					{ ANIM_IDLE, ANIM_FORWARD, ANIM_BACKWARD, ANIM_STRAFELEFT, ANIM_STRAFERIGHT };
	int						currentAnim;

	void					SetZenaAnim(int animType);

	void					UpdateFootstepParticles();
	int						footstepparticleTimer;
	


};
//#pragma once