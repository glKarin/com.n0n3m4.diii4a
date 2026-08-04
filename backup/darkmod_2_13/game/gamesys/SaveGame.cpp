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

#include "precompiled.h"
#pragma hdrstop



#include "../Game_local.h"

#include "TypeInfo.h"

#include "zlib.h"

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

Changes to classes that don't need to break compatibility can use the build number as the savegame version.
Later versions are responsible for restoring from previous versions by ignoring any unused data and initializing
variables that weren't in previous versions with safe information.

At the head of the save game is enough information to restore the player to the beginning of the level should the
file be unloadable in some way (for example, due to script changes).

Note: the caching works only if the compresion is enabled.
The caching mechanism works in the following way. On game saving, there are two types of data:
	1. Ordinary data (almost all the data)
		This type of data is pushed into the "cache" buffer.
	2. Special data which is now:
		//	ui->WriteToSaveGame( file ) - closed source
		//	gameSoundWorld->WriteToSaveGame( file ) - closed source
		//	WriteBuildNumber() and WriteCodeRevision() - vital, should not be compressed
		This data is written directly to the file.
After all the data has been passed to idSaveGame, FinalizeCache must be called.
Depending on the settings it may compress the cache. Then it is finally dumped to idFile.
After that the last 4 bytes are written to the file - offset to cache start from the end of file.
The final file layout is then:
	1. Doom3 header (level info, we cannot change it)
	2. Special data (including version numbers)
	3. Cache image, maybe compressed (consists of ordinary data)
	4. Offset from EOF to cache image (is negative)
Restoring works almost the same way, but the cache must be retrieved at the very beginning.
The InitializeCache must be called before any ordinary data is read from the file.
It uses fseek to get offset to cache image, then reads it, probably decompresses it.
Afterwards it fseeks to the file position on the moment of call.
Then all the Read* happens which reads ordinary data from cache and special data from file.
*/

idSaveGame::idSaveGame( idFile *savefile ) {

	file = savefile;

	// Put NULL at the start of the list so we can skip over it.
	objects.Clear();
	objects.Append( NULL );
}

idSaveGame::~idSaveGame() {
	if ( objects.Num() ) {
		Close();
	}
}

void idSaveGame::Close( void ) {
	int i;

	WriteSoundCommands();

	// read trace models
	idClipModel::SaveTraceModels( this );

	for( i = 1; i < objects.Num(); i++ ) {
		CallSave_r( objects[ i ]->GetType(), objects[ i ] );
	}

	objects.Clear();

#ifdef ID_USE_TYPEINFO
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ), false ) );
#endif
}

void idSaveGame::FinalizeCache( void ) {
	if (!isCompressed) return;
		
	int offset = sizeof(int);

	//resize destination buffer
	CRawVector zipped;
	uLongf zipsize = ExtLibs::compressBound((uLongf)cache.size());
	zipped.resize(zipsize);

	//compress the cache
	int err = ExtLibs::compress(
		(Bytef *)&zipped[0], &zipsize,
		(const Bytef *)&cache[0], (uLongf)cache.size()
	);
	if (err != Z_OK)
		gameLocal.Error("idSaveGame::FinalizeCache: compress failed with code %d", err);
	zipped.resize(zipsize);

	//write compressed size and uncompressed size
	file->WriteInt(zipped.size());				offset += sizeof(int);
	file->WriteInt(cache.size());				offset += sizeof(int);
	//write compressed data
	file->Write(&zipped[0], zipped.size());		offset += zipped.size();
	//write offset from EOF to cache start
	file->WriteInt(-offset);

	cache.clear();
}

void idSaveGame::WriteObjectList( void ) {
	int i;

	WriteInt( objects.Num() - 1 );
	for( i = 1; i < objects.Num(); i++ ) {
		WriteString( objects[ i ]->GetClassname() );
	}
}

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

void idSaveGame::AddObject( const idClass *obj ) {
	objects.AddUnique( obj );
}

void idSaveGame::Write( const void *buffer, int len ) {
	if (len == 0) return;
	if (isCompressed) {
		int sz = cache.size();
		cache.resize(sz + len);
		memcpy(&cache[sz], buffer, len);
	}
	else
		file->Write(buffer, len);
}

