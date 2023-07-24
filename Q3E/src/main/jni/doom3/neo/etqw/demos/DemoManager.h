// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_DEMOS_DEMOMANAGER_H__
#define __GAME_DEMOS_DEMOMANAGER_H__

#include "../guis/UserInterfaceTypes.h"
#include "DemoAnalyzer.h"

class sdDemoCamera;
class sdDemoScript;

class sdDemoProperties : public sdUIPropertyHolder {
public:
												sdDemoProperties() {}
												~sdDemoProperties() {}

	virtual sdProperties::sdProperty*			GetProperty( const char* name );
	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler&	GetProperties() { return properties; }
	virtual const char*							FindPropertyName( sdProperties::sdProperty* property, sdUserInterfaceScope*& scope ) { scope = this; return properties.NameForProperty( property ); }

	virtual const char*							GetName() const { return "demoProperties"; }

	void										Init();
	void										Shutdown();
	void										UpdateProperties();

private:
	sdFloatProperty		state;

	sdFloatProperty		time;
	sdFloatProperty		size;
	sdFloatProperty		position;

	sdFloatProperty		frame;

	sdFloatProperty		cutIsSet;
	sdFloatProperty		cutStartMarker;
	sdFloatProperty		cutEndMarker;

    sdStringProperty	demoName;
	sdFloatProperty		writingMDF;
	sdStringProperty	mdfName;

	sdVec3Property		viewOrigin;
	sdVec3Property		viewAngles;

	sdProperties::sdPropertyHandler				properties;
};

class sdDemoManagerLocal {
public:
	typedef enum demoState_e {
		DS_NONE = -1,
		DS_RECORDING = 0,
		DS_PLAYING = 1,
		DS_PAUSED = 2
	} demoState_t;

										sdDemoManagerLocal() { script = NULL; melDataFile = NULL; }
										~sdDemoManagerLocal();

	void								Init();
	void								InitGUIs();
	void								Shutdown();

	bool								InPlayBack( void ) const { return state == DS_PLAYING || state == DS_PAUSED; }

	demoState_t							GetState() const { return state; }
	int									GetTime() const { return time; }
	int									GetPreviousTime() const { return previousTime; }

	int									GetSize() const { return position; }
	float								GetPosition() const;
	int									GetLength() const { return length; }

	int									GetFrame() const { return demoGameFrames; }

	bool								CutIsSet() const { return cutStartMarker == -1 || cutEndMarker == -1 ? false : true; }
	float								GetCutStartMarker() const;
	float								GetCutEndMarker() const;

	sdUIPropertyHolder&					GetProperties() { return localDemoProperties; }
	guiHandle_t							GetHudHandle() { return hud; }

	void								StartDemo();
	void								EndDemo();

	void								RunDemoFrame( const usercmd_t* demoCmd );
	void								EndDemoFrame();

	void								SetActiveCamera( sdDemoCamera* camera );

	bool								CalculateRenderView( renderView_t* renderView );

	void								SetRenderedView( const renderView_t& renderView ) { renderedView = renderView; }
	const renderView_t&					GetRenderedView() { return renderedView; }

	sdDemoCamera*						CreateCamera( const char* type ) { return cameraFactory.CreateType( type ); }

	bool								NoClip() { return ( ( state == DS_PAUSED || ( state == DS_PLAYING && demo_noclip.GetBool() ) ) && !g_showDemoView.GetBool() ); }

	bool								WritingMDF() const { return melDataFile ? true : false; }
	const idStr&						GetMDFFileName() const { return melDataFileName; }

	sdDemoScript*						GetScript() const { return script; }

	sdDemoAnalyzer&						GetDemoAnalyzer() { return demoAnalyzer; }

	const usercmd_t&					GetDemoCommand() const { return demoCmd; }

public:
	static idCVar						g_showDemoHud;

private:
	typedef sdFactory< sdDemoCamera >	sdCameraFactory;
	struct melPrimitive_t {
		bool		visible;
		idBounds	bounds;
	};

private:
	int									demoGameFrames;

	int									pausedTime;
	int									previousPausedTime;

	demoState_t							state;
	int									time;
	int									previousTime;
	int									position;
	int									length;
	int									startPosition;
	int									endPosition;

	int									cutStartMarker;
	int									cutEndMarker;

	sdDemoScript*						script;
	sdDemoCamera*						activeCamera;

	// moveable camera
	usercmd_t							demoCmd;

	idVec3								viewOrigin;
	idAngles							viewAngles;
	idAngles							deltaViewAngles;
	idVec3								currentVelocity;

	renderView_t						renderedView;

	sdCameraFactory						cameraFactory;

	// conversion to MEL
	static idCVar						g_demoOutputMDF;

	idStr								melDataFileName;
	idFile*								melDataFile;
	idList<melPrimitive_t*>				melPrimitives;
	int									melFrames;

	// hud
	sdDemoProperties					localDemoProperties;

	guiHandle_t							hud;

	// demo analysis
	sdDemoAnalyzer						demoAnalyzer;

	static idCVar						g_showDemoView;
	static idCVar						g_demoAnalyze;
	static idCVar						demo_noclip;
};

typedef sdSingleton< sdDemoManagerLocal > sdDemoManager;

#endif // __GAME_DEMOS_DEMOMANAGER_H__
