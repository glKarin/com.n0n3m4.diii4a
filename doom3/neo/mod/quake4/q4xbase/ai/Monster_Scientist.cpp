
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterScientist : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterScientist );

	rvMonsterScientist ( void );
	
	void				Spawn				( void );
	
#ifdef _Q4XBASE //karin: add killer entity parm
	virtual void		OnDeath				( idEntity *killer );
#else
	virtual void		OnDeath				( void );
#endif
	
	// Add some dynamic externals for debugging
	virtual void		GetDebugInfo		( debugInfoProc_t proc, void* userData );

private:

	CLASS_STATES_PROTOTYPE ( rvMonsterScientist );
};

CLASS_DECLARATION( idAI, rvMonsterScientist )
END_CLASS

/*
================
rvMonsterScientist::rvMonsterScientist
================
*/
rvMonsterScientist::rvMonsterScientist ( void ) {
}

/*
================
rvMonsterScientist::Spawn
================
*/
void rvMonsterScientist::Spawn ( void ) {
	PlayEffect ( "fx_fly", animator.GetJointHandle ( "effects_bone" ), true );
} 

/*
================
rvMonsterScientist::OnDeath
================
*/
#ifdef _Q4XBASE //karin: add killer entity parm
void rvMonsterScientist::OnDeath ( idEntity *killer ) 
#else
void rvMonsterScientist::OnDeath ( void ) 
#endif
{
	StopEffect ( "fx_fly" );
	
#ifdef _Q4XBASE //karin: add killer entity parm
	idAI::OnDeath ( killer );
#else
	idAI::OnDeath ( );
#endif
}

/*
================
rvMonsterScientist::GetDebugInfo
================
*/
void rvMonsterScientist::GetDebugInfo	( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAI::GetDebugInfo ( proc, userData );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterScientist )
END_CLASS_STATES
