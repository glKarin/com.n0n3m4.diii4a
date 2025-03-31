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

#ifndef __GAME_TARGET_H__
#define __GAME_TARGET_H__


/*
===============================================================================

idTarget

===============================================================================
*/

class idTarget : public idEntity {
public:
	CLASS_PROTOTYPE( idTarget );
};


/*
===============================================================================

idTarget_Remove

===============================================================================
*/

class idTarget_Remove : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Remove );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Show

===============================================================================
*/

class idTarget_Show : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Show );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Damage

===============================================================================
*/

class idTarget_Damage : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Damage );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SessionCommand

===============================================================================
*/

class idTarget_SessionCommand : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SessionCommand );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_EndLevel

===============================================================================
*/

class idTarget_EndLevel : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_EndLevel );

private:
	void				Event_Activate( idEntity *activator );

};


/*
===============================================================================

idTarget_WaitForButton

===============================================================================
*/

class idTarget_WaitForButton : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_WaitForButton );

	virtual void		Think( void ) override;

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_SetGlobalShaderTime

===============================================================================
*/

class idTarget_SetGlobalShaderTime : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetGlobalShaderTime );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetShaderParm

===============================================================================
*/

class idTarget_SetShaderParm : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetShaderParm );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetShaderTime

===============================================================================
*/

class idTarget_SetShaderTime : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetShaderTime );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_FadeEntity

===============================================================================
*/

class idTarget_FadeEntity : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_FadeEntity );

						idTarget_FadeEntity( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void ) override;

private:
	idVec4				fadeFrom;
	int					fadeStart;
	int					fadeEnd;

	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_LightFadeIn

===============================================================================
*/

class idTarget_LightFadeIn : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LightFadeIn );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_LightFadeOut

===============================================================================
*/

class idTarget_LightFadeOut : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_LightFadeOut );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

idTarget_Give

===============================================================================
*/

class idTarget_Give : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Give );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetModel

===============================================================================
*/

class idTarget_SetModel : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetModel );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetInfluence

===============================================================================
*/

class idTarget_SetInfluence : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetInfluence );

						idTarget_SetInfluence( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_RestoreInfluence();
	void				Event_GatherEntities();
	void				Event_Flash( float flash, int out );
	void				Event_ClearFlash( float flash );
	virtual void		Think( void ) override;

	idList<int>			lightList;
	idList<int>			guiList;
	idList<int>			soundList;
	idList<int>			genericList;
	float				flashIn;
	float				flashOut;
	float				delay;
	idStr				flashInSound;
	idStr				flashOutSound;
	idEntity *			switchToCamera;
	idInterpolate<float>fovSetting;
	bool				soundFaded;
	bool				restoreOnTrigger;
};


/*
===============================================================================

idTarget_SetKeyVal

===============================================================================
*/

class idTarget_SetKeyVal : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetKeyVal );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_SetFov

===============================================================================
*/

class idTarget_SetFov : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_SetFov );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void ) override;

private:
	idInterpolate<int>	fovSetting;

	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_CallObjectFunction

===============================================================================
*/

class idTarget_CallObjectFunction : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_CallObjectFunction );

private:
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

Tels: idTarget_PostScriptEvent

===============================================================================
*/

class idTarget_PostScriptEvent : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_PostScriptEvent );

private:
	void	Event_Activate( idEntity *activator );
	void	TryPostOrCall( idEntity *ent, idEntity *activator, const idEventDef *ev, const char* funcName, const bool pass_self, const bool pass_activator, const float delay);
};

/*
===============================================================================

idTarget_LockDoor

===============================================================================
*/

class idTarget_EnableLevelWeapons : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_EnableLevelWeapons );

private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_Tip

===============================================================================
*/

class idTarget_Tip : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_Tip );

						idTarget_Tip( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	idVec3				playerPos;

	void				Event_Activate( idEntity *activator );
	void				Event_TipOff( void );
	void				Event_GetPlayerPos( void );
};

/*
===============================================================================

idTarget_RemoveWeapons

===============================================================================
*/
class idTarget_RemoveWeapons : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_RemoveWeapons );
private:
	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idTarget_FadeSoundClass

