// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOT_H__
#define __BOT_H__

#include "BotAI_Main.h"

/*
===============================================================================

   idBot

   This is the in-game entity for a bot. This entity takes care of moving
   the bot through the world while the actual AI is implemented in idBotAI
   which runs in a separate thread.

  - QW:ET Bot. Begun July 5, 2006 
  - bot born in game (spawning in - but buggy, crashes) July 7, 2006
  - made thread safe (for the Xbox) August 8, 2006.
  - bot codename: Strogg Bot August 14, 2006.
  - medic class finshed: August 23, 2006.
===============================================================================
*/

#define MAX_REAR_SPAWN_NUMBER	2

#define BOT_MOVE_LOOKUP
#define BOT_MOVE_LOOKUP_STEPS	720
#define BOT_MOVE_LOOKUP_SCALE	( BOT_MOVE_LOOKUP_STEPS / 360.0f )

#define NEXT_VEHICLE_CMD_TIME	1000

enum projectileTypes_t {
	PROJECTILE_TANK_ROUND,
	PROJECTILE_ROCKET,
	PROJECTILE_GRENADE
};

class idBot : public idPlayer {
public:
	friend class sdAdminSystemCommand_AddBot;

	CLASS_PROTOTYPE( idBot );

							idBot();
	virtual					~idBot();

	virtual void			Think();
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );

	const idAngles &		GetHackMoveAngles() { return botHackMoveAngles; }

	static void				Cmd_AASStats_f( const idCmdArgs &args );

	static void				BuildMoveLookups( float runfwd, float runside, float runback, float sprintfwd, float sprintside );

	void					Bot_InputToUserCmd(); //mal: sends the bots current user cmds to the server, for client prediction.

private:
	static void				Bot_SetClassType( idPlayer* player, int classType, int classWeapon ); //mal: utility function that lets me set bot's class.
	void					Bot_SetWeapon();
	void					Bot_ResetGameState();
	int						FixUcmd( float ucmd );
	void					Bot_ChangeViewAngles( const idAngles &idealAngle, bool fast );
	void					Bot_ClientAimAtEnemy( int clientNum );
	void					Bot_VehicleAimAtEnemy( int clientNum );
	bool					InBotsSights( const idVec3 &origin );
	void					Bot_ResetUcmd(usercmd_t &ucmd );
	void					DeadUcmds( usercmd_t &ucmd );
	void					AngleUcmds( usercmd_t &ucmd );
	void					MoveUcmds( usercmd_t &ucmd );
	void					ActionUcmds( usercmd_t &ucmd );
	void					VehicleUcmds( usercmd_t &ucmd );
	void					GroundVehicleControls( usercmd_t &ucmd );
	void					GetVehicleInfo( int entNum, proxyInfo_t& vehicleInfo );
	void					Bot_ChangeVehicleViewAngles( const idAngles &idealAngle, bool fast );
	bool					Bot_GetProjectileAimPoint( const projectileTypes_t projectileType, idVec3& targetOrg, int ignoreEntNum );
	void					AirVehicleControls( usercmd_t &ucmd );
	void					IcarusVehicleControls( usercmd_t &ucmd );
	void					Bot_ChangeAirVehicleViewAngles( const idAngles &idealAngle, bool fast, bool inCombat );
	idVec3					GetPlayerViewPosition( int clientNum ) const;
	bool					InVehicleGunSights( const idVec3 &org, bool precise );
	void					CheckBotIngameMissionStatus();
	
#ifdef BOT_MOVE_LOOKUP
	void					MoveDirectionToInput( const idVec2& localMoveDir, botMoveFlags_t moveFlags, float& moveForward, float& moveRight );
#endif

private:
	bool					firePistol;
	bool					firePliers;
	bool					lefty;
	bool					moveJumpNow;
	int						leftyTimer;
	int						jumpTimer;
	int						updateAimTime;
	int						proneDelay;
	int						movingForward;
	int						hornTime;
	int						decoyTime;
	int						botNextWeapTime;
	int						botNextVehicleCmdTime;
	int						botNextSiegeCmdTime;
	int						altAttackDelay;
	int						nextAimUpdateTime;
	botGoalTypes_t			oldBotGoalType;
	idAngles				botViewAngles;
	idAngles				botHackMoveAngles;
	idVec3					lowSkillAimPoint;

#ifdef BOT_MOVE_LOOKUP
	// FeaRog: lookup tables to allow bots to easily evaluate moves
	typedef int				moveLookupArray_t[ BOT_MOVE_LOOKUP_STEPS ];

	static void				BuildMoveLookup( moveLookupArray_t& moveForward, moveLookupArray_t& moveRight,
											float fwdSpeed, float sideSpeed, float backSpeed );

	static float			lastRunFwd;
	static float			lastRunSide;
	static float			lastRunBack;
	static float			lastSprintFwd;
	static float			lastSprintSide;

	static moveLookupArray_t	runMoveForward;
	static moveLookupArray_t	runMoveRight;
	static moveLookupArray_t	sprintMoveForward;
	static moveLookupArray_t	sprintMoveRight;
#endif
};

#endif /* !__BOT_H__ */
