// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __SCRIPT_SYSCALL_H__
#define __SCRIPT_SYSCALL_H__

#include "../CommandMapInfo.h"

extern const idEventDefInternal EV_Thread_Execute;

class sdSysCallThread : public sdProgramThread {
public:
	CLASS_PROTOTYPE( sdSysCallThread );

	void						Event_Execute( void );
	void						Event_SetThreadName( const char *name );

	//
	// script callable Events
	//
	void						Event_TerminateThread( int num );
	void						Event_Pause( void );
	void						Event_Wait( float time );
	void						Event_WaitFrame( void );
	void						Event_Print( const char *text );
	void						Event_PrintLn( const char *text );
	void						Event_StackTrace( void );
	void						Event_Assert( bool value );
	void						Event_Trigger( idEntity *ent );
	void						Event_SetCvar( const char *name, const char *value ) const;
	void						Event_GetCVar( const char* name, const char* defaultValue ) const;
	void						Event_Random( float range ) const;
	void						Event_GetTime( void );
	void						Event_ToGuiTime( float time ) const;
	void						Event_KillThread( const char *name );
	void						Event_GetEntity( const char *name );
	void						Event_GetEntityByID( const char* hexSpawnID );
	void						Event_Spawn( const char *classname );
	void						Event_SpawnClient( const char *classname );
	void						Event_SpawnByType( int index );
	void						Event_AngToForward( idAngles &ang );
	void						Event_AngToRight( idAngles &ang );
	void						Event_AngToUp( idAngles &ang );
	void						Event_GetSine( float angle );
	void						Event_GetCosine( float angle );
	void						Event_ArcTan( float a );
	void						Event_ArcTan2( float a1, float a2 );
	void						Event_GetArcCosine( float dot );
	void						Event_ArcSine( float a );
	void						Event_ReturnRoot( int index );
	void						Event_SolveRoots( float a, float b, float c );
	void						Event_AngleNormalize180( float angle );
	void						Event_Fabs( float value );
	void						Event_Floor( float value );
	void						Event_Ceil( float value );
	void						Event_Mod( int value, int mod );
	void						Event_Fmod( float value, float mod );
	void						Event_GetSquareRoot( float theSquare );
	void						Event_VecNormalize( idVec3 &vec );
	void						Event_VecLength( const idVec3 &vec );
	void						Event_VecLengthSquared( const idVec3& vec );
	void						Event_VecCrossProduct( const idVec3 &vec1, const idVec3 &vec2 );
	void						Event_VecToAngles( const idVec3 &vec );
	void						Event_RotateVecByAngles( const idVec3& vec, const idAngles& angles );
	void						Event_RotateAngles( const idAngles& angles1, const idAngles& angles2 );
	void						Event_RotateVec( const idVec3& vec, const idVec3& axis, float angle );
	void						Event_ToLocalSpace( const idVec3& vec, idEntity* ent );
	void						Event_ToWorldSpace( const idVec3& vec, idEntity* ent );

	void						Event_CheckContents( const idVec3 &start, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idScriptObject* passObject );
	void						Event_Trace( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idScriptObject* passObject );
	void						Event_TracePoint( const idVec3 &start, const idVec3 &end, int contents_mask, idScriptObject* passObject );
	void						Event_TraceOriented( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, const idVec3& angles, int contents_mask, idScriptObject* passObject );
	void						Event_SaveTrace( void );
	void						Event_FreeTrace( idScriptObject* object );

	void						Event_GetTraceFraction( void );
	void						Event_GetTraceEndPos( void );
	void						Event_GetTracePoint( void );
	void						Event_GetTraceNormal( void );
	void						Event_GetTraceEntity( void );
	void						Event_GetTraceSurfaceFlags( void );
	void						Event_GetTraceSurfaceType( void );
	void						Event_GetTraceSurfaceColor( void );
	void						Event_GetTraceJoint( void );
	void						Event_GetTraceBody( void );

