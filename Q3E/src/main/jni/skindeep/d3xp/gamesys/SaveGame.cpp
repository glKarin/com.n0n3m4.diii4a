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

#include "sys/platform.h"
#include "framework/BuildVersion.h"
#include "framework/DeclSkin.h"
#include "framework/DeclEntityDef.h"
#include "renderer/ModelManager.h"

#include "idlib/geometry/JointTransform.h"
#include "ui/UserInterfaceLocal.h"
#include "ui/Window.h"
#include "physics/Clip.h"
#include "Entity.h"
#include "Game_local.h"

#include "SaveGame.h"


idCVar sg_debugchecks("sg_debugchecks", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_CHEAT, "write savegame file checks, requires debug build");
idCVar sg_debugdump("sg_debugdump", "0", CVAR_INTEGER | CVAR_SYSTEM | CVAR_CHEAT, "savegame file info dump");

#define SG_OBJ_MATCHINDEX

//#define SG_DEFERRED
#define SG_OBJ_ORDERED

#define SG_ALWAYS_READ_CHECKSTRING

#if defined(_DEBUG)
#define SG_VERIFY_DATA
#endif

const void * SG_UNTRACKED_PTR = (void*)0xbabababadadadada;

#if defined(SG_OBJ_MATCHINDEX)
const int SG_NULL_INDEX = 0xaaaaaaaa; // represents a null ptr in fixups
#else
const int SG_NULL_INDEX = 0; // represents a null ptr in fixups
#endif

const char * IsBadMemoryAddress_Sub( const void * ptr )
{
	if ( ptr == 0 )
	{
		return nullptr;
	}

	if ( ptr == (void*)(uint64)0xcccccccc || ptr == (void*)0xcccccccccccccccc )
	{
		return "uninitialized stack memory";
	}

	if ( ptr == (void*)(uint64)0xbaadf00d  ||  ptr == (void*)0xbaadf00dbaadf00d
		|| ptr == (void*)(uint64)0xcdcdcdcd || ptr == (void*)0xcdcdcdcdcdcdcdcd )
	{
		return "uninitialized heap memory";
	}

	if ( ptr == (void*)(uint64)0xdeadbeef || ptr == (void*)0xdeadbeefdeadbeef
		|| ptr == (void*)(uint64)0xdddddddd || ptr == (void*)0xdddddddddddddddd
		|| ptr == (void*)(uint64)0xfefefefe || ptr == (void*)0xfefefefefefefefe
		|| ptr == (void*)(uint64)0xfeeefeee || ptr == (void*)0xfeeefeeefeeefeee )
	{
		return "deallocated memory ";
	}

	if ( ptr == (void*)(uint64)0xabababab || ptr == (void*)(uint64)0xbdbdbdbd 
		|| ptr == (void*)(uint64)0xfdfdfdfd )
	{
		return "out of range memory";
	}
#if defined(_WIN32)
	if ( ptr > (void*)(uint64)0x00FFFFFFFFFFFFFF ) // more than 64gb
	{
		return "out of range memory (large ptr address)";
	}
	else if ( ptr < (void*)(uint64)0x0000000000010000 ) // less than 64kb, min app space
	{
		return "out of range memory (large ptr address)";
	}
#endif

	if ( ptr == SG_UNTRACKED_PTR )
	{
		return "idSaveGame missing stored ptr";
	}

	return nullptr;
}
const char* IsBadMemoryAddress( const void* ptr, bool checkOnlyPtr = false )
{
	if (ptr == 0)
	{
		return nullptr;
	}

	const char * badMem = IsBadMemoryAddress_Sub( ptr );
	if ( badMem )
	{
		return va( "bad ptr address (%p): %s", ptr, badMem);
	}

	if (checkOnlyPtr)
	{ // skip data check
		return nullptr;
	}

	void * data = (void*) *reinterpret_cast<const uint64*>(ptr); // reading this as a ptr type, but its just arbitrary data
	badMem = IsBadMemoryAddress_Sub( data ); // check if the mem pointed to is fine too

	if ( badMem ) 
	{
		return va( "ptr (%p) to bad data (%p): %s", ptr, data, badMem);
	}

	return nullptr;
}

#if defined(_DEBUG)
struct StackWriter {
	static int stackDepth;
	static idStr stackStr;
	static idStr stackLineTabs;
	static idStr stackStrLast;
	static int stackDepthLast;
	static int hack;
	StackWriter(const char* line) {
		stackDepth++;
		stackLineTabs += "| ";
		AddLine( line );
	}
	~StackWriter() {
		stackDepth--;
		stackLineTabs.CapLength( stackLineTabs.Length() - 2 );
		//if (stackDepth < 0 && sg_debugdump.GetInteger() >= 1) {
		//	idStr nextLine = stackLineTabs + "|\n";
		//	stackStr += nextLine;
		//	if (sg_debugdump.GetInteger() >= 2) {
		//		gameLocal.Printf("%s", nextLine.c_str());
		//	}
		//}
	}
	StackWriter() = delete;
	StackWriter& operator=(const StackWriter&) = delete;
	void AddLine(const char* line) {
		if (sg_debugdump.GetInteger() >= 1) {
			assert(stackDepth >= 0);
			if ((stackStrLast.Icmp(line) == 0) && (stackDepth == stackDepthLast)) {
				stackStr += ".";
				if (sg_debugdump.GetInteger() >= 2) {
					gameLocal.Printf(".");
				}
			}
			else {
				idStr nextLine = stackLineTabs + idStr("> ") + line;
				stackStr += nextLine;

				if (sg_debugdump.GetInteger() >= 2) {
					gameLocal.Printf("%s", nextLine.c_str());
				}
			}

			stackStrLast = line;
			stackDepthLast = stackDepth;
		}
	}
};

int StackWriter::stackDepth = -1;
idStr StackWriter::stackStr = idStr();
idStr StackWriter::stackLineTabs = idStr("\nSG");
idStr StackWriter::stackStrLast = idStr();
int StackWriter::stackDepthLast = 0;
int StackWriter::hack = 0;


#define RecordSaveStack( str ) StackWriter sWriter( str ); sWriter.hack++;
#define ContinueSaveStack( str ) sWriter.AddLine( str ); sWriter.hack++;
#else
#define RecordSaveStack( str )
#define ContinueSaveStack( str )
#endif


#if defined(_DEBUG)
void SG_DumpStack() {
	if (sg_debugdump.GetInteger() >= 1) {

		for (int subStart = 0; subStart < StackWriter::stackStr.Length(); subStart += 1000)
		{
			int len = Min(subStart + 1000,StackWriter::stackStr.Length()) - subStart;
			StackWriter::stackStr.Mid(subStart, len);
			gameLocal.Printf("%s", StackWriter::stackStr.Mid(subStart, len).c_str() );
		}
		gameLocal.Printf("\n" );
	}
}
#endif

void SG_Warn( const char* fmt, ... ) id_attribute((format(printf,1,2))) {
	va_list args;
	va_start(args, fmt);

#if defined(_DEBUG)
	if (sg_debugdump.GetInteger() >= 2) {

		SG_DumpStack();
		gameLocal.Printf("SaveGame: ");
		gameLocal.Printf(fmt, args);
		DebugBreak();
	}
#endif

	gameLocal.Warning(fmt, args); 
	va_end(args);
}

void SG_Error( const char *fmt, ... ) id_attribute((format(printf,1,2))) {
	va_list args;
	va_start(args, fmt);

#if defined(_DEBUG)
	if (sg_debugdump.GetInteger() >= 1) {
		SG_DumpStack();
		gameLocal.Printf("SaveGame: ");
		gameLocal.Printf(fmt, args);
		DebugBreak();
	}
#endif
	gameLocal.Error(fmt, args);
	va_end(args);
}


void SG_Print( const char *fmt, ... ) id_attribute((format(printf,1,2))) {
	va_list args;
	va_start(args, fmt);
	gameLocal.Printf(fmt, args);
	va_end(args);
}

/*
================
idSaveGame::idSaveGame()
================
*/
idSaveGame::idSaveGame( idFile *savefile, const int saveVer, int entityNum, int threadNum ) {
	file = savefile;
	saveVersion = saveVer;
	saveDebugging = sg_debugchecks.GetBool();

	objectsGamePtrCount_w = 0;
	objectsStaticPtrCount_w = 0;
	objectsMiscPtrCount_w = 0;

	objectsGameEntitiesNum_w = entityNum;
	objectsGameThreadsNum_w = threadNum;

	if ( entityNum > 0 || threadNum > 0 ) {
		int gameEntityNum = entityNum + threadNum; // + 1 if using nullptr at 0 index
		int otherObjNum = gameEntityNum /2; // guessing here

#if !defined(SG_OBJ_MATCHINDEX)
		gameEntityNum += 1; // add +1 to gameEntityNum to make room for nullptr obj
#endif

		objectsGame_w.AssureNum( gameEntityNum, (idClass*)SG_UNTRACKED_PTR );
		objectsGameOrder_w.Resize( gameEntityNum );

		objectsStatic_w.Resize( otherObjNum );
		objectsStatic_w.SetGranularity( 1024 );
		objectsStaticStored_w.Resize( otherObjNum );
		objectsStaticStored_w.SetGranularity( 1024 );

		objectsMisc_w.Resize( otherObjNum );
		objectsMisc_w.SetGranularity( 1024 );
		objectsMiscStored_w.Resize( otherObjNum );
		objectsMiscStored_w.SetGranularity( 1024 );

#if  !defined(SG_OBJ_MATCHINDEX)
		// Put NULL at the start of the list so we can skip over it.
		objectsGame_w[0] = nullptr;
		objectsGameOrder_w.Append(0); // match corresponding 0 idx nullptr
#endif

		objectsStatic_w.Append(nullptr);
		objectsMisc_w.Append(nullptr);
		objectsStaticStored_w.Append(0); // match corresponding 0 idx nullptrs as valid
		objectsMiscStored_w.Append(0);  // match corresponding 0 idx nullptrs as valid
	}
}

/*
======================
idSaveGame::WriteSaveInfo
======================
*/
void idSaveGame::WriteSaveInfo() {
	file->WriteInt( saveVersion );
	file->WriteBool( saveDebugging );
}

/*
================
idSaveGame::Begin()
================
*/
idSaveGame idSaveGame::Begin( idFile *savefile, const int saveVer, int gameEntityNum, int threadNum ) {
	idSaveGame save( savefile, saveVer, gameEntityNum, threadNum);
	save.WriteSaveInfo();
	return save;
}

/*
================
idSaveGame::~idSaveGame()
================
*/
idSaveGame::~idSaveGame() {
	// SM: This can happen on an autosave now, since we don't save any objects
	// assert( objectsGame_w.Num() == 0 );
}

/*
================
idSaveGame::AddObjectToList
================
*/
void idSaveGame::AddObjectToList( const idClass *obj, int listIndex ) {
#if !defined(SG_OBJ_MATCHINDEX )
	listIndex += 1;
#endif
	assert( objectsGame_w[listIndex] == SG_UNTRACKED_PTR );
	assert( listIndex < objectsGame_w.Num() );

	if ( obj != nullptr && !obj->IsRemoved() ) {
		if( const char * badDataType = IsBadMemoryAddress( obj ) ) {
			SG_Warn("idSaveGame::AddObjectToList() added obj: %s ", badDataType);
		}

		assert( objectsGameOrder_w.FindIndex( listIndex )  < 0 );

		objectsGame_w[listIndex] = obj;
		objectsGameOrder_w.Append(listIndex);
	}
	else
	{
		objectsGame_w[listIndex] = nullptr;
	}
}

