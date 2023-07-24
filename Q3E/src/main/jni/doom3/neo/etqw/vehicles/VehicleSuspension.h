// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_VEHICLESUSPENSION_H__
#define __GAME_VEHICLES_VEHICLESUSPENSION_H__

class sdTransport;

class sdVehicleSuspensionInterface {
public:
	virtual sdTransport*					GetParent( void ) const = 0;
	virtual float							GetOffset( void ) const = 0;
	virtual jointHandle_t					GetJoint( void ) const = 0;
};

class sdVehicleSuspension {
public:
	typedef sdFactory< sdVehicleSuspension > factoryType_t;

	virtual									~sdVehicleSuspension( void ) { }

	virtual const char*						GetTypeName() const = 0;

	virtual void							SetSuspensionInterface( sdVehicleSuspensionInterface* object ) = 0;

	virtual	bool							Init( sdVehicleSuspensionInterface* object, const idDict& info ) = 0;

	virtual	void							Update( void ) = 0;
	virtual void							ClearIKJoints( idAnimator* animator ) = 0;
	virtual bool							UpdateIKJoints( idAnimator* animator ) = 0;

	static void								Startup( void );
	static void								Shutdown( void );
	static sdVehicleSuspension*				GetSuspension( const char* name );

private:
	static factoryType_t					suspensionFactory;
};

class sdVehicleSuspension_Pivot : public sdVehicleSuspension {
public:
											sdVehicleSuspension_Pivot( void );
	virtual									~sdVehicleSuspension_Pivot( void );

	virtual const char*						GetTypeName() const { return TypeName(); }
	static const char*						TypeName() { return "pivot"; }

	virtual void							SetSuspensionInterface( sdVehicleSuspensionInterface* object ) { _object = object; }

	virtual	bool							Init( sdVehicleSuspensionInterface* object, const idDict& info );

	virtual	void							Update( void ) { ; }
	virtual void							ClearIKJoints( idAnimator* animator );
	virtual bool							UpdateIKJoints( idAnimator* animator );

protected:
	sdVehicleSuspensionInterface*			_object;
	idMat3									_suspensionAxis;
	jointHandle_t							_suspensionJoint;
	float									_suspensionRadius;
	float									_suspensionAngle;
	bool									_reverse;
	float									_initialOffset;
	float									_lastOffset;
	float									_idealSuspensionAngle;
	float									_oldSuspensionAngle;
};


class sdVehicleSuspension_DoubleWishbone : public sdVehicleSuspension {
public:
											sdVehicleSuspension_DoubleWishbone( void );
	virtual									~sdVehicleSuspension_DoubleWishbone( void );

	virtual const char*						GetTypeName() const { return TypeName(); }
	static const char*						TypeName() { return "double_wishbone"; }

	virtual void							SetSuspensionInterface( sdVehicleSuspensionInterface* object ) { _object = object; }

	virtual	bool							Init( sdVehicleSuspensionInterface* object, const idDict& info );

	virtual	void							Update( void ) { ; }
	virtual void							ClearIKJoints( idAnimator* animator );
	virtual bool							UpdateIKJoints( idAnimator* animator );

protected:
	sdVehicleSuspensionInterface*			_object;
	idMat3									_upperSuspensionAxes;
	jointHandle_t							_upperSuspensionJoint;
	idMat3									_lowerSuspensionAxes;
	jointHandle_t							_lowerSuspensionJoint;
	float									_suspensionRadius;
	float									_suspensionAngle;
	float									_initialOffset;
	float									_currentSuspensionAngle;
	float									_oldSuspensionAngle;
	float									_lastOffset;
	bool									_rightWheel;
	float									_lerpScale;
};

class sdVehicleSuspension_Vertical : public sdVehicleSuspension {
public:
											sdVehicleSuspension_Vertical( void );
	virtual									~sdVehicleSuspension_Vertical( void );

	virtual const char*						GetTypeName() const { return TypeName(); }
	static const char*						TypeName() { return "vertical"; }

	virtual void							SetSuspensionInterface( sdVehicleSuspensionInterface* object ) { _object = object; }

	virtual	bool							Init( sdVehicleSuspensionInterface* object, const idDict& info );
	bool									Init( sdVehicleSuspensionInterface* object, const char* jointName );

	virtual	void							Update( void ) { ; }
	virtual void							ClearIKJoints( idAnimator* animator );
	virtual bool							UpdateIKJoints( idAnimator* animator );

protected:
	sdVehicleSuspensionInterface*			_object;
	jointHandle_t							_suspensionJoint;
	float									_oldOffset;
	float									_offset;
};


class sdVehicleSuspension_2JointLeg : public sdVehicleSuspension {
public:
											sdVehicleSuspension_2JointLeg( void );
	virtual									~sdVehicleSuspension_2JointLeg( void ) { }

	virtual const char*						GetTypeName() const { return TypeName(); }
	static const char*						TypeName() { return "2jointleg"; }

	virtual void							SetSuspensionInterface( sdVehicleSuspensionInterface* object ) { _object = object; }

	virtual	bool							Init( sdVehicleSuspensionInterface* object, const idDict& info );

	virtual	void							Update( void ) { ; }
	virtual void							ClearIKJoints( idAnimator* animator );
	virtual bool							UpdateIKJoints( idAnimator* animator );

protected:
	sdVehicleSuspensionInterface*			_object;
	jointHandle_t							_kneeJoint;
	jointHandle_t							_hipJoint;
	float									_lerpScale;
	bool									_reverse;

	idVec3									_offset;

	idVec3									_hipForward;
	idVec3									_kneeForward;

	float									_upperLegLength;
	idMat3									_upperLegToHipJoint;

	float									_lowerLegLength;
	idMat3									_lowerLegToKneeJoint;

	float									_lastOffset;
};

#endif // __GAME_VEHICLES_VEHICLESUSPENSION_H__
