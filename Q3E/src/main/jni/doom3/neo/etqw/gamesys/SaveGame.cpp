// Copyright (C) 2004 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TypeInfo.h"

#include "../../bse/BSEInterface.h"
#include "../../bse/BSE_Envelope.h"
#include "../../bse/BSE_SpawnDomains.h"
#include "../../bse/BSE_Particle.h"
#include "../../bse/BSE.h"

#include "../physics/Clip.h"
#include "../../framework/BuildVersion.h"
#include "../Entity.h"
#include "../structures/TeamManager.h"

/*
Save game related helper classes.

Save games are implemented in two classes, idSaveGame and idRestoreGame, that implement write/read functions for 
common types.  They're passed in to each entity and object for them to archive themselves.  Each class
implements save/restore functions for it's own data.  When restoring, all the objects are instantiated,
then the restore function is called on each, superclass first, then subclasses.

Pointers are restored by saving out an object index for each unique object pointer and adding them to a list of
objects that are to be saved.  Restore instantiates all the objects in the list before calling the Restore function
on each object so that the pointers returned are valid.  No object's restore function should rely on any other objects
being fully instantiated until after the restore process is complete.  Post restore fixup should be done by posting
events with 0 delay.

The savegame header will have the Game Name, Version, Map Name, and Player Persistent Info.

Changes in version make savegames incompatible, and the game will start from the beginning of the level with
the player's persistent info.

Changes to classes that don't need to break compatibilty can use the build number as the savegame version.
Later versions are responsible for restoring from previous versions by ignoring any unused data and initializing
variables that weren't in previous versions with safe information.

At the head of the save game is enough information to restore the player to the beginning of the level should the
file be unloadable in some way (for example, due to script changes).
*/

/*
================
idSaveGame::idSaveGame()
================
*/
idSaveGame::idSaveGame( idFile *savefile ) {

	file = savefile;

	// Put NULL at the start of the list so we can skip over it.
	objects.Clear();
	objects.Append( NULL );
}

/*
================
idSaveGame::~idSaveGame()
================
*/
idSaveGame::~idSaveGame() {
	if ( objects.Num() ) {
		Close();
	}
}

/*
================
idSaveGame::Close
================
*/
void idSaveGame::Close( void ) {
	int i;

	WriteSoundCommands();

	for( i = 1; i < objects.Num(); i++ ) {
		CallSave_r( objects[ i ]->GetType(), objects[ i ] );
	}

	objects.Clear();

/*#ifdef ID_DEBUG_MEMORY
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ), false ) );
#endif*/
}

/*
================
idSaveGame::WriteObjectList
================
*/
void idSaveGame::WriteObjectList( void ) {
	int i;

	WriteInt( objects.Num() - 1 );
	for( i = 1; i < objects.Num(); i++ ) {
		WriteString( objects[ i ]->GetClassname() );
	}
}

/*
================
idSaveGame::CallSave_r
================
*/
void idSaveGame::CallSave_r( const idTypeInfo *cls, const idClass *obj ) {
	if ( cls->super ) {
		CallSave_r( cls->super, obj );
		if ( cls->super->Save == cls->Save ) {
			// don't call save on this inheritance level since the function was called in the super class
			return;
		}
	}
	
	( obj->*cls->Save )( this );
}

/*
================
idSaveGame::AddObject
================
*/
void idSaveGame::AddObject( const idClass *obj ) {
	objects.AddUnique( obj );
}

/*
================
idSaveGame::Write
================
*/
void idSaveGame::Write( const void *buffer, int len ) {
	file->Write( buffer, len );
}

/*
================
idSaveGame::WriteInt
================
*/
void idSaveGame::WriteInt( const int value ) {
	file->WriteInt( value );
}

/*
================
idSaveGame::WriteUnsignedInt
================
*/
void idSaveGame::WriteUnsignedInt( const unsigned int value ) {
	file->WriteUnsignedInt( value );
}

/*
================
idSaveGame::WriteJoint
================
*/
void idSaveGame::WriteJoint( const jointHandle_t value ) {
	file->WriteInt( (int&)value );
}

/*
================
idSaveGame::WriteShort
================
*/
void idSaveGame::WriteShort( const short value ) {
	file->WriteShort( value );
}

/*
================
idSaveGame::WriteByte
================
*/
void idSaveGame::WriteByte( const byte value ) {
	file->Write( &value, sizeof( value ) );
}

/*
================
idSaveGame::WriteSignedChar
================
*/
void idSaveGame::WriteSignedChar( const signed char value ) {
	file->Write( &value, sizeof( value ) );
}

/*
================
idSaveGame::WriteFloat
================
*/
void idSaveGame::WriteFloat( const float value ) {
	file->WriteFloat( value );
}

/*
================
idSaveGame::WriteBool
================
*/
void idSaveGame::WriteBool( const bool value ) {
	file->WriteBool( value );
}

/*
================
idSaveGame::WriteString
================
*/  
void idSaveGame::WriteString( const char *string ) {
	int len;

	len = idStr::Length( string );
	WriteInt( len );
    file->Write( string, len );
}

/*
================
idSaveGame::WriteVec2
================
*/
void idSaveGame::WriteVec2( const idVec2 &vec ) {
	file->WriteVec2( vec );
}

/*
================
idSaveGame::WriteVec3
================
*/
void idSaveGame::WriteVec3( const idVec3 &vec ) {
	file->WriteVec3( vec );
}

/*
================
idSaveGame::WriteVec4
================
*/
void idSaveGame::WriteVec4( const idVec4 &vec ) {
	file->WriteVec4( vec );
}