===============================================================================
*/
class idTarget_FadeSoundClass : public idTarget {
public:
	CLASS_PROTOTYPE( idTarget_FadeSoundClass );
private:
	void				Event_Activate( idEntity *activator );
	void				Event_RestoreVolume();
};

/**
* CTarget_AddObjectives
* Helper entity for the objectives system.  When triggered, adds the
* objectives on it into the objectives system.
*
* It sets the spawnargs "obj_num_offset" on itself for the numerical offset
* of the first objective it added, used for later addressing that objective.
**/
class CTarget_AddObjectives : public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_AddObjectives );
private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
};

/**
 * greebo: Target for altering the state of certain objectives.
 */
class CTarget_SetObjectiveState : 
	public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_SetObjectiveState );
private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
};

/**
* Target for hiding or showing certain objectives.
**/
class CTarget_SetObjectiveVisibility : 
	public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_SetObjectiveVisibility );
private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
};

/**
* Target for setting the state of an objective component
**/
class CTarget_SetObjectiveComponentState : 
	public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_SetObjectiveComponentState );
private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
};

/**
 * greebo: Target for triggerig conversations.
 */
class CTarget_StartConversation : 
	public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_StartConversation );
private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
};

/**
* CTarget_SetFrobable
* Sets all items inside frobable or not when triggered
**/
class CTarget_SetFrobable : public idTarget 
{
public:
	CLASS_PROTOTYPE( CTarget_SetFrobable );

						CTarget_SetFrobable( void );
	
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	void				Event_Activate( idEntity *activator );
	virtual void		Spawn( void );
private:
	/**
	* Current frob state, whether stuff inside has been set frobable or not
	**/
	bool				m_bCurFrobState;

	/**
	* List of the names of entities previously set unfrobable
	* This list is maintained to avoid accidentally setting anything frobable
	* that was not frobable before it went in the brush
	* Only ents that get added to this list will become frobable.
	**/
	idStrList			m_EntsSetUnfrobable;
};

/**
 * greebo: This target calls a specific script function (with no arguments).
 */
class CTarget_CallScriptFunction : 
	public idTarget
{
public:
	CLASS_PROTOTYPE( CTarget_CallScriptFunction );

private:
	void				Event_Activate( idEntity *activator );
};

/**
 * greebo: This target locks or unlocks a specific frobmover.
 */
class CTarget_ChangeLockState : 
	public idTarget
{
public:
	CLASS_PROTOTYPE( CTarget_ChangeLockState );

private:
	void				Event_Activate( idEntity *activator );
};

/**
 * greebo: This target changes the targets of existing entities.
 */
class CTarget_ChangeTarget : 
	public idTarget
{
public:
	CLASS_PROTOTYPE( CTarget_ChangeTarget );

private:
	void				Event_Activate( idEntity *activator );
};

/**
 * greebo: This target eats incoming triggers and releases them when the next map loads.
 */
class CTarget_InterMissionTrigger : 
	public idTarget
{
public:
	CLASS_PROTOTYPE( CTarget_InterMissionTrigger );

private:
	void				Event_Activate(idEntity* activator);
};

/*
===============================================================================

Tels: set the team of its targets.

===============================================================================
*/

class CTarget_SetTeam : public idTarget {
public:
	CLASS_PROTOTYPE( CTarget_SetTeam );

private:
	void				Event_Activate( idEntity *activator );
};


/*
*	CTarget_ItemRemove
*
*	SteveL #3784: Do something about func_itemremove
*/

class CTarget_ItemRemove : public idTarget {
public:
	CLASS_PROTOTYPE( CTarget_ItemRemove );

private:
	void			Event_Activate( idEntity *activator );
	void			RespawnItem( const char* classname, const char* itemname, const int quantity, const bool ammo );
	const idVec3	RespawnPosition( const idEntity* refEnt ) { return refEnt->GetPhysics()->GetOrigin() + idVec3(0.0f, 0.0f, 40.0f) * refEnt->GetPhysics()->GetAxis(); }
};


#endif /* !__GAME_TARGET_H__ */
