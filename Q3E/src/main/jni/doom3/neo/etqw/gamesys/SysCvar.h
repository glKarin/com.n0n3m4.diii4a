// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

class idCVar;

extern idCVar	si_name;
extern idCVar	si_teamDamage;
extern idCVar	si_needPass;
extern idCVar	si_timeLimit;
extern idCVar	si_maxPlayers;
extern idCVar	si_privateClients;
extern idCVar	si_campaignInfo;
extern idCVar	si_adminStart;
extern idCVar	si_disableVoting;
extern idCVar	si_teamForceBalance;
extern idCVar	si_allowLateJoin;
extern idCVar	si_readyPercent;
extern idCVar	si_minPlayers;
extern idCVar	si_spectators;
extern idCVar	si_rules;
extern idCVar	si_noProficiency;
extern idCVar	si_serverURL;


extern idCVar	r_aspectRatio;
extern idCVar	r_customAspectRatioH;
extern idCVar	r_customAspectRatioV;

extern idCVar	g_decals;
extern idCVar	g_knockback;
extern idCVar	g_gravity;

extern idCVar	g_disasm;
extern idCVar	g_debugBounds;
extern idCVar	g_debugAnim;
extern idCVar	g_debugAnimStance;
extern idCVar	g_debugDamage;
extern idCVar	g_debugWeapon;
extern idCVar	g_debugWeaponSpread;
extern idCVar	g_debugScript;
extern idCVar	g_debugCinematic;

extern idCVar	g_debugPlayerList;

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showVehiclePathNodes;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showRenderModelBounds;
extern idCVar	g_collisionModelMask;
extern idCVar	g_showCollisionTraces;
extern idCVar	g_showClipSectors;
extern idCVar	g_showClipSectorFilter;
extern idCVar	g_showAreaClipSectors;
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_debugMask;
extern idCVar	g_debugLocations;
extern idCVar	g_showActiveDeployZones;

extern idCVar	g_disableVehicleSpawns;

extern idCVar	g_frametime;
//extern idCVar	g_timeentities;
//extern idCVar	g_timetypeentities;

extern idCVar	ai_debugScript;
extern idCVar	ai_debugAnimState;
extern idCVar	ai_debugMove;
extern idCVar	ai_debugTrajectory;

extern idCVar	g_kickTime;
extern idCVar	g_kickAmplitude;
//extern idCVar	g_blobTime;
//extern idCVar	g_blobSize;

extern idCVar	g_editEntityMode;
extern idCVar	g_dragEntity;
extern idCVar	g_dragDamping;
extern idCVar	g_dragMaxforce;
extern idCVar	g_dragShowSelection;

extern idCVar	g_vehicleVelocity;
extern idCVar	g_vehicleForce;
extern idCVar	g_vehicleSuspensionUp;
extern idCVar	g_vehicleSuspensionDown;
extern idCVar	g_vehicleSuspensionKCompress;
extern idCVar	g_vehicleSuspensionDamping;
extern idCVar	g_vehicleTireFriction;

extern idCVar	g_commandMapZoomStep;
extern idCVar	g_commandMapZoom;

extern idCVar	g_showPlayerSpeed;

extern idCVar	m_helicopterPitch;
extern idCVar	m_helicopterYaw;

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
extern idCVar	rb_showContacts;

extern idCVar	pm_friction;
extern idCVar	pm_jumpheight;
extern idCVar	pm_stepsize;
extern idCVar	pm_crouchspeed;
extern idCVar	pm_pronespeed;
extern idCVar	pm_walkspeed;
extern idCVar	pm_runspeedforward;
extern idCVar	pm_runspeedback;
extern idCVar	pm_runspeedstrafe;
extern idCVar	pm_sprintspeedforward;
extern idCVar	pm_sprintspeedstrafe;
extern idCVar	pm_noclipspeed;
extern idCVar	pm_noclipspeedsprint;
extern idCVar	pm_noclipspeedwalk;
extern idCVar	pm_democamspeed;
extern idCVar	pm_spectatespeed;
extern idCVar	pm_spectatespeedsprint;
extern idCVar	pm_spectatespeedwalk;
extern idCVar	pm_spectatebbox;
extern idCVar	pm_minviewpitch;
extern idCVar	pm_maxviewpitch;
extern idCVar	pm_minproneviewpitch;
extern idCVar	pm_maxproneviewpitch;
extern idCVar	pm_proneheight;
extern idCVar	pm_proneviewheight;
extern idCVar	pm_proneviewdistance;
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
extern idCVar	pm_skipBob;
extern idCVar	pm_thirdPersonRange;
extern idCVar	pm_thirdPersonHeight;
extern idCVar	pm_thirdPersonAngle;
extern idCVar	pm_thirdPersonOrbit;
extern idCVar	pm_thirdPersonNoPitch;
extern idCVar	pm_thirdPersonClip;
extern idCVar	pm_thirdPerson;
extern idCVar	pm_pausePhysics;
extern idCVar	pm_deployThirdPersonRange;
extern idCVar	pm_deployThirdPersonHeight;
extern idCVar	pm_deployThirdPersonAngle;
extern idCVar	pm_deathThirdPersonRange;
extern idCVar	pm_deathThirdPersonHeight;
extern idCVar	pm_deathThirdPersonAngle;

