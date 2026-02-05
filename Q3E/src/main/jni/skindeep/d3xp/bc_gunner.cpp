//TODO: if cannot move to point (i.e. point is in the sky), then exit out.

//#include "sys/platform.h"
//#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
//#include "Entity.h"
//#include "Light.h"
//#include "Player.h"
#include "Fx.h"

#include "WorldSpawn.h"

#include "gamesys/SysCvar.h"
#include "Player.h"
#include "framework/DeclEntityDef.h"
#include "idlib/LangDict.h"

#include "BrittleFracture.h"

#include "bc_skullsaver.h"
#include "bc_vomanager.h"
#include "bc_airlock.h"
#include "bc_searchnode.h"
#include "bc_ventdoor.h"
#include "bc_interestpoint.h"
#include "bc_idletask.h"
#include "bc_meta.h"
#include "bc_gunner.h"
#include "bc_hazardpipe.h"
#include "bc_radiocheckin.h"
#include "bc_mech.h"

const int IDLESOUND_MINTIME = 5000;
const int IDLESOUND_MAXTIME = 20000;

const int IDLE_CALLRESPONSE_DISTANCE = 200;
const int IDLE_CALLRESPONSE_CHECKINTERVAL = 500;
const int IDLE_CALLRESPONSE_VO_GAPDURATION = 300;
const int IDLE_CALLRESPONSE_COOLDOWNTIME = 15000;

const int SMELLY_VO_DEBOUNCETIME = 5000;

const int PICKPOCKET_REACTIONTIME = 2000;

const int COMBATALERT_DELAYTIME = 2000;		//Only do the combat alert if I haven't taken damage for XX milliseconds. This is to allow the player to take out an enemy without instantly triggering the alarm.

const int SUSPICION_MAXPIPS = 120;				//how many suspicion pips until I enter combat mode.
const int SUSPICION_THINKINTERVAL = 100;		//how often to update suspicion state.
const int SUSPICION_DECAYRATE = 5;				//how fast does suspicion decay when I sense nothing suspicious

const int SUSPICION_IMMEDIATEPROXIMITY = 160;	//When target is XX distance from target, the suspicion increases dramatically quickly.
const int SUSPICION_IMMEDIATEPROXIMITY_RATE = 15; //how fast suspicion increases when in immediate proximity.

const int SUSPICION_EXTREMEIMMEDIATEPROXIMITY = 60;	//When target is XX distance from target, the suspicion increases dramatically quickly.
const int SUSPICION_EXTREMEIMMEDIATEPROXIMITY_RATE = 70; //how fast suspicion increases when in immediate proximity.

const float SUSPICION_APPROACHPENALTY_DOTTHRESHOLD = 0.2f; //dotproduct if target is approaching me, give suspicion penalty (increased suspicion rate)
const int SUSPICION_APPROACHPENALTY_PIPAMOUNT = 10; //penalty amount if target is approaching me

//const float SUSPICION_ALERTMULTIPLIER = 1.25f; //when world is in search state or combat state, the suspicion rate increases
const int SUSPICION_NOTMOVING_PENALTY = 10; //if not moving, ding it with a penalty of increased suspicion rate
const int SUSPICION_MINIMUMTIME = 500; //how long we force suspicion state to be, so that we don't thrash between states.

const float SUSPICION_MULTIPLIER_PLAYER_IN_MECH = 3;
const float SUSPICION_MULTIPLIER_PLAYER_JOCKEYING = 4;
const float SUSPICION_MULTIPLIER_HIGHLYSUSPICIOUS = 5;

#define OBSERVEINTERVAL 200
#define MAX_SIGHTLIMBS 2 //what is maximum amount of limbs that can be exposed. This is handled in GetSightExposure()




//DARKNESS_VIEWRANGE is in actor.h -- This determines how far I can see in the dark.

const int SIGHTED_FLASHTIME = 1500;

const int COMBAT_FIRINGTIME = 3000;	//3000 how long to suppress an area with gunfire.

const int COMBATOBSERVE_TIME = 4000; //after weapon fire done, do a little pause.

//when I check out an interestpoint, how long do I stay to investigate it.
//Note: this plays an animation, rifle_investigate. Make sure animation time is shorter than this duration.
const int SEARCH_WAITTIME = 8000; 

const int SEARCH_INVESTIGATE_PANTIME = 5000; //after directly looking at interestpoint, reset orientation and then do a general 'look around room' animation. This is how long that is.


const int ARRIVAL_DISTANCE = 80; //how close do we need to be to the point to be considered "arrived" at it.

const int SPECIALTRAVERSETIMER = 400; //when in combatobserve, do some fidget traversals.

const int INTEREST_SWITCH_GRACE_TIME = 1000; // Guards cannot switch interest unless they have had their current interest for at least this long


const int BLEEDOUT_DAMAGEMULTIPLIER = 30000;
const int BLEEDOUT_DAMAGEDELAY = 200; //clamp damage to every xx seconds.


const idVec3 COLOR_IDLE = idVec3(.5f, 1, 0);			//lime green
const idVec3 COLOR_SUSPICIOUS = idVec3(1, .8f, 0);		//yellow
const idVec3 COLOR_COMBAT = idVec3(1, .2f, .2f);		//red
const idVec3 COLOR_DEAD = idVec3(.8f, .8f, .8f);		//grey
const idVec3 COLOR_SEARCHING = idVec3(1, .4f, 0);		//orange

const int PET_SPAWN_INTERVAL = 15000;
const int PET_SPAWN_FAILCOOLDOWN = 2000;

const int IDLETASK_ACTIVATEDISTANCE = 32; //has to XX distance to start the idletask.

const int SEARCHNODE_ANIM_COOLDOWNTIME = 15000;


const int JOCKEY_DAMAGE_INITIALDELAY = 2000; //when player mounts enemy, have a delay before the first instance of player inflicting damage on enemy. This is to give player time to see and understand what's happening.
const int JOCKEY_DAMAGE_INTERVAL = 1500; //When being jockeyed, take damage every XX milliseconds.
const int JOCKEY_SLAM_COOLDOWN = 2000; //When player gets damaged during jockey, have a cooldown before player can get damaged again.
const int JOCKEY_WORLDFROB_TIMEINTERVAL = 400;
const int JOCKEY_WORLDFROB_RADIUS = 64; //radius distance for jockeying frobbing things in world
const int JOCKEY_WALLSLAM_RADIUS = 48; //radius distance for searching for walls to slam into
const int JOCKEY_KILLENTITY_RADIUS = 72; //radius distance for context kill entities
const int JOCKEY_WORLD_ANALYZE_INTERVAL = 100; //how often to search the world for attack opportunities
const float JOCKEY_SLAMATTACK_DAMAGE = 1000.0f;
const float JOCKEY_VERT_DISTANCE_MAX = 96; //objects above this vertical distance delta are ignored
const float JOCKEY_VERT_DISTANCE_MIN = -8; //objects below this vertical distance delta are ignored (SW 19th March 2025: Pushing this value a little lower so that objects with their origin on the floor aren't defeated by incredibly minor variations in terrain)


//I have to be in search state for XX msec in order to call initiate a radio checkin. This is so that we avoid weird
//situation where I get suspicious/damaged and immediately do a radio check on the same frame, i.e. an explosion.
const int RADIOCHECK_THRESHOLDTIME = 500;


const int OVERWATCHOBSERVE_MINIMUMTIME = 1000; //Minimum amount of time to stay in overwatch observation time.

const int MELEE_COOLDOWNTIME = 3000;
const int MELEE_CHARGETIME = 2000;

const float INTEREST_BEAM_LENGTH = 64.0f;

const float INTERESTFROB_DISTANCE = 128;

const int SUFFOCATION_INTERVAL = 300; //how frequently to take suffocation damage

#define IDLENODE_LERPTIME 500

#define SOFTFAIL_COOLDOWNTIME 2000
#define SOFTFAIL_THRESHOLD    .5f


const int HIGHLYSUSPICIOUS_TIMEINTERVAL = 2500;


CLASS_DECLARATION(idAI, idGunnerMonster)
	EVENT(EV_PostSpawn, idGunnerMonster::Event_PostSpawn)
END_CLASS



idGunnerMonster::idGunnerMonster()
{
	drawDodgeUI = false;

	jockeyAttackCurrentlyAvailable = 0;

	suspicionIntervalTimer = 0;
	suspicionCounter = 0;
	suspicionStartTime = 0;
	lastFireablePosition = {};
	combatFiringTimer = 0;
	combatStalkInitialized = false;
	stateTimer = 0;
	intervalTimer = 0;

	lastInterestPriority = 0;
	lastInterestSwitchTime = 0;
	ignoreInterestPoints = false;
	interestBeam = nullptr;
	interestBeamTarget = nullptr;
	searchMode = 0;
	searchWaitTimer = 0;
	specialTraverseTimer = 0;
	flailDamageTimer = 0;
	searchStartTime = 0;

	lastState = 0;

	fidgetTimer = 0;

	firstSearchnodeHint = false;

	headLight = nullptr;

	customidleResetTimer = 0;
	waitingforCustomidleReset = false;
	idletaskStartTime = 0;
	idletaskFrobHappened = false;
	energyshieldModuleAvailable = false;

	idlenodeStartTime = 0;
	idlenodePositionSnapped = false;
	idlenodeSnapPosition = {};
	idlenodeOriginalPosition = {};
	idlenodeSnapAxis = {};
	jockeyMoveTimer = 0;
	jockeyBehavior = 0;
	jockeyTurnTimer = 0;

	jockeyDamageTimer = 0;
	jockeyWorldfrobTimer = 0;

	jockStateTimer = 0;
	jockattackState = 0;
	jocksurfacecheckTimer = 0;

	lastJockeyBehavior = 0;
	airlockLockdownCheckTimer = 0;

	hasAlertedFriends = false;

	sighted_flashTimer = 0;

	idleSoundTimer = 0;

	radiocheckinPrimed = false;
	radiocheckinTimer = 0;

	meleeAttackTimer = 0;
	meleeChargeTimer = 0;
	meleeModeActive = 0;

	hasPathNodes = false;

	icr_state = 0;
	icr_timer = 0;

	pickpocketReactionState = 0;
	pickpocketReactionTimer = 0;
	pickpocketReactionPosition = {};

	softfailCooldown = 0;
	lastSoftfailPosition = {};

	highlySuspiciousTimer = 0;

	CanDoUnvacuumableCheck = false;

	memset(&jockeySmashTr, 0, sizeof(trace_t));
}

void idGunnerMonster::Spawn(void)
{
	headLight = NULL;

	aiState = AISTATE_IDLE;
	lastState = AISTATE_IDLE;
	GotoState(AISTATE_IDLE);

	suspicionIntervalTimer =	0;
	suspicionStartTime =			0;
	lastFireablePosition =		vec3_zero;
	combatFiringTimer =			0;
	combatStalkInitialized =	false;
	stateTimer =				0;
	lastInterestPriority =		0;
	intervalTimer =				0;
	searchWaitTimer =			0;
	searchMode =				SEARCHMODE_SEEKINGPOSITION;
	lastInterest =				NULL;
	lastInterestSwitchTime =	0;
	specialTraverseTimer =		0;
	flailDamageTimer =			0;
	lastState =					0;
	fidgetTimer	=				0;
	firstSearchnodeHint =		false;
	lastIdletaskEnt =			NULL;
	customidleResetTimer =		0;
	idletaskStartTime =			0;
	waitingforCustomidleReset = false;
	energyshieldModuleAvailable = true;
	searchStartTime = 0;
	idletaskFrobHappened = false;
	currentSearchNode = NULL;
    lastIdlenodeAnim = idStr();
	idlenodeStartTime = 0;
	idlenodePositionSnapped = false;
	idlenodeOriginalPosition = vec3_zero;
	idlenodeSnapPosition = vec3_zero;
	idlenodeSnapAxis = mat3_identity;
	jockeyMoveTimer = 0;
	jockeyBehavior = JB_BACKWARD;
	lastJockeyBehavior = jockeyBehavior;
	jockeyTurnTimer = 0;
	jockeyDamageTimer = 0;
	jockStateTimer = 0;
    jockeyWorldfrobTimer = 0;
	jockattackState = JOCKATK_NONE;
	jocksurfacecheckTimer = 0;
	airlockLockdownCheckTimer = 0;
	hasAlertedFriends = 0;
	sighted_flashTimer = 0;
	idleSoundTimer = 0;
	meleeAttackTimer = 0;
	meleeChargeTimer = 0;
	meleeModeActive = false;

	SetState("State_Spawned");


	//Spawn the headlight (shoulder mounted flashlight).
	if (1)
	{
		//Spawn lamp.
		idDict lightArgs, args;
		jointHandle_t headJoint;
		idVec3 jointPos;
		idMat3 jointAxis;
	
		headJoint = animator.GetJointHandle(spawnArgs.GetString("lamp_joint"));
		this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);
		lightArgs.Clear();
		lightArgs.SetVector("origin", jointPos);
		lightArgs.Set("texture", spawnArgs.GetString("mtr_flashlight"));
		lightArgs.SetInt("noshadows", 0);
		lightArgs.SetInt("start_off", 1);
		lightArgs.Set("light_right", "0 0 144");
		lightArgs.Set("light_target", "512 0 0"); //how far it extends
		lightArgs.Set("light_up", "0 144 0");
		headLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
		headLight->SetAxis(viewAxis);
		headLight->BindToJoint(this, headJoint, true);

		args.Clear();
		args.SetVector( "origin", vec3_origin );
		interestBeamTarget = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
		interestBeamTarget->BecomeActive( TH_PHYSICS );
		
		args.Clear();
		args.SetVector( "origin", jointPos );
		args.SetBool( "start_off", true );
		args.Set( "width", "3" );
		args.Set( "skin", "skins/beam_interestline" );
		args.Set( "target", interestBeamTarget->name.c_str() );
		args.SetBool("depthhack", true);
		interestBeam = ( idBeam * )gameLocal.SpawnEntityType( idBeam::Type, &args );
	}

	if (spawnArgs.GetBool("has_helmet", "0"))
	{
		SetHelmet(true);
	}

	//special stunned variant
	if (spawnArgs.GetBool("tut_stunned", "0"))
	{
		stateTimer = 100000000;
		stunTime = 100000000;
		GotoState(AISTATE_STUNNED);
	}

	ignoreInterestPoints = spawnArgs.GetBool("ignore_interest_points", "0");

	radiocheckinPrimed = false;
	radiocheckinTimer = 0;

	drawDodgeUI = false;

	jockeyAttackCurrentlyAvailable = JOCKATKTYPE_NONE;
	suspicionCounter = 0;

	

	currentPathTarget = NULL;

	icr_state = ICR_NONE;
	icr_timer = 0;
	icr_responderEnt = NULL;

	pickpocketReactionState = PPR_NONE;
	pickpocketReactionTimer = 0;
	pickpocketReactionPosition = vec3_zero;

	softfailCooldown = 0;
	lastSoftfailPosition = vec3_zero;
	
	highlySuspiciousTimer = 0;

	CanDoUnvacuumableCheck = true;

	//BC SPAWN END

	PostEventMS(&EV_PostSpawn, 0);
}

void idGunnerMonster::Event_PostSpawn(void)
{
	idAI::Event_PostSpawn();
	hasPathNodes = targets.Num() > 0; //does this AI have a set of pathnodes created by the level designer

	if (hasPathNodes)
	{
		//Initialize the first path target.
		currentPathTarget = targets[0]; //TODO: handle choosing from random list of path nodes.

		//Attempt to start the patrol path.
		if (!MoveToEntity(currentPathTarget.GetEntity()))
		{
			//if fail, then just do wander.
			WanderAround();
		}
	}

	AttachBeltattachments();
}


void idGunnerMonster::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( drawDodgeUI ); //  bool drawDodgeUI

	savefile->WriteObject( jockeyKillEntity ); //  idEntityPtr<idEntity> jockeyKillEntity
	savefile->WriteInt( jockeyAttackCurrentlyAvailable ); //  int jockeyAttackCurrentlyAvailable

	savefile->WriteInt( suspicionIntervalTimer ); //  int suspicionIntervalTimer
	savefile->WriteInt( suspicionCounter ); //  int suspicionCounter
	savefile->WriteInt( suspicionStartTime ); //  int suspicionStartTime
	savefile->WriteVec3( lastFireablePosition ); //  idVec3 lastFireablePosition
	savefile->WriteInt( combatFiringTimer ); //  int combatFiringTimer
	savefile->WriteBool( combatStalkInitialized ); //  bool combatStalkInitialized
	savefile->WriteInt( stateTimer ); //  int stateTimer
	savefile->WriteInt( intervalTimer ); //  int intervalTimer
	savefile->WriteObject( lastInterest ); //  idEntityPtr<idEntity> lastInterest
	savefile->WriteInt( lastInterestPriority ); //  int lastInterestPriority
	savefile->WriteInt( lastInterestSwitchTime ); //  int lastInterestSwitchTime
	savefile->WriteBool( ignoreInterestPoints ); //  bool ignoreInterestPoints
	savefile->WriteObject( interestBeam ); //  idBeam* interestBeam
	savefile->WriteObject( interestBeamTarget ); //  idBeam* interestBeamTarget
	savefile->WriteInt( searchMode ); //  int searchMode
	savefile->WriteInt( searchWaitTimer ); //  int searchWaitTimer
	savefile->WriteInt( specialTraverseTimer ); //  int specialTraverseTimer
	savefile->WriteInt( flailDamageTimer ); //  int flailDamageTimer
	savefile->WriteInt( searchStartTime ); //  int searchStartTime

	savefile->WriteInt( lastState ); //  int lastState

	savefile->WriteInt( fidgetTimer ); //  int fidgetTimer

	savefile->WriteBool( firstSearchnodeHint ); //  bool firstSearchnodeHint

	savefile->WriteObject( headLight ); //  idLight * headLight

	savefile->WriteObject( lastIdletaskEnt ); //  idEntityPtr<idEntity> lastIdletaskEnt
	savefile->WriteInt( customidleResetTimer ); //  int customidleResetTimer
	savefile->WriteBool( waitingforCustomidleReset ); //  bool waitingforCustomidleReset
	savefile->WriteInt( idletaskStartTime ); //  int idletaskStartTime
	savefile->WriteBool( idletaskFrobHappened ); //  bool idletaskFrobHappened
	savefile->WriteBool( energyshieldModuleAvailable ); //  bool energyshieldModuleAvailable
	savefile->WriteObject( currentSearchNode ); //  idEntityPtr<idEntity> currentSearchNode
	savefile->WriteString( lastIdlenodeAnim ); // idStr lastIdlenodeAnim
	savefile->WriteInt( idlenodeStartTime ); //  int idlenodeStartTime
	savefile->WriteBool( idlenodePositionSnapped ); //  bool idlenodePositionSnapped
	savefile->WriteVec3( idlenodeSnapPosition ); //  idVec3 idlenodeSnapPosition
	savefile->WriteVec3( idlenodeOriginalPosition ); //  idVec3 idlenodeOriginalPosition
	savefile->WriteMat3( idlenodeSnapAxis ); //  idMat3 idlenodeSnapAxis
	savefile->WriteInt( jockeyMoveTimer ); //  int jockeyMoveTimer
	savefile->WriteInt( jockeyBehavior ); //  int jockeyBehavior
	savefile->WriteInt( jockeyTurnTimer ); //  int jockeyTurnTimer

	savefile->WriteInt( jockeyDamageTimer ); //  int jockeyDamageTimer
	savefile->WriteInt( jockeyWorldfrobTimer ); //  int jockeyWorldfrobTimer

	savefile->WriteInt( jockStateTimer ); //  int jockStateTimer
	savefile->WriteInt( jockattackState ); //  int jockattackState
	savefile->WriteInt( jocksurfacecheckTimer ); //  int jocksurfacecheckTimer
	savefile->WriteTrace( jockeySmashTr ); //  trace_t jockeySmashTr
	savefile->WriteInt( lastJockeyBehavior ); //  int lastJockeyBehavior
	savefile->WriteInt( airlockLockdownCheckTimer ); //  int airlockLockdownCheckTimer

	savefile->WriteBool( hasAlertedFriends ); //  bool hasAlertedFriends

	savefile->WriteInt( sighted_flashTimer ); //  int sighted_flashTimer

	savefile->WriteInt( idleSoundTimer ); //  int idleSoundTimer

	savefile->WriteBool( radiocheckinPrimed ); //  bool radiocheckinPrimed
	savefile->WriteInt( radiocheckinTimer ); //  int radiocheckinTimer

	savefile->WriteInt( meleeAttackTimer ); //  int meleeAttackTimer
	savefile->WriteInt( meleeChargeTimer ); //  int meleeChargeTimer
	savefile->WriteInt( meleeModeActive ); //  int meleeModeActive

	savefile->WriteBool( hasPathNodes ); //  bool hasPathNodes
	savefile->WriteObject( currentPathTarget ); //  idEntityPtr<idEntity> currentPathTarget

	savefile->WriteInt( icr_state ); //  int icr_state
	savefile->WriteInt( icr_timer ); //  int icr_timer
	savefile->WriteObject( icr_responderEnt ); //  idEntityPtr<idEntity> icr_responderEnt

	savefile->WriteInt( pickpocketReactionState ); //  int pickpocketReactionState
	savefile->WriteInt( pickpocketReactionTimer ); //  int pickpocketReactionTimer
	savefile->WriteVec3( pickpocketReactionPosition ); //  idVec3 pickpocketReactionPosition

	savefile->WriteInt( softfailCooldown ); //  int softfailCooldown
	savefile->WriteVec3( lastSoftfailPosition ); //  idVec3 lastSoftfailPosition

	savefile->WriteInt( highlySuspiciousTimer ); //  int highlySuspiciousTimer

	savefile->WriteBool( CanDoUnvacuumableCheck ); //  bool CanDoUnvacuumableCheck
}

