//**************************************************************************
//**
//** PREY_BEAM.CPP
//**
//** Code for Prey-specific beams
//** 
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../idlib/precompiled.h"
#pragma hdrstop

// Game code includes
#include "prey_local.h"

#define DEBUG_BEAMS		0

static idVec3 CurveExtractSpline(idVec3 *cPoints, int cPointCount, float alpha, bool bThruCtrlPnts);

const idEventDef EV_ToggleBeamLength("<toggleBeamLength>", "d");
const idEventDef EV_SetBeamPhaseScale( "setBeamPhaseScale", "f" ); // bg
const idEventDef EV_SetBeamOffsetScale( "setBeamOffsetScale", "f" ); // bg

CLASS_DECLARATION(idEntity, hhBeamSystem)
	EVENT( EV_Activate,				hhBeamSystem::Event_Activate )
	EVENT( EV_SetBeamPhaseScale,	hhBeamSystem::Event_SetBeamPhaseScale) // bg
	EVENT( EV_SetBeamOffsetScale,	hhBeamSystem::Event_SetBeamOffsetScale) // bg
END_CLASS


hhBeamSystem::hhBeamSystem() {
	beamList = NULL;
	offsetScale = 1.0f;
	phaseScale = 1.0f;
	bActive = false;

	targetLocation = vec3_origin;

	decl = NULL;
}

hhBeamSystem::~hhBeamSystem() {
	if(renderEntity.beamNodes) {
		Mem_Free( renderEntity.beamNodes );
		renderEntity.beamNodes = NULL;
	}

	if(beamList) {
		delete [] beamList;
		beamList = NULL;
	}
}

void hhBeamSystem::Spawn(void) {
	beamList = NULL;

	if (!gameLocal.isClient || fl.clientEntity) { //rww - for client in mp this gets done after the first snapshot is read...
		InitSystem( spawnArgs.GetString("model") );
	}

	beamLength = spawnArgs.GetFloat( "lengthBeam" );
	bRigidBeamLength = spawnArgs.GetBool( "rigidBeamLength" );

	targetEntity = NULL;
	targetEntityId = 0;
	targetEntityOffset.Zero();

	SetArcVector( vec3_origin );

	bActive = spawnArgs.GetBool( "start_off" );
	Activate( !bActive );
}

void hhBeamSystem::Activate( const bool bActivate ) {
	if( bActive != bActivate ) {
		bActive = bActivate;

		if( bActive ) {
			BecomeActive( TH_UPDATEPARTICLES );
			Show();
		}else {
			BecomeInactive( TH_UPDATEPARTICLES );
			Hide();
		}
	}
}

hhBeamSystem* hhBeamSystem::SpawnBeam( const idVec3& start, const char* modelName, const idMat3& axis, bool pureLocal ) {
	idDict	Args;
	hhBeamSystem*	beamSystem = NULL;
	idStr modelString;

#if !GOLD
	//rww - checking for creation of beams at nan location
	assert(!FLOAT_IS_NAN(start.x));
	assert(!FLOAT_IS_NAN(start.y));
	assert(!FLOAT_IS_NAN(start.z));
	assert(!FLOAT_IS_NAN(axis[0].x));
	assert(!FLOAT_IS_NAN(axis[0].y));
	assert(!FLOAT_IS_NAN(axis[0].z));
	assert(!FLOAT_IS_NAN(axis[1].x));
	assert(!FLOAT_IS_NAN(axis[1].y));
	assert(!FLOAT_IS_NAN(axis[1].z));
	assert(!FLOAT_IS_NAN(axis[2].x));
	assert(!FLOAT_IS_NAN(axis[2].y));
	assert(!FLOAT_IS_NAN(axis[2].z));
#endif

	if ( !modelName || !modelName[0] ) {
		return NULL;
	}

	// Ensure that the modelname ends with a valid .beam extention
	modelString = modelName;
	modelString.DefaultFileExtension( ".beam" );

	Args.Set( "spawnclass", "hhBeamSystem" );
	Args.Set( "model", modelString.c_str() );
	Args.SetVector( "origin", start );
	Args.SetMatrix( "rotation", axis );

	HH_ASSERT(!gameLocal.isClient || pureLocal);

	beamSystem = (hhBeamSystem *)gameLocal.SpawnEntityTypeClient( hhBeamSystem::Type, &Args );
	if (beamSystem) { //make sure it's sync'd
		beamSystem->fl.networkSync = !pureLocal;
	}
	HH_ASSERT( beamSystem );

	beamSystem->SetTargetEntity( NULL );
	beamSystem->SetTargetLocation( start ); // Necessary in case the beam starts out hidden
	beamSystem->beamAxis = mat3_identity;

	return beamSystem;
}

