#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"
#include "Moveable.h"
#include "idlib/LangDict.h"


#include "bc_interestpoint.h"
#include "bc_skullsaver.h"
#include "bc_meta.h"
#include "bc_lever.h"
#include "bc_airlock_accumulator.h"
#include "bc_airlock.h"


const float	AIRLOCK_INNERDOORMOVETIME	= .8f; //seconds it takes for door to open.
const float AIRLOCK_OUTERDOORMOVETIME	= .3f; //seconds it takes for door to open.


const int	INNER_PRIMEDELAY			= 300; //when pressing inner door button, delay before door opens.
const int	OUTER_PRIMEDELAY			= 1500; //ms pressurization/depressurization delay before attempting to open it.

const int	FLYFORCE					= 128;
const int	FLYFORCE_PLAYER				= 512;

const int	EMERGENCY_OPENTIME			= 3000; //when airlock is vacuum'ing out, how long to stay open.
const int	EMERGENCY_DELAYINTERVAL		= 1000; //during emergency vacuum, close inner door first, then wait XX milliseconds, then close outer door.

const int	PULLTIME_DELAY				= 300;

const char* EXT_BUTTON_SKIN =			"skins/objects/button/button_c_exterior";

const int	PNEUMATIC_SLOW_DOORTIME		= 4000; //after accumulators have exploded, make doors move slower.

//const int	LOCKDOWN_DOORTIME = 300;	//when doing airlock lockdown, make the doors slam shut very quickly.

const int	LOCKDOWN_PREAMBLE_TIME = 6000; //how long warning VO is.
const int	LOCKDOWN_OUTERDOORDELAY = 1000; //after gate slams down, delay before outer door opens.
const int	LOCKDOWN_DELAY_BEFOREALLCLEAR = 3000;

const int	DOOR_WAITTIME = 7;

const int	WARNINGSIGNS_MAX = 1;

//#define FROBINDEX_EMERGENCYBUTTON 2

#define SHUTTER_CLOSETIME 5000

CLASS_DECLARATION(idStaticEntity, idAirlock)
	EVENT(EV_PostSpawn, idAirlockAccumulator::Event_PostSpawn)
END_CLASS

idAirlock::idAirlock(void)
{
	airlockNode.SetOwner(this);
	airlockNode.AddToEnd(gameLocal.airlockEntities);

	itemVacuumSuckTimer = 0;
	canEmergencyPurge = true;
	hasPulledActors = false;

	lastVoiceprint = VOICEPRINT_A;

	shutterState = SHT_IDLE;
	shutterTimer = 0;
}

idAirlock::~idAirlock(void)
{
	airlockNode.Remove();
}

