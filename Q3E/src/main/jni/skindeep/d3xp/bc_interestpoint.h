#include "Misc.h"
#include "Entity.h"

enum
{
	IPTYPE_NOISE,
	IPTYPE_VISUAL
};

class idInterestPoint : public idEntity
{
public:
	CLASS_PROTOTYPE(idInterestPoint);

							idInterestPoint(void);
	virtual					~idInterestPoint(void);

	void					Spawn(void);

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	void					SetComplete(bool investigationMode);


	int						priority;

	int						noticeRadius;			// SW: AI must be inside this radius in order to notice the interestpoint. 0 will bypass the condition check.
	int						duplicateRadius;		// Anti-chaos measure. Interestpoints will not spawn inside duplicateRadius of another interestpoint of the same type.
													// This should help with scenarios that spam a lot of interestpoints all in the same place (explosives going off, glass shattering, etc)
													// Like with noticeRadius, 0 will bypass the condition check.
	
	int						interesttype;
	int						sensoryTimer;
	bool					isClaimed;
	idEntityPtr<idAI>		claimant;				// SW: If the interestPoint is claimed, this should point to the AI currently investigating/interacting with it
	idList<idEntityPtr<idAI>> observers;				// Observers are AIs instructed to perform overwatch. There can only be one claimant, but many observers.
	bool					cleanupWhenUnobserved;	// SW: This interestpoint has ceased to be remarkable. We can destroy it, but only if there are no active investigators/observers.
	bool					forceCombat;			// Forces the AI to enter combat state.
	bool					onlyLocalPVS;			// AI has to be in same PVS as the interestpoint. Only for audio interestpoints. This is so the AI ignores things in other PVSs, i.e. skullsaver yelling.
	idEntityPtr<idEntity>	interestOwner;			// the entity associated with this interestpoint.

	void					SetClaimed(bool value, idAI* claimant = NULL);
	int						GetExpirationTime(void);

	void					AddObserver(idAI* observer);
	void					RemoveObserver(idAI* observer);
	void					ClearObservers(void);
	bool					HasObserver(idAI* observer);

	bool					breaksConfinedStealth;

	void					SetOwnerDisplayName(idStr _name);
	idStr					GetOwnerDisplayName();
	idStr					GetHUDName();

	int						arrivalDistance; //how close do we have to get in order to be considered 'arrived' at the object.

	int						GetCreationTime();

private:

	int						expirationTime;

	idStr					ownerDisplayName;

	int						creationTime;

	

};
//#pragma once