/*
================
idSaveGame::WriteVec6
================
*/
void idSaveGame::WriteVec6( const idVec6 &vec ) {
	file->WriteVec6( vec );
}

/*
================
idSaveGame::WriteBounds
================
*/
void idSaveGame::WriteBounds( const idBounds &bounds ) {
	idBounds b = bounds;
	LittleRevBytes( &b, sizeof(float), sizeof(b)/sizeof(float) );
	file->Write( &b, sizeof( b ) );
}

/*
================
idSaveGame::WriteBounds
================
*/
void idSaveGame::WriteWinding( const idWinding &w )
{
	int i, num;
	num = w.GetNumPoints();
	file->WriteInt( num );
	for ( i = 0; i < num; i++ ) {
		idVec5 v = w[i];
		LittleRevBytes(&v, sizeof(float), sizeof(v)/sizeof(float) );
		file->Write( &v, sizeof(v) );
	}
}


/*
================
idSaveGame::WriteMat3
================
*/
void idSaveGame::WriteMat3( const idMat3 &mat ) {
	file->WriteMat3( mat );
}

/*
================
idSaveGame::WriteAngles
================
*/
void idSaveGame::WriteAngles( const idAngles &angles ) {
	idAngles v = angles;
	LittleRevBytes(&v, sizeof(float), sizeof(v)/sizeof(float) );
	file->Write( &v, sizeof( v ) );
}

/*
================
idSaveGame::WriteObject
================
*/
void idSaveGame::WriteObject( const idClass *obj ) {
	int index;

	index = objects.FindIndex( obj );
	if ( index < 0 ) {
		gameLocal.DPrintf( "idSaveGame::WriteObject - WriteObject FindIndex failed\n" );

		// Use the NULL index
		index = 0;
	}

	WriteInt( index );
}

/*
================
idSaveGame::WriteStaticObject
================
*/
void idSaveGame::WriteStaticObject( const idClass &obj ) {
	CallSave_r( obj.GetType(), &obj );
}

/*
================
idSaveGame::WriteDict
================
*/
void idSaveGame::WriteDict( const idDict *dict ) {
	int num;
	int i;
	const idKeyValue *kv;

	if ( !dict ) {
		WriteInt( -1 );
	} else {
		num = dict->GetNumKeyVals();
		WriteInt( num );
		for( i = 0; i < num; i++ ) {
			kv = dict->GetKeyVal( i );
			WriteString( kv->GetKey() );
			WriteString( kv->GetValue() );
		}
	}
}

/*
================
idSaveGame::WriteMaterial
================
*/
void idSaveGame::WriteMaterial( const idMaterial *material ) {
	if ( !material ) {
		WriteString( "" );
	} else {
		WriteString( material->GetName() );
	}
}

/*
================
idSaveGame::WriteSkin
================
*/
void idSaveGame::WriteSkin( const idDeclSkin *skin ) {
	if ( !skin ) {
		WriteString( "" );
	} else {
		WriteString( skin->GetName() );
	}
}

/*
================
idSaveGame::WriteEffect
================
*/
void idSaveGame::WriteEffect( const rvDeclEffect *fx ) {
	if ( !fx ) {
		WriteString( "" );
	} else {
		WriteString( fx->GetName() );
	}
}

/*
================
idSaveGame::WriteModelDef
================
*/
void idSaveGame::WriteModelDef( const idDeclModelDef *modelDef ) {
	if ( !modelDef ) {
		WriteString( "" );
	} else {
		WriteString( modelDef->GetName() );
	}
}

/*
================
idSaveGame::WriteVehicleScript
================
*/
void idSaveGame::WriteVehicleScript( const sdDeclVehicleScript *vehicleScript ) {
	if ( !vehicleScript ) {
		WriteString( "" );
	} else {
		WriteString( vehicleScript->GetName() );
	}
}

/*
================
idSaveGame::WriteInvItem
================
*/
void idSaveGame::WriteInvItem( const sdDeclInvItem *invItem ) {
	if ( !invItem ) {
		WriteString( "" );
	} else {
		WriteString( invItem->GetName() );
	}
}

/*
================
idSaveGame::WritePlayerClass
================
*/
void idSaveGame::WritePlayerClass( const sdDeclPlayerClass *playerClass ) {
	if ( !playerClass ) {
		WriteString( "" );
	} else {
		WriteString( playerClass->GetName() );
	}
}

/*
================
idSaveGame::WriteTable
================
*/
void idSaveGame::WriteTable( const idDeclTable *table ) {
	if ( !table ) {
		WriteString( "" );
	} else {
		WriteString( table->GetName() );
	}
}

/*
================
idSaveGame::WriteDamage
================
*/
void idSaveGame::WriteDamage( const sdDeclDamage *damage ) {
	if ( !damage ) {
		WriteString( "" );
	} else {
		WriteString( damage->GetName() );
	}
}

/*
================
idSaveGame::WriteToolTip
================
*/
void idSaveGame::WriteToolTip( const sdDeclToolTip *toolTip ) {
	if ( !toolTip ) {
		WriteString( "" );
	} else {
		WriteString( toolTip->GetName() );
	}
}

/*
================
idSaveGame::WriteCampaign
================
*/
void idSaveGame::WriteCampaign( const sdDeclCampaign *campaign ) {
	if ( !campaign ) {
		WriteString( "" );
	} else {
		WriteString( campaign->GetName() );
	}
}

/*
================
idSaveGame::WriteSoundShader
================
*/
void idSaveGame::WriteSoundShader( const idSoundShader *shader ) {
	const char *name;

	if ( !shader ) {
		WriteString( "" );
	} else {
		name = shader->GetName();
		WriteString( name );
	}
}

