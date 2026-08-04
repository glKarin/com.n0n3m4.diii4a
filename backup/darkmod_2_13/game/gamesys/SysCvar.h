/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __SYS_CVAR_H__
#define __SYS_CVAR_H__

/**
* DarkMod cvars - See text description in syscvar.cpp for descriptions
**/
extern idCVar cv_player_spawnclass;
extern idCVar cv_player_waituntilready;

extern idCVar cv_default_mission_info_file;

extern idCVar cv_ai_sndvol;
extern idCVar cv_ai_bark_show;
extern idCVar cv_ai_name_show; // grayman #3857
extern idCVar cv_ai_bumpobject_impulse;
extern idCVar cv_ai_sight_prob;
extern idCVar cv_ai_sight_mag;
extern idCVar cv_ai_sightmaxdist;
extern idCVar cv_ai_sightmindist;
extern idCVar cv_ai_sight_combat_cutoff; // grayman #3063
extern idCVar cv_ai_tactalert;
extern idCVar cv_ai_task_show;
extern idCVar cv_ai_alertlevel_show;
extern idCVar cv_ai_dest_show;
extern idCVar cv_ai_goalpos_show;
extern idCVar cv_ai_aasarea_show;
extern idCVar cv_ai_debug_blocked;
extern idCVar cv_ai_door_show;
extern idCVar cv_ai_elevator_show;
extern idCVar cv_ai_debug;
extern idCVar cv_ai_sight_thresh;
extern idCVar cv_ai_sight_scale;
extern idCVar cv_ai_show_enemy_visibility;
extern idCVar cv_ai_show_conversationstate;
extern idCVar cv_ai_acuity_L3;
extern idCVar cv_ai_acuity_L4;
extern idCVar cv_ai_acuity_L5;
//extern idCVar cv_ai_acuity_susp;
extern idCVar cv_ai_visdist_show;
extern idCVar cv_ai_opt_disable;
extern idCVar cv_ai_opt_noanims;
extern idCVar cv_ai_opt_novisualscan;
extern idCVar cv_ai_opt_forcedormant;
extern idCVar cv_ai_opt_forceopt;
extern idCVar cv_ai_opt_nothink;
extern idCVar cv_ai_opt_interleavethinkmindist;
extern idCVar cv_ai_opt_interleavethinkmaxdist;
extern idCVar cv_ai_opt_interleavethinkskippvscheck;
extern idCVar cv_ai_opt_interleavethinkframes;
extern idCVar cv_ai_opt_update_enemypos_interleave;
extern idCVar cv_ai_opt_nomind;
extern idCVar cv_ai_opt_novisualstim;
extern idCVar cv_ai_opt_nolipsync;
extern idCVar cv_ai_opt_nopresent;
extern idCVar cv_ai_opt_noobstacleavoidance;
extern idCVar cv_ai_hiding_spot_max_light_quotient;
extern idCVar cv_ai_max_hiding_spot_tests_per_frame;
extern idCVar cv_ai_debug_anims;

extern idCVar cv_show_health;

extern idCVar cv_ai_show_aasfuncobstacle_state;

extern idCVar r_customWidth;
extern idCVar r_customHeight;
extern idCVar r_aspectRatio;
extern idCVar cv_tdm_widescreenmode;
extern idCVar cv_tdm_menu_music;

extern idCVar cv_tdm_show_trainer_messages;

extern idCVar cv_tdm_default_relations_def;
extern idCVar cv_tdm_fm_path;
extern idCVar cv_tdm_fm_desc_file;
extern idCVar cv_tdm_fm_current_file;
extern idCVar cv_tdm_fm_notes_file;
extern idCVar cv_tdm_fm_startingmap_file;
extern idCVar cv_tdm_fm_mapsequence_file;
extern idCVar cv_tdm_fm_splashimage_file;
extern idCVar cv_tdm_fm_restart_delay;

