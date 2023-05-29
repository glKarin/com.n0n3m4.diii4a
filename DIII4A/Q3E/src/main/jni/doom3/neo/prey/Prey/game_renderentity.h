#ifndef __GAME_RENDERENTITY_H
#define __GAME_RENDERENTITY_H

extern const idEventDef EV_ModelDefHandleIsValid;

class hhRenderEntity : public idEntity {
	CLASS_PROTOTYPE( hhRenderEntity );
	
	protected:
									hhRenderEntity();
		virtual						~hhRenderEntity();

		void						Save( idSaveGame *savefile ) const;
		void						Restore( idRestoreGame *savefile );

		virtual void				Think();

		virtual void				InitCombatModel( const int renderModelHandle );
		virtual void				LinkCombatModel( idEntity* self, const int renderModelHandle );

	public:
		virtual void				Present();

	protected:
		void						Event_ModelDefHandleIsValid();

	protected:
		idClipModel*				combatModel;
};

#endif