void idSaveGame::WriteJoint( const jointHandle_t value ) {
	WriteInt( (int)value );
}


void idSaveGame::WriteByte( const byte value ) {
	WriteUnsignedChar(value);
}

void idSaveGame::WriteSignedChar( const signed char value ) {
	WriteChar(value);
}

void idSaveGame::WriteString( const char *string ) {
	
    int len = static_cast<int>(strlen(string));
	WriteInt( len );
    Write( string, len );
}

void idSaveGame::WriteBounds( const idBounds &bounds ) {
	WriteVec3(bounds[0]);
	WriteVec3(bounds[1]);
}

void idSaveGame::WriteBox( const idBox &box ) {
	WriteVec3(box.GetCenter());
	WriteVec3(box.GetExtents());
	WriteMat3(box.GetAxis());
}

void idSaveGame::WriteWinding( const idWinding &w ) {
	int i, num;
	num = w.GetNumPoints();
	WriteInt( num );
	for ( i = 0; i < num; i++ )
		WriteVec5(w[i]);
}

void idSaveGame::WriteObject( const idClass *obj ) {
	int index;

	index = objects.FindIndex( obj );
	if ( index < 0 ) {
		//gameLocal.Warning( "idSaveGame::WriteObject - WriteObject FindIndex failed" ); // grayman #4340

		// Use the NULL index
		index = 0;
	}

	WriteInt( index );
}

void idSaveGame::WriteStaticObject( const idClass &obj ) {
	CallSave_r( obj.GetType(), &obj );
}

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

void idSaveGame::WriteMaterial( const idMaterial *material ) {
	if ( !material ) {
		WriteString( "" );
	} else {
		WriteString( material->GetName() );
	}
}

void idSaveGame::WriteSkin( const idDeclSkin *skin ) {
	if ( !skin ) {
		WriteString( "" );
	} else {
		WriteString( skin->GetName() );
	}
}

void idSaveGame::WriteParticle( const idDeclParticle *particle ) {
	if ( !particle ) {
		WriteString( "" );
	} else {
		WriteString( particle->GetName() );
	}
}

void idSaveGame::WriteFX( const idDeclFX *fx ) {
	if ( !fx ) {
		WriteString( "" );
	} else {
		WriteString( fx->GetName() );
	}
}

void idSaveGame::WriteModelDef( const idDeclModelDef *modelDef ) {
	if ( !modelDef ) {
		WriteString( "" );
	} else {
		WriteString( modelDef->GetName() );
	}
}

void idSaveGame::WriteSoundShader( const idSoundShader *shader ) {
	const char *name;

	if ( !shader ) {
		WriteString( "" );
	} else {
		name = shader->GetName();
		WriteString( name );
	}
}

void idSaveGame::WriteModel( const idRenderModel *model ) {
	const char *name;

	if ( !model ) {
		WriteString( "" );
	} else {
		name = model->Name();
		WriteString( name );
	}
}

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

	WriteBool( renderEntity.forceShadowBehindOpaque );

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
		WriteUserInterface( renderEntity.gui[ i ], renderEntity.gui[ i ] ? renderEntity.gui[ i ]->IsUniqued() : false );
	}

	WriteFloat( renderEntity.modelDepthHack );

	WriteBool( renderEntity.noSelfShadow );
	WriteBool( renderEntity.noShadow );
	WriteInt( renderEntity.areaLock );
	WriteBool( renderEntity.noDynamicInteractions );
	WriteBool( renderEntity.weaponDepthHack );

	WriteInt( renderEntity.forceUpdate );
	WriteInt( renderEntity.sortOffset );
	WriteInt( renderEntity.xrayIndex );
}