//==========================================================================
//
// hhBeamSystem::InitSystem
//
// Reads beam system information from the entity definition
// 
// Commands:
//		beamNum		<int>		- total number of beams in the system
//		beamNodes	<int>		- total number of nodes per beam
//
//		Per beam (end number specifies the beam):
//
//		beamThickness0		<int>		- Thickness
//		beamTaperEndPoints0	<bool>		- If the beam should have tapered end points
//		beamShader0			<string>	- Shader name
//		beamSpline0			<bool>		- if this beam system uses splines
//==========================================================================

void hhBeamSystem::InitSystem( const char* modelName ) {
	int				i;

	HH_ASSERT( modelName );

	if(renderEntity.beamNodes) { // Delete the previous version before initializing
		Mem_Free(renderEntity.beamNodes);
		renderEntity.beamNodes = NULL;
	}

	// Initialize beam data from the beam file
	declName = modelName;
	decl = declManager->FindBeam( modelName );
	renderEntity.declBeam = decl;

	// Get new model loaded
	if( !renderEntity.hModel || idStr::Icmp( renderEntity.hModel->Name(), modelName ) ) {
		renderEntity.hModel = renderModelManager->FindModel( modelName );
	}

	// Allocate the beams
	if( beamList ) {
		delete[] beamList;
	}
	beamList = new hhBeam[decl->numBeams];

	// Allocate the number of beam infos (used for rendering)
	renderEntity.beamNodes = (hhBeamNodes_t *)Mem_ClearedAlloc(sizeof(hhBeamNodes_t) * decl->numBeams);
	
	beamAxis = GetPhysics()->GetAxis();

	// Initialize each beam with specific info
	for(i = 0; i < decl->numBeams; i++) {
		beamList[i].Init( this, &renderEntity.beamNodes[i] );
	}

	random.SetSeed( gameLocal.time );
	beamTime = gameLocal.time + random.RandomInt( 5000 ); // Don't let any beams stay in sync.
}

//==========================================================================
//
// hhBeamSystem::SetTargetLocation
//
//==========================================================================

void hhBeamSystem::SetTargetLocation(idVec3 newLoc) {
	targetLocation = newLoc;

	beamLength = (targetLocation - GetPhysics()->GetOrigin()).Length();
}

//==========================================================================
//
// hhBeamSystem::SetTargetEntity
//
// Sets the entity to target from this beam.  If joint is INVALID_JOINT,
// then the origin of the entity will be used.
//
// Passing in a NULL entity will clear the target entity, and set the targetLocation to offset
//
// NOTE: The offset passed in should be in world coordinates relative to the model.
// NOTE: The offset can be used as an offset from the joint as well.
//==========================================================================

void hhBeamSystem::SetTargetEntity( idEntity *ent, int traceId, idVec3 &offset ) {
	idVec3 origin;
	idMat3 axis;

	targetEntity = ent;

	if ( ent == NULL ) {
		targetEntityId = 0;
		targetEntityOffset.Zero();
		SetTargetLocation( offset );
		return;
	}

	targetEntityId = traceId;
	targetEntityOffset = offset;

	// Update the targetLocation
	GetTargetLocation();

	// Calculate the beam length from the new target location
	beamLength = (targetLocation - GetPhysics()->GetOrigin()).Length();
}

//==========================================================================
//
// hhBeamSystem::SetTargetEntity
//
// Bone name version
//==========================================================================

void hhBeamSystem::SetTargetEntity( idEntity *ent, const char *boneName, idVec3 &offset ) {
	idVec3 origin;
	idMat3 axis;

	targetEntity = ent;

	if ( ent == NULL || boneName == NULL ) {
		targetEntityId = 0;
		targetEntityOffset.Zero();
		SetTargetLocation( offset );
		return;
	}

	targetEntityId = ent->GetAnimator()->GetJointHandle( boneName );
	targetEntityOffset = offset;

	// Update the targetLocation
	GetTargetLocation();

	// Calculate the beam length from the new target location
	beamLength = (targetLocation - GetPhysics()->GetOrigin()).Length();
}