extern idCVar	pm_waterSpeed;

extern idCVar	pm_slidevelocity;
extern idCVar	pm_powerslide;

extern idCVar	g_showPlayerArrows;
extern idCVar	g_showPlayerClassIcon;

extern idCVar	g_showPlayerShadow;
extern idCVar	g_showHud;
extern idCVar	g_skipPostProcess;
extern idCVar	g_gun_x;
extern idCVar	g_gun_y;
extern idCVar	g_gun_z;
extern idCVar	g_fov;
extern idCVar	g_skipViewEffects;
extern idCVar	g_forceClear;

extern idCVar	g_testParticle;
extern idCVar	g_testParticleName;

extern idCVar	g_testPostProcess;
extern idCVar	g_testViewSkin;

extern idCVar	si_disableGlobalChat;
extern idCVar	g_disableGlobalAudio;
extern idCVar	g_maxPlayerWarnings;

extern idCVar	g_testModelRotate;
extern idCVar	g_testModelAnimate;
extern idCVar	g_testModelBlend;
extern idCVar	g_exportMask;

extern idCVar	g_gameReviewPause;

extern idCVar	net_useAOR;
extern idCVar	net_aorPVSScale;

extern idCVar	g_debugProficiency;
extern idCVar	g_debugNetworkWrite;
extern idCVar	g_weaponSwitchTimeout;

extern idCVar	g_hitBeep;

extern idCVar	pm_vehicleSoundLerpScale;

extern idCVar	ui_showGun;
extern idCVar	ui_ignoreExplosiveWeapons;
extern idCVar	ui_autoSwitchEmptyWeapons;
extern idCVar	ui_postArmFindBestWeapon;
extern idCVar	ui_voipReceiveGlobal;
extern idCVar	ui_voipReceiveTeam;
extern idCVar	ui_voipReceiveFireteam;

extern idCVar	net_clientShowSnapshot;
extern idCVar	net_clientShowSnapshotRadius;
extern idCVar	net_clientShowAOR;
extern idCVar	net_clientAORFilter;
extern idCVar	net_clientSelfSmoothing;
extern idCVar	net_clientLagOMeter;

extern idCVar	fs_debug;

extern idCVar	anim_showMissingAnims;

extern idCVar	aas_test;
extern idCVar	aas_showEdgeNums;
extern idCVar	aas_showAreas;
extern idCVar	aas_showAreaNumber;
extern idCVar	aas_showPath;
extern idCVar	aas_showHopPath;
extern idCVar	aas_showWallEdges;
extern idCVar	aas_showWallEdgeNums;
extern idCVar	aas_showNearestCoverArea;
extern idCVar	aas_showNearestInsideArea;
extern idCVar	aas_showTravelTime;
extern idCVar	aas_showPushIntoArea;
extern idCVar	aas_showFloorTrace;
extern idCVar	aas_showObstaclePVS;
extern idCVar	aas_showManualReachabilities;
extern idCVar	aas_showFuncObstacles;
extern idCVar	aas_showBadAreas;
extern idCVar	aas_locationMemory;
extern idCVar	aas_pullPlayer;
extern idCVar	aas_randomPullPlayer;