/*
================
idSaveGame::WriteModel
================
*/
void idSaveGame::WriteModel( const idRenderModel *model ) {
	const char *name;

	if ( !model ) {
		WriteString( "" );
	} else {
		name = model->Name();
		WriteString( name );
	}
}

/*
================
idSaveGame::WriteUserInterface
================
*/
void idSaveGame::WriteUserInterface( guiHandle_t guiHandle ) {
	// Gordon: GUI FIXME
/*	const char *name;

	if ( !ui ) {
		WriteString( "" );
	} else {
		name = ui->Name();
		WriteString( name );
		WriteBool( unique->IsUniqued() );
		if ( ui->WriteToSaveGame( file ) == false ) {
			gameLocal.Error( "idSaveGame::WriteUserInterface: ui failed to write properly\n" );
		}
	}*/
}

/*
================
idSaveGame::WriteRenderEntity
================
*/
void idSaveGame::WriteRenderEntity( const renderEntity_t &renderEntity ) {
	int i;

	WriteModel( renderEntity.hModel );

	WriteInt( renderEntity.entityNum );
	WriteInt( renderEntity.bodyId );

	WriteBounds( renderEntity.bounds );

	// callback is set by class's Restore function

	WriteInt( renderEntity.suppressSurfaceInViewID );
	WriteInt( renderEntity.suppressShadowInViewID );
	WriteInt( renderEntity.suppressShadowInLightID );
	WriteInt( renderEntity.allowSurfaceInViewID );

	WriteVec3( renderEntity.origin );
	WriteMat3( renderEntity.axis );

	WriteMaterial( renderEntity.customShader );
	WriteMaterial( renderEntity.referenceShader );
	WriteSkin( renderEntity.customSkin );

	if ( renderEntity.referenceSound != NULL ) {
		WriteInt( renderEntity.referenceSound->Index() );
	} else {
		WriteInt( 0 );
	}

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		WriteFloat( renderEntity.shaderParms[ i ] );
	}

	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		WriteUserInterface( renderEntity.gui[ i ] );
	}

	WriteFloat( renderEntity.modelDepthHack );
	WriteFloat( renderEntity.groundRadiosity );

	WriteBool( renderEntity.noSelfShadow );
	WriteBool( renderEntity.noShadow );
	WriteBool( renderEntity.noDynamicInteractions );
	WriteBool( renderEntity.weaponDepthHack );

	WriteInt( renderEntity.forceUpdate );
	WriteInt( renderEntity.forceOutside );

	WriteInt( renderEntity.maxVisDist );

	for ( i = 0; i < MAX_SURFACE_BITS; i++ ) {
		WriteInt( renderEntity.hideSurfaceMask.Get( i ) );
	}

	WriteFloat( renderEntity.groundRadiosity );
}

/*
================
idSaveGame::WriteRenderLight
================
*/
void idSaveGame::WriteRenderLight( const renderLight_t &renderLight ) {
	int i;

	WriteMat3( renderLight.axis );
	WriteVec3( renderLight.origin );

	WriteInt( renderLight.suppressLightInViewID );
	WriteInt( renderLight.allowLightInViewID );
	WriteBool( renderLight.flags.noShadows );
	WriteBool( renderLight.flags.noSpecular );
	WriteBool( renderLight.flags.pointLight );
	WriteBool( renderLight.flags.parallel );
	WriteBool( renderLight.flags.atmosphereLight );

	WriteVec3( renderLight.lightRadius );
	WriteVec3( renderLight.lightCenter );

	WriteVec3( renderLight.target );
	WriteVec3( renderLight.right );
	WriteVec3( renderLight.up );
	WriteVec3( renderLight.start );
	WriteVec3( renderLight.end );

	// only idLight has a prelightModel and it's always based on the entityname, so we'll restore it there
	// WriteModel( renderLight.prelightModel );

	WriteInt( renderLight.lightId );

	WriteMaterial( renderLight.shader );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		WriteFloat( renderLight.shaderParms[ i ] );
	}

	if ( renderLight.referenceSound != NULL ) {
		WriteInt( renderLight.referenceSound->Index() );
	} else {
		WriteInt( 0 );
	}
}

/*
================
idSaveGame::WriteRenderEffect
================
*/
void idSaveGame::WriteRenderEffect( const renderEffect_t& renderEffect ) {
	int i;

	WriteEffect( reinterpret_cast< const rvDeclEffect* >( renderEffect.declEffect ) );

	WriteInt( renderEffect.suppressSurfaceInViewID );
	WriteInt( renderEffect.allowSurfaceInViewID );
	WriteInt( renderEffect.groupID );

	WriteVec3( renderEffect.origin );
	WriteMat3( renderEffect.axis );

	WriteVec3( renderEffect.endOrigin );

	WriteFloat( renderEffect.attenuation );
	WriteBool( renderEffect.hasEndOrigin );
	WriteBool( renderEffect.loop );
	WriteInt( renderEffect.weaponDepthHackInViewID );
	WriteFloat( renderEffect.modelDepthHack );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		WriteFloat( renderEffect.shaderParms[ i ] );
	}

	WriteVec3( renderEffect.windVector );
}

/*
================
idSaveGame::WriteRefSound
================
*/
void idSaveGame::WriteRefSound( const refSound_t& refSound ) {
	if ( refSound.referenceSound ) {
		WriteInt( refSound.referenceSound->Index() );
	} else {
		WriteInt( 0 );
	}
	WriteVec3( refSound.origin );
	WriteInt( refSound.listenerId );
	WriteSoundShader( refSound.shader );
	WriteFloat( refSound.diversity );
	WriteBool( refSound.waitfortrigger );

	WriteFloat( refSound.parms.minDistance );
	WriteFloat( refSound.parms.maxDistance );
	WriteFloat( refSound.parms.volume );
	WriteFloat( refSound.parms.shakes );
	WriteInt( refSound.parms.soundShaderFlags );
	WriteInt( refSound.parms.soundClass );
}