extern idCVar cv_tdm_proxy;
extern idCVar cv_tdm_proxy_user;
extern idCVar cv_tdm_proxy_pass;
extern idCVar cv_tdm_allow_http_access;
extern idCVar cv_tdm_mission_list_urls;
extern idCVar cv_tdm_mission_details_url;
extern idCVar cv_tdm_mission_screenshot_url;
extern idCVar cv_tdm_version_check_url;
extern idCVar cv_tdm_http_base_url;

extern idCVar cv_debug_aastype;

extern idCVar cv_las_showtraces;
extern idCVar cv_show_gameplay_time;

extern idCVar cv_tdm_difficulty;

extern idCVar cv_sr_disable;
extern idCVar cv_sr_show;

extern idCVar cv_sndprop_disable;
extern idCVar cv_spr_debug;
extern idCVar cv_spr_show;
extern idCVar cv_spr_radius_show;
extern idCVar cv_ko_show;
extern idCVar cv_ai_animstate_show;

extern idCVar cv_debug_mainmenu;
extern idCVar cv_mainmenu_confirmquit;

extern idCVar cv_pm_runmod;
extern idCVar cv_pm_run_backmod;
extern idCVar cv_pm_crouchmod;
extern idCVar cv_pm_max_swimspeed_mod;
extern idCVar cv_pm_swimspeed_variation;
extern idCVar cv_pm_swimspeed_frequency;
extern idCVar cv_pm_creepmod;
extern idCVar cv_pm_running_creepmod;
extern idCVar cv_pm_pushmod;
extern idCVar cv_pm_push_maximpulse;
extern idCVar cv_pm_push_start_delay;
extern idCVar cv_pm_push_accel_time;
extern idCVar cv_pm_push_heavy_threshold;
extern idCVar cv_pm_push_max_mass;

// STiFU #1932: Soft-hinderances: Effectiveness of hinderances on certain walk speed modifiers
extern idCVar cv_pm_softhinderance_active;
extern idCVar cv_pm_softhinderance_creep;
extern idCVar cv_pm_softhinderance_walk;
extern idCVar cv_pm_softhinderance_run;

/**
* TDM CVARs for controlling jumping
*/
extern idCVar cv_tdm_walk_jump_vel;
extern idCVar cv_tdm_run_jump_vel;
extern idCVar cv_tdm_crouch_jump_vel;
extern idCVar cv_tdm_min_vel_jump;
extern idCVar cv_tdm_fwd_jump_vel;
extern idCVar cv_tdm_backwards_jump_modifier;
extern idCVar cv_tdm_jump_relaxation_time;

extern idCVar cv_tdm_footfalls_movetype_specific;

extern idCVar cv_pm_weightmod;

extern idCVar cv_pm_mantle_height;
extern idCVar cv_pm_mantle_reach;
extern idCVar cv_pm_mantle_minflatness;
extern idCVar cv_pm_mantle_jump_hold_trigger;
extern idCVar cv_pm_mantle_min_velocity_for_damage;
extern idCVar cv_pm_mantle_damage_per_velocity_over_minimum;
extern idCVar cv_pm_mantle_hang_msecs;
extern idCVar cv_pm_mantle_pull_msecs;
extern idCVar cv_pm_mantle_shift_hands_msecs;
extern idCVar cv_pm_mantle_push_msecs;
extern idCVar cv_pm_mantle_pushNonCrouched_msecs;
extern idCVar cv_pm_mantle_fastLowObstaces;
extern idCVar cv_pm_mantle_maxLowObstacleHeight;

// Daft Mugi #5892: Mantle while carrying a body
extern idCVar cv_pm_mantle_maxHoldingMidairObstacleHeight;
extern idCVar cv_pm_mantle_maxShoulderingObstacleHeight;
extern idCVar cv_pm_mantle_while_carrying;