/*
================
idSaveGame::WriteObjectListTypes
================
*/
void idSaveGame::WriteObjectListTypes(void) {
	RecordSaveStack("WriteObjectListTypes");
	assert(objectsGame_w.Num() >= objectsGameOrder_w.Num());

	WriteInt(objectsGameEntitiesNum_w);
	WriteInt(objectsGameThreadsNum_w);

#if defined(SG_OBJ_MATCHINDEX)
	WriteInt(objectsGameOrder_w.Num());
	for (int i = 0; i < objectsGameOrder_w.Num(); i++) {
		int entIdx = objectsGameOrder_w[i];
		WriteInt(entIdx);
		WriteString(objectsGame_w[entIdx]->GetClassname());
		WriteCheckSizeMarker();
	}
#else
	WriteInt(objectsGame_w.Num() - 1);
	for (int i = 1; i < objectsGame_w.Num(); i++) {
		WriteString(objectsGame_w[i]->GetClassname());

		WriteCheckSizeMarker();
	}
#endif

	WriteCheckSizeMarker();

#if defined(UI_DEFERRED)
	ContinueSaveStack( "guis write list" );
	// info to allow creation

	idList<idUserInterface*>& guiList = uiManager->GetGuis();
	WriteInt(guiList.Num());
	objectsUI_w.AssureNum( guiList.Num(), nullptr );
	objectsUITagged_w.AssureNum( guiList.Num(), false );
	objectsUIValid_w.AssureNum( guiList.Num(), false );
	for( int idx = 0; idx < guiList.Num(); idx++ ) {
		idUserInterface* ui = guiList[idx];

		objectsUI_w[idx] = ui;

		WriteString( ui->Name() );
		WriteBool( ui->IsUniqued() ); 
		WriteString( ui->GetGroup() );

		bool isMenu = (reinterpret_cast<const idUserInterfaceLocal*>(ui)->GetDesktop()->GetFlags() & WIN_MENUGUI) > 0;
		bool isMenuMap = ui->GetGroup().Icmp("mainmenu") == 0;
		bool validUI = !isMenuMap; // !isMenu && !isMenuMap;

		objectsUIValid_w[idx] = validUI;

		WriteBool( validUI );
		if (!validUI && sg_debugdump.GetInteger() >= 1) {
			SG_Print("Rejected UI %s group:%s (is menu %s) unique:%s", ui->Name(), ui->GetGroup().c_str(), isMenu ? "y" : "n", ui->IsUniqued() ? "y":"n" );
		}
	}
	WriteCheckSizeMarker();
#endif
}

/*
================
idSaveGame::WriteObjectListData
================
*/
bool idSaveGame::WriteObjectListData( void ) {
	RecordSaveStack( "WriteObjectListData" );

	WriteSoundCommands();

	WriteCheckString("post sound commands");

	// read trace models
	ContinueSaveStack( "SaveTraceModels" );
	idClipModel::SaveTraceModels( this );

	WriteCheckString("post trace models");

#if defined(SG_OBJ_MATCHINDEX)
	for( int i = 0; i < objectsGameOrder_w.Num(); i++ ) {
		int entIdx = objectsGameOrder_w[i];
		CallSave( objectsGame_w[ entIdx ]->GetType(), objectsGame_w[ entIdx ] ); // blendo eric: use callsave wrapper for check type purposes
	}
#else
	for( int i = 1; i < objectsGame_w.Num(); i++ ) {
		CallSave( objectsGame_w[ i ]->GetType(), objectsGame_w[ i ] ); // blendo eric: use callsave wrapper for check type purposes
	}
#endif

	WriteCheckString("post objects");


#if defined(UI_DEFERRED) // write out ordered UI list
	ContinueSaveStack( "guis write" );
	idStr curGroupName = uiManager->GetGroup();
	WriteInt(objectsUI_w.Num());
	for( int idx = 0; idx< objectsUI_w.Num(); idx++ ) {
		assert( objectsUI_w[idx] != nullptr );
		const idUserInterfaceLocal * ui = static_cast<const idUserInterfaceLocal*>(objectsUI_w[idx]);

		//bool isMenu = (ui->GetDesktop()->GetFlags() & WIN_MENUGUI) > 0;
		bool menuMap = ui->GetGroup().Icmp("mainmenu") == 0;
		bool validUI = !menuMap;// (!isMenu && !menuMap);
		WriteBool( validUI );
		if ( validUI ) {
			objectsUI_w[idx]->WriteToSaveGame(this);
		}

		if (validUI != objectsUITagged_w[idx] && sg_debugdump.GetInteger() >= 1)
		{
			SG_Print("Mismatch UI tag");
		}
	}

	WriteCheckString("post guis");
#endif


	if (saveDebugging)
	{
		WriteInt(gameRenderWorld->EntityDefNum());
		WriteInt(gameRenderWorld->LightDefNum());
	}

	// we don't need to save out static/misc objects here, unlike GENTITIES, as they should have been written out on CallSave above

	DebugCheckObjectList();

	objectsGame_w.Clear();
	objectsStatic_w.Clear();
	objectsMisc_w.Clear();

#ifdef ID_DEBUG_MEMORY
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ), false ) );
#endif

	return true;
}

/*
================
idSaveGame::DebugCheckObjectList
================
*/
void idSaveGame::DebugCheckObjectList()
{
	// ----------------
	// DEBUG CHECKS

	assert( objectsStatic_w.Num() == objectsStaticStored_w.Num() );
	assert( objectsMisc_w.Num() == objectsMiscStored_w.Num() );

	for (int idx = 1; idx < objectsStatic_w.Num(); idx++) {
		if( const char * badDataType = IsBadMemoryAddress( objectsStatic_w[idx] ) ) {
			SG_Warn("idSaveGame::DebugCheckObjectList() %s ", badDataType);
			break;
		}

#if defined(SG_VERIFY_DATA)
		if( objectsStaticStored_w[idx] <= 0 ) {
			const idClass * miscPtr = objectsStatic_w[idx];
			const char* className = miscPtr ? miscPtr->GetClassname() : "";
			SG_Warn("idSaveGame::DebugCheckObjectList() a static-ent ptr (%s) was never tracked/stored by its owner, and therefore can't be hooked up on restore", className);
		}
#endif
	}


	assert( objectsMiscStored_w.Num() == objectsMisc_w.Num() );
	for (int idx = 1; idx < objectsMiscStored_w.Num(); idx++) {
		if( const char * badDataType = IsBadMemoryAddress( objectsMisc_w[idx] ) ) {
			SG_Warn("idSaveGame::DebugCheckObjectList() %s ", badDataType);
			break;
		}
#if defined(SG_VERIFY_DATA)
		if (objectsMiscStored_w[idx] <= 0) {
			const void* ptr = objectsMisc_w[idx];
			SG_Warn("idSaveGame::DebugCheckObjectList() misc ptr (%p) was never stored by its owner!", ptr);
		}

		int miscIdx = objectsStatic_w.FindIndex(reinterpret_cast<const idClass*>(objectsMisc_w[idx]));
		if (miscIdx >= 0) {
			SG_Warn("idSaveGame::DebugCheckObjectList() [%p](%s) an entity ptr was duplicated, don't use misc ptr for idclass entities", objectsStatic_w[miscIdx], objectsStatic_w[miscIdx]->GetClassname());
		}
#endif
	}

#if defined(SG_VERIFY_DATA)
	if (saveDebugging) {
		// we don't need to save out static/misc objects here, as the indices have already been written out
		for (int idx = 0; idx < objectsGame_w.Num(); idx++) {
			if (const char* badDataType = IsBadMemoryAddress(objectsGame_w[idx])) {
				SG_Warn("idSaveGame::DebugCheckObjectList() objectsGame_w[idx] = %s ", badDataType);
				break;
			}

			int staticIdx = objectsStatic_w.FindIndex(objectsGame_w[idx]);
			if (staticIdx >= 1) {
				SG_Warn("idSaveGame::DebugCheckObjectList() a static object (%s) was stored, but it was already stored as a game local entity", objectsGame_w[idx]->GetClassname());
			}

			int miscIdx = objectsMisc_w.FindIndex(objectsGame_w[idx]);
			if (miscIdx >= 1) {
				SG_Warn("idSaveGame::DebugCheckObjectList() a misc ptr (%s) was stored, but it was already stored as a game local entity", objectsGame_w[idx]->GetClassname());
			}
		}

		for (int idx = 1; idx < objectsStatic_w.Num(); idx++) {
			if (const char* badDataType = IsBadMemoryAddress(objectsStatic_w[idx])) {
				SG_Warn("idSaveGame::DebugCheckObjectList() objectsStatic_w[idx] = %s ", badDataType);
				break;
			}

			int miscIdx = objectsMisc_w.FindIndex(objectsStatic_w[idx]);
			if (miscIdx >= 1) {
				SG_Warn("idSaveGame::DebugCheckObjectList() [%p](%s) an entity ptr was duplicated, don't use misc ptr for idclass entities", objectsStatic_w[idx], objectsStatic_w[idx]->GetClassname());
			}
		}

		for (int idx = 1; idx < objectsMisc_w.Num(); idx++) {
			if (const char* badDataType = IsBadMemoryAddress(objectsMisc_w[idx])) {
				SG_Warn("idSaveGame::DebugCheckObjectList() objectsMisc_w[idx] = %s ", badDataType);
				break;
			}

			if (objectsMiscStored_w[idx] <= 0) {
				const void* ptr = objectsMisc_w[idx];
				SG_Warn("idSaveGame::DebugCheckObjectList() misc ptr (%p) was never stored by its owner!", ptr);
			}
		}
	}
#endif

	if (sg_debugdump.GetInteger() > 0) {

		SG_Print( "\n-------------------- \n" );
		SG_Print( "--- SAVE WRITING --- \n" );

		SG_Print("\n %d game objs (%d ptrs) \n %d static objs (%d ptrs) \n %d misc objs (%d ptrs)\n",
			objectsGame_w.Num(), objectsGamePtrCount_w,
			objectsStatic_w.Num(), objectsStaticPtrCount_w,
			objectsMisc_w.Num(), objectsMiscPtrCount_w);

		idStr dumpTxt;
		for (int i = 0; i < objectsGame_w.Num(); i++) {
			idStr objInfo = idStr::Format("objects: %04d %s", i, objectsGame_w[i] ? objectsGame_w[i]->GetClassname() : "0x00000000");
			while (objInfo.Length() < 20) { objInfo.Append(' '); }

			dumpTxt.Append( objInfo );
			dumpTxt.Append( (i % 3 == 0) ? "\n" : " |" );
		}
		SG_Print( dumpTxt.c_str() );


		SG_Print( "\n--- SAVE WRITING --- \n" );
		SG_Print( "-------------------- \n" );

	}
}

static idStr CallSaveStack;
/*
================
idSaveGame::CallSave
================
*/
// blendo eric: wrapper for CallSave_r for debugging
void idSaveGame::CallSave(const idTypeInfo* cls, const idClass* obj) {
	WriteCheckType(SG_CHECK_CALLSAVE);
// #if defined(_DEBUG)
// 	WriteCheckString(cls->classname);
// #endif

	CallSave_r(cls, obj);

	WriteCheckType(SG_CHECK_CALLSAVE);
}

