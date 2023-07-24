// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DEMOS_DEMOCAMERA_H__
#define __GAME_DEMOS_DEMOCAMERA_H__

#include "../Camera.h"

//===============================================================
//
//	sdDemoCamera
//
//===============================================================

class sdDemoCamera {
public:
							sdDemoCamera() :
								origin( vec3_origin ),
								axis( mat3_identity ),
								fov( 90.f ),
								name( "" ) {
							}
	virtual					~sdDemoCamera() {
							}

	virtual const char*		GetName() const { return name; }

	virtual const idVec3&	GetOrigin() const { return origin; }
	virtual const idMat3&	GetAxis() const { return axis; }
	virtual const float		GetFov() const { return fov; }

	virtual bool			Parse( idParser& src ) = 0;

	virtual void			Start() {}
	virtual void			RunFrame() {}

protected:
	bool					ParseKey( const idToken& key, idParser& src );

protected:
	idVec3					origin;
	idMat3					axis;
	float					fov;

private:
	idStr					name;
};

//===============================================================
//
//	sdDemoCamera_Fixed
//
//===============================================================

class sdDemoCamera_Fixed : public sdDemoCamera {
public:
							sdDemoCamera_Fixed() {
							}
	virtual					~sdDemoCamera_Fixed() {
							}

	static const char*		GetType() { return "fixed"; }

	virtual bool			Parse( idParser& src );
};

//===============================================================
//
//	sdDemoCamera_Anim
//
//===============================================================

class sdDemoCamera_Anim : public sdDemoCamera {
public:
							sdDemoCamera_Anim() {
							}
	virtual					~sdDemoCamera_Anim() {
							}

	static const char*		GetType() { return "anim"; }

	virtual bool			Parse( idParser& src );

	virtual void			Start();
	virtual void			RunFrame();

private:
	idCamera_MD5			cameraMD5;
};

#endif // __GAME_DEMOS_DEMOCAMERA_H__