/*
================
idSaveGame::WriteRenderView
================
*/
void idSaveGame::WriteRenderView( const renderView_t &view ) {
	int i;

	WriteInt( view.viewID );
	WriteInt( view.x );
	WriteInt( view.y );
	WriteInt( view.width );
	WriteInt( view.height );

	WriteFloat( view.fov_x );
	WriteFloat( view.fov_y );
	WriteVec3( view.vieworg );
	WriteMat3( view.viewaxis );

	WriteBool( view.cramZNear );

	WriteInt( view.time );

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		WriteFloat( view.shaderParms[ i ] );
	}
}

/*
===================
idSaveGame::WriteUsercmd
===================
*/
void idSaveGame::WriteUsercmd( const usercmd_t &usercmd ) {
	WriteInt( usercmd.gameFrame );
	WriteInt( usercmd.gameTime );
	WriteInt( usercmd.duplicateCount );
	WriteShort( usercmd.buttons.btnValue );
	WriteSignedChar( usercmd.forwardmove );
	WriteSignedChar( usercmd.rightmove );
	WriteSignedChar( usercmd.upmove );
	WriteShort( usercmd.angles[0] );
	WriteShort( usercmd.angles[1] );
	WriteShort( usercmd.angles[2] );
	WriteSignedChar( usercmd.impulse );
	WriteByte( usercmd.flags );

	clientButtons_s buttons = usercmd.clientButtons;
	LittleBitField( &buttons, sizeof( buttons ) );
	file->Write( &buttons, sizeof( buttons ) );
}

/*
===================
idSaveGame::WriteContactInfo
===================
*/
void idSaveGame::WriteContactInfo( const contactInfo_t &contactInfo ) {
	WriteInt( (int)contactInfo.type );
	WriteVec3( contactInfo.point );
	WriteVec3( contactInfo.normal );
	WriteFloat( contactInfo.dist );
	WriteInt( contactInfo.contents );
	WriteMaterial( contactInfo.material );
	WriteInt( contactInfo.modelFeature );
	WriteInt( contactInfo.trmFeature );
	WriteInt( contactInfo.entityNum );
	WriteInt( contactInfo.id );
}

/*
===================
idSaveGame::WriteTrace
===================
*/
void idSaveGame::WriteTrace( const trace_t &trace ) {
	WriteFloat( trace.fraction );
	WriteVec3( trace.endpos );
	WriteMat3( trace.endAxis );
	WriteContactInfo( trace.c );
}

/*
 ===================
 idRestoreGame::WriteTraceModel
 ===================
 */
void idSaveGame::WriteTraceModel( const idTraceModel &trace ) {
	int j, k;
	
	WriteInt( (int&)trace.type );
	WriteInt( trace.numVerts );
	for ( j = 0; j < MAX_TRACEMODEL_VERTS; j++ ) {
		WriteVec3( trace.verts[j] );
	}
	WriteInt( trace.numEdges );
	for ( j = 0; j < (MAX_TRACEMODEL_EDGES+1); j++ ) {
		WriteInt( trace.edges[j].v[0] );
		WriteInt( trace.edges[j].v[1] );
		WriteVec3( trace.edges[j].normal );
	}
	WriteInt( trace.numPolys );
	for ( j = 0; j < MAX_TRACEMODEL_POLYS; j++ ) {
		WriteVec3( trace.polys[j].normal );
		WriteFloat( trace.polys[j].dist );
		WriteBounds( trace.polys[j].bounds );
		WriteInt( trace.polys[j].numEdges );
		for ( k = 0; k < MAX_TRACEMODEL_POLYEDGES; k++ ) {
			WriteInt( trace.polys[j].edges[k] );
		}
	}
	WriteVec3( trace.offset );
	WriteBounds( trace.bounds );
	WriteBool( trace.isConvex );
	// padding win32 native structs
	char tmp[3];
	memset( tmp, 0, sizeof( tmp ) );
	file->Write( tmp, 3 );
}

/*
===================
idSaveGame::WriteClipModel
===================
*/
void idSaveGame::WriteClipModel( const idClipModel *clipModel ) {
	if ( clipModel != NULL ) {
		WriteBool( true );
		clipModel->Save( this );
	} else {
		WriteBool( false );
	}
}

/*
===================
idSaveGame::WriteSoundCommands
===================
*/
void idSaveGame::WriteSoundCommands( void ) {
	gameSoundWorld->WriteToSaveGame( file );
}

/*
===================
idSaveGame::WriteTeamInfo
===================
*/
void idSaveGame::WriteTeamInfo( const sdTeamInfo* teamInfo ) {
	if ( teamInfo ) {
		WriteString( teamInfo->GetLookupName() );
	} else {
		WriteString( "" );
	}
}

/*
===================
idSaveGame::WriteFunction
===================
*/
void idSaveGame::WriteFunction( const function_t* function ) {
	if ( function ) {
		WriteString( function->Name() );
	} else {
		WriteString( "" );
	}
}

/*
======================
idSaveGame::WriteBuildNumber
======================
*/
void idSaveGame::WriteBuildNumber( const int value ) {
	assert( false );
	file->WriteInt( 1286 );
	//	file->WriteInt( BUILD_NUMBER );
}

/***********************************************************************

	idRestoreGame
	
***********************************************************************/

