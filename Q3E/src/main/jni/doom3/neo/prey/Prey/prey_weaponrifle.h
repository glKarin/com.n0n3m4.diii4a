#ifndef __HH_WEAPON_RIFLE_H
#define __HH_WEAPON_RIFLE_H

#define CLIENT_ZOOM_FUDGE		100

/***********************************************************************

  hhWeaponZoomable
	
***********************************************************************/
class hhWeaponZoomable : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponZoomable );

	public:
		hhWeaponZoomable() : hhWeapon(), bZoomed( false ) { }

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );

		//rww - network friendliness
		virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

		ID_INLINE bool			IsZoomed() const { return bZoomed; }
		virtual void			ZoomInStep();
		virtual void			ZoomOutStep();

		void					Event_ZoomIn();
		void					Event_ZoomOut();
	protected:
		virtual void			ZoomIn();
		virtual void			ZoomOut();
	protected:
		bool					bZoomed;

	public:
		int						clientZoomTime; //rww - a bit hacky, to prevent jittering in prediction
};

/***********************************************************************

  hhWeaponRifle
	
***********************************************************************/
class hhWeaponRifle : public hhWeaponZoomable {
	CLASS_PROTOTYPE( hhWeaponRifle );

	public:		
		void					Spawn();
		virtual					~hhWeaponRifle();

		virtual void			UpdateGUI();

		virtual void			Ticker();

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );

		//rww - network friendliness
		virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
		
		virtual void			ZoomIn();
		virtual void			ZoomOut();

	protected:
		virtual void			ParseDef( const char* objectname );

		virtual void			ActivateLaserSight();
		virtual void			DeactivateLaserSight();
		
	protected:
		ID_INLINE virtual		hhWeaponFireController* CreateAltFireController();
		void					Event_SuppressSurfaceInViewID( const int id );

	protected:
		idUserInterface*		zoomOverlayGui;
		idEntityPtr<hhBeamSystem>	laserSight;
};

class hhSniperRifleFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhSniperRifleFireController);
public:
	void		CalculateMuzzlePosition( idVec3& origin, idMat3& axis );
};

ID_INLINE hhWeaponFireController*	hhWeaponRifle::CreateAltFireController() {
	return new hhSniperRifleFireController;
}

#endif