extern idCVar cv_pm_mantle_fastMediumObstaclesCrouched;
extern idCVar cv_pm_mantle_pullFast_msecs;
extern idCVar cv_pm_mantle_pushNonCrouched_playgrunt_speedthreshold;
extern idCVar cv_pm_mantle_fallingFast_speedthreshold;
extern idCVar cv_pm_mantle_cancel_speed;
extern idCVar cv_pm_mantle_roll_mod;

extern idCVar cv_pm_ladderSlide_speedLimit;

extern idCVar cv_pm_rope_snd_rep_dist;
extern idCVar cv_pm_rope_velocity_letgo;
extern idCVar cv_pm_rope_swing_impulse;
extern idCVar cv_pm_rope_swing_duration;
extern idCVar cv_pm_rope_swing_reptime;
extern idCVar cv_pm_rope_swing_kickdist;
extern idCVar cv_pm_water_downwards_velocity;
extern idCVar cv_pm_water_z_friction;
extern idCVar cv_pm_show_waterlevel;
extern idCVar cv_pm_climb_distance;

extern idCVar cv_pm_blackjack_indicate; // Obsttorte (#4289)

/**
* This cvar controls if ai hiding spot search debug graphics are drawn
* If it is 0, then the graphics are not drawn.  If it is >= 1.0 then it
* is the number of milliseconds for which each graphic should persist.
* For example 3000.0 would mean 3 seconds
*/
extern idCVar cv_ai_search_show;

extern idCVar cv_ai_search_type; // grayman #4220 - control type of search

extern idCVar cv_force_savegame_load;
extern idCVar cv_savegame_compress;

// Daft Mugi #6257: Auto-search bodies
extern idCVar cv_tdm_autosearch_bodies;

// angua: TDM toggle crouch
extern idCVar cv_tdm_crouch_toggle;
extern idCVar cv_tdm_crouch_toggle_hold_time;
extern idCVar cv_tdm_reattach_delay;

// nbohr1more: #558 TDM toggle creep
extern idCVar cv_tdm_toggle_creep;

// #6232: Player choice of sheathe key behavior
extern idCVar cv_tdm_toggle_sheathe;

// stifu #3607: Shouldering animation
extern idCVar cv_pm_shoulderAnim_msecs;
extern idCVar cv_pm_shoulderAnim_dip_duration;
extern idCVar cv_pm_shoulderAnim_rockDist;
extern idCVar cv_pm_shoulderAnim_dip_dist;
extern idCVar cv_pm_shoulderAnim_delay_msecs;

// stifu #4107: Try multiple drop positions for shouldered bodies
extern idCVar cv_pm_shoulderDrop_maxAngle;
extern idCVar cv_pm_shoulderDrop_angleIncrement;

/**
* TDM Leaning vars:
**/
extern idCVar cv_pm_lean_to_valid_increments;
extern idCVar cv_pm_lean_door_increments;
extern idCVar cv_pm_lean_door_max;
extern idCVar cv_pm_lean_door_bounds_exp;
extern idCVar cv_tdm_toggle_lean;

// Daft Mugi #6320: Add New Lean
extern idCVar cv_pm_lean_time_to_lean;
extern idCVar cv_pm_lean_time_to_unlean;
extern idCVar cv_pm_lean_slide;
extern idCVar cv_pm_lean_angle;
extern idCVar cv_pm_lean_angle_mod;

extern idCVar cv_frob_distance_default;
extern idCVar cv_frob_width;
extern idCVar cv_frob_debug_bounds;
extern idCVar cv_frob_fadetime;
extern idCVar cv_frob_weapon_selects_weapon;
extern idCVar cv_frob_debug_hud;

extern idCVar cv_frobhelper_active;
extern idCVar cv_frobhelper_alwaysVisible;
extern idCVar cv_frobhelper_alpha;
extern idCVar cv_frobhelper_fadein_delay;
extern idCVar cv_frobhelper_fadein_duration;
extern idCVar cv_frobhelper_fadeout_duration;
extern idCVar cv_frobhelper_ignore_size;