//==========================================================================
//
// hhBeamSystem::GetTargetLocation
//
//==========================================================================

idVec3 hhBeamSystem::GetTargetLocation( void ) {
	idVec3			origin;
	idMat3			axis;	 
	idVec3			boneOrigin;
	idMat3			boneAxis;	 
	jointHandle_t	joint;
	idAFEntity_Base *af = NULL;

	if ( targetEntity.IsValid() ) { // Has a target entity, so compute the offset	
		if ( targetEntityId > 0 ) { // This is a specific joint:
			joint = (jointHandle_t)targetEntityId;
		} else { // This is a collision handle
			joint = CLIPMODEL_ID_TO_JOINT_HANDLE( targetEntityId );
		}

		if ( targetEntity->IsType( idAFEntity_Base::Type ) ) {
			af = static_cast<idAFEntity_Base *>( targetEntity.GetEntity() );
		}

		axis = targetEntity->GetAxis();
		origin = targetEntity->GetOrigin();

		// Calculate the targetLocation, based upon the type of entity
		if ( af && af->IsActiveAF() ) { // AF, use the clip model id for the origin
			int body = af->BodyForClipModelId( targetEntityId );
			origin = af->GetPhysics()->GetOrigin( body );
			targetLocation = origin + axis * targetEntityOffset;			
		//rww - added check for targetEntity->GetAnimator() not being null, in case targetEntity is not animated
		} else if ( joint != INVALID_JOINT && targetEntity->GetAnimator() && targetEntity->GetAnimator()->GetJointTransform( joint, gameLocal.time, boneOrigin, boneAxis ) ) { // Joint-based
			targetLocation = origin + axis * boneOrigin;
		} else { // Default - use origin of the entity
			targetLocation = origin + axis * targetEntityOffset;
		}
	}

	return targetLocation; 
}

//==========================================================================
//
// hhBeamSystem::SetAxis
//
//==========================================================================
void hhBeamSystem::SetAxis( const idMat3 &axis ) {
	if (!GetPhysics()->GetAxis().Compare(axis)) {
		idEntity::SetAxis(axis);
	}
}

//==========================================================================
//
// hhBeamSystem::SetBeamLength
//
//==========================================================================
void hhBeamSystem::SetBeamLength( const float length ) {
	beamLength = length;

	if( bRigidBeamLength && beamLength > VECTOR_EPSILON ) {
		SetTargetLocation( GetPhysics()->GetOrigin() + GetPhysics()->GetAxis()[0] * beamLength );
	}
}

/*
=================
hhBeamSystem::WriteToSnapshot
rww - write applicable beam values to snapshot
=================
*/
void hhBeamSystem::WriteToSnapshot( idBitMsgDelta &msg ) const {
#if !GOLD
	gameLocal.Warning("Beam %i being sync'd by server!\n", entityNumber);
#endif
	GetPhysics()->WriteToSnapshot( msg );

	msg.WriteFloat(beamTime);

	//handle this.
	//hhBeam					*beamList;

	//this is the common method, but doing WriteDirs might be more efficient?
	idCQuat quat = beamAxis.ToCQuat();
	msg.WriteFloat(quat.x);
	msg.WriteFloat(quat.y);
	msg.WriteFloat(quat.z);

	msg.WriteFloat(arcVector[0]);
	msg.WriteFloat(arcVector[1]);
	msg.WriteFloat(arcVector[2]);

	msg.WriteFloat(targetLocation[0]);
	msg.WriteFloat(targetLocation[1]);
	msg.WriteFloat(targetLocation[2]);

	if (targetEntity.IsValid()) {
		msg.WriteBits(1, 1);
		msg.WriteBits(targetEntity->entityNumber, GENTITYNUM_BITS);
	}
	else {
		msg.WriteBits(0, 1);
	}

	msg.WriteFloat(targetEntityOffset[0]);
	msg.WriteFloat(targetEntityOffset[1]);
	msg.WriteFloat(targetEntityOffset[2]);

	//do i care about this?
	//int						targetEntityId;

	msg.WriteFloat(beamLength);
	msg.WriteBits(bRigidBeamLength, 1);

	msg.WriteFloat(phaseScale);
	msg.WriteFloat(offsetScale);

	msg.WriteBits(bActive, 1);

	//rwwFIXME this is horrible. oh so very horrible. all those calls to SpawnBeam in code need to be removed,
	//and they must all use seperate entity defs, or there is no alternative to sending a real string. =|
	msg.WriteString(spawnArgs.GetString("model"));
}

