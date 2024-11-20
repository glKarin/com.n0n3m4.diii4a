////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TFBaseStates_H__
#define __TFBaseStates_H__

#include "StateMachine.h"
#include "Path.h"
#include "ScriptManager.h"
#include "TF_Messages.h"
#include "InternalFsm.h"

class gmScriptGoal;

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	namespace TF_Options
	{
		extern int BUILD_AMMO_TYPE;
		
		extern int SENTRY_BUILD_AMMO;
		extern int SENTRY_UPGRADE_AMMO;
		extern int SENTRY_REPAIR_AMMO;
		extern int SENTRY_UPGRADE_WPN;

		extern int BUILD_ATTEMPT_DELAY;

		extern int TELEPORT_BUILD_AMMO;
		extern int DISPENSER_BUILD_AMMO;

		extern int PIPE_WEAPON;
		extern int PIPE_WEAPON_WATCH;
		extern int PIPE_AMMO;
		extern int PIPE_MAX_DEPLOYED;

		extern int ROCKETJUMP_WPN;
		extern float GRENADE_VELOCITY;

		extern bool POLL_SENTRY_STATUS;
		extern bool REPAIR_ON_SABOTAGED;
	};
	//////////////////////////////////////////////////////////////////////////
	enum SentryBuildState
	{
		SG_NONE,
		SG_GETTING_AMMO,
		SG_BUILDING,
		SG_AIMING,
		SG_AIMED,
		SG_UPGRADING,
		SG_REPAIRING,
		SG_RESUPPLY,
		SG_DONE,
		SG_NUMBUILDSTATES,
	};
	class SentryBuild : 
		public StateChild, 
		public FollowPathUser, 
		public AimerUser, 
		public InternalFSM<SentryBuild,SG_NUMBUILDSTATES>
	{
	public:

		void GetDebugString(StringStr &out);

		MapGoalPtr GetBuiltSentryMapGoal() const { return m_BuiltSentry; }

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		static int BuildEquipWeapon;

		SentryBuild();
	private:
		MapGoalPtr		m_MapGoalSentry;
		MapGoalPtr		m_BuiltSentry;
		int				m_NextBuildTry;
		int				m_NeedAmmoAmount;

		obReal			m_SentryPriority;

		RecordHandle	m_AmmoPack;

		bool			m_CantBuild : 1;

		bool HasEnoughAmmo(int _ammotype, int _ammorequired);

		STATE_PROTOTYPE(SG_NONE);
		STATE_PROTOTYPE(SG_GETTING_AMMO);
		STATE_PROTOTYPE(SG_BUILDING);
		STATE_PROTOTYPE(SG_AIMING);
		STATE_PROTOTYPE(SG_AIMED);
		STATE_PROTOTYPE(SG_UPGRADING);
		STATE_PROTOTYPE(SG_REPAIRING);
		STATE_PROTOTYPE(SG_RESUPPLY);
		STATE_PROTOTYPE(SG_DONE);
	};

	//////////////////////////////////////////////////////////////////////////
	class SentryAlly : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		enum SubState
		{
			UPGRADING,
			REPAIRING,
			RESUPPLY,
		};

		void GetDebugString(StringStr &out);

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		SentryAlly();
	private:
		RecordHandle	m_AllySentry;
		SubState		m_State;
	};

	//////////////////////////////////////////////////////////////////////////
	class DispenserBuild : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		enum SubState
		{
			DISP_NONE,
			DISP_GETTING_AMMO,
			DISP_BUILDING,
			DISP_REPAIRING,
			DISP_DONE,
			DISP_NUMBUILDSTATES,
		};

		void GetDebugString(StringStr &out);

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		static int BuildEquipWeapon;

		DispenserBuild();
	private:
		MapGoalPtr		m_MapGoalDisp;
		MapGoalPtr		m_BuiltDisp;
		int				m_NextBuildTry;
		int				m_NextAmmoCheck;
		SubState		m_State;
		bool			m_CantBuild : 1;
	};

	//////////////////////////////////////////////////////////////////////////
	class Sentry : public StatePrioritized
	{
	public:
		typedef struct 
		{		
			TF_BuildableStatus	m_Status;
			GameEntity			m_Entity;
			Vector3f			m_Position;
			Vector3f			m_Facing;
			int					m_Level;
			int					m_Health;
			int					m_MaxHealth;
			int					m_Shells[2];
			int					m_Rockets[2];
			bool				m_Sabotaged;

			void Reset()
			{
				m_Status = BUILDABLE_INVALID;
				m_Entity.Reset();
				m_Position = Vector3f::ZERO;
				m_Facing = Vector3f::ZERO;
				m_Level = 0;
				m_Health = m_MaxHealth = 0;
				m_Shells[0] = m_Shells[1] = 0;
				m_Shells[0] = m_Rockets[1] = 0;
				m_Sabotaged = false;
			}
		} SentryStatus;

		void GetDebugString(StringStr &out);

		bool SentryFullyBuilt() const { return (m_SentryStatus.m_Status == BUILDABLE_BUILT); }
		bool HasSentry() const { return m_SentryStatus.m_Entity.IsValid(); }
		bool SentryBuilding() const { return (m_SentryStatus.m_Status == BUILDABLE_BUILDING); }

		const SentryStatus &GetSentryStatus() const { return m_SentryStatus; }

		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		void UpdateSentryStatus(const Event_SentryStatus_TF &_stats);

		Sentry();
	private:
		SentryStatus	m_SentryStatus;
	};

	//////////////////////////////////////////////////////////////////////////
	class Dispenser : public StatePrioritized
	{
	public:
		typedef struct 
		{
			TF_BuildableStatus	m_Status;
			GameEntity			m_Entity;
			Vector3f			m_Position;
			Vector3f			m_Facing;
			int					m_Health;
			int					m_Cells;
			int					m_Nails;
			int					m_Rockets;
			int					m_Shells;
			int					m_Armor;
			bool				m_Sabotaged;

			void Reset()
			{
				m_Status = BUILDABLE_INVALID;
				m_Entity.Reset();
				m_Position = Vector3f::ZERO;
				m_Facing = Vector3f::ZERO;
				m_Health = 0;
				m_Cells = 0;
				m_Nails = 0;
				m_Rockets = 0;
				m_Shells = 0;
				m_Armor = 0;
				m_Sabotaged = false;
			}
		} DispenserStatus;

		void GetDebugString(StringStr &out);

		bool DispenserFullyBuilt() const { return (m_DispenserStatus.m_Status == BUILDABLE_BUILT); }
		bool HasDispenser() const { return m_DispenserStatus.m_Entity.IsValid(); }
		bool DispenserBuilding() const { return (m_DispenserStatus.m_Status == BUILDABLE_BUILDING); }

		const DispenserStatus &GetDispenserStatus() const { return m_DispenserStatus; }

		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		void UpdateDispenserStatus(const Event_DispenserStatus_TF &_stats);

		Dispenser();
	private:
		DispenserStatus		m_DispenserStatus;
	};

	//////////////////////////////////////////////////////////////////////////
	class DetpackBuild : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		enum SubState
		{
			NONE,
			DETPACK_BUILDING,
		};

		void GetDebugString(StringStr &out);

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		DetpackBuild();
	private:
		MapGoalPtr		m_MapGoal;
		int				m_NextBuildTry;
		SubState		m_State;
		int				m_DetpackFuse;
		bool			m_CantBuild : 1;
	};
	//////////////////////////////////////////////////////////////////////////

	class Detpack : public StatePrioritized
	{
	public:
		typedef struct 
		{
			TF_BuildableStatus	m_Status;
			GameEntity			m_Entity;
			Vector3f			m_Position;
		} DetpackStatus;

		bool DetpackFullyBuilt() const { return (m_DetpackStatus.m_Status == BUILDABLE_BUILT); }
		bool HasDetpack() const { return m_DetpackStatus.m_Entity.IsValid(); }
		bool DetpackBuilding() const { return (m_DetpackStatus.m_Status == BUILDABLE_BUILDING); }

		const DetpackStatus &GetDetpackStatus() const { return m_DetpackStatus; }

		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		void UpdateDetpackStatus(GameEntity _ent);

		Detpack();
	private:
		DetpackStatus	m_DetpackStatus;
	};
	//////////////////////////////////////////////////////////////////////////

	class PipeTrap : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		enum SubState
		{
			IDLE,
			LAY_PIPES,
			WATCH_PIPES,
			GETTING_AMMO,
		};

		enum Order
		{
			OrderRandomAll,
			OrderRandomPick1,
			OrderSequentialAll,
		};

		struct PipeInfo
		{
			GameEntity	m_Entity;
			Vector3f	m_Position;
			obuint32	m_TrapIndex : 4;
			obuint32	m_Moving : 1;
		};

		struct Pipes
		{
			enum { MaxPipes = 8 };
			PipeInfo	m_Pipes[MaxPipes];
			int			m_PipeCount;
		};

		struct Trap
		{
			Vector3f	m_Source;
			Vector3f	m_Facing;
			void Reset()
			{
				m_Source = Vector3f::ZERO;
				m_Facing = Vector3f::ZERO;
			}
		};

		struct WaitPos
		{
			Vector3f	m_Position;
			Vector3f	m_Facing;
			int			m_MinWaitTime;
			int			m_MaxWaitTime;

			void Reset()
			{
				m_Position = Vector3f::ZERO;
				m_Facing = Vector3f::ZERO;
				m_MinWaitTime = 2000;
				m_MaxWaitTime = 6000;
			}
		};

		const Pipes &GetPipes() const { return m_Pipes; }
		int GetPipeCount() const { return m_Pipes.m_PipeCount; }

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);
		void GetDebugString(StringStr &out);
		void RenderDebug();

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		PipeTrap();
	private:
		SubState	m_Substate;
		Pipes		m_Pipes;

		enum { MaxPlacementPts = 8, MaxWaitPositions = 8 };
		Trap		m_Traps[MaxPlacementPts];
		WaitPos		m_Wait[MaxWaitPositions];

		MapGoalPtr	m_MapGoal;
		FilterPtr	m_WatchFilter;

		obint32		m_PlaceOrder;
		obint32		m_WaitOrder;

		obint32		m_ExpireTime;

		obuint32	m_NumTraps : 4;
		obuint32	m_NumWaits : 4;

		obuint32	m_CurrentTrap : 4;
		obuint32	m_CurrentWait : 4;

		bool		CacheGoalInfo(MapGoalPtr mg);
		bool		ShouldGoForAmmo();
	};

	//////////////////////////////////////////////////////////////////////////

	class RocketJump : public StateChild, public AimerUser
	{
	public:
		
		void RenderDebug();

		obReal GetPriority();
		StateStatus Update(float fDt);

		void Enter();
		void Exit();

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		RocketJump();
	private:
		Path::PathPoint m_NextPt;
		bool			m_IsDone : 1;
	};

	//////////////////////////////////////////////////////////////////////////

	class ConcussionJump : public StateChild, public AimerUser
	{
	public:

		void RenderDebug();

		obReal GetPriority();
		StateStatus Update(float fDt);

		void Enter();
		void Exit();

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		ConcussionJump();
	private:
		Path::PathPoint m_NextPt;
	};

	//////////////////////////////////////////////////////////////////////////

	class ThrowGrenade : public StateChild, public AimerUser
	{
	public:
		enum AimMode
		{
			GRENADE_AIM,
			GRENADE_RID,
		};

		void GetDebugString(StringStr &out);
		void RenderDebug();

		obReal GetPriority();
		StateStatus Update(float fDt);

		void Enter();
		void Exit();

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		void OnSpawn();

		float GetGrenadeSpeed(int _type) const;

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		ThrowGrenade();
	private:
		int		m_PrimaryGrenade;
		int		m_SecondaryGrenade;

		int		m_Gren1Ammo, m_Gren1AmmoMax;
		int		m_Gren2Ammo, m_Gren2AmmoMax;

		int		m_GrenType;

		int		m_LastThrowTime;

		Vector3f m_AimPos;

		AimMode	m_AimMode;

		bool	m_OnTarget;

		float _GetProjectileSpeed(int _type) const;
		float _GetProjectileGravity(int _type) const;
		void _UpdateAmmo();
		void _UpdateGrenadeTypes();
	};

	//////////////////////////////////////////////////////////////////////////
	class Teleporter : public StatePrioritized
	{
	public:
		typedef struct 
		{
			TF_BuildableStatus	m_StatusEntrance;
			TF_BuildableStatus	m_StatusExit;
			GameEntity			m_EntityEntrance;
			GameEntity			m_EntityExit;

			Vector3f			m_EntrancePos;
			Vector3f			m_ExitPos;

			bool				m_SabotagedEntry;
			bool				m_SabotagedExit;

			void Reset()
			{
				m_StatusEntrance = BUILDABLE_INVALID;
				m_StatusExit = BUILDABLE_INVALID;
				m_EntityEntrance.Reset();
				m_EntityExit.Reset();

				m_SabotagedEntry = false;
				m_SabotagedExit = false;
			}
		} TeleporterStatus;

		bool TeleporterEntranceFullyBuilt() const { return (m_TeleporterStatus.m_StatusEntrance == BUILDABLE_BUILT); }
		bool TeleporterExitFullyBuilt() const { return (m_TeleporterStatus.m_StatusExit == BUILDABLE_BUILT); }
		bool HasTeleporterEntrance() const { return m_TeleporterStatus.m_EntityEntrance.IsValid(); }
		bool HasTeleporterExit() const { return m_TeleporterStatus.m_EntityExit.IsValid(); }
		bool TeleporterEntranceBuilding() const { return (m_TeleporterStatus.m_StatusEntrance == BUILDABLE_BUILDING); }
		bool TeleporterExitBuilding() const { return (m_TeleporterStatus.m_StatusExit == BUILDABLE_BUILDING); }

		const TeleporterStatus &GetTeleporterStatus() const { return m_TeleporterStatus; }

		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		void UpdateTeleporterStatus(const Event_TeleporterStatus_TF &_stats);

		Teleporter();
	private:
		TeleporterStatus		m_TeleporterStatus;
	};

	//////////////////////////////////////////////////////////////////////////
	class TeleporterBuild : public StateChild, public FollowPathUser, public AimerUser
	{
	public:
		enum SubState
		{
			NONE,
			GETTING_AMMO,
			TELE_BUILDING_ENTRANCE,
			TELE_BUILDING_EXIT,
		};

		void GetDebugString(StringStr &out);

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void ProcessEvent(const MessageHelper &_message, CallbackParameters &_cb);

		// FollowPathUser functions.
		bool GetNextDestination(DestinationVector &_desination, bool &_final, bool &_skiplastpt);

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		static int BuildEquipWeapon;

		TeleporterBuild();
	private:
		MapGoalPtr		m_MapGoalTeleEntrance;
		MapGoalPtr		m_BuiltTeleEntrance;

		MapGoalPtr		m_MapGoalTeleExit;
		MapGoalPtr		m_BuiltTeleExit;

		int				m_NextBuildTry;
		int				m_NextAmmoCheck;
		SubState		m_State;
		bool			m_CantBuild : 1;
	};

};

#endif
