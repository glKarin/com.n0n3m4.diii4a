////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: TF2 Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF2_MESSAGES_H__
#define __TF2_MESSAGES_H__

#include "TF_Messages.h"

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct Event_Death_TF2
{
	GameEntity	m_WhoKilledMe;
	char		m_MeansOfDeath[32];
	int			m_Dominated;
	int			m_Dominated_Assister;
	int			m_Revenge;
	int			m_Revenge_Assister;
};

struct Event_KilledSomeone_TF2
{
	GameEntity	m_WhoIKilled;
	char		m_MeansOfDeath[32];
	int			m_Dominated;
	int			m_Dominated_Assister;
	int			m_Revenge;
	int			m_Revenge_Assister;
};

struct Event_SetLoadOut_TF2
{
	struct tScout
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tScout() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Scout;
	struct tSniper
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tSniper() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Sniper;
	struct tSoldier
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tSoldier() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Soldier;
	struct tDemoman 
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tDemoman() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Demoman;
	struct tMedic
	{
		obBool		Blutsauger;		// syringe alt, steals health, no crit
		obBool		Kritzkrieg;		// medigun alt, target  8 seconds crit on ubercharge, 25% faster uber
		obBool		Ubersaw;		// bonesaw alt, hits charge 2nd weapon 25%
		tMedic() : Blutsauger(Invalid), Kritzkrieg(Invalid), Ubersaw(Invalid) {}
	} Medic;
	struct tHwGuy
	{
		obBool		Natascha;		// minigun alt, slow enemy on hit
		obBool		Sandvich;		// shotgun alt, 120hp heal over 4 seconds
		obBool		KGB;			// fists alt, 5 seconds crit after each kill
		tHwGuy() : Natascha(Invalid), Sandvich(Invalid), KGB(Invalid) {}
	} HwGuy;
	struct tPyro
	{
		obBool		Backburner;		// flamethrower alt, crits when attacking from behind
		obBool		FlareGun;		// shotgun alt, ignites enemies on hit
		obBool		Axtinguisher;	// fire axe alt, 100% crit when attacking burning enemy
		tPyro() : Backburner(Invalid), FlareGun(Invalid), Axtinguisher(Invalid) {}
	} Pyro;
	struct tEngineer
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tEngineer() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Engineer;
	struct tSpy
	{
		obBool		Unlock1;
		obBool		Unlock2;
		obBool		Unlock3;
		tSpy() : Unlock1(Invalid), Unlock2(Invalid), Unlock3(Invalid) {}
	} Spy;
};

//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#endif