void idAirlock::Spawn(void)
{
	int i;
	idVec3 forward, right, up;
	int airlockYaw;
	int rightYaw;

	doorOffset = spawnArgs.GetInt("dooroffset", "112");
	int interiorButtonOffset = spawnArgs.GetInt("interiorbuttonoffset", "72");
	int exteriorButtonOffset = spawnArgs.GetInt("exteriorbuttonoffset", "128");
	int interiorbuttonOffsetRight = spawnArgs.GetInt("interiorbuttonoffset_right", "95");
	int exteriorbuttonOffsetRight = spawnArgs.GetInt("exteriorbuttonoffset_right", "90");

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	
	airlockYaw = round( this->GetPhysics()->GetAxis().ToAngles().yaw );
	rightYaw = round (right.ToAngles().yaw);

	//Spawn the doors.
	for (i = 0; i < 4; i++)
	{
		//0-1 = inner doors.
		//2-3 = outer doors.
		idDict args;
		bool innerdoor = (i <= 1);

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (up * (innerdoor ? 64 : 70)) + (forward * (innerdoor ? -doorOffset : doorOffset)));
		args.SetInt("no_touch", 1);
		args.SetInt("lip", 2);
		args.SetFloat("time", innerdoor ? AIRLOCK_INNERDOORMOVETIME : AIRLOCK_OUTERDOORMOVETIME);
		args.Set("owner", this->GetName());
		args.Set("snd_open", spawnArgs.GetString("snd_dooropen"));
		args.Set("snd_close", spawnArgs.GetString("snd_dooropen"));
		args.Set("snd_opened", spawnArgs.GetString("snd_doorclosed"));
		args.Set("snd_closed", spawnArgs.GetString("snd_doorclosed"));
		args.SetInt("wait", DOOR_WAITTIME);
		args.SetInt("angle", airlockYaw);
		args.SetBool("damage_only_objects", true);
		args.SetVector("shoveDir", idVec3(1, 0, 0));
		args.Set("snd_lockdownerror", "cancel3_loud");

		//Assign models.
		if (i == 0 || i == 2)
		{
			args.Set("model", spawnArgs.GetString("model_doorleft"));
			args.SetInt("movedir", rightYaw);
		}
		else
		{
			args.Set("model", spawnArgs.GetString("model_doorright"));
			args.SetInt("movedir", rightYaw + 180 );
		}

		//Assign skins.
		if (innerdoor)
		{			
			args.Set("team", idStr::Format("%s_innerdoor", this->GetName()));
			args.Set("skin", "skins/objects/doors/door_a_innerairlock");
		}
		else
		{
			args.Set("team", idStr::Format("%s_outerdoor", this->GetName()));
			args.Set("skin", "skins/objects/doors/door_a_outerairlock");
		}


		if (innerdoor)
		{
			innerDoor[i] = (idDoor *)gameLocal.SpawnEntityType(idDoor::Type, &args);
			//innerDoor[i]->SetAngles(this->GetPhysics()->GetAxis().ToAngles());

			innerDoor[i]->GetPhysics()->GetClipModel()->SetOwner(this);
		}
		else
		{
			outerDoor[i-2] = (idDoor *)gameLocal.SpawnEntityType(idDoor::Type, &args);
			//outerDoor[i-2]->SetAngles(this->GetPhysics()->GetAxis().ToAngles());

			outerDoor[i-2]->GetPhysics()->GetClipModel()->SetOwner(this);
		}
	}

	//Spawn the interior buttons. These buttons are INSIDE the airlock.
	for (i = 0; i < 2; i++)
	{
		idEntity *buttonEnt;
		idDict args;

		args.Set("classname", "env_lever3");
		args.SetInt("frobindex", i);
		args.SetInt("health", 1);
		args.SetInt("jockeyfrobdebouncetime", 4000);
		gameLocal.SpawnEntityDef(args, &buttonEnt);

		if (!buttonEnt)
		{
			gameLocal.Error("idAirlock %s failed to spawn button.\n", GetName());
		}	
		else
		{
			idVec3 buttonPos;
			idAngles buttonAng;

			buttonAng = GetPhysics()->GetAxis().ToAngles();

			if (i <= 0)
			{
				//Inner door button.
				buttonPos = GetPhysics()->GetOrigin() + (forward * -interiorButtonOffset) + (right * -interiorbuttonOffsetRight) + (up * 56);
				buttonAng.yaw -= 90;
			}
			else
			{
				//Outer door button.
				buttonPos = GetPhysics()->GetOrigin() + (forward * interiorButtonOffset) + (right * interiorbuttonOffsetRight) + (up * 56);
				buttonAng.yaw += 90;
			}

			buttonEnt->SetOrigin(buttonPos);
			buttonEnt->SetAxis(buttonAng.ToMat3());
			//buttonEnt->Bind(this, true);
			buttonEnt->GetPhysics()->GetClipModel()->SetOwner(this);
		}
	}

	//Spawn the exterior buttons. These buttons are OUTSIDE the airlock. 0 = ship interior. 1 = ship hull.
	for (i = 0; i < 2; i++)
	{
		idEntity *buttonEnt;
		idDict args;

		args.Set("classname", "env_lever_c");
		args.SetInt("health", 1);
		args.SetInt("jockeyfrobdebouncetime", 4000);
		args.SetInt("frobindex", i);
		if (i > 0)
			args.Set("skin", EXT_BUTTON_SKIN); // Give external button an alternate skin so it reacts to sunlight properly

		gameLocal.SpawnEntityDef(args, &buttonEnt);

		if (!buttonEnt)
		{
			gameLocal.Error("idAirlock %s failed to spawn button.\n", GetName());
		}
		else
		{
			idVec3 buttonPos;
			idAngles buttonAng;

			buttonAng = GetPhysics()->GetAxis().ToAngles();

			if (i <= 0)
			{
				//Inner door button.
				buttonPos = GetPhysics()->GetOrigin() + (forward * -exteriorButtonOffset) + (right * exteriorbuttonOffsetRight) + (up * 56);
				buttonAng.yaw += 180;
			}
			else
			{
				//Outer door button.
				buttonPos = GetPhysics()->GetOrigin() + (forward * exteriorButtonOffset) + (right * -exteriorbuttonOffsetRight) + (up * 64);
			}

			buttonEnt->SetOrigin(buttonPos);
			buttonEnt->SetAxis(buttonAng.ToMat3());
			//buttonEnt->Bind(this, true);
			buttonEnt->GetPhysics()->GetClipModel()->SetOwner(this);
		}
	}


	//The emergency button.
	//if (1)
	//{
	//	const idDeclEntityDef *buttonDef;
	//	idDict args;
	//
	//	args.Set("classname", "env_lever");
	//	args.Set("model", "model_button_b");
	//	args.SetInt("frobindex", FROBINDEX_EMERGENCYBUTTON);
	//	args.SetBool("frobtextoffset", true);
	//	args.Set("clipmodel", "models/objects/button/button_b_cm.ase");
	//	args.Set("displayname", "Purge");
	//	//args.Set("skin", "skins/objects/button/button_emergency");
	//	gameLocal.SpawnEntityDef(args, &emergencyButton);
	//
	//	if (!emergencyButton)
	//	{
	//		gameLocal.Error("idAirlock '%s' failed to spawn emergency button.\n", GetName());
	//	}
	//	else
	//	{
	//		idVec3 buttonPos;
	//		idAngles buttonAng;
	//
	//		buttonAng = GetPhysics()->GetAxis().ToAngles();
	//		//buttonAng.roll += 90;
	//		
	//		//Position it on the upper center.
	//		buttonPos = GetPhysics()->GetOrigin() + (forward * -exteriorButtonOffset) + (up * 122);
	//		buttonAng.yaw += 180;			
	//
	//		emergencyButton->SetOrigin(buttonPos);
	//		emergencyButton->SetAxis(buttonAng.ToMat3());
	//		emergencyButton->GetPhysics()->GetClipModel()->SetOwner(this);
	//	}
	//}


	if (spawnArgs.GetBool("vacuumseparator", "1"))
	{
		idDict args;

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -doorOffset) + (up * 64));
		args.SetInt( "angle", airlockYaw );
		args.SetBool("airlock", true);
		args.SetInt("radius", 8);
		vacuumSeparators[0] = (idVacuumSeparatorEntity *)gameLocal.SpawnEntityType(idVacuumSeparatorEntity::Type, &args); //inner door vacuumseparator.

		args.Clear();
		args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * doorOffset) + (up * 64));
		args.SetInt( "angle", airlockYaw );
		args.SetBool("airlock", true);
		args.SetInt("radius", 8);
		vacuumSeparators[1] = (idVacuumSeparatorEntity *)gameLocal.SpawnEntityType(idVacuumSeparatorEntity::Type, &args); //outer door vacuumseparator.
	}

	if (spawnArgs.GetBool("sirenlight", "1"))
	{
		idDict lightArgs;

		lightArgs.Clear();
		lightArgs.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 32));
		lightArgs.Set("texture", "lights/defaultPointLight");
		lightArgs.SetInt("noshadows", 0);
		lightArgs.Set("_color", ".4 .4 .4 1");
		lightArgs.SetFloat("light", spawnArgs.GetFloat("lightradius"));
		lightArgs.SetVector("light_offset", idVec3(0, 0, 80));
		sirenLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
		sirenLight->Bind(this, false);
	}

	airlockState = AIRLOCKSTATE_IDLE;
	thinkTimer = 0;
	lastOuterdoorOpenState = 0;
		
	GetPhysics()->SetContents(CONTENTS_SOLID);
	GetPhysics()->SetClipMask(MASK_SOLID);
	BecomeActive(TH_THINK);

	outerDoor[0]->Event_ClosePortal(); //force portals to be closed at level start.
	innerDoor[0]->Event_ClosePortal();

	pullTimer = 0;

	for (int i = 0; i < MAX_GAUGES; i++)
	{
		gauges[i] = NULL;
	}
	gaugeCount = 0;

	accumulatorsAllBroken = false;
	doorslowmodeActivated = false;


	//spawn lockdown gates.
	#define DOORDEPTH 9.5f
	for (int i = 0; i < 2; i++)
	{
		//0 = the gate that is facing OUTWARD, toward the ship interior. 1 = the gate that is facing INWARD, toward the airlock origin point.
		idAngles gateAngle = GetPhysics()->GetAxis().ToAngles();
		if (i <= 0)
			gateAngle.yaw += 180;
		
		idVec3 forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL);
		idVec3 gatePos = GetPhysics()->GetOrigin();
		if (i <= 0)
		{
			gatePos += forward * (-doorOffset - DOORDEPTH);			
		}
		else
		{
			gatePos += forward * (-doorOffset + DOORDEPTH);
		}

		idDict args;
		args.Clear();
		args.SetVector("origin", gatePos);
		args.SetFloat("angle", gateAngle.yaw);
		args.SetBool("hide", true);
		args.Set("model", "model_lockdowngate");
		gateProps[i] = (idAnimatedEntity *)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
		gateProps[i]->Event_PlayAnim("opened", 0);
	}

	lockdownPreambleState = LDPA_NONE;
	lockdownPreambleTimer = 0;


	//spawn info location
	if (1)
	{
		idDict args;
		args.Clear();
		args.SetVector("origin", GetPhysics()->GetAbsBounds().GetCenter());
		args.Set("location", "#str_loc_airlock_00105");
		idEntity *locationEnt = gameLocal.SpawnEntityType(idLocationEntity::Type, &args);
		locationEntNum = locationEnt->entityNumber;
	}

	//Spawn the FTL warning signs.
	if (1)
	{
		//for (int i = 0; i < 2; i++)
		for (int i = 0; i < WARNINGSIGNS_MAX; i++) //used to be on both sides of door; but is now just on the outside.
		{
			idVec3 forward, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

			float verticalOffset = spawnArgs.GetFloat("warnsign_vertical_offset");

			idVec3 signPos;
			idAngles signAngle = GetPhysics()->GetAxis().ToAngles();
			
			if (i <= 0)
			{
				//the sign on the hull exterior.
				float forwardAmount = spawnArgs.GetFloat("warnsign_ext_offset");
				signPos = GetPhysics()->GetOrigin() + (forward * forwardAmount) + (up * verticalOffset);
			}
			else
			{
				//the interior sign.
				float forwardAmount = spawnArgs.GetFloat("warnsign_int_offset");
				signAngle.yaw += 180; //make it face toward the center of the airlock cabin.
				signPos = GetPhysics()->GetOrigin() + (forward * forwardAmount) + (up * verticalOffset);
			}

			idDict args;
			args.Clear();
			args.SetVector("origin", signPos);
			args.SetMatrix("rotation", signAngle.ToMat3());
			args.SetBool("hide", true);
			args.Set("model", "model_ftlwarning");
			args.Set("gui", "guis/game/airlock_ftlsign.gui");
			warningsigns[i] = (idAnimatedEntity *)gameLocal.SpawnEntityType(idAnimatedEntity::Type, &args);
			warningsigns[i]->Event_PlayAnim("stowed", 0);
		}
	}

	//Spawn the FTL gui sign that displays FTL status. This screen is bound to the airlock door.
	//if (1)
	//{
	//	idVec3 forward, up, right;
	//	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	//
	//	idVec3 signPos;
	//	idAngles signAngle = GetPhysics()->GetAxis().ToAngles();
	//	signAngle.yaw += 180;
	//
	//	float forwardAmount = spawnArgs.GetFloat("ftlsign_fwd_offset", "104");
	//	signPos = GetPhysics()->GetOrigin() + (forward * forwardAmount) + (up * 92) + (right * 52);
	//
	//	idDict args;
	//	args.Clear();
	//	args.SetVector("origin", signPos);
	//	args.SetMatrix("rotation", signAngle.ToMat3());		
	//	args.Set("classname", "env_infoscreen_airlockftl");
	//	gameLocal.SpawnEntityDef(args, &ftlDoorSign);
	//
	//	if (ftlDoorSign)
	//	{
	//		ftlDoorSign->Bind(outerDoor[0], true);
	//	}
	//	else
	//	{
	//		gameLocal.Error("airlock: failed to spawn 'env_infoscreen_airlockftl'\n");
	//	}
	//}

	//Spawn the CCTV
	if (1)
	{
		idVec3 forward, up, right;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

		//Where the camera looks out of.
		float nullForwardAmount = spawnArgs.GetFloat("cctvtarget_fwd_offset", "-129");
		idVec3 nullPos = GetPhysics()->GetOrigin() + (forward * nullForwardAmount) + (up * 112);
		idAngles nullAngle = GetPhysics()->GetAxis().ToAngles();
		nullAngle.yaw += 180;
		nullAngle.pitch += 45;

		CCTV_camera = NULL;
		idDict args;
		args.Clear();
		args.SetVector("origin", nullPos);
		args.SetMatrix("rotation", nullAngle.ToMat3());
		args.Set("classname", "target_null");
		gameLocal.SpawnEntityDef(args, &CCTV_camera);

		if (!CCTV_camera)
		{
			gameLocal.Error("airlock: failed to spawn cctv target.\n");
		}

		//Now spawn the monitor model. This is the monitor that displays a CCTV to the ship interior.
		idVec3 monitorPos;
		idAngles monitorAngle = GetPhysics()->GetAxis().ToAngles();
		monitorAngle.yaw -= 45;
		monitorAngle.pitch += 45;

		float monitorForwardAmount = spawnArgs.GetFloat("cctv_fwd_offset", "-80");
		float monitorUpAmount = spawnArgs.GetFloat("cctv_up_offset", "112");
		float monitorRightAmount = spawnArgs.GetFloat("cctv_right_offset", "-80");
		monitorPos = GetPhysics()->GetOrigin() + (forward * monitorForwardAmount) + (up * monitorUpAmount) + (right * monitorRightAmount);

		//Note to keep in mind:
		//When you want something like a CCTV , make sure the entity does not have the standard 'entityGui' material.
		//You instead want to use 'textures/common/camera1'
		args.Clear();
		args.SetVector("origin", monitorPos);
		args.SetMatrix("rotation", monitorAngle.ToMat3());
		args.Set("classname", "env_infoscreen_airlockcctv");
		args.Set("cameratarget", CCTV_camera->GetName()); //points to the null we spawned above.
		gameLocal.SpawnEntityDef(args, &CCTV_monitor);

		if (!CCTV_monitor)		
		{
			gameLocal.Error("airlock: failed to spawn 'env_infoscreen_airlockcctv'\n");
		}		
	}

	//Spawn the fusebox lockdown gates.
	for (int i = 0; i < 2; i++)
	{
		//0 = the gate that is facing toward the ship interior. 1 = the gate that is facing toward space
		idAngles gateAngle = GetPhysics()->GetAxis().ToAngles();
		if (i <= 0)
			gateAngle.yaw += 180;

		idVec3 forward;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL);
		idVec3 gatePos = GetPhysics()->GetOrigin();
		if (i <= 0)
		{
			//gate facing interior
			gatePos += forward * (-doorOffset - DOORDEPTH);
			gatePos.z += 4;
		}
		else
		{
			//gate facing outer space
			gatePos += forward * (doorOffset + DOORDEPTH);
			gatePos.z += 8;
		}		

		idDict args;
		args.Clear();
		args.SetVector("origin", gatePos);
		args.SetFloat("angle", gateAngle.yaw);
		//args.SetBool("hide", true);
		args.SetBool("solid", true);
		args.Set("model", spawnArgs.GetString( "model_gate"));
		args.Set("snd_shutteropen", "shutter_3sec");
		fuseboxGates[i] = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		fuseboxGates[i]->Hide();
	}

	if (!spawnArgs.GetBool("start_on", "1"))
	{
		SetFuseboxGateOpen(false); //close the fusebox lockdown gate.
	}


	if (spawnArgs.GetBool("warninglabel", "1"))
	{
		//Spawn the pressure regulator sign inspectable
		//idVec3 forward, right, up;
		//this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

		float forwardAmount = spawnArgs.GetFloat("exteriorbuttonoffset");

		idVec3 pos = GetPhysics()->GetOrigin() + (forward * -forwardAmount) + (up * 122) + (right * -83);
		//idVec3 campos = pos + (forward * -8);
		//idVec3 inspectAngle = idVec3(0, GetPhysics()->GetAxis().ToAngles().yaw, 0);

		idEntity* inspectpoint = NULL;
		idDict args;
		args.Clear();
		args.Set("classname", "info_zoominspectpoint");
		args.SetVector("origin", pos);
		args.SetFloat("angle", GetPhysics()->GetAxis().ToAngles().yaw);
		args.SetVector("zoominspect_campos", idVec3(-15,0,0));
		args.SetVector("zoominspect_angle", idVec3(0,1,0));
		args.Set("displayname", common->GetLanguageDict()->GetString("#str_def_gameplay_airlockwarning"));
		args.Set("loc_inspectiontext", "#str_label_airlockwarning");
		gameLocal.SpawnEntityDef(args, &inspectpoint);

		//gameRenderWorld->DebugArrowSimple(campos);
	}



	PostEventMS(&EV_PostSpawn, 0);
}

