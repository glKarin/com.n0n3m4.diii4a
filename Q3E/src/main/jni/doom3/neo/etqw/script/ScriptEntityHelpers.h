// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_SCRIPTENTITYHELPER_H__
#define __GAME_SCRIPTENTITYHELPER_H__

class sdScriptEntity;
class idAnimatedEntity;
class sdMotorSound;

class sdPlayerArmIK {
public:
							sdPlayerArmIK( void );

	bool					Init( idEntity* target, const idDict& parms );
	void					Update( idPlayer* player, idEntity* target );

	void					ClearJointMods( idPlayer* player );

private:
	static const int		NUM_ARMS = 2;

	struct armTarget_t {
		jointHandle_t		joint;
	};

	armTarget_t				armTargets[ NUM_ARMS ];
};

class sdScriptedEntityHelper {
public:
	typedef sdFactory< sdScriptedEntityHelper > factoryType_t;
	
												sdScriptedEntityHelper( void ) : _owner( NULL ) { _node.SetOwner( this ); }
	virtual										~sdScriptedEntityHelper( void ) { }

	idLinkList< sdScriptedEntityHelper >&		GetNode( void ) { return _node; }

	virtual void								Update( bool postThink ) = 0;
	virtual void								Init( sdScriptEntity* owner, const sdDeclStringMap* map );
	virtual bool								WantsToThink( void ) = 0;
	virtual void								SetIKTarget( idPlayer* player, int index ) { }

	static sdScriptedEntityHelper*				AllocHelper( const char* type );
	static void									Startup( void );
	static void									Shutdown( void );

protected:
	idLinkList< sdScriptedEntityHelper >		_node;
	sdScriptEntity*								_owner;

	static factoryType_t						helperFactory;
};

class sdScriptedEntityHelper_LegIk : public sdScriptedEntityHelper {
public:
												sdScriptedEntityHelper_LegIk( void ) { }
	virtual										~sdScriptedEntityHelper_LegIk( void );

	virtual void								Update( bool postThink );
	virtual void								Init( sdScriptEntity* owner, const sdDeclStringMap* map );
	virtual bool								WantsToThink( void ) { return true; }

	void										ClearJointMods( void );

private:
	jointHandle_t								upperLegJoint;
	jointHandle_t								middleLegJoint;
	jointHandle_t								lowerLegJoint;

	float										upperLength;
	float										lowerLength;

	idMat3										midToUpperJoint;
	idMat3										lowerToMidJoint;

	float										groundOffset;

	float										blendRate;

	float										currentGroundOffset;

	float										maxUpTrace;
	float										maxDownTrace;

	int											lifetime;
	int											startTime;

	idVec3										upDir;
};


class sdScriptedEntityHelper_PlayerIK : public sdScriptedEntityHelper {
public:
												sdScriptedEntityHelper_PlayerIK( void ) { playerIndex = -1; }
	virtual										~sdScriptedEntityHelper_PlayerIK( void ) { }

	virtual void								Update( bool postThink );
	virtual void								Init( sdScriptEntity* owner, const sdDeclStringMap* map );
	virtual bool								WantsToThink( void );
	virtual void								SetIKTarget( idPlayer* player, int index ) { if ( playerIndex == index ) { target = player; } }

private:
	int											playerIndex;
	sdPlayerArmIK								ik;
	idEntityPtr< idPlayer >						target;
};






// Gordon: this isn't a normal one!
class sdScriptedEntityHelper_Aimer {
private:
	struct angleInfo_t {
		angleClamp_t	clamp;

		float			current;
		float			ideal;
		float			old;

		float			arcLength;
		float			offsetAngle;
		float			initialAngle;

		sdMotorSound*	sound;
	};

	enum {
		AIMER_JOINT_YAW,
		AIMER_JOINT_PITCH,
		AIMER_JOINT_BARREL,
		AIMER_JOINT_SHOULDER,
		AIMER_NUM_JOINTS,
	};

	enum {
		AIMER_IK_BASE,
		AIMER_IK_YAW,
		AIMER_IK_PITCH,
		AIMER_IK_SHOULDER,
		AIMER_IK_NUM_PATHS,
	};

public:
												sdScriptedEntityHelper_Aimer( void ) { }

	void										Update( bool force = false );
	void										Init( bool fixupBarrel, bool invertPitch, sdScriptEntity* owner, int anim, jointHandle_t yawJoint, jointHandle_t pitchJoint, jointHandle_t barrelJoint, jointHandle_t shoulderJoint, const angleClamp_t& _yawInfo, const angleClamp_t& _pitchInfo );

	void										InitAngleInfo( const char* name, angleInfo_t& info, const sdDeclStringMap* map );
	bool										UpdateAngles( angleInfo_t& info, bool force );

	bool										CalcAngle( float targetDistance, float arcLength, float angle, float& out );

	void										ClearTarget( void );
	void										SetTarget( const idVec3& target );
	void										LockTarget( void );
	bool										TargetClose( void ) const;

	bool										CanAimTo( const idAngles& angles ) const;

private:
	angleInfo_t									yawInfo;
	angleInfo_t									pitchInfo;

	jointHandle_t								gunJoints[ AIMER_NUM_JOINTS ];
	idVec3										ikPaths[ AIMER_IK_NUM_PATHS ];
	idMat3										baseAxes[ AIMER_NUM_JOINTS ];
	
	idMat3										yawTranspose;
	idMat3										pitchTranspose;

	sdScriptEntity*								_owner;
};

#endif // __GAME_SCRIPTENTITYHELPER_H__
