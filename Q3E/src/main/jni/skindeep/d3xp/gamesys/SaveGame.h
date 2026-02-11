/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __SAVEGAME_H__
#define __SAVEGAME_H__

#include "framework/BuildVersion.h"
#include "framework/DeclFX.h"
#include "framework/Game.h"
#include "renderer/Model.h"
#include "renderer/RenderSystem.h"
#include "idlib/math/Curve.h"
#include "ui/Rectangle.h"

#include "gamesys/Class.h"

/*

Save game related helper classes.

*/

// each BUILD_NUMBER that can invalidate the previous one without conversion
// blendo eric: rather than using a separate versioning, just use build number
// but keep track of the build numbers that affected savegames below
enum SAVEGAME_VERSIONS
{
	SAVEGAME_VERSION_0000 = 0, // save game changes
	SAVEGAME_VERSION_0001 = 1, // save game changes
	SAVEGAME_VERSION_INVALID = SAVEGAME_VERSION_0001, // the newest version that can no longer be loaded
	SAVEGAME_VERSION_0002 = 2, // wire nades crash
	SAVEGAME_VERSION_0003 = 3, // wire nades fixed
	SAVEGAME_VERSION_0004 = 4, // convert couple of raw entities to idEntityPtr*
	SAVEGAME_VERSION = BUILD_NUMBER,
	SAVEGAME_VERSION_MAX = BUILD_NUMBER
};

#if 0
#if defined(_DEBUG) // savegame var check
	#define SGVAR_CHECK_WRITE( inName ) savefile->WriteCheckString(#inName)
	#define SGVAR_CHECK_READ( inName ) savefile->ReadCheckString(#inName)
#else
	#define SGVAR_WRITE( inName )
	#define SGVAR_WRITE( inName )
#endif
#endif

#if !defined(CastClassPtrRef)
#define CastClassPtrRef(var) reinterpret_cast<idClass *&>(var)
#define CastWriteVoidPtrPtr(var) reinterpret_cast<const void*>( var )
#define CastReadVoidPtrPtr(var) reinterpret_cast<void**>( &var )
#define CastWriteSGPtrPtr(var) reinterpret_cast<const idSaveGamePtr* const *>( &var )
#define CastReadSGPtrPtr(var) reinterpret_cast<idSaveGamePtr**>( &var )
#endif

#if !defined(SaveFileWriteArray)
#define SaveFileWriteArray(list, lnum, funcname) { savefile->WriteInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname(list[idx]); } savefile->WriteCheckSizeMarker(); }
#define SaveFileWriteArrayCast(list, lnum, funcname, castname) { savefile->WriteInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname( reinterpret_cast<castname>( list[idx] )); } savefile->WriteCheckSizeMarker(); }
#define SaveFileWriteList(list, funcname) { savefile->WriteInt( list.Num() ); for( int idx = 0; idx < list.Num(); idx++ ){ savefile->funcname(list[idx]); } savefile->WriteCheckSizeMarker(); }

#define SaveFileReadArray(list, funcname) { int lnum; savefile->ReadInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname(list[idx]); } savefile->ReadCheckSizeMarker();  }
#define SaveFileReadArrayCast(list, funcname, castname) { int lnum; savefile->ReadInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname( reinterpret_cast<castname>( list[idx] )); } savefile->ReadCheckSizeMarker();  }
#define SaveFileReadList(list, funcname) { int lnum; savefile->ReadInt( lnum ); list.SetNum(lnum); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname(list[idx]); } savefile->ReadCheckSizeMarker(); }
#define SaveFileReadListCast(list, funcname, castname) { int lnum; savefile->ReadInt( lnum ); list.SetNum(lnum); for( int idx = 0; idx < lnum; idx++ ){ savefile->funcname(reinterpret_cast<castname>( list[idx] )); } savefile->ReadCheckSizeMarker(); }