void idAirlock::Save(idSaveGame *savefile) const
{
	savefile->WriteBool( canEmergencyPurge ); //  bool canEmergencyPurge
	savefile->WriteInt( airlockState ); //  int airlockState
	savefile->WriteInt( thinkTimer ); //  int thinkTimer

	savefile->WriteObject( sirenLight ); //  idLight * sirenLight

	SaveFileWriteArray(vacuumSeparators, 2, WriteObject);  // idVacuumSeparatorEntity * vacuumSeparators[2]

	SaveFileWriteArray(innerDoor, 2, WriteObject);  // idDoor * innerDoor[2]
	SaveFileWriteArray(outerDoor, 2, WriteObject);  // idDoor * outerDoor[2]

	savefile->WriteBool( lastOuterdoorOpenState ); //  bool lastOuterdoorOpenState
	savefile->WriteInt( pullTimer ); //  int pullTimer

	savefile->WriteInt( doorOffset ); //  int doorOffset

	SaveFileWriteArray(accumulatorList, accumulatorList.Num(), WriteInt);  //  idList<int> accumulatorList

	SaveFileWriteArray(gauges, gaugeCount, WriteObject); // idEntity * gauges[MAX_GAUGES];
	savefile->WriteInt( gaugeCount ); //  int gaugeCount

	savefile->WriteBool( accumulatorsAllBroken ); //  bool accumulatorsAllBroken
	savefile->WriteBool( doorslowmodeActivated ); //  bool doorslowmodeActivated


	SaveFileWriteArray(gateProps, 2, WriteObject); // idAnimatedEntity* gateProps[2];
	savefile->WriteInt( lockdownPreambleState ); //  int lockdownPreambleState
	savefile->WriteInt( lockdownPreambleTimer ); //  int lockdownPreambleTimer

	SaveFileWriteArray(warningsigns, 2, WriteObject); // idAnimatedEntity* warningsigns[2];

	savefile->WriteObject( CCTV_monitor ); //  idEntity *				 CCTV_monitor
	savefile->WriteObject( CCTV_camera ); //  idEntity *				 CCTV_camera

	savefile->WriteInt( locationEntNum ); //  int locationEntNum
	savefile->WriteInt( itemVacuumSuckTimer ); //  int itemVacuumSuckTimer
	savefile->WriteBool( hasPulledActors ); //  bool hasPulledActors
	savefile->WriteInt( lastVoiceprint ); //  int lastVoiceprint

	SaveFileWriteArray(fuseboxGates, 2, WriteObject); // idAnimatedEntity* warningsigns[2];

	savefile->WriteInt( shutterState ); //  int shutterState
	savefile->WriteInt( shutterTimer ); //  int shutterTimer
}

