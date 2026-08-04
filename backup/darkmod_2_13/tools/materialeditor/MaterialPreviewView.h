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
#pragma once

#include "MaterialView.h"
#include "../radiant/GLWidget.h"


class idGLDrawableView : public idGLDrawable {

public:
	idGLDrawableView();
	~idGLDrawableView();

	virtual void setMedia(const char *name);
	virtual void draw(int x, int y, int w, int h);
	virtual void buttonUp(int button){}
	virtual void buttonDown(int _button, float x, float y);
	virtual void mouseMove(float x, float y);
	virtual void Update() {};

	void UpdateCamera( renderView_t *refdef );
	void UpdateModel( void );
	void UpdateLights( void );

	void addLight( void );
	void deleteLight( const int lightId );
	void drawLights( renderView_t *refdef );

	void InitWorld();
	void ResetView( void );

	void setLightShader( const int lightId, const idStr shaderName );
	void setLightColor( const int lightId, const idVec3 &value );
	void setLightRadius( const int lightId, const float radius );
	void setLightAllowMove( const int lightId, const bool move );
	void setObject( int Id );
	void setCustomModel( const idStr modelName );
	void setShowLights( bool _showLights );
	void setLocalParm( int parmNum, float value );
	void setGlobalParm( int parmNum, float value );

protected:
	idRenderWorld		*world;
	idRenderModel		*worldModel;
	const idMaterial	*material;

	bool			showLights;

	idVec3			viewOrigin;
	idAngles		viewRotation;
	float			viewDistance;

	renderEntity_t	worldEntity;
	qhandle_t		modelDefHandle;

	int				objectId;
	idStr			customModelName;

	float			globalParms[MAX_GLOBAL_SHADER_PARMS];

	typedef struct {
		renderLight_t		renderLight;
		qhandle_t			lightDefHandle;
		idVec3				origin;
		const idMaterial	*shader;
		float				radius;
		idVec3				color;
		bool				allowMove;
	} lightInfo_t;

	idList<lightInfo_t>	viewLights;
};


// ==================================================================
// ==================================================================

class MaterialPreviewView : public CView, public MaterialView
{
	DECLARE_DYNCREATE(MaterialPreviewView)

protected:
	MaterialPreviewView();
	virtual ~MaterialPreviewView();

public:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view

	void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);

	void	OnModelChange( int modelId );
	void	OnCustomModelChange( idStr modelName );
	void	OnShowLightsChange( bool showLights );

	void	OnLocalParmChange( int parmNum, float value );
	void	OnGlobalParmChange( int parmNum, float value );

	void	OnLightShaderChange( int lightId, idStr shaderName );
	void	OnLightRadiusChange( int lightId, float radius );
	void	OnLightColorChange( int lightId, idVec3 &color );
	void	OnLightAllowMoveChange( int lightId, bool move );

	void	OnAddLight( void );
	void	OnDeleteLight( int lightId );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	idGLWidget			renderWindow;
	idGLDrawableView	renderedView;

	idStr	currentMaterial;

	DECLARE_MESSAGE_MAP()

public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};