// uses += operator overloaded saving, for very specific one off types
#define SaveFileWriteListCast(list, funcname, castname) { savefile->WriteInt( list.Num() ); for( int idx = 0; idx < list.Num(); idx++ ){ savefile->funcname( reinterpret_cast<castname>( list[idx] )); } savefile->WriteCheckSizeMarker(); }
#define SaveFileWriteArrayConcat(list, lnum) { savefile->WriteInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile += list[idx]; } savefile->WriteCheckSizeMarker(); }
#define SaveFileReadArrayConcat(list) { int lnum; savefile->ReadInt( lnum ); for( int idx = 0; idx < lnum; idx++ ){ savefile += list[idx]; } savefile->ReadCheckSizeMarker();  }
#define SaveFileReadListConcat(list) { int lnum; savefile->ReadInt( lnum ); list.SetNum(lnum); for( int idx = 0; idx < lnum; idx++ ){ savefile += list[idx]; } savefile->ReadCheckSizeMarker();  }

#if defined(_DEBUG)
#define WriteCheckFileValue() { if (sg_debugchecks.GetBool() ) { int checkVal = (int)(INT_PTR)&typeid(this); savefile->WriteInt( checkVal ); } }
#define ReadCheckFileValue() { if (sg_debugchecks.GetBool() ) { int checkVal; savefile->ReadInt(checkVal); assert( checkVal == (int)(INT_PTR)&typeid(this) ); } }
#else
#define WriteCheckFileValue( )
#define ReadCheckFileValue( ) 
#endif
#endif

template <class T> class idEntityPtr;
class idScriptObject;
class idAngles;
class idDict;
class idQuat;
class idDeclEntityDef;
class function_t;
class idEventDef;
class idAnimator;

// blendo eric: debug verification for each val
enum SG_CHECK_TYPES
{
	SG_CHECK_START = 1,
	SG_CHECK_BUFFER,
	SG_CHECK_INT,
	SG_CHECK_UINT,
	SG_CHECK_BYTE,
	SG_CHECK_SHORT,
	SG_CHECK_STRING,
	SG_CHECK_FLOAT,
	SG_CHECK_SCHAR,
	SG_CHECK_BOOL,
	SG_CHECK_VEC2,
	SG_CHECK_VEC3,
	SG_CHECK_VEC4,
	SG_CHECK_VEC6,
	SG_CHECK_QUAT,
	SG_CHECK_MAT,
	SG_CHECK_JOINT,
	SG_CHECK_BOUNDS,
	SG_CHECK_WINDING,
	SG_CHECK_ANGLES,
	SG_CHECK_OBJECT,
	SG_CHECK_ENTITY,
	SG_CHECK_DICT,
	SG_CHECK_MATERIAL,
	SG_CHECK_SKIN,
	SG_CHECK_PARTICLE,
	SG_CHECK_FX,
	SG_CHECK_SOUND,
	SG_CHECK_MODEL,
	SG_CHECK_UI,
	SG_CHECK_RENDER,
	SG_CHECK_LIGHT,
	SG_CHECK_CMD,
	SG_CHECK_PHYS,
	SG_CHECK_LIST,
	SG_CHECK_FUNC,
	SG_CHECK_CURVE,
	SG_CHECK_CALLSAVE,
	SG_CHECK_SGPTR,
	SG_CHECK_CLASS,
	SG_CHECK_END
};

class idSaveGamePtr; // helper class, inheriting from this will help track it in savegame

// blendo eric: associates a index id with a ptr, so instanced members and misc ptrs can be hooked up
template< class ObjTypePtr > struct fixup_ptr_t {
	int index;
	ObjTypePtr * objPtrPtr;
	fixup_ptr_t(int nIndex, ObjTypePtr * newPtrPtr) : index(nIndex), objPtrPtr(newPtrPtr) {}
	fixup_ptr_t() : index(0), objPtrPtr(0) {}
};
typedef fixup_ptr_t<idClass*> ent_fixup_ptr_t;
typedef fixup_ptr_t<void*> misc_fixup_ptr_t;


struct sg_callSaveID_t { // store objects that are CallSaved, for debugging purposes
	sg_callSaveID_t() : _type( 0 ), _obj( nullptr ) {}
	sg_callSaveID_t( int typeNum, const idClass* obj ) : _type( typeNum ), _obj( obj ) {}
	int _type;
	const idClass* _obj;
	ID_INLINE bool operator==(const sg_callSaveID_t& b) const {
		return (_type == b._type) && ( _obj == b._obj );
	}
};


class idSaveGame {
private:

	idSaveGame( idFile *savefile, int saveVer = SAVEGAME_VERSION, int entityNum = 0, int threadNum = 0 ); // use Begin, to always explcitly write out save info 
	void WriteSaveInfo();

public:

