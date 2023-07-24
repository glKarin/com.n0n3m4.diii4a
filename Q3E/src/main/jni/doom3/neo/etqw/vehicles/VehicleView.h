// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_VEHICLEVIEW_H__
#define __GAME_VEHICLES_VEHICLEVIEW_H__

class sdVehiclePosition;
class sdVehicleWeapon;
class sdVehicleView;
class idAnimatedEntity;

typedef sdFactory< sdVehicleView > sdVehicleViewFactory;

class sdVehicleView {
public:
									sdVehicleView() : freshEntry( false ) { } 
	virtual							~sdVehicleView() { } 

	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "standard"; }

	virtual void					Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode );

	virtual void					OnPlayerEntered( idPlayer* player );	// player entered the vehicle into this view
	virtual void					OnPlayerSwitched( idPlayer* player, bool newPosition );	// player toggled views into this view
	virtual void					OnTeleport( void );

	virtual void					CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) = 0;
	void							Update( sdVehicleWeapon* weapon );

	float							GetFov( void ) const;
	void							ZoomCycle( void ) const;

	virtual void					ClampViewAngles( idAngles& viewAngles, const idAngles& oldAngles ) const;
	const positionViewMode_t&		GetViewParms( void ) const { return viewMode; }

	bool							GetSensitivity( float& x, float& y );

	bool							AutoCenter( void ) const { return viewMode.autoCenter; }
	bool							ShowCockpit( void ) const { return viewMode.showCockpit; }	
	bool							IsInterior( void ) const { return viewMode.isInterior; }
	bool							HasPlayerShadow( void ) const { return viewMode.playerShadow; }
	bool							IsCockpitShadowed( void ) const { return !viewMode.noCockpitShadows; }

	void							SetPosition( sdVehiclePosition* _position ) { position = _position; }

	const idVec3&					GetThirdPersonViewOrigin() const { return thirdPersonViewOrigin; }
	const idMat3&					GetThirdPersonViewAxis() const { return thirdPersonViewAxes; }

	virtual const idAngles			GetRequiredViewAngles( const idVec3& aimPosition ) const;

public:
	static void						Startup( void );
	static void						Shutdown( void );
	static sdVehicleView*			AllocView( const char* name );

protected:
	virtual void					SetupEyes( idAnimatedEntity* other ) = 0;

	void							ClipView( idVec3& viewOrigin, const idVec3& pivotPoint );
	void							DoFreshEntryAngles( idAngles& viewAngles, idAngles& oldAngles ) const;
	virtual void					CalcNewViewAngles( idAngles& angles ) const {}
	virtual void					GetInitialViewAxis( idMat3& aim ) const;

	void							ClampFinalAxis( idMat3& axis, const idMat3& baseAxis, idMat3& dampedAxis ) const;

	sdVehiclePosition*				position;
	positionViewMode_t				viewMode;

	const idDeclTable*				zoomTable;

	idCVar*							sensitivityYaw;
	idCVar*							sensitivityPitch;
	idCVar*							sensitivityYawScale;
	idCVar*							sensitivityPitchScale;

	idVec3							thirdPersonViewOrigin;
	idMat3							thirdPersonViewAxes;

	mutable bool					freshEntry;
	idMat3							entryAim;

private:
	static sdVehicleViewFactory	viewFactory;
};


class sdDampedVehicleView : public sdVehicleView {
public:
	virtual							~sdDampedVehicleView() { }

	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "standard"; }

	virtual void					OnPlayerSwitched( idPlayer* player, bool newPosition );

	virtual void					CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );

	virtual const idAngles			GetRequiredViewAngles( const idVec3& aimPosition ) const;

protected:
	virtual void					GetDampingPos( idVec3& pos ) const;
	virtual void					GetDampingAxis( idMat3& axis ) const;

	virtual void					DampEyeAxes( const idMat3& oldAxes, idMat3& outAxes );
	virtual void					SetupEyes( idAnimatedEntity* other );
	virtual void					CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles );
	virtual void					CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles );
	virtual void					CalcThirdPersonView( const idVec3& fpOrigin, const idMat3& fpAxis );

	virtual void					CalcNewViewAngles( idAngles& angles ) const;
	virtual void					GetInitialViewAxis( idMat3& aim ) const;

	jointHandle_t					eyeJoint;
	idVec3							eyeOffset;
	idMat3							eyeBaseAxis;
	idMat3							eyeAxisOffset;

	jointHandle_t					eyeBaseJoint;
	idVec3							eyeBaseOffset;

	idMat3							lastReturnedAxis;
};


class sdDampedVehicleView_Pivot : public sdDampedVehicleView {
public:
	virtual const char*			GetTypeName() const { return TypeName(); }
	static const char*			TypeName() { return "pivot"; }

	virtual const idAngles		GetRequiredViewAngles( const idVec3& aimPosition ) const;

protected:
	virtual void				GetDampingPos( idVec3& pos ) const;
	virtual void				GetDampingAxis( idMat3& axis ) const;

	virtual void				CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles );

	virtual void				CalcNewViewAngles( idAngles& angles ) const;
};

class sdDampedVehicleView_FreePivot : public sdDampedVehicleView_Pivot {
public:
	virtual const char*			GetTypeName() const { return TypeName(); }
	static const char*			TypeName() { return "freepivot"; }

	virtual const idAngles		GetRequiredViewAngles( const idVec3& aimPosition ) const;

	virtual void				CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );
	virtual void				ClampViewAngles( idAngles& viewAngles, const idAngles& oldAngles ) const;

protected:
	virtual void				GetDampingAxis( idMat3& axis ) const;

	virtual void				CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles );
	virtual void				CalcNewViewAngles( idAngles& angles ) const;
};