void idAirlock::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool( canEmergencyPurge ); //  bool canEmergencyPurge
	savefile->ReadInt( airlockState ); //  int airlockState
	savefile->ReadInt( thinkTimer ); //  int thinkTimer

	savefile->ReadObject( CastClassPtrRef( sirenLight) ); //  idLight * sirenLight

	SaveFileReadArrayCast(vacuumSeparators, ReadObject, idClass*& );  // idVacuumSeparatorEntity * vacuumSeparators[2]

	SaveFileReadArrayCast(innerDoor, ReadObject, idClass*&);  // idDoor * innerDoor[2]
	SaveFileReadArrayCast(outerDoor, ReadObject, idClass*&);  // idDoor * outerDoor[2]

	savefile->ReadBool( lastOuterdoorOpenState ); //  bool lastOuterdoorOpenState
	savefile->ReadInt( pullTimer ); //  int pullTimer

	savefile->ReadInt( doorOffset ); //  int doorOffset

	SaveFileReadList(accumulatorList, ReadInt);  //  idList<int> accumulatorList

	SaveFileReadArrayCast(gauges, ReadObject,  idClass*&); // idEntity * gauges[MAX_GAUGES];
	savefile->ReadInt( gaugeCount ); //  int gaugeCount

	savefile->ReadBool( accumulatorsAllBroken ); //  bool accumulatorsAllBroken
	savefile->ReadBool( doorslowmodeActivated ); //  bool doorslowmodeActivated


	SaveFileReadArrayCast(gateProps, ReadObject, idClass*&); // idAnimatedEntity* gateProps[2];
	savefile->ReadInt( lockdownPreambleState ); //  int lockdownPreambleState
	savefile->ReadInt( lockdownPreambleTimer ); //  int lockdownPreambleTimer

	SaveFileReadArrayCast(warningsigns, ReadObject, idClass*&); // idAnimatedEntity* warningsigns[2];

	savefile->ReadObject( CCTV_monitor ); //  idEntity *				 CCTV_monitor
	savefile->ReadObject( CCTV_camera ); //  idEntity *				 CCTV_camera

	savefile->ReadInt( locationEntNum ); //  int locationEntNum
	savefile->ReadInt( itemVacuumSuckTimer ); //  int itemVacuumSuckTimer
	savefile->ReadBool( hasPulledActors ); //  bool hasPulledActors
	savefile->ReadInt( lastVoiceprint ); //  int lastVoiceprint

	SaveFileReadArrayCast(fuseboxGates, ReadObject, idClass*&); // idAnimatedEntity* warningsigns[2];

	savefile->ReadInt( shutterState ); //  int shutterState
	savefile->ReadInt( shutterTimer ); //  int shutterTimer
}

void idAirlock::Event_PostSpawn(void) //We need to do this post-spawn because not all ents exist when Spawn() is called. So, we need to wait until AFTER spawn has happened, and call this post-spawn function.
{
	
	idLocationEntity *locationEnt = gameLocal.LocationForEntity(CCTV_camera);

	if ( !locationEnt )
	{
		gameLocal.Warning( "idAirlock %s can't find location for CCTV camera %s", GetName(), CCTV_camera->GetName() );
	}

	if (CCTV_monitor && locationEnt)
	{
		CCTV_monitor->Event_SetGuiParm("videolabel", locationEnt->GetLocation());
	}
}

