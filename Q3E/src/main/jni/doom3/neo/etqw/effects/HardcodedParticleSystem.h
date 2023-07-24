// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_HARDCODEDPARTICLESYSTEM_H__
#define __GAME_HARDCODEDPARTICLESYSTEM_H__


/************************************************************************/
/* This is a shared base class that does some usefull things			*/
/************************************************************************/
class sdHardcodedParticleSystem {

public:
	sdHardcodedParticleSystem( void );
	virtual	~sdHardcodedParticleSystem( void );
	virtual bool	RenderEntityCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime ) { return false; /* no updates by default */ };
	virtual void	PresentRenderEntity( void );
	virtual void	FreeRenderEntity( void );
	renderEntity_t&	GetRenderEntity() { return renderEntity; }
	int				GetModelHandle( void ) { return renderEntityHandle; }

	int				AddSurface( const idMaterial *material, int numVerts, int numIndexes );
	void			AddSurfaceDB( const idMaterial *material, int numVerts, int numIndexes );
	void			ClearSurfaces( void );
	srfTriangles_t *	GetTriSurf( int model, int index ) { return hModel[model]->Surface( index )->geometry; }
	srfTriangles_t *	GetTriSurf( int index ) { return renderEntity.hModel->Surface( index )->geometry; }
	int					NumSurfaces( void ) { return renderEntity.hModel->NumSurfaces(); }

	void			SetDoubleBufferedModel( void );


protected:
	idRenderModel	*hModel[2];
	renderEntity_t	renderEntity;
	int				Uid;
	int				lastGameFrame;
	int				currentDB;

private:
	int				renderEntityHandle;
	static bool ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime );

};

#endif /* !__GAME_HARDCODEDPARTICLESYSTEM_H__ */