/*
=================
hhBeamSystem::ReadFromSnapshot
rww - read applicable beam values from snapshot
=================
*/
void hhBeamSystem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot( msg );

	beamTime = msg.ReadFloat();

	idCQuat quat;
	quat.x = msg.ReadFloat();
	quat.y = msg.ReadFloat();
	quat.z = msg.ReadFloat();
	beamAxis = quat.ToMat3();

	arcVector[0] = msg.ReadFloat();
	arcVector[1] = msg.ReadFloat();
	arcVector[2] = msg.ReadFloat();

	targetLocation[0] = msg.ReadFloat();
	targetLocation[1] = msg.ReadFloat();
	targetLocation[2] = msg.ReadFloat();

	int hasEnt = msg.ReadBits(1);
	if (hasEnt) {
		int entNum = msg.ReadBits(GENTITYNUM_BITS);
		targetEntity = gameLocal.entities[entNum];
	}
	else {
		targetEntity = NULL;
	}

	targetEntityOffset[0] = msg.ReadFloat();
	targetEntityOffset[1] = msg.ReadFloat();
	targetEntityOffset[2] = msg.ReadFloat();

	beamLength = msg.ReadFloat();
	bRigidBeamLength = !!msg.ReadBits(1);

	phaseScale = msg.ReadFloat();
	offsetScale = msg.ReadFloat();

	bool active = !!msg.ReadBits(1);
	if (active != bActive) {
		Activate(active);
	}

	char modelName[128];
	msg.ReadString(modelName, 128);
	if (!beamList) {
		//then init the system.
		InitSystem(modelName);
	}
}

/*
=================
hhBeamSystem::ClientPredictionThink
rww - minimal think on client
=================
*/
void hhBeamSystem::ClientPredictionThink( void ) {
	if (fl.clientEntity && snapshotOwner.IsValid()) {
		if (!bActive || !gameLocal.EntInClientSnapshot(snapshotOwner->entityNumber)) { //if the snapshot entity i'm associated with is not in the snapshot, i hide
			Hide();
		}
		else {
			Show();
		}
	}

	if (gameLocal.isNewFrame) {
		Think();
	}
}

//==========================================================================
//
// hhBeamSystem::Think
//
// NOTE:  Beams will not think AT ALL if they are hidden or not visible
//==========================================================================

void hhBeamSystem::Think( void ) {

	RunPhysics();

	if (thinkFlags & TH_UPDATEPARTICLES) {

		if( targets.Num() > 0 ) {
			SetTargetEntity( targets[0].GetEntity(), NULL );
		}

		// Update the beamAxis to correctly reflect the target
		if( !bRigidBeamLength ) {
			idVec3 vec = (GetTargetLocation() - GetPhysics()->GetOrigin());
			beamLength = vec.Normalize();

			if ( beamLength <= 0 ) { // Ignore beams with no length
				return;
			}

			if( GetBindMaster() && fl.bindOrientated ) {
				beamAxis = GetAxis();
			} else {
				beamAxis = vec.ToMat3();

				if ( targets.Num() > 0 ) {
					SetAxis( beamAxis ); // Target beams should update renderEntity information
				} else {
					GetPhysics()->SetAxis( beamAxis ); // Normal beams don't need to update render entity information
				}
			}
		} else {
			SetTargetLocation( GetPhysics()->GetOrigin() + GetPhysics()->GetAxis()[0] * beamLength );
			beamAxis = GetPhysics()->GetAxis();
		}

		assert(decl);
		for( int i = 0; i < decl->numBeams; i++) {
			ExecuteBeam( i, &beamList[i] );
			beamList[i].TransformNodes();
		}
	}

	Present();
}

//==========================================================================
//
// hhBeamSystem::Event_Activate
//
// Triggering a beam will toggle its visibility
//==========================================================================

void hhBeamSystem::Event_Activate( idEntity *activator ) {
	Activate( !IsActivated() );
}

//==========================================================================
//
// hhBeamSystem::Event_SetBeamPhaseScale
//
//==========================================================================

void hhBeamSystem::Event_SetBeamPhaseScale( float scale ) {
	SetBeamPhaseScale( scale );
}

//==========================================================================
//
// hhBeamSystem::Event_SetBeamOffsetScale
//
//==========================================================================

