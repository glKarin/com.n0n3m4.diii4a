// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __ADMANAGER_H__
#define __ADMANAGER_H__

struct impressionInfo_t {
	bool			inView;
	unsigned int	size;
	float			angle;
	short			screenWidth;
	short			screenHeight;

	bool			played;
	float			falloff;
};

#ifdef MASSIVE

class sdAdObjectCallback {
public:
	virtual void			OnImageLoaded( class idImage* image ) = 0;
	virtual void			OnDestroyed( void ) = 0;
	virtual void			UpdateImpression( impressionInfo_t& impression, const struct renderView_s& view, const class sdBounds2D& viewPort ) = 0;
};

class sdAdObjectSubscriberInterface {
public:
	virtual void			Free( void ) = 0;
	virtual void			Activate( void ) = 0;
};

class sdAdManager {
public:
	virtual									~sdAdManager( void ) {}

	virtual void							Init() = 0;
	virtual void							Shutdown() = 0;

	virtual sdAdObjectSubscriberInterface*	AllocAdSubscriber( const char* objectName, sdAdObjectCallback* callback ) = 0;
	virtual void							SetAdZone( const char* zoneName ) = 0;
};

extern sdAdManager* adManager;

#else

// stub

class sdAdObjectCallback {
public:
	virtual void			OnImageLoaded( class idImage* image ) {}
	virtual void			OnDestroyed( void ) {}
	virtual void			UpdateImpression( impressionInfo_t& impression, const struct renderView_s& view, const class sdBounds2D& viewPort ) {}
};

class sdAdObjectSubscriberInterface {
public:
	virtual void			Free( void ) {}
	virtual void			Activate( void ) {}
};

class sdAdManager {
public:
	virtual									~sdAdManager( void ) {}

	virtual void							Init() {};
	virtual void							Shutdown() {};

	virtual sdAdObjectSubscriberInterface*	AllocAdSubscriber( const char* objectName, sdAdObjectCallback* callback ) { return NULL; }
	virtual void							SetAdZone( const char* zoneName ) {}
};

extern sdAdManager* adManager;

#endif /* MASSIVE */

#endif /* __ADMANAGER_H__ */