void idAirlock::Think(void)
{
	if (shutterState == SHT_SHUTTERING)
	{
		float lerp = (gameLocal.time - shutterTimer) / (float)SHUTTER_CLOSETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = idMath::CubicEaseOut(lerp);

		for (int i = 0; i < 2; i++)
		{
			fuseboxGates[i]->SetShaderParm(7, lerp);
			fuseboxGates[i]->UpdateVisuals();
		}
		

		if (gameLocal.time >= shutterTimer + SHUTTER_CLOSETIME)
		{
			for (int i = 0; i < 2; i++)
			{
				fuseboxGates[i]->Hide();
			}

			shutterState = SHT_SHUTTERED;
		}
	}

	if (airlockState == AIRLOCKSTATE_INNERDOOR_PRIMING)
	{
		if (gameLocal.time >= thinkTimer && !outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen())
		{
			//outer doors are closed. Great. Attempt to open inner door.
			sirenLight->Fade(idVec4(1, 1, 1, 1), .5f);
			sirenLight->SetShader("lights/defaultPointLight");

			innerDoor[0]->Open();
			airlockState = AIRLOCKSTATE_INNERDOOR_OPEN;
		}
	}
	else if (airlockState == AIRLOCKSTATE_INNERDOOR_OPEN)
	{
		if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && !outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen())
		{
			sirenLight->Fade(idVec4(1, 1, 1, 1), .5f);
			sirenLight->SetShader("lights/defaultPointLight");			

			airlockState = AIRLOCKSTATE_IDLE;
		}
	}
	else if (airlockState == AIRLOCKSTATE_OUTERDOOR_PRIMING)
	{
		if (gameLocal.time >= thinkTimer && !innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen())
		{
			//OPEN THE OUTER DOORS
			idVec3 forward;
			idAngles particleAngle;
			idVec3 particlePos;

			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			particleAngle = GetPhysics()->GetAxis().ToAngles();
			particleAngle.pitch -= 90;
			particleAngle.yaw += 180;
			particlePos = GetPhysics()->GetOrigin() + (forward * doorOffset) + idVec3(0, 0, 64);
			idEntityFx::StartFx(spawnArgs.GetString("fx_vacuum"), particlePos, particleAngle.ToMat3());

			
			StartSound("snd_suck", SND_CHANNEL_BODY2, 0, false, NULL);


			if (gameLocal.time > pullTimer)
			{
				//Do the physics pull.
				idEntity	*entityList[MAX_GENTITIES];
				int			listedEntities, i;
				idBounds	airlockBounds;

				pullTimer = gameLocal.time + PULLTIME_DELAY;

				//detect if there's stuff waiting to be trashed...
				airlockBounds = idBounds(this->GetPhysics()->GetAbsBounds());
				airlockBounds.Expand(-4);
				listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(airlockBounds, entityList, MAX_GENTITIES);

				if (listedEntities > 0)
				{
					idVec3 flyDestination;
					flyDestination = particlePos + forward * 128; //Yank it to a position in outer space, to avoid items clumping up at the doorway

					for (i = 0; i < listedEntities; i++)
					{
						idEntity *ent = entityList[i];

						if (!ent)
						{
							continue;
						}

						if (ent == this || ent->IsHidden())
							continue;

						if (ent->IsType(idMoveableItem::Type) || ent->IsType(idMoveable::Type) || ent->IsType(idActor::Type))
						{
							//Found an entity. Give it a physics yank.
							idVec3 flyDir = flyDestination - ent->GetPhysics()->GetOrigin();
							int amountOfForce;
							flyDir.Normalize();

							if (ent == gameLocal.GetLocalPlayer())
								amountOfForce = FLYFORCE_PLAYER;
							else
								amountOfForce = FLYFORCE;

							if (ent->IsType(idSkullsaver::Type))
							{
								// SW 27th Feb 2025: Check for respawn state too
								if (static_cast<idSkullsaver *>(ent)->IsConveying() || static_cast<idSkullsaver *>(ent)->IsRespawning())
								{
									static_cast<idSkullsaver *>(ent)->ResetConveyTime();
									amountOfForce *= 2; //we really want to eject skullsavers out....give em an extra push
								}
							}

							ent->GetPhysics()->SetLinearVelocity(flyDir * amountOfForce);
						}

						if (ent->IsType(idSkullsaver::Type))
						{
							bool playerIsCarryingThisSkull = false;
							if (gameLocal.GetLocalPlayer()->GetCarryable() != nullptr)
							{
								if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
								{
									playerIsCarryingThisSkull = true;
								}
							}

							if (!playerIsCarryingThisSkull)
							{
								//Force skullsaver to get lost in space.
								static_cast<idSkullsaver*>(ent)->SetLostInSpace();
							}
						}
					}
				}
			}

			outerDoor[0]->Open();
			airlockState = AIRLOCKSTATE_OUTERDOOR_OPEN;
		}
	}
	else if (airlockState == AIRLOCKSTATE_OUTERDOOR_OPEN)
	{		
		if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && !outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen())
		{
			//ALL DOORS are closed.

			sirenLight->Fade(idVec4(1, 1, 1, 1), .5f);
			sirenLight->SetShader("lights/defaultPointLight");
			StopSound(SND_CHANNEL_BODY, false);

			airlockState = AIRLOCKSTATE_IDLE;

			gameLocal.DoGravityCheck();
		}
	}
	else if (airlockState == AIRLOCKSTATE_EMERGENCY_OPEN)
	{
		DoPhysicsPull(true, true);

		if (gameLocal.time >= thinkTimer)
		{
			//Close the inner door FIRST.
			innerDoor[0]->Close();
			airlockState = AIRLOCKSTATE_WAITINGFORINNERCLOSE;
			thinkTimer = gameLocal.time + EMERGENCY_DELAYINTERVAL;
		}
	}
	else if (airlockState == AIRLOCKSTATE_WAITINGFORINNERCLOSE)
	{		
		DoPhysicsPull(true, false);

		if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && gameLocal.time >= thinkTimer)
		{
			//Now after a delay, close the outer door.
			outerDoor[0]->Close();
			airlockState = AIRLOCKSTATE_WAITINGFORCLOSE;
		}
	}
	else if (airlockState == AIRLOCKSTATE_WAITINGFORCLOSE)
	{
		//Do physics pull as long as outer door is open.
		if (outerDoor[0]->IsOpen() || outerDoor[1]->IsOpen())
		{
			DoPhysicsPull(false, false);
		}

		if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && !outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen())
		{
			//ALL DOORS are closed.
			sirenLight->Fade(idVec4(1, 1, 1, 1), .5f);
			sirenLight->SetShader("lights/defaultPointLight");
			StopSound(SND_CHANNEL_BODY, false);
			airlockState = AIRLOCKSTATE_IDLE;
			gameLocal.DoGravityCheck();
		}
	}
	else if (airlockState == AIRLOCKSTATE_IDLE)
	{
		//The door pneumatics are now busted; make them move slower now.
		if (accumulatorsAllBroken && !doorslowmodeActivated)
		{
			doorslowmodeActivated = true;
			innerDoor[0]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
			innerDoor[1]->SetDuration(PNEUMATIC_SLOW_DOORTIME);			

			innerDoor[0]->spawnArgs.Set("snd_move", "d4_move");
			innerDoor[1]->spawnArgs.Set("snd_move", "d4_move");

			// Enable sparks when opening/closing on the inner doors
			innerDoor[0]->SetSparksEnabled(true);
			innerDoor[1]->SetSparksEnabled(true);
		}		
	}	
	else if (airlockState == AIRLOCKSTATE_LOCKDOWN_WAITINGFORINNERCLOSE)
	{
		//Lockdown purge. Waiting for inner doors to be closed.
		if (!innerDoor[0]->IsOpen() && !innerDoor[1]->IsOpen() && gameLocal.time >= thinkTimer)
		{
			//Once inner doors are closed, then show the lockdown gate.
			for (int i = 0; i < 2; i++)
			{
				gateProps[i]->Show();
				gateProps[i]->Event_PlayAnim("close", 0);
			}
			
			airlockState = AIRLOCKSTATE_LOCKDOWN_OUTERDOORDELAY;
			thinkTimer = gameLocal.time + LOCKDOWN_OUTERDOORDELAY;
			StartSound("snd_alarm", SND_CHANNEL_BODY, 0, false, NULL);
		}
	}
	else if (airlockState == AIRLOCKSTATE_LOCKDOWN_OUTERDOORDELAY)
	{
		//Lockdown purge. inner doors are closed; we now have a delay before we open the outer doors.
		if (gameLocal.time >= thinkTimer)
		{
			outerDoor[0]->Open();
			airlockState = AIRLOCKSTATE_LOCKDOWN_WAITINGFORFTL;

			sirenLight->Fade(idVec4(1, 0, 0, 1), .5f);
			sirenLight->SetShader(spawnArgs.GetString("mtr_sirenlight"));

			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->StartAllClearSequence(LOCKDOWN_DELAY_BEFOREALLCLEAR, lastVoiceprint); //purge done. start the all-clear check.

			
			//BC 2-15-2025: do vacuum suck.
			#define	HEIGHTOFFSET 64
			idVec3 forward, up;
			this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
			idVec3 innerPos = GetPhysics()->GetOrigin() + (forward * spawnArgs.GetInt("exteriorbuttonoffset")) + (forward * -32) + (up * HEIGHTOFFSET);
			idVec3 outerPos = GetPhysics()->GetOrigin() + (forward * spawnArgs.GetInt("exteriorbuttonoffset")) + (forward * 128) + (up * HEIGHTOFFSET);			
			gameLocal.DoVacuumSuctionActors(innerPos, outerPos, GetPhysics()->GetAxis().ToAngles(), true);
		}
	}


	if (lockdownPreambleState != LDPA_NONE)
	{
		//We have this on a separate state, because we still want doors to operate normally when the preamble is playing.
		//Lockdown purge. Waiting for preamble VO to end.

		if (lockdownPreambleState == LDPA_DISPATCH)
		{
			//Currently playing dispatch VO audio. This is the airlock saying "I am initiating purge"

			if (gameLocal.time >= lockdownPreambleTimer)
			{
				//Dispatch VO is done. Start the announcer.
				lockdownPreambleState = LDPA_ANNOUNCER;

				int len;
				StartSound("snd_vo_lockdown", SND_CHANNEL_VOICE2, 0, false, &len);
				lockdownPreambleTimer = gameLocal.time + len;
				
				outerDoor[0]->SetPostEvents(false);
				outerDoor[1]->SetPostEvents(false);

				gameLocal.AddEventLog("#str_def_gameplay_airlockpurge", GetPhysics()->GetOrigin(), true, EL_ALERT);
			}
		}
		else if (lockdownPreambleState == LDPA_ANNOUNCER)
		{
			//Currently playing announcer VO audio.

			if (gameLocal.time >= lockdownPreambleTimer)
			{
				airlockState = AIRLOCKSTATE_LOCKDOWN_WAITINGFORINNERCLOSE;
				thinkTimer = gameLocal.time + 300;

				//Inner doors now close very quickly.
				innerDoor[0]->SetCrusher(true);
				innerDoor[1]->SetCrusher(true);
				//innerDoor[0]->SetDuration(LOCKDOWN_DOORTIME);
				//innerDoor[1]->SetDuration(LOCKDOWN_DOORTIME);
				innerDoor[0]->Close();

				//Outer doors now stay open forever.
				outerDoor[0]->wait = -1;
				outerDoor[1]->wait = -1;

				lockdownPreambleState = LDPA_NONE;
			}
		}
	}



	//Keep track of outer door state, for the smoke effects.
	if ((outerDoor[0]->IsOpen() || outerDoor[1]->IsOpen()) && !lastOuterdoorOpenState)
	{
		lastOuterdoorOpenState = true;

		gameLocal.DoGravityCheck();
	}
	else if (!outerDoor[0]->IsOpen() && !outerDoor[1]->IsOpen() && lastOuterdoorOpenState)
	{
		//The outer doors just closed. Do the particle effect of airjets pressurizing the chamber.
		idVec3 forward, right;
		idVec3 particlePos;
		int i;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, NULL);

		for (i = 0; i < 2; i++)
		{
			idVec3 particlePos;
			particlePos = GetPhysics()->GetOrigin() + (forward * (-doorOffset + 8)) + (right * ((i <= 0) ? -40 : 40));

			
			idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
			particleAngle.pitch -= 60;
			particleAngle.yaw += 180;

			gameLocal.DoParticle(spawnArgs.GetString("model_airjets"), particlePos, particleAngle.ToForward());
		}

		lastOuterdoorOpenState = false; //Outer doors just closed.

		
	}

	idStaticEntity::Think();
}