void idSaveGame::WriteRenderLight( const renderLight_t &renderLight ) {
	int i;

	WriteInt( renderLight.entityNum );

	WriteMat3( renderLight.axis );
	WriteVec3( renderLight.origin );

	WriteInt( renderLight.suppressLightInViewID );
	WriteInt( renderLight.suppressInSubview );
	WriteInt( renderLight.allowLightInViewID );
	WriteBool( renderLight.noShadows );
	WriteBool( renderLight.noSpecular );
	WriteBool( renderLight.pointLight );
	WriteBool( renderLight.parallel );
	WriteBool( renderLight.parallelSky );

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

	WriteBool( renderLight.noFogBoundary ); // #3664
	WriteBool( renderLight.noPortalFog ); // #6282
	WriteInt( renderLight.areaLock );

	WriteFloat( renderLight.volumetricDust );
	WriteInt( renderLight.volumetricNoshadows );
}

void idSaveGame::WriteRefSound( const refSound_t &refSound ) {
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
	WriteInt( refSound.parms.overrideMode );
}

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

void idSaveGame::WriteUsercmd( const usercmd_t &usercmd ) {
	WriteInt( usercmd.gameFrame );
	WriteInt( usercmd.gameTime );
	WriteInt( usercmd.duplicateCount );
	WriteByte( usercmd.buttons );
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

void idSaveGame::WriteTrace( const trace_t &trace ) {
	WriteFloat( trace.fraction );
	WriteVec3( trace.endpos );
	WriteMat3( trace.endAxis );
	WriteContactInfo( trace.c );
}

void idSaveGame::WriteTraceModel( const idTraceModel &trace ) {
	int j;
	
	WriteInt( (int&)trace.type );
	WriteInt( trace.numVerts );
	for ( j = 0; j < trace.numVerts; j++ ) {
		WriteVec3( trace.verts[j] );
	}
	WriteInt( trace.numEdges );
	for ( j = 0; j < (trace.numEdges+1); j++ ) {
		WriteInt( trace.edges[j].v[0] );
		WriteInt( trace.edges[j].v[1] );
		WriteVec3( trace.edges[j].normal );
	}
	WriteInt( trace.numEdgeUses );
	for ( j = 0; j < trace.numEdgeUses; j++ ) {
		WriteInt( trace.edgeUses[j] );
	}
	WriteInt( trace.numPolys );
	for ( j = 0; j < trace.numPolys; j++ ) {
		WriteVec3( trace.polys[j].normal );
		WriteFloat( trace.polys[j].dist );
		WriteBounds( trace.polys[j].bounds );
		WriteInt( trace.polys[j].numEdges );
		WriteInt( trace.polys[j].firstEdge );
	}
	WriteVec3( trace.offset );
	WriteBounds( trace.bounds );
	WriteBool( trace.isConvex );
	// padding win32 native structs
	char tmp[3];
	memset( tmp, 0, sizeof( tmp ) );
	Write( tmp, 3 );
}

void idSaveGame::WriteClipModel( const idClipModel *clipModel ) {
	if ( clipModel != NULL ) {
		WriteBool( true );
		clipModel->Save( this );
	} else {
		WriteBool( false );
	}
}

void idSaveGame::WriteUserInterface( const idUserInterface *ui, bool unique ) {
	const char *name;

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

void idSaveGame::WriteSoundCommands( void ) {
	gameSoundWorld->WriteToSaveGame( file );
}

void idSaveGame::WriteHeader()
{
	int revision = RevisionTracker::Instance().GetSavegameRevision();
	file->WriteInt(revision);
	isCompressed = cv_savegame_compress.GetBool();
	file->WriteBool(isCompressed);
}

/***********************************************************************

	idRestoreGame
	
***********************************************************************/

idRestoreGame::idRestoreGame( idFile *savefile ) {
	file = savefile;
}

idRestoreGame::~idRestoreGame() {
}

void idRestoreGame::InitializeCache() {
	if (!isCompressed) return;

	//move to end of file
	int position = file->Tell();
	file->Seek(-4, FS_SEEK_END);
	if (file->Tell() == position)
		Error( "idRestoreGame::InitializeCache: file->Seek failed");

	//read offset to cache and rollback
	int offset = 666;
	file->ReadInt(offset);
	if (offset >= 0)
		Error( "idRestoreGame::InitializeCache: bad cache offset (%d)", offset);
	file->Seek(offset, FS_SEEK_CUR);

	//read compressed cache size
	int zipSize = -1;
	file->ReadInt(zipSize);
	if (zipSize <= 0)
		Error("idRestoreGame::InitializeCache: bad compressed cache size (%d)", zipSize);

	//read decompressed cache size
	int cacheSize = -1;
	file->ReadInt(cacheSize);
	if (cacheSize <= 0)
		Error("idRestoreGame::InitializeCache: bad uncompressed cache size (%d)", cacheSize);

	//read compressed data
	CRawVector zipped;
	zipped.resize(zipSize);
	cache.resize(cacheSize);
	file->Read(&zipped[0], zipped.size());

	//decompress data
	uLongf cacheSizeL = cacheSize;
	int err = ExtLibs::uncompress(
		(Bytef *)&cache[0], &cacheSizeL,
		(const Bytef *)&zipped[0], (uLongf)zipped.size()
	);
	cacheSize = cacheSizeL;
	if (err != Z_OK)
		Error("idRestoreGame::InitializeCache: uncompress failed with code %d", err);
	if (cacheSize != cache.size())
		Error("idRestoreGame::InitializeCache: uncompressed size is %d instead of %d", cacheSize, cache.size());

	//set cache pointer
	cachePointer = 0;
	//return file pointer
	file->Seek(position, FS_SEEK_SET);
}

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

#ifdef ID_USE_TYPEINFO
		InitTypeVariables( objects[i], type->classname, 0xce );
#endif
	}
}