	static idSaveGame		Begin(idFile* savefile, int saveVer = SAVEGAME_VERSION, int entityNum =  0, int threadNum = 0 );
							~idSaveGame();

	void					AddObjectToList( const idClass *obj, int listIndex ); // add objects from (tracked) game entity list
	void					WriteObjectListTypes( void ); // writes out the class types, so they can be instantiated before restore
	bool					WriteObjectListData( void ); // write out the full object data

	void					Write( const void *buffer, int len );
	void					WriteInt( const int value );
	void					WriteUInt( const uint value );
	void					WriteHandle(const qhandle_t value) { WriteInt((int)value); } // blendo eric: cast helper
	void					WriteJoint( const jointHandle_t value );
	void					WriteShort( const short value );
	void					WriteByte( const byte value );
	void					WriteSignedChar( const signed char value );
	void					WriteFloat( const float value );
	void					WriteLong( const long value );
	void					WriteBool( const bool value );
	void					WriteString( const char *string );
	void					WriteVec2( const idVec2 &vec );
	void					WriteVec3( const idVec3 &vec );
	void					WriteVec4( const idVec4 &vec );
	void					WriteVec6( const idVec6 &vec );
	void					WriteQuat( const idQuat &vec );
	void					WriteRect( const idRectangle& rect ) { WriteVec4( const_cast<idVec4&>(rect.ToVec4()) ); }
	void					WriteWinding( const idWinding &winding );
	void					WriteBounds( const idBounds &bounds );
	void					WriteMat3( const idMat3 &mat );
	void					WriteAngles( const idAngles &angles );
	void					WriteObject( const idClass *obj );
	void					WriteStaticObject( const idClass &obj );
	void					WriteMiscPtr( const void * ptr ); // blendo eric: mark this ptr as needing to be filled on post restore
	void					WriteSGPtr( const idSaveGamePtr* ptr ) { WriteMiscPtr(ptr); }
	void					TrackMiscPtr( const void * ptr ); // blendo eric: track this ptr, so it can fill in other ptrs on post restore
	void					TrackSGPtr( const idSaveGamePtr * ptr ) { TrackMiscPtr(ptr); }
	void					WriteObject(const idEntityPtr<idEntity>& obj);// blendo eric: cast helper
	void					WriteScriptObject(const idScriptObject& obj); // blendo eric: cast helper
	void					WriteAnimatorPtr( const idAnimator *obj ); // blendo eric: cast helper
	void					WriteDict( const idDict *dict );
	void					WriteMaterial( const idMaterial *material );
	void					WriteSkin( const idDeclSkin *skin );
	void					WriteParticle( const idDeclParticle *particle );
	void					WriteFX( const idDeclFX *fx );
	void					WriteSoundShader( const idSoundShader *shader );
	void					WriteModelDef( const class idDeclModelDef *modelDef );
	void					WriteModel( const idRenderModel *model );
	void					WriteUserInterface( const idUserInterface *ui, bool unique, bool forceShared );
	//void					WriteUserInterface( const idUserInterface *ui ); // blendo eric: default ask ui if its unique
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

	// blendo eric: helpers
	void					WriteEntityDef( const idDeclEntityDef * ent );
	void					WriteVecX( idVecX& vec );
	void					WriteMatX( idMatX& mat );
	void					WriteFloatArr( const float * arr, int size );
	void					WriteListObject( idList<idEntity*>& list );
	void					WriteListString( idList<idStr>& list );
	void					WriteFunction( const function_t * func );
	void					WriteEventDef( const idEventDef * eventFlag );
	void					WriteCurve( idCurve_Spline<idVec3>* curve );

	// blendo eric: test / debug helpers
	void					WriteCheckSizeMarker();
	void					WriteCheckString(idStr checkStr);
	void					WriteCheckType(char checkVal);

#if 0
	void WriteLinkedList(idLinkList<idEntity> &entList) {
		int num = entList.Num();
		WriteInt( num );
		idLinkList<idEntity> * node = entList.ListHead();
		for( int idx = 0; idx < num; ++idx ) {
			WriteObject( node->Owner() );
			node = node->NextNode();
		}
	}
#endif

private:
	idFile *				file;
	int						saveVersion;
	bool					saveDebugging;

	int						objectsGameEntitiesNum_w;
	int						objectsGameThreadsNum_w;

