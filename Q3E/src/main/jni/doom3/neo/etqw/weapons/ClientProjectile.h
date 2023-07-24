// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_CLIENTPROJECTILE_H__
#define __GAME_CLIENTPROJECTILE_H__

/*
===============================================================================

  sdClientProjectile
	
===============================================================================
*/

class sdClientProjectile : public sdClientEntity {
public :
	CLASS_PROTOTYPE( sdClientProjectile );

							sdClientProjectile( void );
	virtual					~sdClientProjectile( void );

	void					Spawn( void );

	virtual void			Think( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual bool			CollideEffect( idEntity* ent, const trace_t &collision, const idVec3 &velocity );
	virtual void			Explode( const trace_t *collision, const char *sndExplode = "snd_explode" );

	enum {
		EVENT_COLLIDE = sdClientEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	virtual bool			ClientReceiveEvent( const idVec3 &origin, int event, int time, const idBitMsg &msg );

protected:
	void					DefaultDamageEffect( const trace_t &collision, const idVec3 &velocity );
};


#endif /* !__GAME_CLIENTPROJECTILE_H__ */