extern idCVar	bot_threading;
extern idCVar	bot_threadMinFrameDelay;
extern idCVar	bot_threadMaxFrameDelay;
extern idCVar	bot_pause;
extern idCVar	bot_drawClientNumbers;
extern idCVar	bot_drawActions;
extern idCVar	bot_drawBadIcarusActions;
extern idCVar	bot_drawIcarusActions;
extern idCVar	bot_drawActionNumber;
extern idCVar	bot_drawActionGroupNum;
extern idCVar	bot_drawActionTypeOnly;
extern idCVar	bot_drawActionRoutesOnly;
extern idCVar	bot_drawDefuseHints;
extern idCVar	bot_drawActiveActionsOnly;
extern idCVar	bot_drawActionVehicleType;
extern idCVar	bot_drawActionWithClasses;
extern idCVar	bot_drawRoutes;
extern idCVar	bot_drawRouteGroupOnly;
extern idCVar	bot_drawActiveRoutesOnly;
extern idCVar	bot_drawNodes;
extern idCVar	bot_drawNodeNumber;
extern idCVar	bot_drawActionSize;
extern idCVar	bot_drawActionDist;
extern idCVar	bot_drawObstacles;
extern idCVar	bot_drawRearSpawnLocations;
extern idCVar	bot_doObjectives;
extern idCVar	bot_useShotguns;
extern idCVar	bot_useSniperWeapons;
extern idCVar	bot_useVehicles;
extern idCVar	bot_allowObstacleDecay;
extern idCVar	bot_godMode;
extern idCVar	bot_stayInVehicles;
extern idCVar	bot_ignoreGoals;
extern idCVar	bot_useStrafeJump;
extern idCVar	bot_useSuicideWhenStuck;
extern idCVar   bot_useSpawnHosts;
extern idCVar	bot_useUniforms;
extern idCVar	bot_useDeployables;
extern idCVar	bot_useMines;
extern idCVar	bot_testPathToBotAction;
extern idCVar	bot_sillyWarmup;
extern idCVar	bot_noChat;
extern idCVar	bot_noTaunt;
extern idCVar	bot_enable;
extern idCVar   bot_aimSkill;
extern idCVar	bot_skill;
extern idCVar	bot_knifeOnly;
extern idCVar	bot_ignoreEnemies;
extern idCVar	bot_debug;
extern idCVar	bot_debugSpeed;
extern idCVar	bot_debugGroundVehicles;
extern idCVar	bot_debugActionGoalNumber;
extern idCVar	bot_debugAirVehicles;
extern idCVar	bot_debugObstacles;
extern idCVar	bot_spectateDebug;
extern idCVar	bot_followMe;
extern idCVar	bot_breakPoint;
extern idCVar	bot_minClients;
extern idCVar	bot_minClientsMax;
extern idCVar	bot_debugPersonalVehicles;
extern idCVar	bot_debugWeapons;
extern idCVar	bot_uiNumStrogg;
extern idCVar	bot_uiNumGDF;
extern idCVar	bot_uiSkill;
extern idCVar	bot_showPath;
extern idCVar	bot_skipThinkClient;
extern idCVar	bot_debugMapScript;
extern idCVar	bot_useTKRevive;
extern idCVar	bot_debugObstacleAvoidance;
extern idCVar	bot_testObstacleQuery;
extern idCVar	bot_balanceCriticalClass;
extern idCVar	bot_useAltRoutes;
extern idCVar	bot_useRearSpawn;
extern idCVar	bot_sleepWhenServerEmpty;
extern idCVar	bot_allowClassChanges;
extern idCVar	bot_pauseInVehicleTime;
extern idCVar	bot_doObjsInTrainingMode;
extern idCVar	bot_doObjsDelayTimeInMins;
extern idCVar	bot_useAirVehicles;

extern idCVar	g_showCrosshairInfo;
extern idCVar	g_bannerDelay;
extern idCVar	g_bannerLoopDelay;

const int NUM_BANNER_MESSAGES = 16;
extern idCVar*	g_bannerCvars[ NUM_BANNER_MESSAGES ];

extern idCVar	g_complaintLimit;
extern idCVar	g_complaintGUIDLimit;

extern idCVar	g_execMapConfigs;

extern idCVar	g_teamSwitchDelay;
extern idCVar	g_muteSpecs;
extern idCVar	g_warmupDamage;
extern idCVar	g_warmup;
extern idCVar	si_gameReviewReadyWait;
extern idCVar	g_privatePassword;
extern idCVar	g_password;
extern idCVar	g_xpSave;
extern idCVar	g_kickBanLength;
extern idCVar	g_maxSpectateTime;


extern idCVar	g_voteWait;


extern idCVar	g_radialMenuMouseInput;
extern idCVar	g_radialMenuStyle;

extern idCVar	g_unlock_updateViewpos;
extern idCVar	g_unlock_updateAngles;
extern idCVar	g_unlock_interpolateMoving; 
extern idCVar	g_unlock_viewStyle;

extern idCVar	gui_storeTextInfo;

extern idCVar	g_useTraceCollection;

extern idCVar	g_removeStaticEntities;

extern idCVar	g_maxVoiceChats;
extern idCVar	g_maxVoiceChatsOver;

extern idCVar	g_profileEntityThink;
extern idCVar	g_timeoutToSpec;

extern idCVar	g_autoReadyPercent;
extern idCVar	g_autoReadyWait;

extern idCVar	g_useBotsInPlayerTotal;

extern idCVar	g_playTooltipSound;
extern idCVar	g_tooltipTimeScale;
extern idCVar	g_tooltipVolumeScale;
extern idCVar	g_allowSpecPauseFreeFly;
extern idCVar	g_smartTeamBalance;
extern idCVar	g_smartTeamBalanceReward;

extern idCVar	g_keepFireTeamList;

extern idCVar	net_serverDownload;
extern idCVar	net_serverDlBaseURL;
extern idCVar	net_serverDlTable;

#ifdef SD_SUPPORT_REPEATER

extern idCVar	ri_useViewerPass;
extern idCVar	g_viewerPassword;

extern idCVar	ri_privateViewers;
extern idCVar	g_privateViewerPassword;
extern idCVar	g_repeaterPassword;

extern idCVar	ri_name;

extern idCVar	g_noTVChat;

#endif // SD_SUPPORT_REPEATER

extern idCVar	g_trainingMode;

extern idCVar	g_noQuickChats;

#endif /* !__SYS_CVAR_H__ */