void hhBeamSystem::Event_SetBeamOffsetScale( float scale ) {
	SetBeamOffsetScale( scale );
}

//==========================================================================
//
// hhBeamSystem::UpdateModel
//
//==========================================================================

void hhBeamSystem::UpdateModel( void ) {

	if( renderEntity.hModel && !this->fl.hidden ) { // Only calculate the bounds if the model is valid and the model is visible
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
		renderEntity.bounds.AddPoint(idVec3(-16,-16,-16));
		renderEntity.bounds.AddPoint(idVec3( 16, 16, 16));
	}

	idEntity::UpdateModel();
}


// BEAM CODE ---------------------------------------------------------------------

//==========================================================================
//
// hhBeam::hhBeam
//
//==========================================================================

hhBeam::hhBeam() {
	nodeList = NULL;
}

//==========================================================================
//
// hhBeam::~hhBeam
//
//==========================================================================

hhBeam::~hhBeam() {
	nodeList = NULL;
}

//==========================================================================
//
// hhBeam::Init
//
//==========================================================================

void hhBeam::Init( hhBeamSystem *newSystem, hhBeamNodes_t *newInfo ) {
	int i;

	system = newSystem;
	nodeList = newInfo->nodes;

	for(i = 0; i < MAX_BEAM_SPLINE_CONTROLS + EXTRA_SPLINE_CONTROLS; i++) {
		splineList[i] = idVec3(0, 0, 0);
	}
}

//==========================================================================
//
// hhBeam::VerifyNodeIndex
//
//==========================================================================

void hhBeam::VerifyNodeIndex(int index, const char *functionName) {
#if DEBUG_BEAMS
	if(index < 0 || index >= system->decl->numNodes) {
		gameLocal.Error("%s: %d\n", functionName, index);
	}
#endif
}

//==========================================================================
//
// hhBeam::VerifySplineIndex
//
//==========================================================================

void hhBeam::VerifySplineIndex(int index, const char *functionName) {
#if DEBUG_BEAMS
	if(index < 0 || index >= MAX_BEAM_SPLINE_CONTROLS) {
		gameLocal.Error("%s: %d\n", functionName, index);
	}
#endif
}

//==========================================================================
//
// hhBeam::NodeGet
//
// Returns the node location in world space
//==========================================================================

idVec3 hhBeam::NodeGet(int index) {
#if DEBUG_BEAMS
	VerifyNodeIndex(index, "hhBeam::NodeGet");
#endif

	return( nodeList[index] );
}

//==========================================================================
//
// hhBeam::NodeSet
//
//==========================================================================

void hhBeam::NodeSet(int index, idVec3 value) {
#if DEBUG_BEAMS
	VerifyNodeIndex(index, "hhBeam::NodeSet");
#endif

	nodeList[index] = value;
}

//==========================================================================
//
// hhBeam::SplineGet
//
//==========================================================================

idVec3 hhBeam::SplineGet(int index) {
#if DEBUG_BEAMS
	VerifySplineIndex(index, "hhBeam::SplineSet");
#endif

	return(splineList[index]);
}

//==========================================================================
//
// hhBeam::SplineSet
//
//==========================================================================

void hhBeam::SplineSet(int index, idVec3 value) {
#if DEBUG_BEAMS
	VerifySplineIndex(index, "hhBeam::SplineSet");
#endif

	splineList[index] = value;
}

//==========================================================================
//
// hhBeam::SplineLinear
//
//==========================================================================

void hhBeam::SplineLinear(idVec3 start, idVec3 end) {
	int i;
	idVec3 loc;
	idVec3 delta;

	loc = start;
	delta = (end - start) * BEAM_SPLINE_CONTROL_STEP;

	for(i = 0; i < MAX_BEAM_SPLINE_CONTROLS; i++) {
		splineList[i] = loc;
		loc += delta;
	}
}

//==========================================================================
//
// hhBeam::SplineLinearToTarget
//
//==========================================================================

void hhBeam::SplineLinearToTarget( void ) {
	SplineLinear( system->GetRenderEntity()->origin, system->GetTargetLocation() );
}

//==========================================================================
//
// hhBeam::SplineArc
//
//==========================================================================