/*
================
idRestoreGame::RestoreGame
================
*/
idRestoreGame::idRestoreGame( idFile *savefile ) {
	file = savefile;
}

/*
================
idRestoreGame::~idRestoreGame()
================
*/
idRestoreGame::~idRestoreGame() {
}

/*
================
void idRestoreGame::CreateObjects
================
*/
void idRestoreGame::CreateObjects( void ) {
	int i, num;
	idStr classname;
	idTypeInfo *type;

	ReadInt( num );

	// create all the objects
	objects.SetNum( num + 1 );
	memset( objects.Ptr(), 0, sizeof( objects[ 0 ] ) * objects.Num() );

	for( i = 1; i < objects.Num(); i++ ) {
		ReadString( classname );
		type = idClass::GetClass( classname );
		if ( !type ) {
			Error( "idRestoreGame::CreateObjects: Unknown class '%s'", classname.c_str() );
		}
		objects[ i ] = type->CreateInstance();

/*#ifdef ID_DEBUG_MEMORY
		InitTypeVariables( objects[i], type->classname, 0xce );
#endif*/
	}
}

/*
================
void idRestoreGame::RestoreObjects
================
*/
void idRestoreGame::RestoreObjects( void ) {
	int i;

	ReadSoundCommands();

	// restore all the objects
	for( i = 1; i < objects.Num(); i++ ) {
		CallRestore_r( objects[ i ]->GetType(), objects[ i ] );
	}

	// regenerate render entities and render lights because are not saved
	for( i = 1; i < objects.Num(); i++ ) {
		if ( objects[ i ]->IsType( idEntity::Type ) ) {
			idEntity *ent = static_cast<idEntity *>( objects[ i ] );
			ent->UpdateVisuals();
			ent->Present();
		}
	}

/*#ifdef ID_DEBUG_MEMORY
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_restore", gameState.c_str() ), false ) );
	//CompareGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ) ) );
	gameLocal.Error( "dumped game states" );
#endif*/
}

/*
====================
void idRestoreGame::DeleteObjects
====================
*/
void idRestoreGame::DeleteObjects( void ) {

	// Remove the NULL object before deleting
	objects.RemoveIndex( 0 );

	objects.DeleteContents( true );
}

/*
================
idRestoreGame::Error
================
*/
void idRestoreGame::Error( const char *fmt, ... ) {
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	objects.DeleteContents( true );

	gameLocal.Error( "%s", text );
}

/*
================
idRestoreGame::CallRestore_r
================
*/
void idRestoreGame::CallRestore_r( const idTypeInfo *cls, idClass *obj ) {
	if ( cls->super ) {
		CallRestore_r( cls->super, obj );
		if ( cls->super->Restore == cls->Restore ) {
			// don't call save on this inheritance level since the function was called in the super class
			return;
		}
	}
	
	( obj->*cls->Restore )( this );
}

/*
================
idRestoreGame::Read
================
*/
void idRestoreGame::Read( void *buffer, int len ) {
	file->Read( buffer, len );
}

/*
================
idRestoreGame::ReadInt
================
*/
void idRestoreGame::ReadInt( int &value ) {
	file->ReadInt( value );
}

/*
================
idRestoreGame::ReadInt
================
*/
int idRestoreGame::ReadInt( void ) {
	int value;
	file->ReadInt( value );
	return value;
}

/*
================
idRestoreGame::ReadJoint
================
*/
void idRestoreGame::ReadJoint( jointHandle_t &value ) {
	file->ReadInt( (int&)value );
}

/*
================
idRestoreGame::ReadShort
================
*/
void idRestoreGame::ReadShort( short &value ) {
	file->ReadShort( value );
}

/*
================
idRestoreGame::ReadByte
================
*/
void idRestoreGame::ReadByte( byte &value ) {
	file->Read( &value, sizeof( value ) );
}

/*
================
idRestoreGame::ReadSignedChar
================
*/
void idRestoreGame::ReadSignedChar( signed char &value ) {
	file->Read( &value, sizeof( value ) );
}

/*
================
idRestoreGame::ReadFloat
================
*/
void idRestoreGame::ReadFloat( float &value ) {
	file->ReadFloat( value );
}

/*
================
idRestoreGame::ReadFloat
================
*/
float idRestoreGame::ReadFloat( void ) {
	float value;
	file->ReadFloat( value );
	return value;
}

/*
================
idRestoreGame::ReadBool
================
*/
void idRestoreGame::ReadBool( bool &value ) {
	file->ReadBool( value );
}

/*
================
idRestoreGame::ReadBool
================
*/
bool idRestoreGame::ReadBool( void ) {
	bool value;
	file->Read( &value, sizeof( value ) );
	return value;
}

/*
================
idRestoreGame::ReadString
================
*/
void idRestoreGame::ReadString( idStr &string ) {
	int len;

	ReadInt( len );
	if ( len < 0 ) {
		Error( "idRestoreGame::ReadString: invalid length" );
	}

	string.Fill( ' ', len );
	file->Read( &string[ 0 ], len );
}

/*
================
idRestoreGame::ReadVec2
================
*/
void idRestoreGame::ReadVec2( idVec2 &vec ) {
	file->ReadVec2( vec );
}

/*
================
idRestoreGame::ReadVec3
================
*/
void idRestoreGame::ReadVec3( idVec3 &vec ) {
	file->ReadVec3( vec );
}

/*
================
idRestoreGame::ReadVec4
================
*/
void idRestoreGame::ReadVec4( idVec4 &vec ) {
	file->ReadVec4( vec );
}

/*
================
idRestoreGame::ReadVec6
================
*/
void idRestoreGame::ReadVec6( idVec6 &vec ) {
	file->ReadVec6( vec );
}