void idGunnerMonster::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( drawDodgeUI ); //  bool drawDodgeUI

	savefile->ReadObject( jockeyKillEntity ); //  idEntityPtr<idEntity> jockeyKillEntity
	savefile->ReadInt( jockeyAttackCurrentlyAvailable ); //  int jockeyAttackCurrentlyAvailable

	savefile->ReadInt( suspicionIntervalTimer ); //  int suspicionIntervalTimer
	savefile->ReadInt( suspicionCounter ); //  int suspicionCounter
	savefile->ReadInt( suspicionStartTime ); //  int suspicionStartTime
	savefile->ReadVec3( lastFireablePosition ); //  idVec3 lastFireablePosition
	savefile->ReadInt( combatFiringTimer ); //  int combatFiringTimer
	savefile->ReadBool( combatStalkInitialized ); //  bool combatStalkInitialized
	savefile->ReadInt( stateTimer ); //  int stateTimer
	savefile->ReadInt( intervalTimer ); //  int intervalTimer
	savefile->ReadObject( lastInterest ); //  idEntityPtr<idEntity> lastInterest
	savefile->ReadInt( lastInterestPriority ); //  int lastInterestPriority
	savefile->ReadInt( lastInterestSwitchTime ); //  int lastInterestSwitchTime
	savefile->ReadBool( ignoreInterestPoints ); //  bool ignoreInterestPoints
	savefile->ReadObject( CastClassPtrRef(interestBeam) ); //  idBeam* interestBeam
	savefile->ReadObject( CastClassPtrRef(interestBeamTarget) ); //  idBeam* interestBeamTarget
	savefile->ReadInt( searchMode ); //  int searchMode
	savefile->ReadInt( searchWaitTimer ); //  int searchWaitTimer
	savefile->ReadInt( specialTraverseTimer ); //  int specialTraverseTimer
	savefile->ReadInt( flailDamageTimer ); //  int flailDamageTimer
	savefile->ReadInt( searchStartTime ); //  int searchStartTime

	savefile->ReadInt( lastState ); //  int lastState

	savefile->ReadInt( fidgetTimer ); //  int fidgetTimer

	savefile->ReadBool( firstSearchnodeHint ); //  bool firstSearchnodeHint

	savefile->ReadObject( CastClassPtrRef(headLight) ); //  idLight * headLight

	savefile->ReadObject( lastIdletaskEnt ); //  idEntityPtr<idEntity> lastIdletaskEnt
	savefile->ReadInt( customidleResetTimer ); //  int customidleResetTimer
	savefile->ReadBool( waitingforCustomidleReset ); //  bool waitingforCustomidleReset
	savefile->ReadInt( idletaskStartTime ); //  int idletaskStartTime
	savefile->ReadBool( idletaskFrobHappened ); //  bool idletaskFrobHappened
	savefile->ReadBool( energyshieldModuleAvailable ); //  bool energyshieldModuleAvailable
	savefile->ReadObject( currentSearchNode ); //  idEntityPtr<idEntity> currentSearchNode
	savefile->ReadString( lastIdlenodeAnim ); // idStr lastIdlenodeAnim
	savefile->ReadInt( idlenodeStartTime ); //  int idlenodeStartTime
	savefile->ReadBool( idlenodePositionSnapped ); //  bool idlenodePositionSnapped
	savefile->ReadVec3( idlenodeSnapPosition ); //  idVec3 idlenodeSnapPosition
	savefile->ReadVec3( idlenodeOriginalPosition ); //  idVec3 idlenodeOriginalPosition
	savefile->ReadMat3( idlenodeSnapAxis ); //  idMat3 idlenodeSnapAxis
	savefile->ReadInt( jockeyMoveTimer ); //  int jockeyMoveTimer
	savefile->ReadInt( jockeyBehavior ); //  int jockeyBehavior
	savefile->ReadInt( jockeyTurnTimer ); //  int jockeyTurnTimer

	savefile->ReadInt( jockeyDamageTimer ); //  int jockeyDamageTimer
	savefile->ReadInt( jockeyWorldfrobTimer ); //  int jockeyWorldfrobTimer

	savefile->ReadInt( jockStateTimer ); //  int jockStateTimer
	savefile->ReadInt( jockattackState ); //  int jockattackState
	savefile->ReadInt( jocksurfacecheckTimer ); //  int jocksurfacecheckTimer
	savefile->ReadTrace( jockeySmashTr ); //  trace_t jockeySmashTr
	savefile->ReadInt( lastJockeyBehavior ); //  int lastJockeyBehavior
	savefile->ReadInt( airlockLockdownCheckTimer ); //  int airlockLockdownCheckTimer

	savefile->ReadBool( hasAlertedFriends ); //  bool hasAlertedFriends

	savefile->ReadInt( sighted_flashTimer ); //  int sighted_flashTimer

	savefile->ReadInt( idleSoundTimer ); //  int idleSoundTimer

	savefile->ReadBool( radiocheckinPrimed ); //  bool radiocheckinPrimed
	savefile->ReadInt( radiocheckinTimer ); //  int radiocheckinTimer

	savefile->ReadInt( meleeAttackTimer ); //  int meleeAttackTimer
	savefile->ReadInt( meleeChargeTimer ); //  int meleeChargeTimer
	savefile->ReadInt( meleeModeActive ); //  int meleeModeActive

	savefile->ReadBool( hasPathNodes ); //  bool hasPathNodes
	savefile->ReadObject( currentPathTarget ); //  idEntityPtr<idEntity> currentPathTarget

	savefile->ReadInt( icr_state ); //  int icr_state
	savefile->ReadInt( icr_timer ); //  int icr_timer
	savefile->ReadObject( icr_responderEnt ); //  idEntityPtr<idEntity> icr_responderEnt

	savefile->ReadInt( pickpocketReactionState ); //  int pickpocketReactionState
	savefile->ReadInt( pickpocketReactionTimer ); //  int pickpocketReactionTimer
	savefile->ReadVec3( pickpocketReactionPosition ); //  idVec3 pickpocketReactionPosition

	savefile->ReadInt( softfailCooldown ); //  int softfailCooldown
	savefile->ReadVec3( lastSoftfailPosition ); //  idVec3 lastSoftfailPosition

	savefile->ReadInt( highlySuspiciousTimer ); //  int highlySuspiciousTimer

	savefile->ReadBool( CanDoUnvacuumableCheck ); //  bool CanDoUnvacuumableCheck
}

//This gets called when the pathnode has been reached, and I need a new pathnode to go to next.
void idGunnerMonster::HasArriveAtPathNode()
{
	if (!currentPathTarget.IsValid())
	{
		common->Warning("'%s' path is not a closed loop.\n", GetName());
		WanderAround();
		return;
	}
	
	int numOfTargets = currentPathTarget.GetEntity()->targets.Num();
	if (numOfTargets <= 0)
	{
		common->Warning("'%s' path '%s' has no targets.\n", GetName(), currentPathTarget.GetEntity()->GetName() );
		WanderAround();
		return;
	}


	//Check if this path corner has an animation on it or not.
	idStr animName = currentPathTarget.GetEntity()->spawnArgs.GetString("anim");
	if (animName.IsEmpty())
	{
		//The pathnode I just arrived at has no animation. It's just a plain pathnode. Go directly to next pathnode.
		GotoNextPathNode();
		return;
	}


	if (!customIdleAnim.IsEmpty() && AI_NODEANIM == false)
	{
		//has just finished playing a path animation.
		customIdleAnim.Empty();
		GotoNextPathNode();
		return;
	}


	// ========= handle the path node animation logic here ===========

	//Ok, we have an animation we need to play.
	StopMove(MOVE_STATUS_DONE);
	
	if (currentPathTarget.GetEntity()->spawnArgs.GetBool("use_yaw"))
	{
		//we want to lerp position/yaw to the pathnode.
		idlenodePositionSnapped = false;
		idlenodeStartTime = gameLocal.time;
		
		if (currentPathTarget.GetEntity()->spawnArgs.GetBool("use_position", "1"))
			idlenodeSnapPosition = currentPathTarget.GetEntity()->GetPhysics()->GetOrigin();
		else
			idlenodeSnapPosition = GetPhysics()->GetOrigin();


		idlenodeOriginalPosition = GetPhysics()->GetOrigin();		
		idlenodeSnapAxis = currentPathTarget.GetEntity()->GetPhysics()->GetAxis();
	}
	else
	{
		//Do not lerp/snap my position or yaw. Just do the animation where I stand.
		idlenodePositionSnapped = true;
	}

	animName = GetPathcornerParsedAnim(currentPathTarget.GetEntity()); //pick random animation from the animation list (if applicable), otherwise just pick the first and only animation
	customIdleAnim = animName;
	if (animName.Length() > 0)
	{
		AI_NODEANIM = true;
	}
}

void idGunnerMonster::GotoNextPathNode()
{
	int numOfTargets = currentPathTarget.GetEntity()->targets.Num();
	int pathIndex = 0;
	if (numOfTargets > 1)
	{
		pathIndex = gameLocal.random.RandomInt(numOfTargets); //pathnode has multiple targets..... randomly choose one.
	}
	currentPathTarget = currentPathTarget.GetEntity()->targets[pathIndex];

	if (currentPathTarget.GetEntity()->spawnArgs.GetBool("use_position", "1"))
	{
		//Default behavior: move to the path corner.
		if (!MoveToEntity(currentPathTarget.GetEntity()))
		{
#if _DEBUG
			gameRenderWorld->DebugArrow(colorRed, GetPhysics()->GetOrigin(), currentPathTarget.GetEntity()->GetPhysics()->GetOrigin(), 8, 120000);
			gameRenderWorld->DebugTextSimple("PATH FAIL", currentPathTarget.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 16), 120000, colorRed);
			common->Warning("GotoNextPathNode(): '%s' path fail. (%.0f %.0f %.0f)", GetName(),
				currentPathTarget.GetEntity()->GetPhysics()->GetOrigin().x, currentPathTarget.GetEntity()->GetPhysics()->GetOrigin().y, currentPathTarget.GetEntity()->GetPhysics()->GetOrigin().z);
#endif
		}
	}
	else
	{
		//Special behavior: ignore the position.
		StopMove(MOVE_STATUS_DONE);
	}
}

void idGunnerMonster::Think(void)
{
	if (ai_debugPerception.GetInteger() )
	{
		if (visionBox != nullptr)
		{
			jointHandle_t headjoint = animator.GetJointHandle(spawnArgs.GetString("vision_joint"));

			idVec3 visionboxForward = visionBox->GetPhysics()->GetAxis().ToAngles().ToForward();

			idVec3 jointPos;
			idMat3 jointAxis;
			this->GetJointWorldTransform(headjoint, gameLocal.time, jointPos, jointAxis);
			gameRenderWorld->DebugArrow(colorWhite, jointPos, jointPos + (visionboxForward * 128), 4, 10);
		}
	}

	idAI::Think();

	if (health <= 0)
		return;

	switch (aiState)
	{
		case AISTATE_SPACEFLAIL:
			State_Spaceflail();
			break;
		case AISTATE_IDLE:
			State_Idle();
			break;
		case AISTATE_SUSPICIOUS:
			State_Suspicious();
			break;
		case AISTATE_COMBAT:
			State_Combat();
			break;
		case AISTATE_COMBATOBSERVE:
			State_CombatObserve();
			break;
		case AISTATE_COMBATSTALK:
			State_CombatStalk();
			break;
		case AISTATE_SEARCHING:
			State_Searching();
			break;
		case AISTATE_OVERWATCH:
			State_Overwatch();
			break;
		case AISTATE_VICTORY:
			State_Victory();
			break;
		case AISTATE_STUNNED:

			//BC 3-17-2025: if player is in mid pick pocket, cancel it out.
			if (gameLocal.GetLocalPlayer()->IsPickpocketing())
			{
				idEntity* pickpocketEnt = gameLocal.GetLocalPlayer()->GetPickpocketEnt();
				if (pickpocketEnt != NULL)
				{
					if (pickpocketEnt->GetBindMaster() != NULL)
					{
						if (pickpocketEnt->GetBindMaster()->entityNumber == this->entityNumber)
						{
							gameLocal.GetLocalPlayer()->DoPickpocketFail(false);
						}
					}
				}
			}


			State_Stunned();
			break;
		case AISTATE_JOCKEYED:
			State_Jockeyed();
			break;
		default:
			common->Error("invalid AI state %d in '%s'", state, GetName());
			break;
	}


	if (aiState != AISTATE_SPACEFLAIL)
	{
		//check if we should switch to airless state.
		if (gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetAbsBounds().GetCenter()) && spawnArgs.GetBool("vacuumable", "1"))
		{
			GotoState(AISTATE_SPACEFLAIL);
		}
	}

	DoZeroG_Unvacuumable_Check();

	if (ai_showState.GetBool())
	{
		idStr statename;

		switch (aiState)
		{
			case AISTATE_IDLE: statename = "idle"; break;
			case AISTATE_SUSPICIOUS: statename = "suspicious"; break;
			case AISTATE_COMBAT: statename = "combat"; break;
			case AISTATE_COMBATOBSERVE: statename = "combatobserve"; break;
			case AISTATE_COMBATSTALK: statename = "combatstalk"; break;
			case AISTATE_SEARCHING: statename = "searching"; break;
			case AISTATE_OVERWATCH: statename = "overwatch"; break;
			case AISTATE_SPACEFLAIL: statename = "spaceflail"; break;
			case AISTATE_STUNNED: statename = "stunned"; break;
		}

		gameRenderWorld->DrawText( statename.c_str(), this->GetEyePosition() + idVec3(0,0,8), 0.3f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

		idStr rawStateName;
		int colonPos;
		idVec3 aboveHead(0, 0, 20);

		rawStateName = state->Name();
		colonPos = rawStateName.Find(':');

		if (colonPos >= 0)
		{
			rawStateName = rawStateName.Right(rawStateName.Length() - colonPos - 8);
		}

		gameRenderWorld->DrawText(rawStateName, this->GetEyePosition() + aboveHead, 0.5f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	}

	

	if (ai_showInterestPoints.GetBool())
	{
		if (this->lastInterest.IsValid())
		{
			gameRenderWorld->DrawText(idStr::Format("%s -- %i", this->lastInterest.GetEntity()->name.c_str(), this->lastInterestPriority), this->GetEyePosition() + idVec3(0, 0, 8), .1f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);
		}
		else
		{
			gameRenderWorld->DrawText("No interest", this->GetEyePosition() + idVec3(0, 0, 8), .1f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10);
		}	
	}

	// Update interest beam
	if ( lastInterest.IsValid() )
	{
		idEntity* interestEnt = lastInterest.GetEntity();
		UpdateInterestBeam( interestEnt );

		idVec3 interestPos = interestEnt->GetPhysics()->GetOrigin();
		float dist = ( interestPos - lastFireablePosition ).Length();
		if ( dist > 100 )
		{
			idVec3 interestWalkableDestination = FindValidPosition( interestPos );

			MoveToPosition( interestWalkableDestination );
			lastFireablePosition = interestPos;
		}
	}
	else
	{
		// Occasionally we may end up with the interest beam still showing, hide it in that case
		interestBeam->Hide();
	}

	if (gameLocal.time >= airlockLockdownCheckTimer && health > 0 && CanAcceptStimulus() && aiState != AISTATE_IDLE && aiState != AISTATE_SUSPICIOUS  && team == TEAM_ENEMY)
	{
		airlockLockdownCheckTimer = gameLocal.time + 1500;
		DoAirlockLockdownCheck();
	}
}

void idGunnerMonster::DoAirlockLockdownCheck()
{
	//check if can see an enemy inside an airlock.
	idEntity *enemyEnt = FindEnemyAIVisible();

	if (enemyEnt == NULL)
		return;

	for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
	{
		if (!airlockEnt)
			continue;

		if (!airlockEnt->IsType(idAirlock::Type))
			continue;

		//See if airlock is already purged.
		if (static_cast<idAirlock *>(airlockEnt)->IsAirlockLockdownActive())
			continue;

		//See if entity is inside the airlock.
		idVec3 playerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1);
		if (airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(enemyEnt->GetPhysics()->GetOrigin())
			&& airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(playerPosition))
		{
			//Ent is inside airlock. Activate the lockdown.
			idStr airlockLog = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_airlock_see"), displayName.c_str());
			gameLocal.AddEventLog(airlockLog.c_str(), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
			static_cast<idAirlock *>(airlockEnt)->DoAirlockLockdown(true);
			continue;
		}
	}
}

void idGunnerMonster::State_Spaceflail()
{
	//Check if gravity has returned.
	if (!gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetAbsBounds().GetCenter()))
	{
		this->GetPhysics()->SetGravity(gameLocal.GetGravity());
		GotoState(AISTATE_SEARCHING);
	}

	//Take damage over time.
	if (gameLocal.time > flailDamageTimer)
	{
		flailDamageTimer = gameLocal.time + SUFFOCATION_INTERVAL;
		this->Damage(NULL, NULL, idVec3(0, 0, 1), "damage_noair", 1.0f, 0);
	}
}

void idGunnerMonster::State_Idle()
{
	idEntity *enemyEnt = FindEnemyAIVisible();

	//note: the logic for handling ai seeing the player in Unseen state is in: idAI::ReactionTo

	Event_SetLaserActive(0); //Turn off laser.

	//Eyeballs.
	if (enemyEnt)
	{
		//if (!TargetIsInDarkness(enemyEnt))
		{
			idVec3 enemyEyePos = GetEyePositionIfPossible(enemyEnt);
			LookAtPointMS(enemyEyePos, SUSPICION_THINKINTERVAL + 500); //Look at the enemy.

			//I see enemy. Go to suspicion state.
			suspicionIntervalTimer = gameLocal.time + SUSPICION_THINKINTERVAL;
			suspicionCounter = 0;
			suspicionStartTime = gameLocal.time;
			GotoState(AISTATE_SUSPICIOUS);
			return;
		}
	}

	//Take damage.
	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
        AI_SHIELDHIT = false;		

		//BC 3-14-2025: turn toward the inflictor, not the object that caused damage. This handles situation where player runs up to enemy and melees them.
		//lastFireablePosition = lastAttackerDamageOrigin; //turn toward the object that inflicted damage.
		if (lastDamageOrigin != vec3_zero)
			lastFireablePosition = lastDamageOrigin;
		else
			lastFireablePosition = lastAttackerDamageOrigin;

		TurnToward(lastFireablePosition);

		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;

		GotoState(AISTATE_COMBAT);
	}

	//basic patrol.
	if (AI_CUSTOMIDLEANIM)
	{
		//Currently in an idletask. Idletask example: using a health station.
		//Check if we need to do any idletask update stuff.
		if (lastIdletaskEnt.IsValid())
		{
			if (!idletaskFrobHappened)
			{
				int frobDelayTimer = lastIdletaskEnt.GetEntity()->spawnArgs.GetInt("frob_delay", "-1");
				if (frobDelayTimer >= 0)
				{
					if (gameLocal.time > idletaskStartTime + frobDelayTimer)
					{
						//Frob delay has happened.
						idletaskFrobHappened = true;

						if (lastIdletaskEnt.GetEntity()->IsType(idIdleTask::Type))
						{
							idIdleTask *idletask = static_cast<idIdleTask *>(lastIdletaskEnt.GetEntity());
							if (idletask->assignedOwner.IsValid())
							{
								idletask->assignedOwner.GetEntity()->DoFrob(0, this);
								gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_enemyuse"), displayName.c_str(), idletask->assignedOwner.GetEntity()->displayName.c_str()), idletask->assignedOwner.GetEntity()->GetPhysics()->GetOrigin());
							}
						}						
					}
				}
			}

			int endDelayTimer = lastIdletaskEnt.GetEntity()->spawnArgs.GetInt("end_delay", "-1");
			if (endDelayTimer >= 0)
			{
				if (gameLocal.time > idletaskStartTime + endDelayTimer)
				{
					//the end timer has expired. End the idle task.
					if (lastIdletaskEnt.GetEntity()->IsType(idIdleTask::Type))
					{
						static_cast<idIdleTask *>(lastIdletaskEnt.GetEntity())->assignedActor = NULL;
						AI_CUSTOMIDLEANIM = false;
						//customIdleAnim = "";

						idVec3 eyeForward = viewAxis.ToAngles().ToForward();
						Event_LookAtPoint(GetEyePosition() + eyeForward * 16, .1f); //Reset  the look function.

						if (lastIdletaskEnt.GetEntity()->spawnArgs.GetBool("delete_when_done", "0")) //Option to delete the task when the actor completes the task.
						{
							lastIdletaskEnt.GetEntity()->PostEventMS(&EV_Remove, 0);
						}
					}
				}
			}

			if (lastIdletaskEnt.GetEntity()->IsType(idIdleTask::Type))
			{
				if (lastIdletaskEnt.GetEntity()->spawnArgs.GetBool("delete_when_start", "0")) //Option to delete the task when the actor starts the task.
				{
					lastIdletaskEnt.GetEntity()->PostEventMS(&EV_Remove, 0);
				}
			}
		}

		if (waitingforCustomidleReset && gameLocal.time > customidleResetTimer && move.moveStatus != MOVE_STATUS_MOVING)
		{
			//This gets called ONCE when the idletask starts.
			waitingforCustomidleReset = false; //This flag makes the actor snap to the position.

			if (lastIdletaskEnt.IsValid())
			{
				if (lastIdletaskEnt.GetEntity())
				{
					//Since the ai will interact with world things, we want to guarantee the monster is at a specific position in order for the animations to line up.
					//So we just brute force it and snap the ai into the correct place.

					if (static_cast<idIdleTask *>(lastIdletaskEnt.GetEntity())->assignedOwner.IsValid())
					{
						idVec3 ownerPos = static_cast<idIdleTask *>(lastIdletaskEnt.GetEntity())->assignedOwner.GetEntity()->GetPhysics()->GetOrigin();
						idVec3 idlePos = lastIdletaskEnt.GetEntity()->GetPhysics()->GetOrigin();

						//Snap to the desired position.
						idAngles angleDelta = (ownerPos - idlePos).ToAngles(); //get new angle.
						//this->Teleport(idlePos, idAngles(0, angleDelta.yaw, 0), NULL);
						this->Teleport(idlePos, idAngles(0, angleDelta.yaw, 0), lastIdletaskEnt.GetEntity());
					}

				}
			}
		}
	}
	else if (AI_NODEANIM)
	{
		//we do position snap so that animations line up correctly with world objects. we lerp it so that it looks smoother.
		if (!idlenodePositionSnapped)
		{
			float lerp = (gameLocal.time - idlenodeStartTime) / (float)IDLENODE_LERPTIME;
			lerp = idMath::ClampFloat(0, 1, lerp);
			lerp = idMath::CubicEaseInOut(lerp);

			TurnToward(idlenodeSnapPosition + idlenodeSnapAxis.ToAngles().ToForward() * 64); //force the angle to be the node's yaw.

			//Lerp the actor into the final position.
			if (currentPathTarget.IsValid())
			{
				if (currentPathTarget.GetEntity()->spawnArgs.GetBool("use_position", "1"))
				{
					idVec3 lerpedPosition;
					lerpedPosition.Lerp(idlenodeOriginalPosition, idlenodeSnapPosition, lerp);
					GetPhysics()->SetOrigin(lerpedPosition + idVec3(0, 0, CM_CLIP_EPSILON));
					UpdateVisuals();
					spawnArgs.SetVector("origin", lerpedPosition);
				}
			}

			if (gameLocal.time >= idlenodeStartTime + IDLENODE_LERPTIME)
			{
				idlenodePositionSnapped = true;
			}
		}
	}
	else
	{
		if (move.moveStatus != MOVE_STATUS_MOVING && spawnArgs.GetBool("wander", "1"))
		{
			//Did I arrive at an idleTask?

			for (idEntity* entity = gameLocal.idletaskEntities.Next(); entity != NULL; entity = entity->idletaskNode.Next()) //Iterate through all idletasks.
			{
				if (!entity)
					continue;

				if (static_cast<idIdleTask *>(entity)->IsClaimed())
				{
					if (static_cast<idIdleTask *>(entity)->assignedActor.IsValid())
					{
						if (static_cast<idIdleTask *>(entity)->assignedActor.GetEntity() == this) //THIS ACTOR is the one assigned to idletask. Great.
						{
							//Do distance check.
							float distance = (entity->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
							if (distance <= IDLETASK_ACTIVATEDISTANCE)
							{
								//I am at the idleTask location. Start the idletask here.
								customIdleAnim = entity->spawnArgs.GetString("idleanim");
								if (customIdleAnim.Length() > 0)
								{
									AI_CUSTOMIDLEANIM = true;
								}
								else
								{
									gameLocal.Warning("AI_CUSTOMIDLEANIM ERROR: no idleanim on '%s'\n", entity->GetName());
									gameRenderWorld->DebugArrow(colorRed, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 128), entity->GetPhysics()->GetOrigin(), 4, 90000);
								}

								idletaskFrobHappened = false;
								//StopMove(MOVE_STATUS_DONE);

								if (static_cast<idIdleTask *>(entity)->assignedOwner.IsValid())
								{
									idVec3 ownerPos = static_cast<idIdleTask *>(entity)->assignedOwner.GetEntity()->GetPhysics()->GetOrigin();
									TurnToward(ownerPos);
									Event_LookAtPoint(ownerPos, 1000000); //should probably find a better solution for this -- currently just makes them look at it for a long time								
								}

								lastIdletaskEnt = entity;
								customidleResetTimer = gameLocal.time + 500;
								waitingforCustomidleReset = true;
								idletaskStartTime = gameLocal.time;
								return;
							}
						}
					}
				}
			}

			if (currentSearchNode.IsValid())
			{
				//See if we're at the searchnode location.

				int lastTimeUsed = gameLocal.time;
				//if (currentSearchNode.GetEntity()->IsType(idSearchNode::Type))
				//{
				//	lastTimeUsed = static_cast<idPathCorner *>(currentSearchNode.GetEntity())->lastNodeAnimTime;
				//}

				//we only do idle anims if player has exited the cryo pod. This a kludgy solution to solving the issue where AI were sometimes doing the idle animations
				//inside the cryo pod room. This prevents them from doing that ; by essentially just not allowing any idle animations when the player is inside the cryo pod.

				float distance = (currentSearchNode.GetEntity()->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
				idStr idleAnim = GetParsedAnim(currentSearchNode.GetEntity());
				if (distance <= IDLETASK_ACTIVATEDISTANCE && idleAnim.Length() > 0 && gameLocal.time > lastTimeUsed + SEARCHNODE_ANIM_COOLDOWNTIME && spawnArgs.GetBool("has_idlenodes", "0")
					&& static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerExitedCryopod())
				{
					//Standing very close to searchnode.
					StopMove(MOVE_STATUS_DONE);

					customIdleAnim = idleAnim;
                    lastIdlenodeAnim = customIdleAnim;					
					AI_NODEANIM = true;
					

                    if (customIdleAnim.Find("yaw") >= 0)
                    {
                        //Animation is locked to node direction. Turn the actor.
						GetPhysics()->SetLinearVelocity(vec3_origin);
                        idVec3 lookPosition = currentSearchNode.GetEntity()->GetPhysics()->GetOrigin() + currentSearchNode.GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward() * 64;
                        TurnToward(lookPosition);

						idlenodePositionSnapped = false;
						idlenodeStartTime = gameLocal.time;
						idlenodeSnapPosition = currentSearchNode.GetEntity()->GetPhysics()->GetOrigin();
						idlenodeOriginalPosition = GetPhysics()->GetOrigin();

						idlenodeSnapAxis = currentSearchNode.GetEntity()->GetPhysics()->GetAxis();
                    }
					else
					{
						idlenodePositionSnapped = true;
					}

					//if (currentSearchNode.GetEntity()->IsType(idSearchNode::Type))
					//{
					//	static_cast<idSearchNode *>(currentSearchNode.GetEntity())->lastNodeAnimTime = gameLocal.time;
					//}

					currentSearchNode = NULL;
					return;
				}
				else
				{
					currentSearchNode = NULL;
				}				
			}


			//Find a search node. Walk to it.
			idEntity *searchnode = GetSearchNodeSpreadOut();
			if (searchnode && !hasPathNodes)
			{
				//Found a valid search node. Go to it.
				if (MoveToPosition(searchnode->GetPhysics()->GetOrigin()))
				{
					currentSearchNode = searchnode;
				}
				else
				{
					//Failed to move to the searchnode for some reason... do a wander fallback.
					if (hasPathNodes)
						HasArriveAtPathNode();
					else
						WanderAround();
				}
			}
			else
			{
				//Failed to find a valid search node... do a wander fallback.
				if (hasPathNodes)
					HasArriveAtPathNode();
				else
					WanderAround();
			}
		}
	}

	if (gameLocal.time >= idleSoundTimer)
	{
		idleSoundTimer = gameLocal.time + gameLocal.random.RandomInt(IDLESOUND_MINTIME, IDLESOUND_MAXTIME);
		//int remainingEnemies = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetEnemiesRemaining(false);
		//gameLocal.voManager.SayVO(this, (remainingEnemies <= 1) ? "snd_vo_wanderscare" : "snd_vo_wander", VO_CATEGORY_GRUNT);

		//Only play idle sound if I'm not emitting any sound
 		if (this->refSound.referenceSound == NULL)
		{
			int remainingEnemies = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetEnemiesRemaining(false);
			gameLocal.voManager.SayVO(this, (remainingEnemies <= 1) ? "snd_vo_wanderscare" : "snd_vo_wander", VO_CATEGORY_GRUNT);
		}		
	}

	//Check if I'm near someone, for the call and response logic.
	if (icr_state == ICR_NONE && gameLocal.time > 2000)
	{
		//am I near someone.
		if (gameLocal.time > icr_timer)
		{
			icr_timer = gameLocal.time + IDLE_CALLRESPONSE_CHECKINTERVAL;
			idEntity* responder = IsNearSomeone();
			if (responder != NULL)
			{
				//This is the first "volley" in the call-response (i.e. this is the Call)
				int len = gameLocal.voManager.SayVO(this, "snd_vo_idlecall", VO_CATEGORY_BARK);
				if (len > 0)
				{
 					icr_timer = gameLocal.time + len + IDLE_CALLRESPONSE_VO_GAPDURATION;
					icr_state = ICR_CALLER;
					icr_responderEnt = responder;
					if (responder->IsType(idGunnerMonster::Type))
					{
						static_cast<idGunnerMonster*>(responder)->ResetCallresponseTimer();
					}
				}
			}
		}
	}
	else if (icr_state == ICR_CALLER)
	{
		if (gameLocal.time > icr_timer)
		{
			icr_state = ICR_NONE;
			icr_timer = gameLocal.time + IDLE_CALLRESPONSE_COOLDOWNTIME;

			//Make the responder respond.
			if (icr_responderEnt.IsValid())
			{
				gameLocal.voManager.SayVO(icr_responderEnt.GetEntity(), "snd_vo_idleresponse", VO_CATEGORY_BARK);
			}
		}
	}

	UpdatePickpocketReaction();
}

