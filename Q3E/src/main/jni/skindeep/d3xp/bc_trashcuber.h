#include "Misc.h"
#include "Entity.h"
#include "Mover.h"

class idTrashcuber : public idMover
{
public:
	CLASS_PROTOTYPE(idTrashcuber);

							idTrashcuber(void);
	virtual					~idTrashcuber(void);

	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	void					Spawn(void);

	virtual void			Think(void);

private:

	idLight *				interiorLight = nullptr;
	idLight *				exteriorLights[4] = {};
	idLight *				spotlight = nullptr;

	int						stateTimer;
	int						cuberState;
	enum					{ CUBER_IDLE, CUBER_MOVING, CUBER_CHARGEDELAY, CUBER_CHARGINGUP, CUBER_SLAMMINGDOWN, CUBER_MAKINGCUBE, CUBER_RISINGUP };

	int						lastZPosition;
	void					UpdateSpotlightSize();

	
	idStr					targetAirlock;
	void					SpawnTrashcube();
	idVec3					FindValidPosition();
};
