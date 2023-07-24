// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MISC_ADENTITY_H__
#define __GAME_MISC_ADENTITY_H__

#include "../Entity.h"
#include "../../framework/AdManager.h"

class sdAdEntity;

class sdAdEntityCallback : public sdAdObjectCallback {
public:
										sdAdEntityCallback( void ) : owner( NULL ) { ; }

	void								Init( sdAdEntity* _owner ) { owner = _owner; }

	virtual void						OnImageLoaded( idImage* image );
	virtual void						OnDestroyed( void );
	virtual void						UpdateImpression( impressionInfo_t& impression, const renderView_t& view, const sdBounds2D& viewPort );

private:
	sdAdEntity*							owner;
};

class sdAdEntity : public idEntity {
public:
	CLASS_PROTOTYPE( sdAdEntity );

										sdAdEntity( void );
	virtual								~sdAdEntity( void );

	void								OnImageLoaded( idImage* image );
	void								OnAdDestroyed( void );
	void								UpdateImpression( impressionInfo_t& impression, const renderView_t& view, const sdBounds2D& viewPort );

	virtual void						OnBulletImpact( idEntity* attacker, const trace_t& trace );

//	virtual void						Think( void );

	void								DrawImpression( void );

	void								Spawn( void );

protected:
	const modelSurface_t*				adSurface;
	idVec3								adSurfaceNormal;
	sdAdEntityCallback					adCallback;
	sdAdObjectSubscriberInterface*		adObject;
	impressionInfo_t					lastImpression;
};

#endif // __GAME_MISC_DEFENCETURRET_H__
