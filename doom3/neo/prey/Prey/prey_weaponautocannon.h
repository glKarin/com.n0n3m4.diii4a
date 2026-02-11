#ifndef __HH_WEAPON_AUTOCANNON_H
#define __HH_WEAPON_AUTOCANNON_H

/***********************************************************************

  hhWeaponAutoCannon
	
***********************************************************************/
class hhWeaponAutoCannon : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponAutoCannon );

	public:
		enum {
			EVENT_OVERHEAT = hhWeapon::EVENT_MAXEVENTS,
			EVENT_MAXEVENTS
		};

		void						Spawn();
		virtual 					~hhWeaponAutoCannon();

		virtual void				ParseDef( const char* objectname );
		virtual void				UpdateGUI();

		virtual void				Show();
		virtual void				Hide();

		void						Save( idSaveGame *savefile ) const;
		void						Restore( idRestoreGame *savefile );

	protected:
		void						Ticker();
		void						AdjustHeat( const float amount );

		void						InitBoneInfo();
		void						SpawnSpark();

		void						SetHeatLevel( const float heatLevel );
		float						GetHeatLevel() const { return heatLevel; }

		virtual void				PresentWeapon( bool showViewModel );

		virtual void				WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void				ReadFromSnapshot( const idBitMsgDelta &msg );
	protected:
		void						Event_SpawnRearGasFX();
		void						Event_AssignLeftRearFx( hhEntityFx* fx );
		void						Event_AssignRightRearFx( hhEntityFx* fx );
		void						Event_AdjustHeat( const float amount );
		void						Event_GetHeatLevel();
		void						Event_SpawnSparkLocal( const char* defName );
		void						Event_OverHeatNetEvent(); //rww

		virtual bool				ClientReceiveEvent( int event, int time, const idBitMsg &msg ); //rww

	protected:
		float						heatLevel;

		float						sparkGapSize;
		idEntityPtr<hhBeamSystem>	beamSystem;
		weaponJointHandle_t			sparkBoneL;
		weaponJointHandle_t			sparkBoneR;

		idEntityPtr<hhEntityFx>		rearGasFxL;
		idEntityPtr<hhEntityFx>		rearGasFxR;
};

#endif