bool idAirlock::DoFrob(int index, idEntity * frobber)
{

	if (index <= 0)
	{
		//inner door switches.

		if (!ShouldButtonsWork())
		{
			innerDoor[0]->StartSound("snd_lockdownerror", SND_CHANNEL_BODY); //Note: this sound is defined not in a .def, but in the spawn code in this file.
			return false;
		}
		
		if (outerDoor[0]->IsOpen() || outerDoor[1]->IsOpen())
			outerDoor[0]->Close();

		if (innerDoor[0]->IsOpen() || innerDoor[1]->IsOpen())
		{
			innerDoor[0]->Close();
		}
		else
		{
			if (airlockState != AIRLOCKSTATE_INNERDOOR_PRIMING)
			{
				StopSound(SND_CHANNEL_BODY, false);
				StartSound("snd_cycle", SND_CHANNEL_BODY, 0, false, NULL);

				airlockState = AIRLOCKSTATE_INNERDOOR_PRIMING;
				thinkTimer = gameLocal.time + INNER_PRIMEDELAY;
			}			

			sirenLight->Fade(idVec4(1, 1, 1, 1), .5f);
			sirenLight->SetShader("lights/defaultPointLight");
		}
	}
	else if (index == 1)
	{
		//outer door switches.

        if (airlockState == AIRLOCKSTATE_BOARDING_DOORSOPENING || airlockState == AIRLOCKSTATE_BOARDING_DOCKED)
        {
            //when boarding ship is doing stuff, disable the buttons.
            innerDoor[0]->StartSound("snd_lockdownerror", SND_CHANNEL_BODY);
            return false;
        }

		if (!ShouldButtonsWork())
		{
			if (!outerDoor[0]->IsOpen() || !outerDoor[1]->IsOpen())
			{
				outerDoor[0]->Open();
			}
			else
			{
				innerDoor[0]->StartSound("snd_lockdownerror", SND_CHANNEL_BODY); //Note: this sound is defined not in a .def, but in the spawn code in this file.
			}

			return false;
		}
		
		if (innerDoor[0]->IsOpen() || innerDoor[1]->IsOpen())
			innerDoor[0]->Close();

		if (outerDoor[0]->IsOpen() || outerDoor[1]->IsOpen())
		{
			outerDoor[0]->Close();
		}
		else
		{
			innerDoor[0]->Close();

			if (airlockState != AIRLOCKSTATE_OUTERDOOR_PRIMING)
			{
				sirenLight->Fade(idVec4(1, 0, 0, 1), .5f);
				sirenLight->SetShader(spawnArgs.GetString("mtr_sirenlight"));
				StartSound("snd_alarm", SND_CHANNEL_BODY, 0, false, NULL);

				airlockState = AIRLOCKSTATE_OUTERDOOR_PRIMING;
				//thinkTimer = gameLocal.time + OUTER_PRIMEDELAY;
				thinkTimer = gameLocal.time + 300;
			}
		}
	}
	//else if (index == FROBINDEX_EMERGENCYBUTTON)
	//{
	//	DoEmergencyPurge();
	//
	//}

	return true;
}

//This is the airlock accumulator sabotage.
void idAirlock::DoEmergencyPurge()
{

	//if airlock is locked down, deactivate the lockdown.
	if (IsAirlockLockdownActive(true))
	{
		DoAirlockLockdown(false);
	}


	//Emergency door open.
	idVec3 buttonPos, forward, up;

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	buttonPos = GetPhysics()->GetOrigin() + (forward * -128) + (up * 122);

	thinkTimer = gameLocal.time + EMERGENCY_OPENTIME;
	idEntityFx::StartFx("fx/machine_sparkdamage", &buttonPos, &mat3_identity, NULL, false);
	airlockState = AIRLOCKSTATE_EMERGENCY_OPEN;

	//make inner door open very quickly.
	innerDoor[0]->SetDuration(100);
	innerDoor[1]->SetDuration(100);

	innerDoor[0]->Open();
	outerDoor[0]->Open();

	sirenLight->Fade(idVec4(1, 0, 0, 1), .5f);
	sirenLight->SetShader(spawnArgs.GetString("mtr_sirenlight"));
	StartSound("snd_alarm", SND_CHANNEL_BODY, 0, false, NULL);

	if (1)
	{
		//Particle fx. Position it at the inner door.
		idVec3 forward;
		idAngles particleAngle;
		idVec3 particlePos;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		particleAngle = GetPhysics()->GetAxis().ToAngles();
		particleAngle.pitch -= 90;
		particleAngle.yaw += 180;
		particlePos = GetPhysics()->GetOrigin() + (forward * -128) + idVec3(0, 0, 64);
		idEntityFx::StartFx(spawnArgs.GetString("fx_vacuum"), particlePos, particleAngle.ToMat3());

		StartSound("snd_suck", SND_CHANNEL_BODY2, 0, false, NULL);
	}

	//This is a ONE-USE only button. Unuseable after pressing it once.
	//static_cast<idLever *>(emergencyButton)->SetActive(false);
	//emergencyButton->SetSkin(declManager->FindSkin("skins/objects/button/button_emergency_done"));

	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->DoHighlighter(this, NULL);
}

//totalPull = attempts to pull things that do NOT have LOS to outer space.
void idAirlock::DoPhysicsPull(bool totalPull, bool affectPlayer)
{
	#define	HEIGHTOFFSET 64		

	idVec3 forward, up;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);

	//Only pull in actors IF the lockdown gate has been lifted/unlocked.
	if (!IsFuseboxgateShut() && !hasPulledActors)
	{
		hasPulledActors = true;

		idVec3 innerPos = GetPhysics()->GetOrigin() + (forward * -spawnArgs.GetInt("exteriorbuttonoffset")) + (forward * -32) + (up * HEIGHTOFFSET);
		idVec3 outerPos = GetPhysics()->GetOrigin() + (forward * -spawnArgs.GetInt("exteriorbuttonoffset")) + (forward * 128) + (up * HEIGHTOFFSET);
		gameLocal.DoVacuumSuctionActors(innerPos, outerPos, GetPhysics()->GetAxis().ToAngles(), true);		
	}

	//Suck in items.
	idVec3 itemSuckPosInner = GetPhysics()->GetOrigin() + (forward * -spawnArgs.GetInt("exteriorbuttonoffset")) + (up * HEIGHTOFFSET);
	idVec3 itemSuckPosOuter = GetPhysics()->GetOrigin() + (forward * (spawnArgs.GetInt("exteriorbuttonoffset") + 32)) + (up * HEIGHTOFFSET);
	VacuumSuckItems(itemSuckPosInner, itemSuckPosOuter);
	


	/*
	//Do the physics pull.
	if (gameLocal.time > pullTimer)
	{
		idVec3 outerspacePullPoint, forward, up;

		pullTimer = gameLocal.time + PULLTIME_DELAY;

		this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		outerspacePullPoint = GetPhysics()->GetOrigin() + (forward * 250) + (up * 16);

		//piggyback on auto aim system.
		for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
		{
			trace_t pullpointTr;
			idVec3 frontOfMachinePos;
			idVec3 dirToEntity;
			float facingResult;

			if (!entity)
				continue;

			if (!affectPlayer && entity == gameLocal.GetLocalPlayer())
				continue;

			//Verify whether entity is in outer space.
			frontOfMachinePos = this->GetPhysics()->GetOrigin() + forward * 128; //This is the point that's right at the airlock exterior doorway threshold.
			dirToEntity = frontOfMachinePos - entity->GetPhysics()->GetOrigin();
			facingResult = DotProduct(dirToEntity, forward);

			if (facingResult < 0)
				continue;

			//Ok, entity is NOT in outer space. Suck 'em out.

			gameLocal.clip.TracePoint(pullpointTr, outerspacePullPoint, entity->GetPhysics()->GetOrigin(), MASK_SOLID, entity);

			if (pullpointTr.fraction >= 1.0f)
			{
				//Clear LOS from outer space to the entity. Yank it.
				int amountOfForce;
				idVec3 flyDir = outerspacePullPoint - entity->GetPhysics()->GetOrigin();
				flyDir.Normalize();

				if (entity == gameLocal.GetLocalPlayer())
					amountOfForce = FLYFORCE_PLAYER;
				else
					amountOfForce = FLYFORCE / 4;

				entity->GetPhysics()->SetLinearVelocity(entity->GetPhysics()->GetLinearVelocity() + (flyDir * amountOfForce));
			}
			else
			{
				if (!totalPull)
					return;

				//Ok, we now have an entity that does not have LOS to outer space. See if they're eligible for getting yanked toward the airlock door. i.e., they're in the room but are off to the side somewhere.
				idVec3 innerdoorPullpoint;

				innerdoorPullpoint = this->GetPhysics()->GetOrigin() + (forward * -128) + idVec3(0, 0, 8);

				gameLocal.clip.TracePoint(pullpointTr, innerdoorPullpoint, entity->GetPhysics()->GetOrigin(), MASK_SOLID, entity);

				if (pullpointTr.fraction >= 1.0f)
				{
					//Clear LOS from inner door to the entity. Yank it.
					int amountOfForce;
					idVec3 flyDir = innerdoorPullpoint - entity->GetPhysics()->GetOrigin();
					flyDir.Normalize();

					if (entity == gameLocal.GetLocalPlayer())
						amountOfForce = FLYFORCE_PLAYER / 2;
					else
						amountOfForce = FLYFORCE / 8;

					entity->GetPhysics()->SetLinearVelocity(entity->GetPhysics()->GetLinearVelocity() + (flyDir * amountOfForce));
				}
			}
		}
	}
	*/
}