// Daft Mugi #6316: Hold Frob for alternate interaction
extern idCVar cv_holdfrob_delay;
extern idCVar cv_holdfrob_bounds;
extern idCVar cv_holdfrob_drag_body_behavior;

extern idCVar cv_holdfrob_drag_all_entities;

//Obsttorte: #5984 (multilooting)
extern idCVar cv_multiloot_min_interval;
extern idCVar cv_multiloot_max_interval;

extern idCVar cv_weapon_next_on_empty;

// physics
extern idCVar cv_collision_damage_scale_vert;
extern idCVar cv_collision_damage_scale_horiz;
extern idCVar cv_dragged_item_highlight;
extern idCVar cv_drag_jump_masslimit;
extern idCVar cv_drag_encumber_minmass;
extern idCVar cv_drag_encumber_maxmass;
extern idCVar cv_drag_encumber_max;
extern idCVar cv_drag_stuck_dist;
extern idCVar cv_drag_force_max;
extern idCVar cv_drag_new;
extern idCVar cv_drag_limit_force;
extern idCVar cv_drag_damping;
extern idCVar cv_drag_damping_AF;
extern idCVar cv_drag_AF_ground_timer;
extern idCVar cv_drag_AF_free;
extern idCVar cv_drag_debug;
extern idCVar cv_drag_targetpos_averaging_time;
extern idCVar cv_drag_rigid_silentmode;
extern idCVar cv_drag_rigid_distance_halfing_time;
extern idCVar cv_drag_rigid_acceleration_radius;
extern idCVar cv_drag_rigid_angle_halfing_time;
extern idCVar cv_drag_rigid_acceleration_angle;
extern idCVar cv_drag_af_weight_ratio;
extern idCVar cv_drag_af_weight_ratio_canlift;
extern idCVar cv_drag_af_reduceforce_radius;
extern idCVar cv_drag_af_inair_friction;

extern idCVar cv_melee_debug;
extern idCVar cv_melee_state_debug;
extern idCVar cv_melee_mouse_thresh;
extern idCVar cv_melee_mouse_decision_time;
extern idCVar cv_melee_mouse_dead_time;
extern idCVar cv_melee_mouse_slowview;
extern idCVar cv_melee_invert_attack;
extern idCVar cv_melee_invert_parry;
extern idCVar cv_melee_auto_parry;
extern idCVar cv_melee_forbid_auto_parry;
extern idCVar cv_melee_max_particles;
extern idCVar cv_phys_show_momentum;

extern idCVar cv_throw_impulse_min;
extern idCVar cv_throw_impulse_max;
extern idCVar cv_throw_vellimit_min;
extern idCVar cv_throw_vellimit_max;
extern idCVar cv_throw_time;

extern idCVar cv_bounce_sound_max_vel;
extern idCVar cv_bounce_sound_min_vel;

extern idCVar cv_reverse_grab_control;

extern idCVar cv_tdm_rope_pull_force_factor;

extern idCVar cv_tdm_obj_gui_file;
extern idCVar cv_tdm_waituntilready_gui_file;
extern idCVar cv_tdm_invgrid_gui_file;	// #4286
extern idCVar cv_tdm_invgrid_sortstyle; // #6592
extern idCVar cv_tdm_subtitles_gui_file;

extern idCVar cv_tdm_hud_opacity;
extern idCVar cv_tdm_hud_hide_lightgem;
extern idCVar cv_tdm_underwater_blur;

//Obsttorte: cvars to allow altering the gui size
extern idCVar cv_gui_iconSize;		//inventory and weapon icons
extern idCVar cv_gui_smallTextSize;	//weapon and item names and counts
extern idCVar cv_gui_bigTextSize;		//game saved, new objective, pickup messages
extern idCVar cv_gui_lightgemSize;	//lightgem and crouch indicator
extern idCVar cv_gui_barSize;			//breath and health bar
extern idCVar cv_gui_objectiveTextSize; 

// Daft Mugi #6331: Show viewpos on player HUD
extern idCVar cv_show_viewpos;

