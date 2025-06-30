/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

struct skilldata_t
{
	int iSkillLevel;
	float agruntHealth;
	float agruntDmgPunch;
	float apacheHealth;
	float barneyHealth;
	float bigmommaHealthFactor;
	float bigmommaDmgSlash;
	float bigmommaDmgBlast;
	float bigmommaRadiusBlast;
	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;
	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;
	float hassassinHealth;
	float headcrabHealth;
	float headcrabDmgBite;
	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;
	float houndeyeHealth;
	float houndeyeDmgBlast;
	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;
	float ichthyosaurHealth;
	float ichthyosaurDmgShake;
	float leechHealth;
	float leechDmgBite;
	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;
	float nihilanthHealth;
	float nihilanthZap;
	float scientistHealth;
	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;
	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;
	float turretHealth;
	float miniturretHealth;
	float sentryHealth;
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

extern DLL_GLOBAL skilldata_t gSkillData;
float GetSkillCvar(char *pName);
extern DLL_GLOBAL int g_iSkillLevel;

#define SKILL_EASY 1
#define SKILL_MEDIUM 2
#define SKILL_HARD 3