//This is used for the idle response VO system.
//Note: this ignores spearbot.
idEntity * idGunnerMonster::IsNearSomeone()
{
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0)
			continue;

		if (!entity->IsType(idAI::Type) || entity->team != this->team || entity->entityNumber == this->entityNumber)
			continue;

		if (!entity->spawnArgs.GetBool("can_talk", "1") || !static_cast<idAI *>(entity)->CanAcceptStimulus())
			continue;

		float dist = (entity->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).LengthFast();
		if (dist <= IDLE_CALLRESPONSE_DISTANCE)
		{
			idVec3 responderEyePos = static_cast<idAI*>(entity)->GetEyePosition();

			//Do traceline check to see if there's geometry in the way.
			trace_t tr;
			gameLocal.clip.TracePoint(tr, GetEyePosition(), responderEyePos, MASK_SOLID, NULL);
			if (tr.fraction >= 1)
			{
				return entity;
			}
		}
	}

	return NULL;
}

void idGunnerMonster::ResetCallresponseTimer()
{
	icr_timer = gameLocal.time + IDLE_CALLRESPONSE_COOLDOWNTIME;
}


//If there are multiple separated by semicolon, then split htem up
idStr idGunnerMonster::GetParsedAnim(idEntity *searchnode)
{
    if (!searchnode->IsType(idSearchNode::Type))
        return NULL;

    if (static_cast<idSearchNode *>(searchnode)->nodeAnimList.Num() <= 0)
        return NULL;

    if (static_cast<idSearchNode *>(searchnode)->nodeAnimList.Num() == 1)
    {
        if (!idStr::Icmp(static_cast<idSearchNode *>(searchnode)->nodeAnimList[0], lastIdlenodeAnim))
        {
            //There's only one available animation and it matches our last anim. Skip....
            return NULL;
        }

        return static_cast<idSearchNode *>(searchnode)->nodeAnimList[0].c_str(); //Only one in the list. Return it.
    }

    //Ok, we now have a LIST of possible animations. We want to:
    //A. pick a random animation from the list.
    //B. ensure the animation we pick is NOT the same animation as our previous idle animation.

    idList<idStr> animCandidates;
    for (int i = 0; i < static_cast<idSearchNode *>(searchnode)->nodeAnimList.Num(); i++)
    {
        if (!idStr::Icmp(static_cast<idSearchNode *>(searchnode)->nodeAnimList[i], lastIdlenodeAnim))
            continue; //matches the same name as previous idle anim. Skip it...

        animCandidates.Append(static_cast<idSearchNode *>(searchnode)->nodeAnimList[i]);
    }

    //We now have a curated list, that excludes our last idleanim. Return a random anim from this new list.
    return animCandidates[gameLocal.random.RandomInt(animCandidates.Num())];
}

idStr idGunnerMonster::GetPathcornerParsedAnim(idEntity *searchnode)
{
	if (!searchnode->IsType(idPathCorner::Type))
		return NULL;

	int listCount = static_cast<idPathCorner *>(searchnode)->nodeAnimList.Num();
	if (listCount <= 0)
		return NULL;

	if (listCount == 1)
	{
		return static_cast<idPathCorner *>(searchnode)->nodeAnimList[0].c_str(); //Only one in the list. Return it.
	}

	//Ok, we now have a LIST of possible animations. We want to pick a random animation from the list.

	int randomIndex = gameLocal.random.RandomInt(listCount);
	return static_cast<idPathCorner *>(searchnode)->nodeAnimList[randomIndex];
}

//This state is preceded by either IDLE state or SEARCHING state. I caught a glimpse of enemy, but I give a grace period for enemy to "escape" my vision and not be seen by me.
void idGunnerMonster::State_Suspicious()
{
	idEntity *enemyEnt = FindEnemyAIVisible();

	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
		AI_SHIELDHIT = false;
		lastFireablePosition = lastAttackerDamageOrigin;
		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;

		if (enemyEnt != NULL)
		{
			if (enemyEnt == gameLocal.GetLocalPlayer())
			{
				sighted_flashTimer = gameLocal.time + SIGHTED_FLASHTIME; //I see the enemy. Display the sighted flash UI.
			}
		}

		GotoState(AISTATE_COMBAT);
		return;
	}

	//common->Printf("enemy: %s\n", (enemyEnt == nullptr) ? "null" : enemyEnt->GetName());
	if (enemyEnt)
	{
        lastEnemySeen = enemyEnt; //keep track of this so that the suspicious UI knows when I am suspcicious at player.
		lastFovCheck = true;
	}
	else
	{
        lastEnemySeen = NULL; //keep track of this so that the suspicious UI knows when I am suspcicious at player.
		lastFovCheck = false;
	}

	if (gameLocal.time > suspicionIntervalTimer && lastFovCheck)
	{	
		//I can see enemy.
		//Ok, we can see the enemy's eyes. We want to ramp up the suspicious pips depending on how exposed the enemy is.

		int amountExposed = 1;
		bool isInImmediateProximity = false;
		

		//How many limbs is the enemy exposing to me.
		int limbsExposed = enemyEnt->IsType(idActor::Type) ? GetSightExposure(static_cast<idActor *>(enemyEnt)) : 1;

		// SW 26th March 2025:
		// Sigh. Okay. Enemies will typically react very slowly to the player when they see Nina floating around outside the ship,
		// because the player's number of exposed limbs is treated as "0" (the check doesn't account for glass in the way).
		// We're running out of time here, so I'm going to implement the fast and stupid solution:
		// If the player is in outer space, just assume we can fully see them.
		if (enemyEnt->isInOuterSpace())
		{
			limbsExposed = MAX_SIGHTLIMBS;
		}

		float distanceToEnemy = (enemyEnt->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
		if (distanceToEnemy <= SUSPICION_IMMEDIATEPROXIMITY)
		{
			if (distanceToEnemy <= SUSPICION_EXTREMEIMMEDIATEPROXIMITY)
			{
				amountExposed = SUSPICION_EXTREMEIMMEDIATEPROXIMITY_RATE;
			}
			else
			{
				amountExposed = SUSPICION_IMMEDIATEPROXIMITY_RATE;
			}
			
			isInImmediateProximity = true;
			limbsExposed = MAX_SIGHTLIMBS; //if very close to me, then assume I can see all enemy limbs.
		}
		else
		{	
			//is my enemy on the move.
			float enemySpeed = enemyEnt->GetPhysics()->GetLinearVelocity().LengthFast();

			if (enemySpeed > 0 && limbsExposed < MAX_SIGHTLIMBS)
			{
				//If enemy is MOVING, and enemy is partially hidden, then keep suspicion increase rate low. Do nothing here. Let amountExposed remain its minimum amount.
			}		
			else
			{
				amountExposed += limbsExposed;

				if (limbsExposed > 1)
					amountExposed += 2; //as more limbs are increased, the suspicion counter increases non-linearly.


				if (enemySpeed <= 1 && limbsExposed >= MAX_SIGHTLIMBS)
				{
					//This penalty is only applied if I'm in my target's front 180 cone (my target can feasibly see me).
					//If my target is turned away from me, I'm lenient and don't apply this penalty.
					if (IsTargetLookingAtMe(enemyEnt))
					{
						//if target not moving and totally exposed, then penalty is increased suspicion increase.
						amountExposed += SUSPICION_NOTMOVING_PENALTY;
					}
				}
			}
		}




		//if in darkness, then exit suspicious state
		//if (enemyEnt == gameLocal.GetLocalPlayer())
		//{
		//	//enemy is suspicious of player. Handle the darkness/lightness modifier.
		//	int darknessValue = TargetIsInDarkness(enemyEnt);
		//	
		//	if (darknessValue)
		//	{
		//		//player is in dark. Decelerated suspicion.
		//		//amountExposed /= 4;
		//
		//		//player is in dark. exit.
		//		suspicionCounter = 0;
		//		lastEnemySeen = NULL;
		//		Event_RestoreMove();
		//		GotoState(lastState);
		//		return;
		//	}
		//}

		//In combat state or search state, the suspicion rate is increased.
		//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT || static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_SEARCH)
		//{
		//	amountExposed *= SUSPICION_ALERTMULTIPLIER;
		//}


		//If target is moving TOWARD ME, then suspicion rate is increased.
		//This is to handle aggro players just bulldozing straight into enemies.
		if (limbsExposed >= MAX_SIGHTLIMBS)
		{
			float enemyVelocityLength = enemyEnt->GetPhysics()->GetLinearVelocity().Length();
			if (enemyVelocityLength > 0)
			{
				idVec3 enemyVelocityDir = enemyEnt->GetPhysics()->GetLinearVelocity();
				enemyVelocityDir.Normalize();

				idVec3 dirToEnemy = this->GetPhysics()->GetOrigin() - enemyEnt->GetPhysics()->GetOrigin();
				dirToEnemy.Normalize();

				float vdot = DotProduct(dirToEnemy, enemyVelocityDir);
				if (vdot > SUSPICION_APPROACHPENALTY_DOTTHRESHOLD)
				{
					//This penalty is only applied if I'm in my target's front 180 cone (my target can feasibly see me).
					//If my target is turned away from me, I'm lenient and don't apply this penalty.
					if (IsTargetLookingAtMe(enemyEnt))
					{
						amountExposed += SUSPICION_APPROACHPENALTY_PIPAMOUNT;
					}
				}
			}
		}

		
		//4/19/2023 have light meter adjust suspicion rate.
		if (enemyEnt == gameLocal.GetLocalPlayer() && !isInImmediateProximity)
		{
			int playerLuminanceState = gameLocal.GetLocalPlayer()->GetHiddenStatus();

			if (playerLuminanceState == LIGHTMETER_SHADOWY)
			{
				//Player is in shadowy state. make the suspicion rate be very slow.
				amountExposed = 1;
			}
		}

		//g_perceptionScale scaler.
		float perceptionScale = g_perceptionScale.GetFloat();
		if (perceptionScale == 1)
		{
			//do nothing.
		}
		else if (perceptionScale > 0)
		{
			amountExposed = max(1, amountExposed * perceptionScale);
		}
		else
		{
			amountExposed = 0;
		}


		
		
		//common->Printf("exposed %d\n", amountExposed);


		//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_COMBAT)
		//{
		//	//if world is in combat state, then suspicion increase rate is higher.
		//	amountExposed = max(amountExposed, 3);
		//}



		//If player is in mech, increase suspicion rate.
		if (enemyEnt->IsType(idMech::Type))
		{
			amountExposed *= SUSPICION_MULTIPLIER_PLAYER_IN_MECH;
		}

		if (enemyEnt->IsType(idPlayer::Type))
		{
			if (static_cast<idPlayer*>(enemyEnt)->IsJockeying())
			{
				amountExposed *= SUSPICION_MULTIPLIER_PLAYER_JOCKEYING;
			}
		}

		//When player gets knocked off from jockey, make the suspicion rate increase dramatically.
		if (highlySuspiciousTimer > gameLocal.time && enemyEnt->IsType(idPlayer::Type))
		{
			amountExposed *= SUSPICION_MULTIPLIER_HIGHLYSUSPICIOUS;
		}


		
		suspicionCounter += amountExposed; //Increase suspicionCounter.


		


		//Turn head toward suspicious thing.
		idVec3 enemyEyePos = GetEyePositionIfPossible(enemyEnt);
		LookAtPointMS(enemyEyePos, SUSPICION_THINKINTERVAL + 100); //Look at the enemy.
		
		lastSoftfailPosition = enemyEyePos;

		if (move.moveStatus != MOVE_STATUS_MOVING)
		{
			TurnToward(enemyEyePos); //If I'm standing still, then turn toward what caught my eye.
		}


			
		if (suspicionCounter >= SUSPICION_MAXPIPS)
		{
			//I can see enemy. Suspicion is maxed out. Go to combat state.
			lastFireablePosition = GetEyePositionIfPossible(enemyEnt);
			lastEnemySeen = enemyEnt;
			GotoState(AISTATE_COMBAT);
			
			if (enemyEnt != NULL)
			{
				if (enemyEnt == gameLocal.GetLocalPlayer())
				{
					sighted_flashTimer = gameLocal.time + SIGHTED_FLASHTIME; //I see the enemy. Display the sighted flash UI.
				}
			}
			return;
		}

		suspicionIntervalTimer = gameLocal.time + SUSPICION_THINKINTERVAL;
	}

	//if (gameLocal.time > suspicionTimer)
	//{
	//	//Suspicious timer expired. Return to idle state.
	//	Event_RestoreMove();
	//	GotoState(lastState);
	//}
	
	//If I am moving, then stop moving so that I don't accidentally pass by suspicious thing.
	if (move.moveStatus == MOVE_STATUS_MOVING && suspicionCounter > 0)
	{
		StopMove(MOVE_STATUS_WAITING);
	}

	//common->Printf("%d    fovcheck: %d\n", suspicionCounter, lastFovCheck);
	if (gameLocal.time > suspicionIntervalTimer && !lastFovCheck)
	{
		suspicionCounter -= SUSPICION_DECAYRATE;
		suspicionIntervalTimer = gameLocal.time + 50;

		if (gameLocal.time > softfailCooldown && suspicionCounter > (SUSPICION_MAXPIPS * SOFTFAIL_THRESHOLD))
		{
			softfailCooldown = gameLocal.time + SOFTFAIL_COOLDOWNTIME;
			suspicionCounter = 0; // SW 20th March 2025: Canceling out suspicion counter so they immediately notice the interestpoint (enemies no longer care about interestpoints while in suspicion state)
			gameLocal.SpawnInterestPoint(gameLocal.GetLocalPlayer(), lastSoftfailPosition, "interest_player");
		}

		if (suspicionCounter <= 0 && gameLocal.time > suspicionStartTime + SUSPICION_MINIMUMTIME) //Stay in suspicion mode for a mandatory minimum amount of time.
		{
			Event_RestoreMove();
			GotoState(lastState);
		}
	}

	UpdatePickpocketReaction();
}

//Is this entity potentially at me (am I within their 180 front cone).
bool idGunnerMonster::IsTargetLookingAtMe(idEntity* enemyEnt)
{
	if (enemyEnt == nullptr)
		return false;

	idVec3 enemyLookDir;

	if (enemyEnt == gameLocal.GetLocalPlayer())
	{
		gameLocal.GetLocalPlayer()->viewAngles.ToVectors(&enemyLookDir, NULL, NULL);
	}
	else if (enemyEnt->IsType(idActor::Type))
	{
		static_cast<idActor*>(enemyEnt)->viewAxis.ToAngles().ToVectors(&enemyLookDir, NULL, NULL);
	}
	else
	{
		return false;
	}

	idVec3 dirToEnemy = enemyEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	dirToEnemy.Normalize();

	float facingResult = DotProduct( enemyLookDir, dirToEnemy);

	return (facingResult < 0);
}

void idGunnerMonster::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	//bc if take damage while in bleed out state.
	if (bleedoutState == BLEEDOUT_ACTIVE && gameLocal.time > bleedoutDamageTimer)
	{
		int timeDelta = bleedoutTime - gameLocal.time;

		if (timeDelta < (spawnArgs.GetInt("bleedouttime", "60") * 1000))
		{
			bleedoutTime -= (damage * BLEEDOUT_DAMAGEMULTIPLIER);

			bleedoutDamageTimer = gameLocal.time + BLEEDOUT_DAMAGEDELAY;

			if (bleedoutTime < 1000) //always leave at least one second on the bleedout timer.
				bleedoutTime = 1000;
		}
	}

	//Jockey logic.
	if (gameLocal.GetLocalPlayer()->IsJockeying())
	{
		if (gameLocal.GetLocalPlayer()->meleeTarget.IsValid())
		{
			if (gameLocal.GetLocalPlayer()->meleeTarget.GetEntityNum() == this->entityNumber)
			{
				//I am being jockeyed.
				gameLocal.GetLocalPlayer()->SetJockeyMode(false);
			}
		}
	}

	


    if (AI_DEAD)
        return;

	//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->IncreaseEscalationLevel();

	idAI::Killed(inflictor, attacker, damage, dir, location);

	SetColor(COLOR_DEAD); //make my color become the 'dead' color.

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_dead"))); //death skin.

	lastFovCheck = false;
	SetCombatState(0);

	SetFlashlight(false);

	//if currently investigating an interestpoint...
	if (lastInterest.IsValid())
	{
		if (lastInterest.GetEntity() != NULL)
		{
			// We are distracted (in a sense -- we're distracted by our own mortality)
			static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
		}
	}

	gameLocal.voManager.SayVO(this, "snd_vo_death", VO_CATEGORY_DEATH); //"oh no I'm dead!!!!"

	
	//Drop any attached skullsavers.
	for (idEntity* skullsaverEnt = gameLocal.skullsaverEntities.Next(); skullsaverEnt != NULL; skullsaverEnt = skullsaverEnt->skullsaverNode.Next())
	{
		if (!skullsaverEnt)
			continue;

		if (!skullsaverEnt->IsType(idSkullsaver::Type) || skullsaverEnt->GetBindMaster() == NULL)
			continue;

		if (skullsaverEnt->GetBindMaster() == this)
		{
			//I am the AI that was carrying this skullsaver. Drop the skullsaver.
			static_cast<idSkullsaver *>(skullsaverEnt)->DetachFromEnemy();
		}
	}

	//if (DoesPlayerHaveLOStoMe())
	{
		if (jockattackState == JOCKATK_NONE)
		{
			if (!static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->DoHighlighter(this, inflictor))
			{
				gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);
			}
		}
	}



	DetachPointdefense();

	if (attacker != NULL)
	{
		if (attacker == gameLocal.GetLocalPlayer())
		{
			//BC 2-14-2025: determine what "nina has just killed enemy" vo line to play.
			//there are 3 variants:
			//- opencombat: world is in combat state.
			//- stealth: world is unaware of nina.
			//- kill: everything else.

			idStr desiredVO = "";
			bool isWorldIdlestate = (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_IDLE);

			if (!isWorldIdlestate)
			{
				//World is NOT in idle state. So do the combat variant.
				desiredVO = "snd_vo_kill_opencombat";
			}
			else if (isWorldIdlestate && !static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetEnemiesKnowOfNina())
			{
				//Idle state, and enemies don't know about nina yet.
				desiredVO = "snd_vo_kill_stealth";
			}
			else
			{
				desiredVO = "snd_vo_kill";
			}

			//Play player VO: "I killed an enemy"
			gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay_msDelayed(desiredVO.c_str(), 500);
		}
	}

	common->g_SteamUtilities->SetSteamTimelineEvent("steam_death");
}



idEntityPtr<idEntity> idGunnerMonster::GetLastInterest(void)
{
	return lastInterest;
}

void idGunnerMonster::Resurrect()
{
	//Go to search state and move to a new position immediately.
	GotoState(AISTATE_SEARCHING);
	searchMode = SEARCHMODE_PANNING;
	searchWaitTimer = 0;
	skullEnt = NULL;

	idAI::Resurrect();
}

void idGunnerMonster::UpdateLKP(idEntity *enemyEnt)
{
	static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPPositionByEntity(enemyEnt);
}


