// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ANIMATEDENTITY_H__
#define __GAME_ANIMATEDENTITY_H__

#include "Entity.h"
#include "anim/Anim.h"

class idAnimatedEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idAnimatedEntity );

							idAnimatedEntity();
							~idAnimatedEntity();

	virtual void			Think( void );

	void					UpdateAnimation( void );

	virtual idAnimator *	GetAnimator( void );
	virtual void			SetModel( const char *modelname );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset, idMat3 &axis ) const;
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset ) const;
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idMat3 &axis ) const;

	virtual void			OnModelDefCreated( void );
	virtual void			FreeModelDef( void );

	virtual void			LinkCombat( void );
	virtual void			UnLinkCombat( void );
	virtual void			DisableCombat( void );
	virtual void			EnableCombat( void );
	virtual void			SetCombatModel( void );
	idClipModel *			GetCombatModel( void ) const;

	virtual const idBounds*	GetSelectionBounds( void ) const;
	void					SetSelectionCombatModel( void );
	void					FreeSelectionCombatModel( void );

	virtual void			OnUpdateVisuals( void );

protected:
	idAnimator				animator;
	idClipModel*			combatModel;
	idBounds				selectionBounds;
	int						lastServiceTime;

private:
	void 					Event_ClearAllJoints( void );
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void					Event_GetLocalJointPos( jointHandle_t jointnum );
	void					Event_GetJointAxis( jointHandle_t jointnum, int index );
	void 					Event_GetJointAngle( jointHandle_t jointnum );
	void					Event_JointToWorldTransform( jointHandle_t jointnum, const idVec3& vector );
	void					Event_SetAnimFrame( const char* anim, animChannel_t channel, float frame );
	void					Event_PlayAnim( animChannel_t channel, const char *animname );
	void					Event_PlayCycle( animChannel_t channel, const char *animname );
	void					Event_PlayAnimBlended( animChannel_t channel, const char *animname, float blendTime );
	void					Event_GetAnimatingOnChannel( animChannel_t channel );
	void					Event_IsAnimating( void );
	void					Event_IsAnimatingOnChannel( animChannel_t channel );
	void					Event_WorldToModelSpace( const idVec3& vector );
	void					Event_AnimLength( const char* animName );
	void					Event_AnimName( int index );
	void					Event_HideSurface( int index );
	void					Event_ShowSurface( int index );
	void					Event_GetSurfaceId( const char* surfaceName );
	void					Event_IsSurfaceHidden( int index );
	void					Event_GetNumFrames( const char* animName );
};

#endif // __GAME_ANIMATEDENTITY_H__

