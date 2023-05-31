
#ifndef __PREY_AI_NAVIGATOR_H__
#define __PREY_AI_NAVIGATOR_H__

// Forward declaration for hhAI
class hhAI;

class hhNavigator : public idNavigator {

public:

	hhNavigator(void);

	void Spawn(void);

	//virtual void		SetAlly(idActor *ally); JRM removed

	virtual void		SetOwner(idAI *owner);

	virtual boolean		IsNearDest( void );

protected:

	hhAI *				hhSelf;

	//virtual void		FollowAlly(idActor *ally); JRM removed

	/* JRM removed
	void		FollowAllyStay(idActor *ally);
	void		FollowAllyFollow(idActor *ally);
	void		FollowAllyLead(idActor *ally);
	*/

//	void (hhNavigator::* followStateFunction) (idActor *ally); JRM removed

	//idVec3		FindNewLeadPosition(idActor *ally); JRM removed

};


#endif /* __PREY_AI_NAVIGATOR_H__ */