//TODO: check if a fellow ai is in my firing arc, and stop firing if so.
void idGunnerMonster::State_Combat()
{
	idEntity *enemyEnt;
	enemyEnt = FindEnemyAIVisible();

	if (enemyEnt == NULL)
	{
		//If no enemy found, then see if I was damaged within the last 1000ms. If so, use that as the enemy.
		if (lastDamagedTime + 1000 >= gameLocal.time && lastAttacker.IsValid())
		{
			enemyEnt = lastAttacker.GetEntity();
		}
	}


	//This is what does the combat alert, sets global combat state, alerts other AI. We have a delay so that the player has time to 'quietly' eliminate the AI (instead of
	//the world immediately going into combat state).
	if (gameLocal.time > lastDamagedTime + COMBATALERT_DELAYTIME && !hasAlertedFriends)
	{
		if (enemyEnt != NULL)
		{
			if (enemyEnt == gameLocal.GetLocalPlayer())
			{
				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_seen_player"), displayName.c_str()), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
			}
		}

		hasAlertedFriends = true;

		Eventlog_StartCombatAlert();
		SetCombatState(1, true);
	}


	if (enemyEnt)
	{
		if (move.moveStatus == MOVE_STATUS_MOVING)
		{
			//if moving, then stop moving.
			StopMove(MOVE_STATUS_DONE);
		}

		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->OnEnemySeeHostile(); //I saw enemy, so refresh the combat world state timer.
		

		//I can see enemy with my EYES.

		idVec3 fireablepositionCandidate = GetEyePositionIfPossible(enemyEnt);
		if (fireablepositionCandidate != vec3_zero)
		{
			lastFireablePosition = fireablepositionCandidate;
		}

		TurnToward(lastFireablePosition);
		LookAtPointMS(lastFireablePosition, 100);

		//Aim laser at the enemy.
		idVec3 newLaserPosition = lastFireablePosition;
		if (enemyEnt->IsType(idActor::Type))
		{
			float aimAheadDistance = 16.0f;
			
			//If jockeying, then do some adjustments to the aim position.
			if (enemyEnt == gameLocal.GetLocalPlayer())
			{
				if (gameLocal.GetLocalPlayer()->IsJockeying())
				{
					aimAheadDistance = 1.0f;
					newLaserPosition += idVec3(0, 0, -8);
				}

				if (gameLocal.GetLocalPlayer()->GetFallenState())
				{
					newLaserPosition += idVec3(0, 0, -8); //if player is in fallen state, then aim lower.
				}
			}		
			
			// Do basic target prediction based on the enemy's velocity
			float projectileTravelTime = (newLaserPosition - GetPhysics()->GetOrigin()).Length() / projectileSpeed;
			float predictedTime = ai_targetPredictTime.GetFloat() + projectileTravelTime;
			newLaserPosition += enemyEnt->GetPhysics()->GetLinearVelocity() * predictedTime;

			// OLD CODE kept for now
// 			float actorMovespeed = enemyEnt->GetPhysics()->GetLinearVelocity().Length();
// 			if (actorMovespeed > 10)
// 			{
// 				//Enemy target is MOVING. Aim slightly ahead of the target. TODO: this is currently just a hardcoded number. May need to make this more robust.
// 				idVec3 actorMoveDir = enemyEnt->GetPhysics()->GetLinearVelocity();
// 				actorMoveDir.NormalizeFast();
// 				newLaserPosition = newLaserPosition + (actorMoveDir * aimAheadDistance);
// 			}			
		}
		Event_SetLaserLock(newLaserPosition);

		combatFiringTimer = gameLocal.time + COMBAT_FIRINGTIME; //Replenish the firing burst timer.		
		lastEnemySeen = enemyEnt;

		//Update LKP stuff.
		lastVisibleEnemyPos = enemyEnt->GetPhysics()->GetOrigin();
		UpdateLKP(enemyEnt);

		if (!lastFovCheck)
		{
			//if I gained sight of enemy, update the LKP system.
			lastFovCheck = true;
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(lastFovCheck);
		}

		if (gameLocal.time > specialTraverseTimer)
		{
			specialTraverseTimer = gameLocal.time + SPECIALTRAVERSETIMER;

			if (!CanHitFromAnim("ranged_attack", lastFireablePosition))
			{
				if (TestAnimMove("ranged_attack_stepleft10") && CanHitFromAnim("ranged_attack_stepleft10", lastFireablePosition))
				{
					AI_LEFT = true;
				}
				else if (TestAnimMove("ranged_attack_stepright10") && CanHitFromAnim("ranged_attack_stepright10", lastFireablePosition))
				{
					AI_RIGHT = true;
				}
			}
		}
		
		if ((gameLocal.GetLocalPlayer()->GetDefibState() && enemyEnt == gameLocal.GetLocalPlayer()) || (enemyEnt->health <= 0 && !enemyEnt->IsType(idWorldspawn::Type)))
		{
			GotoState(AISTATE_VICTORY);
		}

		//Check for melee attack.
		if (TestMelee() && gameLocal.time > meleeAttackTimer)
		{
			if (meleeModeActive == false)
			{
				meleeModeActive = true;
				meleeChargeTimer = gameLocal.time + MELEE_CHARGETIME;
			}
			else if (gameLocal.time >  meleeChargeTimer)
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_kick", 4);
				meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;
				meleeModeActive = false;
			}
		}
		else
		{
			meleeModeActive = false;
		}
	}
	else 
	{
		//I can NOT see the enemy.

		if (gameLocal.time > combatFiringTimer)
		{
			//The firing burst timer has expired.
			GotoState(AISTATE_COMBATOBSERVE);
		}
		else
		{
			LookAtPointMS(lastFireablePosition, 100); //Continue looking (and firing) at the last place we fired at.
		}

		if (lastEnemySeen.IsValid())
		{
			if (CanSee(lastEnemySeen.GetEntity(), false))
			{
				//The enemy we were attacking is now dead.
				GotoState(AISTATE_COMBATOBSERVE);
				return;
			}
		}

		if (lastFovCheck)
		{
			//if I lost sight of enemy, update the LKP system.
			lastFovCheck = false;
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(lastFovCheck);
		}		
	}

	UpdatePickpocketReaction();
}


#define COMBATOBSERVE_STEPLEFT40		0
#define COMBATOBSERVE_STEPRIGHT40		1
#define COMBATOBSERVE_STEPBACK40		2
#define COMBATOBSERVE_STEPBACKLEFT40	3
#define COMBATOBSERVE_STEPBACKRIGHT40	4

#define FIDGET_MINIMUMTIME				1300
#define FIDGET_RANDOMVARIATION			2000

void idGunnerMonster::State_CombatObserve()
{
	//Just finished firing weapon at a location. Keep looking at the lastfireableposition, do some idle sidesteps or idle walking backward.

	//Eyeballs.
	idEntity *enemyEnt;

	enemyEnt = FindEnemyAIVisible();

	if (enemyEnt && CanHitFromAnim("ranged_attack", lastFireablePosition))
	{
		//I see enemy. Go to combat mode.
		lastEnemySeen = enemyEnt;
		GotoState(AISTATE_COMBAT);
		return;
	}

	if (gameLocal.time > fidgetTimer)
	{
		idList<int> fidgetCandidates;
		
		if (CanHitFromAnim("aimed_stepleft40", lastFireablePosition) && TestAnimMove("aimed_stepleft40"))			{ fidgetCandidates.Append(COMBATOBSERVE_STEPLEFT40); }
		if (CanHitFromAnim("aimed_stepright40", lastFireablePosition) && TestAnimMove("aimed_stepright40"))			{ fidgetCandidates.Append(COMBATOBSERVE_STEPRIGHT40);}
		if (CanHitFromAnim("aimed_stepback40", lastFireablePosition) && TestAnimMove("aimed_stepback40"))			{ fidgetCandidates.Append(COMBATOBSERVE_STEPBACK40); }
		if (CanHitFromAnim("aimed_stepbackleft40", lastFireablePosition) && TestAnimMove("aimed_stepbackleft40"))	{ fidgetCandidates.Append(COMBATOBSERVE_STEPBACKLEFT40); }
		if (CanHitFromAnim("aimed_stepbackright40", lastFireablePosition) && TestAnimMove("aimed_stepbackright40"))	{ fidgetCandidates.Append(COMBATOBSERVE_STEPBACKRIGHT40); }
		
		if (fidgetCandidates.Num() > 0)
		{
			//pick a random anim to play.
			int randomFidget = fidgetCandidates[gameLocal.random.RandomInt(fidgetCandidates.Num())];

			if (randomFidget == COMBATOBSERVE_STEPLEFT40)		{ SetAnimState(ANIMCHANNEL_LEGS, "Legs_StepLeft40", 4); }
			if (randomFidget == COMBATOBSERVE_STEPRIGHT40)		{ SetAnimState(ANIMCHANNEL_LEGS, "Legs_StepRight40", 4); }
			if (randomFidget == COMBATOBSERVE_STEPBACK40)		{ SetAnimState(ANIMCHANNEL_LEGS, "Legs_StepBack40", 4); }
			if (randomFidget == COMBATOBSERVE_STEPBACKLEFT40)	{ SetAnimState(ANIMCHANNEL_LEGS, "Legs_StepBackLeft40", 4); }
			if (randomFidget == COMBATOBSERVE_STEPBACKRIGHT40)	{ SetAnimState(ANIMCHANNEL_LEGS, "Legs_StepBackRight40", 4); }
		}

		fidgetTimer = gameLocal.time + FIDGET_MINIMUMTIME + gameLocal.random.RandomInt(FIDGET_RANDOMVARIATION);
	}
	
	if (enemyEnt)
	{
		//Only update this pets/grenades if enemy is valid.
		UpdateGrenade();
		UpdatePet();
	}

	trace_t lastFireablePositionTr;
	gameLocal.clip.TracePoint(lastFireablePositionTr, GetEyePosition(), lastFireablePosition, MASK_SOLID, this);

	if (gameLocal.time > stateTimer || lastFireablePositionTr.fraction < .9f)
	{
		//Observation time has expired, or, no longer have LOS to the lastfireableposition. Start the stalking state.
		GotoState(AISTATE_COMBATSTALK);
		return;
	}
	else if (gameLocal.time > intervalTimer)
	{
		LookAtPointMS(lastFireablePosition, OBSERVEINTERVAL + 50);
		intervalTimer = gameLocal.time + OBSERVEINTERVAL;
	}

	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
		AI_SHIELDHIT = false;
		lastFireablePosition = lastAttackerDamageOrigin;
		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;
		GotoState(AISTATE_COMBAT);
	}


	UpdatePickpocketReaction();
}

#define GRENADE_DANGERZONE 160			//Only throw grenade if no ally is within XX units of grenade destination.
#define GRENADE_SUCCESSCOOLDOWN 10000	//After successful grenade throw, wait XX time until next throw.
#define GRENADE_FAILCOOLDOWN	2000	//If grenade fails to throw for whatever reason, try the grenade check again after XX time.

void idGunnerMonster::UpdateGrenade()
{
	if (gameLocal.time > gameLocal.nextGrenadeTime)
	{
		//a grenade throw event is available....

		const char *grenadeDefName = spawnArgs.GetString("def_grenade");

		if (grenadeDefName[0] == '\0')
		{
			return; //no grenade. get outta here.
		}

		const idDeclEntityDef *grenadeDef = gameLocal.FindEntityDef(grenadeDefName, false);

		if (!grenadeDef)
		{
			gameLocal.Error("'%s' has invalid grenade definition: '%s'\n", GetName(), grenadeDefName); //grenade def doesn't exist... error out.
			return;
		}

				
		if (grenadeDef->dict.GetBool("splashdamage_check", "1")) //some grenades don't care about friendlies, i.e. the seeker grenade.
		{
			//Check if there's allies (including myself) in the splash zone.
			for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
			{
				if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0)		//sanity check
					continue;

				if (!entity->IsType(idAI::Type) || entity->team != this->team)
					continue;

				//ok, we have a bad guy now.
				float distToEnemy = (lastFireablePosition - entity->GetPhysics()->GetOrigin()).LengthFast();

				if (distToEnemy <= GRENADE_DANGERZONE)
				{
					gameLocal.nextGrenadeTime = gameLocal.time + GRENADE_FAILCOOLDOWN;
					return; //An ally is too close to grenade destination. Exit.
				}
			}
		}

		
		//Spawn grenade.
		idDict args;
		idEntity *grenadeEnt;

		args.Clear();
		args.SetInt("team", this->team);
		args.Set("owner", this->GetName());
		args.Set("classname", spawnArgs.GetString("def_grenade"));
		gameLocal.SpawnEntityDef(args, &grenadeEnt);

		//attempt to throw grenade.
		if (grenadeEnt)
		{
			idVec3 throwSpot;
			trace_t throwSpotTr;

			//See if there's a nearby spot on ground we should be throwing to.
			gameLocal.clip.TracePoint(throwSpotTr, lastFireablePosition, lastFireablePosition + idVec3(0, 0, -80), CONTENTS_SOLID, this);

			if (throwSpotTr.fraction >= 1)
			{
				throwSpot = lastFireablePosition;
			}
			else
			{
				throwSpot = throwSpotTr.endpos;
			}

			bool shouldThrow;
			if (grenadeDef->dict.GetBool("throwarc_check", "1")) //some grenades don't care about a clear/clean throw arc (i.e. seeker grenade doesn't care)
			{
				shouldThrow = Event_ThrowObjectAtPosition(grenadeEnt, throwSpot);
			}
			else
			{
				//force spawn the grenade. Don't care about arc.
				idVec3 throwOrigin = GetPhysics()->GetOrigin() + idVec3(0, 0, 70) + (viewAxis.ToAngles().ToForward() * 16);
				idVec3 throwDir = idVec3(throwSpot.x, throwSpot.y, throwOrigin.z + 32) - throwOrigin;
				throwDir.Normalize();

				grenadeEnt->SetOrigin(throwOrigin);
				grenadeEnt->GetPhysics()->SetLinearVelocity(throwDir * 512);

				shouldThrow = true;
			}

			if (shouldThrow)
			{
				//Successful throw.
				gameLocal.nextGrenadeTime = gameLocal.time + GRENADE_SUCCESSCOOLDOWN; //Cooldown until next throw.
				SetAnimState(ANIMCHANNEL_TORSO, "Torso_Grenadethrow", 4);

				gameLocal.voManager.SayVO(this, "snd_vo_grenadethrow", VO_CATEGORY_BARK); //"throwing grenade!"

				return;
			}
			else
			{
				//Throw fail.
				grenadeEnt->PostEventMS(&EV_Remove, 0);
			}
		}

		//Fail. Try again in a short time.
		gameLocal.nextGrenadeTime = gameLocal.time + GRENADE_FAILCOOLDOWN;
	}
}

void idGunnerMonster::State_CombatStalk()
{
	if (!combatStalkInitialized)
	{
		idVec3 LKPReachablePosition = vec3_zero;

		combatStalkInitialized = true;
		LKPReachablePosition = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetLKPReachablePosition();

		if (!MoveToPosition(LKPReachablePosition))
		{
			//Cannot move directly there.

			//Attempt to find LOS to the LKP.
			LKPReachablePosition = FindValidPosition(LKPReachablePosition);

			if (!MoveToPosition(LKPReachablePosition))
			{
				//Failed this attempt as well.
				//Cannot path to the position.
				GotoState(AISTATE_SEARCHING);
				return;
			}
		}		
	}

	if (move.moveStatus == MOVE_STATUS_DONE)
	{
		//We arrived at the destination.
		GotoState(AISTATE_SEARCHING);
	}
	else
	{
		//On the move.

		//Eyeballs.
		idEntity *enemyEnt;

		enemyEnt = FindEnemyAIVisible();

		if (enemyEnt && CanHitFromAnim("ranged_attack", GetEyePositionIfPossible(enemyEnt)))
		{
			//I see enemy. Go to combat mode.
			lastEnemySeen = enemyEnt;
			GotoState(AISTATE_COMBAT);
			return;
		}

		if (gameLocal.time > intervalTimer)
		{
			intervalTimer = OBSERVEINTERVAL;

			if (CheckSearchLook(lastFireablePosition, 1, true))
			{
				LookAtPointMS(lastFireablePosition, OBSERVEINTERVAL + 50);

				//we don't want the ai to stop directly on top of the point, since it looks kinda awkard.
				//so do a distance check and stop when we're pretty close to the point.

				if ((move.moveDest - GetPhysics()->GetOrigin()).LengthFast() <= ARRIVAL_DISTANCE)
				{
					static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPVisible(false);
					StopMove(MOVE_STATUS_DONE);

					//Tell any overwatch'd friends to start search.
					OnArriveAtLKP();
				}
			}
		}
	}

	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
		AI_SHIELDHIT = false;
		lastFireablePosition = lastAttackerDamageOrigin;
		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;
		GotoState(AISTATE_COMBAT);
	}


	UpdatePickpocketReaction();
}

void idGunnerMonster::State_Searching()
{
	idEntity *enemyEnt;

	enemyEnt = FindEnemyAIVisible();

	if (enemyEnt)
	{
		//if (!TargetIsInDarkness(enemyEnt))
		{
			//I see enemy. Go to combat mode.
			lastFovCheck = true;
			lastEnemySeen = enemyEnt;

			suspicionIntervalTimer = gameLocal.time + SUSPICION_THINKINTERVAL;
			suspicionCounter = 0;
			suspicionStartTime = gameLocal.time;
			GotoState(AISTATE_SUSPICIOUS);
			return;
		}
	}

	lastFovCheck = false;

	if (searchMode == SEARCHMODE_SEEKINGPOSITION)
	{
		if (move.moveStatus != MOVE_STATUS_MOVING && gameLocal.time > searchWaitTimer)
		{
			//We are STOPPED. When investigating an interestpoint, this gets called when the AI arrives at the interestpoint.

			searchMode = SEARCHMODE_OBSERVING;
			searchWaitTimer = gameLocal.time + 1500; //stay for a short time. This gets extended if AI_CUSTOMIDLEANIM is played.

			if (lastFireablePosition != vec3_zero)
			{
				TurnToward(lastFireablePosition); //Turn to face the last fireable pos.
			}

			//Check if Interestpoint was reached.
			if (lastInterest.IsValid())
			{
				if (lastInterest.GetEntity() != NULL)
				{
					trace_t interestTr;

					gameLocal.clip.TracePoint(interestTr, GetEyePosition(), lastInterest.GetEntity()->GetPhysics()->GetOrigin(), MASK_SOLID, this);

					float distToInterestpoint = (interestTr.endpos - lastInterest.GetEntity()->GetPhysics()->GetOrigin()).Length();

					if (distToInterestpoint <= 32)
					{
						//I have stopped moving and have line of sight to the interestpoint I was investigating.

						AI_CUSTOMIDLEANIM = true; //this flag makes me play the 'investigate' animation. in monster_gunner4.script , this is handled in Torso_Idle_Combat()
						customIdleAnim = "rifle_investigate";
						searchWaitTimer = gameLocal.time + SEARCH_WAITTIME;

						idInterestPoint* lastInterestEnt = static_cast<idInterestPoint*>(lastInterest.GetEntity());
						int frobIndex = lastInterestEnt->spawnArgs.GetInt("frobindex", "0");
						idEntity* interestOwner = lastInterestEnt->interestOwner.GetEntity();

						//TODO: 4/19/2023 crash seems to happen here where interestowner is sometimes garbage info
						if (!interestOwner || interestOwner->IsRemoved() || !interestOwner->GetPhysics())
						{
							common->Warning("Interest owner of %s is null/removed??", lastInterestEnt->GetName());
						}

						//I just arrived at my lastInterest interestpoint.
						if (frobIndex > 0 && interestOwner && !interestOwner->IsRemoved() && interestOwner->GetPhysics())
						{
							bool isPlayerHolding = gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->HasEntityInCarryableInventory(interestOwner);
							if (!isPlayerHolding)
							{
								//Do distance check.
								float distanceToInterest = (interestOwner->GetPhysics()->GetOrigin() - GetEyePosition()).Length();
								if (distanceToInterest <= INTERESTFROB_DISTANCE)
								{
									interestOwner->DoFrob(frobIndex, this); //Frob the object.
									gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_enemyuse"), displayName.c_str(), interestOwner->displayName.c_str()), interestOwner->GetPhysics()->GetOrigin());
								}
							}
						}

						static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointInvestigated(lastInterest.GetEntity());

						//Start radio checkin sequence.
						radiocheckinTimer = gameLocal.time + RADIOCHECK_THRESHOLDTIME;
						radiocheckinPrimed = true;
					}
				}
			}

			//If we have arrived at LKP.
			if (!static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->lkpEnt->IsHidden())
			{
				//Get distance between my feet and LKPReachable.
				float LKPDist = (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetLKPReachablePosition() - GetPhysics()->GetOrigin()).LengthFast();
				if (LKPDist <= ARRIVAL_DISTANCE)
				{
					//Pretty dang close to the LKP. Hide the LKP.
					static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPVisible(false);
					
					//Tell any overwatch'd friends to start search.
					OnArriveAtLKP();
				}
			}
		}
	}
	else if (searchMode == SEARCHMODE_OBSERVING)
	{
		if (gameLocal.time > searchWaitTimer)
		{
			searchMode = SEARCHMODE_PANNING;
			searchWaitTimer = gameLocal.time + SEARCH_INVESTIGATE_PANTIME;

			AI_CUSTOMIDLEANIM = true; //this flag makes me play the 'rifle_investigate_pan' animation. in monster_gunner4.script , this is handled in Torso_Idle_Combat()
			customIdleAnim = "rifle_investigate_pan";

			lastFireablePosition = vec3_zero; //I'm done with looking at object. Reset my last fireable position.

			//idVec3 eyeForward = viewAxis.ToAngles().ToForward();
			//Event_LookAtPoint(GetEyePosition() + eyeForward * 2048, .1f); //Reset  the look function.
			//gameRenderWorld->DebugArrowSimple(GetEyePosition() + eyeForward * 64);

			//12/6/2023 - instead of staying in search state, return to idle state after investigating interestpoint.
			Event_ResetLookPoint();
			Event_SetLaserLock(vec3_zero);
			AI_CUSTOMIDLEANIM = false;
			AI_NODEANIM = false;
			//customIdleAnim = "";

			if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT)
			{
				//lastInterest = NULL;
				//lastInterestPriority = 0;
				lastFireablePosition = vec3_zero;
				searchWaitTimer = 0;

				idEntity *searchnode;
				searchnode = GetBiasedSearchNode();
				if (searchnode)
				{
					//Found a valid search node. Go to it.
					if (!MoveToPosition(searchnode->GetPhysics()->GetOrigin()))
					{					
						WanderAround();
					}
				}

				GotoState(AISTATE_SEARCHING);
			}
			else
			{


				bool radiocheckinActive = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->radioCheckinEnt->CheckinActive();
				if (!radiocheckinActive)
				{
					GotoState(AISTATE_IDLE);
				}
			}
		}
	}
	else if (searchMode == SEARCHMODE_PANNING)
	{
		//Stopped, observing.

		if (gameLocal.time > searchWaitTimer)
		{
			idEntity *searchnode;

			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointInvestigated(lastInterest.GetEntity());
			}

			searchWaitTimer = gameLocal.time + SEARCH_WAITTIME;
			lastFireablePosition = vec3_zero; //Deactivate the last fireable position.

			if (firstSearchnodeHint)
			{
				firstSearchnodeHint = false;

				//Has JUST entered search state. Bias it toward the enemy location.
				searchnode = GetBiasedSearchNode();
			}
			else
			{
				//Get a searchpoint to go to.
				searchnode = GetSearchNode();
			}

			if (searchnode)
			{
				//Found a valid search node. Go to it.
				if (!MoveToPosition(searchnode->GetPhysics()->GetOrigin()))
				{
					//Failed to move to the searchnode for some reason... do a wander fallback.
					if (hasPathNodes)
						HasArriveAtPathNode();
					else
						WanderAround();
				}
			}
			else
			{
				//Failed to find a valid search node... do a wander fallback.
				if (hasPathNodes)
					HasArriveAtPathNode();
				else
					WanderAround();
			}

			searchMode = SEARCHMODE_SEEKINGPOSITION;
			searchWaitTimer = gameLocal.time + 1000; //buffer time before we start detecting if ai has stopped moving in SEARCHMODE_SEEKINGPOSITION state.
		}
	}

	//Look at the lastfireableposition.
	if (gameLocal.time > intervalTimer && lastFireablePosition != vec3_zero)
	{
		intervalTimer = OBSERVEINTERVAL;

		if (CheckSearchLook(lastFireablePosition, 1, false))
		{
			LookAtPointMS(lastFireablePosition, OBSERVEINTERVAL + 50);

			//If we're pretty close to the point, then stop early.
			if (searchMode == SEARCHMODE_SEEKINGPOSITION)
			{
				if (lastInterest.IsValid())
				{
					if (lastInterest.GetEntity()->IsType(idInterestPoint::Type))
					{
						int arrivalDistance = static_cast<idInterestPoint *>(lastInterest.GetEntity())->arrivalDistance;

						if ((move.moveDest - GetPhysics()->GetOrigin()).LengthFast() <= arrivalDistance)
						{
							StopMove(MOVE_STATUS_DONE);
						}
					}
				}
			}
		}
	}

	UpdateEnergyshieldActivationCheck();

	if (gameLocal.time > static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetSearchTimer())
	{
		//Global search timer has expired. Return to idle.
		GotoState(AISTATE_IDLE);
	}

	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
		AI_SHIELDHIT = false;
		lastFireablePosition = lastAttackerDamageOrigin;
		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;
		GotoState(AISTATE_COMBAT);
	}

	
	//currently, don't do radio checkin during reinforcement phase.
	//Future note: if we do want radio checkins during reinforcement phase, we'll need to re-initialize idRadioCheckin::InitializeEnemyCount in order to get an accurate enemy count.
	if (radiocheckinPrimed && !static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetReinforcementsActive() && gameLocal.time > radiocheckinTimer)
	{
		radiocheckinPrimed = false;
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->radioCheckinEnt->StartCheckin(GetEyePosition());		
	}

	UpdatePickpocketReaction();
}

