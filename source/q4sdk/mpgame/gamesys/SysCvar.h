
#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

extern idCVar	developer;

extern idCVar	g_cinematic;
extern idCVar	g_cinematicMaxSkipTime;

// RAVEN BEGIN
// jnewquist: vertical stretch for letterboxed cinematics authored for 4:3 aspect
extern idCVar	g_fixedHorizFOV;
// RAVEN END

extern idCVar	g_monsters;
extern idCVar	g_decals;
extern idCVar	g_knockback;
extern idCVar	g_skill;
extern idCVar	g_gravity;
extern idCVar	g_mp_gravity;
extern idCVar	g_skipFX;
extern idCVar	g_skipParticles;
extern idCVar	g_projectileLights;
extern idCVar	g_doubleVision;
extern idCVar	g_muzzleFlash;

extern idCVar	g_nailTrail;
extern idCVar	g_grenadeTrail;
extern idCVar	g_rocketTrail;
extern idCVar	g_railTrail;
extern idCVar	g_napalmTrail;

extern idCVar	g_predictedProjectiles;

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
// RAVEN BEGIN
// bdube: added
extern idCVar	g_debugState;
extern idCVar	g_stopTime;
extern idCVar	g_armorProtection;
extern idCVar	g_armorProtectionMP;
//extern idCVar	g_damageScale;
// jsinger: added to support binary read/write
extern idCVar	com_BinaryRead;
#ifdef RV_BINARYDECLS
extern idCVar	com_BinaryDeclRead;
#endif
// jsinger: added to support loading all decls from a single file
#ifdef RV_SINGLE_DECL_FILE
extern idCVar	com_SingleDeclFile;
extern idCVar	com_WriteSingleDeclFile;
#endif
extern idCVar	com_BinaryWrite;
// RAVEN END
extern idCVar	g_useDynamicProtection;
extern idCVar	g_healthTakeTime;
extern idCVar	g_healthTakeAmt;
extern idCVar	g_healthTakeLimit;

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showCollisionTraces;
// RAVEN BEGIN
// ddynerman: SD's clip sector code
extern idCVar	g_showClipSectors;
extern idCVar	g_showClipSectorFilter;
extern idCVar	g_showAreaClipSectors;
// RAVEN END
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showviewpos;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_showEnemies;
extern idCVar	g_frametime;
extern idCVar	g_timeentities;

// RAVEN BEGIN
// bdube: new debug cvar
extern idCVar	g_debugVehicle;
extern idCVar	g_showFrameCmds;
extern idCVar	g_showGodDamage;
// RAVEN END

// RAVEN BEGIN
// twhitaker: debug cvars for rvVehicleDriver
extern idCVar	g_debugVehicleDriver;
extern idCVar	g_debugVehicleAI;
extern idCVar	g_vehicleMode;
// RAVEN END
extern idCVar	g_allowVehicleGunOverheat;

extern idCVar	ai_debugScript;
extern idCVar	ai_debugMove;
extern idCVar	ai_debugTrajectory;
extern idCVar	ai_debugTactical;
extern idCVar	ai_debugFilterString;
extern idCVar	ai_testPredictPath;
extern idCVar	ai_showCombatNodes;
extern idCVar	ai_showPaths;
extern idCVar	ai_showObstacleAvoidance;
extern idCVar	ai_blockedFailSafe;
extern idCVar	ai_debugSquad;
extern idCVar	ai_debugStealth;
extern idCVar	ai_allowTacticalRush;

// RAVEN BEGIN
// nmckenzie: added speeds and freeze
extern idCVar	ai_speeds;
extern idCVar	ai_freeze;
extern idCVar	ai_animShow;
extern idCVar	ai_showCover;
extern idCVar	ai_showTacticalFeatures;
extern idCVar	ai_disableEntTactical;
extern idCVar	ai_disableAttacks;
extern idCVar	ai_disableSimpleThink;
extern idCVar	ai_disableCover;
extern idCVar	ai_debugHelpers;
// cdr: added new master move type
extern idCVar	ai_useRVMasterMove;
//jshepard: allow old AAS files
extern idCVar	ai_allowOldAAS;
// twhitaker: debugging support for eye focus
extern idCVar	ai_debugEyeFocus;
//mcg: always allow player to push buddies, unless scripted
extern idCVar	ai_playerPushAlways;
// RAVEN END

