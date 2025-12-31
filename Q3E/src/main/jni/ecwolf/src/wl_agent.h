#ifndef __WL_AGENT__
#define __WL_AGENT__

#include "wl_def.h"
#include "a_playerpawn.h"
#include "id_vl.h"
#include "tmemory.h"
#include "weaponslots.h"

/*
=============================================================================

							WL_AGENT DEFINITIONS

=============================================================================
*/

extern  AActor   *LastAttacker;

void    CheckSpawnPlayer (bool setup=false);
void    SpawnPlayer (int num);

//
// Status bar interface
//
class DBaseStatusBar
{
public:
	virtual ~DBaseStatusBar() {}

	virtual void DrawStatusBar()=0;
	virtual unsigned int GetHeight(bool top)=0;
	virtual void NewGame() {}
	virtual void Tick() {}
	virtual void RefreshBackground(bool noborder=false);
	virtual void UpdateFace (int damage=0) {}
	virtual void WeaponGrin () {}
};
extern DBaseStatusBar *StatusBar;
void	CreateStatusBar();

//
// player state info
//

void    CheckWeaponChange (AActor *self);
void    ControlMovement (class APlayerPawn *self);

////////////////////////////////////////////////////////////////////////////////

class AWeapon;

#define WP_NOCHANGE ((AWeapon*)~0)

extern class player_t
{
	public:
		enum PSprite
		{
			ps_weapon,
			ps_flash,

			NUM_PSPRITES
		};

		player_t();

		void	BobWeapon(fixed_t *x, fixed_t *y);
		void	BringUpWeapon();
		void	DeathFade();
		void	DeathFadeClear();
		AActor	*FindTarget();
		inline unsigned int GetPlayerNum() const;
		void	GiveExtraMan(int amount);
		void	GivePoints(int32_t points);
		size_t	PropagateMark();
		void	Reborn();
		void	Serialize(FArchive &arc);
		void	SetPSprite(const Frame *frame, PSprite layer);
		void	SetFOV(float newlyDesiredFOV);
		void	AdjustFOV();
		void	TakeDamage(int points, AActor *attacker);

		enum State
		{
			PST_ENTER,
			PST_DEAD,
			PST_REBORN,
			PST_LIVE
		};

		enum PlayerFlags
		{
			PF_WEAPONREADY = 0x1,
			PF_WEAPONBOBBING = 0x2,
			PF_WEAPONREADYALT = 0x4,
			PF_WEAPONSWITCHOK = 0x8,
			PF_DISABLESWITCH = 0x10,
			PF_WEAPONRELOADOK = 0x20,
			PF_WEAPONZOOMOK = 0x40,
			PF_REFIRESWITCHOK = 0x80,

			PF_READYFLAGS = PF_WEAPONREADY|PF_WEAPONBOBBING|PF_WEAPONREADYALT|PF_WEAPONSWITCHOK|PF_WEAPONRELOADOK|PF_WEAPONZOOMOK
		};

		APlayerPawn	*mo;
		TObjPtr<AActor>	camera;
		TObjPtr<AActor>	killerobj;

		TUniquePtr<FFader> ScreenFader;

		int32_t		oldscore,score,nextextra;
		int32_t		health;
		int32_t		frags;
		short		lives;
		float		FOV, DesiredFOV;

		int32_t		thrustspeed;

		FWeaponSlots	weapons;
		AWeapon			*ReadyWeapon;
		AWeapon			*PendingWeapon;
		struct
		{
			const Frame	*frame;
			short		ticcount;
			fixed		sx;
			fixed		sy;
		} psprite[NUM_PSPRITES];
		fixed			bob;

		// Attackheld is similar to buttonheld[bt_attack] only it only gets set
		// to true when an attack is registered. If the button is released and
		// then pressed again it should initiate a second attack even if
		// NOAUTOFIRE is set.
		bool		attackheld;
		short		extralight;

		int32_t		RespawnEligible;
		int32_t		flags;
		State		state;
} players[];

inline unsigned int player_t::GetPlayerNum() const
{
	return static_cast<unsigned int>(this - players);
}

FArchive &operator<< (FArchive &arc, player_t *&player);

#endif