void idAirlock::DoAccumulatorStatusUpdate(int gaugeIndex)
{
	//This gets called when an accumulator has been deflated.
	//Check all the accumulators. If they're all deflated, then we do the purge.

	if (accumulatorList.Num() <= 0)
		return;

	int numDeflated = 0;
	for (int i = 0; i < accumulatorList.Num(); i++)
	{
		int entityNum = accumulatorList[i];

		if (!gameLocal.entities[entityNum]->IsType(idAirlockAccumulator::Type))
			continue;

		if (static_cast<idAirlockAccumulator *>(gameLocal.entities[entityNum])->IsDeflated())
			numDeflated++;
	}

	//update gauge display.
	if (gaugeIndex >= 0 && gaugeIndex <= accumulatorList.Num() - 1)
	{
		#define DEADSKIN "skins/objects/airlock/gauge/skin_dead"
		const idDeclSkin *skinDecl = gauges[gaugeIndex]->GetSkin();
		if (skinDecl != declManager->FindSkin(DEADSKIN))
		{
			gauges[gaugeIndex]->SetSkin(declManager->FindSkin(DEADSKIN));
			gauges[gaugeIndex]->UpdateVisuals();

			//Make a little steam jet particle appear on the gauge.
			idAngles gaugeAngle = gauges[gaugeIndex]->GetPhysics()->GetAxis().ToAngles();
			idVec3 gaugeUp;
			gauges[gaugeIndex]->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &gaugeUp);
			gaugeAngle.pitch += 45;
			gameLocal.DoParticle("airlock_gauge01.prt", gauges[gaugeIndex]->GetPhysics()->GetOrigin() + (gaugeUp * 4.6f), gaugeAngle.ToForward(), false);
			gauges[gaugeIndex]->StartSound("snd_whistle", SND_CHANNEL_BODY);


			//Play VO sound from airlock.
			StartSound("snd_vo_pressure", SND_CHANNEL_VOICE2);
			idVec3 gaugePosition = gauges[gaugeIndex]->GetPhysics()->GetOrigin();
			gameLocal.DoParticle(spawnArgs.GetString("model_soundwave"), gaugePosition);

			//Create interestpoint
			gameLocal.SpawnInterestPoint(this, gaugePosition, spawnArgs.GetString("interest_use"));

		}
	}
	else
	{
		gameLocal.Warning("Airlock '%s' has accumulators that use an index (%d) out of range.", GetName(), gaugeIndex);
	}


	

	//Check if we should blow the airlock.
	if (numDeflated >= accumulatorList.Num())
	{
		accumulatorsAllBroken = true;

		if (canEmergencyPurge)
		{
			DoEmergencyPurge();
			StartSound("snd_emergencyopen", SND_CHANNEL_BODY);
		}
	}
	
	


}

void idAirlock::InitializeAccumulator(idEntity *accumulator)
{
	accumulatorList.Append(accumulator->entityNumber);
	
	if (gaugeCount >= MAX_GAUGES)
		return; //no more slots available.

	//Add gauge model.
	//Find good position.
	idVec3 forward, up, right;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idVec3 spawnPos = GetPhysics()->GetOrigin() + (forward * -spawnArgs.GetFloat("exteriorbuttonoffset")) + (up * 122) + (right * -63) + (right * (gaugeCount * 12));

	idAngles spawnAngle = GetPhysics()->GetAxis().ToAngles();
	spawnAngle.yaw += 180;

	idDict args;
	args.Set("classname", "func_static");
	args.Set("model", "models/objects/airlock/gauge/gauge02.ase");
	args.SetVector("origin", spawnPos);
	args.SetMatrix("rotation", spawnAngle.ToMat3());
	args.Set("snd_whistle", "airlock_gaugewhistle01");
	gameLocal.SpawnEntityDef(args, &gauges[gaugeCount]);

	gaugeCount++;
}

bool idAirlock::IsAirlockLockdownActive(bool includePreamble)
{
	if (includePreamble)
		return (lockdownPreambleState != LDPA_NONE || airlockState == AIRLOCKSTATE_LOCKDOWN_WAITINGFORINNERCLOSE || airlockState == AIRLOCKSTATE_LOCKDOWN_WAITINGFORFTL || airlockState == AIRLOCKSTATE_LOCKDOWN_OUTERDOORDELAY);
		
	return (airlockState == AIRLOCKSTATE_LOCKDOWN_WAITINGFORINNERCLOSE || airlockState == AIRLOCKSTATE_LOCKDOWN_WAITINGFORFTL || airlockState == AIRLOCKSTATE_LOCKDOWN_OUTERDOORDELAY);	
}

void idAirlock::DoAirlockLockdown(bool value, idEntity* interestpoint)
{
	if (value)
	{
		if (airlockState == AIRLOCKSTATE_BOARDING_DOORSOPENING || airlockState == AIRLOCKSTATE_BOARDING_DOCKED) //if boarding ship stuff is happening, disable the lockdown stuff.
			return;

		//Get the voiceprint of who "reported" this airlock lockdown.
		lastVoiceprint = VOICEPRINT_A;
		if (interestpoint != nullptr)
		{
			if (interestpoint->IsType(idInterestPoint::Type))
			{
				if (static_cast<idInterestPoint*>(interestpoint)->claimant.IsValid())
				{
					lastVoiceprint = static_cast<idInterestPoint*>(interestpoint)->claimant.GetEntity()->spawnArgs.GetInt("voiceprint");
				}
			}
		}

		//Activate lockdown. This is the dispatch VO of a pirate saying "we need to initiate purge"
		idStr soundCue = "snd_vo_dispatch_a";
		if (lastVoiceprint == VOICEPRINT_BOSS)
		{
			soundCue = "snd_vo_dispatch_boss";
		}
		else if (lastVoiceprint == VOICEPRINT_B)
		{
			soundCue = "snd_vo_dispatch_b";
		}
		else
		{
			soundCue = "snd_vo_dispatch_a";
		}


		//If the interestpoint was a smelly one, do smell-specific VO.
		if (interestpoint != nullptr)
		{
			if (interestpoint->spawnArgs.GetBool("is_smelly"))
			{
				if (lastVoiceprint == VOICEPRINT_BOSS)
				{
					soundCue = "snd_vo_dispatch_boss_smelly";
				}
				else if (lastVoiceprint == VOICEPRINT_B)
				{
					soundCue = "snd_vo_dispatch_b_smelly";
				}
				else
				{
					soundCue = "snd_vo_dispatch_a_smelly";
				}
			}
		}

		int len;
		StartSound(soundCue.c_str(), SND_CHANNEL_VOICE, 0, false, &len);
		lockdownPreambleState = LDPA_DISPATCH;
		lockdownPreambleTimer = gameLocal.time + len;
	}
	else
	{
		//Release lockdown. Reset the airlock settings.
		//if (doorslowmodeActivated)
		//{
		//	innerDoor[0]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
		//	innerDoor[1]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
		//}
		//else
		//{
		//	int movetime = (int)(AIRLOCK_INNERDOORMOVETIME * 1000.0f);
		//	innerDoor[0]->SetDuration(movetime);
		//	innerDoor[1]->SetDuration(movetime);
		//}


		outerDoor[0]->SetPostEvents(true);
		outerDoor[1]->SetPostEvents(true);

		innerDoor[0]->SetCrusher(false);
		innerDoor[1]->SetCrusher(false);

		outerDoor[0]->wait = DOOR_WAITTIME;
		outerDoor[1]->wait = DOOR_WAITTIME;

		airlockState = AIRLOCKSTATE_OUTERDOOR_OPEN;
		thinkTimer = gameLocal.time + 300;
		innerDoor[0]->Close();
		outerDoor[0]->Close();
				
		gateProps[0]->Event_PlayAnim("open", 0);
		gateProps[1]->Event_PlayAnim("open", 0);

		lockdownPreambleState = LDPA_NONE;
	}
}