void idGunnerMonster::State_Overwatch()
{
	idEntity *enemyEnt = FindEnemyAIVisible();

	//Eyeballs.
	if (enemyEnt)
	{
		//if (!TargetIsInDarkness(enemyEnt))
		{
			//I see enemy.
			lastEnemySeen = enemyEnt;

			suspicionIntervalTimer = gameLocal.time + SUSPICION_THINKINTERVAL;
			suspicionCounter = 0;
			suspicionStartTime = gameLocal.time;
			GotoState(AISTATE_SUSPICIOUS);
			return;
		}
	}

	if (searchMode == SEARCHMODE_SEEKINGPOSITION)
	{
		if (gameLocal.time > intervalTimer)
		{
			intervalTimer = gameLocal.time + OBSERVEINTERVAL;
		
			if (CheckSearchLook(lastFireablePosition, 0, false))
			{
				//I am in overwatch, I just arrived at a place where I can see the LKP. Stop moving, just look at the LKP.
				searchMode = SEARCHMODE_OBSERVING;
				StopMove(MOVE_STATUS_DONE);
				TurnToward(lastFireablePosition);
				Event_SetLaserLock(lastFireablePosition);

				stateTimer = gameLocal.time + OVERWATCHOBSERVE_MINIMUMTIME;

				//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);
			}
		}
	}
	else if (searchMode == SEARCHMODE_OBSERVING)
	{
		//Just keep looking at the position.
		if (gameLocal.time > intervalTimer)
		{
			intervalTimer = gameLocal.time + OBSERVEINTERVAL;

			if (CheckSearchLook(lastFireablePosition, 0, false))
			{
				LookAtPointMS(lastFireablePosition, OBSERVEINTERVAL + 50);
			}
		}

		if (ShouldExitOverwatchState())		
		{
			//the interestpoint is no longer valid. Exit overwatch.
			GotoState(AISTATE_SEARCHING);
		}
	}

	if (gameLocal.time > static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetSearchTimer())
	{
		//Global search timer has expired. Return to idle.
		Event_SetLaserLock(vec3_zero);
		GotoState(AISTATE_IDLE);
	}

	if (AI_DAMAGE || AI_SHIELDHIT)
	{
		AI_DAMAGE = false;
		AI_SHIELDHIT = false;
		lastFireablePosition = lastAttackerDamageOrigin;
		meleeAttackTimer = gameLocal.time + MELEE_COOLDOWNTIME;
		GotoState(AISTATE_COMBAT);
	}	

	UpdatePickpocketReaction();
}

//If in overwatch observation, what conditions kick me out back into search state.
bool idGunnerMonster::ShouldExitOverwatchState()
{
	if (!lastInterest.IsValid() && gameLocal.time > stateTimer)
		return true;

	if (lastInterest.IsValid() && gameLocal.time > stateTimer)
	{
		if (lastInterest.GetEntity()->IsType(idInterestPoint::Type))
		{
			if (!static_cast<idInterestPoint *>(lastInterest.GetEntity())->claimant.IsValid())
				return true;

			if (static_cast<idInterestPoint *>(lastInterest.GetEntity())->claimant.IsValid())
			{
				if (static_cast<idInterestPoint *>(lastInterest.GetEntity())->claimant.GetEntity()->health <= 0)
				{
					//If the person investigating the interestpoint is dead, then kick me out of overwatch.
					return true;
				}
			}
		}
	}

	return false;
}

//Target is DEAD or DEFIBBING. I will walk to the target and stand there...
void idGunnerMonster::State_Victory()
{
	if (searchMode == SEARCHMODE_SEEKINGPOSITION)
	{
		if (gameLocal.time > stateTimer)
		{
			//Check if I am near the enemy target.		
			if (lastEnemySeen.IsValid())
			{
				float distToEnemy;
				idVec3 lastEnemyPos;

				lastEnemyPos = lastEnemySeen.GetEntity()->GetPhysics()->GetOrigin();
				distToEnemy = (lastEnemyPos - GetPhysics()->GetOrigin()).LengthFast();

				if (distToEnemy <= 128)
				{
					//I have arrived at the target.
					searchMode = SEARCHMODE_OBSERVING;
					StopMove(MOVE_STATUS_DONE);
				}
			}

			stateTimer = gameLocal.time + 300;
		}
	}
	else
	{
		//standing next to target.
		if (lastEnemySeen.IsValid())
		{
			//Look at target.
			LookAtPointMS(lastEnemySeen.GetEntity()->GetPhysics()->GetOrigin(), 100);
		}
	}

	if (!gameLocal.GetLocalPlayer()->GetDefibState())
	{
		//player is not defibbing...
		GotoState(AISTATE_SEARCHING);
	}
}

void idGunnerMonster::State_Stunned()
{
	if (gameLocal.GetLocalPlayer()->IsJockeying())
	{
		if (gameLocal.GetLocalPlayer()->meleeTarget.IsValid())
		{
			if (gameLocal.GetLocalPlayer()->meleeTarget.GetEntityNum() == this->entityNumber)
			{
				GotoState(AISTATE_JOCKEYED);
				return;
			}
		}
	}


	if (gameLocal.time > stateTimer)
	{
		//The stun state has ended.

		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_conscious"), displayName.c_str()), GetPhysics()->GetOrigin());

		
		Eventlog_StartCombatAlert();		
		SetCombatState(1, true); //Restart the combat timer.



		WanderAround();		
		GotoState(AISTATE_SEARCHING);
	}
}

void idGunnerMonster::State_Jockeyed()
{
	#define BOUNDRADIUS 8
	#define BOUND_STARTHEIGHT 64
	//I am currently being jockey-ridden by the player.	

	allowPain = false;

	idVec3 playerDesiredDir = gameLocal.GetLocalPlayer()->GetCameraRelativeDesiredMovement(); //vector direction of player's WASD keypresses
	idVec3 myDesiredDir = viewAxis.ToAngles().ToForward() * -1; //Direction baddie's body is facing.


	//Aim the laser
	//idVec3 laserDir = viewAxis.ToAngles().ToForward();
	//idVec3 desiredLaserTarget = GetPhysics()->GetOrigin() + idVec3(0, 0, 48) + laserDir * 256;
	//Event_SetLaserLock(desiredLaserTarget);

	
	if (gameLocal.GetLocalPlayer()->usercmd.forwardmove > 0)
	{
		jockeyBehavior = JB_FORWARD; //I walk forward.
	}
	else if (gameLocal.GetLocalPlayer()->usercmd.rightmove < 0 && gameLocal.GetLocalPlayer()->usercmd.forwardmove == 0)
	{
		jockeyBehavior = JB_STRAFELEFT;
	}
	else if (gameLocal.GetLocalPlayer()->usercmd.rightmove > 0 && gameLocal.GetLocalPlayer()->usercmd.forwardmove == 0)
	{
		jockeyBehavior = JB_STRAFERIGHT;
	}
	else //if ((playerDesiredDir == vec3_zero || gameLocal.GetLocalPlayer()->usercmd.forwardmove < 0))	
	{
		jockeyBehavior = JB_BACKWARD; //I walk backward.
	}

	if (lastJockeyBehavior != jockeyBehavior)
	{
		lastJockeyBehavior = jockeyBehavior;
		jockeyMoveTimer = 0; //allow interrupt of move direction changes. This stops the current animation and begins new one in new direction.
	}

	//float facingResult = DotProduct(playerDesiredDir, myDesiredDir);
	//if ((facingResult > 0 || playerDesiredDir == vec3_zero) && gameLocal.time >= stateTimer + 1500) //give some buffer time at the beginning, so the player has time to react.
	//{
	//	//Postive value. pushing AWAY from enemy's desired direction, we are pulling in opposite directions.
	//	jockeyBehavior = JB_BACKWARD; //I walk backward.
	//}
	//else
	//{
	//	//player is pushing TOWARD my desired direction, we both going in same direction.
	//	jockeyBehavior = JB_FORWARD; //I walk forward.
	//}




	if (gameLocal.time >= jockeyTurnTimer && jockattackState == JOCKATK_NONE)
	{
		//jockeyTurnTimer = gameLocal.time + gameLocal.random.RandomInt(500, 1000);		
        jockeyTurnTimer = gameLocal.time + 100;

		//if (gameLocal.random.RandomInt(100) > 33)
		{
			idVec3 playerDesiredLookPos;

			if (jockeyBehavior == JB_FORWARD)
				playerDesiredLookPos = GetPhysics()->GetOrigin() + playerDesiredDir * 64;
			else if (jockeyBehavior == JB_BACKWARD)
				playerDesiredLookPos = GetPhysics()->GetOrigin() + playerDesiredDir * -64;
			else
			{
				//strafing.
				idAngles forwardDir = playerDesiredDir.ToAngles();

				if (jockeyBehavior == JB_STRAFELEFT)
					forwardDir.yaw -= 90;
				else
					forwardDir.yaw += 90;

				playerDesiredLookPos = GetPhysics()->GetOrigin() + forwardDir.ToForward() * 64;
			}

			

			//TurnToward(playerDesiredLookPos + idVec3(gameLocal.random.RandomInt(-16, 16), gameLocal.random.RandomInt(-16, 16), 0));
			TurnToward(playerDesiredLookPos);


			//Do a minor turn variation.
			//if (jockeyBehavior == JB_FORWARD)
			//{
			//	idVec3 playerDesiredLookPos = GetPhysics()->GetOrigin() + playerDesiredDir * 64;
			//	TurnToward(playerDesiredLookPos + idVec3(gameLocal.random.RandomInt(-16, 16), gameLocal.random.RandomInt(-16, 16), 0));
			//}
			//else
			//{
			//	TurnToward(GetPhysics()->GetOrigin() + (viewAxis.ToAngles().ToForward() * 64) + idVec3(gameLocal.random.RandomInt(-32, 32), gameLocal.random.RandomInt(-32, 32), 0));
			//}
		}
		//else
		//{
		//	//Try to find nearest wall / surface to smash player.
		//	idVec3 directions[8] =
		//	{
		//		//Cardinal directions
		//		idVec3(1,0,0),
		//		idVec3(-1,0,0),
		//		idVec3(0,1,0),
		//		idVec3(0,-1,0),
		//
		//		//Diagonals.
		//		idVec3(1,1,0),
		//		idVec3(-1,1,0),
		//		idVec3(1,-1,0),
		//		idVec3(-1,-1,0),
		//	};
		//	
		//	idVec3 closestWallPosition = vec3_zero;
		//	int closestDistance = 99999;
		//	for (int i = 0; i < 8; i++)
		//	{
		//		trace_t wallTr;
		//		idVec3 traceStart = GetPhysics()->GetOrigin() + idVec3(0, 0, BOUND_STARTHEIGHT);
		//		gameLocal.clip.TraceBounds(wallTr, traceStart, traceStart + directions[i] * 256, idBounds(idVec3(-BOUNDRADIUS, -BOUNDRADIUS, -BOUNDRADIUS), idVec3(BOUNDRADIUS, BOUNDRADIUS, BOUNDRADIUS)), MASK_SOLID, this);
		//		if (wallTr.fraction < 1)
		//		{
		//			float distance = (traceStart - wallTr.endpos).LengthFast();
		//			if (distance < closestDistance)
		//			{
		//				closestDistance = distance;
		//				closestWallPosition = wallTr.endpos;
		//			}
		//		}
		//	}
		//
		//	if (closestWallPosition != vec3_zero)
		//	{
		//		idVec3 wallDirection = (GetPhysics()->GetOrigin() + idVec3(0, 0, 64)) - closestWallPosition;
		//		wallDirection.NormalizeFast();
		//		TurnToward(GetPhysics()->GetOrigin() + wallDirection * 64);
		//	}
		//}
	}

	//if (gameLocal.time >= jockeyDamageTimer)
	//{
	//	if (jockeyBehavior == JB_FORWARD)
	//	{
	//		//inflict damage.
	//		Damage(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), vec3_zero, "damage_jockey", 1.0f, 0);
	//		jockeyDamageTimer = gameLocal.time + JOCKEY_DAMAGE_INTERVAL;
	//
	//		gameLocal.GetLocalPlayer()->StartSound("snd_melee", SND_CHANNEL_ANY);
	//
	//		gameLocal.DoParticle("blood_jockey.prt", this->GetEyePosition());
	//	}
	//}


	//Jockee shoots their weapon.
	if (jockattackState == JOCKATK_SLAMATTACK)
	{
		if (gameLocal.time >= jockStateTimer)
		{
			jockattackState = JOCKATK_NONE;
		}
	}
	else if (jockattackState == JOCKATK_KILLENTITYATTACK)
	{
		if (gameLocal.time >= jockStateTimer)
		{
			//StartJockeySlam();

			jockeyMoveTimer = gameLocal.time;
			jockeyDamageTimer = gameLocal.time; //after slam attack, do a cooldown on player inflicting damage on me.	
			jockattackState = JOCKATK_NONE;
		}
	}

	


	if (gameLocal.time >= jockeyMoveTimer && jockattackState == JOCKATK_NONE)
	{
		if (jockeyBehavior == JB_FORWARD)
		{
			//trace_t wallTr;
			//idVec3 traceStart = GetPhysics()->GetOrigin() + idVec3(0, 0, BOUND_STARTHEIGHT);
			//gameLocal.clip.TraceBounds(wallTr, traceStart, traceStart + viewAxis.ToAngles().ToForward() * 48, idBounds(idVec3(-BOUNDRADIUS, -BOUNDRADIUS, -BOUNDRADIUS), idVec3(BOUNDRADIUS, BOUNDRADIUS, BOUNDRADIUS)), MASK_SOLID, this);
			//if (wallTr.fraction < 1)
			//{
			//	//There IS a wall nearby.
			//	idVec3 directionTowardWall = wallTr.endpos - traceStart;
			//	directionTowardWall.NormalizeFast();
            //
			//	bool shouldDoSlam = true;
			//	if (playerDesiredDir != vec3_zero)
			//	{
			//		float facingResult = DotProduct(playerDesiredDir, directionTowardWall);
			//		if (facingResult > 0)
			//		{
			//			//Player is pushing TOWARD a nearby wall, or is not pressing any keys at all. Do the slam.
			//		}
			//		else
			//		{
			//			//Player is pushing away from wall. Don't do slam.
			//			shouldDoSlam = false;
			//		}
			//	}
            //
			//	if (shouldDoSlam)
			//	{
			//		TurnToward(wallTr.endpos + directionTowardWall * -128);
			//		jockeyTurnTimer = gameLocal.time + 2000;
			//		StartJockeySlam();
			//		return;
			//	}
			//}


			//pushing AWAY from enemy's desired direction.
			if (TestAnimMove("jockey_fwd_96"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_fwd_96", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_fwd_96")) - 100;
			}
			else if (TestAnimMove("jockey_fwd_32"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_fwd_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_fwd_32")) - 100;
			}
			else
			{
				//common->Warning("unstuck");
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_fwd_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_fwd_32")) - 100;
			}
		}
		else if (jockeyBehavior == JB_STRAFELEFT)
		{
			if (TestAnimMove("jockey_left_96"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_left_96", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_left_96")) - 100;
			}
			else if (TestAnimMove("jockey_left_32"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_left_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_left_32")) - 100;
			}
			else
			{
				//common->Warning("unstuck");
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_left_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_left_32")) - 100;
			}
		}
		else if (jockeyBehavior == JB_STRAFERIGHT)
		{
			if (TestAnimMove("jockey_right_96"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_right_96", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_right_96")) - 100;
			}
			else if (TestAnimMove("jockey_right_32"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_right_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_right_32")) - 100;
			}
			else
			{
				//common->Warning("unstuck");
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_right_32", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_right_32")) - 100;
			}
		}
		else
		{
			//First, we check if we're near a wall, to see if slam attack is eligible.
			//trace_t wallTr;
			//idVec3 traceStart = GetPhysics()->GetOrigin() + idVec3(0, 0, BOUND_STARTHEIGHT);
			//gameLocal.clip.TraceBounds(wallTr, traceStart, traceStart + viewAxis.ToAngles().ToForward() * -48, idBounds(idVec3(-BOUNDRADIUS, -BOUNDRADIUS, -BOUNDRADIUS), idVec3(BOUNDRADIUS, BOUNDRADIUS, BOUNDRADIUS)), MASK_SOLID, this);			
			//if (wallTr.fraction < 1) //If there is a wall
			//{
			//	//Do slam attack.
			//	StartJockeySlam();
			//	return;
			//}

			//pushing TOWARD enemy's desired direction.
			if (TestAnimMove("jockey_back_128"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_back_128", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_back_128")) - 100;
			}
			else if (TestAnimMove("jockey_back_64"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_back_64", 8);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_back_64")) - 100;
			}
			else if (TestAnimMove("jockey_back_16"))
			{
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_back_16", 1);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_back_16"));
			}
			else
			{
				//common->Warning("unstuck");
				SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_back_16", 1);
				jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_back_16"));
			}
		}
	}

    if (gameLocal.time > jockeyWorldfrobTimer && jockattackState == JOCKATK_NONE)
    {
        jockeyWorldfrobTimer = gameLocal.time + JOCKEY_WORLDFROB_TIMEINTERVAL;

        //see if there's stuff around us that we can damage or frob.
        DoJockeyWorldfrob();
    }

	//Update whether there's something I can be smashed into.
	if (jockattackState == JOCKATK_NONE)
	{
		UpdateJockeyAttackAvailability();
	}
}

//If player presses LMB, determine what will happen.
void idGunnerMonster::UpdateJockeyAttackAvailability()
{
	if (gameLocal.time < jocksurfacecheckTimer || jockattackState != JOCKATK_NONE)
		return;

	jocksurfacecheckTimer = gameLocal.time + JOCKEY_WORLD_ANALYZE_INTERVAL;
	
	if (jockeyKillEntity.IsValid())
	{
		jockeyKillEntity.GetEntity()->SetPostFlag(POST_OUTLINE_FROB, false);
	}

	//First: see if there's a nearby context-kill entity.
	jockeyKillEntity = FindJockeyKillEntity();

	if (jockeyKillEntity.IsValid())
	{
		//Found a kill entity.
		jockeyAttackCurrentlyAvailable = JOCKATKTYPE_KILLENTITY;
		jockeyKillEntity.GetEntity()->SetPostFlag(POST_OUTLINE_FROB, true);
		return;
	}
	//BC disabling the wall slam...
	//else
	//{
	//	//No kill entity found.
	//	//See if there's a wall I can be smashed into.
	//
	//	jockeySmashTr = FindJockeyClosestSurface();
	//	if (jockeySmashTr.endpos != vec3_zero)
	//	{
	//		jockeyAttackCurrentlyAvailable = JOCKATKTYPE_WALLSMASH;
	//		return;
	//	}
	//}

	//No attacks available.
	jockeyAttackCurrentlyAvailable = JOCKATKTYPE_NONE;
}

//This returns the CURRENT kill entity. The closest entity the player can kill with.
idEntity * idGunnerMonster::FindJockeyKillEntity()
{
	int i;
	int entityCount;
	idEntity *entityList[MAX_GENTITIES];
	idVec3 myPosition = GetPhysics()->GetOrigin() + idVec3(0, 0, 64);

	entityCount = gameLocal.EntitiesWithinRadius(myPosition, JOCKEY_KILLENTITY_RADIUS, entityList, MAX_GENTITIES);

	float shortestDistance = 30000;
	idEntity *closestEnt = NULL;
	for (i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];

		if (!IsObjectKillEntity(ent) || !gameLocal.HasJockeyLOSToEnt(ent))
			continue;

		float distance = (ent->GetPhysics()->GetOrigin() - myPosition).LengthSqr();
		if (distance < shortestDistance)
		{
			shortestDistance = distance;
			closestEnt = ent;
		}
	}

	return closestEnt;
}

bool idGunnerMonster::IsObjectKillEntity(idEntity *ent)
{
	if (!ent)
		return false;

	if (!ent->fl.takedamage || ent->health <= 0 || ent->IsHidden() || ent == gameLocal.GetLocalPlayer() || ent == this 
		|| ent->GetBindMaster() != NULL || !ent->spawnArgs.GetBool("jockey_killent", "1") || ent->IsType(idMoveableItem::Type))
		return false;

	//get vertical distance delta.
	float distanceDelta = ent->GetPhysics()->GetAbsBounds().GetCenter().z - this->GetPhysics()->GetOrigin().z;
	if (distanceDelta > JOCKEY_VERT_DISTANCE_MAX)
		return false;

	if (distanceDelta < JOCKEY_VERT_DISTANCE_MIN)
		return false;

	return true;
}


idVec3 idGunnerMonster::FindJockeyShootTarget()
{
	idVec3 defaultTarget = GetEyePosition() + viewAxis.ToAngles().ToForward() * 128; //Just shoot straight.

	idEntity *bestEnt = NULL;
	float smallestDotProduct = 9999;
	idEntity *ent = NULL;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->fl.takedamage || ent->health <= 0 || !gameLocal.InPlayerPVS(ent) || ent == gameLocal.GetLocalPlayer() || ent == this)
			continue;
				
		//Found a possible candidate.
		idVec3 myDir = viewAxis.ToAngles().ToForward();
		idVec3 dirToEntity = GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin();
		dirToEntity.Normalize();
		float facingResult = DotProduct(dirToEntity, myDir);

		if (facingResult > -.8f)
			continue;

		if (facingResult < smallestDotProduct || ent->IsType(idAI::Type)) //Bias it toward other AI
		{
			smallestDotProduct = facingResult;
			bestEnt = ent;
		}
	}

	if (bestEnt != NULL)
	{
		return bestEnt->GetPhysics()->GetOrigin();
	}

	return defaultTarget;
}

