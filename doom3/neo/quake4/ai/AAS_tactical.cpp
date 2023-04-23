///////////////////////////////////////////////////////////////////////////////////
// rvAAS_tactical.cpp
//
// AAST Tactical Search Parameters Documentation
// ---------------------------------------------
// There are 10 tests at your disposal when creating search functions, 3 
// sets of 3 and one special case test.  Each of these 10 tests can have 
// hard min and max limits, as well as a "soft" weight which will determine how
// highly the feature is ranked against other features.  Let's examine a
// diagram of what these 10 tests are:
//
//         [F]    #########     Key:
//          |     #########      [F] = Focus Test Set (enemy, or forward projection)
//         /      #########      [O] = Owner Test Set (actor who is doing the search)
//       [P]      #########      [P] = Path Test Set (line between owner and focus)
//        |       #########      <-> = Advance Test (In front / behind owner)
//       /        x   #####       x  = Feature (feature being tested) 
// <---[O]--->        #####      ### = Walls
//     ####################
//     ####################
//     ####################
//
// Each Test Set Has The Following:
// - Distance		RANGE=[ 0, 1]		WEIGHT=(-1=Close, 1=Far)
//		Distance is computed from the origin of the subject to the origin of the
//		feature being tested (x in the above diagram).  Distance will be a common
//		test to use on all three sets, with all mannor of clamped ranges and
//		sort values.  Most common though will be to sort close to the owner so
//		as to minimize how long it will take for the AI to get to the spot.
//
// - FacingDot		RANGE=[-1, 1]		WEIGHT=(-1=Behind, 1=In Front)
//		FacingDot is computed with the dotproduct of the subject's facing and
//		the feature's normal.  This too will be a common test to compare with 
//		all three subjects.  With it you can prefer points in front or behind
//		things.
//
// - DirectionDot	RANGE=[-1, 1]		WEIGHT=(-1=Toward, 1=Away)
//		DirectionDot is computed by subtracing the origins of the feature and
//		the subject, normalizing and taking the dotproduct of the result with
//		the feature's normal.  Usually, you will want a negative weight on this
//		test to comare how close the feature is pointing at the subject (usually
//		tested against Focus)
//
// Lastly, there is a special "additional" test for "advance":
// - Advance		RANGE=[-1, 1]		WEIGHT=(-1=Backward, 1=Forward)
//		Advance is computed using the owner direction dot with the path facing.
//		This test will tell you how much the feature is between the owner and
//		the focus, reguardless of what direction either is facing at the time.
//		As a result, it approximates "advancing".
//		
///////////////////////////////////////////////////////////////////////////////////
#include "../../idlib/precompiled.h"
#pragma hdrstop


///////////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////////
#include "../Game_local.h"
#include "AI.h"
#include "AI_Manager.h"
#include "AI_Util.h"
#include "AAS_local.h"
#include "AAS_tactical.h"

///////////////////////////////////////////////////////////////////////////////////
// Global::CONTANER SIZES AND OTHER LIMITS
///////////////////////////////////////////////////////////////////////////////////
const int							MAX_FEATURE_LIST	= 20;
const int							MAX_AREAS_TOUCHED	= 450;

///////////////////////////////////////////////////////////////////////////////////
// Global::FEATURE TESTING DISTANCES
///////////////////////////////////////////////////////////////////////////////////
const float							MAX_DISTANCE		= 650.0f;
const float							MIN_DISTANCE		= 72.0f;
const float							MIN_NEAR_DISTANCE	= 112.0f;
const float							FROM_ENEMY_PROJECT	= 350.0f;
const float							TEST_TEAMMATE_DIST	= 150.0f;




///////////////////////////////////////////////////////////////////////////////
// aasFeature_s::Normal
///////////////////////////////////////////////////////////////////////////////
idVec3&	aasFeature_s::Normal()
{
	static idVec3 n(0,0,0);
	n[0] = ((float)(normalx) / 127.0f) - 1.0f;
	n[1] = ((float)(normaly) / 127.0f) - 1.0f;
	return n;
}

///////////////////////////////////////////////////////////////////////////////
// aasFeature_s::Origin()
///////////////////////////////////////////////////////////////////////////////
idVec3& aasFeature_s::Origin()
{
	static idVec3 o;
	o.Set((float)x, (float)y, (float)z);
	return o;
}

