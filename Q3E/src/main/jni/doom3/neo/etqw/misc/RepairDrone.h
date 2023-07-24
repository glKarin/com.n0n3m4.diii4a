// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __REPAIR_DRONE_H__
#define __REPAIR_DRONE_H__

#include "../ScriptEntity.h"
#include "../effects/Effects.h"

class sdRepairDroneBroadcastData : public sdScriptEntityBroadcastData {
public:
	sdRepairDroneBroadcastData() {
		repairTarget = NULL;
		ownerEntity = NULL;
	}

	virtual void		MakeDefault( void );

	virtual void		Write( idFile* file ) const;
	virtual void		Read( idFile* file );

	idEntity*			repairTarget;
	idEntity*			ownerEntity;
};

class sdRepairDrone : public sdScriptEntity {
	CLASS_PROTOTYPE( sdRepairDrone );

	void				Spawn( void );
	virtual void		Think( void );
	virtual void		PostThink( void );

	// script events
	void				Event_MoveTo( const idVec3& newOrigin, float timeDelta );
	void				Event_SetEffectOrigins( const idVec3& start, const idVec3& end, bool active );
	void				Event_HideThrusters( void );

	void				Event_SetEntities( idEntity* repair, idEntity* owner );
	void				Event_GetRepairTarget( void );
	void				Event_GetOwnerEntity( void );

	void				Event_Disable( void );

	// networking methods
	virtual void		ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void		ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void		WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual bool		WantsToThink( void ) const { return true; }

protected:
	void				UpdateMove( void );
	void				UpdateEffects( void );
	void				UpdateWaterDamage( void );

	// data from script
	idEntityPtr< idEntity >		repairTarget;
	idEntityPtr< idEntity >		ownerEntity;

	// movement ideals
	idVec3				moveToOrigin;
	idVec3				moveFromOrigin;
	idVec3				moveVelocity;
	float				moveToTime;
	float				moveFromTime;

	// constants from decl
	idVec3				jet1Offset;
	idVec3				jet2Offset;
	idVec3				jet3Offset;
	idVec3				jet4Offset;

	float				throttleVelScale;
	float				throttleMin;
	float				throttleMax;

	float				velocityToAngle;
	float				directionRecovery;
	float				angleMax;
	float				angleToForce;

	float				maxSideVel;
	float				maxUpVel;

	// state
	float				lastThrottle;
	idVec3				effectStart;
	idVec3				effectEnd;
	bool				effectActive;

	sdEffect			repairBeamEffect;
	sdEffect			engineEffect;

	float				effectVelocityScale;

	int						lastWaterDamageTime;
	int						submergeTime;
	const sdDeclDamage*		waterDamageDecl;
};

#endif // __REPAIR_DRONE_H__