/*
================
idSaveGame::CallSave_r
================
*/
void idSaveGame::CallSave_r( const idTypeInfo *cls, const idClass *obj ) {
	RecordSaveStack( va("[%p](%d) %s->Save", &obj, cls->typeNum, cls->classname ) );

	WriteCheckSizeMarker();

	sg_callSaveID_t typeObjPtr( cls->typeNum, obj );
	if ( objectsCalledSave_w.FindIndex( typeObjPtr ) >= 0 ) {
		SG_Warn("CallSave repeated on same obj (%s)! This should only happen once.", obj->GetClassname());
	}
	objectsCalledSave_w.Append( typeObjPtr );

	if ( cls->super ) {
		CallSave_r( cls->super, obj );
		if ( cls->super->Save == cls->Save ) {
			// don't call save on this inheritance level since the function was called in the super class
			return;
		}
	}

	WriteCheckSizeMarker();

	( obj->*cls->Save )( this );

	WriteCheckSizeMarker(); // blendo eric: verify size matches
	WriteCheckType(SG_CHECK_CALLSAVE);
}

/*
================
idSaveGame::Write
================
*/
void idSaveGame::Write( const void *buffer, int len ) {
	file->Write( buffer, len );
	WriteCheckType(SG_CHECK_BUFFER);
}

/*
================
idSaveGame::WriteInt
================
*/
void idSaveGame::WriteInt( const int value ) {
	file->WriteInt( value );
	WriteCheckType(SG_CHECK_INT);
}

/*
================
idSaveGame::WriteUInt
================
*/
void idSaveGame::WriteUInt( const uint value ) {
	file->WriteUnsignedInt( value );
	WriteCheckType(SG_CHECK_UINT);
}

/*
================
idSaveGame::WriteJoint
================
*/
void idSaveGame::WriteJoint( const jointHandle_t value ) {
	file->WriteInt( (int&)value );
	WriteCheckType(SG_CHECK_JOINT);
}

/*
================
idSaveGame::WriteShort
================
*/
void idSaveGame::WriteShort( const short value ) {
	file->WriteShort( value );
	WriteCheckType(SG_CHECK_SHORT);
}

/*
================
idSaveGame::WriteByte
================
*/
void idSaveGame::WriteByte( const byte value ) {
	file->Write( &value, sizeof( value ) );
	WriteCheckType(SG_CHECK_BYTE);
}

/*
================
idSaveGame::WriteSignedChar
================
*/
void idSaveGame::WriteSignedChar( const signed char value ) {
	file->Write( &value, sizeof( value ) );
	WriteCheckType(SG_CHECK_SCHAR);
}

/*
================
idSaveGame::WriteFloat
================
*/
void idSaveGame::WriteFloat( const float value ) {
	file->WriteFloat( value );
	WriteCheckType(SG_CHECK_FLOAT);
}


/*
================
idSaveGame::WriteLong
================
*/
void idSaveGame::WriteLong( const long value ) {
	file->WriteLong( value );
	WriteCheckType(SG_CHECK_INT);
}


/*
================
idSaveGame::WriteBool
================
*/
void idSaveGame::WriteBool( const bool value ) {
	file->WriteBool( value );
	WriteCheckType(SG_CHECK_BOOL);
}

/*
================
idSaveGame::WriteString
================
*/
void idSaveGame::WriteString( const char *string ) {
	int len;

	len = strlen( string );
	WriteInt( len );
	file->Write( string, len );

	WriteCheckType(SG_CHECK_STRING);
}

/*
================
idSaveGame::WriteVec2
================
*/
void idSaveGame::WriteVec2( const idVec2 &vec ) {
	file->WriteVec2( vec );
	WriteCheckType(SG_CHECK_VEC2);
}

/*
================
idSaveGame::WriteVec3
================
*/
void idSaveGame::WriteVec3( const idVec3 &vec ) {
	file->WriteVec3( vec );
	WriteCheckType(SG_CHECK_VEC3);
}

/*
================
idSaveGame::WriteVec4
================
*/
void idSaveGame::WriteVec4( const idVec4 &vec ) {
	file->WriteVec4( vec );
	WriteCheckType(SG_CHECK_VEC4);
}

/*
================
idSaveGame::WriteVec6
================
*/
void idSaveGame::WriteVec6( const idVec6 &vec ) {
	file->WriteVec6( vec );
	WriteCheckType(SG_CHECK_VEC6);
}

/*
================
idSaveGame::WriteQuat
================
*/
void idSaveGame::WriteQuat(const idQuat& quat) {
	idVec4 vec = idVec4(quat.x, quat.y, quat.z, quat.w);
	file->WriteVec4( vec );
	WriteCheckType(SG_CHECK_QUAT);
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
	WriteCheckType(SG_CHECK_BOUNDS);
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
	WriteCheckType(SG_CHECK_WINDING);
}


/*
================
idSaveGame::WriteMat3
================
*/
void idSaveGame::WriteMat3( const idMat3 &mat ) {
	file->WriteMat3( mat );
	WriteCheckType(SG_CHECK_MAT);
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
	WriteCheckType(SG_CHECK_ANGLES);
}

/*
================
idSaveGame::WriteObject
================
*/
void idSaveGame::WriteObject( const idClass *obj ) {
	RecordSaveStack( va("[%p] WriteObject", obj ) );

	int index = SG_NULL_INDEX; // blendo eric: default to SG_NULL_INDEX when null

	if ( const char * msg = IsBadMemoryAddress(obj) ) {
		SG_Warn("idSaveGame::WriteObject(obj) obj = %s ", msg );
	}
	
	if ( obj && !obj->IsRemoved() ) {
		int savedIndex = objectsGame_w.FindIndex(obj);
		if (savedIndex >= 0) {
			index = savedIndex;
			objectsGamePtrCount_w++;
		} else {
			// blendo eric: if not in tracked objects list, probably a static object instead
			int staticIndex = objectsStatic_w.AddUnique(obj);
			objectsStaticStored_w.AssureNum( objectsStatic_w.Num(), 0 );
			index = -staticIndex;// save static object indexes as negative
			objectsStaticPtrCount_w++;
		}
	}

	WriteInt( index );

	WriteCheckType(SG_CHECK_OBJECT);
}

/*
================
idSaveGame::WriteStaticObject
================
*/
void idSaveGame::WriteStaticObject( const idClass &obj ) {
	RecordSaveStack( va("[%p] WriteStaticObject(%s)", &obj, obj.GetClassname() ) );

	assert( &obj != nullptr ); // staticobjects shouldn't be null

	if ( const char * msg = IsBadMemoryAddress( &obj ) ) {
		SG_Warn("idSaveGame::WriteStaticObject(obj) static entity obj: %s ", msg );
	}
	int index = objectsGame_w.FindIndex(&obj);
	if ( index>= 0 ) {
		SG_Warn("idSaveGame::WriteStaticObject() object was written as a StaticObject, but already exists as a game local entity (%s)", obj.GetClassname() );
	}

	index = objectsStatic_w.FindIndex(&obj);
	if ( index >= 0 && objectsStaticStored_w[index] > 0 ) {
		SG_Warn("idSaveGame::WriteStaticObject() object was written as a StaticObject already (%s)", obj.GetClassname() );
	}

	if (index < 0 ) {
		if (!obj.IsRemoved()) {
			index = objectsStatic_w.Append(&obj);
		}
	}
	WriteInt( index );

	if (index > 0) {
		objectsStaticStored_w.AssureNum(objectsStatic_w.Num(), 0);
		objectsStaticStored_w[index]++;
		objectsStaticStored_w[0]++;

		CallSave(obj.GetType(), &obj);
	}

	// blendo eric: save index, as we need to reference statics when they're used as pointers elsewhere
	// i.e. idClass obj; is used as idClass* objPtrPtr = &obj;

	if (index >= MAX_GENTITIES) {
		SG_Warn("idSaveGame::WriteStaticObject() there shouldn't be more static entities (%d) than MAX_GENTITIES", index);
	}

	WriteCheckType(SG_CHECK_OBJECT);
}

// blendo eric: mark this ptrRef as needing to be filled on restore
void idSaveGame::WriteMiscPtr( const void * ptrRef ) {
	RecordSaveStack( va("[%p] WriteMiscPtr", ptrRef ) );

	if ( const char * msg = IsBadMemoryAddress( ptrRef ) ) {
		SG_Warn("idSaveGame::WriteMiscPtr(obj) %s ", msg );
	}

	int index = objectsMisc_w.AddUnique( ptrRef );

	WriteInt( index );

	WriteCheckType(SG_CHECK_SGPTR);

	objectsMiscPtrCount_w++;
}

// blendo eric: track this object, so it can fill in pointer refs on restore
void idSaveGame::TrackMiscPtr( const void * ptr ) {
	RecordSaveStack( va("[%p] TrackMiscPtr", ptr ) );

	if ( const char * msg = IsBadMemoryAddress( ptr ) ) {
		SG_Warn("idSaveGame::TrackMiscPtr(obj) %s ", msg );
	}

	int index = objectsMisc_w.AddUnique( ptr );
	WriteInt( index );

	// whether the object this points to was actually tracked
	objectsMiscStored_w.AssureNum( objectsMisc_w.Num(), 0 );

	assert(objectsMiscStored_w[index] == 0);
	objectsMiscStored_w[index]++;
	objectsMiscStored_w[0]++;

	WriteCheckType(SG_CHECK_SGPTR);
}


// blendo eric: cast helper
void idSaveGame::WriteObject(const idEntityPtr<idEntity>& obj) {
	obj.Save(this);
	WriteCheckType(SG_CHECK_ENTITY);
}
// blendo eric: cast helper
void idSaveGame::WriteScriptObject(const idScriptObject& obj) {
	obj.Save(this);
	WriteCheckType(SG_CHECK_OBJECT);
}

void idSaveGame::WriteAnimatorPtr(const idAnimator* obj) {
	if ( const char * msg = IsBadMemoryAddress( obj ) ) {
		SG_Warn("idSaveGame::WriteAnimatorPtr(obj) %s ", msg );
	}

	bool bExists = obj != nullptr ;
	WriteBool( bExists );
	if( obj ) {
		idEntity * ent = obj->GetEntity();
		assert(ent->GetAnimator() == obj);
		WriteObject( ent );
	}

	WriteCheckType(SG_CHECK_OBJECT);
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
	WriteCheckType(SG_CHECK_DICT);
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
	WriteCheckType(SG_CHECK_MATERIAL);
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
	WriteCheckType(SG_CHECK_SKIN);
}

/*
================
idSaveGame::WriteParticle
================
*/
void idSaveGame::WriteParticle( const idDeclParticle *particle ) {
	if ( !particle ) {
		WriteString( "" );
	} else {
		WriteString( particle->GetName() );
	}
	WriteCheckType(SG_CHECK_PARTICLE);
}

/*
================
idSaveGame::WriteFX
================
*/
void idSaveGame::WriteFX( const idDeclFX *fx ) {
	if ( !fx ) {
		WriteString( "" );
	} else {
		WriteString( fx->GetName() );
	}
	WriteCheckType(SG_CHECK_FX);
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
	WriteCheckType(SG_CHECK_MODEL);
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
	WriteCheckType(SG_CHECK_SOUND);
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
	WriteCheckType(SG_CHECK_MODEL);
}

/*
================
idSaveGame::WriteUserInterface
================
*/

// blendo eric: default only unique if ui is already unique
//void idSaveGame::WriteUserInterface(const idUserInterface* ui)
//{
//	WriteUserInterface( ui,  ui ? ui->IsUniqued() : false, false );
//}

