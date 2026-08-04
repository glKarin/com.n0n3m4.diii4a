/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
	virtual renderView_t *	GetRenderView() override;
	virtual void			Stop( void ) { }
};

/*
===============================================================================

idCameraView

===============================================================================
*/

class idCameraView : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraView );
							idCameraView();

	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( );
	virtual void			GetViewParms( renderView_t *view ) override;
	virtual void			Stop( void ) override;

protected:
	void					Event_Activate( idEntity *activator );
	void					Event_SetAttachments();
	void					SetAttachment( idEntity **e, const char *p );
	float					fov;
	idEntity				*attachedTo;
	idEntity				*attachedView;
};



/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/

typedef struct {
	idCQuat				q;
	idVec3				t;
	float				fov;
} cameraFrame_t;

class idCameraAnim : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraAnim );

							idCameraAnim();
	virtual					~idCameraAnim() override;

	// save games
	void					Save( idSaveGame *savefile ) const;				// archives object for save game file
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view ) override;

private:
	int						threadNum;
	idVec3					offset;
	int						frameRate;
	int						starttime;
	int						cycle;
	idList<int>				cameraCuts;
	idList<cameraFrame_t>	camera;
	idEntityPtr<idEntity>	activator;

	void					Start( void );
	virtual void			Stop( void ) override;
	virtual void			Think( void ) override;

	void					LoadAnim( void );
	void					Event_Start( void );
	void					Event_Stop( void );
	void					Event_SetCallback( void );
	void					Event_Activate( idEntity *activator );
};

#endif /* !__GAME_CAMERA_H__ */