void idGunnerMonster::DoJockeyWorldfrob()
{
    //while being jockeyed, frob/damage stuff around us. This is to simulate the jockee crashing into things and bumping into buttons, doors, etc.	   

    int i;
    int entityCount;
    idEntity *entityList[MAX_GENTITIES];

    entityCount = gameLocal.EntitiesWithinRadius(GetPhysics()->GetOrigin() + idVec3(0,0,64), JOCKEY_WORLDFROB_RADIUS, entityList, MAX_GENTITIES);

    idList<int> interactables;

    for (i = 0; i < entityCount; i++)
    {
        idEntity *ent = entityList[i];
        trace_t splashTr;

        if (!ent)
            continue;

        if (!ent->isFrobbable && !ent->fl.takedamage) //if it can't be interacted with, then skip.
            continue;

        if (ent->health <= 0 || ent->IsHidden() || ent == gameLocal.GetLocalPlayer() || ent == this || ent->GetBindMaster() != NULL)
            continue;

        interactables.AddUnique(ent->entityNumber);
    }

    if (interactables.Num() <= 0) //If there's nothing to interact with, then exit here.
        return;

    //We now have a list of things we can interact with. To make things less crazy, we only choose one object.
    int randomIndex = interactables[ gameLocal.random.RandomInt(interactables.Num())];

	idEntity *frobEnt = gameLocal.entities[randomIndex];

	//If it's frobbable, frob it.
    if (frobEnt->isFrobbable && frobEnt->spawnArgs.GetBool("jockey_frob", "1"))
    {
		idEntityFx::StartFx("fx/frob_lines", &frobEnt->GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		frobEnt->DoFrob(JOCKEYFROB_INDEX, this);


		if (!frobEnt->IsType(idItem::Type)) //BC this is a hack kludge, we just don't print event messages for jockeystrugglefrobbing items in the world.
		{
			idStr frobEntName;
			if (frobEnt->displayName.Length() > 0)
				frobEntName = frobEnt->displayName;
			else
				frobEntName = frobEnt->GetName();

			gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_struggleuse"), displayName.c_str(), frobEntName.c_str()), frobEnt->GetPhysics()->GetOrigin());
		}
    }

	//If it's a window, damage it.
	//if (frobEnt->IsType(idBrittleFracture::Type))
	//{
	//	trace_t tr;
	//	idVec3 pushOrigin = GetPhysics()->GetOrigin() + idVec3(0, 0, 64);
	//	idVec3 pushTarget = frobEnt->GetPhysics()->GetAbsBounds().GetCenter();
	//	pushTarget += idVec3(gameLocal.random.RandomInt(-32, 32), gameLocal.random.RandomInt(-32, 32), gameLocal.random.RandomInt(-32, 0));
	//	gameLocal.clip.TracePoint(tr, pushOrigin, pushTarget, MASK_SOLID, NULL);
	//
	//	idVec3 pushDir = (frobEnt->GetPhysics()->GetAbsBounds().GetCenter() - pushOrigin);
	//	pushDir.Normalize();
	//
	//	frobEnt->AddDamageEffect(tr, pushDir * 128, "damage_generic");
	//	frobEnt->Damage(this, this, pushDir, "damage_generic", 1.0f, 0);
	//	frobEnt->AddForce(this, this->entityNumber, tr.endpos, pushDir * 16384.0f);
	//
	//	idAngles particleAng = tr.endAxis.ToAngles();
	//	particleAng.pitch -= 90;
	//	gameLocal.DoParticle("glass_shatter_big.prt", tr.endpos, particleAng.ToForward());
	//}

	//if (frobEnt->IsType(idHazardPipe::Type))
	//{
	//	//attempt to damage the hazardpipe.
	//	DoJockeyHazardpipeDamage(frobEnt);
	//}
	
	//If it can be knocked around, shove it.
	if (frobEnt->IsType(idMoveableItem::Type))
	{
		int shoveAmount = gameLocal.random.RandomInt(64, 128);
		idVec3 shoveDir = (frobEnt->GetPhysics()->GetOrigin() - idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, frobEnt->GetPhysics()->GetOrigin().z - 24)); //we want it to kick upward a little.
		shoveDir.Normalize();
		idVec3 impulse = shoveDir * shoveAmount * frobEnt->GetPhysics()->GetMass();

		frobEnt->ApplyImpulse(frobEnt, 0, frobEnt->GetPhysics()->GetAbsBounds().GetCenter(), impulse);

		gameLocal.DoParticle("smoke_ring13.prt", frobEnt->GetPhysics()->GetOrigin());
	}
}

void idGunnerMonster::DoWorldDamage()
{
	//This is for things like the banana fall. We damage things around us.
	DoJockeyWorldDamage();
}

void idGunnerMonster::DoJockeyWorldDamage()
{
	//When slammed on wall, do damage to nearby things.

	int i;
	int entityCount;
	idEntity *entityList[MAX_GENTITIES];
	entityCount = gameLocal.EntitiesWithinRadius(GetPhysics()->GetOrigin() + idVec3(0, 0, 64), JOCKEY_WORLDFROB_RADIUS, entityList, MAX_GENTITIES);
	for (i = 0; i < entityCount; i++)
	{
		idEntity *ent = entityList[i];
		trace_t splashTr;

		if (!ent)
			continue;

		if (!ent->fl.takedamage) //if it can't be interacted with, then skip.
			continue;

		if (ent->health <= 0 || ent->IsHidden() || ent == gameLocal.GetLocalPlayer() || ent == this || ent->GetBindMaster() != NULL)
			continue;

		//If it's a window, damage it.
		if (ent->IsType(idBrittleFracture::Type))
		{
			#define PUSH_RANDVARIANCE 24
			trace_t tr;
			idVec3 pushOrigin = GetPhysics()->GetOrigin() + idVec3(0, 0, 64);
			idVec3 pushTarget = ent->GetPhysics()->GetAbsBounds().GetCenter();
			pushTarget += idVec3(gameLocal.random.RandomInt(-PUSH_RANDVARIANCE, PUSH_RANDVARIANCE), gameLocal.random.RandomInt(-PUSH_RANDVARIANCE, PUSH_RANDVARIANCE), gameLocal.random.RandomInt(-PUSH_RANDVARIANCE, 0));
			gameLocal.clip.TracePoint(tr, pushOrigin, pushTarget, MASK_SOLID, NULL);
		
			idVec3 pushDir = (ent->GetPhysics()->GetAbsBounds().GetCenter() - pushOrigin);
			pushDir.Normalize();
		
			ent->AddDamageEffect(tr, pushDir * 128, "damage_generic");
			ent->Damage(this, this, pushDir, "damage_generic", 1.0f, 0);
			ent->AddForce(this, this->entityNumber, tr.endpos, pushDir * 16384.0f);
		
			idAngles particleAng = tr.endAxis.ToAngles();
			particleAng.pitch -= 90;
			gameLocal.DoParticle("glass_shatter_big.prt", tr.endpos, particleAng.ToForward());
		}

		if (ent->IsType(idHazardPipe::Type))
		{
			//attempt to damage the hazardpipe.
			DoJockeyHazardpipeDamage(ent);
		}
	}

}

void idGunnerMonster::DoJockeyHazardpipeDamage(idEntity *frobEnt)
{
	//do a traceline to find LOS to the hazardpipe.
	trace_t tr;
	idVec3 trStart = GetPhysics()->GetOrigin() + idVec3(0, 0, 48 + gameLocal.random.RandomInt(16));
	idVec3 trEnd = idVec3(frobEnt->GetPhysics()->GetOrigin().x, frobEnt->GetPhysics()->GetOrigin().y, trStart.z);
	gameLocal.clip.TracePoint(tr, trStart, trEnd, MASK_SOLID, NULL);

	if (tr.fraction >= 1)
		return; //trace didn't hit anything. exit.

	if (tr.c.entityNum != frobEnt->entityNumber)
		return; //trace didn't touch the hazardpipe. exit.

	//trace did hit. Do the hazardpipe damage.
	frobEnt->Damage(this, this, tr.c.normal, "damage_generic", 1.0f, CLIPMODEL_ID_TO_JOINT_HANDLE(tr.c.id));
	frobEnt->AddDamageEffect(tr, vec3_zero, "damage_generic");
}



//This is the enemy inflicting damage on the player.
void idGunnerMonster::StartJockeySlam()
{
	SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_slam", 2);
	jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_slam"));
	jockeyDamageTimer = jockeyMoveTimer + JOCKEY_SLAM_COOLDOWN; //after slam attack, do a cooldown on player inflicting damage on me.	
}

//When player presses LMB while jockeying.
void idGunnerMonster::DoJockeyBrutalSlam()
{
	if (jockattackState != JOCKATK_NONE) //if I am not in an attacked state, then I'm available to be attacked
	{
		return;
	}

	if (jockeyAttackCurrentlyAvailable == JOCKATKTYPE_KILLENTITY && jockeyKillEntity.IsValid())
	{
		//Turn toward the kill entity.


		TurnToward(jockeyKillEntity.GetEntity()->GetPhysics()->GetOrigin()); //force the angle to be the node's yaw.


		gameLocal.GetLocalPlayer()->playerView.DoDurabilityFlash();

		//Player is doing kill-entity attack on me.
		SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_exec", 2);
		jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_slam"));
		jockattackState = JOCKATK_KILLENTITYATTACK;
		jockStateTimer = jockeyMoveTimer + 300;

		gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), "snd_vo_jockeyattack", VO_CATEGORY_BARK);
	}
	//else if (jockeyAttackCurrentlyAvailable == JOCKATKTYPE_WALLSMASH)
	//{
	//	//Player is wall slamming me. I get slammed against wall.
	//	SetAnimState(ANIMCHANNEL_LEGS, "Legs_jockey_slam", 2);
	//	jockeyMoveTimer = gameLocal.time + GetAnimator()->AnimLength(GetAnim(ANIMCHANNEL_LEGS, "jockey_slam")) - 200;
	//	jockattackState = JOCKATK_SLAMATTACK;
	//	jockStateTimer = jockeyMoveTimer;
	//}
}

//Is anyone jockey-attached to me right now
bool idGunnerMonster::IsJockeyBeingAttacked()
{
	return (jockattackState != JOCKATK_NONE);
}

//Am I actively being slammed into a wall right now
bool idGunnerMonster::IsJockeyBeingSlammed()
{
	return (jockattackState == JOCKATK_SLAMATTACK || jockattackState == JOCKATK_KILLENTITYATTACK);
}


trace_t idGunnerMonster::FindJockeyClosestSurface()
{
	//Try to find nearest wall / surface to smash player. Returns 0,0,0 if nothing suitable found.
	#define DIR_COUNT 8
	idVec3 directions[DIR_COUNT] =
	{
		//Cardinal directions
		idVec3(1,0,0),
		idVec3(-1,0,0),
		idVec3(0,1,0),
		idVec3(0,-1,0),
	
		//Diagonals.
		idVec3(1,1,0),
		idVec3(-1,1,0),
		idVec3(1,-1,0),
		idVec3(-1,-1,0),
	};
	
	trace_t closestTr;
	closestTr.endpos = vec3_zero;
	int closestDistance = 99999;
	for (int i = 0; i < DIR_COUNT; i++)
	{
		trace_t wallTr;
		idVec3 traceStart = GetPhysics()->GetOrigin() + idVec3(0, 0, BOUND_STARTHEIGHT);
		gameLocal.clip.TraceBounds(wallTr, traceStart, traceStart + directions[i] * JOCKEY_WALLSLAM_RADIUS, idBounds(idVec3(-BOUNDRADIUS, -BOUNDRADIUS, -BOUNDRADIUS), idVec3(BOUNDRADIUS, BOUNDRADIUS, BOUNDRADIUS)), MASK_SOLID, this);
		if (wallTr.fraction < 1)
		{
			float distance = (traceStart - wallTr.endpos).LengthFast();
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestTr = wallTr;
			}
		}
	}

	return closestTr;
}


//Starts the stun state. 
void idGunnerMonster::StartStunState(const char *damageDefName)
{
	// SW 20th Feb 2025: You can't be stunned if you're dead!
	// (stops bleedout pirates from sneezing, as funny as it is to witness)
	if (AI_DEAD)
		return;

	StopSound(SND_CHANNEL_VOICE); //halt any sound they may have been making.

	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
	if (damageDef)
	{
		// SW: added support for alternate stun animations/times during the tutorial
		// This lets us make stun timing a little more forgiving when teaching the player.
		// If doTutorialStun is set and no alternate anim/time is found, we fall back to the default
		bool doTutorialStun = this->spawnArgs.GetBool("doTutorialStun", "0");
		const char* newAnim;
		
		if (doTutorialStun && damageDef->FindKeyIndex("stun_anim_tut") != -1)
		{
			newAnim = damageDef->GetString("stun_anim_tut", "pain_stun");
		}
		else
		{
			newAnim = damageDef->GetString("stun_anim", "pain_stun");
		}

		if ((aiState == AISTATE_STUNNED && idStr::Icmp(newAnim, stunAnimationName) == 0) || aiState == AISTATE_JOCKEYED)
			return;		 //Ignore stun state if I am already in stun state, using the same animation.

		//note: stun_time is defined per-item, in the .def files. Search the .def files for: stun_time
		if (doTutorialStun && damageDef->FindKeyIndex("stun_time_tut") != -1)
		{
			stunTime = (int)(damageDef->GetFloat("stun_time_tut", "5") * 1000.0f);
		}
		else
		{
			stunTime = (int)(damageDef->GetFloat("stun_time", "5") * 1000.0f);
		}
			

		stunStartTime = gameLocal.time;
		stunAnimationName = newAnim;
		GotoState(AISTATE_STUNNED);


		//Make status text fly out of head.
		//jointHandle_t headJoint;
		//headJoint = animator.GetJointHandle("head");
		//if (headJoint != INVALID_JOINT)
		//{
		//	idVec3 jointPos;
		//	idMat3 jointAxis;
		//	this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);
		//	gameLocal.GetLocalPlayer()->SetFlytextEvent(jointPos, "STUNNED", idDeviceContext::ALIGN_CENTER);
		//}
	}
}

void idGunnerMonster::OnArriveAtLKP()
{
	//I've arrived at LKP.
	//Tell any overwatch'd friends to enter search mode.

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity || !entity->IsActive() || entity->IsHidden() || entity->health <= 0 || entity->entityNumber == this->entityNumber)		//sanity check
			continue;

		if (!entity->IsType(idAI::Type))
			continue;

        //If not on my team, then ignore.
		if (entity->team != this->team || static_cast<idAI*>(entity)->aiState != AISTATE_OVERWATCH)
			continue;

		//ok, we have a bad guy now. They're in overwatch... switch them over to search mode.
		static_cast<idAI*>(entity)->GotoState(AISTATE_SEARCHING);
	}
}

//Check whether this person is a candidate or not for the interest point.
bool idGunnerMonster::InterestPointCheck(idEntity *ent)
{
	idInterestPoint *interestpoint;	
	int priority;

	if (!CanAcceptStimulus() || ignoreInterestPoints)
		return false;

	if (!ent->IsType(idInterestPoint::Type))
	{
		gameLocal.Error("InterestPointCheck() received an entity that was not an interest point.\n");
		return false;
	}

	//ok. we have an interestpoint.
	interestpoint = static_cast<idInterestPoint *>(ent);
	priority = interestpoint->priority;
	

	//Priority filter.
	// SW 19th March 2025: Adding AISTATE_SUSPICIOUS to this collection to avoid state thrash.
	// The problematic scenario was where an AI was confronted with both a visible player and also a persistent interestpoint (e.g. a sound or smell) at the same time.
	if ((aiState == AISTATE_COMBAT || aiState == AISTATE_COMBATOBSERVE || aiState == AISTATE_COMBATSTALK || aiState == AISTATE_SUSPICIOUS) && priority < INTEREST_COMBATPRIORITY)
	{
		//We're in combat and it's a lesser priority. Exit.
		return false;
	}
	else
	{
		//if we encounter something of lower priority, ignore it.
		//TODO: This is buggy -- there are situations where the AI gets trapped in a state where the previous lastInterestPriority prevents the AI from acquiring new lower-priority interestpoints. Need a cooldown of some kind?
		if (priority < lastInterestPriority)
		{
			//debug.
			if (lastInterest.IsValid())
			{
				//idEntity *ent = lastInterest.GetEntity();
				//common->Printf("");
			}

			return false;
		}
	}

	// When did our guard last switch their interest? If it's too recent, let them keep their current interest
	if (lastInterest.IsValid() && gameLocal.time < lastInterestSwitchTime + INTEREST_SWITCH_GRACE_TIME)
	{
		return false;
	}

	const idVec3 & interestPos = interestpoint->GetPhysics()->GetOrigin();

	//Check if we're within radius of it.
	if (interestpoint->noticeRadius > 0)
	{
		int distToInterestpoint = (this->GetPhysics()->GetOrigin() - interestPos).Length();
		if (distToInterestpoint > interestpoint->noticeRadius)
		{ 
			//too far.
			return false;
		}
	}

	if (interestpoint->interesttype == IPTYPE_NOISE)
	{
		//Noise disturbance.
		if (interestpoint->onlyLocalPVS && !gameLocal.InPVS_Pos(GetEyePosition(), interestPos))
		{
			return false;			
		}
		
		// SW 4th March 2025: Swapping idActor::PointVisible for the more reliable idActor::CanSee
		if (CanSee(interestpoint, false) && CheckFOV(interestPos) && (aiState == AISTATE_COMBAT || aiState == AISTATE_COMBATOBSERVE || aiState == AISTATE_COMBATSTALK))
		{
			//Sound is right in front of me and I'm in combat. Ignore it.
			return false;
		}

		//BC 2-14-2025: smells ignore obstructions and always return true.
		if (interestpoint->spawnArgs.GetBool("is_smelly", "0") || !interestpoint->spawnArgs.GetBool("is_obstructed", "1"))
			return true;
		else
			return GetSoundIntensityObstructed(interestPos,interestpoint->noticeRadius) > 0.01f;
	}
	else if (interestpoint->interesttype == IPTYPE_VISUAL)
	{
		//Visual disturbance.
		// SW 4th March 2025: Swapping idActor::PointVisible for the more reliable idActor::CanSee
		if (CanSee(interestpoint, false) && CheckFOV(interestPos))
		{
			//I can see the disturbance.
			return true;
		}

		return false;
	}

	return false;
}


//So, this isn't an ai state. This is just a router that directs the AI toward the appropriate state to transition to.
void idGunnerMonster::InterestPointReact(idEntity *interestpoint, int roletype)
{
	int nextState = 0;
	idVec3 interestWalkableDestination;

	if (!interestpoint->IsType(idInterestPoint::Type))
	{
		common->Error("InterestPointReact() was sent an entity that wasn't an interest point.");
		return; //uhhh this should never happen...
	}
	
	if (lastInterest.IsValid())
	{
		gameLocal.Warning("idGunnerMonster::InterestPointReact: Previous interestpoint was not cleared first.");
	}

	lastInterestPriority = static_cast<idInterestPoint *>(interestpoint)->priority;
	lastInterest = static_cast<idInterestPoint *>(interestpoint);
	lastInterestSwitchTime = gameLocal.time;

	// Get direction from target to eyes
	UpdateInterestBeam( interestpoint );

	interestBeam->Show();

	if (static_cast<idInterestPoint *>(interestpoint)->forceCombat /*&& CanSee(interestpoint, false)*/)
	{
		//This interestpoint forces combat to happen.

		if (idMath::Abs(static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatStartTime() - gameLocal.time) > 2000)
		{
			AddEventlog_Interestpoint(interestpoint);

			//Interestpoint that forces combat state to happen.
			Eventlog_StartCombatAlert();
			SetCombatState(1, true); //Restart the combat timer.
		}
		
		UpdateLKP(interestpoint);
		nextState = AISTATE_COMBATSTALK;

		//PrintInterestpointEvent(interestpoint, "combatstalk");
	}
	else if (roletype == INTERESTROLE_INVESTIGATE)
	{
		//Investigate an interestpoint.
		nextState = AISTATE_SEARCHING;



		//PrintInterestpointEvent(interestpoint, "investigator");
	}
	else if (roletype == INTERESTROLE_OVERWATCH)
	{
		//Provide overwatch.
		nextState = AISTATE_OVERWATCH;
		lastFireablePosition = interestpoint->GetPhysics()->GetOrigin();
		MoveToPosition(lastFireablePosition); //TODO: drop to ground...
		GotoState(nextState);

		//PrintInterestpointEvent(interestpoint, "overwatch");

		return;
	}


	//Say interestpoint reaction VO.
	if (static_cast<idInterestPoint*>(interestpoint)->interesttype == IPTYPE_NOISE)
	{
		//Audio interestpoint.

		if (static_cast<idInterestPoint*>(interestpoint)->priority >= INTEREST_COMBATPRIORITY)
		{
			gameLocal.voManager.SayVO(this, "snd_vo_heardanger", VO_CATEGORY_BARK);	//"I hear a combat sound!"
		}
		else if (interestpoint->spawnArgs.GetBool("is_smelly", "0"))
		{
			if (vo_lastSpeakTime + SMELLY_VO_DEBOUNCETIME < gameLocal.time)
			{
				gameLocal.voManager.SayVO(this, "snd_vo_smell", VO_CATEGORY_BARK);		//"I smell something suspicious..."
			}
		}
		else
		{
			gameLocal.voManager.SayVO(this, "snd_vo_hearsus", VO_CATEGORY_BARK);		//"I hear something suspicious..."
		}
	}
	else
	{
		//Visual interestpoint.
		idStr overrideVO = interestpoint->spawnArgs.GetString("vo_notice");
		if (overrideVO.Length() > 0)
		{
			//Use the special override VO.
			gameLocal.voManager.SayVO(this, overrideVO.c_str(), VO_CATEGORY_BARK);
		}
		else
		{
			gameLocal.voManager.SayVO(this, "snd_vo_seesus", VO_CATEGORY_BARK);
		}
	}




	//this is for detecting player in vent. Start the ventpurge.
	if (static_cast<idInterestPoint *>(interestpoint)->breaksConfinedStealth && gameLocal.GetLocalPlayer()->inConfinedState
		&& gameLocal.GetLocalPlayer()->confinedType == CONF_VENT
		&& interestpoint->GetPhysics()->GetClipModel()->GetOwner() != NULL && interestpoint->GetPhysics()->GetClipModel()->GetOwner() == gameLocal.GetLocalPlayer())
	{
		
		if (static_cast<idInterestPoint *>(interestpoint)->interesttype == IPTYPE_VISUAL)
		{
			gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_seen_vent"), displayName.c_str()), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
		}
		else
		{
			if (interestpoint->spawnArgs.GetBool("is_smelly", "0"))
			{
				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_smelled_vent"), displayName.c_str()), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
			}
			else
			{
				gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_hear_vent"), displayName.c_str()), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
			}
		}

		gameLocal.GetLocalPlayer()->confinedStealthActive = false;
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartVentPurge(interestpoint);
	}


	//Do airlock lockdown if interestpoint is inside the airlock.
	if (static_cast<idInterestPoint *>(interestpoint)->breaksConfinedStealth)
	{
		//interestpoint->GetPhysics()->GetClipModel()->GetOwner() != NULL && interestpoint->GetPhysics()->GetClipModel()->GetOwner() == gameLocal.GetLocalPlayer()

		for (idEntity* airlockEnt = gameLocal.airlockEntities.Next(); airlockEnt != NULL; airlockEnt = airlockEnt->airlockNode.Next())
		{
			if (!airlockEnt)
				continue;

			if (!airlockEnt->IsType(idAirlock::Type))
				continue;

			//See if airlock is already purged.
			if (static_cast<idAirlock *>(airlockEnt)->IsAirlockLockdownActive())
				continue;

			//See if interestpoint is inside the airlock.
			if (airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(interestpoint->GetPhysics()->GetOrigin()))
			{
				//Only do it if player is physically inside the airlock. This doesn't completely make sense!!! But, it makes the game a ton more readable / understandable.
				idVec3 playerPosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0,0,1);
				if (!airlockEnt->GetPhysics()->GetAbsBounds().ContainsPoint(playerPosition))
					continue;

				//Interestpoint is inside airlock. Activate the lockdown.
				
				idStr airlockLog = "";
				if (static_cast<idInterestPoint*>(interestpoint)->interesttype == IPTYPE_VISUAL)
				{
					//Visual interestpoint.			
					airlockLog = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_airlock_see"), displayName.c_str());
				}
				else
				{
					if (interestpoint->spawnArgs.GetBool("is_smelly", "0"))
					{
						//Smell interestpoint.
						airlockLog = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_airlock_smell"), displayName.c_str());
					}
					else
					{
						//Audio interestpoint.
						airlockLog = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_airlock_hear"), displayName.c_str());
					}
				}			
				


				gameLocal.AddEventLog(airlockLog.c_str(), GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);

				static_cast<idAirlock *>(airlockEnt)->DoAirlockLockdown(true, interestpoint);
				continue;
			}
		}
	}

	

	if (ai_showInterestPoints.GetInteger() == 2)
	{
		//debug.
		idStr intName;
		int arrowLength = 24 + gameLocal.random.RandomInt(48);
	
		intName = interestpoint->GetName();
		if (intName.Find("idinterestpoint", false) == 0)
		{
			intName = intName.Right(intName.Length() - 16);
		}
	
		gameRenderWorld->DebugArrow(colorWhite, interestpoint->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength), interestpoint->GetPhysics()->GetOrigin(), 4, 30000);
		gameRenderWorld->DrawText(idStr::Format("%s", intName.c_str()), interestpoint->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength + 8), .1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 30000);
		gameRenderWorld->DrawText(idStr::Format("priority: %d", static_cast<idInterestPoint *>(interestpoint)->priority), interestpoint->GetPhysics()->GetOrigin() + idVec3(0, 0, arrowLength + 4), .1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 30000);
	}

	interestWalkableDestination = FindValidPosition(interestpoint->GetPhysics()->GetOrigin());

	MoveToPosition(interestWalkableDestination);
	lastFireablePosition = interestpoint->GetPhysics()->GetOrigin();
	

	
	AddEventlog_Interestpoint(interestpoint);
	

	GotoState(nextState);
}

