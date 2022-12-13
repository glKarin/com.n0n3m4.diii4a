#ifndef __PREY_ITEMS_H__
#define __PREY_ITEMS_H__

// These defined in item.cpp
extern const idEventDef EV_RespawnItem;
extern const idEventDef EV_HideObjective;
extern const idEventDef EV_HideItem;

class hhItem: public idItem {
	CLASS_PROTOTYPE( hhItem );

	public:
		void			Spawn();

		virtual bool	Pickup( idPlayer *player );

		void			EnablePickup();
		void			DisablePickup();

	protected:
		bool			ShouldRespawn( float* respawnDelay = NULL ) const;
		void			PostRespawn( float delay );
		void			DetermineRemoveOrRespawn( int removeDelay );
		int				AnnouncePickup( idPlayer* player );

		void			SinglePlayerPickup( idPlayer *player );

		void			CoopPickup( idPlayer* player );
		void			CoopWeaponPickup( idPlayer *player );
		void			CoopItemPickup( idPlayer *player );

		bool			MultiplayerPickup( idPlayer *player );

		void			SetPickupState( int contents, bool allowPickup );

	protected:
		void			Event_SetPickupState( int contents, bool allowPickup );
		void			Event_Respawn( void );
};	

class hhItemSoul : public hhItem {
	CLASS_PROTOTYPE(hhItemSoul);

	public:
		void			Spawn();
		virtual			~hhItemSoul();

		virtual	void	Think();

		//rww - netcode
		virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );
		virtual void	ClientPredictionThink( void );

		virtual bool	Pickup( idPlayer *player );
		void			Event_TalonAction( idEntity *talon, bool landed );
		void			Event_PostSpawn();
		void			Event_PlaySpiritSound();
		void			Event_AssignFxSoul_Spirit( hhEntityFx* fx );
		void			Event_AssignFxSoul_Physical( hhEntityFx* fx );

		virtual void	Event_Remove();

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		idEntityPtr<idEntityFx>	soulFx;
		idEntityPtr<idEntityFx>	physicalSoulFx;
		idEntityPtr<idEntity>	parentMonster;
		idVec3					velocity;
		idVec3					acceleration;
		float					surge;
		bool					bFollowTriggered;
		idVec3					lastPlayerOrigin;
};


#endif /* __PREY_ITEMS_H__ */