extern idCVar	g_dvTime;
extern idCVar	g_dvAmplitude;
extern idCVar	g_dvFrequency;

extern idCVar	g_kickTime;
extern idCVar	g_kickAmplitude;
extern idCVar	g_blobTime;
extern idCVar	g_blobSize;

extern idCVar	g_testHealthVision;
extern idCVar	g_editEntityMode;
// RAVEN BEGIN
extern idCVar	g_editEntityDistance;
// rhummer: Allow to customize the distance the text is drawn for edit entities, Zack request.
extern idCVar	g_editEntityTextDistance;
// rjohnson: entity usage stats
extern idCVar	g_keepEntityStats;
// RAVEN END
extern idCVar	g_dragEntity;
extern idCVar	g_dragDamping;
extern idCVar	g_dragShowSelection;
extern idCVar	g_dropItemRotation;

extern idCVar	g_vehicleVelocity;
extern idCVar	g_vehicleForce;

extern idCVar	hud_showSpeed;
extern idCVar	hud_showInput;
extern idCVar	hud_inputPosition;
extern idCVar	hud_inputColor;

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
// RAVEN BEGIN
extern idCVar	pm_speed;
extern idCVar	pm_walkspeed;
extern idCVar	pm_zoomedSlow;
extern idCVar 	pm_isZoomed;
// RAVEN END
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
extern idCVar	pm_modelView;
extern idCVar	pm_airTics;

// nmckenzie: added ability to try alternate accelerations.
extern idCVar	pm_acceloverride;
extern idCVar	pm_frictionoverride;
extern idCVar	pm_forcespectatormove;
extern idCVar	pm_thirdPersonTarget;
// bdube: vehicle
extern idCVar	pm_vehicleLean;
extern idCVar	pm_vehicleCameraSnap;
extern idCVar	pm_vehicleCameraScaleMax;
extern idCVar	pm_vehicleSoundLerpScale;
extern idCVar	pm_vehicleCameraSpeedScale;
extern idCVar	pm_vehicleCameraMinDist;
// RAVEN END

extern idCVar	g_showPlayerShadow;

extern idCVar	g_skipPlayerShadowsMP;
extern idCVar	g_skipItemShadowsMP;

extern idCVar	g_simpleItems;
extern idCVar	g_showHud;
// RAVEN BEGIN
extern idCVar	g_crosshairColor;
// cnicholson: Custom Crosshair 
extern idCVar	g_crosshairCustom;
extern idCVar	g_crosshairCustomFile;
extern idCVar	g_crosshairCharInfoFar;
// bdube: hud popups
extern idCVar	g_showHudPopups;
// bdube: range
extern idCVar	g_showRange;
// bdube: debug hud
extern idCVar	g_showDebugHud;
// RAVEN END
extern idCVar	g_showProjectilePct;
// RAVEN BEGIN
// bdube: brass time
extern idCVar	g_brassTime;
// RAVEN END
extern idCVar	g_gun_x;
extern idCVar	g_gun_y;
extern idCVar	g_gun_z;
extern idCVar	g_weaponFovEffect;
// RAVEN BEGIN
// bdube: cvar for messing with foreshortening
extern idCVar	g_gun_pitch;
extern idCVar	g_gun_yaw;
extern idCVar	g_gun_roll;
// abahr:
extern idCVar	g_gunViewStyle;
// jscott: for playbacks
extern idCVar	g_showPlayback;
extern idCVar	g_currentPlayback;
// RAVEN END
extern idCVar	g_viewNodalX;
extern idCVar	g_viewNodalZ;
extern idCVar	g_fov;
extern idCVar	g_testDeath;
extern idCVar	g_skipViewEffects;
extern idCVar   g_mpWeaponAngleScale;