/*
================
idRestoreGame::ReadBounds
================
*/
void idRestoreGame::ReadBounds( idBounds &bounds ) {
	file->Read( &bounds, sizeof( bounds ) );
	LittleRevBytes( &bounds, sizeof(float), sizeof(bounds)/sizeof(float) );
}

/*
================
idRestoreGame::ReadWinding
================
*/
void idRestoreGame::ReadWinding( idWinding &w )
{
	int i, num;
	file->ReadInt( num );
	w.SetNumPoints( num );
	for ( i = 0; i < num; i++ ) {
		file->Read( &w[i], sizeof(idVec5) );
		LittleRevBytes(&w[i], sizeof(float), sizeof(idVec5)/sizeof(float) );
	}
}

/*
================
idRestoreGame::ReadMat3
================
*/
void idRestoreGame::ReadMat3( idMat3 &mat ) {
	file->ReadMat3( mat );
}

/*
================
idRestoreGame::ReadAngles
================
*/
void idRestoreGame::ReadAngles( idAngles &angles ) {
	file->Read( &angles, sizeof( angles ) );
	LittleRevBytes(&angles, sizeof(float), sizeof(idAngles)/sizeof(float) );
}

/*
================
idRestoreGame::ReadObject
================
*/
void idRestoreGame::ReadObject( idClass *&obj ) {
	int index;

	ReadInt( index );
	if ( ( index < 0 ) || ( index >= objects.Num() ) ) {
		Error( "idRestoreGame::ReadObject: invalid object index" );
	}
	obj = objects[ index ];
}

/*
================
idRestoreGame::ReadStaticObject
================
*/
void idRestoreGame::ReadStaticObject( idClass &obj ) {
	CallRestore_r( obj.GetType(), &obj );
}

/*
================
idRestoreGame::ReadDict
================
*/
void idRestoreGame::ReadDict( idDict *dict ) {
	int num;
	int i;
	idStr key;
	idStr value;

	ReadInt( num );

	if ( num < 0 ) {
		dict = NULL;
	} else {
		dict->Clear();
		for( i = 0; i < num; i++ ) {
			ReadString( key );
			ReadString( value );
			dict->Set( key, value );
		}
	}
}

/*
================
idRestoreGame::ReadMaterial
================
*/
void idRestoreGame::ReadMaterial( const idMaterial *&material ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		material = NULL;
	} else {
		material = declManager->FindMaterial( name );
	}
}

/*
================
idRestoreGame::ReadSkin
================
*/
void idRestoreGame::ReadSkin( const idDeclSkin *&skin ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		skin = NULL;
	} else {
		skin = declManager->FindSkin( name );
	}
}

/*
================
idRestoreGame::ReadEffect
================
*/
void idRestoreGame::ReadEffect( const rvDeclEffect *&fx ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		fx = NULL;
	} else {
		fx = declManager->FindEffect( name );
	}
}

/*
================
idRestoreGame::ReadSoundShader
================
*/
void idRestoreGame::ReadSoundShader( const idSoundShader *&shader ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		shader = NULL;
	} else {
		shader = declManager->FindSound( name );
	}
}