void idSaveGame::WriteUserInterface( const idUserInterface * ui, bool unique, bool forceShared ) {
	RecordSaveStack( "WriteUserInterface" );
	WriteCheckSizeMarker();

	bool bExists = ui != nullptr;

	WriteBool( bExists );
	if (bExists) {
#if defined(UI_DEFERRED)
		int gIdx = objectsUI_w.FindIndex( ui );
		bool validIdx = gIdx >= 0 && gIdx < objectsUI_w.Num();
		if (validIdx) {
			objectsUITagged_w[gIdx] = true;
		} else {
			gIdx = -1;
		}
		WriteInt( gIdx );
		if ( !validIdx )
#endif
		{
			WriteString( ui->Name() );
			WriteBool( unique );
			WriteBool( forceShared );
			
			int miscIdx = objectsMisc_w.FindIndex(ui);
			bool duplicate = miscIdx >= 0 && objectsMiscStored_w[miscIdx];

			WriteBool( duplicate );
			if ( duplicate ) {
				WriteSGPtr( ui );
			} else {
				ui->WriteToSaveGame(this);
			}
		}
	}

	WriteCheckType(SG_CHECK_UI);
	WriteCheckSizeMarker();
}

/*
================
idSaveGame::WriteRenderEntity
================
*/
void idSaveGame::WriteRenderEntity( const renderEntity_t &renderEntity ) {
	RecordSaveStack( "WriteRenderEntity" );

	WriteModel( renderEntity.hModel ); // saveidModel * hModel

	WriteInt( renderEntity.entityNum ); // int entityNum
	WriteInt( renderEntity.bodyId ); // int bodyId
	WriteBounds( renderEntity.bounds ); // idBounds bounds

	// callback is set by class's Restore function
	//deferredEntityCallback_t	callback; // deferredEntityCallback_t callback
	//void * callbackData; // void * callbackData

	WriteInt( renderEntity.suppressSurfaceInViewID ); // int suppressSurfaceInViewID
	WriteInt( renderEntity.suppressShadowInViewID ); // int suppressShadowInViewID
	WriteInt( renderEntity.suppressShadowInLightID ); // int suppressShadowInLightID
	WriteInt( renderEntity.allowSurfaceInViewID ); // int allowSurfaceInViewID

	WriteVec3( renderEntity.origin ); // idVec3 origin
	WriteMat3( renderEntity.axis ); // idMat3 axis

	WriteMaterial( renderEntity.customShader ); // const idMaterial * customShader
	WriteMaterial( renderEntity.referenceShader ); // const idMaterial * referenceShader
	WriteSkin( renderEntity.customSkin ); // const idDeclSkin * customSkin
	WriteSkin( renderEntity.overrideSkinInSubview ); // const idDeclSkin * overrideSkinInSubview // blendo eric: added

	if ( renderEntity.referenceSound != NULL ) {  // class idSoundEmitter * referenceSound
		WriteInt( renderEntity.referenceSound->Index() );
	} else {
		WriteInt( 0 );
	}

	for(int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) { // float shaderParms[ MAX_ENTITY_SHADER_PARMS ]
		WriteFloat( renderEntity.shaderParms[ i ] );
	}

	//Write idImage* fragmentMapOverride; // idImage* fragmentMapOverride

	for(int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) { // idUserInterface * gui[ MAX_RENDERENTITY_GUI ]
		bool isUnique = renderEntity.gui[ i ] ? renderEntity.gui[ i ]->IsUniqued() : false;
		WriteUserInterface( renderEntity.gui[ i ], isUnique, !isUnique  );
	}

	//WriteBool( renderEntity.remoteRenderView != nullptr ); // renderView_s *	remoteRenderView // regened
	//if (renderEntity.remoteRenderView)
	//{
	//	WriteRenderView(*renderEntity.remoteRenderView);
	//}

	WriteInt( renderEntity.numJoints ); // int numJoints
	for (int idx = 0; idx < renderEntity.numJoints; idx++)
	{
		const float* farr = renderEntity.joints[idx].ToFloatPtr();
		for ( int subIdx = 0; subIdx < 12; subIdx++ ) {
			WriteFloat( farr[subIdx] ); // idJointMat * joints
		}
	}

	WriteFloat( renderEntity.modelDepthHack ); // float modelDepthHack
	WriteBool( renderEntity.noSelfShadow ); // bool noSelfShadow
	WriteBool( renderEntity.noShadow ); // bool noShadow

	WriteBool( renderEntity.noDynamicInteractions ); // bool noDynamicInteractions

	WriteBool( renderEntity.trackInteractions ); // bool trackInteractions

	// interactionCallback_t interactionCallback // regen

	WriteBool( renderEntity.noFrustumCull ); // bool noFrustumCull

	WriteBool( renderEntity.weaponDepthHack ); // bool weaponDepthHack
	WriteInt( renderEntity.spectrum ); // int spectrum
	WriteInt( renderEntity.forceUpdate ); // int forceUpdate
	WriteInt( renderEntity.timeGroup ); // int timeGroup
	WriteInt( renderEntity.xrayIndex ); // int xrayIndex


	WriteUInt( renderEntity.customPostValue ); // unsigned int customPostValue  // blendo eric: added
	WriteBool( renderEntity.isPlayer ); // bool isPlayer  // blendo eric: added
	WriteBool( renderEntity.isFrozen ); // bool isFrozen  // blendo eric: added

	WriteCheckType(SG_CHECK_RENDER);
}

/*
================
idSaveGame::WriteRenderLight
================
*/
void idSaveGame::WriteRenderLight( const renderLight_t &renderLight ) {
	RecordSaveStack( "WriteRenderLight" );

	WriteMat3( renderLight.axis ); // idMat3 axis
	WriteVec3( renderLight.origin ); // idVec3 origin
	WriteInt( renderLight.suppressLightInViewID ); // int suppressLightInViewID
	WriteInt( renderLight.allowLightInViewID ); // int allowLightInViewID
	WriteBool( renderLight.noShadows ); // bool noShadows
	WriteBool( renderLight.noSpecular ); // bool noSpecular

	WriteBool( renderLight.pointLight ); // bool pointLight
	WriteBool( renderLight.parallel ); // bool parallel
	WriteBool( renderLight.isAmbient ); // bool isAmbient
	WriteBool( renderLight.castPlayerShadow ); // bool castPlayerShadow
	WriteVec3( renderLight.lightRadius ); // idVec3 lightRadius
	WriteVec3( renderLight.lightCenter ); // idVec3 lightCenter
	WriteVec3( renderLight.target ); // idVec3 target
	WriteVec3( renderLight.right ); // idVec3 right
	WriteVec3( renderLight.up ); // idVec3 up
	WriteVec3( renderLight.start ); // idVec3 start
	WriteVec3( renderLight.end ); // idVec3 end
	WriteModel( renderLight.prelightModel ); // saveidModel * prelightModel
	WriteInt( renderLight.lightId ); // int lightId


	WriteMaterial( renderLight.shader ); // const idMaterial * shader

	WriteInt(MAX_ENTITY_SHADER_PARMS); // float shaderParms[MAX_ENTITY_SHADER_PARMS]
	for( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		WriteFloat( renderLight.shaderParms[ i ] );
	}

	if ( renderLight.referenceSound != NULL ) { // idSoundEmitter * referenceSound
		WriteInt( renderLight.referenceSound->Index() );
	} else {
		WriteInt( 0 );
	}

	WriteInt( renderLight.spectrum ); // int spectrum // added by darkmod
	WriteBool( renderLight.affectLightMeter ); // bool affectLightMeter // added by SW

	WriteCheckType(SG_CHECK_LIGHT);
}

/*
================
idSaveGame::WriteRefSound
================
*/
void idSaveGame::WriteRefSound( const refSound_t &refSound ) {
	RecordSaveStack( "WriteRefSound" );

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

	WriteCheckType(SG_CHECK_SOUND);
}

/*
================
idSaveGame::WriteRenderView
================
*/
void idSaveGame::WriteRenderView( const renderView_t &view ) {
	RecordSaveStack( "WriteRenderView" );

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

	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		WriteFloat( view.shaderParms[ i ] );
	}

	WriteMaterial(view.globalMaterial);

	WriteCheckType(SG_CHECK_RENDER);
}

/*
===================
idSaveGame::WriteUsercmd
===================
*/
void idSaveGame::WriteUsercmd( const usercmd_t &usercmd ) {
	RecordSaveStack( "WriteUsercmd" );

	WriteInt( usercmd.gameFrame );
	WriteInt( usercmd.gameTime );
	WriteInt( usercmd.duplicateCount );
	WriteShort( usercmd.buttons ); // blendo eric: fix to match readusercmd
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

	// blendo eric
	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		WriteInt(usercmd.buttonState[idx]);
	}

	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		WriteInt(usercmd.prevButtonState[idx]);
	}

	WriteCheckType(SG_CHECK_CMD);
}

/*
===================
idSaveGame::WriteContactInfo
===================
*/
void idSaveGame::WriteContactInfo( const contactInfo_t &contactInfo ) {
	RecordSaveStack( "WriteContactInfo" );

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


	WriteCheckType(SG_CHECK_PHYS);
}

/*
===================
idSaveGame::WriteTrace
===================
*/
void idSaveGame::WriteTrace( const trace_t &trace ) {
	RecordSaveStack( "WriteTrace" );

	WriteFloat( trace.fraction );
	WriteVec3( trace.endpos );
	WriteMat3( trace.endAxis );
	WriteContactInfo( trace.c );

	WriteCheckType(SG_CHECK_PHYS);
}

/*
 ===================
 idRestoreGame::WriteTraceModel
 ===================
 */
void idSaveGame::WriteTraceModel( const idTraceModel &trace ) {
	RecordSaveStack( "WriteTraceModel" );
 
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


	WriteCheckType(SG_CHECK_PHYS);
}

/*
===================
idSaveGame::WriteClipModel
===================
*/
void idSaveGame::WriteClipModel( const idClipModel *clipModel ) {
	RecordSaveStack( "WriteClipModel" );
	if ( clipModel != NULL ) {
		WriteBool( true );
		clipModel->Save( this );
	} else {
		WriteBool( false );
	}

	WriteCheckType(SG_CHECK_PHYS);
}

/*
===================
idSaveGame::WriteSoundCommands
===================
*/
void idSaveGame::WriteSoundCommands( void ) {
	RecordSaveStack( "WriteSoundCommands" );

	gameSoundWorld->WriteToSaveGame( this );

	WriteCheckType(SG_CHECK_SOUND);
}


// ================
// blendo eric: helpers

void idSaveGame::WriteEntityDef( const idDeclEntityDef *ent ) {
	if ( !ent ) {
		WriteString( "" );
	} else {
		WriteString( ent->GetName() );
	}
	WriteCheckType(SG_CHECK_ENTITY);
}

void idSaveGame::WriteVecX(idVecX& vec) {
	WriteInt(vec.GetSize());
	WriteFloatArr(vec.ToFloatPtr(),  vec.GetSize());
}

void idSaveGame::WriteMatX(idMatX& mat) {
	WriteInt(mat.GetNumRows());
	WriteInt(mat.GetNumColumns());
	WriteFloatArr(mat.ToFloatPtr(),  mat.GetDimension());
}

void idSaveGame::WriteFloatArr( const float * arr, int num ) {
	WriteInt(num);

	int size = num * sizeof(float);
	float * fArr;
#if !defined(WIN32) //karin: alloca in stack on non-win
	fArr = (float*)alloca( size );
#else
	fArr = (float*)_malloca( size );
#endif
	memcpy(fArr, arr, size);

	LittleRevBytes( fArr, sizeof(float), num );

	file->Write( fArr, size );

	WriteCheckType(SG_CHECK_ANGLES);
}

void idSaveGame::WriteListObject(idList<idEntity*>& list) {
	WriteInt(list.Num());
	for (int idx = 0; idx < list.Num(); ++idx) {
		WriteObject(list[idx]);
	}

	WriteCheckType(SG_CHECK_LIST);
}

