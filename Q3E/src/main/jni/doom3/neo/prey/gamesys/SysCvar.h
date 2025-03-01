// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

// HUMANHEAD
extern idCVar	g_tips;
extern idCVar	g_jawflap;
extern idCVar	g_wicked;
extern idCVar	g_casino;
extern idCVar	g_roadhouseCompleted;
extern idCVar	g_precache;
extern idCVar	g_debugProjections;
extern idCVar	g_showProjectileLaunchPoint;
extern idCVar	p_tripwireDebug;
extern idCVar	p_playerPhysicsDebug;
extern idCVar	p_camRotRateScale;
extern idCVar	p_camInterpDebug;
extern idCVar	p_iterRotMoveNumIterations;
extern idCVar	p_iterRotMoveTransDist;
extern idCVar	p_disableCamInterp;
extern idCVar	p_mountedGunDebug;
extern idCVar	g_mbNumBlurs;
extern idCVar	g_mbFrameSpan;
extern idCVar	g_postEventsDebug;
extern idCVar	g_debugger;
extern idCVar	g_nodormant;
extern idCVar	g_robustDormantAll;
extern idCVar	g_dormanttests;
extern idCVar	pm_wallwalkstepsize;
extern idCVar	g_vehicleDebug;
extern idCVar	sys_SavedPosition;
extern idCVar	g_crosshair;
extern idCVar	g_springConstant;
extern idCVar	g_debugAFs;
extern idCVar	g_debugFX;
extern idCVar	ai_debugActions;
extern idCVar	ai_debugBrain;
extern idCVar	ai_printSpeech;
extern idCVar	ai_talonAttack;
extern idCVar	ai_debugPath;
extern idCVar	ai_hideSkipThink;
extern idCVar	g_showDormant;
extern idCVar	ai_showNoAAS;
extern idCVar	ai_skipSpeech;
extern idCVar	ai_skipThink;
extern idCVar	g_useDDA;
extern idCVar	g_printDDA;
extern idCVar	g_trackDDA;
extern idCVar	g_dumpDDA;
extern idCVar	g_debugMatter;
extern idCVar	g_debugImpulse;
extern idCVar	sys_forceCache;		//mdc - added for forcing caching even if developer is set
extern idCVar	g_showGamePortals;
extern idCVar	g_showValidSoundAreas;
extern idCVar	g_testModelPitch;
extern idCVar	g_maxEntitiesWarning;
extern idCVar	g_showEntityCount;
extern idCVar	g_expensiveMS;
extern idCVar	g_nogore;
extern idCVar	g_runMapCycle;
extern idCVar	g_forceSingleSmokeView;
//HUMANHEAD END

extern idCVar	developer;

extern idCVar	g_cinematic;
extern idCVar	g_cinematicMaxSkipTime;

extern idCVar	r_aspectRatio;

extern idCVar	g_monsters;
extern idCVar	g_decals;
extern idCVar	g_knockback;
//extern idCVar	g_skill;				// HUMANHEAD pdm: not used
extern idCVar	g_gravity;
extern idCVar	g_skipFX;
extern idCVar	g_skipParticles;
extern idCVar	g_bloodEffects;
extern idCVar	g_projectileLights;
extern idCVar	g_doubleVision;
extern idCVar	g_muzzleFlash;
extern idCVar	g_ragdollDecals;

extern idCVar	g_disasm;
extern idCVar	g_debugBounds;
extern idCVar	g_debugAnim;
extern idCVar	g_debugMove;
extern idCVar	g_debugDamage;
extern idCVar	g_debugWeapon;
extern idCVar	g_debugScript;
extern idCVar	g_debugMover;
extern idCVar	g_debugTriggers;
extern idCVar	g_debugCinematic;
extern idCVar	g_stopTime;
//extern idCVar	g_armorProtection;			// HUMANHEAD pdm: not used
//extern idCVar	g_armorProtectionMP;		// HUMANHEAD pdm: not used
//extern idCVar	g_damageScale;				// HUMANHEAD pdm: not used
//extern idCVar	g_useDynamicProtection;		// HUMANHEAD pdm: not used
//extern idCVar	g_healthTakeTime;			// HUMANHEAD pdm: not used
//extern idCVar	g_healthTakeAmt;			// HUMANHEAD pdm: not used
//extern idCVar	g_healthTakeLimit;			// HUMANHEAD pdm: not used

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showCollisionTraces;
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showviewpos;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_showEnemies;