void idRestoreGame::RestoreObjects( void ) {
	int i;

	ReadSoundCommands();

	// read trace models
	idClipModel::RestoreTraceModels( this );

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

#ifdef ID_USE_TYPEINFO
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_restore", gameState.c_str() ), false ) );
	CompareGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ), false ) );
	gameLocal.Error( "dumped game states" );
#endif
}

void idRestoreGame::DeleteObjects( void ) {

	// Remove the NULL object before deleting
	objects.RemoveIndex( 0 );

	objects.DeleteContents( true );
}

void idRestoreGame::Error( const char *fmt, ... ) {
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	objects.DeleteContents( true );

	gameLocal.Error( "%s", text );
}

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

void idRestoreGame::Read( void *buffer, int len ) {
	if (len == 0) return;
	if (isCompressed) {
		assert(cachePointer + len <= int(cache.size()));
		memcpy(buffer, &cache[cachePointer], len);
		cachePointer += len;
	}
	else
		file->Read(buffer, len);
}

void idRestoreGame::ReadJoint( jointHandle_t &value ) {
	ReadInt( (int&)value );
}

void idRestoreGame::ReadByte( byte &value ) {
	ReadUnsignedChar(value);
}

void idRestoreGame::ReadSignedChar( signed char &value ) {
	ReadChar( (char&)value );
}

void idRestoreGame::ReadString( idStr &string ) {
	int len;

	ReadInt( len );
	if ( len < 0 ) {
		Error( "idRestoreGame::ReadString: invalid length (%d)", len );
	}

	string.Fill( ' ', len );
	Read( &string[ 0 ], len );
}

void idRestoreGame::ReadBounds( idBounds &bounds ) {
	ReadVec3(bounds[0]);
	ReadVec3(bounds[1]);
}

void idRestoreGame::ReadBox( idBox &box ) {
	idVec3 center, extents;
	idMat3 axis;
	ReadVec3(center);
	ReadVec3(extents);
	ReadMat3(axis);
	box = idBox(center, extents, axis);
}

void idRestoreGame::ReadWinding( idWinding &w ) {
	int i, num;
	ReadInt( num );
	if (num < 0)
		Error("idRestoreGame::ReadWinding: negative number of points (%d)", num);
	w.SetNumPoints( num );
	for ( i = 0; i < num; i++ )
		ReadVec5(w[i]);
}

void idRestoreGame::ReadObject( idClass *&obj ) {
	int index;

	ReadInt( index );
	if ( ( index < 0 ) || ( index >= objects.Num() ) ) {
		Error( "idRestoreGame::ReadObject: invalid object index" );
	}
	obj = objects[ index ];
}