class sdDampedVehicleView_Player : public sdDampedVehicleView {
public:
	virtual const char*			GetTypeName() const { return TypeName(); }
	static const char*			TypeName() { return "player"; }

	virtual void				Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode );

	virtual const idAngles		GetRequiredViewAngles( const idVec3& aimPosition ) const;

protected:
	virtual void				GetDampingAxis( idMat3& axis ) const;
	virtual void				GetDampingPos( idVec3& pos ) const;

	virtual void				CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles );
	virtual void				CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles );
	virtual void				CalcNewViewAngles( idAngles& angles ) const;
};

class sdSmoothVehicleView : public sdVehicleView {
public:
	sdSmoothVehicleView() {
		firstFrame = false;
	}

	virtual void					Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode );

	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "smooth"; }

	virtual void					CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );

	virtual void					ClampViewAngles( idAngles& viewAngles, const idAngles& oldAngles ) const;

	virtual const idAngles			GetRequiredViewAngles( const idVec3& aimPosition ) const;

	virtual void					OnPlayerSwitched( idPlayer* player, bool newPosition );

protected:
	virtual void					SetupEyes( idAnimatedEntity* other );
	virtual void					CalcNewViewAngles( idAngles& angles ) const;

	// viewEvalProperties_t is used to pass pre-calculated game information around to evaluation functions
	typedef struct {
		float			timeStep;

		idVec3			oldOrigin;
		idMat3			oldAxis;

		//  properties
		idPlayer*		driver;
		sdTransport*	owner;
		idPhysics*		ownerPhysics;
		idVec3			ownerOrigin;
		idVec3			ownerCenter;
		idMat3			ownerAxes;
		idMat3			ownerAxesT;
		idAngles		ownerAngles; 
		idVec3			ownerVelocity;
		idVec3			ownerDirection;
		float			ownerSpeed;

		idAngles		oldAxisAngles;
		idAngles		viewAngles; 
	} viewEvalProperties_t;

	// viewEvalState_t is just used to pass around to the various evaluation functions
	// note that it DOES NOT store anything across frames.
	typedef struct {
		idMat3			ownerYawAxis;

		// evaluation
		idMat3			aimMatrix;
		idVec3			cameraOrigin;
		idMat3			cameraAxis;

		idVec3			focusPoint;
		float			newCameraDistance;
		float			newCameraHeightDelta;
	} viewEvalState_t;

	// Stages of CalculateViewPos
	virtual void					DoTeleporting( viewEvalProperties_t& state ) const;
	virtual void					CalculateAimMatrix( const viewEvalProperties_t& state );
	virtual idVec3					CalculateCameraDelta( const viewEvalProperties_t& state );
	virtual void					ClampToViewConstraints( const viewEvalProperties_t& state );
	virtual void					ClampToWorld( const viewEvalProperties_t& state );
	virtual void					DampenMotion( const viewEvalProperties_t& state );

	virtual float					DampenYaw( float input ) const { return input * viewMode.dampSpeed; }
	virtual float					DampenPitch( float input ) const { return input * viewMode.dampSpeed; }

	viewEvalState_t					evalState;

	idMat3							previousAimMatrix;
	idMat3							previousRawAimMatrix;
	float							previousCameraDistance;
	float							previousCameraHeightDelta;
	idVec3							previousOwnerOrigin;

	bool							firstFrame;
};

class sdSmoothVehicleView_Free : public sdSmoothVehicleView {
public:
	sdSmoothVehicleView_Free() {
	}

	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "smooth_free"; }

	virtual void					ClampViewAngles( idAngles& viewAngles, const idAngles& oldAngles ) const;

protected:
	virtual void					CalcNewViewAngles( idAngles& angles ) const;

	// Stages of CalculateViewPos
	virtual void					DoTeleporting( viewEvalProperties_t& state ) const;
	virtual void					CalculateAimMatrix( const viewEvalProperties_t& state );

	virtual float					DampenYaw( float input ) const { return input; }
};

class sdSmoothVehicleView_Locked : public sdSmoothVehicleView {
public:
	sdSmoothVehicleView_Locked() {
		topHatTransition = 0.0f;
		previousViewAngles.Zero();
	}

	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "smooth_locked"; }

	virtual void					ClampViewAngles( idAngles& viewAngles, const idAngles& oldAngles ) const;
	virtual void					CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );

	virtual void					OnPlayerSwitched( idPlayer* player, bool newPosition );

protected:
	bool							InTophat( const idPlayer* player ) const;

	virtual void					CalcNewViewAngles( idAngles& angles ) const;

	// Stages of CalculateViewPos
	virtual void					DoTeleporting( viewEvalProperties_t& state ) const;
	virtual void					CalculateAimMatrix( const viewEvalProperties_t& state );
	virtual idVec3					CalculateCameraDelta( const viewEvalProperties_t& state );
	virtual void					ClampToViewConstraints( const viewEvalProperties_t& state );
	virtual void					DampenMotion( const viewEvalProperties_t& state );

	virtual float					DampenYaw( float input ) const;
	virtual float					DampenPitch( float input ) const;

	float							topHatTransition;

	idAngles						previousViewAngles;
};

class sdIcarusVehicleView : public sdVehicleView {
public:
	virtual const char*				GetTypeName() const { return TypeName(); }
	static const char*				TypeName() { return "icarus"; }

	virtual void					CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );
	virtual const idAngles			GetRequiredViewAngles( const idVec3& aimPosition ) const;

protected:
	virtual void					SetupEyes( idAnimatedEntity* other );
	virtual void					CalcNewViewAngles( idAngles& angles ) const;
};

#endif // __GAME_VEHICLES_VEHICLEVIEW_H__