extern idCVar	g_artificialPlayerCount; //HUMANHEAD rww

extern idCVar	g_frametime;
extern idCVar	g_timeentities;

extern idCVar	ai_debugScript;
extern idCVar	ai_debugMove;
extern idCVar	ai_debugTrajectory;
extern idCVar	ai_testPredictPath;
extern idCVar	ai_showCombatNodes;
extern idCVar	ai_showPaths;
extern idCVar	ai_showObstacleAvoidance;
extern idCVar	ai_blockedFailSafe;

extern idCVar	g_dvTime;
extern idCVar	g_dvAmplitude;
extern idCVar	g_dvFrequency;

extern idCVar	g_kickTime;
extern idCVar	g_kickAmplitude;
extern idCVar	g_blobTime;
extern idCVar	g_blobSize;

extern idCVar	g_testHealthVision;
extern idCVar	g_editEntityMode;
extern idCVar	g_dragEntity;
extern idCVar	g_dragDamping;
extern idCVar	g_dragShowSelection;
extern idCVar	g_dropItemRotation;

extern idCVar	g_vehicleVelocity;
extern idCVar	g_vehicleForce;
extern idCVar	g_vehicleSuspensionUp;
extern idCVar	g_vehicleSuspensionDown;
extern idCVar	g_vehicleSuspensionKCompress;
extern idCVar	g_vehicleSuspensionDamping;
extern idCVar	g_vehicleTireFriction;

extern idCVar	ik_enable;
extern idCVar	ik_debug;

extern idCVar	af_useLinearTime;
extern idCVar	af_useImpulseFriction;
extern idCVar	af_useJointImpulseFriction;
extern idCVar	af_useSymmetry;
extern idCVar	af_skipSelfCollision;
extern idCVar	af_skipLimits;
extern idCVar	af_skipFriction;
extern idCVar	af_forceFriction;
extern idCVar	af_maxLinearVelocity;
extern idCVar	af_maxAngularVelocity;
extern idCVar	af_timeScale;
extern idCVar	af_jointFrictionScale;
extern idCVar	af_contactFrictionScale;
extern idCVar	af_highlightBody;
extern idCVar	af_highlightConstraint;
extern idCVar	af_showTimings;
extern idCVar	af_showConstraints;
extern idCVar	af_showConstraintNames;
extern idCVar	af_showConstrainedBodies;
extern idCVar	af_showPrimaryOnly;
extern idCVar	af_showTrees;
extern idCVar	af_showLimits;
extern idCVar	af_showBodies;
extern idCVar	af_showBodyNames;
extern idCVar	af_showMass;
extern idCVar	af_showTotalMass;
extern idCVar	af_showInertia;
extern idCVar	af_showVelocity;
extern idCVar	af_showActive;
extern idCVar	af_testSolid;

extern idCVar	rb_showTimings;
extern idCVar	rb_showBodies;
extern idCVar	rb_showMass;
extern idCVar	rb_showInertia;
extern idCVar	rb_showVelocity;
extern idCVar	rb_showActive;