void idRestoreGame::ReadStaticObject( idClass &obj ) {
	CallRestore_r( obj.GetType(), &obj );
}

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

void idRestoreGame::ReadMaterial( const idMaterial *&material ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		material = NULL;
	} else {
		material = declManager->FindMaterial( name );
	}
}

void idRestoreGame::ReadSkin( const idDeclSkin *&skin ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		skin = NULL;
	} else {
		skin = declManager->FindSkin( name );
	}
}

void idRestoreGame::ReadParticle( const idDeclParticle *&particle ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		particle = NULL;
	} else {
		particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, name ) );
	}
}

void idRestoreGame::ReadFX( const idDeclFX *&fx ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		fx = NULL;
	} else {
		fx = static_cast<const idDeclFX *>( declManager->FindType( DECL_FX, name ) );
	}
}

void idRestoreGame::ReadSoundShader( const idSoundShader *&shader ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		shader = NULL;
	} else {
		shader = declManager->FindSound( name );
	}
}

void idRestoreGame::ReadModelDef( const idDeclModelDef *&modelDef ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		modelDef = NULL;
	} else {
		modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	}
}

void idRestoreGame::ReadModel( idRenderModel *&model ) {
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		model = NULL;
	} else {
		model = renderModelManager->FindModel( name );
	}
}

void idRestoreGame::ReadUserInterface( idUserInterface *&ui ) {
	idStr name;

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
				ui->StateChanged( gameLocal.time );
			}
		}
	}
}

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

	ReadBool( renderEntity.forceShadowBehindOpaque );

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

	ReadBool( renderEntity.noSelfShadow );
	ReadBool( renderEntity.noShadow );
	ReadInt( (int&)renderEntity.areaLock );
	ReadBool( renderEntity.noDynamicInteractions );
	ReadBool( renderEntity.weaponDepthHack );

	ReadInt( renderEntity.forceUpdate );
	ReadInt( renderEntity.sortOffset );
	ReadInt( renderEntity.xrayIndex );
}

void idRestoreGame::ReadRenderLight( renderLight_t &renderLight ) {
	int index;
	int i;

	ReadInt( renderLight.entityNum );

	ReadMat3( renderLight.axis );
	ReadVec3( renderLight.origin );

	ReadInt( renderLight.suppressLightInViewID );
	ReadInt( renderLight.suppressInSubview );
	ReadInt( renderLight.allowLightInViewID );
	ReadBool( renderLight.noShadows );
	ReadBool( renderLight.noSpecular );
	ReadBool( renderLight.pointLight );
	ReadBool( renderLight.parallel );
	ReadBool( renderLight.parallelSky );

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

	ReadBool( renderLight.noFogBoundary ); // #3664
	ReadBool( renderLight.noPortalFog ); // #6282
	ReadInt( (int&)renderLight.areaLock );

	ReadFloat( renderLight.volumetricDust );
	ReadInt( renderLight.volumetricNoshadows );
}

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
	ReadInt( refSound.parms.overrideMode );
}