void hhBeam::SplineArc( idVec3 start, idVec3 end, idVec3 startVec ) {
	int i;
	idMat3 axis;
	idVec3 loc;
	idVec3 delta;

	idVec3 temp = startVec;
	temp.Normalize();

	// Calculate the translation matrix
	axis[0] = ( end - start );
	float dist = axis[0].Normalize();
	axis[2] = temp.Cross( axis[0] );
	axis[1] = axis[2].Cross( axis[0] );

	float dp = temp * axis[0];

	loc = start;
	delta = (end - start) * BEAM_SPLINE_CONTROL_STEP;

	float x = 0;
	for( i = 0; i < MAX_BEAM_SPLINE_CONTROLS; i++, x += BEAM_SPLINE_CONTROL_STEP ) {
		float temp = -dist * dp * x * ( 1 - x );
		splineList[i] = loc + axis[1] * temp;
		loc += delta;
	}
}

//==========================================================================
//
// hhBeam::SplineArcToTarget
//
//==========================================================================

void hhBeam::SplineArcToTarget( void ) {
	SplineArc( system->GetRenderEntity()->origin, system->GetTargetLocation(), system->GetArcVector() );
}

//==========================================================================
//
// hhBeam::SplineAddSin
//
//==========================================================================

void hhBeam::SplineAddSin(int index, float phaseX, float phaseY, float phaseZ, float offsetX, float offsetY, float offsetZ) {
	idVec3 sinOffset;

#if DEBUG_BEAMS
	VerifySplineIndex(index, "hhBeam::SplineAddSin");
#endif

	sinOffset.x = offsetX * idMath::Sin(phaseX);
	sinOffset.y = offsetY * idMath::Sin(phaseY);
	sinOffset.z = offsetZ * idMath::Sin(phaseZ);

	splineList[index] += sinOffset * system->GetBeamAxis();
}

//==========================================================================
//
// hhBeam::SplineAddSinTime
//
// Multiplies the sin value by the current game time
//==========================================================================

void hhBeam::SplineAddSinTime(int index, float phaseX, float phaseY, float phaseZ, float offsetX, float offsetY, float offsetZ) {
	float t;

	t = (gameLocal.time - system->GetBeamTime()) * 0.01f;
	SplineAddSin( index, phaseX * t, phaseY * t, phaseZ * t, offsetX, offsetY, offsetZ );
}

//==========================================================================
//
// hhBeam::SplineAddSinTimeScaled
//
// Multiplies the sin value by the current game time
//==========================================================================

void hhBeam::SplineAddSinTimeScaled(int index, float phaseX, float phaseY, float phaseZ, float offsetX, float offsetY, float offsetZ) {
	float t;
	float offsetScale = system->GetBeamOffsetScale();

	t = system->GetBeamPhaseScale() * (gameLocal.time - system->GetBeamTime()) * 0.01f;
	SplineAddSin( index, phaseX * t, phaseY * t, phaseZ * t, 
		offsetX * offsetScale, offsetY * offsetScale, offsetZ * offsetScale );
}

//==========================================================================
//
// hhBeam::SplineAdd
//
//==========================================================================

void hhBeam::SplineAdd(int index, idVec3 offset) {
#if DEBUG_BEAMS
	VerifySplineIndex(index, "hhBeam::SplineAdd");
#endif

	float offsetScale = system->GetBeamOffsetScale();
	splineList[index] += offset * offsetScale;
}

//==========================================================================
//
// hhBeam::ConvertSplineToNodes
//
//==========================================================================

void hhBeam::ConvertSplineToNodes(void) {
	int		i;

	// Copy the last spline point into the extra "slop" points
	// The extra points are necessary to calculate the final spline segment
	for(i = 0; i < EXTRA_SPLINE_CONTROLS; i++) {
		splineList[MAX_BEAM_SPLINE_CONTROLS + i] = splineList[MAX_BEAM_SPLINE_CONTROLS - 1];
	}

	// Construct the beam nodes from the spline points
	float inc = (float)(MAX_BEAM_SPLINE_CONTROLS) / system->decl->numNodes;
	float alpha = 0;

	for(i = 0; i < system->decl->numNodes; i++) {
		nodeList[i] = CurveExtractSpline(splineList, MAX_BEAM_SPLINE_CONTROLS + EXTRA_SPLINE_CONTROLS, alpha, true);
		alpha += inc;
	}
}

//==========================================================================
//
// hhBeam::NodeLinear
//
//==========================================================================

void hhBeam::NodeLinear(idVec3 start, idVec3 end) {
	int i;

	idVec3 sLoc = start;
	idVec3 sDelta = (end - start) / (system->decl->numNodes - 1);

	for(i = 0; i < system->decl->numNodes; i++) {
		nodeList[i] = sLoc;
		sLoc += sDelta;
	}
}