extern idCVar	pm_jumpheight;
extern idCVar	pm_stepsize;
extern idCVar	pm_crouchspeed;
extern idCVar	pm_walkspeed;
extern idCVar	pm_runspeed;
extern idCVar	pm_noclipspeed;
extern idCVar	pm_spectatespeed;
extern idCVar	pm_spectatebbox;
extern idCVar	pm_usecylinder;
extern idCVar	pm_minviewpitch;
extern idCVar	pm_maxviewpitch;
extern idCVar	pm_stamina;
extern idCVar	pm_staminathreshold;
extern idCVar	pm_staminarate;
extern idCVar	pm_crouchheight;
extern idCVar	pm_crouchviewheight;
extern idCVar	pm_normalheight;
extern idCVar	pm_normalviewheight;
extern idCVar	pm_deadheight;
extern idCVar	pm_deadviewheight;
extern idCVar	pm_crouchrate;
extern idCVar	pm_bboxwidth;
extern idCVar	pm_crouchbob;
extern idCVar	pm_walkbob;
extern idCVar	pm_runbob;
extern idCVar	pm_runpitch;
extern idCVar	pm_runroll;
extern idCVar	pm_bobup;
extern idCVar	pm_bobpitch;
extern idCVar	pm_bobroll;
extern idCVar	pm_thirdPersonRange;
extern idCVar	pm_thirdPersonHeight;
extern idCVar	pm_thirdPersonAngle;
extern idCVar	pm_thirdPersonClip;
extern idCVar	pm_thirdPerson;
extern idCVar	pm_thirdPersonDeath;
extern idCVar	pm_thirdPersonDeathMP; //HUMANHEAD rww
extern idCVar	pm_modelView;
extern idCVar	pm_airTics;

extern idCVar	g_showAimHealth; //HUMANHEAD rww
extern idCVar	g_showPlayerShadow;
extern idCVar	g_showHud;
extern idCVar	g_showProjectilePct;
extern idCVar	g_showBrass;
extern idCVar	g_gun_x;
extern idCVar	g_gun_y;
extern idCVar	g_gun_z;
extern idCVar	g_viewNodalX;
extern idCVar	g_viewNodalZ;
extern idCVar	g_fov;
extern idCVar	g_testDeath;
extern idCVar	g_skipViewEffects;
extern idCVar   g_mpWeaponAngleScale;

extern idCVar	g_testParticle;
extern idCVar	g_testParticleName;

extern idCVar	g_testPostProcess;

extern idCVar	g_testModelRotate;
extern idCVar	g_testModelAnimate;
extern idCVar	g_testModelBlend;
extern idCVar	g_exportMask;
extern idCVar	g_flushSave;

extern idCVar	aas_test;
extern idCVar	aas_showAreas;
extern idCVar	aas_showPath;
extern idCVar	aas_showFlyPath;
extern idCVar	aas_showWallEdges;
extern idCVar	aas_showHideArea;
extern idCVar	aas_pullPlayer;
extern idCVar	aas_randomPullPlayer;
extern idCVar	aas_goalArea;
extern idCVar	aas_showPushIntoArea;

extern idCVar	net_clientPredictGUI;

extern idCVar	g_voteFlags;
extern idCVar	g_mapCycle;
extern idCVar	g_balanceTDM;

extern idCVar	si_timeLimit;
extern idCVar	si_fragLimit;
extern idCVar	si_gameType;
extern idCVar	si_map;
extern idCVar	si_spectators;

extern idCVar	net_clientSelfSmoothing;
extern idCVar	net_clientLagOMeter;

extern const char *si_gameTypeArgs[];

//extern const char *ui_skinArgs[];	// HUMANHEAD pdm: removed

#ifdef _MOD_FULL_BODY_AWARENESS
extern idCVar harm_pm_fullBodyAwareness;
extern idCVar harm_pm_fullBodyAwarenessOffset;
extern idCVar harm_pm_fullBodyAwarenessHeadJoint;
extern idCVar harm_pm_fullBodyAwarenessFixed;
extern idCVar harm_pm_fullBodyAwarenessHeadVisible;
#endif
#ifdef __ANDROID__ //karin: re-normalize player movement direction. only for DIII4A smooth onscreen joystick control
extern idCVar harm_g_normalizeMovementDirection;
#endif
#endif /* !__SYS_CVAR_H__ */
