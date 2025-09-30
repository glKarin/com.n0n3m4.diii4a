class rvSegmentTemplate;
class rvSegment;

// rvBSE::mFlags
enum {
	EF_LOOP = 1, // 0x1 explicit loop
	EF_USES_END_ORIGIN = 1 << 1, // 0x2 effect uses end-origin
	EF_MARK_BOUNDS_DIRTY = 1 << 2, // 0x4 mark “world bounds dirty”
	EF_SOUND = 1 << 3, // 0x8 Global no-sound flag?
	EF_AMBIENT = 1 << 4, // 0x10 ambient
};

#define F_STOP_REQUESTED 8

//======================================
//              rvBSE
//======================================
class rvBSE
{
public:
	//----------------------------------
	//  Construction & lifetime
	//----------------------------------
	rvBSE()
    : mFlags(0),
      mDeclEffect(NULL),
      mStartTime(0.0f),
      mCurrentTime(0.0f),
      mLastTime(0.0f),
      mDuration(0.0f),
      mCost(0.0f),
      mAttenuation(1.0f),
      mOriginDistanceToCamera(0.0f),
      mShortestDistanceToCamera(0.0f),
      mBrightness(0.0f),
      mReferenceSoundHandle(-1), // 0
      mShortestDistanceDummy(0.0f)
    {}
	~rvBSE();                                           //  Destroy()

	//----------------------------------
	//  Initialisation / shutdown
	//----------------------------------
	void     Init(rvDeclEffect* declEffect,
		renderEffect_s* parms,
		float              time);
	void     Destroy();                                 // explicit shutdown

	//----------------------------------
	//  High-level servicing
	//----------------------------------
	bool     Service(renderEffect_s* parms,
		float              time);      // returns “finished?”
	void     UpdateSegments(float              time);

	//----------------------------------
	//  Owner-driven state updates
	//----------------------------------
	void     UpdateFromOwner(renderEffect_s* parms,
		float              time,
		bool               init);
	void     UpdateAttenuation();

	//----------------------------------
	//  Segment / sound helpers
	//----------------------------------
	void     UpdateSoundEmitter(const rvSegmentTemplate* st, rvSegment* seg);

	//----------------------------------
	//  Loop handling
	//----------------------------------
	void     LoopInstant(float              time);
	void     LoopLooping(float              time);

	//----------------------------------
	//  Queries / maths
	//----------------------------------
	float    GetAttenuation(const rvSegmentTemplate* st) const;
	float    GetOriginAttenuation(const rvSegmentTemplate* st) const;
	idVec3 GetInterpolatedOffset(idVec3 *result, float time) const;
	void     SetDuration(float time);
	const char* GetDeclName();

	float    EvaluateCost(int   segment = -1);

	//----------------------------------
	//  Visualisation / rendering
	//----------------------------------
	void             DisplayDebugInfo(const renderEffect_s* parms,
		const viewDef_s* view,
		idBounds& worldBounds);
	rvRenderModelBSE* Render(const renderEffect_s* owner, const viewDef_s* view);
    bool CanInterpolate() const;
    idSoundEmitter * GetReferenceSound(int worldId);

	//----------------------------------
	//  Public data - none
	//----------------------------------

public:
	//----------------------------------
	//  Flags  (bit assignments recovered from usage)
	//----------------------------------
	//  0x0001 : used together with 0x0010 during rewind/advance
	//  0x0002 : hasEndOrigin
	//  0x0004 : world-bounds dirty (set in UpdateFromOwner, cleared in Service)
	//  0x0008 : sound-suppressed / “no-sound” (checked in Service & UpdateSoundEmitter)
	//  0x0010 : ambient effect         (set from renderEffect_s::ambient)
	int                 mFlags;

	//----------------------------------
	//  Effect definition & segments
	//----------------------------------
	rvDeclEffect* mDeclEffect;
	idList< rvSegment* > mSegments;

	//----------------------------------
	//  Timing
	//----------------------------------
	float               mStartTime;
	float               mCurrentTime;
	float               mLastTime;
	float               mDuration;   // 0 ⇒ “instant”

	//----------------------------------
	//  Cost / attenuation
	//----------------------------------
	float               mCost;
	float               mAttenuation;
	float               mOriginDistanceToCamera;
	float               mShortestDistanceToCamera;

	//----------------------------------
	//  Spatial state – world space
	//----------------------------------
	idVec3              mCurrentOrigin;
	idVec3              mLastOrigin;
	idVec3              mCurrentVelocity;
	idMat3              mCurrentAxis;
	idMat3              mCurrentAxisTransposed;
	idBounds            mCurrentWorldBounds;

	//----------------------------------
	//  Spatial state – local space
	//----------------------------------
	idBounds            mCurrentLocalBounds;

	//----------------------------------
	//  Original (authoring-time) reference frame
	//----------------------------------
	idVec3              mOriginalOrigin;
	idVec3              mOriginalEndOrigin;
	idVec3              mCurrentEndOrigin;
	idMat3              mOriginalAxis;

	//----------------------------------
	//  Gravity
	//----------------------------------
	idVec3              mGravity;
	idVec3              mGravityDir;

	//----------------------------------
	//  Rendering parameters
	//----------------------------------
	idVec4              mTint;                 // shaderParms[0-3]
	float               mBrightness;    // shaderParms[6]
	idVec2              mSpriteSize;           // shaderParms[8-9]

	//----------------------------------
	//  Camera-relative data (cached each frame)
	//----------------------------------
	idMat3              mViewAxis;
	idVec3              mViewOrg;

	//----------------------------------
	//  Audio
	//----------------------------------
	int                 mReferenceSoundHandle;

	//----------------------------------
	//  Misc
	//----------------------------------
	float               mShortestDistanceDummy;  // present only to match the dump size
};

/*==================================================
   Inline destructor – defined here because the body
   is trivial and already supplied in Destroy().
==================================================*/
inline rvBSE::~rvBSE()
{
	Destroy();
}
