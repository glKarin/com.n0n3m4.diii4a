
#ifndef __PREY_BEAM_H__
#define __PREY_BEAM_H__

#define MAX_BEAM_SPLINE_CONTROLS	6
#define BEAM_SPLINE_CONTROL_STEP	( 1.0f / ( MAX_BEAM_SPLINE_CONTROLS - 1 ) )
#define	EXTRA_SPLINE_CONTROLS		2

#define BEAM_HZ						30
#define BEAM_MSEC					(1000 / BEAM_HZ)

extern const idEventDef EV_ToggleBeamLength;

class hhBeamSystem;
class hhBeam;
class hhDeclBeam;

/*
class beamCmd {
public:
	virtual void	Execute( hhBeam *beam );
};

// BEAM COMMANDS
class beamCmd_SplineLinearToTarget : public beamCmd {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_SplineArcToTarget : public beamCmd {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_SplineAdd : public beamCmd {
public:
	void	Execute( hhBeam *beam );

	int		index;
	idVec3	offset;
};

class beamCmd_SplineAddSin : public beamCmd_SplineAdd {
public:
	void	Execute( hhBeam *beam );

	idVec3	phase;
};

class beamCmd_SplineAddSinTime : public beamCmd_SplineAddSin {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_SplineAddSinTimeScaled : public beamCmd_SplineAddSin {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_ConvertSplineToNodes : public beamCmd {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_NodeLinearToTarget : public beamCmd {
public:
	void	Execute( hhBeam *beam );
};

class beamCmd_NodeElectric : public beamCmd {
public:
	void	Execute( hhBeam *beam );

	idVec3	offset;
	bool	bNotEnds;
};
// END BEAM COMMANDS
*/

class hhBeam {
public:
			hhBeam();	
			~hhBeam();
	void	Init( hhBeamSystem *newSystem, hhBeamNodes_t *newInfo );

	idBounds	GetBounds();

	void	VerifyNodeIndex(int index, const char *functionName);
	void	VerifySplineIndex(int index, const char *functionName);
	idVec3	NodeGet(int index);
	void	NodeSet(int index, idVec3 value);
	idVec3	SplineGet(int index);
	void	SplineSet(int index, idVec3 value);

	void	SplineLinear(idVec3 start, idVec3 end);
	void	SplineLinearToTarget( void );
	void	SplineArc( idVec3 start, idVec3 end, idVec3 startVec );
	void	SplineArcToTarget( void );

	void	SplineAddSin(int index, float phaseX, float phaseY, float phaseZ, float	offsetX, float offsetY, float offsetZ);
	void	SplineAddSinTime(int index, float phaseX, float phaseY, float phaseZ, float	offsetX, float offsetY, float offsetZ);
	void	SplineAddSinTimeScaled(int index, float phaseX, float phaseY, float phaseZ, float	offsetX, float offsetY, float offsetZ);
	void	SplineAdd(int index, idVec3 offset);
	void	ConvertSplineToNodes(void);

	void	NodeLinear(idVec3 start, idVec3 end);
	void	NodeLinearToTarget( void );
	void	NodeAdd(int index, idVec3 offset);
	void	NodeElectric(float x, float y, float z, bool bNotEnds);

	void	TransformNodes(void);

	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile, hhBeamSystem *newSystem, hhBeamNodes_t *newInfo );

protected:
	idVec3			*nodeList; // index into renderEntity nodes
	hhBeamSystem	*system;
	idVec3			splineList[MAX_BEAM_SPLINE_CONTROLS + EXTRA_SPLINE_CONTROLS];
};


class hhBeamSystem : public idEntity {
public:
	CLASS_PROTOTYPE( hhBeamSystem );

	void			Spawn(void);
					hhBeamSystem();
					~hhBeamSystem();

	virtual void	Think(void);
	virtual void	UpdateModel( void );

	//rww - network friendliness
	virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual void	ClientPredictionThink( void );

	virtual void	InitSystem( const char* modelName );
	virtual float	GetBeamTime( void ) { return beamTime; }
	virtual void	SetTargetLocation(idVec3 newLoc);
	virtual void	SetTargetEntity( idEntity *ent, int traceId=0, idVec3 &offset=vec3_origin );
	virtual void	SetTargetEntity( idEntity *ent, const char *boneName, idVec3 &offset=vec3_origin );

	virtual bool	IsActivated() const { return bActive; }
	virtual void	Activate( const bool bActivate );

	virtual idVec3	GetTargetLocation( void );
	virtual idMat3	GetBeamAxis( void ) { return( beamAxis ); }

	virtual void	SetArcVector( idVec3 vec ) { arcVector = vec; }
	virtual idVec3	GetArcVector( void ) { return( arcVector ); }

	virtual float	GetBeamPhaseScale( void ) { return phaseScale; }
	virtual void	SetBeamPhaseScale( float scale ) { phaseScale = scale; }
	virtual float	GetBeamOffsetScale( void ) { return offsetScale; }
	virtual void	SetBeamOffsetScale( float scale ) { offsetScale = scale; }

	//AOB
	virtual void	SetAxis( const idMat3 &axis );
	virtual float	GetBeamLength( void ) {	return( beamLength ); }
	virtual void	SetBeamLength( const float length );
	virtual void	ToggleBeamLength( bool rigid ) { bRigidBeamLength = rigid; }

	virtual void	ExecuteBeam( int index, hhBeam *beam );

	static hhBeamSystem* SpawnBeam( const idVec3& start, const char* modelName, const idMat3& axis = mat3_identity, bool pureLocal = false );

	idRandom				random;
	idStr					declName;
	const hhDeclBeam		*decl;

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	//rww - for functionality regarding client entity beams.
	idEntityPtr<idEntity>	snapshotOwner;
protected:
	virtual void			Event_Activate( idEntity *activator );
	void					Event_SetBeamPhaseScale( float scale ); // bg
	void					Event_SetBeamOffsetScale( float scale ); // bg

	float					beamTime;
	hhBeam					*beamList;

	idMat3					beamAxis;
	idVec3					arcVector; // For SplineArc

	idVec3					targetLocation;
	idEntityPtr<idEntity>	targetEntity;
	idVec3					targetEntityOffset;
	int						targetEntityId;

	//AOB
	float					beamLength;
	bool					bRigidBeamLength;

	// Used to scale the phase/offset of the beam in code
	float					phaseScale;
	float					offsetScale;

protected:
	bool			bActive;
};

#endif /* __PREY_BEAM_H__ */
