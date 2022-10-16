
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

#ifdef _WIN32
#include "TypeInfo.h"
#else
#include "NoGameTypeInfo.h"
#endif

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

// RAVEN BEGIN
// jscott: sanity length check for strings
#define MAX_PRINT_MSG		4096
// RAVEN END

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
idSaveGame::~idSaveGame( void ) {
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

	// read trace models
	idClipModel::SaveTraceModels( this );

	for( i = 1; i < objects.Num(); i++ ) {
// RAVEN BEGIN
		WriteSyncId();
// RAVEN END
		CallSave_r( objects[ i ]->GetType(), objects[ i ] );
	}

	objects.Clear();

#ifdef ID_DEBUG_MEMORY
// RAVEN BEGIN
// jscott: don't use type info
//	idStr gameState = file->GetName();
//	gameState.StripFileExtension();
//	WriteGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ), false ) );
// RAVEN END
#endif
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

	WriteSyncId();
	( obj->*cls->Save )( this );
	WriteSyncId();
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
idSaveGame::WriteSyncId
================
*/
void idSaveGame::WriteSyncId( void ) {
	file->WriteSyncId();
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
idSaveGame::WriteJoint
================
*/
void idSaveGame::WriteJoint( const jointHandle_t value ) {
	file->WriteInt( value );
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
	file->WriteUnsignedChar( value );
}

/*
================
idSaveGame::WriteSignedChar
================
*/
void idSaveGame::WriteSignedChar( const signed char value ) {
	file->WriteChar( value );
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

	len = strlen( string );

// RAVEN BEGIN
// jscott: added safety check for silly length strings
	if( len < 0 || len >= MAX_PRINT_MSG ) {

		common->Error( "idSaveGame::WriteString invalid string length (%d)", len );
	}
// RAVEN END

	file->WriteString( string );
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
idSaveGame::WriteVec5
================
*/
void idSaveGame::WriteVec5( const idVec5 &vec ) {
	file->WriteVec5( vec );
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
	file->Write( &bounds, sizeof( bounds ) );
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
	WriteInt( num );
	for ( i = 0; i < num; i++ ) {
		WriteVec5( w[i] );
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
	file->Write( &angles, sizeof( angles ) );
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
// RAVEN BEGIN
	WriteSyncId();
// RAVEN END
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

// RAVEN BEGIN
	WriteSyncId();
// RAVEN END

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

// RAVEN BEGIN
// bdube: material type
/*
================
idSaveGame::WriteMaterial
================
*/
void idSaveGame::WriteMaterialType ( const rvDeclMatType* materialType ) {
	if ( !materialType ) {
		WriteString( "" );
	} else {
		WriteString( materialType->GetName() );
	}
}

/*
================
idSaveGame::WriteTable
================
*/
void idSaveGame::WriteTable ( const idDeclTable* table ) {
	if ( !table ) {
		WriteString( "" );
	} else {
		WriteString( table->GetName() );
	}
}
// RAVEN END

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
void idSaveGame::WriteUserInterface( const idUserInterface *ui, bool unique ) {
	const char *name;

// RAVEN BEGIN
	WriteSyncId();
// RAVEN END

	if ( !ui ) {
		WriteString( "" );
	} else {
		name = ui->Name();
		WriteString( name );
		WriteBool( unique );
		if ( ui->WriteToSaveGame( file ) == false ) {
			gameLocal.Error( "idSaveGame::WriteUserInterface: ui failed to write properly\n" );
		}
	}
}

// RAVEN BEGIN
// abahr
/*
================
idSaveGame::WriteExtrapolate
================
*/
void idSaveGame::WriteExtrapolate( const idExtrapolate<int>& extrap ) {
	WriteInt( (int)extrap.GetExtrapolationType() );
	WriteFloat(	extrap.GetStartTime() );
	WriteFloat(	extrap.GetDuration() );

	WriteInt( extrap.GetStartValue() );
	WriteInt( extrap.GetBaseSpeed() );
	WriteInt( extrap.GetSpeed() );
}

/*
================
idSaveGame::WriteExtrapolate
================
*/
void idSaveGame::WriteExtrapolate( const idExtrapolate<float>& extrap ) {
	WriteInt( (int)extrap.GetExtrapolationType() );
	WriteFloat(	extrap.GetStartTime() );
	WriteFloat(	extrap.GetDuration() );

	WriteFloat( extrap.GetStartValue() );
	WriteFloat( extrap.GetBaseSpeed() );
	WriteFloat( extrap.GetSpeed() );
}

/*
================
idSaveGame::WriteExtrapolate
================
*/
void idSaveGame::WriteExtrapolate( const idExtrapolate<idVec3>& extrap ) {
	WriteInt( (int)extrap.GetExtrapolationType() );
	WriteFloat(	extrap.GetStartTime() );
	WriteFloat(	extrap.GetDuration() );

	WriteVec3( extrap.GetStartValue() );
	WriteVec3( extrap.GetBaseSpeed() );
	WriteVec3( extrap.GetSpeed() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolateAccelDecelLinear<int>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );
	WriteFloat( lerp.GetAcceleration() );
	WriteFloat( lerp.GetDeceleration() );
	
	WriteInt( lerp.GetStartValue() );
	WriteInt( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolateAccelDecelLinear<float>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );
	WriteFloat( lerp.GetAcceleration() );
	WriteFloat( lerp.GetDeceleration() );
	
	WriteFloat( lerp.GetStartValue() );
	WriteFloat( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolateAccelDecelLinear<idVec3>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );
	WriteFloat( lerp.GetAcceleration() );
	WriteFloat( lerp.GetDeceleration() );
	
	WriteVec3( lerp.GetStartValue() );
	WriteVec3( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolate<int>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );

	WriteInt( lerp.GetStartValue() );
	WriteInt( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolate<float>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );

	WriteFloat( lerp.GetStartValue() );
	WriteFloat( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteInterpolate
================
*/
void idSaveGame::WriteInterpolate( const idInterpolate<idVec3>& lerp ) {
	WriteFloat( lerp.GetStartTime() );
	WriteFloat( lerp.GetDuration() );

	WriteVec3( lerp.GetStartValue() );
	WriteVec3( lerp.GetEndValue() );
}

/*
================
idSaveGame::WriteRenderEffect
================
*/
void idSaveGame::WriteRenderEffect( const renderEffect_t &renderEffect ) {
	WriteSyncId();

	WriteFloat( renderEffect.startTime );
	WriteInt( renderEffect.suppressSurfaceInViewID );
	WriteInt( renderEffect.allowSurfaceInViewID );
	WriteInt( renderEffect.groupID );

	WriteVec3( renderEffect.origin );
	WriteMat3( renderEffect.axis );

	WriteVec3( renderEffect.gravity );
	WriteVec3( renderEffect.endOrigin );

	WriteFloat( renderEffect.attenuation );
	WriteBool( renderEffect.hasEndOrigin );
	WriteBool( renderEffect.loop );
	WriteBool( renderEffect.ambient );
	WriteBool( renderEffect.inConnectedArea );
	WriteInt( renderEffect.weaponDepthHackInViewID );
	WriteFloat( renderEffect.modelDepthHack );

	WriteInt( renderEffect.referenceSoundHandle );

	for( int ix = 0; ix < MAX_ENTITY_SHADER_PARMS; ++ix ) {
		WriteFloat( renderEffect.shaderParms[ ix ] );
	}

	if( renderEffect.declEffect ) {
		WriteString( renderEffect.declEffect->GetName() );
	} else {
		WriteString( "" );
	}
}

/*
================
idSaveGame::WriteFrustum
================
*/
void idSaveGame::WriteFrustum( const idFrustum& frustum ) {
// RAVEN BEGIN
	WriteSyncId();
// RAVEN END
	WriteVec3( frustum.GetOrigin() );
	WriteMat3( frustum.GetAxis() );
	WriteFloat( frustum.GetNearDistance() );
	WriteFloat( frustum.GetFarDistance() );
	WriteFloat( frustum.GetLeft() );
	WriteFloat( frustum.GetUp() );
}

/*
================
idSaveGame::WriteRenderEntity
================
*/
void idSaveGame::WriteRenderEntity( const renderEntity_t &renderEntity ) {
	int i;

	WriteSyncId();

	WriteModel( renderEntity.hModel );

	WriteInt( renderEntity.entityNum );
	WriteInt( renderEntity.bodyId );

	assert( renderEntity.bounds[0][0] <= renderEntity.bounds[1][0] ); 
	assert( renderEntity.bounds[0][1] <= renderEntity.bounds[1][1] );
	assert( renderEntity.bounds[0][2] <= renderEntity.bounds[1][2] );

	assert( renderEntity.bounds[1][0] - renderEntity.bounds[0][0] < MAX_BOUND_SIZE );
	assert( renderEntity.bounds[1][1] - renderEntity.bounds[0][1] < MAX_BOUND_SIZE );
	assert( renderEntity.bounds[1][2] - renderEntity.bounds[0][2] < MAX_BOUND_SIZE );

	WriteBounds( renderEntity.bounds );

	// callback is set by class's Restore function

	WriteInt( renderEntity.suppressSurfaceInViewID );
	WriteInt( renderEntity.suppressShadowInViewID );
	WriteInt( renderEntity.suppressShadowInLightID );
	WriteInt( renderEntity.allowSurfaceInViewID );

	WriteInt( renderEntity.suppressSurfaceMask );

	WriteVec3( renderEntity.origin );
	WriteMat3( renderEntity.axis );

	WriteMaterial( renderEntity.customShader );
	WriteMaterial( renderEntity.referenceShader );
	WriteMaterial( renderEntity.overlayShader );
	WriteSkin( renderEntity.customSkin );

	WriteInt( renderEntity.referenceSoundHandle );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		WriteFloat( renderEntity.shaderParms[ i ] );
	}

	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		WriteUserInterface( renderEntity.gui[ i ], renderEntity.gui[ i ] ? renderEntity.gui[ i ]->IsUniqued() : false );
	}

	WriteFloat( renderEntity.modelDepthHack );

	WriteBool( renderEntity.noSelfShadow );
	WriteBool( renderEntity.noShadow );
	WriteBool( renderEntity.noDynamicInteractions );
	WriteBool( renderEntity.forceUpdate );

	WriteInt( renderEntity.weaponDepthHackInViewID );
	WriteFloat( renderEntity.shadowLODDistance );
	WriteInt( renderEntity.suppressLOD );
}
// RAVEN END

/*
================
idSaveGame::WriteRenderLight
================
*/
void idSaveGame::WriteRenderLight( const renderLight_t &renderLight ) {
	int i;

// RAVEN BEGIN
	WriteSyncId();
// RAVEN END

	WriteMat3( renderLight.axis );
	WriteVec3( renderLight.origin );

	WriteInt( renderLight.suppressLightInViewID );
	WriteInt( renderLight.allowLightInViewID );
	WriteBool( renderLight.noShadows );
	WriteBool( renderLight.noSpecular );
	WriteBool( renderLight.noDynamicShadows );
	WriteBool( renderLight.pointLight );
	WriteBool( renderLight.parallel );
	WriteBool( renderLight.globalLight );

// RAVEN BEGIN
// dluetscher: added detail levels to render lights
	WriteFloat( renderLight.detailLevel );
// RAVEN END

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

// RAVEN BEGIN
	WriteInt( renderLight.referenceSoundHandle );
// RAVEN END
}

/*
================
idSaveGame::WriteRefSound
================
*/
void idSaveGame::WriteRefSound( const refSound_t &refSound ) {
// RAVEN BEGIN
	WriteSyncId();

	WriteInt( refSound.referenceSoundHandle );
// RAVEN END
	WriteVec3( refSound.origin );
// RAVEN BEGIN
	WriteVec3( refSound.velocity );
// RAVEN END
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

// RAVEN BEGIN
	WriteSyncId();
// RAVEN END

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
// RAVEN BEGIN
	WriteSyncId();
// RAVEN END
	WriteInt( usercmd.gameFrame );
	WriteInt( usercmd.gameTime );
	WriteInt( usercmd.duplicateCount );
// RAVEN BEGIN
// ddynerman: larger button bitfield
	WriteShort( usercmd.buttons );
// RAVEN END
	WriteSignedChar( usercmd.forwardmove );
	WriteSignedChar( usercmd.rightmove );
	WriteSignedChar( usercmd.upmove );
	WriteShort( usercmd.angles[0] );
	WriteShort( usercmd.angles[1] );
	WriteShort( usercmd.angles[2] );
	WriteShort( usercmd.mx );
	WriteShort( usercmd.my );
	WriteSignedChar( usercmd.impulse );
	WriteByte( usercmd.flags );
	WriteInt( usercmd.sequence );
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
	WriteMaterialType( contactInfo.materialType );
}

/*
===================
idSaveGame::WriteTrace
===================
*/
void idSaveGame::WriteTrace( const trace_t &trace ) {
// RAVEN BEGIN
	WriteSyncId();
// RAVEN END
	WriteFloat( trace.fraction );
	WriteVec3( trace.endpos );
	WriteMat3( trace.endAxis );
	WriteContactInfo( trace.c );
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
	soundSystem->WriteToSaveGame( SOUNDWORLD_GAME, file );
}

/*
======================
idSaveGame::WriteBuildNumber
======================
*/
void idSaveGame::WriteBuildNumber( const int value ) {
	WriteInt( value );
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

// RAVEN BEGIN
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

	for ( i = 1; i < objects.Num(); i++ ) {
		ReadString( classname );
		type = idClass::GetClass( classname );
		if ( !type ) {
			Error( "idRestoreGame::CreateObjects: Unknown class '%s'", classname.c_str() );
		}
		objects[ i ] = type->CreateInstance();
		assert( objects[ i ] != NULL );
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

	// read trace models
	idClipModel::RestoreTraceModels( this );

	// restore all the objects
	for( i = 1; i < objects.Num(); i++ ) {
		file->ReadSyncId( "Restore objects", objects[ i ]->GetClassname() );
		CallRestore_r( objects[ i ]->GetType(), objects[ i ] );
	}

	// regenerate render entities and render lights because are not saved
	for( i = 1; i < objects.Num(); i++ ) {
		if ( objects[ i ]->IsType( idEntity::GetClassType() ) ) {
			idEntity *ent = static_cast<idEntity *>( objects[ i ] );
			ent->UpdateVisuals();
			ent->Present();
		}
	}
}
// RAVEN END

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

// RAVEN BEGIN
	// FIXME: this crashes. It now leaks, but that's better than crashing.
	// The problem is that some entities delete attached ents that are also in this list. When this call gets to them
	// it tries to delete an already deleted object
//	objects.DeleteContents( true );
// RAVEN END

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
	
	file->ReadSyncId( "Callrestore_r start ", cls->classname );
	( obj->*cls->Restore )( this );
	file->ReadSyncId( "Callrestore_r end ", cls->classname );
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
idRestoreGame::ReadJoint
================
*/
void idRestoreGame::ReadJoint( jointHandle_t &value ) {
	file->ReadInt( ( int &)value );
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
	file->ReadUnsignedChar( value );
}

/*
================
idRestoreGame::ReadSignedChar
================
*/
void idRestoreGame::ReadSignedChar( signed char &value ) {
	file->ReadChar( ( char & )value );
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
idRestoreGame::ReadBool
================
*/
void idRestoreGame::ReadBool( bool &value ) {
	file->ReadBool( value );
}

/*
================
idRestoreGame::ReadString
================
*/
void idRestoreGame::ReadString( idStr &string ) {
/*	int len;

	ReadInt( len );
// RAVEN BEGIN
// jscott: added max check - should be big enough
	if ( len < 0 || len > MAX_PRINT_MSG ) {
		Error( "idRestoreGame::ReadString: invalid length (%d)", len );
// RAVEN END
	}

	string.Fill( ' ', len );*/
	file->ReadString( string );
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
idRestoreGame::ReadVec5
================
*/
void idRestoreGame::ReadVec5( idVec5 &vec ) {
	file->ReadVec5( vec );
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
		file->ReadVec5( w[i] );
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
// RAVEN BEGIN
	file->ReadSyncId( "ReadStaticObject", obj.GetClassname() );
// RAVEN END

	CallRestore_r( obj.GetType(), &obj );

// RAVEN BEGIN
	obj.PostEventMS( &EV_PostRestore, 0 );
// RAVEN END
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

// RAVEN BEGIN
	file->ReadSyncId( "ReadDict" );
// RAVEN END

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

// RAVEN BEGIN
// bdube: material type
/*
================
idRestoreGame::ReadMaterialType
================
*/
void idRestoreGame::ReadMaterialType ( const rvDeclMatType* &materialType ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		materialType = NULL;
	} else {
		materialType = declManager->FindMaterialType ( name );
	}
}

/*
================
idRestoreGame::ReadTable
================
*/
void idRestoreGame::ReadTable  ( const idDeclTable* &table ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		table = NULL;
	} else {
		table = declManager->FindTable( name );
	}
}

// RAVEN END

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
		modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
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
// RAVEN BEGIN
void idRestoreGame::ReadUserInterface( idUserInterface *&ui, const idDict *args ) {
// RAVEN END
	idStr name;

// RAVEN BEGIN
	file->ReadSyncId( "ReadUserInterface" );
// RAVEN END

	ReadString( name );
	if ( !name.Length() ) {
		ui = NULL;
	} else {
		bool unique;
		ReadBool( unique );
		ui = uiManager->FindGui( name, true, unique );
		if ( ui ) {
			if ( ui->ReadFromSaveGame( file ) == false ) {
				Error( "idSaveGame::ReadUserInterface: ui failed to read properly\n" );
			} else {
// RAVEN BEGIN
				UpdateGuiParms( ui, args );
// RAVEN END
			}
		}
	}
}

// RAVEN BEGIN
// abahr
/*
================
idRestoreGame::ReadExtrapolate
================
*/
void idRestoreGame::ReadExtrapolate( idExtrapolate<int>& extrap ) {
	int		extrapType;
	float	startTime;
	float	duration;
	int		startValue;
	int		baseSpeed;
	int		speed;

	ReadInt( extrapType );
	ReadFloat( startTime );
	ReadFloat( duration );

	ReadInt( startValue );
	ReadInt( baseSpeed );
	ReadInt( speed );

	extrap.Init( startTime, duration, startValue, baseSpeed, speed, (extrapolation_t)extrapType );
}

/*
================
idRestoreGame::ReadExtrapolate
================
*/
void idRestoreGame::ReadExtrapolate( idExtrapolate<float>& extrap ) {
	int		extrapType;
	float	startTime;
	float	duration;
	float	startValue;
	float	baseSpeed;
	float	speed;

	ReadInt( extrapType );
	ReadFloat( startTime );
	ReadFloat( duration );

	ReadFloat( startValue );
	ReadFloat( baseSpeed );
	ReadFloat( speed );

	extrap.Init( startTime, duration, startValue, baseSpeed, speed, (extrapolation_t)extrapType );
}

/*
================
idRestoreGame::ReadExtrapolate
================
*/
void idRestoreGame::ReadExtrapolate( idExtrapolate<idVec3>& extrap ) {
	int		extrapType;
	float	startTime;
	float	duration;
	idVec3	startValue;
	idVec3	baseSpeed;
	idVec3	speed;

	ReadInt( extrapType );
	ReadFloat( startTime );
	ReadFloat( duration );

	ReadVec3( startValue );
	ReadVec3( baseSpeed );
	ReadVec3( speed );

	extrap.Init( startTime, duration, startValue, baseSpeed, speed, (extrapolation_t)extrapType );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolateAccelDecelLinear<int>& lerp ) {
	float	startTime;
	float	duration;
	float	accelTime;
	float	decelTime;
	
	int		startValue;
	int		endValue;
	
	ReadFloat( startTime );
	ReadFloat( duration );
	ReadFloat( accelTime );
	ReadFloat( decelTime );

	ReadInt( startValue );
	ReadInt( endValue );

	lerp.Init( startTime, accelTime, decelTime, duration, startValue, endValue );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolateAccelDecelLinear<float>& lerp ) {
	float	startTime;
	float	duration;
	float	accelTime;
	float	decelTime;
	
	float	startValue;
	float	endValue;
	
	ReadFloat( startTime );
	ReadFloat( duration );
	ReadFloat( accelTime );
	ReadFloat( decelTime );

	ReadFloat( startValue );
	ReadFloat( endValue );

	lerp.Init( startTime, accelTime, decelTime, duration, startValue, endValue );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolateAccelDecelLinear<idVec3>& lerp ) {
	float	startTime;
	float	duration;
	float	accelTime;
	float	decelTime;
	
	idVec3	startValue;
	idVec3	endValue;
	
	ReadFloat( startTime );
	ReadFloat( duration );
	ReadFloat( accelTime );
	ReadFloat( decelTime );

	ReadVec3( startValue );
	ReadVec3( endValue );

	lerp.Init( startTime, accelTime, decelTime, duration, startValue, endValue );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolate<int>& lerp ) {
	float	startTime;
	float	duration;

	int		startValue;
	int		endValue;

	ReadFloat( startTime );
	ReadFloat( duration );

	ReadInt( startValue );
	ReadInt( endValue );

	lerp.Init( startTime, duration, startValue, endValue );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolate<float>& lerp ) {
	float	startTime;
	float	duration;

	float	startValue;
	float	endValue;

	ReadFloat( startTime );
	ReadFloat( duration );

	ReadFloat( startValue );
	ReadFloat( endValue );

	lerp.Init( startTime, duration, startValue, endValue );
}

/*
================
idRestoreGame::ReadInterpolate
================
*/
void idRestoreGame::ReadInterpolate( idInterpolate<idVec3>& lerp ) {
	float	startTime;
	float	duration;

	idVec3	startValue;
	idVec3	endValue;

	ReadFloat( startTime );
	ReadFloat( duration );

	ReadVec3( startValue );
	ReadVec3( endValue );

	lerp.Init( startTime, duration, startValue, endValue );
}
/*
================
idRestoreGame::ReadRenderEffect
================
*/
void idRestoreGame::ReadRenderEffect( renderEffect_t &renderEffect ) {
	idStr	name;

	file->ReadSyncId( "ReadRenderEffect" );

	renderEffect.declEffect = NULL;

	ReadFloat( renderEffect.startTime );
	ReadInt( renderEffect.suppressSurfaceInViewID );
	ReadInt( renderEffect.allowSurfaceInViewID );
	ReadInt( renderEffect.groupID );

	ReadVec3( renderEffect.origin );
	ReadMat3( renderEffect.axis );

	ReadVec3( renderEffect.gravity );
	ReadVec3( renderEffect.endOrigin );

	ReadFloat( renderEffect.attenuation );
	ReadBool( renderEffect.hasEndOrigin );
	ReadBool( renderEffect.loop );
	ReadBool( renderEffect.ambient );
	ReadBool( renderEffect.inConnectedArea );
	ReadInt( renderEffect.weaponDepthHackInViewID );
	ReadFloat( renderEffect.modelDepthHack );

	ReadInt( renderEffect.referenceSoundHandle );

	for( int ix = 0; ix < MAX_ENTITY_SHADER_PARMS; ++ix ) {
		ReadFloat( renderEffect.shaderParms[ ix ] );
	}

	ReadString( name );
	if( name.Length() ) {
		renderEffect.declEffect = declManager->FindType( DECL_EFFECT, name );
	}
}

/*
================
idRestoreGame::ReadFrustum
================
*/
void idRestoreGame::ReadFrustum( idFrustum& frustum ) {
	idVec3 origin;
	idMat3 axis;
	float dNear = 0.0f, dFar = 0.0f, dLeft = 0.0f, dUp = 0.0f;
// RAVEN BEGIN
	file->ReadSyncId( "ReadFrustum" );
// RAVEN END
	ReadVec3( origin );
	frustum.SetOrigin( origin );

	ReadMat3( axis );
	frustum.SetAxis( axis );

	ReadFloat( dNear );
	ReadFloat( dFar );
	ReadFloat( dLeft );
	ReadFloat( dUp );
	frustum.SetSize( dNear, dFar, dLeft, dUp ); 
}

/*
================
idRestoreGame::ReadRenderEntity
================
*/
// RAVEN BEGIN
void idRestoreGame::ReadRenderEntity( renderEntity_t &renderEntity, const idDict *args ) {
// RAVEN END
	int i;

	file->ReadSyncId( "ReadRenderEntity" );

	ReadModel( renderEntity.hModel );

	ReadInt( renderEntity.entityNum );
	ReadInt( renderEntity.bodyId );

	ReadBounds( renderEntity.bounds );

	assert( renderEntity.bounds[0][0] <= renderEntity.bounds[1][0] ); 
	assert( renderEntity.bounds[0][1] <= renderEntity.bounds[1][1] );
	assert( renderEntity.bounds[0][2] <= renderEntity.bounds[1][2] );

	assert( renderEntity.bounds[1][0] - renderEntity.bounds[0][0] < MAX_BOUND_SIZE );
	assert( renderEntity.bounds[1][1] - renderEntity.bounds[0][1] < MAX_BOUND_SIZE );
	assert( renderEntity.bounds[1][2] - renderEntity.bounds[0][2] < MAX_BOUND_SIZE );

	// callback is set by class's Restore function
	renderEntity.callback = NULL;
	renderEntity.callbackData = NULL;

	ReadInt( renderEntity.suppressSurfaceInViewID );
	ReadInt( renderEntity.suppressShadowInViewID );
	ReadInt( renderEntity.suppressShadowInLightID );
	ReadInt( renderEntity.allowSurfaceInViewID );

	ReadInt( renderEntity.suppressSurfaceMask );

	ReadVec3( renderEntity.origin );
	ReadMat3( renderEntity.axis );

	ReadMaterial( renderEntity.customShader );
	ReadMaterial( renderEntity.referenceShader );
	ReadMaterial( renderEntity.overlayShader );
	ReadSkin( renderEntity.customSkin );

	ReadInt( renderEntity.referenceSoundHandle );

	for( i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		ReadFloat( renderEntity.shaderParms[ i ] );
	}

	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
// RAVEN BEGIN
		ReadUserInterface( renderEntity.gui[ i ], args );
// RAVEN END
	}

	// idEntity will restore "cameraTarget", which will be used in idEntity::Present to restore the remoteRenderView
	renderEntity.remoteRenderView = NULL;

	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;

	ReadFloat( renderEntity.modelDepthHack );

	ReadBool( renderEntity.noSelfShadow );
	ReadBool( renderEntity.noShadow );
	ReadBool( renderEntity.noDynamicInteractions );
	ReadBool( (bool &)renderEntity.forceUpdate ); // int& -> bool&

	ReadInt( renderEntity.weaponDepthHackInViewID );
	ReadFloat( renderEntity.shadowLODDistance );
	ReadInt( renderEntity.suppressLOD );
}
// RAVEN END

/*
================
idRestoreGame::ReadRenderLight
================
*/
void idRestoreGame::ReadRenderLight( renderLight_t &renderLight ) {
	int i;

	file->ReadSyncId( "ReadRenderLight" );

	ReadMat3( renderLight.axis );
	ReadVec3( renderLight.origin );

	ReadInt( renderLight.suppressLightInViewID );
	ReadInt( renderLight.allowLightInViewID );
	ReadBool( renderLight.noShadows );
	ReadBool( renderLight.noSpecular );
	ReadBool( renderLight.noDynamicShadows );
	ReadBool( renderLight.pointLight );
	ReadBool( renderLight.parallel );
	ReadBool( renderLight.globalLight );

// RAVEN BEGIN
// dluetscher: added detail levels to render lights
	ReadFloat( renderLight.detailLevel );
// RAVEN END

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

// RAVEN BEGIN
	ReadInt( renderLight.referenceSoundHandle );
// RAVEN END
}

/*
================
idRestoreGame::ReadRefSound
================
*/
void idRestoreGame::ReadRefSound( refSound_t &refSound ) {
// RAVEN BEGIN
	file->ReadSyncId( "ReadRefSound" );
// RAVEN END

	ReadInt( refSound.referenceSoundHandle );
	ReadVec3( refSound.origin );
// RAVEN BEGIN
	ReadVec3( refSound.velocity );
// RAVEN END
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

// RAVEN BEGIN
	file->ReadSyncId( "ReadRenderView" );
// RAVEN END

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
// RAVEN BEGIN
	file->ReadSyncId( "ReadUsercmd" );
// RAVEN END
	ReadInt( usercmd.gameFrame );
	ReadInt( usercmd.gameTime );
	ReadInt( usercmd.duplicateCount );
// RAVEN BEGIN
// ddynerman: larger button bitfield
	ReadShort( (short &)usercmd.buttons ); //k byte& -> short&
// RAVEN END
	ReadSignedChar( usercmd.forwardmove );
	ReadSignedChar( usercmd.rightmove );
	ReadSignedChar( usercmd.upmove );
	ReadShort( usercmd.angles[0] );
	ReadShort( usercmd.angles[1] );
	ReadShort( usercmd.angles[2] );
	ReadShort( usercmd.mx );
	ReadShort( usercmd.my );
	ReadSignedChar( usercmd.impulse );
	ReadByte( usercmd.flags );
	ReadInt( usercmd.sequence );
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
	ReadMaterialType( contactInfo.materialType );
}

/*
===================
idRestoreGame::ReadTrace
===================
*/
void idRestoreGame::ReadTrace( trace_t &trace ) {
// RAVEN BEGIN
	file->ReadSyncId( "ReadTrace" );
// RAVEN END
	ReadFloat( trace.fraction );
	ReadVec3( trace.endpos );
	ReadMat3( trace.endAxis );
	ReadContactInfo( trace.c );
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
	soundSystem->StopAllSounds( SOUNDWORLD_GAME );
	soundSystem->ReadFromSaveGame( SOUNDWORLD_GAME, file );
}

/*
=====================
idRestoreGame::ReadBuildNumber
=====================
*/
void idRestoreGame::ReadBuildNumber( void ) {
	ReadInt( buildNumber );
}

/*
=====================
idRestoreGame::GetBuildNumber
=====================
*/
int idRestoreGame::GetBuildNumber( void ) {
	return buildNumber;
}



void Cmd_CheckSave_f( const idCmdArgs &args )
{
#if 0 //k: not implement
	idPlayer	*lp = gameLocal.GetLocalPlayer();
	idFile		*mp = fileSystem->GetNewFileMemory();
	idSaveGame	sg( mp );

	sg.CallSave_r( lp->GetType(), lp );


	mp->Rewind();
	idPlayer		test;
	idRestoreGame	rg( mp );

	rg.CallRestore_r( test.GetType(), &test );
#endif
}