//==========================================================================
//
// hhBeam::NodeLinearToTarget
//
//==========================================================================

void hhBeam::NodeLinearToTarget( void ) {
	NodeLinear( system->GetRenderEntity()->origin, system->GetTargetLocation() );
}

//==========================================================================
//
// hhBeam::NodeAdd
//
//==========================================================================

void hhBeam::NodeAdd(int index, idVec3 offset) {
#if DEBUG_BEAMS
	VerifyNodeIndex(index,"hhBeam::NodeAdd");
#endif

	nodeList[index] += offset;
}

//==========================================================================
//
// hhBeam::NodeElectric
//
//==========================================================================

void hhBeam::NodeElectric(float x, float y, float z, bool bNotEnds) {
	int i;
	int start = 0;
	int end = system->decl->numNodes;

	if(bNotEnds) {
		start = 1;
		end = system->decl->numNodes - 1;
	}

	for(i = start; i < end; i++) {
		nodeList[i].x += system->random.CRandomFloat() * x;
		nodeList[i].y += system->random.CRandomFloat() * y;
		nodeList[i].z += system->random.CRandomFloat() * z;
	}
}

//==========================================================================
//
// hhBeam::TransformNodes
//
// Transform the worldspace nodes into model local space
//==========================================================================

void hhBeam::TransformNodes(void) {
	idVec3 origin	= system->GetRenderEntity()->origin;
	idMat3 axis		= system->GetRenderEntity()->axis.Transpose(); 
	for(int i = 0; i < system->decl->numNodes; i++) {
		nodeList[i] = ( nodeList[i] - origin ) * axis;
	}
}

//==========================================================================
//
// hhBeam::GetBounds
//
// Get local bounds of beam
//==========================================================================
idBounds hhBeam::GetBounds() {
	idBounds bounds;

	if( !nodeList ) {
		return bounds_zero;
	}

	for( int ix = 0; ix < system->decl->numNodes; ++ix ) {
		bounds.AddPoint( nodeList[ix] );
	}

	return bounds;
}

//================
//hhBeam::Save
//================
void hhBeam::Save( idSaveGame *savefile ) const {
	for( int i = 0; i < MAX_BEAM_SPLINE_CONTROLS + EXTRA_SPLINE_CONTROLS; i++ ) {
		savefile->WriteVec3( splineList[i] );
	}
}

//================
//hhBeam::Restore
//================
void hhBeam::Restore( idRestoreGame *savefile, hhBeamSystem *newSystem, hhBeamNodes_t *newInfo) {
	system = newSystem;
	nodeList = newInfo->nodes;

	for( int i = 0; i < MAX_BEAM_SPLINE_CONTROLS + EXTRA_SPLINE_CONTROLS; i++ ) {
		savefile->ReadVec3( splineList[i] );
	}
}

//=============================================================================
//
// CurveExtractSpline
//
//=============================================================================

static idVec3 CurveExtractSpline(idVec3	*cPoints, int cPointCount, float alpha, bool bThruCtrlPnts ) {
	float t, t2, t3, w1, w2, w3, w4;
	int p1, p2, p3, p4, intAlpha;
	idVec3 result;

	intAlpha = alpha;
	p2 = intAlpha;
	p1 = (p2 == 0) ? p2 : p2-1;
	p3 = p2+1;
	p4 = (p3 == cPointCount-1) ? p3 : p3+1;
	t = alpha-intAlpha;
	t2 = t * t;
	t3 = t2 * t;

	if(!bThruCtrlPnts) {
		w1 = (1-t) * (1-t) * (1-t);
		w2 = 3.0f * t3 - 6.0f * t2 + 4;
		w3 = -3.0f * t3 + 3.0f * t2 + 3.0f * t + 1;
		w4 = t3;
		return
			( w1 * cPoints[p1]
			+ w2 * cPoints[p2]
			+ w3 * cPoints[p3]
			+ w4 * cPoints[p4]) * (1.0f/6.0f);
	}

	// Uses Catmull-Rom to pass thru the control points.

	if((cPoints[p3] - cPoints[p2]).Length() <= 4.0f) // was 16
	{ // If points are close enough, just linearly interpolate
		result = cPoints[p2] + t * (cPoints[p3] - cPoints[p2]);
	}
	else {
		result = 0.5f * ((-cPoints[p1] + 3 * cPoints[p2] - 3 * cPoints[p3] + cPoints[p4]) * t3
			+ (2 * cPoints[p1] - 5 * cPoints[p2]+ 4 * cPoints[p3] - cPoints[p4]) * t2
			+ (-cPoints[p1] + cPoints[p3]) * t + 2 * cPoints[p2]);
	}

	return(result);
}