void idRestoreGame::ReadRenderView( renderView_t &view ) {
	int i;

	ReadInt( (int&)view.viewID );
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

void idRestoreGame::ReadUsercmd( usercmd_t &usercmd ) {
	ReadInt( usercmd.gameFrame );
	ReadInt( usercmd.gameTime );
	ReadInt( usercmd.duplicateCount );
	ReadByte( usercmd.buttons );
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

void idRestoreGame::ReadTrace( trace_t &trace ) {
	ReadFloat( trace.fraction );
	ReadVec3( trace.endpos );
	ReadMat3( trace.endAxis );
	ReadContactInfo( trace.c );
}

void idRestoreGame::ReadTraceModel( idTraceModel &trace ) {
	int j;
	
	ReadInt( (int&)trace.type );
	ReadInt( trace.numVerts );
	for ( j = 0; j < trace.numVerts; j++ ) {
		ReadVec3( trace.verts[j] );
	}
	ReadInt( trace.numEdges );
	for ( j = 0; j < (trace.numEdges+1); j++ ) {
		ReadInt( trace.edges[j].v[0] );
		ReadInt( trace.edges[j].v[1] );
		ReadVec3( trace.edges[j].normal );
	}
	ReadInt( trace.numEdgeUses );
	for ( j = 0; j < trace.numEdgeUses; j++ ) {
		ReadInt( trace.edgeUses[j] );
	}
	ReadInt( trace.numPolys );
	for ( j = 0; j < trace.numPolys; j++ ) {
		ReadVec3( trace.polys[j].normal );
		ReadFloat( trace.polys[j].dist );
		ReadBounds( trace.polys[j].bounds );
		ReadInt( trace.polys[j].numEdges );
		ReadInt( trace.polys[j].firstEdge );
	}
	ReadVec3( trace.offset );
	ReadBounds( trace.bounds );
	ReadBool( trace.isConvex );
	// padding win32 native structs
	char tmp[3];
	Read( tmp, 3 );
}

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

void idRestoreGame::ReadSoundCommands( void ) {
	gameSoundWorld->StopAllSounds();
	gameSoundWorld->ReadFromSaveGame( file );
}

void idRestoreGame::ReadHeader( void ) {
	// taaaki: final fix for bug #2997. The WriteHeader() call does not save
    //         a buildNumber to the file, so reading it out was causing things
    //         to break.
    // file->ReadInt( buildNumber );
	file->ReadInt( codeRevision );
	file->ReadBool(isCompressed);
}

/***********************************************************************

	Read/write for common types
	
***********************************************************************/

#define LittleChar(x) x

#define DEFINE_READWRITE_PRIMITIVE(cpp_type, name_type, conv_type)			\
void idRestoreGame::Read##name_type (cpp_type &value)						\
{														    				\
	if (isCompressed) {														\
		const cpp_type *value_ptr = (const cpp_type*)&cache[cachePointer];	\
		value = Little##conv_type (*value_ptr);								\
		cachePointer += sizeof(cpp_type);									\
	}																		\
	else																	\
		file->Read##name_type(value);										\
}                                                           				\
void idSaveGame::Write##name_type (const cpp_type value)					\
{														    				\
	if (isCompressed) {														\
		int sz = cache.size();												\
		cache.resize(sz + sizeof(cpp_type));								\
		cpp_type *value_ptr = (cpp_type*)&cache[sz];						\
		*value_ptr = Little##conv_type (value);								\
	}																		\
	else																	\
		file->Write##name_type(value);										\
}

DEFINE_READWRITE_PRIMITIVE(int, Int, Int)
DEFINE_READWRITE_PRIMITIVE(unsigned int, UnsignedInt, Int)
DEFINE_READWRITE_PRIMITIVE(short, Short, Short)
DEFINE_READWRITE_PRIMITIVE(char, Char, Char)
DEFINE_READWRITE_PRIMITIVE(unsigned char, UnsignedChar, Char)
DEFINE_READWRITE_PRIMITIVE(float, Float, Float)
DEFINE_READWRITE_PRIMITIVE(bool, Bool, Char)

#define DEFINE_READWRITE_VECMAT(cpp_type, name_type)        		\
void idRestoreGame::Read##name_type (cpp_type &vec)					\
{																	\
	for (int i = 0; i<vec.GetDimension(); i++)						\
		ReadFloat(vec.ToFloatPtr()[i]);								\
}																	\
void idSaveGame::Write##name_type (const cpp_type &vec)				\
{																	\
	for (int i = 0; i<vec.GetDimension(); i++)						\
		WriteFloat(vec.ToFloatPtr()[i]);							\
}

DEFINE_READWRITE_VECMAT(idVec2, Vec2)
DEFINE_READWRITE_VECMAT(idVec3, Vec3)
DEFINE_READWRITE_VECMAT(idVec4, Vec4)
DEFINE_READWRITE_VECMAT(idVec5, Vec5)
DEFINE_READWRITE_VECMAT(idVec6, Vec6)
DEFINE_READWRITE_VECMAT(idMat3, Mat3)
DEFINE_READWRITE_VECMAT(idAngles, Angles)

