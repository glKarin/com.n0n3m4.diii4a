// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_GUIS_USERINTERFACERENDERWORLD_H__
#define __GAME_GUIS_USERINTERFACERENDERWORLD_H__

#include "UserInterfaceTypes.h"
#include "../Camera.h"

extern const char sdUITemplateFunctionInstance_IdentifierRenderWorld[];
SD_UI_PROPERTY_TAG(
	alias = "renderworld";
)
class sdUIRenderWorld :
	public sdUIWindow {
public:
	SD_UI_DECLARE_CLASS( sdUIRenderWorld )
	typedef sdUITemplateFunction< sdUIRenderWorld > RenderWorldTemplateFunction;
	sdUIRenderWorld();
	virtual										~sdUIRenderWorld();

	virtual const char*							GetScopeClassName() const { return "sdUIRenderWorld"; }

	virtual sdUIFunctionInstance*				GetFunction( const char* name );
	virtual bool								RunNamedMethod( const char* name, sdUIFunctionStack& stack );

	static void									InitFunctions( void );
	static void									ShutdownFunctions( void ) { renderWorldFunctions.DeleteContents(); }
	static const RenderWorldTemplateFunction*	FindFunction( const char* name );	

protected:
	virtual void								DrawLocal();	

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderWorld/atmosphere";
	desc				= "Set an atmosphere (see atmosphere defs)";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	atmosphere;
	// ===========================================

private:	
	void OnAtmosphereChanged( const idStr& oldValue, const idStr& newValue );	
	void Update_r( sdUIObject* child );

private:
	renderView_t		viewDef;
	idRenderWorld*		world;

	static idHashMap< RenderWorldTemplateFunction* >	renderWorldFunctions;
};


/*
============
sdUIRenderWorld_Child
============
*/
class sdUIRenderWorld_Child :
	public sdUIObject {
public:
	SD_UI_DECLARE_ABSTRACT_CLASS( sdUIRenderWorld_Child )
				
						sdUIRenderWorld_Child();
	virtual				~sdUIRenderWorld_Child() {}

	virtual void		SetRenderWorld( idRenderWorld* world ) { renderWorld = world; }
	virtual void		Update( renderView_t& view ) = 0;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderWorld_Child/Visible";
	desc				= "Set visibility.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		visible;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderWorld_Child/BindJoint";
	desc				= "Bind joint for object.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	bindJoint;
	// ===========================================
protected:
	idRenderWorld* renderWorld;
};

/*
============
sdUIRenderModel
============
*/
class sdUIRenderModel :
	public sdUIRenderWorld_Child {
public:
	SD_UI_DECLARE_CLASS( sdUIRenderModel )

						sdUIRenderModel();
	virtual				~sdUIRenderModel();

	virtual const char*	GetScopeClassName() const { return "sdUIRenderModel"; }

	virtual void		SetRenderWorld( idRenderWorld* world );
	virtual void		Update( renderView_t& viewDef );

protected:
	void 				OnModelInfoChanged( const idStr& oldValue, const idStr& newValue );
	void				OnVisibleChanged( const float oldValue, const float newValue );
	void 				SpawnDefs();
	bool 				SpawnInfoValid() const;
	void				FreeModel();

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/ModelName";
	desc				= "Model name.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	modelName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/SkinName";
	desc				= "Skin name.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	skinName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/AnimName";
	desc				= "Animation name.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	animName;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/AnimClass";
	desc				= "Animation class.";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	animClass;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/ModelOrigin";
	desc				= "Model origin in the render world.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		modelOrigin;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/ModelRotation";
	desc				= "Model rotation.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		modelRotation;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderModel/Animate";
	desc				= "Animate the model.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		animate;
	// ===========================================

private:
	renderEntity_t		worldEntity;
	const idMD5Anim*	modelAnim;
	idRenderModel*		worldModel;
	qhandle_t   		modelDef;
	int 				animLength;
	int 				animEndTime;
};

/*
============
sdUIRenderLight
============
*/
class sdUIRenderLight :
	public sdUIRenderWorld_Child {
public:
	SD_UI_DECLARE_CLASS( sdUIRenderLight )

						sdUIRenderLight();
	virtual				~sdUIRenderLight();

	virtual const char*	GetScopeClassName() const { return "sdUIRenderLight"; }

	virtual void		SetRenderWorld( idRenderWorld* world );
	virtual void		Update( renderView_t& viewDef );

protected:
	void				OnVisibleChanged( const float oldValue, const float newValue );
	void				SpawnDefs();
	void				FreeLight();

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderLight/LightOrigin";
	desc				= "Light origin.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		lightOrigin;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderLight/LightColor";
	desc				= "Light color.";
	editor				= "edit";
	datatype			= "vec4";
	)
	sdVec4Property		lightColor;
	// ===========================================

private:
	renderLight_t		rLight;
	qhandle_t			lightDef;
};

/*
============
sdUIRenderCamera
============
*/
class sdUIRenderCamera :
	public sdUIRenderWorld_Child {
public:
	SD_UI_DECLARE_CLASS( sdUIRenderCamera )

						sdUIRenderCamera();
	virtual				~sdUIRenderCamera();

	virtual const char*	GetScopeClassName() const { return "sdUIRenderCamera"; }

	virtual void		Update( renderView_t& viewDef );

private:
	void				CalcFov( renderView_t& viewDef ) const;

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCamera/CameraOrigin";
	desc				= "Light color.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		cameraOrigin;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCamera/CameraRotation";
	desc				= "Camera rotation.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		cameraRotation;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCamera/Fov";
	desc				= "Camera field of view.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		fov;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCamera/AspectRatio";
	desc				= "Camera aspect ratio.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		aspectRatio;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCamera/Active";
	desc				= "Camera is active.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		active;
	// ===========================================

private:
};

class idCamera_MD5;
/*
============
sdUIRenderCamera_Animated
============
*/
class sdUIRenderCamera_Animated :
	public sdUIRenderWorld_Child {
public:
	SD_UI_DECLARE_CLASS( sdUIRenderCamera_Animated )

						sdUIRenderCamera_Animated();
	virtual				~sdUIRenderCamera_Animated();

	virtual const char*	GetScopeClassName() const { return "sdUIRenderCamera_Animated"; }

	virtual void		Update( renderView_t& viewDef );

private:
	void				OnCameraAnimChanged( const idStr& oldValue, const idStr& newValue );
	void				OnCameraOriginChanged( const idVec3& oldValue, const idVec3& newValue );
	void				OnCycleChanged( const float oldValue, const float newValue );
	
protected:
	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCameraAnimated/CameraAnim";
	desc				= "Camera animation. Set to a md5camera model";
	editor				= "edit";
	datatype			= "string";
	)
	sdStringProperty	cameraAnim;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCameraAnimated/Active";
	desc				= "Camera is active.";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty		active;
	// ===========================================

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/RenderCameraAnimated/CameraOrigin";
	desc				= "Camera animation path will be translated by this vector.";
	editor				= "edit";
	datatype			= "vec3";
	)
	sdVec3Property		cameraOrigin;
	// ===========================================

	sdFloatProperty		cycle;
private:
	idCamera_MD5		camera;
};

#endif // ! __GAME_GUIS_USERINTERFACERENDERWORLD_H__