	idList<const idClass *>	objectsGame_w; // indexed idGame entities
	idList<int>				objectsGameOrder_w; // the order the entities are saved and written, which may be different from their idGame::entities index

	int						objectsGamePtrCount_w; // ptrs referring to idGame entities

	// blendo eric note: instanced member entities/idClasses are called "StaticObjects" in this code,
	//						referring to ents that aren't spawned/alloc'd/tracked by gamelocal,
	//						but is not "static" in the c++ sense

	// blendo eric: create handles to instance member entities
	//				by associating an index in the objectStatics array to an untracked entity ptr
	idList<const idClass *>	objectsStatic_w; // untracked instanced member entities 
	idList<int>				objectsStaticStored_w; // whether the object actually written by its owner

	int						objectsStaticPtrCount_w = 0; // ptrs referring to member instanced objects
	int						objectsStaticStoreCount_w = 0; // stored member instanced objects

	idList<const void *>	objectsMisc_w;		// untracked non-idClass objects
	idList<int>				objectsMiscStored_w; // whether the misc ptr was actually hooked up by its owner

	int						objectsMiscPtrCount_w = 0; // ptrs referring to non idClass objects
	int						objectsMiscStoreCount_w = 0; // ptrs referring to non idClass objects

	idList<const idUserInterface*>	objectsUIDangling_w; 

	idList<const idUserInterface*>	objectsUI_w; 
	idList<bool>					objectsUIValid_w; 
	idList<bool>					objectsUITagged_w; 

	idList<sg_callSaveID_t> objectsCalledSave_w;

	// blendo eric: wrapper for the full object, rather than each sub type in recursive CallSave_r 
	void					CallSave( const idTypeInfo *cls, const idClass *obj );

	void					CallSave_r( const idTypeInfo *cls, const idClass *obj );

	void					DebugCheckObjectList(); // check lists once all objects are written
};



class idRestoreGame {
private:
							idRestoreGame( idFile *savefile ); // force reading of save info on creation
	void					ReadSaveInfo( void );
public:
	static idRestoreGame	Begin( idFile *savefile );
							~idRestoreGame();

	void					ReadAndCreateObjectsListTypes( void );
	bool					ReadAndRestoreObjectsListData( void );

	void					DeleteObjects( void );