extern idCVar cv_tdm_inv_loot_item_def;
extern idCVar cv_tdm_inv_gui_file;
extern idCVar cv_tdm_inv_hud_pickupmessages;
extern idCVar cv_tdm_inv_loot_sound;
extern idCVar cv_tdm_inv_use_on_frob;
extern idCVar cv_tdm_inv_use_visual_feedback;
extern idCVar cv_tdm_subtitles;

extern idCVar cv_tdm_door_control;
extern idCVar cv_tdm_door_control_sensitivity;

extern idCVar cv_pm_stepvol_walk;
extern idCVar cv_pm_stepvol_run;
extern idCVar cv_pm_stepvol_creep;
extern idCVar cv_pm_stepvol_crouch_walk;
extern idCVar cv_pm_stepvol_crouch_creep;
extern idCVar cv_pm_stepvol_crouch_run;
extern idCVar cv_pm_min_stepsound_interval;

// Lightgem
extern idCVar cv_lg_distance;
extern idCVar cv_lg_xoffs;
extern idCVar cv_lg_yoffs;
extern idCVar cv_lg_zoffs;
extern idCVar cv_lg_oxoffs;
extern idCVar cv_lg_oyoffs;
extern idCVar cv_lg_ozoffs;
extern idCVar cv_lg_fov;
extern idCVar cv_lg_interleave;
// nbohr1more #4369 Dynamic Lightgem Interleave
extern idCVar cv_lg_interleave_min;
extern idCVar cv_lg_weak;
extern idCVar cv_lg_model;
extern idCVar cv_lg_adjust;
extern idCVar cv_lg_crouch_modifier;
extern idCVar cv_lg_image_width;
extern idCVar cv_lg_screen_width;
extern idCVar cv_lg_screen_height;
extern idCVar cv_lg_velocity_mod_min_velocity;
extern idCVar cv_lg_velocity_mod_max_velocity;
extern idCVar cv_lg_velocity_mod_amount;
extern idCVar g_lightQuotientAlgo;

extern idCVar cv_lg_fade_delay;						// Added by  J.C.Denton

extern idCVar cv_empty_model;

// Lockpicking
extern idCVar cv_lp_pin_base_count;
extern idCVar cv_lp_sample_delay;
extern idCVar cv_lp_pick_timeout;
extern idCVar cv_lp_max_pick_attempts;
extern idCVar cv_lp_auto_pick;
extern idCVar cv_lp_randomize;
extern idCVar cv_lp_pawlow;
extern idCVar cv_lp_debug_hud;

// Bow type
extern idCVar cv_bow_aimer;
// melee difficulty
extern idCVar cv_melee_difficulty;

// grayman #3492 - AI Vision
extern idCVar cv_ai_vision;
extern idCVar cv_ai_vision_nearly_blind;
extern idCVar cv_ai_vision_forgiving;
extern idCVar cv_ai_vision_challenging;
extern idCVar cv_ai_vision_hardcore;

// grayman #3682 - AI Hearing
extern idCVar cv_ai_hearing;
extern idCVar cv_ai_hearing_nearly_deaf;
extern idCVar cv_ai_hearing_forgiving;
extern idCVar cv_ai_hearing_challenging;
extern idCVar cv_ai_hearing_hardcore;

extern idCVar cv_door_auto_open_on_unlock;
extern idCVar cv_door_ignore_locks;

extern idCVar cv_dm_distance;

// Volume of music speakers
extern idCVar cv_music_volume;

// Tels: Volume of the "player voice" speaker
extern idCVar cv_voice_player_volume;
// Tels: Volume of the "speaker-from-off voice" speaker
extern idCVar cv_voice_from_off_volume;

// angua: Velocity and sound volume of collisions
extern idCVar cv_moveable_collision;

// Tels: LOD system: multiplier for the LOD distance to be used
extern idCVar cv_lod_bias;

