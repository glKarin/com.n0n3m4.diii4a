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

#ifndef __SAVEGAME_H__
#define __SAVEGAME_H__

/*

Save game related helper classes.

*/

#include "../RawVector.h"

const int INITIAL_RELEASE_BUILD_NUMBER = 1262;

class idDeclSkin;
class idDeclParticle;
class idDeclFX;
class idSoundShader;
class idUserInterface;
typedef struct renderEntity_s renderEntity_t;
typedef struct renderLight_s renderLight_t;
struct refSound_t;
typedef struct renderView_s renderView_t;
class usercmd_t;
struct contactInfo_t;
typedef struct trace_s trace_t;
class idTraceModel;
class idClipModel;

class idSaveGame {
public:
							idSaveGame( idFile *savefile );
							~idSaveGame();

	void					Close( void );

	void					AddObject( const idClass *obj );
	void					WriteObjectList( void );

	void					Write( const void *buffer, int len );

	void					WriteInt( const int value );
	void					WriteUnsignedInt( const unsigned int value );
	void					WriteShort( const short value );
	void					WriteUnsignedShort( unsigned short value );
	void					WriteChar( const char value );
	void					WriteUnsignedChar( const unsigned char value );
	void					WriteFloat( const float value );
	void					WriteBool( const bool value );
	void					WriteString( const char *string );

	void					WriteJoint( const jointHandle_t value );
	void					WriteByte( const byte value );
	void					WriteSignedChar( const signed char value );

	void					WriteVec2( const idVec2 &vec );
	void					WriteVec3( const idVec3 &vec );
	void					WriteVec4( const idVec4 &vec );
	void					WriteVec5( const idVec5 &vec );
	void					WriteVec6( const idVec6 &vec );
	void					WriteMat3( const idMat3 &mat );

	void					WriteWinding( const idWinding &winding );
	void					WriteBox( const idBox &box );
	void					WriteBounds( const idBounds &bounds );
	void					WriteAngles( const idAngles &angles );
	void					WriteObject( const idClass *obj );
	void					WriteStaticObject( const idClass &obj );
	void					WriteDict( const idDict *dict );
	void					WriteMaterial( const idMaterial *material );
	void					WriteSkin( const idDeclSkin *skin );
	void					WriteParticle( const idDeclParticle *particle );
	void					WriteFX( const idDeclFX *fx );
	void					WriteSoundShader( const idSoundShader *shader );
	void					WriteModelDef( const class idDeclModelDef *modelDef );
	void					WriteModel( const idRenderModel *model );
	void					WriteUserInterface( const idUserInterface *ui, bool unique );
	void					WriteRenderEntity( const renderEntity_t &renderEntity );
	void					WriteRenderLight( const renderLight_t &renderLight );
	void					WriteRefSound( const refSound_t &refSound );
	void					WriteRenderView( const renderView_t &view );
	void					WriteUsercmd( const usercmd_t &usercmd );
	void					WriteContactInfo( const contactInfo_t &contactInfo );
	void					WriteTrace( const trace_t &trace );
	void					WriteTraceModel( const idTraceModel &trace );
	void					WriteClipModel( const class idClipModel *clipModel );
	void					WriteSoundCommands( void );

	// Write all the data necessary to determine the save format
	void					WriteHeader();

	// Dump the contents of cache buffer to file
	void					FinalizeCache();

private:
	idFile *				file;

	idList<const idClass *>	objects;

	bool					isCompressed;
	CRawVector				cache;

	void					CallSave_r( const idTypeInfo *cls, const idClass *obj );
};

class idRestoreGame {
public:
							idRestoreGame( idFile *savefile );
							~idRestoreGame();

	void					CreateObjects( void );
	void					RestoreObjects( void );
	void					DeleteObjects( void );

	void					Error( const char *fmt, ... ) id_attribute((format(printf,2,3)));

	void					Read( void *buffer, int len );

	void					ReadInt( int &value );
	void					ReadUnsignedInt( unsigned int &value );
	void					ReadShort( short &value );
	void					ReadUnsignedShort( unsigned short &value );
	void					ReadChar( char &value );
	void					ReadUnsignedChar( unsigned char &value );
	void					ReadFloat( float &value );
	void					ReadBool( bool &value );
	void					ReadString( idStr &string );

	void					ReadJoint( jointHandle_t &value );
	void					ReadByte( byte &value );
	void					ReadSignedChar( signed char &value );

	void					ReadVec2( idVec2 &vec );
	void					ReadVec3( idVec3 &vec );
	void					ReadVec4( idVec4 &vec );
	void					ReadVec5( idVec5 &vec );
	void					ReadVec6( idVec6 &vec );
	void					ReadMat3( idMat3 &mat );

	void					ReadWinding( idWinding &winding );
	void					ReadBounds( idBounds &bounds );
	void					ReadBox( idBox &box );
	void					ReadAngles( idAngles &angles );
	void					ReadObject( idClass *&obj );
	void					ReadStaticObject( idClass &obj );
	void					ReadDict( idDict *dict );
	void					ReadMaterial( const idMaterial *&material );
	void					ReadSkin( const idDeclSkin *&skin );
	void					ReadParticle( const idDeclParticle *&particle );
	void					ReadFX( const idDeclFX *&fx );
	void					ReadSoundShader( const idSoundShader *&shader );
	void					ReadModelDef( const idDeclModelDef *&modelDef );
	void					ReadModel( idRenderModel *&model );
	void					ReadUserInterface( idUserInterface *&ui );
	void					ReadRenderEntity( renderEntity_t &renderEntity );
	void					ReadRenderLight( renderLight_t &renderLight );
	void					ReadRefSound( refSound_t &refSound );
	void					ReadRenderView( renderView_t &view );
	void					ReadUsercmd( usercmd_t &usercmd );
	void					ReadContactInfo( contactInfo_t &contactInfo );
	void					ReadTrace( trace_t &trace );
	void					ReadTraceModel( idTraceModel &trace );
	void					ReadClipModel( idClipModel *&clipModel );
	void					ReadSoundCommands( void );

	// Read all the data necessary to determine the save format
	void					ReadHeader();

	// Read the contents of cache buffer before restoring
	void					InitializeCache();

	inline int				GetBuildNumber() { return buildNumber; }
	inline int				GetCodeRevision() { return codeRevision; }

private:
	idFile *				file;

	int						buildNumber;
	int						codeRevision;

	idList<idClass *>		objects;

	bool					isCompressed;
	CRawVector				cache;
	int						cachePointer;

	void					CallRestore_r( const idTypeInfo *cls, idClass *obj );
};

#endif /* !__SAVEGAME_H__*/