void idGunnerMonster::PrintInterestpointEvent(idEntity *interestpoint, idStr rolename)
{
	idStr interestpointname = (interestpoint->displayName.Length() > 0) ? interestpoint->displayName : interestpoint->name;
	gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_investigating"), displayName.c_str(), interestpointname.c_str()), GetPhysics()->GetOrigin());
}

void idGunnerMonster::UpdateInterestBeam( idEntity *interestpoint )
{
	idVec3 interestTarget = interestpoint->GetPhysics()->GetOrigin();
	jointHandle_t headJoint;
	idVec3 jointPos;
	idMat3 jointAxis;

	interestBeamTarget->SetOrigin( interestTarget );
	headJoint = animator.GetJointHandle( spawnArgs.GetString("bone_interestbeam") );

	if (headJoint != INVALID_JOINT)
	{
		this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);
		idVec3 dir = jointPos - interestTarget;
		dir.Normalize();
		interestBeam->SetOrigin(interestTarget + dir * INTEREST_BEAM_LENGTH);
	}
}

//Find a valid point to investigate a point. Will try to find a point that is reachable by the pathfinding system.
idVec3 idGunnerMonster::FindValidPosition(idVec3 targetPos)
{
	trace_t tr;
	idBounds actorBounds;
	trace_t boundTr;
	idVec3 interestWalkableDestination = vec3_zero;

	//Find a suitable place to investigate the interestpoint.
	//interestpoint->GetPhysics()->GetClipModel()->GetOwner()	
	gameLocal.clip.TracePoint(tr, targetPos + idVec3(0, 0, .1f), targetPos + idVec3(0, 0, -256), MASK_MONSTERSOLID, NULL);

	if (this->aas == NULL)
	{
		return this->GetPhysics()->GetOrigin();
	}

	actorBounds = this->aas->GetSettings()->boundingBoxes[0]; // Guards should only have one bounding box, so this should be fine I think
	gameLocal.clip.TraceBounds(boundTr, tr.endpos, tr.endpos, actorBounds, MASK_MONSTERSOLID, NULL); //bounding box check.

	if (boundTr.fraction >= 1.0f)
	{
		//Space is CLEAR.
		if (CanReachPosition(tr.endpos))
		{
			interestWalkableDestination = tr.endpos;
		}
	}
	
	if (interestWalkableDestination == vec3_zero)
	{
		int i;

		//Attempt to find a spot beneath the interestpoint.
		idVec3 offsets[] =
		{
			idVec3(0,32,0),
			idVec3(0,-32,0),
			idVec3(32,0,0),
			idVec3(-32,0,0),

			idVec3(32,32,0),
			idVec3(-32,32,0),
			idVec3(32,-32,0),
			idVec3(-32,-32,0),

			idVec3(0,64,0),
			idVec3(0,-64,0),
			idVec3(64,0,0),
			idVec3(-64,0,0),

			idVec3(64,64,0),
			idVec3(-64,64,0),
			idVec3(64,-64,0),
			idVec3(-64,-64,0),

			idVec3(0,96,0),
			idVec3(0,-96,0),
			idVec3(96,0,0),
			idVec3(-96,0,0),

			idVec3(96,96,0),
			idVec3(-96,96,0),
			idVec3(96,-96,0),
			idVec3(-96,-96,0)
		};

		trace_t offsetTr;
		for (i = 0; i < 24; i++)
		{
			// SW: Confirm there is solid ground under our offset location (or reasonably close, at least)
			// For some confounding reason, this is *not* weeded out by CanReachPosition()
			gameLocal.clip.TracePoint(offsetTr, tr.endpos + offsets[i] + idVec3(0, 0, 0.1f), tr.endpos + offsets[i] + idVec3(0, 0, -64), MASK_MONSTERSOLID, NULL);
			if (offsetTr.fraction < 1.0f)
			{
				// Check there is room for our bounds
				gameLocal.clip.TraceBounds(boundTr, offsetTr.endpos + idVec3(0, 0, 0.1f), offsetTr.endpos + idVec3(0, 0, 0.1f), actorBounds, MASK_MONSTERSOLID, NULL); //bounding box check.

				if (boundTr.fraction >= 1.0f)
				{
					if (CanReachPosition(offsetTr.endpos))
					{
						interestWalkableDestination = offsetTr.endpos;
						break;
					}
				}
			}
		}
	}

	if (interestWalkableDestination == vec3_zero)
	{
		//Offset bounding box check failed. So, attempt to find a searchnode that can see the investigation spot.
		idVec3 observationPoint = GetObservationViaNodes(targetPos);

		if (observationPoint != vec3_zero)
		{
			//Found a searchnode observation spot. Great.
			if (CanReachPosition(observationPoint))
			{
				interestWalkableDestination = observationPoint;
			}
		}
	}

	if (interestWalkableDestination == vec3_zero)
	{
		//if we are on top of a door, do a special check. This is mostly for ventdoors that are on the floor.
		trace_t doorTr;

		gameLocal.clip.TracePoint(doorTr, targetPos, targetPos + idVec3(0, 0, -80), MASK_MONSTERSOLID, this);

		if (doorTr.c.entityNum <= MAX_GENTITIES - 2 && doorTr.c.entityNum >= 0 && (gameLocal.entities[doorTr.c.entityNum]->IsType(idVentdoor::Type) || gameLocal.entities[doorTr.c.entityNum]->IsType(idDoor::Type)))
		{
			//interestpoint is above an idDoor.
			//do a traceline past the idDoor.

			trace_t groundTr;
			idVec3 doorPos = gameLocal.entities[doorTr.c.entityNum]->GetPhysics()->GetOrigin();
			idBounds doorBounds = gameLocal.entities[doorTr.c.entityNum]->GetPhysics()->GetBounds();
			idVec3 pointBelowDoor = doorPos + idVec3(0, 0, doorBounds[0].z);

			//Great. we now have a point that's right below the door. Nudge it down a little, just so the next tracepoint check has clearance.
			pointBelowDoor.z -= 1;

			//Trace to ground.
			gameLocal.clip.TracePoint(groundTr, pointBelowDoor, pointBelowDoor + idVec3(0, 0, -512), MASK_MONSTERSOLID, this);

			if (groundTr.fraction < 1)
			{
				trace_t actorTr;

				//Check if there's clearance for an actor on ground.
				gameLocal.clip.TraceBounds(actorTr, groundTr.endpos, groundTr.endpos, actorBounds, MASK_MONSTERSOLID, this); //bounding box check.

				if (actorTr.fraction >= 1.0f)
				{
					//There is clearance. Great. This is a valid destination for AI.
					if (CanReachPosition(actorTr.endpos))
					{
						interestWalkableDestination = actorTr.endpos;
					}
				}
			}
		}
	}

	


	if (interestWalkableDestination == vec3_zero)
	{
		//Attempt to find searchnodes that are within the same room.
		idLocationEntity *targetLocation = gameLocal.LocationForPoint(targetPos + idVec3(0,0,1));

		if (targetLocation != NULL)
		{
			idStaticList<spawnSpot_t, MAX_GENTITIES> spawnspots;

			for (idEntity* entity = gameLocal.searchnodeEntities.Next(); entity != NULL; entity = entity->aiSearchNodes.Next())
			{
				idLocationEntity *newLocation = gameLocal.LocationForPoint(entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 1));
				spawnSpot_t	spot;

				if (newLocation == NULL)
					continue;

				if (targetLocation->entityNumber == newLocation->entityNumber) //Same match.
				{
					spot.ent = entity;
					spawnspots.Append(spot);
					continue;
				}
			}

			if (spawnspots.Num() > 0)
			{
				int randomIdx = gameLocal.random.RandomInt(spawnspots.Num());
				if (CanReachPosition(spawnspots[randomIdx].ent->GetPhysics()->GetOrigin()))
				{
					interestWalkableDestination = spawnspots[randomIdx].ent->GetPhysics()->GetOrigin();
				}
			}
		}
	}

	if (interestWalkableDestination == vec3_zero)
	{
		//Ok, we STILL don't have a valid point. Let's try another check!!!!!
	
		//EXPENSIVE check. We have this check on a cooldown timer because when multiple monsters call it at the same time, it causes a performance hitch.
		if (gameLocal.time > gameLocal.lastExpensiveObservationCallTime + EXPENSIVEOBSERVATION_TIMEDELAY)
		{
			idVec3 expensiveObservationPoint;
			gameLocal.lastExpensiveObservationCallTime = gameLocal.time;
			expensiveObservationPoint = GetObservationPosition(targetPos, 1.0f, 0);
	
			if (CanReachPosition(expensiveObservationPoint))
			{
				interestWalkableDestination = expensiveObservationPoint;
			}
		}
	}

	if (interestWalkableDestination == vec3_zero)
	{
		//Errrr... every approach failed. Just don't move.
		interestWalkableDestination = this->GetPhysics()->GetOrigin();
	}
	
	return interestWalkableDestination;
}

void idGunnerMonster::ClearInterestPoint()
{
	lastInterest = NULL;
	lastInterestPriority = 0;
	interestBeam->Hide();

	if (aiState == AISTATE_OVERWATCH)
	{
		lastFireablePosition = vec3_zero;
		GotoState(AISTATE_SEARCHING);
	}
}

bool idGunnerMonster::CanAcceptStimulus()
{
	if (aiState == AISTATE_STUNNED || aiState == AISTATE_JOCKEYED || aiState == AISTATE_SPACEFLAIL || health <= 0)
		return false;

	return true;
}

void idGunnerMonster::SetAlertedState(bool investigateLKP)
{
	if (!CanAcceptStimulus())
		return; //ignore these people...

	if (investigateLKP)
	{
		if (aiState == AISTATE_COMBAT)
			return;

		//kick me into combat state.

		//if LKP is on ground, then aim up a little so we're not just aiming at feet. But if it's airborne, then aim directly at origin.
		bool groundContact = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->lkpEnt->GetPhysics()->HasGroundContacts();
		idVec3 aimOffset = groundContact ? idVec3(0, 0, 48) : vec3_zero;		

		lastFireablePosition = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->lkpEnt->GetPhysics()->GetOrigin() + aimOffset; //Place to keep eyes locked on.
		MoveToPosition(static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetLKPReachablePosition());
		GotoState(AISTATE_OVERWATCH); //Move to LKP.
	}
	else
	{
		if (aiState == AISTATE_SEARCHING)
			return;

		//go to search state.
		//Just wander around the search nodes.
		idEntity *searchnode = GetSearchNodeSpreadOut();
		if (searchnode)
		{
			//Found a valid search node. Go to it.
			if (MoveToPosition(searchnode->GetPhysics()->GetOrigin()))
			{
				currentSearchNode = searchnode;
			}
			else
			{
				//Failed to move to the searchnode for some reason... do a wander fallback.
				if (hasPathNodes)
					HasArriveAtPathNode();
				else
					WanderAround();
			}
		}
		GotoState(AISTATE_SEARCHING); //Move to LKP.
	}
}

void idGunnerMonster::FindPositionalIdlePathcorner()
{
	//When returning back to idle state, we want to make sure that the path corner we return to has a position value.
	//Otherwise, we might end up doing a path_corner_yawlook_6sec behavior on the spot, which looks weird.

	if (!currentPathTarget.IsValid() || !hasPathNodes)
		return;

	if (currentPathTarget.GetEntity()->spawnArgs.GetBool("use_position", "1"))
	{
		MoveToEntity(currentPathTarget.GetEntity());
		return;
	}

	//Ok, we have a problem now.
	//The currentpathtarget does NOT use its position. So therefore we want to iterate through the corners until we do find one that uses position.
	idEntityPtr<idEntity>	candidatePath = currentPathTarget;
	#define MAXSEARCH 32 //Assume we'll never have a patrol path more than this amount........
	for (int i = 0; i < MAXSEARCH; i++)
	{
		int numTargets = candidatePath.GetEntity()->targets.Num();
		if (numTargets <= 0)
		{
			gameLocal.Warning("FindPositionalIdlePathcorner(): '%s' path corner network is not a closed loop.", GetName());
			return;
		}

		candidatePath = candidatePath.GetEntity()->targets[0];

		if (!candidatePath.IsValid())
		{
			gameLocal.Warning("FindPositionalIdlePathcorner(): '%s' has a path corner that is invalid.", GetName());
			return;
		}

		if (candidatePath.GetEntity()->spawnArgs.GetBool("use_position", "1"))
		{
			//success.
			currentPathTarget = candidatePath;
			MoveToEntity(currentPathTarget.GetEntity());
			return;
		}
	}

	gameLocal.Warning("FindPositionalIdlePathcorner(): '%s' failed to find a path_corner that uses a position. Needs at least one path_corner that has a position.", GetName());
}

//Switch to a different state. Handle stuff that happens whenever I transition into a new state.
void idGunnerMonster::GotoState(int _state)
{
	lastState = aiState;
	aiState = _state;

	//common->Printf("newstate %d %d\n", gameLocal.time, _state);

	if (lastState == AISTATE_SUSPICIOUS && aiState != AISTATE_SUSPICIOUS)
	{
		//if exiting suspicious state, then clear out the suspicion counter.
		suspicionCounter = 0;
	}

	if (_state != AISTATE_IDLE)
	{
		//go to a state that is NOT idle.
		AI_CUSTOMIDLEANIM = false;
		AI_NODEANIM = false;
		//customIdleAnim = "";

		SetFOV(spawnArgs.GetFloat("fov_combat")); //combat fov.
	}
	else
	{
		//go to idle state.
		SetFOV(spawnArgs.GetFloat("fov")); //default.
	}

	pickpocketReactionState = PPR_NONE;

	switch (aiState)
	{
		case AISTATE_IDLE:
			AI_DAMAGE = false;
			AI_SHIELDHIT = false;
			SetState("State_Idle");
			SetColor(COLOR_IDLE);
			SetCombatState(0);
			hasAlertedFriends = false;			
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}

			if (lastState != AISTATE_IDLE && lastState != AISTATE_SUSPICIOUS)
			{
				idStr soundCue;

				if (lastState == AISTATE_COMBAT || lastState == AISTATE_COMBATOBSERVE || lastState == AISTATE_COMBATSTALK || lastState == AISTATE_OVERWATCH)
					soundCue = "snd_vo_combatend";
				else
				{
					if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetEnemiesKnowOfNina())
						soundCue = "snd_vo_idle";
					else
						soundCue = "snd_vo_idle_unaware";
				}

				gameLocal.voManager.SayVO(this, soundCue.c_str(), VO_CATEGORY_BARK);

				SetEnergyshieldActive(false);
			}
			SetFlashlight(false);

			FindPositionalIdlePathcorner();

			break;

		case AISTATE_SEARCHING:
			AI_DAMAGE = false;
			AI_SHIELDHIT = false;
			SetState("State_Searching");
			SetColor(COLOR_SEARCHING);
			Event_SetLaserActive(1); //Turn on laser.
			SetCombatState(0);
			//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SomeoneStartedSearching();
			searchWaitTimer = gameLocal.time + SEARCH_WAITTIME;
			searchMode = SEARCHMODE_SEEKINGPOSITION;
			Event_SetLaserLock(vec3_zero);
			firstSearchnodeHint = true;
			searchStartTime = gameLocal.time;

			radiocheckinPrimed = false;
			radiocheckinTimer = 0;

			if (lastState != AISTATE_SEARCHING)
			{
				if (lastEnemySeen.IsValid())
				{
					if (!lastEnemySeen.GetEntity()->IsType(idWorldspawn::Type))
					{
						bool targetIsAlive = lastEnemySeen.GetEntity()->health > 0;
						gameLocal.voManager.SayVO(this, targetIsAlive ? "snd_vo_search" : "snd_vo_killconfirmed", VO_CATEGORY_BARK); //say "I'm searching for you!" but ONLY if my last seen target is still alive.					
					}
				}
			}

			SetFlashlight(true);
			break;
		
		case AISTATE_COMBAT:
			SetState("State_Combat");
			SetColor(COLOR_COMBAT);
			Event_SetLaserActive(1); //Turn on laser.
			combatFiringTimer = gameLocal.time + COMBAT_FIRINGTIME;

			//if world state is NOT combat, then do slow mo.
			if (lastEnemySeen.IsValid())
			{
				if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_IDLE && DoesPlayerHaveLOStoMe() && lastEnemySeen.GetEntity() == gameLocal.GetLocalPlayer())
				{
					gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);					
				}
			}


			if (lastEnemySeen.IsValid())
			{
				UpdateLKP(lastEnemySeen.GetEntity());
			}
			
			hasAlertedFriends = false;

			lastFovCheck = false; //reset this, so that the updateLKP gets called.

			if (move.moveStatus != MOVE_STATUS_DONE) //If I'm moving, then stop moving.
				StopMove(MOVE_STATUS_DONE);

			// Tell manager that we are no longer paying attention to this interest point
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}

			if (lastEnemySeen.IsValid())
			{
				if ((lastState == AISTATE_IDLE || lastState == AISTATE_SUSPICIOUS || lastState == AISTATE_SEARCHING) && lastEnemySeen.GetEntity() == gameLocal.GetLocalPlayer())
				{
					//Transitioned from un-alert to combat state.
					static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateSighted();
				}
			}

			if (lastState != AISTATE_COMBAT)
			{
				gameLocal.voManager.SayVO(this, "snd_vo_startattack", VO_CATEGORY_BARK); //"I am attacking now!"
			}
			SetFlashlight(true);
			break;

		case AISTATE_SUSPICIOUS:
			SetState("State_Suspicious");
			SetColor(COLOR_SUSPICIOUS);
			Event_SaveMove();
			SetCombatState(0);
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}
			break;

		case AISTATE_COMBATOBSERVE:
			SetState("State_CombatObserve");
			SetColor(COLOR_COMBAT);
			Event_SetLaserActive(1); //Turn on laser.
			stateTimer = gameLocal.time + COMBATOBSERVE_TIME;
			intervalTimer = 0;
			SetCombatState(1, false);
			fidgetTimer = 0;
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}
			SetFlashlight(true);
			break;
		case AISTATE_COMBATSTALK:
			SetState("State_CombatStalk");
			SetColor(COLOR_COMBAT);
			Event_SetLaserActive(1); //Turn on laser.
			Event_SetLaserLock(vec3_zero);
			combatStalkInitialized = false;
			SetCombatState(1, false);
			//if (lastInterest.IsValid())
			//{
			//	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			//}
			SetFlashlight(true);

			if (lastEnemySeen.IsValid())
			{
				if (!lastEnemySeen.GetEntity()->IsType(idWorldspawn::Type))
				{
					bool targetIsAlive = lastEnemySeen.GetEntity()->health > 0;
					gameLocal.voManager.SayVO(this, targetIsAlive ? "snd_vo_movein" : "snd_vo_investigatekill", VO_CATEGORY_BARK); //say "I'm searching for you!" but ONLY if my last seen target is still alive.					
				}
			}

			break;		
		case AISTATE_OVERWATCH:
			SetState("State_Searching");
			SetColor(COLOR_SEARCHING);
			Event_SetLaserActive(1); //Turn on laser.
			SetCombatState(0);
			intervalTimer = 0;
			//static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SomeoneStartedSearching();
			searchMode = SEARCHMODE_SEEKINGPOSITION;
			Event_SetLaserLock(vec3_zero);
			SetFlashlight(true);
			break;
		case AISTATE_SPACEFLAIL:
			SetState("State_Spaceflail");
			SetCombatState(0);
			Event_SetLaserActive(0);
			SetColor(COLOR_IDLE);
			Event_SetLaserLock(vec3_zero);
			lastFovCheck = false;
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}

			//If I was the one being jockeyed, they turn off player jockey state.
			if (gameLocal.GetLocalPlayer()->IsJockeying())
			{
				if (gameLocal.GetLocalPlayer()->meleeTarget.IsValid())
				{
					if (gameLocal.GetLocalPlayer()->meleeTarget.GetEntityNum() == this->entityNumber)
					{
						gameLocal.GetLocalPlayer()->SetJockeyMode(false);
					}
				}
			}

			break;
		case AISTATE_VICTORY:
			SetState("State_Victory");
			Event_SetLaserActive(0);
			SetColor(COLOR_SEARCHING);
			Event_SetLaserLock(vec3_zero);
			stateTimer = gameLocal.time + 300;
			searchMode = SEARCHMODE_SEEKINGPOSITION;
			if (lastInterest.IsValid())
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}
		
			if (lastEnemySeen.IsValid())
			{
				MoveToPosition(lastEnemySeen.GetEntity()->GetPhysics()->GetOrigin());
			}

			break;
		case AISTATE_STUNNED:
			StopMove(MOVE_STATUS_DONE);
			SetState("State_Stunned");
			Event_SetLaserActive(0);
			Event_SetLaserLock(vec3_zero);
			stateTimer = gameLocal.time + stunTime; //amount of time to stay stunned.
			break;
		case AISTATE_JOCKEYED:
			StopMove(MOVE_STATUS_DONE);
			SetState("State_Jockeyed");
			Event_SetLaserActive(0);
			Event_SetLaserLock(vec3_zero);
			stateTimer = gameLocal.time;
			break;
		default:
			common->Error("invalid AI state %d in '%s'", state, GetName());
			break;
	}

	
}

//Get a searchnode biased toward last enemy position.
idEntity *idGunnerMonster::GetBiasedSearchNode()
{
	if (!lastEnemySeen.IsValid())
	{
		return GetSearchNode();
	}

	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnspots;	

	for (idEntity* entity = gameLocal.searchnodeEntities.Next(); entity != NULL; entity = entity->aiSearchNodes.Next())
	{
		spawnSpot_t	spot;
		trace_t		tr;

		gameLocal.clip.TracePoint(tr, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), lastEnemySeen.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), MASK_SOLID, this);
		if (tr.fraction >= 1.0f)
		{
			spot.ent = entity;
			spawnspots.Append(spot);
			continue;
		}

		//Attempt again, but do the traceline check from a slightly elevated position.
		gameLocal.clip.TracePoint(tr, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), lastEnemySeen.GetEntity()->GetPhysics()->GetOrigin() + idVec3(0, 0, 64), MASK_SOLID, this);
		if (tr.fraction >= 1.0f)
		{
			spot.ent = entity;
			spawnspots.Append(spot);
		}
	}

	if (spawnspots.Num() <= 0)
	{
		return GetSearchNode();
	}

	qsort((void *)spawnspots.Ptr(), spawnspots.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortSpawnPoints_Nearest);
	int randomIdx = gameLocal.random.RandomInt( spawnspots.Num() );

	return spawnspots[randomIdx].ent;
}

idVec3 idGunnerMonster::GetEyePositionIfPossible(idEntity *ent)
{
    if (ent->IsType(idActor::Type))
    {
        return static_cast<idActor *>(ent)->GetEyePosition();
    }
    
    return ent->GetPhysics()->GetOrigin();
}

//return TRUE if player is in darkness.
bool idGunnerMonster::TargetIsInDarkness(idEntity *ent)
{
	if (ent != gameLocal.GetLocalPlayer())
	{
		//entity is not player. Only player can be in darkness; so return false here.
		return false;		
	}

	if (gameLocal.GetLocalPlayer()->GetDarknessValue() >= 1)
	{
		//Player is not in darkness. Return false.
		return false;
	}

	//Ok, player is in darkness. However, we want to allow AI to see into the darkness at close range.
	float distanceBetweenMeAndTarget = (ent->GetPhysics()->GetOrigin() - this->GetPhysics()->GetOrigin()).LengthFast();
	
	if (distanceBetweenMeAndTarget < DARKNESS_VIEWRANGE)
		return false; //the player and the AI are very close to each other. At this close range, allow the AI to "see" into the darkness.

	return true; //player and AI are far apart, AND the player is in darkness. Return true, player is in darkness.
}