// grayman: for debugging 'evidence' barks and greetings
extern idCVar cv_ai_debug_transition_barks;
extern idCVar cv_ai_debug_greetings;

/**
* CVars added for Darkmod knockout and field of vision changes
*/
extern idCVar cv_ai_fov_show;
extern idCVar cv_ai_ko_show;

/** Screen width * gui_Width = GUI width */
extern idCVar	cv_gui_Width;
/** Screen height * gui_Height = GUI height */
extern idCVar	cv_gui_Height;
/** Screen width * gui_CenterX = GUI center X */
extern idCVar	cv_gui_CenterX;
/** Screen height * gui_CenterY = GUI center Y */
extern idCVar	cv_gui_CenterY;

/**
* End DarkMod cvars
**/

extern idCVar	com_developer;

extern idCVar	g_cinematic;
extern idCVar	g_cinematicMaxSkipTime;

extern idCVar	g_monsters;
extern idCVar	g_decals;
extern idCVar	g_knockback;
extern idCVar	g_gravity;
extern idCVar	g_skipFX;
//extern idCVar	g_skipParticles;
extern idCVar	g_bloodEffects;
extern idCVar	g_projectileLights;
extern idCVar	g_doubleVision;
extern idCVar	g_muzzleFlash;

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
extern idCVar	g_damageScale;

extern idCVar	g_showPVS;
extern idCVar	g_showTargets;
extern idCVar	g_showTriggers;
extern idCVar	g_showCollisionWorld;
extern idCVar	g_showCollisionModels;
extern idCVar	g_showCollisionTraces;
extern idCVar	g_showCollisionAlongView;
extern idCVar	g_maxShowDistance;
extern idCVar	g_showEntityInfo;
extern idCVar	g_showviewpos;
extern idCVar	g_showcamerainfo;
extern idCVar	g_showTestModelFrame;
extern idCVar	g_showActiveEntities;
extern idCVar	g_showEnemies;
extern idCVar	g_showLightQuotient;

extern idCVar	g_frametime;
extern idCVar	g_timeentities;

extern idCVar	g_timeModifier;

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

extern idCVar	g_enablePortalSky;



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
//extern idCVar	pm_crouchspeed;
extern idCVar	pm_walkspeed;
//extern idCVar	pm_runspeed;
extern idCVar	pm_noclipspeed;
extern idCVar	pm_spectatespeed;
extern idCVar	pm_spectatebbox;
extern idCVar	pm_usecylinder;
extern idCVar	pm_minviewpitch;
extern idCVar	pm_maxviewpitch;
// Commented out by Dram. Not needed as TDM does not use stamina
//extern idCVar	pm_stamina;
//extern idCVar	pm_staminathreshold;
//extern idCVar	pm_staminarate;
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

// Daft Mugi #6310: Add simplified head bob cvar
extern idCVar	pm_headbob_mod;

extern idCVar	pm_thirdPersonRange;
extern idCVar	pm_thirdPersonHeight;
extern idCVar	pm_thirdPersonAngle;
extern idCVar	pm_thirdPersonClip;
extern idCVar	pm_thirdPerson;
extern idCVar	pm_thirdPersonDeath;
extern idCVar	pm_modelView;
extern idCVar	pm_airTics;
extern idCVar	pm_airTicsRegainingSpeed;

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
extern idCVar	g_rotoscope;

extern idCVar	g_testModelHead;
extern idCVar	g_testModelHeadJoint;
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

extern idCVar	net_clientLagOMeter;

extern const char *si_gameTypeArgs[];

extern const char *ui_skinArgs[];


#ifdef MOD_WATERPHYSICS

extern idCVar af_useBodyDensityBuoyancy;			// MOD_WATERPHYSICS

extern idCVar af_useFixedDensityBuoyancy;			// MOD_WATERPHYSICS

extern idCVar rb_showBuoyancy;								// MOD_WATERPHYSICS

#endif

#endif /* !__SYS_CVAR_H__ */