/*
================
idRestoreGame::ReadModelDef
================
*/
void idRestoreGame::ReadModelDef( const idDeclModelDef *&modelDef ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		modelDef = NULL;
	} else {
		modelDef = gameLocal.declModelDefType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadVehicleScript
================
*/
void idRestoreGame::ReadVehicleScript( const sdDeclVehicleScript *&vehicleScript ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		vehicleScript = NULL;
	} else {
		vehicleScript = gameLocal.declVehicleScriptDefType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadInvItem
================
*/
void idRestoreGame::ReadInvItem( const sdDeclInvItem *&invItem ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		invItem = NULL;
	} else {
		invItem = gameLocal.declInvItemType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadPlayerClass
================
*/
void idRestoreGame::ReadPlayerClass( const sdDeclPlayerClass *&playerClass ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		playerClass = NULL;
	} else {
		playerClass = gameLocal.declPlayerClassType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadTable
================
*/
void idRestoreGame::ReadTable( const idDeclTable *&table ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		table = NULL;
	} else {
		table = gameLocal.declTableType[ name ];
	}
}

/*
================
idRestoreGame::ReadDamage
================
*/
void idRestoreGame::ReadDamage( const sdDeclDamage *&damage ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		damage = NULL;
	} else {
		damage = gameLocal.declDamageType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadToolTip
================
*/
void idRestoreGame::ReadToolTip( const sdDeclToolTip *&toolTip ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		toolTip = NULL;
	} else {
		toolTip = gameLocal.declToolTipType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadCampaign
================
*/
void idRestoreGame::ReadCampaign( const sdDeclCampaign *&campaign ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		campaign = NULL;
	} else {
		campaign = gameLocal.declCampaignType.LocalFind( name, false );
	}
}

/*
================
idRestoreGame::ReadModel
================
*/
void idRestoreGame::ReadModel( idRenderModel *&model ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		model = NULL;
	} else {
		model = renderModelManager->FindModel( name );
	}
}

/*
================
idRestoreGame::ReadUserInterface
================
*/
void idRestoreGame::ReadUserInterface( guiHandle_t& guiHandle ) {
	// Gordon: GUI FIXME
/*	idStr name;
	ReadString( name );

	if ( !name.Length() ) {
		guiHandle.Release();
	} else {
		bool unique;
		ReadBool( unique );
		ui = uiManager->FindGui( name, true, unique );
		if ( ui ) {
			if ( ui->ReadFromSaveGame( file ) == false ) {
				Error( "idSaveGame::ReadUserInterface: ui failed to read properly\n" );
			} else {
				ui->StateChanged( gameLocal.time );
			}
		}
	}*/
}

/*
================
idRestoreGame::ReadRenderEntity
================
*/
void idRestoreGame::ReadRenderEntity( renderEntity_t &renderEntity ) {
	int i;
	int index;

	ReadModel( renderEntity.hModel );

	ReadInt( renderEntity.entityNum );
	ReadInt( renderEntity.bodyId );

	ReadBounds( renderEntity.bounds );

	// callback is set by class's Restore function
	renderEntity.callback = NULL;
	renderEntity.callbackData = NULL;

	ReadInt( renderEntity.suppressSurfaceInViewID );
	ReadInt( renderEntity.suppressShadowInViewID );
	ReadInt( renderEntity.suppressShadowInLightID );
	ReadInt( renderEntity.allowSurfaceInViewID );

	ReadVec3( renderEntity.origin );
	ReadMat3( renderEntity.axis );

	ReadMaterial( renderEntity.customShader );
	ReadMaterial( renderEntity.referenceShader );
	ReadSkin( renderEntity.customSkin );

	ReadInt( index );
	renderEntity.referenceSound = gameSoundWorld->EmitterForIndex( index );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		ReadFloat( renderEntity.shaderParms[ i ] );
	}

	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		ReadUserInterface( renderEntity.gui[ i ] );
	}

	// idEntity will restore "cameraTarget", which will be used in idEntity::Present to restore the remoteRenderView
	renderEntity.remoteRenderView = NULL;

	renderEntity.joints = NULL;
	renderEntity.numJoints = 0;

	ReadFloat( renderEntity.modelDepthHack );
	ReadFloat( renderEntity.groundRadiosity );

	ReadBool( renderEntity.noSelfShadow );
	ReadBool( renderEntity.noShadow );
	ReadBool( renderEntity.noDynamicInteractions );
	ReadBool( renderEntity.weaponDepthHack );

	ReadInt( renderEntity.forceUpdate );
	ReadInt( renderEntity.forceOutside );

	ReadInt( renderEntity.maxVisDist );

	renderEntity.hideSurfaceMask.Clear();
	for ( i = 0; i < MAX_SURFACE_BITS; i++ ) {
		if ( ReadInt() == 1 ) {
			renderEntity.hideSurfaceMask.Set( i );
		}
	}

	ReadFloat( renderEntity.groundRadiosity );
}

/*
================
idRestoreGame::ReadRenderLight
================
*/
void idRestoreGame::ReadRenderLight( renderLight_t &renderLight ) {
	int index;
	int i;

	ReadMat3( renderLight.axis );
	ReadVec3( renderLight.origin );

	ReadInt( renderLight.suppressLightInViewID );
	ReadInt( renderLight.allowLightInViewID );
	renderLight.flags.noShadows = ReadBool();
	renderLight.flags.noSpecular = ReadBool();
	renderLight.flags.pointLight = ReadBool();
	renderLight.flags.parallel = ReadBool();
	renderLight.flags.atmosphereLight = ReadBool();

	ReadVec3( renderLight.lightRadius );
	ReadVec3( renderLight.lightCenter );

	ReadVec3( renderLight.target );
	ReadVec3( renderLight.right );
	ReadVec3( renderLight.up );
	ReadVec3( renderLight.start );
	ReadVec3( renderLight.end );

	// only idLight has a prelightModel and it's always based on the entityname, so we'll restore it there
	// ReadModel( renderLight.prelightModel );
	renderLight.prelightModel = NULL;

	ReadInt( renderLight.lightId );

	ReadMaterial( renderLight.shader );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		ReadFloat( renderLight.shaderParms[ i ] );
	}

	ReadInt( index );
	renderLight.referenceSound = gameSoundWorld->EmitterForIndex( index );
}

/*
================
idRestoreGame::ReadRenderEffect
================
*/
void idRestoreGame::ReadRenderEffect( renderEffect_t &renderEffect ) {
	int i;

	ReadEffect( reinterpret_cast< const rvDeclEffect*& >( renderEffect.declEffect ) );

	ReadInt( renderEffect.suppressSurfaceInViewID );
	ReadInt( renderEffect.allowSurfaceInViewID );
	ReadInt( renderEffect.groupID );

	ReadVec3( renderEffect.origin );
	ReadMat3( renderEffect.axis );

	ReadVec3( renderEffect.endOrigin );

	ReadFloat( renderEffect.attenuation );
	ReadBool( renderEffect.hasEndOrigin );
	ReadBool( renderEffect.loop );
	ReadInt( renderEffect.weaponDepthHackInViewID );
	ReadFloat( renderEffect.modelDepthHack );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		ReadFloat( renderEffect.shaderParms[ i ] );
	}

	ReadVec3( renderEffect.windVector );
}

/*
================
idRestoreGame::ReadRefSound
================
*/
void idRestoreGame::ReadRefSound( refSound_t &refSound ) {
	int		index;
	ReadInt( index );

	refSound.referenceSound = gameSoundWorld->EmitterForIndex( index );
	ReadVec3( refSound.origin );
	ReadInt( refSound.listenerId );
	ReadSoundShader( refSound.shader );
	ReadFloat( refSound.diversity );
	ReadBool( refSound.waitfortrigger );

	ReadFloat( refSound.parms.minDistance );
	ReadFloat( refSound.parms.maxDistance );
	ReadFloat( refSound.parms.volume );
	ReadFloat( refSound.parms.shakes );
	ReadInt( refSound.parms.soundShaderFlags );
	ReadInt( refSound.parms.soundClass );
}