void idAirlock::SetWarningSign(bool value)
{
	if (value)
	{
		for (int i = 0; i < WARNINGSIGNS_MAX; i++)
		{
			warningsigns[i]->Event_PlayAnim("deploy", 0);
			warningsigns[i]->Show();
		}
	}
	else
	{
		for (int i = 0; i < WARNINGSIGNS_MAX; i++)
		{
			warningsigns[i]->Event_PlayAnim("undeploy", 0);
		}
	}
}

void idAirlock::SetWarningSignText(const char *text)
{
	warningsigns[0]->Event_SetGuiParm("ftltimer", text);
}

void idAirlock::SetFTLDoorGuiNamedEvent(const char* eventName)
{
	//if (ftlDoorSign->health <= 0)
	//	return;
	//
	//ftlDoorSign->Event_GuiNamedEvent(1, eventName);
}

void idAirlock::SetFTLDoorGuiDisplaytime(const char* value)
{
	//ftlDoorSign->Event_SetGuiParm("ftltimer", value);
}

void idAirlock::StartBoardingSequence()
{
	DoAirlockLockdown(false);
	airlockState = AIRLOCKSTATE_BOARDING_DOCKED;
}

// When this gets called, the following is true:
// * the boarding ship is successfully DOCKED with the airlock.
// * We slowly open the airlock doors.
// * The ship is 'sealing' the airlock, so we block vacuum.
void idAirlock::StartBoardingDoorOpen()
{
	DoAirlockLockdown(false);
	airlockState = AIRLOCKSTATE_BOARDING_DOORSOPENING;

	innerDoor[0]->SetSparksEnabled(true);
	innerDoor[1]->SetSparksEnabled(true);
	outerDoor[0]->SetSparksEnabled(true);
	outerDoor[1]->SetSparksEnabled(true);

	innerDoor[0]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
	innerDoor[1]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
	outerDoor[0]->SetDuration(PNEUMATIC_SLOW_DOORTIME);
	outerDoor[1]->SetDuration(PNEUMATIC_SLOW_DOORTIME);

	innerDoor[0]->spawnArgs.Set("snd_move", "d4_move");
	innerDoor[1]->spawnArgs.Set("snd_move", "d4_move");
	outerDoor[0]->spawnArgs.Set("snd_move", "d4_move");
	outerDoor[1]->spawnArgs.Set("snd_move", "d4_move");

	innerDoor[0]->wait = -1;
	innerDoor[1]->wait = -1;
	outerDoor[0]->wait = -1;
	outerDoor[1]->wait = -1;

	innerDoor[0]->Open();
	outerDoor[0]->Open();


	// Since the boarding ship is docked with the airlock, we treat it as 'sealing' up the vacuum. So, even though the outer door is open, we block the vacuum.
	// SW 5th March 2025: Removing bounds expansion to prevent airlock from grabbing outermost portal by accident
	qhandle_t portal;
	idBounds bounds = idBounds(outerDoor[0]->GetPhysics()->GetAbsBounds());
	portal = gameRenderWorld->FindPortal(bounds);
	if (portal)
	{
		gameLocal.SetPortalState(portal, PS_BLOCK_AIR );
	}
}

bool idAirlock::ShouldButtonsWork()
{
	if (IsAirlockLockdownActive(false) || airlockState == AIRLOCKSTATE_BOARDING_DOORSOPENING || airlockState == AIRLOCKSTATE_BOARDING_DOCKED)
		return false;

	return true;
}

void idAirlock::VacuumSuckItems(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition)
{
	if (itemVacuumSuckTimer > gameLocal.time)
		return;

	itemVacuumSuckTimer = gameLocal.time + 100;

	//Suck world items out.
	idLocationEntity *innerLocEnt = gameLocal.LocationForPoint(suctionInteriorPosition);
	if (innerLocEnt == NULL)
		return;

	idVec3 suctionDirection = suctionExteriorPosition - suctionInteriorPosition;
	suctionDirection.Normalize();

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if ((ent->IsType(idActor::Type) && ent->health > 0) || ent->IsHidden() || !ent->spawnArgs.GetBool("zerog", "0") || ent->isInOuterSpace())
			continue;

		//Check if object is in the same room.
		idLocationEntity *entLoc = gameLocal.LocationForEntity(ent);
		if (entLoc == NULL)
			continue;

		//If the item is not in the room AND is not in the airlock, then exit.
		if (entLoc->entityNumber != innerLocEnt->entityNumber && entLoc->entityNumber != locationEntNum)
			continue;	

		// SW 1st April 2025: don't try to drag the item out of the player's hands
		idEntity* carryable = gameLocal.GetLocalPlayer()->GetCarryable();
		if (carryable && carryable->entityNumber == ent->entityNumber)
			continue;

		//entity is in the room.
		trace_t trInterior, trExterior;
		idVec3 pointToPullTo;
		gameLocal.clip.TracePoint(trInterior, ent->GetPhysics()->GetOrigin(), suctionInteriorPosition, MASK_SOLID, NULL);
		gameLocal.clip.TracePoint(trExterior, ent->GetPhysics()->GetOrigin(), suctionExteriorPosition, MASK_SOLID, NULL);

		//gameRenderWorld->DebugArrow(trExterior.fraction >= 1 ? colorGreen : colorRed, ent->GetPhysics()->GetOrigin(), trExterior.endpos, 4, 60000);

		#define	VACUUM_OBJECT_CORPSEFORCE 24
		#define	VACUUM_OBJECT_NORMALFORCE 48
		#define VACUUM_OBJECT_SPACESUCK 256
		int pushForce;
		if (trExterior.fraction >= 1)
		{
			//pull to outer space.
			pointToPullTo = suctionExteriorPosition;
			pushForce = VACUUM_OBJECT_SPACESUCK;
		}
		else if (trInterior.fraction >= 1)
		{
			//If it's behind the suctionInterior point, then ignore it.		
			idVec3 dirToEntity = suctionInteriorPosition - ent->GetPhysics()->GetOrigin();
			dirToEntity.Normalize();
			float facingResult = DotProduct(dirToEntity, suctionDirection);
			if (facingResult < 0)
				continue;
			
			//pull toward airlock door
			pointToPullTo = suctionInteriorPosition;
			pushForce = VACUUM_OBJECT_NORMALFORCE;
			//gameRenderWorld->DebugArrow(colorGreen, ent->GetPhysics()->GetOrigin(), force * 32, 8, 10000);
		}
		else
			continue; //no LOS to anything. exit.


		if (ent->IsType(idActor::Type) && ent->health <= 0)
		{
			pushForce = VACUUM_OBJECT_CORPSEFORCE; //for ragdoll corpses we don't want to fling it too hard
		}

		//Apply physics impulse.
		
		idVec3 entityCenter = ent->GetPhysics()->GetAbsBounds().GetCenter();
		idVec3 force = pointToPullTo - entityCenter;
		force.Normalize();
		//gameRenderWorld->DebugArrow(colorGreen, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + force * 64, 8, 10000); //draw forces pulling the item.
		ent->ApplyImpulse(ent, 0, entityCenter, force * pushForce * ent->GetPhysics()->GetMass());
	}
}

void idAirlock::SetFuseboxGateOpen(bool value)
{
	for (int i = 0; i < 2; i++)
	{
		if (value)
		{
			//BC 2-18-2025: airlock shutter logic
			//Open the shutter....
			if (shutterState != SHT_SHUTTERING)
			{
				fuseboxGates[i]->StartSound("snd_shutteropen", SND_CHANNEL_BODY);
			}

			shutterState = SHT_SHUTTERING;
			shutterTimer = gameLocal.time;			

			fuseboxGates[i]->fl.takedamage = false;
			fuseboxGates[i]->GetPhysics()->SetContents(0);
		}
		else
		{
			fuseboxGates[i]->Show();
		}
	}
}

void idAirlock::SetCanEmergencyPurge(bool value)
{
	canEmergencyPurge = value;
}

bool idAirlock::IsFuseboxgateShut()
{
	for (int i = 0; i < 2; i++)
	{
		if (!fuseboxGates[i]->IsHidden())
			return true;
	}

	return false;
}