void idSaveGame::WriteListString(idList<idStr>& list) {
	WriteInt(list.Num());
	for (int idx = 0; idx < list.Num(); ++idx) {
		WriteString(list[idx]);
	}

	WriteCheckType(SG_CHECK_LIST);
}

void idSaveGame::WriteFunction( const function_t* func )
{
	if ( func ) {
		WriteString( func->Name() );
	} else {
		WriteString( "" );
	}

	WriteCheckType(SG_CHECK_FUNC);
}

void idSaveGame::WriteEventDef( const idEventDef* eventFlag )
{
	if ( eventFlag ) {
		WriteString( eventFlag->GetName() );
	} else {
		WriteString( "" );
	}

	WriteCheckType(SG_CHECK_FUNC);
}

void idSaveGame::WriteCurve(idCurve_Spline<idVec3>* curve)
{
	WriteBool(curve != nullptr);
	
	if (curve)
	{
		WriteInt(curve->times.Num()); // idList<float> times
		for (int idx = 0; idx < curve->times.Num(); idx++)
		{
			WriteFloat(curve->times[idx]);
		}
		WriteInt(curve->values.Num()); // idList<idVec3> values
		for (int idx = 0; idx < curve->values.Num(); idx++)
		{
			WriteVec3(curve->values[idx]);
		}

		WriteInt( curve->currentIndex ); // mutable int currentIndex
		WriteBool( curve->changed ); // mutable bool changed

		WriteInt( curve->boundaryType ); // boundary_t boundaryType
		WriteFloat( curve->closeTime ); // float closeTime
	}

	WriteCheckType(SG_CHECK_CURVE);
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

	saveVersion = -1;
	saveDebugging = sg_debugchecks.GetBool();

	objectsGameEntitiesNum_r = 0;
	objectsGameThreadsNum_r = 0;

	objectsGamePtrCount_r = 0;
	objectsStaticPtrCount_r = 0;
	objectsMiscPtrCount_r = 0;
}

/*
=====================
idRestoreGame::ReadSaveInfo
=====================
*/
void idRestoreGame::ReadSaveInfo( void ) {
	file->ReadInt( saveVersion );
	file->ReadBool( saveDebugging );
}