/*
================
idRestoreGame::ReadRenderView
================
*/
void idRestoreGame::ReadRenderView( renderView_t &view ) {
	int i;

	ReadInt( view.viewID );
	ReadInt( view.x );
	ReadInt( view.y );
	ReadInt( view.width );
	ReadInt( view.height );

	ReadFloat( view.fov_x );
	ReadFloat( view.fov_y );
	ReadVec3( view.vieworg );
	ReadMat3( view.viewaxis );

	ReadBool( view.cramZNear );

	ReadInt( view.time );

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		ReadFloat( view.shaderParms[ i ] );
	}
}

/*
=================
idRestoreGame::ReadUsercmd
=================
*/
void idRestoreGame::ReadUsercmd( usercmd_t &usercmd ) {
	ReadInt( usercmd.gameFrame );
	ReadInt( usercmd.gameTime );
	ReadInt( usercmd.duplicateCount );
	ReadShort( usercmd.buttons.btnValue );
	ReadSignedChar( usercmd.forwardmove );
	ReadSignedChar( usercmd.rightmove );
	ReadSignedChar( usercmd.upmove );
	ReadShort( usercmd.angles[0] );
	ReadShort( usercmd.angles[1] );
	ReadShort( usercmd.angles[2] );
	ReadSignedChar( usercmd.impulse );
	ReadByte( usercmd.flags );

	file->Read( &usercmd.clientButtons, sizeof( usercmd.clientButtons ) );
	LittleBitField( &usercmd.clientButtons, sizeof( usercmd.clientButtons ) );
}

/*
===================
idRestoreGame::ReadContactInfo
===================
*/
void idRestoreGame::ReadContactInfo( contactInfo_t &contactInfo ) {
	ReadInt( (int &)contactInfo.type );
	ReadVec3( contactInfo.point );
	ReadVec3( contactInfo.normal );
	ReadFloat( contactInfo.dist );
	ReadInt( contactInfo.contents );
	ReadMaterial( contactInfo.material );
	ReadInt( contactInfo.modelFeature );
	ReadInt( contactInfo.trmFeature );
	ReadInt( contactInfo.entityNum );
	ReadInt( contactInfo.id );
}

/*
===================
idRestoreGame::ReadTrace
===================
*/
void idRestoreGame::ReadTrace( trace_t &trace ) {
	ReadFloat( trace.fraction );
	ReadVec3( trace.endpos );
	ReadMat3( trace.endAxis );
	ReadContactInfo( trace.c );
}

/*
 ===================
 idRestoreGame::ReadTraceModel
 ===================
 */
void idRestoreGame::ReadTraceModel( idTraceModel &trace ) {
	int j, k;
	
	ReadInt( (int&)trace.type );
	ReadInt( trace.numVerts );
	for ( j = 0; j < MAX_TRACEMODEL_VERTS; j++ ) {
		ReadVec3( trace.verts[j] );
	}
	ReadInt( trace.numEdges );
	for ( j = 0; j < (MAX_TRACEMODEL_EDGES+1); j++ ) {
		ReadInt( trace.edges[j].v[0] );
		ReadInt( trace.edges[j].v[1] );
		ReadVec3( trace.edges[j].normal );
	}
	ReadInt( trace.numPolys );
	for ( j = 0; j < MAX_TRACEMODEL_POLYS; j++ ) {
		ReadVec3( trace.polys[j].normal );
		ReadFloat( trace.polys[j].dist );
		ReadBounds( trace.polys[j].bounds );
		ReadInt( trace.polys[j].numEdges );
		for ( k = 0; k < MAX_TRACEMODEL_POLYEDGES; k++ ) {
			ReadInt( trace.polys[j].edges[k] );
		}
	}
	ReadVec3( trace.offset );
	ReadBounds( trace.bounds );
	ReadBool( trace.isConvex );
	// padding win32 native structs
	char tmp[3];
	file->Read( tmp, 3 );
}

/*
=====================
idRestoreGame::ReadClipModel
=====================
*/
void idRestoreGame::ReadClipModel( idClipModel *&clipModel ) {
	bool restoreClipModel;

	ReadBool( restoreClipModel );
	if ( restoreClipModel ) {
		clipModel = new idClipModel();
		clipModel->Restore( this );
	} else {
		clipModel = NULL;
	}
}

/*
=====================
idRestoreGame::ReadSoundCommands
=====================
*/
void idRestoreGame::ReadSoundCommands( void ) {
	gameSoundWorld->StopAllSounds();
	gameSoundWorld->ReadFromSaveGame( file );
}

/*
===================
idRestoreGame::ReadTeamInfo
===================
*/
void idRestoreGame::ReadTeamInfo( sdTeamInfo *&teamInfo ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		teamInfo = NULL;
	} else {
		teamInfo = &sdTeamManager::GetInstance().GetTeam( name );
	}
}

/*
===================
idRestoreGame::ReadFunction
===================
*/
void idRestoreGame::ReadFunction( const function_t*& function ) {
	idStr funcname;
	
	ReadString( funcname );
	if ( funcname.Length() ) {
		function = gameLocal.program.FindFunction( funcname );
		if ( function == NULL ) {
			gameLocal.Warning( "Unknown function '%s'", funcname.c_str() );
		}
	} else {
		function = NULL;
	}
}

/*
=====================
idRestoreGame::ReadBuildNumber
=====================
*/
void idRestoreGame::ReadBuildNumber( void ) {
	file->ReadInt( buildNumber );
}

/*
=====================
idRestoreGame::GetBuildNumber
=====================
*/
int idRestoreGame::GetBuildNumber( void ) {
	return buildNumber;
}
