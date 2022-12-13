
#ifndef __GAME_ANIMATED_H__
#define __GAME_ANIMATED_H__

class hhAnimated : public idAnimated {
	CLASS_PROTOTYPE( hhAnimated );

	public:
		void			Spawn();
		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );
		virtual void	Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
		virtual void	Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
		virtual bool	UpdateAnimationControllers( void );

		bool			StartRagdoll( void );

	protected:
		void			Event_SetAnim( const char* animname );
		void			Event_IsAnimDone( int timeMS );
		virtual void	Event_AnimDone( int animIndex );
		void			Event_Start( void );
		void			Event_PositionDefaultPose();
		void			Event_StartDefaultAnim();
		virtual void	Event_Activate( idEntity *_activator );
		void			Event_Footstep();

	protected:
		bool			isAnimDone;
};

#endif /* __GAME_ANIMATED_H__ */
