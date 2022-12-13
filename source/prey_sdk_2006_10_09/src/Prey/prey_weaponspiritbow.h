#ifndef __HH_WEAPON_SPIRITBOW_H
#define __HH_WEAPON_SPIRITBOW_H

/***********************************************************************

  hhWeaponSpiritBow
	
***********************************************************************/

class hhSpiritBowFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhSpiritBowFireController);
public:
	int				AmmoRequired() const;
	virtual	const idDict*		GetProjectileDict() const;
};

class hhWeaponSpiritBow : public hhWeaponZoomable {
	CLASS_PROTOTYPE( hhWeaponSpiritBow );

	public:
		~hhWeaponSpiritBow();

		void			Spawn();

		bool			BowVisionIsEnabled() const { return visionEnabled; }
		void			BowVisionIsEnabled( bool enabled ) { visionEnabled = enabled; }
		void			StopBowVision();
		virtual void	BeginAltAttack( void );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

		//rww - network friendliness
		virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

	protected:
		virtual void	Ticker();
		hhTargetProxy *	GetProxyOf( const idEntity *target );
		bool			ProxyShouldBeVisible( const idEntity* ent );
		ID_INLINE virtual		hhWeaponFireController* CreateFireController();

	protected:
		void			Event_StartSeeThroughWalls();
		void			Event_FadeOutSeeThroughWalls();
		void			Event_StopSeeThroughWalls();
		void			Event_UpdateBowVision();

		void			Event_BowVisionIsEnabled();

	protected:
		idInterpolate<float>	fadeAlpha;
		int						updateRover;

		bool					visionEnabled;
};

ID_INLINE hhWeaponFireController*	hhWeaponSpiritBow::CreateFireController() {
	return new hhSpiritBowFireController;
}


#endif