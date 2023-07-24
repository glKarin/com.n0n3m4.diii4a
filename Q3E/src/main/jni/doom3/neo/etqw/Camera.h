// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__


/*
===============================================================================

Camera providing an alternative view of the level.

===============================================================================
*/

class idCamera : public idEntity {
public:
	ABSTRACT_PROTOTYPE( idCamera );

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view ) = 0;
	virtual renderView_t *	GetRenderView();
	virtual void			Stop( void ){};

	void					InitRenderView( void );

private:
	renderView_s			renderView;
};

class sdCamera_Placement : public idCamera {
public:
	CLASS_PROTOTYPE( sdCamera_Placement );

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t* view );

private:
	float					fov;
};

/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/

class idCamera_MD5 {
public:
							idCamera_MD5();
							~idCamera_MD5();

	bool					LoadAnim( const char* filename );

	int						GetStartTime() const { return starttime; }

	void					SetOffset( const idVec3& offset )	{ this->offset = offset; }
    void					SetCycle( int cycle )				{ this->cycle = cycle; }
	void					SetStartTime( int starttime )		{ this->starttime = starttime; }

	// these two return true if the cinematic stopped
	bool					SkipToEnd();
	bool					Evaluate( idVec3& origin, idMat3& axis, float& fov, int time );

private:
	idVec3					offset;
	int						frameRate;
	int						cycle;
	int						starttime;
	idList<int>				cameraCuts;
	idList<cameraFrame_t>	camera;
};

#endif /* !__GAME_CAMERA_H__ */