void idGunnerMonster::UpdatePet()
{
	if (gameLocal.time < gameLocal.nextPetTime)
		return;

	if (spawnArgs.GetString("def_pet")[0] == '\0') //rewrite this to be better later
	{
		gameLocal.nextPetTime = gameLocal.time + PET_SPAWN_FAILCOOLDOWN;
		return;
	}

	//check how many pets in the world. Skip if there's too many.
	int petCount = 0;
	for (idEntity* entity = gameLocal.petEntities.Next(); entity != NULL; entity = entity->petNode.Next())
	{
		petCount++;
	}

	#define MAX_PETS_IN_WORLD 1
	if (petCount >= MAX_PETS_IN_WORLD)
	{
		gameLocal.nextPetTime = gameLocal.time + PET_SPAWN_FAILCOOLDOWN;
		return;
	}


	idVec3 petSpawnPos = FindPetSpawnPos();
	if (petSpawnPos == vec3_zero)
	{
		gameLocal.nextPetTime = gameLocal.time + PET_SPAWN_FAILCOOLDOWN;
		return;
	}

	//Found a spot. Spawn it.
	idDict args;
	idEntity *petEnt;
	args.Clear();
	args.SetVector("origin", petSpawnPos);
	args.Set("owner", this->GetName());
	args.Set("classname", spawnArgs.GetString("def_pet", "monster_spearbot"));
	gameLocal.SpawnEntityDef(args, &petEnt);

	gameLocal.nextPetTime = gameLocal.time + PET_SPAWN_INTERVAL;

	gameLocal.voManager.SayVO(this, "snd_vo_deployfish", VO_CATEGORY_BARK); //"Deploying spearfish!"
}

idVec3 idGunnerMonster::FindPetSpawnPos()
{
	idVec3 forward, right;
	viewAxis.ToAngles().ToVectors(&forward, &right, NULL);
	
	//Positions that are in front of me, diagonal to me, and to my sides.
	idVec3 positionArray[] = 
	{
		idVec3(48, 0, 0),
		idVec3(48, 48, 0),
		idVec3(48,-48,0),
		idVec3(0,48,0),
		idVec3(0,-48,0)
	};

	//Bounds of the pet we spawn.
	idBounds petBounds = idBounds(idVec3(-24, -24, -24), idVec3(24, 24, 24));

	for (int i = 0; i < 5; i++)
	{
		trace_t petTr;
		idVec3 candidatePos = GetEyePosition() + (forward * positionArray[i].x) + (right * positionArray[i].y);		
		gameLocal.clip.TraceBounds(petTr, candidatePos, candidatePos, petBounds, MASK_SHOT_RENDERMODEL, NULL);

		if (petTr.fraction >= 1.0f)
			return candidatePos; //Success. Return the position.
	}

	return vec3_zero; //Fail.
}

void idGunnerMonster::UpdateEnergyshieldActivationCheck()
{
	return; //BC don't do the energy shield stuff.

	//if (energyShieldState != ENERGYSHIELDSTATE_STOWED)
	//	return; //shield already out or has been destroyed. exit here.
	//
	////check if it is time to raise the energy shield.
	//if (health >= maxHealth || health <= 0)
	//	return;
	//
	//#define TIME_UNTIL_ENERGYSHIELD_ACTIVATION 5000
	//if (gameLocal.time > searchStartTime + TIME_UNTIL_ENERGYSHIELD_ACTIVATION && gameLocal.time > lastDamagedTime + TIME_UNTIL_ENERGYSHIELD_ACTIVATION)
	//{
	//	//AI is ready. Deploy the energyshield.
	//	SetEnergyshieldActive(true);
	//}
}

void idGunnerMonster::SetEnergyshieldActive(bool value)
{
	if (value)
	{
		int lastState = energyShieldState;

		energyShieldState = ENERGYSHIELDSTATE_IDLE;
		energyShieldCurrent = energyShieldMax;

		if (energyShieldModel == NULL)
		{
			idDict args;
			args.Clear();
			args.SetVector("origin", GetPhysics()->GetOrigin());
			args.Set("model", "models/objects/energyshield/energyshield80.ase");
			args.SetInt("solid", 0);
			args.SetBool("noclipmodel", true);
			args.Set("snd_loop", "pointdefense_loop");
			args.Set("snd_start", "pointdefense_start");
			args.Set("snd_deactivate", "pointdefense_deactivate");
			energyShieldModel = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
			energyShieldModel->Bind(this, false);

			if (lastState == ENERGYSHIELDSTATE_STOWED)
				energyShieldModel->StartSound("snd_start", SND_CHANNEL_BODY2);

			energyShieldModel->StartSound("snd_loop", SND_CHANNEL_BODY);
		}
		else
		{
			energyShieldModel->Show();
		}
	}
	else
	{
		if (energyShieldState != ENERGYSHIELDSTATE_DESTROYED)
			energyShieldState = ENERGYSHIELDSTATE_STOWED; //if it's not destroyed, then stow it away.

		energyShieldCurrent = 0; //Remove shield.		

		if (energyShieldModel != NULL)
		{
			energyShieldModel->Hide();
			energyShieldModel->StopSound(SND_CHANNEL_BODY);
			energyShieldModel->StopSound(SND_CHANNEL_BODY2);

			energyShieldModel->StartSound("snd_deactivate", SND_CHANNEL_BODY3);
		}		
	}
}

void idGunnerMonster::OnShieldDestroyed()
{
	SetEnergyshieldActive(false);
}

void idGunnerMonster::SetJockeyState(bool value)
{	
	if (value)
	{
		SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 2);
		allowPain = false;
		jockeyDamageTimer = gameLocal.time + JOCKEY_DAMAGE_INITIALDELAY;
		GotoState(AISTATE_JOCKEYED);

		StartSound("snd_jockeystruggle", SND_CHANNEL_VOICE); //start choke noise.
	}
	else
	{
		//My jockey state has ended. This only gets called if the player exits, or gets kicked off, or I die via jockey slam.

		//Clear out the interestpoint, so that I prioritize investigating the player position.
		if (lastInterest.IsValid())
		{
			if (lastInterest.GetEntity() != NULL)
			{
				static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->InterestPointDistracted(lastInterest.GetEntity(), this);
			}
		}

		StopSound(SND_CHANNEL_VOICE); //stop any struggle sounds.

		allowPain = true;
		SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle_Combat", 4);
		lastFireablePosition = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 48);
		lastEnemySeen = gameLocal.GetLocalPlayer();
		GotoState(AISTATE_COMBAT);

		if (health > 0)
		{
			//Make global combat state happen.
			Eventlog_StartCombatAlert();
			SetCombatState(1, true); //Restart the combat timer.
		}

		TurnToward(lastFireablePosition);

		//Make my suspicion rate increase dramatically so that I can spot the player easily.
		highlySuspiciousTimer = gameLocal.time + HIGHLYSUSPICIOUS_TIMEINTERVAL;
	}
}

trace_t idGunnerMonster::GetJockeySmashTrace()
{
	return jockeySmashTr;
}

idMat3 idGunnerMonster::GetJockeySmashAxis()
{
	return mat3_identity;
}

//Called when the slam attack animation hits the animevent that inflicts damage.
//I'm having damage inflicted upon ME, from the player slamming ME against a wall or killentity.
void idGunnerMonster::DoJockeySlamDamage()
{
	//if (jockattackState == JOCKATK_SLAMATTACK)
	//{
	//	//Apply damage to myself. I got slammed on wall.
	//	Damage(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), vec3_zero, "damage_generic", 1.0f, 0);
	//	gameLocal.GetLocalPlayer()->SetImpactSlowmo(true); //slow mo.
	//
	//	//Make a loud interestpoint.
	//	if (jockeySmashTr.endpos != vec3_zero)
	//	{
	//		//Try to spawn it on the wall.
	//		gameLocal.SpawnInterestPoint(this, jockeySmashTr.endpos, "interest_jockeystruggle");
	//
	//		//Decal.
	//		gameLocal.ProjectDecal(jockeySmashTr.c.point, -jockeySmashTr.c.normal, 8.0f, true, gameLocal.random.RandomInt(32, 64), "textures/decals/bloodsplat03");
	//
	//		//Particles.
	//		idVec3 particlePos = jockeySmashTr.endpos + jockeySmashTr.c.normal * 4;
	//		idAngles particleAngle = jockeySmashTr.c.normal.ToAngles();
	//		particleAngle.pitch += 90;
	//		idEntityFx::StartFx("fx/jockey_slam", particlePos, particleAngle.ToMat3());
	//	}
	//	else
	//	{
	//		//If for some reason wall is unavailable, then just spawn it directly on me.
	//		gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin() + idVec3(0, 0, 56), "interest_jockeystruggle");
	//	}
	//
	//	DoJockeyWorldDamage();
	//}
	//else if (jockattackState == JOCKATK_KILLENTITYATTACK)
	//{
	//	//kill entity attack.
	//	
	//	//blow up the kill entity.
	//	if (jockeyKillEntity.IsValid())
	//	{
	//		if (jockeyKillEntity.GetEntity()->health > 0)
	//		{
	//			jockeyKillEntity.GetEntity()->Damage(this, this, vec3_zero, "damage_1000", 1.0f, 0);
	//		}
	//	}
	//	
	//	gameLocal.GetLocalPlayer()->SetImpactSlowmo(true); //slow mo.
	//	
	//	//kill self.
	//	Damage(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), vec3_zero, "damage_1000", 1.0f, 0);
	//	
	//	//tell player to dismount.
	//	gameLocal.GetLocalPlayer()->SetJockeyMode(false);
	//}

	//The killentity used to just outright kill the entity. Trying a change where we instead just apply more damage and reset the timer.

	if (jockattackState == JOCKATK_SLAMATTACK || jockattackState == JOCKATK_KILLENTITYATTACK)
	{
		//Apply damage to myself. I got slammed on wall.
		
		if (jockattackState == JOCKATK_KILLENTITYATTACK)
		{
			//I receive damage.

			if (jockeyKillEntity.IsValid())
			{
				//BC 3-5-2025: fixed bug with missing languagedict call.
				gameLocal.AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_jockey_slam"),
					common->GetLanguageDict()->GetString(displayName.c_str()),
					common->GetLanguageDict()->GetString(jockeyKillEntity.GetEntity()->displayName.c_str())),
					jockeyKillEntity.GetEntity()->GetPhysics()->GetOrigin(), true, EL_DAMAGE);

				jockeyKillEntity.GetEntity()->SetPostFlag(POST_OUTLINE_FROB, false);
			}
			
			Damage(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), vec3_zero, "damage_jockey", 1.0f, 0);

            if (health > JOCKEY_SLAMATTACK_DAMAGE)
            {
                //reset jockey grip strength.
                gameLocal.GetLocalPlayer()->ResetJockeyTimer(); //Reset the jockey grip strength.
                //("GRIP STRENGTH RESTORED.");
            }

			gameLocal.GetLocalPlayer()->SetStaminaDelta(-1); //do this so the stamina system does a little delay before recharging.

			if (health > 0)
			{
				gameLocal.GetLocalPlayer()->SetImpactSlowmo(true); //slow mo.
			}
		}
		else if (jockattackState == JOCKATK_SLAMATTACK && gameLocal.GetLocalPlayer()->IsJockeying())
		{			
            if (health > JOCKEY_SLAMATTACK_DAMAGE)
            {
                gameLocal.GetLocalPlayer()->ResetJockeyTimer(); //Reset the jockey grip strength.
                //("GRIP STRENGTH RESTORED.");
            }
		}

		//gameLocal.GetLocalPlayer()->SetImpactSlowmo(true); //slow mo.
		
		if (jockattackState == JOCKATK_SLAMATTACK && jockeySmashTr.endpos != vec3_zero)
		{			
			//Try to spawn it on the wall.
			gameLocal.SpawnInterestPoint(this, jockeySmashTr.endpos, "interest_jockeystruggle"); //Make a loud interestpoint.

			//Decal.
			if (g_bloodEffects.GetBool())
			{
				gameLocal.ProjectDecal(jockeySmashTr.c.point, -jockeySmashTr.c.normal, 8.0f, true, gameLocal.random.RandomInt(32, 64), "textures/decals/bloodsplat03");
			}

			//Particles.
			idVec3 particlePos = jockeySmashTr.endpos + jockeySmashTr.c.normal * 4;
			idAngles particleAngle = jockeySmashTr.c.normal.ToAngles();
			particleAngle.pitch += 90;
			idEntityFx::StartFx("fx/jockey_slam", particlePos, particleAngle.ToMat3());
			
		}
		else if (jockattackState == JOCKATK_KILLENTITYATTACK && jockeyKillEntity.IsValid())
		{
			//Try to spawn it on the wall.
			gameLocal.SpawnInterestPoint(this, jockeyKillEntity.GetEntity()->GetPhysics()->GetOrigin(), "interest_jockeystruggle"); //Make a loud interestpoint.

			//Decal.
			trace_t tr;
			gameLocal.clip.TracePoint(tr, GetEyePosition(), jockeyKillEntity.GetEntity()->GetPhysics()->GetOrigin(), MASK_SOLID, NULL);
			if (g_bloodEffects.GetBool())
			{
				gameLocal.ProjectDecal(tr.c.point, -tr.c.normal, 8.0f, true, 64, "textures/decals/bloodsplat03");
			}
			

			//Particles.
			idVec3 particlePos = jockeyKillEntity.GetEntity()->GetPhysics()->GetOrigin();
			idAngles particleAngle = tr.c.normal.ToAngles();
			particleAngle.pitch += 90;
			idEntityFx::StartFx("fx/jockey_slam", particlePos, particleAngle.ToMat3());

		}
		else
		{
			//If for some reason wall is unavailable, then just spawn it directly on me.
			gameLocal.SpawnInterestPoint(this, GetPhysics()->GetOrigin() + idVec3(0, 0, 56), "interest_jockeystruggle");
		}

		DoJockeyWorldDamage();

		if (jockattackState == JOCKATK_KILLENTITYATTACK)
		{
			//blow up the kill entity.
			if (jockeyKillEntity.IsValid())
			{
				if (jockeyKillEntity.GetEntity()->health > 0)
				{
					jockeyKillEntity.GetEntity()->Damage(this, this, vec3_zero, "damage_jockey_killentity", 1.0f, 0);
				}
			}
		}

		jockeyAttackCurrentlyAvailable = JOCKATKTYPE_NONE;

		// SW: Used for tutorial scripting
		idStr callOnJockeySlam;
		if (spawnArgs.GetString("callOnJockeySlam", "", callOnJockeySlam))
		{
			gameLocal.RunMapScript(callOnJockeySlam);
		}
	}

}

bool idGunnerMonster::DoJockeyNearbyDamage(trace_t damageTr, idVec3 impulseDir)
{
	if (damageTr.fraction >= 1)
		return false;
	
	if (damageTr.c.entityNum <= MAX_GENTITIES - 2 && damageTr.c.entityNum >= 0)
	{
		//Make sure it's not myself, make sure it's not player, make sure it's damageable.
		if (gameLocal.entities[damageTr.c.entityNum] != this && gameLocal.entities[damageTr.c.entityNum] != gameLocal.GetLocalPlayer() && gameLocal.entities[damageTr.c.entityNum]->fl.takedamage)
		{
			//Inflict damage on the damageable thing...
			gameLocal.entities[damageTr.c.entityNum]->AddDamageEffect(damageTr, impulseDir * 16, "damage_generic");

			gameLocal.entities[damageTr.c.entityNum]->Damage(this, this, vec3_zero, "damage_generic", 1.0f, 0);

			gameLocal.entities[damageTr.c.entityNum]->ApplyImpulse(this,
				gameLocal.entities[damageTr.c.entityNum]->GetPhysics()->GetClipModel()->GetId(),
				gameLocal.entities[damageTr.c.entityNum]->GetPhysics()->GetClipModel()->GetOrigin(),
				impulseDir * 256);

			return true;
		}
	}
	
	return false;
}

int idGunnerMonster::GetSuspicionCounter()
{
	return suspicionCounter;
}

int idGunnerMonster::GetSuspicionMaxValue()
{
	return SUSPICION_MAXPIPS;
}

bool idGunnerMonster::GetSightedFlashTime()
{
	return (sighted_flashTimer > gameLocal.time);
}

void idGunnerMonster::AddEventlog_Interestpoint(idEntity *interestpoint)
{
	if (!interestpoint->IsType(idInterestPoint::Type))
		return;

	idStr interestpointDisplayname = static_cast<idInterestPoint *>(interestpoint)->GetHUDName();
	if (interestpointDisplayname.Length() > 0)
	{
		idStr outputStr = "";

		//Discern interestpoint type.
		if (static_cast<idInterestPoint *>(interestpoint)->interesttype == IPTYPE_VISUAL)
		{
			//Visual interestpoint.			
			outputStr = idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_saw"), displayName.c_str(), interestpointDisplayname.c_str());
		}
		else
		{
			if (interestpoint->spawnArgs.GetBool("is_smelly", "0"))
			{
				//Smell interestpoint.
				outputStr = idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_smelled"), displayName.c_str(), interestpointDisplayname.c_str());
			}
			else
			{
				//Audio interestpoint.
				outputStr = idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_heard"), displayName.c_str(), interestpointDisplayname.c_str());
			}
		}

		if (outputStr.Length() > 0)
		{
			//event log for interestpoint interaction.
			gameLocal.AddEventLog(outputStr, GetPhysics()->GetOrigin(), true, EL_INTERESTPOINT);
		}
	}
}

void idGunnerMonster::SetFlashlight(bool value)
{
	if (headLight == NULL)
		return;
	
	if (!spawnArgs.GetBool("flashlight_enabled", "1"))
		return;

	if (value)
	{
		if (!headLight->Event_IsOn())
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_flashlight")));
			headLight->FadeIn(.5f);
			StartSound("snd_flashlight", SND_CHANNEL_ANY);
		}
	}
	else
	{
		if (headLight->Event_IsOn())
		{
			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin")));
			headLight->FadeOut(.5f);
			StartSound("snd_flashlight", SND_CHANNEL_ANY);
		}
	}
	
}


bool idGunnerMonster::StartDodge(idVec3 _trajectoryPosition)
{
	//if (gameLocal.time < specialTraverseTimer)
	//	return false;
	//
	//specialTraverseTimer = gameLocal.time + SPECIALTRAVERSETIMER;

	//Only do dodge if not idle.
	if (aiState == AISTATE_IDLE)
		return false;


	//Determine if we should dodge left or right. We do this by measuring what is farther from the trajectory arc point.	
	idVec3 leftPoint = GetPhysics()->GetOrigin() + viewAxis[1] * 64;
	idVec3 rightPoint = GetPhysics()->GetOrigin() + viewAxis[1] * -64;

	float distToLeftPoint = (leftPoint - _trajectoryPosition).LengthSqr();
	float distToRightPoint = (rightPoint - _trajectoryPosition).LengthSqr();

	//We now know which direction we want to go in.
	bool wantToMoveLeft = (distToLeftPoint > distToRightPoint);	

	if (wantToMoveLeft)
	{
		AI_DODGELEFT = true;
		return true;
	}
	
	if (!wantToMoveLeft)
	{
		AI_DODGERIGHT = true;		
		return true;
	}

	return false;

	//TODO: if fail, do a generic crouch?

	//We can't dodge in the direction we want. So just do nothing.
}

//Player pickpocketed me.
//After a short delay, turn around and investigate what's behind myself.
void idGunnerMonster::DoPickpocketReaction(idVec3 investigatePosition)
{
	if (!CanAcceptStimulus())
		return;

	if (pickpocketReactionState != PPR_DELAY
		&& (aiState == AISTATE_IDLE || aiState == AISTATE_SUSPICIOUS || aiState == AISTATE_COMBAT || aiState == AISTATE_COMBATOBSERVE || aiState == AISTATE_COMBATSTALK || aiState == AISTATE_SEARCHING || aiState == AISTATE_OVERWATCH))
	{
		gameLocal.voManager.SayVO(this, "snd_vo_pickpocketed", VO_CATEGORY_BARK);

		pickpocketReactionState = PPR_DELAY;
		pickpocketReactionPosition = investigatePosition;
		
		AI_CUSTOMIDLEANIM = true; //this flag makes me play the 'investigate' animation. in monster_gunner4.script , this is handled in Torso_Idle_Combat()
		customIdleAnim = "pickpocketed";
		pickpocketReactionTimer = gameLocal.time + PICKPOCKET_REACTIONTIME;
		
		StopMove(MOVE_STATUS_WAITING);
	}
}

void idGunnerMonster::Event_SetPathEntity(idEntity* pathEnt)
{
	if (pathEnt == nullptr)
	{
		gameLocal.Warning("Event_SetPathEntity: ent is null");
		return;
	}

	currentPathTarget = pathEnt;
}

void idGunnerMonster::UpdatePickpocketReaction()
{
	if (pickpocketReactionState != PPR_DELAY)
		return;
	
	if (gameLocal.time < pickpocketReactionTimer)
		return;	
	
	pickpocketReactionState = PPR_NONE;
	//AI_CUSTOMIDLEANIM = false; //force the custom animation to end.

	//gameLocal.voManager.SayVO(this, "snd_vo_pickpocketed", VO_CATEGORY_BARK);

	//if (aiState == AISTATE_COMBAT || aiState == AISTATE_COMBATOBSERVE || aiState == AISTATE_COMBATSTALK || aiState == AISTATE_OVERWATCH)
	//{
	//	//in combat state.
	//}
	//else
	{
		//in idle state. Spawn the pickpocket interestpoint. So that I turn around to investigate.

		#define PICKPOCKET_INTEREST_MAXDISTANCE 100 //make sure this number is SMALLER than the 'noticeRadius' in 'entityDef interest_pickpocketnoise'.
		idVec3 interestPos = pickpocketReactionPosition;

		if ((interestPos - GetPhysics()->GetOrigin()).Length() > PICKPOCKET_INTEREST_MAXDISTANCE)
		{
			idVec3 interestDir = interestPos - GetPhysics()->GetOrigin();
			interestDir.Normalize();

			interestPos = GetPhysics()->GetOrigin() + interestDir * PICKPOCKET_INTEREST_MAXDISTANCE;
		}

		idEntity* interestPoint = gameLocal.SpawnInterestPoint(gameLocal.GetLocalPlayer(), interestPos, spawnArgs.GetString("interest_pickpocket"));

		// SW 5th May 2025: if the interestpoint fails to spawn for some reason (this hopefully shouldn't happen),
		// then we need to manually break the pirate out of their idle animation.
		// This fixes an issue where a pirate could get locked into their pickpocket react animation.
		if (interestPoint == NULL)
		{
			AI_CUSTOMIDLEANIM = false;
		}
	}
	
}

void idGunnerMonster::Eventlog_StartCombatAlert()
{
	if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT)
		return;

	gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_startcombatalert"), displayName.c_str()), GetPhysics()->GetOrigin(), true, EL_ALERT);
}

void idGunnerMonster::DoZeroG_Unvacuumable_Check()
{
	//BC 2-15-2025: this is the visual effect that makes it more clear that heavies are not affected by vacuum suck.
	//Make a visual magnet effect on the heavy's booties and make the heavy do a hearty laugh.

	if (spawnArgs.GetBool("vacuumable", "1") || !CanAcceptStimulus() || (health <= 0))
		return;

	bool inPressurizedArea = !gameLocal.GetAirlessAtPoint(this->GetPhysics()->GetAbsBounds().GetCenter());

	if (inPressurizedArea)
	{
		//If i'm in a pressurized area, then re-enable the vacuum check.
		CanDoUnvacuumableCheck = true;
	}

	if (!CanDoUnvacuumableCheck)
		return;

	if (!inPressurizedArea)
	{
		CanDoUnvacuumableCheck = false;
		gameLocal.voManager.SayVO(this, "snd_vo_zerog_activate", VO_CATEGORY_NARRATIVE);
		gameLocal.DoParticleAng(spawnArgs.GetString("model_zerog_activate"), GetPhysics()->GetOrigin(), idAngles(0,0,0));
	}
}
