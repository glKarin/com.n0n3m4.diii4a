#include "Misc.h"
#include "Target.h"

class idVacuumSpline : public idEntity
{
public:
	CLASS_PROTOTYPE(idVacuumSpline);

							idVacuumSpline();
							~idVacuumSpline();
	void					Spawn();
	void					Save(idSaveGame *savefile) const; // blendo eric: savegame pass 1
	void					Restore(idRestoreGame *savefile);

	virtual void			Think(void);


private:
	

	enum                    { VS_VACUUMING, VS_DONE };
	int						state;
	
	idEntityPtr<idEntity>	targetActor;

	idCurve_Spline<idVec3> *spline = nullptr;

	idEntity				*mover = nullptr;

	int						startTime;
	idEntity				*splineEnt = nullptr;

	idVec3					flingDirection;

	void					Detach();

	bool					is_player;
};