	void						Event_SetShaderParm( int parmnum, float value );
	void						Event_StartMusic( const char *name );
	void						Event_StartSoundDirect( const char* shader, int channel );
	void						Event_Warning( const char *text );
	void						Event_Error( const char *text );
	void 						Event_StrLen( const char *string );
	void 						Event_StrLeft( const char *string, int num );
	void 						Event_StrRight( const char *string, int num );
	void 						Event_StrSkip( const char *string, int num );
	void 						Event_StrMid( const char *string, int start, int num );
	void						Event_StrToFloat( const char *string );
	void						Event_IsClient( void );
	void						Event_IsServer( void );
	void						Event_DoClientSideStuff( void );
	void						Event_IsNewFrame( void );
	void 						Event_GetFrameTime( void );
	void 						Event_GetTicsPerSecond( void );
	void						Event_BroadcastToolTip( int index, idEntity* other, const wchar_t* string1, const wchar_t* string2, const wchar_t* string3, const wchar_t* string4 );
	void						Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime );
	void						Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime );
	void						Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime );
	void						Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime );
	void						Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime );

	void						Event_GetDeclTypeHandle( const char* declTypeName );
	void						Event_GetDeclIndex( int declTypeHandle, const char* declName );
	void						Event_GetDeclName( int declTypeHandle, int index );
	void						Event_ApplyRadiusDamage( const idVec3& origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, idEntity *ignorePush, int damageIndex, float damagePower, float radiusScale );
	void						Event_FilterEntity( int filterIndex, idEntity* entity );
	void						Event_GetTableCount( int tableIndex );
	void						Event_GetTableValue( int tableIndex, int valueIndex );
	void						Event_GetTableValueExact( int tableIndex, float valueIndex );
	void						Event_GetTypeHandle( const char* typeName );
	void						Event_GetDeclCount( int declTypeHandle );
	void						Event_Argc( void );
	void						Event_Argv( int index );
	void						Event_FloatArgv( int index );
	void						Event_SetActionCommand( const char* message );
	void						Event_GetTeamName( int index );
	void						Event_GetTeam( const char* name );
	void						Event_PlayWorldEffect( const char *effectName, const idVec3& color, const idVec3& org, const idVec3& angle );
	void						Event_PlayWorldEffectRotate( const char *effectName, const idVec3& color, const idVec3& org, const idVec3& angle, const idAngles& rot );
	void						Event_PlayWorldEffectRotateAlign( const char *effectName, const idVec3& color, const idVec3& org, const idVec3& angle, const idAngles& rot, idEntity *ent );
	void						Event_PlayWorldBeamEffect( const char *effectName, const idVec3& color, const idVec3& org, const idVec3& endOrigin );
	void						Event_GetLocalPlayer( void );
	void						Event_GetLocalViewPlayer( void );

	void						Event_SetGUIFloat( int handle, const char* name, float value );
	void						Event_SetGUIInt( int handle, const char* name, int value );
	void						Event_SetGUIVec2( int handle, const char* name, float x, float y );
	void						Event_SetGUIVec3( int handle, const char* name, float x, float y, float z );
	void						Event_SetGUIVec4( int handle, const char* name, float x, float y, float z, float w );
	void						Event_SetGUIString( int handle, const char* name, const char* value );
	void						Event_SetGUIWString( int handle, const char* name, const wchar_t* value );
	void						Event_SetGUITheme( guiHandle_t handle, const char* theme );
	void						Event_AddNotifyIcon( guiHandle_t handle, const char* window, const char* material );
	void						Event_RemoveNotifyIcon( guiHandle_t handle, const char* window, int iconHandle );
	void						Event_BumpNotifyIcon( guiHandle_t handle, const char* window, int iconHandle );

	void						Event_GUIPostNamedEvent( int handle, const char* window, const char* name );
	void						Event_GetGUIFloat( int handle, const char* name );
	void						Event_ClearDeployRequest( int deployIndex );
	void						Event_GetDeployMask( const char* maskname );
	void						Event_CheckDeployMask( const idVec3& mins, const idVec3& maxs, int handle );
	void						Event_GetWorldPlayZoneIndex( const idVec3& point );

	void						Event_SetTargetTimerValue( qhandle_t handle, idEntity* entity, float t );
	void						Event_GetTargetTimerValue( qhandle_t handle, idEntity* entity );
	void						Event_AllocTargetTimer( const char* targetName );

	void						Event_AllocCMIcon( idEntity* owner, int sort );
	void						Event_FreeCMIcon( idEntity* owner, qhandle_t handle );
	void						Event_SetCMIconSize( qhandle_t handle, float size );
	void						Event_SetCMIconUnknownSize( qhandle_t handle, float size );
	void						Event_SetCMIconSize2d( qhandle_t handle, float sizeX, float sizeY );
	void						Event_SetCMIconUnknownSize2d( qhandle_t handle, float sizeX, float sizeY );
	void						Event_SetCMIconSizeMode( qhandle_t handle, sdCommandMapInfo::scaleMode_t mode );
	void						Event_SetCMIconPositionMode( qhandle_t handle, sdCommandMapInfo::positionMode_t mode );
	void						Event_SetCMIconColor( qhandle_t handle, const idVec3& color, float alpha );
	void						Event_SetCMIconColorMode( qhandle_t handle, sdCommandMapInfo::colorMode_t mode );
	void						Event_SetCMIconDrawMode( qhandle_t handle, sdCommandMapInfo::drawMode_t mode );
	void						Event_SetCMIconAngle( qhandle_t handle, float angle );
	void						Event_SetCMIconSides( qhandle_t handle, int sides );
	void						Event_HideCMIcon( qhandle_t handle );
	void						Event_ShowCMIcon( qhandle_t handle );
	void						Event_AddCMIconRequirement( qhandle_t handle, const char* requirement );
	void						Event_SetCMIconMaterial( qhandle_t handle, int materialIndex );
	void						Event_SetCMIconUnknownMaterial( qhandle_t handle, int materialIndex );
	void						Event_SetCMIconFireteamMaterial( qhandle_t handle, int materialIndex );
	void						Event_SetCMIconGuiMessage( qhandle_t handle, const char* message );
	void						Event_SetCMIconFlag( qhandle_t handle, int flag );
	void						Event_ClearCMIconFlag( qhandle_t handle, int flag );
	void						Event_SetCMIconArcAngle( qhandle_t handle, float angle );
	void						Event_SetCMIconShaderParm( qhandle_t handle, int index, float value );
	void						Event_SetCMIconOrigin( qhandle_t handle, const idVec3& origin );
	void						Event_GetCMIconFlags( qhandle_t handle );
	void						Event_FlashCMIcon( qhandle_t handle, int materialIndex, float seconds, int setFlags );

	void						Event_GetEntityDefKey( int index, const char* key );
	void						Event_GetEntityDefIntKey( int index, const char* key );
	void						Event_GetEntityDefFloatKey( int index, const char* key );
	void						Event_GetEntityDefVectorKey( int index, const char* key );

	void						Event_AllocDecal( const char* material );
	void 						Event_ProjectDecal( qhandle_t handle, const idVec3& origin, const idVec3& direction, float depth, bool parallel, const idVec3& size, float angle, const idVec3& color );
	void 						Event_ResetDecal( qhandle_t handle );
	void 						Event_FreeDecal( qhandle_t handle );

	void						Event_AllocHudModule( const char* name, int sort, bool allowInhibit );
	void						Event_FreeHudModule( int handle );

	void						Event_RequestDeployment( idEntity* other, int deploymentObjectIndex, const idVec3& position, float yaw, float extraDelay );
	void						Event_RequestCheckedDeployment( idEntity* other, int deploymentObjectIndex, float yaw, float extraDelay );

	void						Event_GetWorldMins( void );
	void						Event_GetWorldMaxs( void );

	void						Event_SetDeploymentObject( int index );
	void						Event_SetDeploymentState( deployResult_t state );
	void						Event_SetDeploymentMode( bool mode );
	void						Event_GetDeploymentMode( void );
	void						Event_GetDeploymentRotation( void );
	void						Event_AllowDeploymentRotation( void );

	void						Event_GetDefaultFov( void );
	void						Event_GetTerritory( const idVec3& point, idScriptObject* object, bool requireTeam, bool requireActive );

	void						Event_GetMaxClients( void );
	void						Event_GetClient( int index );

	void						Event_GetDeployObjectCategory( int index );
	void						Event_GetDeployObjectDistanceLimit( int index );
	void						Event_GetDeployCategoryDistanceLimit( int index );

	void						Event_HandleToString( const int handle );
	void						Event_StringToHandle( const char* str );

	void						Event_ToWideString( const char* str );

	void						Event_PushLocalizationString( const wchar_t* str );
	void						Event_PushLocalizationStringIndex( const int index );
	void						Event_LocalizeStringIndexArgs( const int index );
	void						Event_LocalizeStringArgs( const char* str );
	void						Event_LocalizeString( const char* str );
	
	void						Event_GetMatchTimeRemaining( void );
	void						Event_GetMatchState( void );

	void						Event_CreateMaskEditSession( void );

	void						Event_GetStat( const char* name );
	void 						Event_AllocStatInt( const char* name );
	void 						Event_AllocStatFloat( const char* name );
	void 						Event_IncreaseStatInt( int handle, int playerIndex, int count );
	void 						Event_IncreaseStatFloat( int handle, int playerIndex, float count );
	void						Event_GetStatValue( int handle, int playerIndex );
	void						Event_GetStatDelta( int handle, int playerIndex );
	void						Event_SetStatBaseLine( int playerIndex );

	void						Event_GetClimateSkin( const char* key );

	void						Event_SendQuickChat( idEntity* sender, int quickChatIndex, idEntity* other );
	void						Event_GetContextEntity();

	void						Event_SetEndGameStatValue( int statIndex, idEntity* ent, float value );
	void						Event_SetEndGameStatWinner( int statIndex, idEntity* ent );
	void						Event_AllocEndGameStat( void );
	void						Event_SendEndGameStats( void );

	void						Event_HeightMapTrace( const idVec3& start, const idVec3& end );

	void						Event_EnableBotReachability( const char *name, int team, bool enable );

	void						Event_GetNextBotActionIndex( int startIndex, int type );
	void						Event_GetBotActionOrigin( int index );
	void						Event_GetBotActionDeployableType( int index );
	void						Event_GetBotActionBaseGoalType( int index, idScriptObject* obj );

	void						Event_EnablePlayerHeadModels( void );
	void						Event_DisablePlayerHeadModels( void );

protected:
	static trace_t				trace;
	static float				cachedRoot[ 2 ];
	static idWStrList			localizationStrings;
};

#endif // __SCRIPT_SYSCALL_H__
