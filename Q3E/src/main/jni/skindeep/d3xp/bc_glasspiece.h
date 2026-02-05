#include "Misc.h"
#include "Entity.h"

#include "Item.h"

#define CLEANUP_RADIUS 32 //when glass is stepped on, remove all other glass pieces in XX radius.

const int GLASS_PIECES_ACTIVE_MAX = 64;
const int GLASS_PIECES_DESTROY_MAX = 64; //  per frame destroy
const int GLASS_PIECES_SAMPLE_SIZE = 10; // should be big enough sample to compare pieces, but smaller than max

class idGlassPiece : public idMoveableItem
{
public:
	CLASS_PROTOTYPE(idGlassPiece);

	virtual					~idGlassPiece();
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	void					Create(const idVec3 &start, const idMat3 &axis);

	void					Event_Touch(idEntity *other, trace_t *trace);

	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);
	virtual bool			Collide(const trace_t &collision, const idVec3 &velocity);

	virtual void			Think(void);
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);

	void					ShatterAndRemove(bool fastCleanUpOnly = false);

	bool					IsDoingInitialSpawnFall();

	static void				LimitActiveGlassPieces();

private:

	int						nextTouchTime;	

	bool					hasSettledDown;
	int						spawnTimer;

	void					RemoveNearbyGlassPieces();

	int						shineTimer;

	void					UpdateShine();

	int						initialSpawnFallTimer;
	bool					spawnfallDone;

	float					modelRadius;

	static idList<idGlassPiece*> glassList;

};
//#pragma once