///////////////////////////////////////////////////////////////////////////////
// aasFeature_s::GetLookPos()
///////////////////////////////////////////////////////////////////////////////
int aasFeature_s::GetLookPos( idVec3& lookPos,  const idVec3& aimAtOrigin, const float leanDistance )
{
	static idVec3		up(0.0f,0.0f,1.0f);
	static idVec3		direction;
	static idVec3		right;
	static float		rightDot;
	static float		distance;

	lookPos		 = Origin();
	lookPos[2]	+= height - leanDistance;
	direction	 = aimAtOrigin - lookPos;
	distance	 = direction.NormalizeFast();
	right		 = Normal().Cross(up);	
	rightDot	 = right * direction;


	// Check For Optimal Conditions
	//------------------------------
	if (flags&FEATURE_LOOK_OVER && fabsf(rightDot)<0.2f) 
	{
		lookPos[2] += leanDistance*2.0f;				// CDR_TODO: Hard coded numbers make me sad
		return FEATURE_LOOK_OVER;
	}

	if (flags&FEATURE_LOOK_RIGHT && rightDot>0.0f) 
	{
		lookPos += right * leanDistance;
		return FEATURE_LOOK_RIGHT;
	}

	if (flags&FEATURE_LOOK_LEFT && rightDot<0.0f) 
	{
		lookPos -= right * leanDistance;
		return FEATURE_LOOK_LEFT;
	}


	// So Nothing Matches Perfectly, Let's Try Fallback Cases In This Order
	//----------------------------------------------------------------------
	if (flags&FEATURE_LOOK_OVER)
	{
		lookPos[2] += leanDistance*2.0f;				// CDR_TODO: Hard coded numbers make me sad
		return FEATURE_LOOK_OVER;
	}

	if (flags&FEATURE_LOOK_RIGHT)
	{
		lookPos += right * leanDistance;
		return FEATURE_LOOK_RIGHT;
	}

	if (flags&FEATURE_LOOK_LEFT)
	{
		lookPos -= right * leanDistance;
		return FEATURE_LOOK_LEFT;
	}

	// This Is Odd, There Must Be No Look Flags On This Feature At All
	//-----------------------------------------------------------------
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// rvSortReach
//
// This structure is used by the search heap below to sort areas by actual
// distance from the start point to do a distance based BFS instead of a
// least links based BFS.
///////////////////////////////////////////////////////////////////////////////
struct rvSortReach
{
	int				mAreaNum;
	idReachability*	mReach;
	float			mDistance;

	bool		operator<(const rvSortReach& r) const
	{
		return (mDistance>r.mDistance);
	}
};

///////////////////////////////////////////////////////////////////////////////
// rvTest
//
// A test is a mechanism to weight values and to provide min and max bounds.
///////////////////////////////////////////////////////////////////////////////
struct rvTest
{
	float					mMin;
	float					mMax;
	float					mWeight;
	float					mValue;

	///////////////////////////////////////////////////////////////////////////
	// Save
	///////////////////////////////////////////////////////////////////////////
	void			Save(idSaveGame *savefile)
	{
		savefile->WriteFloat(mMin);
		savefile->WriteFloat(mMax);
		savefile->WriteFloat(mWeight);
	}

	///////////////////////////////////////////////////////////////////////////
	// Restore
	///////////////////////////////////////////////////////////////////////////
	void			Restore(idRestoreGame *savefile)
	{
		savefile->ReadFloat(mMin);
		savefile->ReadFloat(mMax);
		savefile->ReadFloat(mWeight);
	}

	///////////////////////////////////////////////////////////////////////////
	// Reset - Sets the values to defaults
	///////////////////////////////////////////////////////////////////////////
	void			Reset()
	{
		mMax	=  1.0f;
		mMin	= -1.0f;
		mWeight	=  0.0f;
		mValue	=  0.0f;
	}

	///////////////////////////////////////////////////////////////////////////
	// Weight - Simple compute weight
	///////////////////////////////////////////////////////////////////////////
	float			Weight()
	{
		return mValue * mWeight;
	}

	///////////////////////////////////////////////////////////////////////////
	// Test - Records the value out of the range and returns true if succeeded
	///////////////////////////////////////////////////////////////////////////
	bool			Test(float Value, float MaxScale=1.0f)	
	{
		if (mMin>-1.0f || mMax<1.0f || mWeight!=0.0f)
		{
			mValue = Value;
			if (MaxScale!=1.0f)
			{
				mValue /= MaxScale;
			}
			mValue = (mValue-mMin) / (mMax-mMin);	// Now Scale It [0.0, 1.0] Of Min And Max
			return (mValue>=0.0f && mValue<=1.0f);	// If Not In [0.0, 1.0], Test Fails
		}
		return true;	// Test Is Not Active
	}

	///////////////////////////////////////////////////////////////////////////
	// WeightRange - Returns the abs of the weight, and adds to negatives
	///////////////////////////////////////////////////////////////////////////
	float			WeightRange(float& negatives)
	{
		if (mWeight<0.0f)
		{
			negatives += mWeight;
		}
		return fabsf(mWeight);
	}

	void			DrawDebugInfo(const idVec4 color, const idVec3& origin, const idVec3& direction);
	void			DrawDebugInfo(const idVec4 color, const idVec3& origin);
};

///////////////////////////////////////////////////////////////////////////////
// rvTestSet
//
// Test sets are designed to be used by the rvAASTacticalSensorLocal class to compute
// various vectors against a given feature.
///////////////////////////////////////////////////////////////////////////////
struct rvTestSet
{
	// Parameters
	//------------
	bool					mProjectOrigin;		// If True, Origin Is Cast Out Along mFacing Vector
	idVec3					mOrigin;			// Position Of The Test Subject
	idVec3					mFacing;			// Orientation Of The Test Subject

	// Computed During Test()
	//------------------------
	rvTest					mDistance;			// Resulting Distance To Feature
	rvTest					mFacingDot;			// Resulting Facing Dot Product To Feature
	rvTest					mDirectionDot;		// Resulting Direction Dot Product To Feature
	idVec3					mDirection;			// Resulting Direction To Feature

	///////////////////////////////////////////////////////////////////////////
	// Constructor
	///////////////////////////////////////////////////////////////////////////
	rvTestSet( void )
		: mOrigin(0,0,0), mFacing(0,0,0), mProjectOrigin(false)
	{
	}

	///////////////////////////////////////////////////////////////////////////
	// Save
	///////////////////////////////////////////////////////////////////////////
	void			Save(idSaveGame *savefile)
	{
		savefile->WriteBool(mProjectOrigin);
		savefile->WriteVec3(mOrigin);
		savefile->WriteVec3(mFacing);
		mDistance.Save(savefile);
		mFacingDot.Save(savefile);
		mDirectionDot.Save(savefile);
	}

	///////////////////////////////////////////////////////////////////////////
	// Restore
	///////////////////////////////////////////////////////////////////////////
	void			Restore(idRestoreGame *savefile)
	{
		savefile->ReadBool(mProjectOrigin);
		savefile->ReadVec3(mOrigin);
		savefile->ReadVec3(mFacing);
		mDistance.Restore(savefile);
		mFacingDot.Restore(savefile);
		mDirectionDot.Restore(savefile);
	}


	///////////////////////////////////////////////////////////////////////////
	// Reset - Restets all sub tests
	///////////////////////////////////////////////////////////////////////////
	void			Reset()
	{
		mProjectOrigin	= false;
		mDirectionDot.Reset();
		mFacingDot.Reset();
		mDistance.Reset();
// bdube: Had to comment this out becaue it would break the test function which checks for non defaults 
//		mDistance.mMin = 0.0f;	// special case (default would be -1.0f because all others are dot products)
	}

	///////////////////////////////////////////////////////////////////////////
	// Weight - Adds up the weights of the sub tests
	///////////////////////////////////////////////////////////////////////////
	float			Weight()
	{
		return (mDistance.Weight() + mFacingDot.Weight() + mDirectionDot.Weight());
	}

	///////////////////////////////////////////////////////////////////////////
	// WeightRange - Computes weight range of all sub tests
	///////////////////////////////////////////////////////////////////////////
	float			WeightRange(float& negatives)
	{
		return (mDistance.WeightRange(negatives) + mFacingDot.WeightRange(negatives) + mDirectionDot.WeightRange(negatives));
	}

	///////////////////////////////////////////////////////////////////////////
	// Test - This is the actuall feature test
	///////////////////////////////////////////////////////////////////////////
	bool			Test(aasFeature_t* f, float distance=0.0f)
	{
		// First Compute The Direction
		//-----------------------------
		mDirection		= f->Origin() - mOrigin;

		// If Project Origin Is True, Then Move Origin Along Facing Vector
		//-----------------------------------------------------------------
		if (mProjectOrigin)
		{
			mDirection.ProjectOntoVector(mFacing);
			mOrigin	   += mDirection;
			mDirection	= f->Origin() - mOrigin;
		}

		// If No Override On Distance, Compute It By Normalizing The Direction
		//---------------------------------------------------------------------
		if (!distance)
		{
			distance = mDirection.Normalize();
		}
		else
		{
			mDirection.Normalize();
		}


		// THE DISTANCE TEST
		//-------------------
		if (!mDistance.Test(distance, MAX_DISTANCE))
		{
			return false;
		}

		// THE FACING TEST
		//-----------------
		if (!mFacingDot.Test(f->Normal()*mFacing))
		{
			return false;
		}

		// THE DIRECTION TEST
		//--------------------
		if (!mDirectionDot.Test(f->Normal()*mDirection))
		{
			return false;
		}
		return true;
	}		

	///////////////////////////////////////////////////////////////////////////
	// SetupOriginAndFacing - Called For Each Test Set
	///////////////////////////////////////////////////////////////////////////
	void			SetupOriginAndFacing(const idEntity* ent, const idVec3* originOverride=0, const idVec3* facingOverride=0)
	{
		if (!ent)
		{
			mOrigin		= (originOverride)?(*originOverride):(vec3_origin);
			mFacing		= (facingOverride)?(*facingOverride):(vec3_origin);
		} 
		else 
		{
			const idActor* entActor = dynamic_cast<const idActor*>(ent);	// bleh.  Base entity class should properly return origin, angles, and forward vector

			mOrigin		= (originOverride)?(*originOverride):(ent->GetPhysics()->GetOrigin());
			mFacing		= (facingOverride)?(*facingOverride):(entActor?entActor->viewAxis[0]:ent->GetPhysics()->GetAxis(0)[0]);
		}
			
		mFacing[2]	= 0;
		mFacing.Normalize();
	}

	///////////////////////////////////////////////////////////////////////////
	// ProjectOriginForward - Used By Several Setup Options To Project Origin
	//   Along Facing Direction Some
	///////////////////////////////////////////////////////////////////////////
	void			ProjectOriginForward(float distance, float xyRange=0.0f, bool randomFacing=0.0f)
	{
		mOrigin			+= (mFacing * distance);
		if (xyRange)
		{
			mOrigin[0]		+= rvRandom::flrand(-xyRange, xyRange);
			mOrigin[1]		+= rvRandom::flrand(-xyRange, xyRange);
		}
		if (randomFacing)
		{
			mFacing[0]		 = rvRandom::flrand(-1.0f, 1.0f);
			mFacing[1]		 = rvRandom::flrand(-1.0f, 1.0f);
		}
		mFacing[2]		 = 0.0f;
		mFacing.Normalize();
	}

	void			DrawDebugInfo(const idVec4& color, const idVec3& nonProjectedOrigin);
};





///////////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal
//
// This is the local implimentation of the rvAASTacticalSensor interface.  It 
// contains all the various local data and functions needed to execute a 
// search of the tactical data in AAS.
///////////////////////////////////////////////////////////////////////////////////
struct rvAASTacticalSensorLocal : rvAASTacticalSensor
{
	// Owner
	///////////////////////////////////////////////////////////////////////////////
	idActor*				mOwner;					// Owner Of The Sensor
	idAI*					mOwnerAI;				// Owner As Already Cast To AI Type

	// Search Parameters
	///////////////////////////////////////////////////////////////////////////////
	idStr					mSearchName;			// Current Search Name
	int						mFlagsMatchAny;			// Features must match AT LEAST ONE of these flags
	int						mFlagsMatchAll;			// Features must match ALL of these flags
	int						mFlagsMatchNone;		// Features must match NONE of these flags
	int						mFeaturesSearchMax;		// Maximum number of features to extract from the grid
	int						mFeaturesFinalMax;		// After sorting, prune list down to this size
	rvTestSet				mFromOwner;				// Test Set For Owner Relation
	rvTestSet				mFromEnemy;				// Test Set For Focus Relation
	rvTestSet				mFromTether;			// Test Set for Tether Relation
	rvTestSet				mFromPath;				// Test Set For Path Relation
	rvTest					mAdvance;				// Single Test For Advance / Retreat
	rvTest					mAssignment;			// Single Test For Assignment Direction Dot Product
 	rvTest					mLeanNormal;			// Single Test For Lean Normal Biasing
	idVec3					mAssignmentDirection;	// Used By Assignment Test
	bool					mAssignmentValid;		// Turns On And Off The Assignment Test
	idEntityPtr<idEntity>	mEnemyOverride;			// Overrides Enemy Pointer To Any Entity

	// Search & Update Results
	///////////////////////////////////////////////////////////////////////////////
	idList<aasFeature_t*>	mFeatures;				// The list of all features found in the most recent search
	idVec3					mReservedOrigin;		// Origin of feature that is currently reserved
	aasFeature_t*			mReserved;				// Which feature is currently reserved
	aasFeature_t*			mNear;					// Which feature is closest
	aasFeature_t*			mLook;					// Which feature to look down
	int						mLookStartTime;
	float					mLookStopDist;





	// Local API
	///////////////////////////////////////////////////////////////////////////////
	rvAASTacticalSensorLocal();
	~rvAASTacticalSensorLocal();
	void			Update();
	void			Save(idSaveGame *savefile);
	void			Restore(idRestoreGame *savefile);
	void			Clear();
	void			DrawDebugInfo();

	// Search
	///////////////////////////////////////////////////////////////////////////////
	void			Search();
	void			SearchReset(idEntity* enemyOverride=0, float ownerRangeMin=0.0f, float ownerRangeMax=1.0f);
	void			SearchRadius(const idVec3& origin=vec3_origin, float rangeMin=0.0f, float rangeMax=1.0f);
	void			SearchCover(float rangeMin=0.0f, float rangeMax=1.0f);
	void			SearchHide(idEntity* from=0);
	void			SearchFlank();
	void			SearchAdvance();
	void			SearchRetreat();
	void			SearchAmbush();
	void			SearchDebug();
	float			SearchComputeWeightRange(float& rangeNegative);
	float			SearchComputeWeight();

	// Feature Testing
	///////////////////////////////////////////////////////////////////////////////
	void			TestSetupCurrentValues();
	bool			TestValid(aasFeature_t* f, float walkDistanceToFeature);
	bool			TestValidWithCurrentState(aasFeature_t* f=0);

	// Feature Reservation
	///////////////////////////////////////////////////////////////////////////////
	void			Reserve(aasFeature_t* f);

	// Access To Results
	///////////////////////////////////////////////////////////////////////////////
	int				FeatureCount()			{return mFeatures.Num();}
	aasFeature_t*	Feature(int i)			{return mFeatures[i];}
	aasFeature_t*	Near() const			{return mNear;}
	aasFeature_t*	Look() const			{return mLook;}
	aasFeature_t*	Reserved() const		{return mReserved;}
	const idVec3&	ReservedOrigin() const	{return mReservedOrigin;}
};

///////////////////////////////////////////////////////////////////////////////
// Global::Objects
///////////////////////////////////////////////////////////////////////////////
rvAASTacticalSensorLocal*			mSensor;
float								mDebugRadius;

///////////////////////////////////////////////////////////////////////////////
// Global::Typedefines
///////////////////////////////////////////////////////////////////////////////
typedef	idEntityPtr<idEntity>		TEntPtr;
typedef	aasFeature_t*				TFeaturePtr;



///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensor::CREATE_SENSOR
///////////////////////////////////////////////////////////////////////////////
rvAASTacticalSensor* rvAASTacticalSensor::CREATE_SENSOR(idActor* owner)
{
	rvAASTacticalSensorLocal* nSensor = new rvAASTacticalSensorLocal();
	nSensor->mOwner		= owner;
	nSensor->mOwnerAI	= dynamic_cast<idAI*>(owner);
	return nSensor;
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal
///////////////////////////////////////////////////////////////////////////////
rvAASTacticalSensorLocal::rvAASTacticalSensorLocal()
{
	mOwner					= 0;
	mOwnerAI				= 0;
	Clear();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Destructor
///////////////////////////////////////////////////////////////////////////////
rvAASTacticalSensorLocal::~rvAASTacticalSensorLocal()
{
	Reserve(0);
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Clear
///////////////////////////////////////////////////////////////////////////////
void			rvAASTacticalSensorLocal::Clear()
{
	mReserved				= 0;
	mNear					= 0;
	mLook					= 0;
	mSearchName				= "";
	mFeatures.Clear();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Save
///////////////////////////////////////////////////////////////////////////////
void	rvAASTacticalSensorLocal::Save(idSaveGame *savefile)
{
	savefile->WriteString(mSearchName);
	savefile->WriteInt(mFlagsMatchAny);
	savefile->WriteInt(mFlagsMatchAll);
	savefile->WriteInt(mFlagsMatchNone);
	savefile->WriteInt(mFeaturesSearchMax);
	savefile->WriteInt(mFeaturesFinalMax);
	mFromOwner.Save(savefile);
	mFromEnemy.Save(savefile);
	mFromTether.Save(savefile);
	mFromPath.Save(savefile);
	mAdvance.Save(savefile);
	mAssignment.Save(savefile);
	mLeanNormal.Save(savefile);
	savefile->WriteVec3(mAssignmentDirection);
	savefile->WriteBool(mAssignmentValid);
	mEnemyOverride.Save(savefile);

// cnicholson: NOTE: The following 4 vars are set to 0 / cleared in the restore, so don't save them.
	// NOSAVE: idList<aasFeature_t*>	mFeatures;
	// NOSAVE: savefile->WriteVec3(mFeatures);
	// NOSAVE: aasFeature_t*			mReserved;
	// NOSAVE: aasFeature_t*			mNear;
	// NOSAVE: aasFeature_t*			mLook;
	savefile->WriteInt(mLookStartTime);	// cnicholson: Added unsaved var
	savefile->WriteFloat(mLookStopDist);// cnicholson: Added unsaved var
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Restore
///////////////////////////////////////////////////////////////////////////////
void	rvAASTacticalSensorLocal::Restore(idRestoreGame *savefile)
{
	// Clear Old Data
	//----------------
	mFeatures.Clear();
	mNear		= 0;
	mLook		= 0;
	mReserved	= 0;

	// Read The Save File Search Parameters
	//--------------------------------------
	savefile->ReadString(mSearchName);
	savefile->ReadInt(mFlagsMatchAny);
	savefile->ReadInt(mFlagsMatchAll);
	savefile->ReadInt(mFlagsMatchNone);
	savefile->ReadInt(mFeaturesSearchMax);
	savefile->ReadInt(mFeaturesFinalMax);
	mFromOwner.Restore(savefile);
	mFromEnemy.Restore(savefile);
	mFromTether.Restore(savefile);
	mFromPath.Restore(savefile);
	mAdvance.Restore(savefile);
	mAssignment.Restore(savefile);
	mLeanNormal.Restore(savefile);
	savefile->ReadVec3(mAssignmentDirection);
	savefile->ReadBool(mAssignmentValid);
	mEnemyOverride.Restore(savefile);
//	Search();

	savefile->ReadInt(mLookStartTime); // cnicholson: Added unrestored var
	savefile->ReadFloat(mLookStopDist);// cnicholson: Added unrestored var
}


///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Update
//
// If called regularly, this function will handle drawing debug information
///////////////////////////////////////////////////////////////////////////////
void	rvAASTacticalSensorLocal::Update()
{
	idAAS* aas = (mOwnerAI)?(mOwnerAI->aas):(gameLocal.GetAAS(0));
	if (!aas || !aas->GetFile() || !aas->GetFile()->GetNumFeatures() || !mOwner || mOwner->IsHidden())
	{
		return;
	}

	idAASFile* file = aas->GetFile();

	idVec3			velocityFwd			= mOwner->GetPhysics()->GetLinearVelocity();
	const idVec3&	ownerOrigin			= mOwner->GetPhysics()->GetOrigin();
	int				ownerAreaNum		= mOwnerAI ? mOwnerAI->PointReachableAreaNum ( ownerOrigin ) : aas->PointReachableAreaNum(ownerOrigin, mOwner->GetPhysics()->GetBounds(), (AREA_REACHABLE_WALK|AREA_REACHABLE_FLY) );
	aasFeature_t*	feature				= 0;
	aasArea_t&		area				= file->GetArea(ownerAreaNum);
	idActor*		teammate			= NULL;
	float			featureDistance		= 0.0f;
	float			featureDotLeft		= 0.0f;
	float			featureDotDirection	= 0.0f;
	float			teammateDistance	= 0.0f;
	float			closestDistance		= 0.0f;
	idVec3			velocityLeft;
	idVec3			velocityDown;
	idVec3			featureDirection;
	idVec3			teammateDirection;



	// Update And Possibly Clear The Near Feature
	//--------------------------------------------
	if (mNear)
	{
		closestDistance = mNear->Origin().Dist(ownerOrigin);
		if (closestDistance>MIN_NEAR_DISTANCE)
		{
			mNear = 0;
		}
	}


	// Search For Features In This Area That Are Close To Owner Origin
	//-----------------------------------------------------------------
	if (area.numFeatures)
	{
		for (int areaFeatureNum=0; areaFeatureNum<area.numFeatures; areaFeatureNum++)
		{
			feature				 = & (file->GetFeature(file->GetFeatureIndex(area.firstFeature+areaFeatureNum)));
			if (feature!=mNear)
			{
				featureDirection	 = feature->Origin();
				featureDirection	-= ownerOrigin;
				featureDistance		 = featureDirection.NormalizeFast();

				if (featureDistance<MIN_NEAR_DISTANCE && (!mNear || featureDistance<closestDistance))
				{
					mNear			 = feature;
					closestDistance	 = featureDistance;
				}
			}
		}
	}

	if (velocityFwd.LengthSqr()>1.0f)
	{
		// Compute Velocity Vectors
		//--------------------------
		velocityFwd.NormalizeFast();
		velocityFwd.NormalVectors(velocityLeft, velocityDown);


		// Check If We Should Clear The Look Feature
		//-------------------------------------------
		if (mLook)
		{
			const idVec3& featureOrigin = mLook->Origin();
			const idVec3& featureNormal = mLook->Normal();

			// Too Far?
			//----------
			if (featureOrigin.Dist(ownerOrigin)>mLookStopDist)
			{
				mLook = 0;
			}

			// Check All Team Mates To See If This Look Points At Them
			//---------------------------------------------------------
			for (teammate = aiManager.GetAllyTeam ( (aiTeam_t)mOwner->team ); teammate; teammate = teammate->teamNode.Next())
			{
				if (teammate->fl.hidden || teammate == mOwner || teammate->health <= 0) 
				{
					continue;
				}

				teammateDirection	= featureOrigin - teammate->GetPhysics()->GetOrigin();
				teammateDistance	= teammateDirection.NormalizeFast();
				if (teammateDistance<TEST_TEAMMATE_DIST && teammateDirection*featureNormal>0.85f)
				{
					mLook = 0;
					break;
				}
			}
			teammate = NULL;
		}

		// And, If We Are Moving, Check The Near Feature To See If It Qualifies As A Look Feature
		//----------------------------------------------------------------------------------------
		if (mNear && mNear!=mLook && (gameLocal.GetTime() - mLookStartTime)>3000)
		{
			const idVec3& featureOrigin = mNear->Origin();
			const idVec3& featureNormal = mNear->Normal();

			// Compute Feature Direction
			//---------------------------
			featureDirection	 = featureOrigin;
			featureDirection	-= ownerOrigin;
			featureDistance		 = featureDirection.NormalizeFast();

			// Must Be Behind Me (I've Alreay Walked Past It)
			//------------------------------------------------
			if (featureDistance>16.0f && featureDirection*velocityFwd<0.0f)
			{
				// Must Be Facing Away From Me
				//-----------------------------
				featureDotDirection = featureNormal*featureDirection;
				if (featureDotDirection>-0.8f)
				{
					// Must Be Roughly Perpendicular To My Velocity (Within 45 Degrees)
					//------------------------------------------------------------------
					featureDotLeft = featureNormal*velocityLeft;
					if (fabsf(featureDotLeft)>0.5f)
					{
						// If On Left Of Me, Must Have A Right Lean, And Converse
						//--------------------------------------------------------
						if ((featureDotLeft>0.0f && mNear->flags&FEATURE_LOOK_RIGHT) || 
							(featureDotLeft<0.0f && mNear->flags&FEATURE_LOOK_LEFT))
						{
							// Check All Team Mates To See If This Look Points At Them
							//---------------------------------------------------------
							for (teammate = aiManager.GetAllyTeam ( (aiTeam_t)mOwner->team ); teammate; teammate = teammate->teamNode.Next())
							{
								if (teammate->fl.hidden || teammate == mOwner || teammate->health <= 0) 
								{
									continue;
								}

								teammateDirection	= featureOrigin - teammate->GetPhysics()->GetOrigin();
								teammateDistance	= teammateDirection.NormalizeFast();
								if (teammateDistance<TEST_TEAMMATE_DIST && teammateDirection*featureNormal>0.85f)
								{
									break;
								}
							}

							if (!teammate)
							{
								mLook			 = mNear;
								mLookStartTime	 = gameLocal.GetTime();
								mLookStopDist	 = rvRandom::flrand(48.0f, 80.0f);
							}
						}
					}
				}
			}
		}
	}
	else if (!mOwnerAI || !mOwnerAI->move.fl.moving)
	{
		mLook = 0;
	}

	// Draw Any Debug Info
	//---------------------
	DrawDebugInfo();
}


///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Reserve
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::Reserve(aasFeature_t* f)
{
	if (f!=mReserved && mOwner)
	{
		mReserved = f;
		
		if ( f ) 
		{
			mReservedOrigin = f->Origin ( );
		}	
	}
}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FEATURE TESTING
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// TestSetupCurrentValuesFor
//
// This function sets up the test sets to have new
///////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::TestSetupCurrentValues()
{
	// Owner Test Set
	//----------------
	mFromOwner.SetupOriginAndFacing(mOwner);


	// Enemy Test Set
	//----------------
	//NOTE!!!  This does NOT clear any old info about your enemy, so if any tests
	//			use this info ASSUMING you have an enemy, your test will be totally
	//			wrong!!!
	if (mOwnerAI && mEnemyOverride)
	{
		mFromEnemy.SetupOriginAndFacing(mEnemyOverride, &mOwnerAI->LastKnownPosition(mEnemyOverride), 0);
	}
	else if (mOwnerAI && mOwnerAI->GetEnemy())
	{
		mFromEnemy.SetupOriginAndFacing(mOwnerAI->GetEnemy(), &mOwnerAI->LastKnownPosition(mOwnerAI->GetEnemy()), 0);
	}

	// Tether Test Set
	//----------------
	if (mOwnerAI && mOwnerAI->IsTethered ( ) )
	{
		mFromTether.SetupOriginAndFacing(mOwnerAI->GetTether ( ));
	}

	// Path Test Set
	//----------------
	mFromPath.mProjectOrigin	= true;
	mFromPath.mOrigin			= mFromOwner.mOrigin;
	mFromPath.mFacing			= mFromEnemy.mOrigin - mFromOwner.mOrigin;
	mFromPath.mFacing[2]		= 0;
	mFromPath.mFacing.Normalize();

	// Advance Test Set
	//------------------
	// NOTHING TO DO HERE FOR NOW...
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::TestValidReserved
///////////////////////////////////////////////////////////////////////////////
bool rvAASTacticalSensorLocal::TestValidWithCurrentState(aasFeature_t* f)
{
	// If Trying To Hide From An Enemy That No Longer Exists, Any Feature is Invalid
	//-------------------------------------------------------------------------------
	if (mEnemyOverride.GetSpawnId() && !mEnemyOverride.IsValid())
	{
		return false;
	}

	// Reset The Test Parameters With Current Origins (Cuz Things May Have Moved)
	//----------------------------------------------------------------------------
	TestSetupCurrentValues();

	// And Run The Test That The Original Search Ran
	//-----------------------------------------------
	if (!f)
	{
		return TestValid(mReserved, 0.0f);
	}
	return TestValid(f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::TestValid
//
// This is THE function that tests if a given feature matches the current
// search parameters.  An optional walk distance may be passed into the function
// to replace the from owner straight line distance.
///////////////////////////////////////////////////////////////////////////////
bool rvAASTacticalSensorLocal::TestValid(aasFeature_t* f, float walkDistanceToFeature)
{
 	static idVec3 LeanNormal;
 	static idVec3 Up(0.0f,0.0f,1.0f);
 	static float  LeanNormalDot;


	// Is There A Feature At All
	//---------------------------
	if (!f)
	{
		return false;
	}

	// Does It Match The Flags?
	//--------------------------
	if (!(f->flags&mFlagsMatchAny) || 
		((f->flags&mFlagsMatchAll)!=mFlagsMatchAll) || 
 		 (f->flags&mFlagsMatchNone))
	{
		return false;
	}

	// Does It Pass The Tests?
	//-------------------------
	if (!mFromOwner.Test(f, walkDistanceToFeature) ||
		!mFromEnemy.Test(f) ||
		!mFromTether.Test(f) ||
		!mFromPath.Test(f) ||
		!mAdvance.Test(mFromOwner.mDirection*mFromPath.mFacing) ||
		!mAssignment.Test(mFromOwner.mDirection*mAssignmentDirection))
	{
		return false;
	}

	// Make sure this cover point is vaild for the current tether
	//------------------------------------------------------------
	if ( mOwnerAI && mOwnerAI->IsTethered() && !mOwnerAI->GetTether()->ValidateDestination ( mOwnerAI, f->Origin() ) )
	{
		return false;
	}

 	// Does It Pass The Lean Normal Test?
 	//------------------------------------
	if (mOwnerAI && (mEnemyOverride||mOwnerAI->GetEnemy()))
	{//we have an enemy position to test against
		mLeanNormal.mValue = 0.0f;
		if (!(f->flags&FEATURE_LOOK_OVER) && ((f->flags&FEATURE_LOOK_RIGHT) || (f->flags&FEATURE_LOOK_LEFT)))
		{
 			LeanNormal		= f->Normal().Cross(Up);	// Start With Left
 			LeanNormalDot	= LeanNormal * mFromEnemy.mDirection;

 			if (!(f->flags&FEATURE_LOOK_LEFT) || ((f->flags&FEATURE_LOOK_RIGHT) && LeanNormalDot<0.0f))
 			{
 				LeanNormalDot *= -1.0f;					// Use The Right Normal
 			}

 			if (!mLeanNormal.Test(LeanNormalDot))
 			{
 				return false;
			}
		}
	}


	// Is Anyone Else Going There?
	//-----------------------------
	if (mOwnerAI && !aiManager.ValidateDestination(mOwnerAI, f->Origin())) 
	{
		return false;
	}

	// Everything Passed, This Feature Is Good To Go
	//-----------------------------------------------
	return true;
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SEARCH PARAMETERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// SearchReset
//
// Initialize Default Flags, Feature Counts, And Population Points.  This
// function must be called before first in any search function, because
// the search functions rely on this standard set of parameters and then
// build upon them.
///////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchReset(idEntity* enemyOverride, float ownerRangeMin, float ownerRangeMax)
{
	if (!mOwner)
	{
		return;
	}

	// Setup Base Search Parameters
	//------------------------------
	mFlagsMatchAll			= 0;															// Must Have All Of These
	mFlagsMatchAny			= (FEATURE_LOOK_LEFT|FEATURE_LOOK_RIGHT|FEATURE_LOOK_OVER);		// Must Have At Least One Of These
	mFlagsMatchNone			= (FEATURE_PINCH|FEATURE_VANTAGE);								// Don't Want Any Of These
	mFeaturesSearchMax		= 100;
	mFeaturesFinalMax		= 20;
	mAssignmentValid		= false;					// TODO: Turn This Back On
	mAssignmentDirection	= vec3_zero;
	mEnemyOverride			= enemyOverride;


	// Lean Normal Test
	//------------------
	mLeanNormal.Reset();
	mLeanNormal.mMin							=-0.2f;		// Must Lean Toward Enemy


	// Owner Test Set Default Values
	//-------------------------------
	mFromOwner.Reset();
	mFromOwner.mDistance.mMin				= ownerRangeMin;
	mFromOwner.mDistance.mMax				= ownerRangeMax;
	mFromOwner.mDistance.mWeight			=-1.0f;		// Prefer Close To Owner

	// Enemy Test Set Default Values
	//-------------------------------
	//NOTE!!!  This does NOT clear any old info about your enemy, so if any tests
	//			use this info ASSUMING you have an enemy, your test will be totally
	//			wrong!!!
	mFromEnemy.Reset();
	if ( mOwnerAI && mOwnerAI->enemy.ent )
	{
		mFromEnemy.mDistance.mMin				= mOwnerAI->combat.awareRange / MAX_DISTANCE;		// must be at least 100 units from enemy
		mFromEnemy.mDistance.mMax				= 2.0f;		// don't care how far the distance is to the enemy, let it go over max (up to 1600)
   		mFromEnemy.mDirectionDot.mMax			=-0.7f;		// Must Face Within 45 Degrees Of enemy
   		mFromEnemy.mDirectionDot.mWeight		=-0.3f;		// Prefer To Face Toward Enemy
		if (mOwnerAI && mOwnerAI->enemy.ent )
		{
			// Cap Min And Max Distances To Attack Range, and Aware Range
			//------------------------------------------------------------
			mFromEnemy.mDistance.mMax			= mOwnerAI->combat.attackRange[1] / MAX_DISTANCE;
			mFromEnemy.mDistance.mMin			= mOwnerAI->combat.attackRange[0] / MAX_DISTANCE;

			if (mFromEnemy.mDistance.mMin <		 (mOwnerAI->combat.awareRange / MAX_DISTANCE))
			{
				mFromEnemy.mDistance.mMin		= mOwnerAI->combat.awareRange / MAX_DISTANCE;
			}

			// If haven't seen enemy in a while, allow you to go right to his last known spot
			//--------------------------------------------------------------------------------
			if ( mOwnerAI->enemy.lastVisibleTime && (gameLocal.GetTime() - mOwnerAI->enemy.lastVisibleTime)>mOwnerAI->combat.maxLostVisTime/2.0f)
			{
				mFromEnemy.mDistance.mMin		= 0.0f;
			}
		}
	}


	// Tether test set
	//-------------------------------
	mFromTether.Reset();
	if (mOwnerAI && mOwnerAI->IsTethered ( ) ) 
	{
		mFromTether.mFacingDot.mMin = 0.5f;
		mFromTether.mFacingDot.mWeight = 0.3f; 
		
		// Disable the owner distance test
		mFromOwner.Reset();		
	}	

	// Other Test Sets
	//-----------------
	mFromPath.Reset();
	mAdvance.Reset();
	mAssignment.Reset();

	
	// Default Test Values
	//---------------------
	TestSetupCurrentValues();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchDebug
//
// TO RUN THIS FUNCTION, TYPE "extract_tactical" ON THE CONSOLE.  
//
//    Feel free to modify this function to test whatever search or other 
//    operation you need.  The owner will be the player.
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchDebug()
{
	SearchCover();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchRadius
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchRadius(const idVec3& origin, float rangeMin, float rangeMax)
{
	SearchReset(0, rangeMin, rangeMax);
	mSearchName							= "Radius";
	mFromEnemy.Reset();								// Don't Care About Enemy At All
	if ( origin != vec3_origin ) 
	{
		mFromOwner.mOrigin = origin;				// Override the owner origin
	}
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchCover
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchCover(float rangeMin, float rangeMax)
{
	SearchReset(0, rangeMin, rangeMax);
	mSearchName							= "Cover";
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchHide
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchHide(idEntity* from)
{
	SearchReset(from);
	mSearchName							= "Hide";
	mFlagsMatchNone						|= FEATURE_LOOK_OVER; // Want Full Height Walls Here
   	mFromEnemy.mDirectionDot.mMax		= -0.8f;	// Must Almost Exactly At The Enemy
	mFromOwner.mDistance.mMax			=  2.0f;	// Go as far as you need to - ignore tethering for hide
	mFromOwner.mDistance.mMin			=  0.4f;	// Get A Good Distance Away
	mFromOwner.mDirectionDot.Reset();				// Ignore any direction dot with the leader
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchFlank
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchFlank()
{
	SearchReset();
	mSearchName							= "Flank";
	mFromOwner.mDistance.mMin			=  0.35f;	// Must Be A Good Distance From Where We Are
	mFromEnemy.mFacingDot.mMin			= -0.2f;	// Must Be Behind Enemy
	mAdvance.mMin						= -0.5f;	// In Front Of Owner
	mAdvance.mMax						=  0.8f;	// But Not Directly Along Path
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchAdvance
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchAdvance()
{
	SearchReset();
	mSearchName							= "Advance";
	mFromOwner.mDistance.mMin			=  0.15f;	// Make Sure To Move Some
	mAdvance.mMin						=  0.3f;	// Forward!
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchRetreat
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchRetreat()
{
	SearchReset();
	mSearchName							= "Retreat";
	mFromOwner.mDistance.mMin			=  0.1f;	// Make Sure To Move Some
	mAdvance.mMax						= -0.3f;	// Backward!
	Search();
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::SearchAmbush
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::SearchAmbush()
{
	SearchReset();
	mSearchName							= "Ambush";
	mFromOwner.mDistance.mMin			=  0.35f;	// Must Be A Good Distance From Where We Are
	mFromEnemy.mFacingDot.mMax			=  0.2f;	// Must Be In Front Of Enemy
	mAdvance.mMin						= -0.5f;	// In Front Of Owner
	mAdvance.mMax						=  0.8f;	// But Not Directly Along Path
	Search();
}







	





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THE SEARCH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// SearchComputeWeightRange
//
// We compute the weight range so that we can make a better scale factor
// between 0.0 and 1.0 later on.  It's not critical that all the weights
// factor exactly 0.0 to 1.0, but it is nice to see how "good" a feature
// matches the given search parameters
///////////////////////////////////////////////////////////////////////////
float rvAASTacticalSensorLocal::SearchComputeWeightRange(float& rangeNegative)
{
	return (mFromOwner.WeightRange(rangeNegative) + 
			mFromEnemy.WeightRange(rangeNegative) + 
			mFromTether.WeightRange(rangeNegative) +
			mFromPath.WeightRange(rangeNegative) + 
			mAdvance.WeightRange(rangeNegative) + 
			mLeanNormal.WeightRange(rangeNegative) +
			mAssignment.WeightRange(rangeNegative));
}

///////////////////////////////////////////////////////////////////////////////
// Weight
// 
// Add up the computed weight of all tests.
///////////////////////////////////////////////////////////////////////////////
float rvAASTacticalSensorLocal::SearchComputeWeight()
{
	return (mFromOwner.Weight() + 
			mFromEnemy.Weight() + 
			mFromTether.Weight() +
			mFromPath.Weight() + 
			mAdvance.Weight() + 
			mLeanNormal.Weight() +
			mAssignment.Weight());
}

///////////////////////////////////////////////////////////////////////////////
// SortFeature function (used by Search() below)
///////////////////////////////////////////////////////////////////////////////
ID_INLINE int rvSortFeature( const TFeaturePtr *a, const TFeaturePtr *b )
{
	if ((*a)->weight > (*b)->weight)
	{
		return -1;
	}
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::Search
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::Search()
{
	idAAS* aas = (mOwnerAI)?(mOwnerAI->aas):(gameLocal.GetAAS(0));
	if (!mOwner || !aas || !aas->GetFile() || !aas->GetFile()->GetNumFeatures())
	{
		return;
	}


	static idAASFile*			file;
	static int					areaNum;
	static int					areaNumOwner;
	static rvBits<32000>		areaVisit;
	static int					areaVisitCount;
	static int					travelTime;
	static int					areaFeatureNum;
	static int					featureNum;
	static aasFeature_t*		featurePtr;
	static idVec3				featureOrigin;
	static idReachability*		reach;
	static idList<rvSortReach>	searchHeap;
	static rvSortReach			sortReach;
	static float				walkDistanceToFeature;
	static float				weight;
	static float				weightRangeNegative;
	static float				weightRangeTotal;
	static idVec3				from;
	static idVec3				endPos;
	static int					endAreaNum;




//-----------------------------------------------------------------------------
// SETUP
//-----------------------------------------------------------------------------
	file						= aas->GetFile();
	weightRangeNegative			= 0.0f;
	weightRangeTotal			= SearchComputeWeightRange(weightRangeNegative);

	mFeatures.Clear();
	searchHeap.Clear();
	areaVisit.clear();
	if (!mAssignmentValid)
	{
		mAssignment.Reset();	// Never Worry About Squad Assignments If No Leader Is Active
	}




//-----------------------------------------------------------------------------
// PHASE I - POPULATE AREA QUEUE
//-----------------------------------------------------------------------------
	if ( mOwnerAI ) 
	{
		areaNumOwner = mOwnerAI->PointReachableAreaNum ( mFromOwner.mOrigin );
	} 
	else 
	{
		areaNumOwner = aas->PointReachableAreaNum(mFromOwner.mOrigin, mOwner->GetPhysics()->GetBounds(), AREA_REACHABLE_WALK);
	}

	from			= mFromOwner.mOrigin;
	areaNum			= areaNumOwner;
	sortReach.mAreaNum	= areaNumOwner;
	sortReach.mDistance = 0.0f;
	sortReach.mReach	= 0;
	searchHeap.Append(sortReach);




//-----------------------------------------------------------------------------
// PHASE II - BREADTH FIRST SEARCH NEIGHBORING AREAS FOR FEATURES THAT TEST OK
//-----------------------------------------------------------------------------
	areaVisitCount = 0;
 	while (searchHeap.Num() && areaVisitCount<MAX_AREAS_TOUCHED && mFeatures.Num()<mFeaturesSearchMax)
	{
		rvSortReach	sr = searchHeap[0];
		searchHeap.HeapPop();
		if (areaVisit[sr.mAreaNum])
		{
			continue;
		}
		areaVisit.set(sr.mAreaNum);
		areaVisitCount ++;


		// Check For Features Here
		//-------------------------
		const aasArea_t& area = file->GetArea(sr.mAreaNum);
		if (area.numFeatures)
		{
			for (areaFeatureNum=0; areaFeatureNum<area.numFeatures; areaFeatureNum++)
			{
				featurePtr		= & (file->GetFeature(file->GetFeatureIndex(area.firstFeature+areaFeatureNum)));
				featureOrigin	= featurePtr->Origin();

				// If Walk Path Is Valid, Allow From Owner Test To Use Computed Straight Line Distance
				//-------------------------------------------------------------------------------------
				if (aas->WalkPathValid(areaNumOwner, mFromOwner.mOrigin, sr.mAreaNum, featureOrigin, TFL_WALK, endPos, endAreaNum))
				{
					walkDistanceToFeature = 0.0f;	// Allows Test to use straight line distance computed
				}

				// If It Is Not Possible To Straight Line Walk To A Feature, Use The Enter Point And Distance Of The Area (Which Is A Rough Appx)
				//--------------------------------------------------------------------------------------------------------------------------------
				else
				{
					walkDistanceToFeature = sr.mDistance + ((sr.mReach)?(sr.mReach->end.Dist(featureOrigin)):(mFromOwner.mOrigin.Dist(featureOrigin)));
				}

				// Test The Feature To See If It's Valid
				//---------------------------------------
				if (!TestValid(featurePtr, walkDistanceToFeature))
				{
					continue;
				}

				// Compute The Weight 
				//--------------------
				if (weightRangeTotal>0.0f)
				{
					weight		= SearchComputeWeight();			// Compute Weight Sum
					weight		-= weightRangeNegative;				// Bring it into a positive range
					weight		/= weightRangeTotal;				// Scale down to 0.0 - 1.0
	
					assert(weight>0.0f && weight<255.0f);
					featurePtr->weight = (char)(weight*255);
				}
				else
				{
					featurePtr->weight = (unsigned char)(128);		// No Sorting
				}


				// Append The Feature To The List
				//--------------------------------
				mFeatures.Append(featurePtr);
			}
		}


		// Add Neighboring Areas To Search
		//---------------------------------
		for (reach=area.reach; reach; reach=reach->next)
		{
			if ((reach->travelType&TFL_WALK))
			{
				walkDistanceToFeature = sr.mDistance + ((sr.mReach)?(sr.mReach->end.Dist(reach->end)):(mFromOwner.mOrigin.Dist(reach->end)));

				if (walkDistanceToFeature<MAX_DISTANCE)
				{
					sortReach.mDistance		= walkDistanceToFeature;
					sortReach.mAreaNum		= reach->toAreaNum;
					sortReach.mReach		= reach;
					searchHeap.HeapAdd(sortReach);
				}
			}
		}
	}




//-----------------------------------------------------------------------------
// PHASE III - SORT AND CLIP THE FEATURE LIST
//-----------------------------------------------------------------------------
	mFeatures.Sort(rvSortFeature);

	// Now, Clip The Sorted List To The Max Size
	//-------------------------------------------
	if (mFeatures.Num()>MAX_FEATURE_LIST)
	{
		mFeatures.Resize(MAX_FEATURE_LIST);
	}

	// Now Reset Any Parameters Which Were Only "Temporary" During The Search, and Do Not Invalidate The Point Later
	//----------------------------------------------------------------------------------------------------------------
	mFromOwner.mDistance.mMin = 0.0f;	// Allow Getting Close Again


	// Print Search Results
	//----------------------
	if (ai_showTacticalFeatures.GetInteger()==3)
	{
		common->Printf( "[%10d] Search%s Found %d Features For %s\n", gameLocal.GetTime(), mSearchName.c_str(), mFeatures.Num(), mOwner->GetName() );
	}
}







////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG GRAPHICS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// aasFeature_t::DrawDebugInfo
///////////////////////////////////////////////////////////////////////////////
void aasFeature_t::DrawDebugInfo( int index )
{
	static idVec3 Height;
	static idVec3 Orig;
	static idVec3 Norm;
	static idVec3 Text;
	static idVec4 color;
	static idVec3 Left;

	int lifetime	= 0;

	color			= colorWhite;

	if (flags & FEATURE_COVER)
	{
		color		= colorGreen;
	}

	Orig		= Origin();
	Orig[2]		+= 1.0f;

	Height		= Orig;
	Height[2]	+= height;

	Norm		= Orig;
	Norm		+= Normal() * mDebugRadius;
	Left		=  Normal().Cross(idVec3(0,0,-1)) * mDebugRadius;

	gameRenderWorld->DebugLine( color,	Orig,	Height,	lifetime );
	gameRenderWorld->DebugLine( color,  Orig,	Norm,	lifetime );

	if (index>=0)
	{
		Text = (Origin() + Height) * 0.5f;
 		gameRenderWorld->DrawText( va( "%d", index ), Text, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis, 1, lifetime );

		Text[2] += 15.0f;
		gameRenderWorld->DrawText( va( "%d", (int)weight ), Text, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis, 1, lifetime );
	}

	// Left Corner Is On The Ground
	//------------------------------
	if (flags & FEATURE_CORNER_LEFT)
	{
		gameRenderWorld->DebugLine( color, Orig,	Orig + Left, lifetime );
	}

	// Otherwise Windows Are At The Given Height
	//-------------------------------------------
	else if (flags & FEATURE_LOOK_LEFT)
	{
		gameRenderWorld->DebugLine( color, Height,	Height + Left, lifetime );
	}

	if (flags & FEATURE_CORNER_RIGHT)
	{
		gameRenderWorld->DebugLine( color, Orig,	Orig - Left, lifetime );
	}
	else if (flags & FEATURE_LOOK_RIGHT)
	{
		gameRenderWorld->DebugLine( color, Height,	Height - Left, lifetime );
	}

	if (flags & FEATURE_LOOK_OVER)
	{
		gameRenderWorld->DebugLine( color, Height,	Height + (Normal()*mDebugRadius), lifetime );
	}
}


///////////////////////////////////////////////////////////////////////////
// rvTest::DrawDebugInfo
///////////////////////////////////////////////////////////////////////////
void			rvTest::DrawDebugInfo(const idVec4 color, const idVec3& origin, const idVec3& direction)
{
	gameRenderWorld->DebugFOV(color, origin, direction, mMax, 20.0f, mMin, 10.0f, 20.0f);
}

///////////////////////////////////////////////////////////////////////////
// rvTest::DrawDebugInfo
///////////////////////////////////////////////////////////////////////////
void			rvTest::DrawDebugInfo(const idVec4 color, const idVec3& origin)
{
	static	idVec3	up(0.0f,0.0f,-1.0f);
 	if (mMax<1.0f)
	{
		gameRenderWorld->DebugCircle(color, origin, up, (mMax * MAX_DISTANCE), 25);
	}
	if (mMin>0.0f)
	{
		gameRenderWorld->DebugCircle(color, origin, up, (mMin * MAX_DISTANCE), 25);
	}
}
///////////////////////////////////////////////////////////////////////////
// rvTestSet::DrawDebugInfo
///////////////////////////////////////////////////////////////////////////
void			rvTestSet::DrawDebugInfo(const idVec4& color, const idVec3& nonProjectedOrigin)
{
	static	int		lifetime = 0;
	static	idVec3	origin;

	origin = mOrigin;
	if (mProjectOrigin)
	{
		origin = nonProjectedOrigin;
	}

	gameRenderWorld->DebugArrow( color, origin,	origin+mFacing * 25.0f, 8, lifetime );
	mFacingDot.DrawDebugInfo(color, origin, mFacing);
	mDistance.DrawDebugInfo(color, origin);
}

///////////////////////////////////////////////////////////////////////////////
// rvAASTacticalSensorLocal::DrawDebugInfo
///////////////////////////////////////////////////////////////////////////////
void rvAASTacticalSensorLocal::DrawDebugInfo() 
{
	idAAS* aas = (mOwnerAI)?(mOwnerAI->aas):(gameLocal.GetAAS(0));
	if (!aas || !aas->GetFile() || !aas->GetFile()->GetNumFeatures() || !mOwner || mOwner->IsHidden() || (ai_showTacticalFeatures.GetInteger()<2 && !mOwner->DebugFilter(ai_showTacticalFeatures) && !mOwner->DebugFilter(ai_debugTactical)))
	{
		return;
	}


	static idVec3 pos;
	bool	reservedDrawn = false;
	bool	nearDrawn = false;
	bool	lookDrawn = false;

	mDebugRadius = aas->GetSettings()->boundingBoxes[0][1][0];


	// Draw Parameters
	//-----------------
	if (ai_showTacticalFeatures.GetInteger()==1)
	{
		if (!mSearchName.IsEmpty())
		{
			pos		= mFromOwner.mOrigin + mFromEnemy.mOrigin;
			pos		*= 0.5f;
			pos[2]	+= 25.0f;
			gameRenderWorld->DrawText(mSearchName.c_str(), pos, 0.5f, colorWhite, gameLocal.GetLocalPlayer()->viewAxis, 1, 0 );
			pos[2]	-= 25.0f;

			// Draw Tests
			//------------
			mFromEnemy.DrawDebugInfo(colorYellow,	pos);
			mFromTether.DrawDebugInfo(colorOrange,  pos);
			mFromOwner.DrawDebugInfo(colorMagenta,	pos);
			mFromPath.DrawDebugInfo(colorCyan,		pos);
			mAssignment.DrawDebugInfo(colorPink,	pos);
			mAdvance.DrawDebugInfo(colorPurple,		mFromOwner.mOrigin, mFromPath.mFacing);
			mLeanNormal.DrawDebugInfo(colorPurple,	pos);
		}


		// Draw Features
		//---------------
		for (int i=0; i<mFeatures.Num(); i++) 
		{
			mFeatures[i]->DrawDebugInfo(i);
			if (mFeatures[i]==mReserved)
			{
				reservedDrawn = true;
			}
			if (mFeatures[i]==mNear)
			{
				nearDrawn = true;
			}
			if (mFeatures[i]==mLook)
			{
				lookDrawn = true;
			}
		}
	}

	// Draw All Neighboring Features If Player & CVar==2
	//---------------------------------------------------
	if (mOwner==gameLocal.GetLocalPlayer() && ai_showTacticalFeatures.GetInteger()>=2)
	{
		idAASFile* file = aas->GetFile();
		const idVec3& playerOrigin = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
		for (int i=0; i<file->GetNumFeatures(); i++)
		{
			if (file->GetFeature(i).Origin().Dist(playerOrigin)<600.0f)
			{
				file->GetFeature(i).DrawDebugInfo();
			}
		}
	}


	// Always Draw Reserved
	//----------------------
 	if (mReserved)
	{
		if (!reservedDrawn)
		{
			mReserved->DrawDebugInfo();
		}
		gameRenderWorld->DebugArrow(colorBlue, mOwner->GetPhysics()->GetOrigin(), mReserved->Origin(), 8);
	}

	// If Near Is Valid, Draw It
	//---------------------------
	if (mNear && (!mReserved || mNear!=mReserved))
	{
		if (!nearDrawn)
		{
			mNear->DrawDebugInfo();
		}
		gameRenderWorld->DebugArrow(colorOrange, mOwner->GetPhysics()->GetOrigin(), mNear->Origin(), 8);
	}

	// If Look Is True, Then Draw That
	//---------------------------------
	if (mLook && (!mOwnerAI || mOwnerAI->InLookAtCoverMode()))
	{
		idVec3 n = mLook->Normal();
		n *= 64.0f;
		gameRenderWorld->DebugArrow(colorYellow, mOwner->GetPhysics()->GetOrigin() + idVec3(0,0,32), mOwner->GetPhysics()->GetOrigin() + idVec3(0,0,32) + n, 8);
	}
}




// RAVENEND - CDR





