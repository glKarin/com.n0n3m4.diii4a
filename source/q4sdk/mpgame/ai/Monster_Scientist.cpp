
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

class rvMonsterScientist : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterScientist );

	rvMonsterScientist ( void );
	
	void				Spawn				( void );
	
	virtual void		OnDeath				( void );
	
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
void rvMonsterScientist::OnDeath ( void ) {
	StopEffect ( "fx_fly" );
	
	idAI::OnDeath ( );
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
