// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __SAVEGAME_H__
#define __SAVEGAME_H__

class idClass;
class idTypeInfo;
class sdUserInterfaceLocal;
class idClipModel;
class idDeclSkin;
class rvDeclEffect;
class idSoundShader;
class idDeclModelDef;
class idRenderModel;
class sdTeamInfo;
class function_t;
struct renderEntity_s;
struct renderLight_s;
struct renderView_s;
struct contactInfo_t;
struct trace_t;

/*

Save game related helper classes.

*/

const int INITIAL_RELEASE_BUILD_NUMBER = 1262;

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
	void					WriteJoint( const jointHandle_t value );
	void					WriteShort( const short value );
	void					WriteByte( const byte value );
	void					WriteSignedChar( const signed char value );
	void					WriteFloat( const float value );
	void					WriteBool( const bool value );
	void					WriteString( const char *string );
	void					WriteVec2( const idVec2 &vec );
	void					WriteVec3( const idVec3 &vec );
	void					WriteVec4( const idVec4 &vec );
	void					WriteVec6( const idVec6 &vec );
	void					WriteWinding( const idWinding &winding );
	void					WriteBounds( const idBounds &bounds );
	void					WriteMat3( const idMat3 &mat );
	void					WriteAngles( const idAngles &angles );
	void					WriteObject( const idClass *obj );
	void					WriteStaticObject( const idClass &obj );
	void					WriteDict( const idDict *dict );
	void					WriteModel( const idRenderModel *model );
	void					WriteRenderEntity( const renderEntity_s& renderEntity );
	void					WriteRenderLight( const renderLight_s& renderLight );
	void					WriteRenderEffect( const renderEffect_s& renderEffect );
	void					WriteRefSound( const refSound_t& refSound );
	void					WriteRenderView( const renderView_s& view );
	void					WriteUsercmd( const usercmd_t& usercmd );
	void					WriteContactInfo( const contactInfo_t& contactInfo );
	void					WriteTrace( const trace_t& trace );
	void					WriteTraceModel( const idTraceModel &trace );
	void					WriteClipModel( const class idClipModel *clipModel );
	void					WriteSoundCommands( void );
	void					WriteTeamInfo( const sdTeamInfo* teamInfo );
	void					WriteFunction( const function_t* function );

	//						decls
	void					WriteMaterial( const idMaterial *material );
	void					WriteSkin( const idDeclSkin *skin );
	void					WriteEffect( const rvDeclEffect *fx );
	void					WriteSoundShader( const idSoundShader *shader );
	void					WriteModelDef( const class idDeclModelDef *modelDef );
	void					WriteVehicleScript( const class sdDeclVehicleScript *vehicleScript );
	void					WriteInvItem( const class sdDeclInvItem *invItem );
	void					WritePlayerClass( const class sdDeclPlayerClass *playerClass );
	void					WriteTable( const class idDeclTable *table );
	void					WriteDamage( const class sdDeclDamage *damage );
	void					WriteToolTip( const class sdDeclToolTip *toolTip );
	void					WriteCampaign( const class sdDeclCampaign *campaign );

	void					WriteBuildNumber( const int value );

private:
	idFile *				file;

	idList<const idClass *>	objects;

	void					CallSave_r( const idTypeInfo *cls, const idClass *obj );
};

class idRestoreGame {
public:
							idRestoreGame( idFile *savefile );
							~idRestoreGame();

	void					CreateObjects( void );
	void					RestoreObjects( void );
	void					DeleteObjects( void );

	void					Error( const char *fmt, ... );

	void					Read( void *buffer, int len );
	void					ReadInt( int &value );
	int						ReadInt( void );
	void					ReadJoint( jointHandle_t &value );
	void					ReadShort( short &value );
	void					ReadByte( byte &value );
	void					ReadSignedChar( signed char &value );
	void					ReadFloat( float &value );
	float					ReadFloat( void );
	void					ReadBool( bool &value );
	bool					ReadBool( void );
	void					ReadString( idStr &string );
	void					ReadVec2( idVec2 &vec );
	void					ReadVec3( idVec3 &vec );
	void					ReadVec4( idVec4 &vec );
	void					ReadVec6( idVec6 &vec );
	void					ReadWinding( idWinding &winding );
	void					ReadBounds( idBounds &bounds );
	void					ReadMat3( idMat3 &mat );
	void					ReadAngles( idAngles &angles );
	void					ReadObject( idClass *&obj );
	void					ReadStaticObject( idClass &obj );
	void					ReadDict( idDict *dict );
	void					ReadModel( idRenderModel *&model );
	void					ReadRenderEntity( renderEntity_t &renderEntity );
	void					ReadRenderLight( renderLight_t &renderLight );
	void					ReadRenderEffect( renderEffect_s &renderEffect );
	void					ReadRefSound( refSound_t &refSound );
	void					ReadRenderView( renderView_t &view );
	void					ReadUsercmd( usercmd_t &usercmd );
	void					ReadContactInfo( contactInfo_t &contactInfo );
	void					ReadTrace( trace_t &trace );
	void					ReadTraceModel( idTraceModel &trace );
	void					ReadClipModel( idClipModel *&clipModel );

	void					ReadSoundCommands( void );
	void					ReadTeamInfo( sdTeamInfo*& teamInfo );
	void					ReadFunction( const function_t*& function );

	//						decls
	void					ReadMaterial( const idMaterial *&material );
	void					ReadSkin( const idDeclSkin *&skin );
	void					ReadEffect( const rvDeclEffect*&fx );
	void					ReadSoundShader( const idSoundShader *&shader );
	void					ReadModelDef( const idDeclModelDef *&modelDef );
	void					ReadVehicleScript( const class sdDeclVehicleScript *&vehicleScript );
	void					ReadInvItem( const class sdDeclInvItem *&invItem );
	void					ReadPlayerClass( const class sdDeclPlayerClass *&playerClass );
	void					ReadTable( const class idDeclTable *&table );
	void					ReadDamage( const class sdDeclDamage *&damage );
	void					ReadToolTip( const class sdDeclToolTip *&toolTip );
	void					ReadCampaign( const class sdDeclCampaign *&campaign );

	void					ReadBuildNumber( void );
 
	//						Used to retrieve the saved game buildNumber from within class Restore methods
	int						GetBuildNumber( void );

private:
	int						buildNumber;

	idFile *				file;

	idList<idClass *>		objects;

	void					CallRestore_r( const idTypeInfo *cls, idClass *obj );
};

#endif /* !__SAVEGAME_H__*/