//=============================================================================
//
// hhBeamSystem::ExecuteBeam
//
// Applies the commands to the specified beam
//=============================================================================

void hhBeamSystem::ExecuteBeam( int index, hhBeam *beam ) {
	int i;
	const beamCmd_t *cmd;

	for(i = 0; i < decl->cmds[index].Num(); i++) {
		cmd = &decl->cmds[index][i];

		switch( cmd->type ) {
		case BEAMCMD_SplineLinearToTarget:
			beam->SplineLinearToTarget();
			break;
		case BEAMCMD_SplineArcToTarget:
			beam->SplineArcToTarget();
			break;
		case BEAMCMD_SplineAdd:
			beam->SplineAdd( cmd->index, cmd->offset );
			break;
		case BEAMCMD_SplineAddSin:
			beam->SplineAddSin( cmd->index, cmd->phase.x, cmd->phase.y, cmd->phase.z, cmd->offset.x, cmd->offset.y, cmd->offset.z );
			break;
		case BEAMCMD_SplineAddSinTim:
			beam->SplineAddSinTime( cmd->index, cmd->phase.x, cmd->phase.y, cmd->phase.z, cmd->offset.x, cmd->offset.y, cmd->offset.z );
			break;
		case BEAMCMD_SplineAddSinTimeScaled:
			beam->SplineAddSinTimeScaled( cmd->index, cmd->phase.x, cmd->phase.y, cmd->phase.z, cmd->offset.x, cmd->offset.y, cmd->offset.z );
			break;
		case BEAMCMD_ConvertSplineToNodes:
			beam->ConvertSplineToNodes();
			break;
		case BEAMCMD_NodeLinearToTarget:
			beam->NodeLinearToTarget();
			break;
		case BEAMCMD_NodeElectric:
			beam->NodeElectric( cmd->offset.x, cmd->offset.y, cmd->offset.z, true );
			break;
		}
	}	
}

//================
//hhBeamSystem::Save
//================
void hhBeamSystem::Save( idSaveGame *savefile ) const {
	savefile->WriteString( declName );
	savefile->WriteInt( random.GetSeed() );
	savefile->WriteFloat( beamTime );

	for( int i = 0; i < decl->numBeams; i++) {
		beamList[i].Save( savefile );
	}

	savefile->WriteMat3( beamAxis );
	savefile->WriteVec3( arcVector );
	savefile->WriteVec3( targetLocation );
	targetEntity.Save( savefile );
	savefile->WriteVec3( targetEntityOffset );
	savefile->WriteInt( targetEntityId );
	savefile->WriteFloat( beamLength );
	savefile->WriteBool( bRigidBeamLength );
	savefile->WriteFloat( phaseScale );
	savefile->WriteFloat( offsetScale );
	savefile->WriteBool( bActive );
}

//================
//hhBeamSystem::Restore
//================
void hhBeamSystem::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( declName );

	// Initialize beam data from the beam file
	decl = declManager->FindBeam( declName );
	HH_ASSERT( renderEntity.declBeam == decl );

	// Initialize the random number generator
	int seed;
	savefile->ReadInt( seed );
	random.SetSeed( seed );

	savefile->ReadFloat( beamTime );

	// Allocate the beams
	if( beamList ) {
		delete[] beamList;
	}
	beamList = new hhBeam[decl->numBeams];

	for( int i = 0; i < decl->numBeams; i++) {
		beamList[i].Restore( savefile, this, &renderEntity.beamNodes[i]);
	}

	savefile->ReadMat3( beamAxis );
	savefile->ReadVec3( arcVector );
	savefile->ReadVec3( targetLocation );
	targetEntity.Restore( savefile );
	savefile->ReadVec3( targetEntityOffset );
	savefile->ReadInt( targetEntityId );
	savefile->ReadFloat( beamLength );
	savefile->ReadBool( bRigidBeamLength );
	savefile->ReadFloat( phaseScale );
	savefile->ReadFloat( offsetScale );
	savefile->ReadBool( bActive );

	Activate(bActive);
}