/*
================
idRestoreGame::Begin
================
*/
idRestoreGame idRestoreGame::Begin(idFile* savefile) {
	idRestoreGame save( savefile );
	save.ReadSaveInfo();
	return save;
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
void idRestoreGame::ReadAndCreateObjectsListTypes
================
*/
void idRestoreGame::ReadAndCreateObjectsListTypes( void ) {
	RecordSaveStack( "ReadAndCreateObjectsListTypes" );

	int gameEntCount;
	int threadCount;
	ReadInt( gameEntCount );
	ReadInt( threadCount );

	objectsGame_r.AssureNum( gameEntCount + threadCount , 0 );

#if defined(SG_OBJ_MATCHINDEX)
	int indicesCount;
	ReadInt( indicesCount );
	objectsGameOrder_r.SetNum( indicesCount );
	for (int idx = 0; idx < indicesCount; idx++)
	{
		int entIdx;
		idStr classname;
		ReadInt(entIdx);

		objectsGameOrder_r[idx] = entIdx;

		ReadString(classname);
		ReadCheckSizeMarker();

		if (classname.IsEmpty()) {
			continue;
		}

		idTypeInfo* type = idClass::GetClass(classname);
		if (!type) {
			Error("idRestoreGame::ReadAndCreateObjectsListTypes: Unknown class '%s'", classname.c_str());
		}
		objectsGame_r[entIdx] = type->CreateInstance();

		#ifdef ID_DEBUG_MEMORY
			InitTypeVariables(objectsGame_r[i], type->classname, 0xce);
		#endif
	}
#else
	int indicesCount;
	ReadInt( indicesCount );
	objectsGameOrder_r.SetNum( indicesCount );
	for (int idx = 0; idx < indicesCount; idx++ )
	{
		int entIdx;
		idStr classname;
		ReadInt( entIdx );

		objectsGameOrder_r[idx] = entIdx;

		ReadString( classname );
		ReadCheckSizeMarker();

		if (classname.IsEmpty()) {
			continue;
		}

		idTypeInfo *type = idClass::GetClass( classname );
		if ( !type ) {
			Error( "idRestoreGame::ReadAndCreateObjectsListTypes: Unknown class '%s'", classname.c_str() );
		}
		objectsGame_r[ entIdx ] = type->CreateInstance();

		#ifdef ID_DEBUG_MEMORY
			InitTypeVariables( objectsGame_r[i], type->classname, 0xce );
		#endif
	}

#endif

	ReadCheckSizeMarker();

#if defined(UI_DEFERRED)
	ContinueSaveStack( "guis create" );

	int guiCount;
	ReadInt(guiCount);
	objectsUI_r.AssureNum( guiCount, nullptr );
	for( int idx = 0; idx < guiCount; idx++ ) {
		idStr name;
		ReadString( name );
		bool isUnique;
		ReadBool( isUnique );
		idStr groupName;
		ReadString( groupName );
		bool isValid;
		ReadBool( isValid );

		if (isValid) {
			objectsUI_r[idx] = uiManager->FindGui( name, true, isUnique, !isUnique );
		} else {
			objectsUI_r[idx] = nullptr;
		}
	}


	ReadCheckSizeMarker();
#endif

	objectsStatic_r.SetGranularity(2048);
	objectsStatic_r.Append( nullptr );

	objectsMisc_r.SetGranularity(2048);
	objectsMisc_r.Append( nullptr );
}

/*
================
void idRestoreGame::ReadAndRestoreObjectsListData
================
*/
bool idRestoreGame::ReadAndRestoreObjectsListData( void ) {
	RecordSaveStack( "ReadAndRestoreObjectsListData" );

	ContinueSaveStack( "ReadSoundCommands" );
	ReadSoundCommands();

	if (!ReadCheckString("post sound commands")) { return false; }


	ContinueSaveStack( "RestoreTraceModels" );

	// read trace models
	idClipModel::RestoreTraceModels( this );

	if (!ReadCheckString("post trace models")) { return false; }


	ContinueSaveStack( "objects game" );

	// restore all the objects
#if defined(SG_OBJ_MATCHINDEX)
	for( int i = 0; i < objectsGameOrder_r.Num(); i++ ) {
		int entIdx = objectsGameOrder_r[i];
		assert( objectsGame_r[ entIdx ] );
		bool restoreOK = CallRestore( objectsGame_r[ entIdx ]->GetType(), objectsGame_r[ entIdx ] );
		if (!restoreOK) {
			SG_Warn("saveload data corrupt restoring object: %s", objectsGame_r[ i ]->GetType()->classname);
			return false;
		}
	}
#else
	for( int i = 1; i < objectsGame_r.Num(); i++ ) {
		bool restoreOK = CallRestore( objectsGame_r[ i ]->GetType(), objectsGame_r[ i ] );
		if (!restoreOK) {
			SG_Warn("saveload data corrupt restoring object: %s", objectsGame_r[ i ]->GetType()->classname);
			return false;
		}
	}
#endif


	if (!ReadCheckString("post objects")) { return false; }

#if defined(UI_DEFERRED)
	ContinueSaveStack( "guis restore" );

	int uiCount = 0;
	ReadInt( uiCount );

	objectsUI_r.AssureNum( uiCount, nullptr );

	for( int idx = 0; idx< objectsUI_r.Num(); idx++ ) {
		bool validMap;
		ReadBool( validMap );

		idUserInterface * ui = nullptr;
		if (validMap) {
			ui = objectsUI_r[idx];
			ui->ReadFromSaveGame(this);
		}
		objectsUI_r[idx] = ui;
	}

	if (!ReadCheckString("post guis")) { return false; }
#endif


	// blendo eric: after collecting all 'restored' pointers, and reading all "static" objects
	//				insert the final instanced member entity pointers into their spots
	for (int idx = 0; idx < objectsStaticFixUp_r.Num(); idx++) {
		ent_fixup_ptr_t valPair = objectsStaticFixUp_r[idx];
		if (valPair.index >= 0 && valPair.index < objectsStatic_r.Num()) {
			if ( const char * msg = IsBadMemoryAddress( *(valPair.objPtrPtr) ) ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() *(objectsStaticFixUp_r[idx].objPtrPtr): %s ", msg );
			}

			if ( *(valPair.objPtrPtr) != nullptr && *(valPair.objPtrPtr) != objectsStatic_r[valPair.index] ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() *(objectsStaticFixUp_r[idx].objPtrPtr) already set, this could be double instantiation");
			}

			*(valPair.objPtrPtr) = objectsStatic_r[valPair.index];
		} else {
			SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() objectsStaticFixUp_r[idx].index out of range ");
			return false;
		}
	}

	// blendo eric: fixup misc ptrs
	for (int idx = objectsMiscFixUp_r.Num()-1; idx >= 0; idx--) {
		misc_fixup_ptr_t& valPair = objectsMiscFixUp_r[idx];
		if (valPair.index > 0 && valPair.index < objectsMisc_r.Num()) {
			if ( const char * msg = IsBadMemoryAddress( *(valPair.objPtrPtr) ) ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() *(objectsMiscFixUp_r[idx].objPtrPtr) %s ", msg );
			}

			if ( *(valPair.objPtrPtr) != nullptr && *(valPair.objPtrPtr) != objectsMisc_r[valPair.index] ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() misc ptr (%p) was already set (%p), but does not match, this could be double instantiation", objectsMisc_r[valPair.index], *(valPair.objPtrPtr));
			}

			*(valPair.objPtrPtr) = objectsMisc_r[valPair.index];
			objectsMiscFixUp_r.RemoveIndex(idx);
		}
	}

	// SM: Need to make sure only update visuals/present objects which need it
	for( int i = 0; i < objectsGameOrder_r.Num(); i++ ) {
		int entIdx = objectsGameOrder_r[i];
		idClass* obj = objectsGame_r[entIdx];
		if ( obj->IsType( idEntity::Type ) ) {
			idEntity *ent = static_cast<idEntity *>( obj );
			if (ent->RequiresPresentOnRestore())
			{
				ent->UpdateVisuals();
				ent->Present();
			}
		}
	}

	// blendo eric: fixup remaining misc ptrs after present (which might fixup some ptrs)
	for (int idx = 0; idx < objectsMiscFixUp_r.Num() ; idx++) {
		misc_fixup_ptr_t& valPair = objectsMiscFixUp_r[idx];
		if (valPair.index > 0 && valPair.index < objectsMisc_r.Num()) {
			if ( const char * msg = IsBadMemoryAddress( *(valPair.objPtrPtr) ) ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() bad fixup misc obj ptr ", msg );
			}

			if ( *(valPair.objPtrPtr) != nullptr  ) {
				SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() misc ptr was already set, this could be double instantiation");
			}
			*(valPair.objPtrPtr) = objectsMisc_r[valPair.index];
		} else if( valPair.index >= objectsMisc_r.Num() ) {
			SG_Warn("idRestoreGame::ReadAndRestoreObjectsListData() failed to find static/instanced object ");
			return false;
		}
	}

	DebugCheckObjectLists();


	if (saveDebugging)
	{
		int num;

		ReadInt(num);
		SG_Print("EntityDefs on save %d, after load %d\n", num, gameRenderWorld->EntityDefNum() );

		ReadInt(num);
		SG_Print("LightDefs on save %d, after load %d\n", num, gameRenderWorld->LightDefNum() );
	}


#ifdef ID_DEBUG_MEMORY
	idStr gameState = file->GetName();
	gameState.StripFileExtension();
	WriteGameState_f( idCmdArgs( va( "test %s_restore", gameState.c_str() ), false ) );
	//CompareGameState_f( idCmdArgs( va( "test %s_save", gameState.c_str() ) ) );
	SG_Warn( "dumped game states" );
#endif

	return true;
}

/*
====================
void idRestoreGame::DebugCheckObjectLists
====================
*/
void idRestoreGame::DebugCheckObjectLists()
{
	if (sg_debugdump.GetInteger()) {
		SG_Print( "-------------------- \n" );
		SG_Print( "--- SAVE READING --- \n" );

		SG_Print(" %d game objs (%d ptrs) \n %d static objs (%d ptrs) \n %d misc objs (%d ptrs)\n",
			objectsGame_r.Num(), objectsGamePtrCount_r,
			objectsStatic_r.Num(), objectsStaticPtrCount_r,
			objectsMisc_r.Num(), objectsMiscPtrCount_r);

		idStr dumpTxt;
		for( int i = 0; i < objectsGame_r.Num(); i++ ) {
			dumpTxt.Append( va(" %-16s", objectsGame_r[i] ? objectsGame_r[i]->GetClassname() : "0x00000000" ) );
			dumpTxt.Append( ((i % 3 == 0) && (i < objectsGame_r.Num()-1)) ? " |"  : "\n");

			SG_Print( dumpTxt );
			dumpTxt.Clear();
		}

		SG_Print( "--- SAVE READING --- \n" );
		SG_Print( "-------------------- \n" );
	}
}

/*
====================
void idRestoreGame::DeleteObjects
====================
*/
void idRestoreGame::DeleteObjects( void ) {

	// Remove the NULL object before deleting
	objectsGame_r.RemoveIndex( 0 );

	objectsGame_r.DeleteContents( true );
}

/*
================
idRestoreGame::Error
================
*/
void idRestoreGame::Warn( const char *fmt, ... ) {
	va_list	argptr;
	va_start( argptr, fmt );
	SG_Warn( fmt, argptr );
	va_end( argptr );

	objectsGame_r.DeleteContents( true );
}

/*
================
idRestoreGame::Error
================
*/
void idRestoreGame::Error( const char *fmt, ... ) {
	va_list	argptr;
	va_start( argptr, fmt );
	SG_Error( fmt, argptr );
	va_end( argptr );

	objectsGame_r.DeleteContents( true );
}


/*
================
idRestoreGame::CallRestore
================
*/
// blendo eric: wrapper for the full object, rather than each type in CallRestore_r
bool idRestoreGame::CallRestore(const idTypeInfo* cls, idClass* obj) {
	ReadCheckType(SG_CHECK_CALLSAVE);
// #if defined(_DEBUG)
// 	ReadCheckString(cls->classname);
// #endif

	bool restoreOk = CallRestore_r(cls, obj);
	if (!restoreOk) {
		SG_Warn("saveload data corrupt restoring object: %s", cls->classname);
		return false;
	}

	ReadCheckType(SG_CHECK_CALLSAVE);

	return restoreOk;
}

/*
================
idRestoreGame::CallRestore_r
================
*/
bool idRestoreGame::CallRestore_r(const idTypeInfo* cls, idClass* obj) {
	RecordSaveStack( va("[%p](%d) %s->Restore", &obj, cls->typeNum, cls->classname ) );

	ReadCheckSizeMarker();

	bool restoreOK = true;
	if (cls->super) {
		restoreOK = restoreOK && CallRestore_r(cls->super, obj);
		if (cls->super->Restore == cls->Restore) {
			// don't call save on this inheritance level since the function was called in the super class
			return true;
		}
	}

	if (!restoreOK) {
		Error( "idRestoreGame::CallRestore_r() failed to restore %s ", cls->classname );
		return false;
	}


	ReadCheckSizeMarker();

	( obj->*cls->Restore )( this );

	ReadCheckSizeMarker();
	ReadCheckType(SG_CHECK_CALLSAVE);

	return restoreOK;
}

/*
================
idRestoreGame::Read
================
*/
void idRestoreGame::Read( void *buffer, int len ) {
	file->Read( buffer, len );
	ReadCheckType(SG_CHECK_BUFFER);
}

/*
================
idRestoreGame::ReadInt
================
*/
void idRestoreGame::ReadInt( int &value ) {
	file->ReadInt( value );
	ReadCheckType(SG_CHECK_INT);
}

/*
================
idRestoreGame::ReadUInt
================
*/
// blendo eric
void idRestoreGame::ReadUInt( uint &value ) {
	file->ReadUnsignedInt( value );
	ReadCheckType(SG_CHECK_UINT);
}

/*
================
idRestoreGame::ReadJoint
================
*/
void idRestoreGame::ReadJoint( jointHandle_t &value ) {
	file->ReadInt( (int&)value );
	ReadCheckType(SG_CHECK_JOINT);
}

/*
================
idRestoreGame::ReadShort
================
*/
void idRestoreGame::ReadShort( short &value ) {
	file->ReadShort( value );
	ReadCheckType(SG_CHECK_SHORT);
}

/*
================
idRestoreGame::ReadByte
================
*/
void idRestoreGame::ReadByte( byte &value ) {
	file->Read( &value, sizeof( value ) );
	ReadCheckType(SG_CHECK_BYTE);
}

/*
================
idRestoreGame::ReadSignedChar
================
*/
void idRestoreGame::ReadSignedChar( signed char &value ) {
	file->Read( &value, sizeof( value ) );
	ReadCheckType(SG_CHECK_SCHAR);
}

/*
================
idRestoreGame::ReadFloat
================
*/
void idRestoreGame::ReadFloat( float &value ) {
	file->ReadFloat( value );
	ReadCheckType(SG_CHECK_FLOAT);
}

/*
================
idRestoreGame::ReadLong
================
*/
void idRestoreGame::ReadLong( long &value ) {
	file->ReadLong( value );
	ReadCheckType(SG_CHECK_INT);
}

/*
================
idRestoreGame::ReadBool
================
*/
void idRestoreGame::ReadBool( bool &value ) {
	file->ReadBool( value );
	ReadCheckType(SG_CHECK_BOOL);
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

	ReadCheckType(SG_CHECK_STRING);
}

/*
================
idRestoreGame::ReadString
================
*/
void idRestoreGame::ReadString( const char *& string) {
	int len;

	ReadInt( len );
	if ( len < 0 ) {
		Error( "idRestoreGame::ReadString: invalid length" );
	}
	if (len > 0)
	{
		char * newStr = new char[len];
		file->Read(newStr, len);
		string = newStr;
	}

	ReadCheckType(SG_CHECK_STRING);
}

/*
================
idRestoreGame::ReadStringToArray
================
*/
void idRestoreGame::ReadCharArray( char * chars ) {
	int len;

	ReadInt( len );
	if ( len < 0 ) {
		Error( "idRestoreGame::ReadString: invalid length" );
	}

	file->Read( &chars, len );

	ReadCheckType(SG_CHECK_STRING);
}

/*
================
idRestoreGame::ReadVec2
================
*/
void idRestoreGame::ReadVec2( idVec2 &vec ) {
	file->ReadVec2( vec );
	ReadCheckType(SG_CHECK_VEC2);
}

/*
================
idRestoreGame::ReadVec3
================
*/
void idRestoreGame::ReadVec3( idVec3 &vec ) {
	file->ReadVec3( vec );
	ReadCheckType(SG_CHECK_VEC3);
}

/*
================
idRestoreGame::ReadVec4
================
*/
void idRestoreGame::ReadVec4( idVec4 &vec ) {
	file->ReadVec4( vec );
	ReadCheckType(SG_CHECK_VEC4);
}

/*
================
idRestoreGame::ReadVec6
================
*/
void idRestoreGame::ReadVec6( idVec6 &vec ) {
	file->ReadVec6( vec );
	ReadCheckType(SG_CHECK_VEC6);
}


/*
================
idRestoreGame::ReadQuat
================
*/
void idRestoreGame::ReadQuat(idQuat& quat) {
	idVec4 vec;
	file->ReadVec4( vec );
	quat.Set(vec.x, vec.y, vec.z, vec.w);
	ReadCheckType(SG_CHECK_QUAT);
}

/*
================
idRestoreGame::ReadBounds
================
*/
void idRestoreGame::ReadBounds( idBounds &bounds ) {
	file->Read( &bounds, sizeof( bounds ) );
	LittleRevBytes( &bounds, sizeof(float), sizeof(bounds)/sizeof(float) );
	ReadCheckType(SG_CHECK_BOUNDS);
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
	ReadCheckType(SG_CHECK_WINDING);
}

/*
================
idRestoreGame::ReadMat3
================
*/
void idRestoreGame::ReadMat3( idMat3 &mat ) {
	file->ReadMat3( mat );
	ReadCheckType(SG_CHECK_MAT);
}

/*
================
idRestoreGame::ReadAngles
================
*/
void idRestoreGame::ReadAngles( idAngles &angles ) {
	file->Read( &angles, sizeof( angles ) );
	LittleRevBytes(&angles, sizeof(float), sizeof(idAngles)/sizeof(float) );
	ReadCheckType(SG_CHECK_ANGLES);
}

/*
================
idRestoreGame::ReadObject
================
*/
void idRestoreGame::ReadObject( idClass *&obj ) {
	ReadObjectPtr(&obj);
}

/*
================
idRestoreGame::ReadObject
================
*/
void idRestoreGame::ReadObjectPtr( idClass** obj ) {
	RecordSaveStack( va("[%p] ReadObjectPtr", obj ) );

	if ( const char * msg = IsBadMemoryAddress( *obj ) ) {
		SG_Warn("idRestoreGame::ReadObjectPtr() bad ptr ", msg );
	}

	int index;
	ReadInt(index);

	if ( index == SG_NULL_INDEX ) {
		*obj = nullptr;
	} else if (index < 0) { // negative indices indicate this is a member instanced obj
		// associate the pointer-to-pointer and the index, so it can be matched to the actual pointer later
		objectsStaticFixUp_r.Append(ent_fixup_ptr_t(-index, obj));
		objectsStaticPtrCount_r++;
	} else if (index >= objectsGame_r.Num()) {
		Error( "idRestoreGame::ReadObject: invalid object index" );
		*obj = nullptr;
	} else {
		if ( (*obj != nullptr) && (*obj != objectsGame_r[index]) ) {
			SG_Warn("idSaveGame::ReadObjectPtr() mismatched ptr, might be double instantiation.");
		}

		*obj = objectsGame_r[index];

		if ( const char * msg = IsBadMemoryAddress( *obj ) ) {
			SG_Warn("idRestoreGame::ReadObjectPtr() bad ptr ", msg );
		}

		objectsGamePtrCount_r++;
	}

	ReadCheckType(SG_CHECK_OBJECT);
}

/*
================
idRestoreGame::ReadStaticObject
================
*/
void idRestoreGame::ReadStaticObject( idClass &obj ) {
	RecordSaveStack( va("[%p] ReadStaticObject(%s)", &obj, obj.GetClassname() ) );

	int index;
	ReadInt( index );

	if (index >= 0) {
		objectsStatic_r.AssureNum(index + 1, (idClass*)SG_UNTRACKED_PTR);

		objectsStatic_r[index] = &obj;

		CallRestore(obj.GetType(), &obj);
	}

	ReadCheckType(SG_CHECK_OBJECT);
}

/*
================
idRestoreGame::ReadMiscPtr
================
*/
// blendo eric: save a non idclass pointer ref to be filled when the obj address is located later
void idRestoreGame::ReadMiscPtr(void ** ptr) {
	RecordSaveStack( va("[%p] ReadMiscPtr", ptr ) );

	if ( const char * msg = IsBadMemoryAddress( *ptr ) ) {
		SG_Warn("idRestoreGame::ReadMiscPtr() bad ptr ", msg );
	}

	int index;
	ReadInt( index );

	assert( index >= 0 );

	// fill this pointer ref in later using the index
	objectsMiscFixUp_r.Append( misc_fixup_ptr_t( index, ptr ) );

	//if ((*ptr != nullptr)) { // this will be caught on fixup, but can read stack here
	//	SG_Warn("idRestoreGame::ReadMiscPtr() ptr was set previously, will be overwritten");
	//}

	objectsMiscPtrCount_r++;

	ReadCheckType(SG_CHECK_SGPTR);
}

/*
================
idRestoreGame::TrackMiscPtr
================
*/
// blendo eric: begin tracking this non idclass pointer, so it can be used to fill in missing ptrs later
void idRestoreGame::TrackMiscPtr( void * ptr ) {
	RecordSaveStack( va("[%p] TrackMiscPtr", ptr ) );

	if ( const char * msg = IsBadMemoryAddress( ptr ) ) {
		SG_Warn("idRestoreGame::TrackMiscPtr() bad ptr ", msg );
	}

	int index;
	ReadInt( index );
	assert( index >= 0 );

	objectsMisc_r.AssureNum( index+1, (idClass*)SG_UNTRACKED_PTR ); 
	assert( objectsMisc_r[index] == (idClass*)SG_UNTRACKED_PTR  );
	objectsMisc_r[index] = ptr; // index this object, so it can fill in pointer refs later

	ReadCheckType(SG_CHECK_SGPTR);
}

// blendo eric: cast helper
void idRestoreGame::ReadObject(idEntityPtr<idEntity>& obj) {
	obj.Restore(this);
	ReadCheckType(SG_CHECK_ENTITY);
}
// blendo eric: cast helper
void idRestoreGame::ReadScriptObject(idScriptObject & obj) {
	obj.Restore(this);
	ReadCheckType(SG_CHECK_OBJECT);
}

void idRestoreGame::ReadAnimatorPtr(idAnimator*& obj) {
	RecordSaveStack( va("[%p] ReadAnimatorPtr (parent=%s) ", obj, obj ? obj->GetEntity()->GetClassname() : "" ) );
	bool bExists;
	ReadBool( bExists );

	if (bExists) {
		idEntity* objEnt = nullptr;
		ReadObject(objEnt);
		if (objEnt) {
			obj = objEnt->GetAnimator();
		} else {
			obj = nullptr;
		}
	} else {
		obj = nullptr;
	}
	ReadCheckType(SG_CHECK_OBJECT);
}

// blendo eric: cast helper
void idRestoreGame::ReadDict(idDict& dict) {
	ReadDict(&dict);
}

/*
================
idRestoreGame::ReadDict
================
*/
void idRestoreGame::ReadDict( idDict *dict ) {
	RecordSaveStack( va("[%p] ReadDict ", dict ) );
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
	ReadCheckType(SG_CHECK_DICT);
}

/*
================
idRestoreGame::ReadMaterial
================
*/
void idRestoreGame::ReadMaterial( const idMaterial *&material ) {
	RecordSaveStack( va("[%p] ReadMaterial ", material ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		material = NULL;
	} else {
		material = declManager->FindMaterial( name );
	}
	ReadCheckType(SG_CHECK_MATERIAL);
}

/*
================
idRestoreGame::ReadSkin
================
*/
void idRestoreGame::ReadSkin( const idDeclSkin *&skin ) {
	RecordSaveStack( va("[%p] ReadSkin ", skin ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		skin = NULL;
	} else {
		skin = declManager->FindSkin( name );
	}
	ReadCheckType(SG_CHECK_SKIN);
}

/*
================
idRestoreGame::ReadParticle
================
*/
void idRestoreGame::ReadParticle( const idDeclParticle *&particle ) {
	RecordSaveStack( va("[%p] ReadParticle ", particle ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		particle = NULL;
	} else {
		particle = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, name ) );
	}
	ReadCheckType(SG_CHECK_PARTICLE);
}

/*
================
idRestoreGame::ReadFX
================
*/
void idRestoreGame::ReadFX( const idDeclFX *&fx ) {
	RecordSaveStack( va("[%p] ReadFX ", fx ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		fx = NULL;
	} else {
		fx = static_cast<const idDeclFX *>( declManager->FindType( DECL_FX, name ) );
	}
	ReadCheckType(SG_CHECK_FX);
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
	ReadCheckType(SG_CHECK_SOUND);
}

/*
================
idRestoreGame::ReadModelDef
================
*/
void idRestoreGame::ReadModelDef( const idDeclModelDef *&modelDef ) {
	RecordSaveStack( va("[%p] ReadModelDef ", modelDef ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		modelDef = NULL;
	} else {
		modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, name, false ) );
	}
	ReadCheckType(SG_CHECK_MODEL);
}

/*
================
idRestoreGame::ReadModel
================
*/
void idRestoreGame::ReadModel( idRenderModel *&model ) {
	RecordSaveStack( va("[%p] ReadModel ", model ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		model = NULL;
	} else {
		model = renderModelManager->FindModel( name );
	}
	ReadCheckType(SG_CHECK_MODEL);
}

/*
================
idRestoreGame::ReadUserInterface
================
*/
void idRestoreGame::ReadUserInterface( idUserInterface *&ui ) {
	RecordSaveStack( va("[%p] ReadUserInterface", ui ) );
	idStr name;

	ReadCheckSizeMarker();

	bool bExists;
	ReadBool( bExists );
	if (bExists) {
#if defined(UI_DEFERRED)
		int gIdx;
		ReadInt(gIdx);
		bool validIndex = gIdx >= 0 && gIdx < objectsUI_r.Num();
		if ( validIndex ) {
			ui = objectsUI_r[gIdx];
		}
		else
#endif
		{
			// backup creation
			idStr name;
			bool unique;
			bool shared;
			ReadString(name);
			ReadBool(unique);
			ReadBool(shared);

			bool duplicate;
			ReadBool( duplicate );
			if (duplicate) {
				ReadSGPtr( (idSaveGamePtr**)&ui );
			} else {
				ui = uiManager->FindGui(name, true, unique, shared);
				if (ui) {
					ui->ReadFromSaveGame(this);
					ui->StateChanged(gameLocal.time);
				}
			}
		}
	} else {
		ui = nullptr;
	}

	ReadCheckType(SG_CHECK_UI);
	ReadCheckSizeMarker();
}

/*
================
idRestoreGame::ReadRenderEntity
================
*/
void idRestoreGame::ReadRenderEntity( renderEntity_t &renderEntity ) {
	RecordSaveStack( va("[%p] ReadRenderEntity",  &renderEntity ) );

	ReadModel( renderEntity.hModel ); // saveidModel * hModel

	ReadInt( renderEntity.entityNum ); // int entityNum
	ReadInt( renderEntity.bodyId ); // int bodyId
	ReadBounds( renderEntity.bounds ); // idBounds bounds

	// callback is set by class's Restore function
	renderEntity.callback = nullptr; // deferredEntityCallback_t callback
	renderEntity.callbackData = nullptr; // void * callbackData

	ReadInt( renderEntity.suppressSurfaceInViewID ); // int suppressSurfaceInViewID
	ReadInt( renderEntity.suppressShadowInViewID ); // int suppressShadowInViewID
	ReadInt( renderEntity.suppressShadowInLightID ); // int suppressShadowInLightID
	ReadInt( renderEntity.allowSurfaceInViewID ); // int allowSurfaceInViewID

	ReadVec3( renderEntity.origin ); // idVec3 origin
	ReadMat3( renderEntity.axis ); // idMat3 axis

	ReadMaterial( renderEntity.customShader ); // const idMaterial * customShader
	ReadMaterial( renderEntity.referenceShader ); // const idMaterial * referenceShader
	ReadSkin( renderEntity.customSkin ); // const idDeclSkin * customSkin
	ReadSkin( renderEntity.overrideSkinInSubview ); // const idDeclSkin * overrideSkinInSubview // blendo eric: added

	int index;
	ReadInt( index );
	renderEntity.referenceSound = gameSoundWorld->EmitterForIndex(index);
	assert(renderEntity.referenceSound != (void*)0x00000001);

	for(int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) { // float shaderParms[ MAX_ENTITY_SHADER_PARMS ]
		ReadFloat( renderEntity.shaderParms[ i ] );
	}

	renderEntity.fragmentMapOverride = nullptr; // idImage* fragmentMapOverride

	for(int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) { // idUserInterface * gui[ MAX_RENDERENTITY_GUI ]
		ReadUserInterface( renderEntity.gui[ i ]  );
	}

	// idEntity will restore "cameraTarget", which will be used in idEntity::Present to restore the remoteRenderView
	renderEntity.remoteRenderView = NULL;

	ReadInt( renderEntity.numJoints ); // int numJoints
	if (renderEntity.numJoints > 0)
	{
		// idJointMat * joints
		renderEntity.joints = (idJointMat*)_alloca16( renderEntity.numJoints * sizeof( idJointMat ) );
		for (int idx = 0; idx < renderEntity.numJoints; idx++)
		{
			float* farr = renderEntity.joints[idx].ToFloatPtr();
			for (int subIdx = 0; subIdx < 12; subIdx++) {
				ReadFloat(farr[subIdx]);
			}
		}
	}

	ReadFloat( renderEntity.modelDepthHack ); // float modelDepthHack
	ReadBool( renderEntity.noSelfShadow ); // bool noSelfShadow
	ReadBool( renderEntity.noShadow ); // bool noShadow

	ReadBool( renderEntity.noDynamicInteractions ); // bool noDynamicInteractions

	ReadBool( renderEntity.trackInteractions ); // bool trackInteractions

	renderEntity.interactionCallback = nullptr; // interactionCallback_t interactionCallback

	ReadBool( renderEntity.noFrustumCull ); // bool noFrustumCull

	ReadBool( renderEntity.weaponDepthHack ); // bool weaponDepthHack
	ReadInt( renderEntity.spectrum ); // int spectrum
	ReadInt( renderEntity.forceUpdate ); // int forceUpdate
	ReadInt( renderEntity.timeGroup ); // int timeGroup
	ReadInt( renderEntity.xrayIndex ); // int xrayIndex


	ReadUInt( renderEntity.customPostValue ); // unsigned int customPostValue  // blendo eric: added
	ReadBool( renderEntity.isPlayer ); // bool isPlayer  // blendo eric: added
	ReadBool( renderEntity.isFrozen ); // bool isFrozen  // blendo eric: added

	ReadCheckType(SG_CHECK_RENDER);
}

/*
================
idRestoreGame::ReadRenderLight
================
*/
void idRestoreGame::ReadRenderLight( renderLight_t &renderLight ) {
	RecordSaveStack( va("[%p] ReadRenderLight",  &renderLight ) );
	ReadMat3( renderLight.axis ); // idMat3 axis
	ReadVec3( renderLight.origin ); // idVec3 origin
	ReadInt( renderLight.suppressLightInViewID ); // int suppressLightInViewID
	ReadInt( renderLight.allowLightInViewID ); // int allowLightInViewID
	ReadBool( renderLight.noShadows ); // bool noShadows
	ReadBool( renderLight.noSpecular ); // bool noSpecular

	ReadBool( renderLight.pointLight ); // bool pointLight
	ReadBool( renderLight.parallel ); // bool parallel
	ReadBool( renderLight.isAmbient ); // bool isAmbient
	ReadBool( renderLight.castPlayerShadow ); // bool castPlayerShadow
	ReadVec3( renderLight.lightRadius ); // idVec3 lightRadius
	ReadVec3( renderLight.lightCenter ); // idVec3 lightCenter
	ReadVec3( renderLight.target ); // idVec3 target
	ReadVec3( renderLight.right ); // idVec3 right
	ReadVec3( renderLight.up ); // idVec3 up
	ReadVec3( renderLight.start ); // idVec3 start
	ReadVec3( renderLight.end ); // idVec3 end

	//// only idLight has a prelightModel and it's always based on the entityname, so we'll restore it there
	//// ReadModel( renderLight.prelightModel );
	//renderLight.prelightModel = NULL; // blendo eric: why bother?

	ReadModel( renderLight.prelightModel ); // idRenderModel * prelightModel

	ReadInt( renderLight.lightId ); // int lightId


	ReadMaterial( renderLight.shader ); // const idMaterial * shader

	int num;
	ReadInt(num); // float shaderParms[MAX_ENTITY_SHADER_PARMS]
	for( int i = 0; i < num; i++ ) {
		ReadFloat( renderLight.shaderParms[ i ] );
	}

	ReadInt( num ); // idSoundEmitter * referenceSound
	renderLight.referenceSound = gameSoundWorld->EmitterForIndex( num );
	assert(renderLight.referenceSound != (void*)0x00000001);

	ReadInt( renderLight.spectrum ); // int spectrum // added by darkmod
	ReadBool( renderLight.affectLightMeter ); // bool affectLightMeter // added by SW

	ReadCheckType(SG_CHECK_LIGHT);
}

/*
================
idRestoreGame::ReadRefSound
================
*/
void idRestoreGame::ReadRefSound( refSound_t &refSound ) {
	int		index;
	ReadInt( index );

	refSound.referenceSound = gameSoundWorld->EmitterForIndex(index);
	assert(refSound.referenceSound != (void*)0x00000001);
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

	ReadCheckType(SG_CHECK_SOUND);
}

/*
================
idRestoreGame::ReadRenderView
================
*/
void idRestoreGame::ReadRenderView( renderView_t &view ) {
	RecordSaveStack( va("[%p] ReadRenderView",  &view ) );
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

	ReadMaterial(view.globalMaterial);

	ReadCheckType(SG_CHECK_RENDER);
}

/*
=================
idRestoreGame::ReadUsercmd
=================
*/
void idRestoreGame::ReadUsercmd( usercmd_t &usercmd ) {
	RecordSaveStack( va("[%p] ReadUsercmd",  &usercmd ) );
	ReadInt( usercmd.gameFrame );
	ReadInt( usercmd.gameTime );
	ReadInt( usercmd.duplicateCount );
	
	ReadShort(usercmd.buttons); //changed from byte to short, so that we can have more buttons.

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

	// blendo eric

	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		ReadInt(usercmd.buttonState[idx]);
	}

	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		ReadInt(usercmd.prevButtonState[idx]);
	}

	ReadCheckType(SG_CHECK_CMD);
}

/*
===================
idRestoreGame::ReadContactInfo
===================
*/
void idRestoreGame::ReadContactInfo( contactInfo_t &contactInfo ) {
	RecordSaveStack( va("[%p] ReadContactInfo",  &contactInfo ) );
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

	ReadCheckType(SG_CHECK_PHYS);
}

/*
===================
idRestoreGame::ReadTrace
===================
*/
void idRestoreGame::ReadTrace( trace_t &trace ) {
	RecordSaveStack( va("[%p] ReadTrace",  &trace ) );
	ReadFloat( trace.fraction );
	ReadVec3( trace.endpos );
	ReadMat3( trace.endAxis );
	ReadContactInfo( trace.c );

	ReadCheckType(SG_CHECK_PHYS);
}

/*
 ===================
 idRestoreGame::ReadTraceModel
 ===================
 */
void idRestoreGame::ReadTraceModel( idTraceModel &trace ) {
	RecordSaveStack( va("[%p] ReadTraceModel",  &trace ) );
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

	ReadCheckType(SG_CHECK_PHYS);
}

/*
=====================
idRestoreGame::ReadClipModel
=====================
*/
void idRestoreGame::ReadClipModel( idClipModel *&clipModel ) {
	RecordSaveStack( va("[%p] ReadClipModel",  &clipModel ) );
	bool restoreClipModel;

	ReadBool( restoreClipModel );
	if ( restoreClipModel ) {
		clipModel = new idClipModel();
		clipModel->Restore( this );
	} else {
		clipModel = NULL;
	}

	ReadCheckType(SG_CHECK_PHYS);
}

/*
=====================
idRestoreGame::ReadSoundCommands
=====================
*/
void idRestoreGame::ReadSoundCommands( void ) {
	gameSoundWorld->StopAllSounds();
	gameSoundWorld->ReadFromSaveGame( this );

	ReadCheckType(SG_CHECK_SOUND);
}


idStr idRestoreGame::GetFileName()
{
	return file ? file->GetName() : "";
}

// =====================
// blendo eric
//

void idRestoreGame::ReadEntityDef( const idDeclEntityDef *& ent ) {
	RecordSaveStack( va("[%p] ReadEntityDef",  &ent ) );
	idStr name;

	ReadString( name );
	if ( !name.Length() ) {
		ent = NULL;
	} else {
		ent = static_cast<const idDeclEntityDef *>( declManager->FindType( DECL_ENTITYDEF, name ) );
	}
	ReadCheckType(SG_CHECK_ENTITY);
}


void idRestoreGame::ReadVecX(idVecX& vec) {
	int num;
	ReadInt(num);
	vec.SetSize(num);
	ReadFloatArr( vec.ToFloatPtr() );
}

void idRestoreGame::ReadMatX(idMatX& mat) {
	int row, col;
	ReadInt(row);
	ReadInt(col);
	mat.SetSize(row,col);
	ReadFloatArr( mat.ToFloatPtr() );
}

void idRestoreGame::ReadFloatArr( float * arr ) {
	int num;
	ReadInt(num);

	file->Read( arr, num * sizeof(float) );
	LittleRevBytes(arr, sizeof(float), num );

	ReadCheckType(SG_CHECK_ANGLES);
}

void idRestoreGame::ReadListObject(idList<idEntity*>& list) {
	RecordSaveStack( va("[%p] ReadListObject",  &list ) );
	int num;
	ReadInt(num);
	list.SetNum(num);
	for (int idx = 0; idx < list.Num(); ++idx) {
		ReadObject(list[idx]);
	}

	ReadCheckType(SG_CHECK_LIST);
}

void idRestoreGame::ReadListString(idList<idStr>& list) {
	int num;
	ReadInt(num);
	list.SetNum(num);
	for (int idx = 0; idx < num; ++idx) {
		ReadString(list[idx]);
	}

	ReadCheckType(SG_CHECK_LIST);
}

void idRestoreGame::ReadFunction(const function_t*& func) {
	RecordSaveStack( va("[%p] ReadFunction",  &func ) );
	idStr funcname;
	ReadString( funcname );
	if ( funcname.Length() ) {
		func = gameLocal.program.FindFunction( funcname );
		if ( func == NULL ) {
			SG_Warn( "idRestoreGame::ReadFunction function not found '%s'", funcname.c_str() );
		}
	} else {
		func = NULL;
	}

	ReadCheckType(SG_CHECK_FUNC);
}

void idRestoreGame::ReadEventDef(const idEventDef*& eventFlag) {
	RecordSaveStack( va("[%p] ReadEventDef",  &eventFlag ) );
	idStr funcname;
	ReadString( funcname );
	if ( funcname.Length() ) {
		eventFlag = idEventDef::FindEvent( funcname );
		if ( eventFlag == NULL ) {
			SG_Warn( "idRestoreGame::ReadEventDef event function not found '%s'", funcname.c_str() );
		}
	} else {
		eventFlag = NULL;
	}

	ReadCheckType(SG_CHECK_FUNC);
}

void idRestoreGame::ReadCurve(idCurve_Spline<idVec3>*& curve) {
	bool bExists;
	ReadBool(bExists);

	if (bExists)
	{
		curve = new idCurve_CatmullRomSpline<idVec3>();
		int num;
		ReadInt(num); // idList<float> times
		curve->times.SetNum(num);
		for (int idx = 0; idx < curve->times.Num(); idx++)
		{
			ReadFloat(curve->times[idx]);
		}
		ReadInt(num); // idList<idVec3> values
		curve->values.SetNum(num);
		for (int idx = 0; idx < curve->values.Num(); idx++)
		{
			ReadVec3(curve->values[idx]);
		}

		ReadInt( curve->currentIndex ); // mutable int currentIndex
		ReadBool( curve->changed ); // mutable bool changed

		ReadInt( (int&)curve->boundaryType ); // boundary_t boundaryType
		ReadFloat( curve->closeTime ); // float closeTime
	}
	else
	{
		curve = nullptr;
	}

	ReadCheckType(SG_CHECK_CURVE);
}


// ==========================================
// blendo eric: test helpers


#if defined(_DEBUG) || defined(SG_ALWAYS_READ_CHECKSTRING)
void idSaveGame::WriteCheckString(idStr checkStr) { // blendo eric: due to size, should only be used for checkpoints, not individual ents
	file->WriteString(checkStr);
}
#else
void idSaveGame::WriteCheckString(idStr checkStr){}
#endif

#if defined(_DEBUG)
void idSaveGame::WriteCheckSizeMarker()
{
	if (saveDebugging)
	{
		int curSize = file->Tell();
		file->WriteInt(curSize);
	}
}

void idSaveGame::WriteCheckType(char checkVal)
{
	if (saveDebugging)
	{
		file->WriteChar(checkVal);
		WriteCheckSizeMarker();
	}
}
#else
void idSaveGame::WriteCheckSizeMarker(){}
void idSaveGame::WriteCheckType(char checkVal) {}
#endif

#if defined(_DEBUG) || defined(SG_ALWAYS_READ_CHECKSTRING)
bool idRestoreGame::ReadCheckString(const char* checkStr) {
	idStr checkStr2;
	file->ReadString(checkStr2);
	bool validStr = checkStr2.Cmp(checkStr) == 0;
	if (!validStr) {
		Error("saveload data corrupt before savefile load checkpoint: %s", checkStr);
	}
	return validStr;
}
#else
bool idRestoreGame::ReadCheckString(const char* checkStr) { return true; }
#endif

#if defined(_DEBUG)
	bool idRestoreGame::ReadCheckSizeMarker() {
		if (saveDebugging) {
			int actualOffset = file->Tell();
			int checkOffset;
			file->ReadInt(checkOffset);
			bool validOffset = actualOffset == checkOffset;
			if (!validOffset) {
				Error("saveload data corrupt at savefile location: %d b", actualOffset);
			}
			return validOffset;
		}
		return true;
	}

	bool idRestoreGame::ReadCheckType(char checkType) {
		if (saveDebugging) {
			char readType;
			file->ReadChar(readType);
			int dbgReadInt = (int)readType; // compiler keeps complaining unless char=int=enum
			int dbgCheckInt = (int)checkType;
			SG_CHECK_TYPES dbgRead = static_cast<SG_CHECK_TYPES>(dbgReadInt);
			SG_CHECK_TYPES dbgCheck = static_cast<SG_CHECK_TYPES>(dbgCheckInt);
			bool validType = dbgRead == dbgCheck;
			if (!validType) {
				Error("saveload data corrupt in savefile, mismatched type!");
			}
			bool validSize = ReadCheckSizeMarker();
			return validType && validSize;
		}
		return true;
	}
#else
bool idRestoreGame::ReadCheckSizeMarker() { return true; }
bool idRestoreGame::ReadCheckType(char checkType) { return true; }
#endif

