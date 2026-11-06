/*
================

AAS_Find.h

================
*/

#ifndef __AAS_FIND__
#define __AAS_FIND__

class idAI;
class rvAIHelper;

/*
===============================================================================
								rvAASFindHide
===============================================================================
*/

class rvAASFindGoalForHide : public idAASCallback {
public:
	rvAASFindGoalForHide	( const idVec3 &hideFromPos );
	~rvAASFindGoalForHide	( void );

protected:

	virtual bool		TestArea	( class idAAS *aas, int areaNum, const aasArea_t& area );

private:

	pvsHandle_t			hidePVS;
	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
};

/*
===============================================================================
								rvAASFindAreaOutOfRange
===============================================================================
*/

class rvAASFindGoalOutOfRange : public idAASCallback {
public:
	
	rvAASFindGoalOutOfRange ( idAI* _owner );

protected:

	virtual bool		TestPoint		( class idAAS *aas, const idVec3& point, const float zAllow=0.0f );

private:

	idAI*			owner;
};

/*
===============================================================================
								rvAASFindAttackPosition
===============================================================================
*/

class rvAASFindGoalForAttack : public idAASCallback {
public:
	rvAASFindGoalForAttack		( idAI *self );
	~rvAASFindGoalForAttack	( void );


	bool				TestCachedGoals	( int count, aasGoal_t& goal );

	virtual void		Init			( void );
	virtual void		Finish			( void );
	
private:

	virtual bool		TestArea		( class idAAS *aas, int areaNum, const aasArea_t& area );
	virtual bool		TestPoint		( class idAAS *aas, const idVec3& point, const float zAllow=0.0f );
	
	bool				TestCachedGoal	( int index );

	idAI*				owner;
	
  	pvsHandle_t			targetPVS;
  	int					PVSAreas[ idEntity::MAX_PVS_AREAS ];
	
	idList<aasGoal_t>	cachedGoals;
	int					cachedIndex;
	int					cachedAreaNum;
};

/*
===============================================================================
							rvAASFindGoalForTether
===============================================================================
*/

class rvAASFindGoalForTether : public idAASCallback {
public:
	rvAASFindGoalForTether	( idAI* owner, rvAITether* helper );
	~rvAASFindGoalForTether	( void );

protected:

	virtual bool		TestArea	( class idAAS *aas, int areaNum, const aasArea_t& area );
	virtual bool		TestPoint	( class idAAS* aas, const idVec3& pos, const float zAllow=0.0f );

private:

	idAI*			owner;
	rvAITether*		tether;
};

#endif // __AAS_FIND__