extern idCVar	g_testParticle;
extern idCVar	g_testParticleName;
// RAVEN BEGIN
// bdube: more rigid body debug
extern idCVar	rb_showContacts;
// RAVEN END

extern idCVar	g_testPostProcess;

extern idCVar	g_testModelRotate;
extern idCVar	g_testModelAnimate;
extern idCVar	g_testModelBlend;

extern idCVar	g_forceModel;
extern idCVar	g_forceStroggModel;
extern idCVar	g_forceMarineModel;

// RAVEN BEGIN
// bdube: test scoreboard
extern idCVar	g_testScoreboard;
extern idCVar	g_testPlayer;
// RAVEN END
extern idCVar	g_exportMask;
extern idCVar	g_flushSave;

extern idCVar	aas_test;
extern idCVar	aas_showAreas;
extern idCVar	aas_showAreaBounds;
extern idCVar	aas_showPath;
extern idCVar	aas_showFlyPath;
extern idCVar	aas_showWallEdges;
extern idCVar	aas_showHideArea;
extern idCVar	aas_pullPlayer;
extern idCVar	aas_randomPullPlayer;
extern idCVar	aas_goalArea;
extern idCVar	aas_showPushIntoArea;
// RAVEN BEGIN
// rjohnson: added aas help
extern idCVar	aas_showProblemAreas;
// cdr: added rev reach
extern idCVar	aas_showRevReach;
// RAVEN END

extern idCVar	net_clientPredictGUI;

extern idCVar	si_voteFlags;
extern idCVar	g_mapCycle;
// RAVEN BEGIN
// shouchard:  g_balanceTDM->g_balanceTeams so we can also use it for CTF
extern idCVar	si_autobalance;
// RAVEN END

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
extern idCVar	si_isBuyingEnabled;
extern idCVar	si_dropWeaponsInBuyingModes;
extern idCVar	si_controlTime;
// RITUAL END

extern idCVar	si_timeLimit;
extern idCVar	si_fragLimit;
extern idCVar	si_gameType;
extern idCVar	si_map;
extern idCVar	si_mapCycle;
extern idCVar	si_spectators;
extern idCVar	si_minPlayers;
// RAVEN BEGIN
// shouchard:  CTF
extern idCVar	si_captureLimit;
// shouchard:  Tourney
extern idCVar	si_tourneyLimit;
// RAVEN END

extern const char *si_gameTypeArgs[];

extern idCVar	g_gamelog;
extern idCVar	cl_showEntityInfo;
extern idCVar	g_forceUndying;
extern idCVar g_perfTest_weaponNoFX;
extern idCVar g_perfTest_hitscanShort;
extern idCVar g_perfTest_hitscanBBox;
extern idCVar g_perfTest_aiStationary;
extern idCVar g_perfTest_aiNoDodge;
extern idCVar g_perfTest_aiNoRagdoll;
extern idCVar g_perfTest_aiNoObstacleAvoid;
extern idCVar g_perfTest_aiUndying;
extern idCVar g_perfTest_aiNoVisTrace;
extern idCVar g_perfTest_noJointTransform;
extern idCVar g_perfTest_noPlayerFocus;
extern idCVar g_perfTest_noProjectiles;

extern idCVar net_clientLagOMeter;
extern idCVar net_clientLagOMeterResolution;

extern idCVar net_warnStale;

extern idCVar ri_useViewerPass;
extern idCVar ri_privateViewers;
extern idCVar ri_numViewers;
extern idCVar ri_numPrivateViewers;
extern idCVar ri_name;

extern idCVar g_noTVChat;

extern idCVar pm_slidevelocity;
extern idCVar pm_powerslide;

extern idCVar g_playerLean;

extern idCVar net_clientPredictWeaponSwitch;

#endif /* !__SYS_CVAR_H__ */
