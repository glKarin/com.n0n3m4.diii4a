
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "AI.h"

class rvMonsterFailedTransfer : public idAI {
public:

	CLASS_PROTOTYPE( rvMonsterFailedTransfer );

	rvMonsterFailedTransfer ( void );

	void				Spawn			( void );
	void				Killed			( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void				Save			( idSaveGame *savefile ) const;
	void				Restore			( idRestoreGame *savefile );

protected:

	bool				allowSplit;

	virtual void		OnDeath			( void );

private:

	CLASS_STATES_PROTOTYPE ( rvMonsterFailedTransfer );
};

CLASS_DECLARATION( idAI, rvMonsterFailedTransfer )
END_CLASS

/*
================
rvMonsterFailedTransfer::rvMonsterFailedTransfer
================
*/
rvMonsterFailedTransfer::rvMonsterFailedTransfer ( ) {
	allowSplit = false;
}

/*
================
rvMonsterFailedTransfer::Spawn
================
*/
void rvMonsterFailedTransfer::Spawn ( void ) {
	LoadAF ( "ragdoll_legs", true );
	LoadAF ( NULL, true );
}

/*
================
rvMonsterFailedTransfer::Save
================
*/
void rvMonsterFailedTransfer::Save( idSaveGame *savefile ) const {
	savefile->WriteBool ( allowSplit );
}

/*
================
rvMonsterFailedTransfer::Restore
================
*/
void rvMonsterFailedTransfer::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool ( allowSplit );
}

/*
================
rvMonsterFailedTransfer::OnDeath
================
*/
void rvMonsterFailedTransfer::OnDeath ( void ) {
	idAI::OnDeath ( );
	
	if ( allowSplit ) {
		idEntity* torso;
		idDict	  args;

		LoadAF ( "ragdoll_legs", true );

		PlayEffect ( "fx_bloodyburst", animator.GetJointHandle ( "chest" ) );	
		SetSkin ( declManager->FindSkin	 ( spawnArgs.GetString ( "skin_legs" ) ) );

		args.Copy ( *gameLocal.FindEntityDefDict ( "monster_failed_transfer_torso" ) );
		args.SetVector ( "origin", GetPhysics()->GetOrigin() + GetPhysics()->GetGravityNormal() * -50.0f );
		args.SetInt ( "angle", move.current_yaw );
		gameLocal.SpawnEntityDef ( args, &torso );
		torso->fl.takedamage = false;
		PostEventMS( &AI_TakeDamage, 100, 1.0f );
	}
}

/*
================
rvMonsterFailedTransfer::Killed
================
*/
void rvMonsterFailedTransfer::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( !idStr::Icmp ( GetDamageGroup( location ), "legs" ) && damage < 999 ) {
		allowSplit = true;
	} else {
		allowSplit = false;
	}

	idAI::Killed ( inflictor, attacker, damage, dir, location );
}

/*
===============================================================================

	States 

===============================================================================
*/

CLASS_STATES_DECLARATION ( rvMonsterFailedTransfer )
END_CLASS_STATES

