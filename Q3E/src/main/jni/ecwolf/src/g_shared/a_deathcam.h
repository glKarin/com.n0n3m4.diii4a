#ifndef __A_DEATHCAM__
#define __A_DEATHCAM__

#include "actor.h"

class ADeathCam : public AActor
{
	DECLARE_NATIVE_CLASS(DeathCam, Actor)
	HAS_OBJECT_POINTERS
	public:
		void	SetupDeathCam(AActor *actor, AActor *killer);
		void	Tick();

		TObjPtr<AActor>	actor;
		TObjPtr<AActor>	killer;
		enum
		{
			CAM_STARTED,
			CAM_ACTIVE,
			CAM_FINISHED
		} camState;
};

#endif