	void					Warn( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	void					Error( const char *fmt, ... ) id_attribute((format(printf,2,3)));

	void					Read( void *buffer, int len );
	void					ReadInt( int &value );
	void					ReadUInt( uint &value );
	void					ReadHandle( qhandle_t& value ) { ReadInt(value); } // blendo eric: cast helper
	void					ReadJoint( jointHandle_t &value );
	void					ReadShort( short &value );
	void					ReadByte( byte &value );
	void					ReadSignedChar( signed char &value );
	void					ReadFloat( float &value );
	void					ReadLong( long &value );
	void					ReadBool( bool &value );
	void					ReadString( idStr & string );
	void					ReadString( const char *& string );
	void					ReadCharArray( char * chars );
	void					ReadVec2( idVec2 &vec );
	void					ReadVec3( idVec3 &vec );
	void					ReadVec4( idVec4 &vec );
	void					ReadVec6( idVec6 &vec );
	void					ReadQuat( idQuat &quat );
	void					ReadRect( idRectangle& rect ) { ReadVec4( (idVec4&)(rect[0]) ); }
	void					ReadWinding( idWinding &winding );
	void					ReadBounds( idBounds &bounds );
	void					ReadMat3( idMat3 &mat );
	void					ReadAngles( idAngles &angles );
	void					ReadObject( idClass*& obj );
	void					ReadObjectPtr( idClass ** obj );
	void					ReadStaticObject( idClass &obj );
	void					ReadMiscPtr( void** ptr ); // blendo eric: a pointer to be hooked up later
	void					ReadSGPtr( idSaveGamePtr** ptr ) { ReadMiscPtr((void**)ptr); } // blendo eric: a pointer to be hooked up later
	void					TrackMiscPtr( void* ptr ); // blendo eric: a pointer to be indexed, so it can fill in pointer refs later
	void					TrackSGPtr( idSaveGamePtr* ptr ) { TrackMiscPtr((void*)ptr); } // blendo eric: a pointer to be hooked up later
	void					ReadObject(idEntity*& obj) { ReadObject(reinterpret_cast<idClass*&>(obj)); } // blendo eric: cast helper
	void					ReadObject(idEntityPtr<idEntity>& obj); // blendo eric: cast helper
	void					ReadScriptObject(idScriptObject& obj); // blendo eric: cast helper
	void					ReadAnimatorPtr( idAnimator *&obj ); // blendo eric: cast helper
	void					ReadDict( idDict & dict ); // blendo eric: cast helper
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
	//void					ReadRenderEntity( renderEntity_t &renderEntity, qhandle_t& handle);
	//void					ReadRenderLight( renderLight_t &renderLight, qhandle_t& handle );
	void					ReadRefSound( refSound_t &refSound );
	void					ReadRenderView( renderView_t &view );
	void					ReadUsercmd( usercmd_t &usercmd );
	void					ReadContactInfo( contactInfo_t &contactInfo );
	void					ReadTrace( trace_t &trace );
	void					ReadTraceModel( idTraceModel &trace );
	void					ReadClipModel( idClipModel *&clipModel );
	void					ReadSoundCommands( void );

	//						Used to retrieve the saved game saveVersion from within class Restore methods
	int						GetSaveVersion() { return saveVersion; }
	bool					SaveVersionValid() { return saveVersion >= SAVEGAME_VERSION_INVALID; }
	idStr					GetFileName();

	// blendo eric: helpers
	void					ReadEntityDef( const idDeclEntityDef *& ent );
	void					ReadVecX( idVecX& vec );
	void					ReadMatX( idMatX& mat );
	void					ReadFloatArr( float * arr );
	void					ReadListObject( idList<idEntity*>& list );
	void					ReadListString( idList<idStr>& list );
	void					ReadFunction( const function_t *& func );
	void					ReadEventDef( const idEventDef *& eventFlag );
	void					ReadCurve( idCurve_Spline<idVec3>*& curve );


	// blendo eric: test / debug helpers
	bool					ReadCheckString(const char * checkStr);
	bool					ReadCheckSizeMarker();
	bool					ReadCheckType(char checkType);

#if 0

	template <typename T,typename C> void ReadLinkedList(idLinkList<T> &list)
	{
		int num;
		file->ReadInt(num);
		T* head;
		ReadObject(head);
		list.SetOwner(head);
		idLinkList<T>* prevNode = nullptr;
		idLinkList<T>* curNode = headNode;
		for (int idx = 0; idx <  list.Num() && curNode; idx++)
		{
			list.Set
			T* next;
			ReadObject(next);

			curNode  
		}
	}
#endif

private:
	// blendo eric: wrapper for the full object, rather than each sub type in recursive CallRestore_r
	bool					CallRestore( const idTypeInfo *cls, idClass *obj ); 
	bool					CallRestore_r( const idTypeInfo *cls, idClass *obj );

	void					DebugCheckObjectLists();

	idFile *				file = nullptr;

	int						saveVersion = 0;
	bool					saveDebugging = false;

	idList<idClass *>		objectsGame_r;
	idList<int>				objectsGameOrder_r;

	idList<idClass *>		objectsStatic_r; // instanced member entities
	idList<void *>			objectsMisc_r; // non class objects

	// blendo eric: the pointers to each instanced member entities index/handle
	//				which will be used to populate missing pointers to instanced member entities
	//				filled in after all instanced member entities ("staticobjects") are read


	idList<ent_fixup_ptr_t>	 objectsStaticFixUp_r;  // instanced member entities
	idList<misc_fixup_ptr_t> objectsMiscFixUp_r; // non class objects

	idList<idUserInterface*> objectsUI_r;

	int						objectsGameEntitiesNum_r = 0;
	int						objectsGameThreadsNum_r = 0;

	int						objectsGamePtrCount_r = 0;
	int						objectsStaticPtrCount_r = 0;
	int						objectsMiscPtrCount_r = 0;

};

class idSaveGamePtr { // blendo eric: try to keep this free of data, since it might be inherited by tiny data structs
public:

	virtual void WriteToSaveGame( idSaveGame *savefile ) const {
		savefile->TrackSGPtr(this);
		savefile->WriteCheckSizeMarker();
	}

	virtual void ReadFromSaveGame( idRestoreGame *savefile ) {
		savefile->TrackSGPtr(this);
		savefile->ReadCheckSizeMarker();
	}
};



extern idCVar sg_debugchecks;

#endif /* !__SAVEGAME_H__*/
