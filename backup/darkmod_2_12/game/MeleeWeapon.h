/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __MELEEWEAPON_H__
#define __MELEEWEAPON_H__

#include "Entity.h"
#include "Actor.h"

class idPlayer;

/**
* Class for melee weapons (and shields) that are usually attached to an AI.
* Handles activating and deactivating the clipmodels for attacks and parries.
* Handles setting correct contents on the clipmodels.
* Handles collisions between weapons and actors, between weapons and parries, 
* between weapons and world, and between weapons and the item that
* the player is currently holding in the Grabber.
**/

class CMeleeWeapon : public idMoveable 
{
public:
	CLASS_PROTOTYPE( CMeleeWeapon );

							CMeleeWeapon( void );
	virtual					~CMeleeWeapon( void ) override;

	/**
	* Think loop calls idMoveable::Think, then checks collisions for attacks with bound weapons
	**/
	virtual void			Think( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	/**
	* Get, set or clear the actor that owns this weapon
	**/
	idActor *GetOwner( void ) { return m_Owner.GetEntity(); };
	void SetOwner( idActor *actor ) { m_Owner = actor; };
	void ClearOwner( void ) { m_Owner = NULL; };

	/**
	* Called when we are attached to the given actor
	* Sets owner and parses our spawnargs to actor variables
	**/
	void AttachedToActor( idActor *actor );

	/**
	* Activate an attack and test it for collisions from this point on
	* ActOwner is the actor that owns this weapon, AttName the string name of the attack,
	**/
	void ActivateAttack( idActor *ActOwner, const char *AttName );
	/**
	* Disables the attack (stops testing for collisions and clears any custom clipmodel)
	**/
	void DeactivateAttack( void );

	/**
	* Set up the clipmodel, clipmask and stims for a parry
	* ActOwner is the actor that owns this weapon, ParryName the string name of the parry
	**/
	void ActivateParry( idActor *ActOwner, const char *ParryName );

	/**
	* Disables the parry clipmodel and clears vars
	**/
	void DeactivateParry( void );

protected:
	/**
	* Clears the extra melee clipmodel if it exists,
	* resets member vars
	**/
	void ClearClipModel( void );

	/**
	* Test to see if a melee attack in progress collides with
	* anything that should stop it.
	**/
	void CheckAttack( idVec3 OldOrigin, idMat3 OldAxis ); 

	/**
	* Parry success test, called when a melee attack hits a melee parry
	* Also handles deactivating/bouncing the attack if parry succeeded
	* 
	* POTENTIAL PROBLEM: What happens for fake cases like thrust parry on slash attack, how does
	* it let an attack thru if it fails, without always returning itself in the trace?
	* One solution would be to temporarily disable the parry for a moment, but what if the thrust
	* attack was not going to hit distance-wise, and another slash attack came in a little later?
	* Would modeling just the very tip of a thrust attack help?
	*
	* Maybe a better solution is to modify the attack in progress so that
	* it no longer collides with parries once it "gets past" an invalid parry
	**/
	void TestParry( CMeleeWeapon *other, idVec3 dir, trace_t *trace );

	/**
	* Check to see what AI are in range, and whether we should modify their CMs
	* (We do this if there is a flag on the weapon, if the AI is conscious, and not at combat alert)
	**/
	void CheckAICMSwaps( void );

	/**
	* Handle valid collision, called when trace hit something valid
	* in either StarAttack or CheckAttack
	* Arguments:
	* tr: Trace object that should be the result of a trace that was found to have hit something
	* OldOrigin: Used for debug display of trace lines, can be origin of CM in previous frame,
	* or just the start point of the trace.
	* PointVelDir: Normalized velocity direction vector of the collision point.
	* Used for debug display and imparting velocity to objects hit
	**/
	void HandleValidCollision( trace_t &tr, idVec3 OldOrigin, idVec3 PointVelDir );

	/**
	* Called to handle physics, particle FX, sound, damage
	* When a melee attack hits something
	* Location is the JOINT HANDLE that was hit
	* If we directly hit an AF, not necessarily equal to  CLIPMODEL_ID_FOR_JOINT_HANDLE(tr.c.id)
	**/
	void MeleeCollision( idEntity *other, idVec3 dir, trace_t *trace, int location );

	/**
	* Set up the clipmodel for the current parry/attack action
	* Expects m_ActionName, m_bAttacking and m_bParrying to be already set.
	**/
	void SetupClipModel( void );

	/**
	* Return the type of attack or parry.  Used to test whether a parry will
	* work against a given attack.
	**/
	EMeleeType GetMeleeType( void ) { return m_MeleeType; };

protected:
	/**
	* Actor that owns this melee weapon.  Most likely also the bindmaster.
	**/
	idEntityPtr<idActor>	m_Owner;

	/**
	* Modified clipmodel for the weapon
	* Can optionally be used when either parrying or attacking
	* If we use the actual weapon clipmodel, this is set to NULL
	**/
	idClipModel				*m_WeapClip;
	/**
	* Translational offset and rotation of the melee clipmodel
	* relative to this moveable entity's origin and orientation
	**/
	idVec3					m_ClipOffset;
	idMat3					m_ClipRotation;
	/**
	* In some special cases we only want yaw rotation applied to the weapon CM,
	* not pitch and roll (e.g., player thrust parry)
	* The boolean sets this and the angle supplis what pitch angle relative to world (NYI)
	**/
	bool					m_bClipYawOnly;
	float					m_ClipPitchAngle; // NYI!

	/**
	* Whether we are actively parrying or attacking
	**/
	bool					m_bAttacking;
	bool					m_bParrying;

	/**
	* Name of the attack or parry (e.g., "slash_lr")
	**/
	idStr					m_ActionName;

	/**
	* Whether this attack is set to collide with world/solids or not
	* (Typically player attacks collide with solids, AI's only collide with melee parries)
	**/
	bool					m_bWorldCollide;

	/**
	* Type of melee action for parry tests
	* Slash, overhand, thrust, or unblockable
	**/
	EMeleeType				m_MeleeType;

	/**
	* Origin and axis from the previous frame
	**/
	idVec3					m_OldOrigin;
	idMat3					m_OldAxis;

	/**
	* Whether the parry stops being effective after a successful parry
	* (Applies to player weapon parries)
	**/
	bool					m_bParryStopOnSuccess;

	/**
	* Set to true if we want to execute a failsafe trace for certain attacks
	* This happens after the attack CM starts out colliding with something
	* as soon as it is instanced.  Try do do another ray-trace to fix this.
	**/
	bool					m_bFailsafeTrace;
	/**
	* Start of the failsafe trace, in the coordinates of the attack CM
	**/
	idVec3					m_vFailsafeTraceStart;
	/**
	* End of the failsafe trace, in the coordinates of the attack CM
	**/
	idVec3					m_vFailsafeTraceEnd;

// Cosmetic stuff:

	/**
	* Keep swinging when we hit moveables at or below this mass
	**/
	float					m_StopMass;

	/**
	* Number of particles we've generated in this attack
	**/
	int						m_ParticlesMade;

	/**
	* Whether this attack swaps in different CMs on the AI in range
	* (for example, the blackjack swapping in easy-KO CMs)
	**/
	bool					m_bModAICMs;

	/**
	* List of AI whose CMs we've changed
	**/
	idList< idEntityPtr<idAI> >	m_AIWithModCMs;

};

#endif /* !__MELEEWEAPON_H__ */
