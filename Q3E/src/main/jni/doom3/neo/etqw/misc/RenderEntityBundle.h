// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_MISC_RENDERENTITYBUNDLE_H__
#define __GAME_MISC_RENDERENTITYBUNDLE_H__

class sdRenderEntityBundle {
public:
							sdRenderEntityBundle( void );
							~sdRenderEntityBundle( void );

	void					Copy( const renderEntity_t& rhs );
	void					Update( void );
	void					Show( void );
	void					Hide( void );

	renderEntity_t&			GetEntity( void ) { return entity; }
	qhandle_t				GetHandle( void ) { return handle; }

private:
	qhandle_t				handle;
	renderEntity_t			entity;
};

#endif // __GAME_MISC_RENDERENTITYBUNDLE_H__
