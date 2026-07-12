/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

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

#include "../idlib/precompiled.h"
#pragma hdrstop

#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: BSE
#ifdef _RAVEN_BSE
#include "../raven/bse/BSE.h"
#else
#include "../raven/fx/BSE.h"
#endif
#endif

#ifdef _SPLASHDAMAGE //karin: binary data info

#define GENERATED_PREFIX "generated"
#define GENERATED_DECLB "declb"

#define CACHEB_MAGIC 1212367428
#define CACHEB_MAGIC_CHARS "DBCH"
#define CACHEB_VERSION 2

#define DCLB_MAGIC 1112294212
#define DCLB_MAGIC_CHARS "DCLB"
#define DCLB_VERSION 3

#define GLOBALTOKENS_CACHEB "globaltokens.cache"

#define DECL_CUSTOMER_TYPE(x) (DECL_MAPDEF + 1 + (x))

extern const char* declIdentifierList[];

struct binaryTokenCache_t {
	int version; // 2
	unsigned int uncompressedLength;
	unsigned int compressedLength;
	idList<byte> compressedData;
};

struct binaryDeclEntry_t {
	idStr type;
	idStr name;
	//byte whitespace;
	unsigned int offset;

	unsigned int uncompressedLength;
	unsigned int compressedLength;
	idList<byte> data;

	bool IsCompressed() const {
		return uncompressedLength != compressedLength; // invalid, maybe equals
	}
};

struct binaryDecl_t {
	int version; // 3

	int checksum;
	int num;

	idList<binaryDeclEntry_t> entries;
};

#endif

#ifdef _RAVEN
// jmarshall: Quake 4 Guide(template) support
struct rvGuideTemplate
{
    idStr name;
    idList<idStr> parms;
    idStr body;
    bool inlineGuide;
};
// jmarshall end
#endif

/*

GUIs and script remain separately parsed

Following a parse, all referenced media (and other decls) will have been touched.

sinTable and cosTable are required for the rotate material keyword to function

A new FindType on a purged decl will cause it to be reloaded, but a stale pointer to a purged
decl will look like a defaulted decl.

Moving a decl from one file to another will not be handled correctly by a reload, the material
will be defaulted.

NULL or empty decl names will always return NULL
	Should probably make a default decl for this

Decls are initially created without a textSource
A parse without textSource set should always just call MakeDefault()
A parse that has an error should internally call MakeDefault()
A purge does nothing to a defaulted decl

Should we have a "purged" media state separate from the "defaulted" media state?

reloading over a decl name that was defaulted

reloading over a decl name that was valid

missing reload over a previously explicit definition

*/

//#if !defined(_SPLASHDAMAGE)
#define USE_COMPRESSED_DECLS
//#define GET_HUFFMAN_FREQUENCIES
//#endif

#if !defined(_SPLASHDAMAGE) //karin: move to heaeder
class idDeclType
{
	public:
		idStr						typeName;
		declType_t					type;
		idDecl *(*allocator)(void);
};
#endif

class idDeclFolder
{
	public:
		idStr						folder;
		idStr						extension;
		declType_t					defaultType;
};

class idDeclFile;

class idDeclLocal : public idDeclBase
{
		friend class idDeclFile;
		friend class idDeclManagerLocal;

	public:
		idDeclLocal();
		virtual 					~idDeclLocal() {};
		virtual const char 		*GetName(void) const;
		virtual declType_t			GetType(void) const;
		virtual declState_t			GetState(void) const;
		virtual bool				IsImplicit(void) const;
		virtual bool				IsValid(void) const;
		virtual void				Invalidate(void);
		virtual void				Reload(void);
		virtual void				EnsureNotPurged(void);
		virtual int					Index(void) const;
		virtual int					GetLineNum(void) const;
		virtual const char 		*GetFileName(void) const;
		virtual size_t				Size(void) const;
		virtual void				GetText(char *text) const;
		virtual int					GetTextLength(void) const;
		virtual void				SetText(const char *text);
		virtual bool				ReplaceSourceFileText(void);
		virtual bool				SourceFileChanged(void) const;
		virtual void				MakeDefault(void);
		virtual bool				EverReferenced(void) const;
#ifdef _SPLASHDAMAGE
		virtual void			SetBinarySource( const byte* source, int length );
		virtual void			GetBinarySource( byte*& source, int& length ) const;
		virtual void			FreeSourceBuffer( byte* buffer ) const;
		virtual bool			HasBinaryBuffer() const;
		virtual const idStrList&	GetIncludeDependencies() const;
    	virtual const idStrList*	GetFileLevelIncludeDependencies() const;
		void					AddIncludeDependency(const char *file);
#endif

	protected:
		virtual bool				SetDefaultText(void);
		virtual const char 		*DefaultDefinition(void) const;
#ifdef _RAVEN
		virtual bool			Parse(const char *text, const int textLength, bool precache = false);
#else
		virtual bool				Parse(const char *text, const int textLength);
#endif
		virtual void				FreeData(void);
		virtual void				List(void) const;
		virtual void				Print(void) const;

	protected:
		void						AllocateSelf(void);

		// Parses the decl definition.
		// After calling parse, a decl will be guaranteed usable.
		void						ParseLocal(void);

		// Does a MakeDefualt, but flags the decl so that it
		// will Parse() the next time the decl is found.
		void						Purge(void);

		// Set textSource possible with compression.
		void						SetTextLocal(const char *text, const int length);

	private:
		idDecl 					*self;

		idStr						name;					// name of the decl
		char 						*textSource;				// decl text definition
		int							textLength;				// length of textSource
		int							compressedLength;		// compressed length
		idDeclFile 				*sourceFile;				// source file in which the decl was defined
		int							sourceTextOffset;		// offset in source file to decl text
		int							sourceTextLength;		// length of decl text in source file
		int							sourceLine;				// this is where the actual declaration token starts
		int							checksum;				// checksum of the decl text
		declType_t					type;					// decl type
		declState_t					declState;				// decl state
		int							index;					// index in the per-type list

		bool						parsedOutsideLevelLoad;	// these decls will never be purged
		bool						everReferenced;			// set to true if the decl was ever used
		bool						referencedThisLevel;	// set to true when the decl is used for the current level
		bool						redefinedInReload;		// used during file reloading to make sure a decl that has
		// its source removed will be defaulted
		idDeclLocal 				*nextInFile;				// next decl in the decl file
#ifdef _SPLASHDAMAGE //karin: binary decls
		idStrList					includeDependencies;
		idList<byte>				binarySource;
#endif
};

class idDeclFile
{
	public:
		idDeclFile();
		idDeclFile(const char *fileName, declType_t defaultType);

		void						Reload(bool force);
		int							LoadAndParse();

	public:
		idStr						fileName;
		declType_t					defaultType;

		ID_TIME_T						timestamp;
		int							checksum;
		int							fileSize;
		int							numLines;

		idDeclLocal 				*decls;
#ifdef _RAVEN // quake4 guide
// jmarshall: guide support
	private:
	    idStr						PreprocessGuides(const char* buffer, int length);
	    idStr						PreprocessInlineGuides(const char* buffer, int length);
// jmarshall end
#endif
#ifdef _SPLASHDAMAGE //karin: binary decls
		void						MakeBinaryFilename(idStr &filename);
		int							LoadAndParseBinary(void);
#endif
};

class idDeclManagerLocal : public idDeclManager
{
		friend class idDeclLocal;

	public:
		virtual void				Init(void);
		virtual void				Shutdown(void);
		virtual void				Reload(bool force);
		virtual void				BeginLevelLoad();
		virtual void				EndLevelLoad();
		virtual void				RegisterDeclType(const char *typeName, declType_t type, idDecl *(*allocator)(void));
		virtual void				RegisterDeclFolder(const char *folder, const char *extension, declType_t defaultType);
		virtual int					GetChecksum(void) const;
		virtual int					GetNumDeclTypes(void) const;
		virtual int					GetNumDecls(declType_t type);
		virtual const char 		*GetDeclNameFromType(declType_t type) const;
		virtual declType_t			GetDeclTypeFromName(const char *typeName) const;
		virtual const idDecl 		*FindType(declType_t type, const char *name, bool makeDefault = true);
		virtual const idDecl 		*DeclByIndex(declType_t type, int index, bool forceParse = true);

		virtual const idDecl		*FindDeclWithoutParsing(declType_t type, const char *name, bool makeDefault = true);
		virtual void				ReloadFile(const char *filename, bool force);

		virtual void				ListType(const idCmdArgs &args, declType_t type);
		virtual void				PrintType(const idCmdArgs &args, declType_t type);

		virtual idDecl 			*CreateNewDecl(declType_t type, const char *name, const char *fileName);

		//BSM Added for the material editors rename capabilities
		virtual bool				RenameDecl(declType_t type, const char *oldName, const char *newName);

		virtual void				MediaPrint(const char *fmt, ...) id_attribute((format(printf,2,3)));
		virtual void				WritePrecacheCommands(idFile *f);

		virtual const idMaterial 		*FindMaterial(const char *name, bool makeDefault = true);
		virtual const idDeclSkin 		*FindSkin(const char *name, bool makeDefault = true);
		virtual const idSoundShader 	*FindSound(const char *name, bool makeDefault = true);
#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: recurse subfolders
	    void						RegisterDeclSubFolder(const char* folder, const char* extension, idList<idStr>& fileList, bool norecurse = false);
		void						RegisterDeclFolderWrapper( const char *folder, const char *extension, declType_t defaultType, bool unique = false, bool norecurse = false );
#endif
#ifdef _RAVEN
		virtual const idDeclTable *		FindTable( const char *name, bool makeDefault = true );
		// RAVEN BEGIN
		// jscott: for new Raven decls
		virtual const rvDeclMatType *	FindMaterialType( const char *name, bool makeDefault = true );
		virtual	const rvDeclLipSync *	FindLipSync( const char *name, bool makeDefault = true );
		virtual	const rvDeclPlayback *	FindPlayback( const char *name, bool makeDefault = true );
		virtual	const rvDeclEffect *	FindEffect( const char *name, bool makeDefault = true );
		// RAVEN END
		// jscott: for new Raven decls
		virtual const rvDeclMatType *	MaterialTypeByIndex( int index, bool forceParse = true );
		virtual const rvDeclLipSync *	LipSyncByIndex( int index, bool forceParse = true );
		virtual	const rvDeclPlayback *	PlaybackByIndex( int index, bool forceParse = true );
		virtual const rvDeclEffect *	EffectByIndex( int index, bool forceParse = true );

// RAVEN BEGIN
// jscott: precache any guide (template) files
		virtual void				ParseGuides(void);
		virtual	void				ShutdownGuides(void) { }
		virtual bool				EvaluateGuide(idStr& name, idLexer* src, idStr& definition) {
			return false;
		}
		virtual bool				EvaluateInlineGuide(idStr& name, idStr& definition) {
			return false;
		}
// RAVEN END

		virtual bool					GetPlaybackData( const rvDeclPlayback *playback, int control, int now, int last, class rvDeclPlaybackData *pbd ) { (void)playback; (void)control; (void)now; (void)last; (void) pbd; return false; }
		virtual bool					SetPlaybackData(rvDeclPlayback* playback, int now, int control, class rvDeclPlaybackData* pbd) { (void)playback; (void)control; (void)now; (void) pbd; return false; }
		virtual void					StartPlaybackRecord(rvDeclPlayback* playback) { (void)playback; }
		virtual bool					FinishPlayback( rvDeclPlayback *playback ) { (void)playback; return false; }

		virtual const idDecl *	FindType( declType_t type, const char *name, bool makeDefault, bool noCaching ) { (void)noCaching; return FindType(type, name, makeDefault); }

		//k: find map def
		virtual const idDeclEntityDef * FindMapDef(const char *mapName, const char *entityFilter = 0) const {
			return GetMapDef(mapName, entityFilter);
		}
		virtual idDeclEntityDef * FindMapDef(const char *mapName, const char *entityFilter = 0) {
			return const_cast<idDeclEntityDef *>(GetMapDef(mapName, entityFilter));
		}

	private:
		const idDeclEntityDef * GetMapDef(const char *mapName, const char *entityFilter) const;

	public:
		// jmarshall - Quake 4 guide(template) support
		idList<rvGuideTemplate>		guides;
		// jmarshall end
#endif
#ifdef _HUMANHEAD
        virtual const hhDeclBeam *		FindBeam( const char *name, bool makeDefault = true );
        virtual const hhDeclBeam *		BeamByIndex( int index, bool forceParse = true );
        virtual void					SetInsideLevelLoad(bool b) {
            inLevelLoad = b;
        }
        virtual bool					GetInsideLevelLoad(void) {
            return inLevelLoad;
        }
#endif
#ifdef _SPLASHDAMAGE
		// Returns the system token cache
		virtual idTokenCache&	GetGlobalTokenCache();

		// Registers a new decl type.
		virtual void			RegisterDeclType( idDeclTypeInterface* type );
		virtual void			UnregisterDeclType( idDeclTypeInterface* type );

		// Registers a new folder with decl files.
		virtual void			RegisterDeclFolder( const char *folder, const char *extension );

		//Unregister a previously-registered folder
		virtual void			UnregisterDeclFolder( const char *folder, const char *extension );

		// Called when finished registering decl folders
		// attempts to find binary decls without a source text file and load them properly
		virtual void			FinishedRegistering();
		virtual void			ListType( const idCmdArgs &args, const char* typeName );
		virtual void			PrintType( const idCmdArgs &args, const char* typeName );
		virtual int						GetNumMaterials( void );
		virtual void					CacheFromDict( const idDict& dict );
		virtual	const rvDeclEffect *	FindEffect( const char *name, bool makeDefault = true );
		virtual idDeclTypeInterface*	GetDeclType( const char* typeName ) const;
		virtual idDeclTypeInterface*	GetDeclType( qhandle_t typeHandle ) const;
		virtual qhandle_t				GetDeclTypeHandle( const char* typeName ) const;
		virtual const char*				GetDeclTypeName( qhandle_t typeHandle ) const;

		virtual void					AddDependency( const idDecl* decl, const idDecl* dependency );
		virtual void					AddDependency( const idDecl* decl, const char* fileName );
		virtual void					AddDependencies( const idDecl* decl, const idParser& parser );

		bool							LoadGlobalTokenCache(void);
		void							RegisterDeclFolderWrapperBinary( idDeclFolder *declFolder, bool unique = false, bool norecurse = false );
#endif

		virtual const idMaterial 		*MaterialByIndex(int index, bool forceParse = true);
		virtual const idDeclSkin 		*SkinByIndex(int index, bool forceParse = true);
		virtual const idSoundShader 	*SoundByIndex(int index, bool forceParse = true);

        virtual const idDecl 	        *AddDeclDef(const char *defname, declType_t type, const idDict &args, bool force = false);
		virtual bool					EntityDefSet(const char *name, const char *key, const char *value = NULL);

public:
		static void					MakeNameCanonical(const char *name, char *result, int maxLength);
		idDeclLocal 				*FindTypeWithoutParsing(declType_t type, const char *name, bool makeDefault = true);

#if !defined(_SPLASHDAMAGE)
		idDeclType 				*GetDeclType(int type) const {
			return declTypes[type];
		}
#endif
		const idDeclFile 			*GetImplicitDeclFile(void) const {
			return &implicitDecls;
		}

	private:
		idList<idDeclType *>		declTypes;
		idList<idDeclFolder *>		declFolders;

		idList<idDeclFile *>		loadedFiles;
		idHashIndex					hashTables[DECL_MAX_TYPES];
		idList<idDeclLocal *>		linearLists[DECL_MAX_TYPES];
		idDeclFile					implicitDecls;	// this holds all the decls that were created because explicit
		// text definitions were not found. Decls that became default
		// because of a parse error are not in this list.
		int							checksum;		// checksum of all loaded decl text
		int							indent;			// for MediaPrint
		bool						insideLevelLoad;

		static idCVar				decl_show;

#ifdef _HUMANHEAD
        bool						inLevelLoad;
#endif
#ifdef _SPLASHDAMAGE
		idTokenCache				globalTokencache;
		mutable idStrList			declTypeTables;

private:
		static void					DeclbToText_f(const idCmdArgs &args);
		static void					ExportDeclSource_f(const idCmdArgs &args);
		static void					ExportDeclExpandSource_f(const idCmdArgs &args);
		void						ExportDeclSource(const char *savePath, const char *filePath = NULL, bool expand = false);
#endif

	private:
		static void					ListDecls_f(const idCmdArgs &args);
		static void					ReloadDecls_f(const idCmdArgs &args);
		static void					TouchDecl_f(const idCmdArgs &args);
		static void					ParseAllDecls_f(const idCmdArgs &args);
};

idCVar idDeclManagerLocal::decl_show("decl_show", "0", CVAR_SYSTEM, "set to 1 to print parses, 2 to also print references", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2>);

idDeclManagerLocal	declManagerLocal;
idDeclManager 		*declManager = &declManagerLocal;

/*
====================================================================================

 decl text huffman compression

====================================================================================
*/

const int MAX_HUFFMAN_SYMBOLS	= 256;

typedef struct huffmanNode_s {
	int						symbol;
	int						frequency;
	struct huffmanNode_s 	*next;
	struct huffmanNode_s 	*children[2];
} huffmanNode_t;

typedef struct huffmanCode_s {
	unsigned long			bits[8];
	int						numBits;
} huffmanCode_t;

// compression ratio = 64%
static int huffmanFrequencies[] = {
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00078fb6, 0x000352a7, 0x00000002, 0x00000001, 0x0002795e, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00049600, 0x000000dd, 0x00018732, 0x0000005a, 0x00000007, 0x00000092, 0x0000000a, 0x00000919,
	0x00002dcf, 0x00002dda, 0x00004dfc, 0x0000039a, 0x000058be, 0x00002d13, 0x00014d8c, 0x00023c60,
	0x0002ddb0, 0x0000d1fc, 0x000078c4, 0x00003ec7, 0x00003113, 0x00006b59, 0x00002499, 0x0000184a,
	0x0000250b, 0x00004e38, 0x000001ca, 0x00000011, 0x00000020, 0x000023da, 0x00000012, 0x00000091,
	0x0000000b, 0x00000b14, 0x0000035d, 0x0000137e, 0x000020c9, 0x00000e11, 0x000004b4, 0x00000737,
	0x000006b8, 0x00001110, 0x000006b3, 0x000000fe, 0x00000f02, 0x00000d73, 0x000005f6, 0x00000be4,
	0x00000d86, 0x0000014d, 0x00000d89, 0x0000129b, 0x00000db3, 0x0000015a, 0x00000167, 0x00000375,
	0x00000028, 0x00000112, 0x00000018, 0x00000678, 0x0000081a, 0x00000677, 0x00000003, 0x00018112,
	0x00000001, 0x000441ee, 0x000124b0, 0x0001fa3f, 0x00026125, 0x0005a411, 0x0000e50f, 0x00011820,
	0x00010f13, 0x0002e723, 0x00003518, 0x00005738, 0x0002cc26, 0x0002a9b7, 0x0002db81, 0x0003b5fa,
	0x000185d2, 0x00001299, 0x00030773, 0x0003920d, 0x000411cd, 0x00018751, 0x00005fbd, 0x000099b0,
	0x00009242, 0x00007cf2, 0x00002809, 0x00005a1d, 0x00000001, 0x00005a1d, 0x00000001, 0x00000001,

	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
	0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
};

static huffmanCode_t huffmanCodes[MAX_HUFFMAN_SYMBOLS];
static huffmanNode_t *huffmanTree = NULL;
static int totalUncompressedLength = 0;
static int totalCompressedLength = 0;
static int maxHuffmanBits = 0;


/*
================
ClearHuffmanFrequencies
================
*/
void ClearHuffmanFrequencies(void)
{
	int i;

	for (i = 0; i < MAX_HUFFMAN_SYMBOLS; i++) {
		huffmanFrequencies[i] = 1;
	}
}

/*
================
InsertHuffmanNode
================
*/
huffmanNode_t *InsertHuffmanNode(huffmanNode_t *firstNode, huffmanNode_t *node)
{
	huffmanNode_t *n, *lastNode;

	lastNode = NULL;

	for (n = firstNode; n; n = n->next) {
		if (node->frequency <= n->frequency) {
			break;
		}

		lastNode = n;
	}

	if (lastNode) {
		node->next = lastNode->next;
		lastNode->next = node;
	} else {
		node->next = firstNode;
		firstNode = node;
	}

	return firstNode;
}

/*
================
BuildHuffmanCode_r
================
*/
void BuildHuffmanCode_r(huffmanNode_t *node, huffmanCode_t code, huffmanCode_t codes[MAX_HUFFMAN_SYMBOLS])
{
	if (node->symbol == -1) {
		huffmanCode_t newCode = code;
		assert(code.numBits < sizeof(codes[0].bits) * 8);
		newCode.numBits++;

		if (code.numBits > maxHuffmanBits) {
			maxHuffmanBits = newCode.numBits;
		}

		BuildHuffmanCode_r(node->children[0], newCode, codes);
		newCode.bits[code.numBits >> 5] |= 1 << (code.numBits & 31);
		BuildHuffmanCode_r(node->children[1], newCode, codes);
	} else {
		assert(code.numBits <= sizeof(codes[0].bits) * 8);
		codes[node->symbol] = code;
	}
}

/*
================
FreeHuffmanTree_r
================
*/
void FreeHuffmanTree_r(huffmanNode_t *node)
{
	if (node->symbol == -1) {
		FreeHuffmanTree_r(node->children[0]);
		FreeHuffmanTree_r(node->children[1]);
	}

	delete node;
}

/*
================
HuffmanHeight_r
================
*/
int HuffmanHeight_r(huffmanNode_t *node)
{
	if (node == NULL) {
		return -1;
	}

	int left = HuffmanHeight_r(node->children[0]);
	int right = HuffmanHeight_r(node->children[1]);

	if (left > right) {
		return left + 1;
	}

	return right + 1;
}

/*
================
SetupHuffman
================
*/
void SetupHuffman(void)
{
	int i, height;
	huffmanNode_t *firstNode, *node;
	huffmanCode_t code;

	firstNode = NULL;

	for (i = 0; i < MAX_HUFFMAN_SYMBOLS; i++) {
		node = new huffmanNode_t;
		node->symbol = i;
		node->frequency = huffmanFrequencies[i];
		node->next = NULL;
		node->children[0] = NULL;
		node->children[1] = NULL;
		firstNode = InsertHuffmanNode(firstNode, node);
	}

	for (i = 1; i < MAX_HUFFMAN_SYMBOLS; i++) {
		node = new huffmanNode_t;
		node->symbol = -1;
		node->frequency = firstNode->frequency + firstNode->next->frequency;
		node->next = NULL;
		node->children[0] = firstNode;
		node->children[1] = firstNode->next;
		firstNode = InsertHuffmanNode(firstNode->next->next, node);
	}

	maxHuffmanBits = 0;
	memset(&code, 0, sizeof(code));
	BuildHuffmanCode_r(firstNode, code, huffmanCodes);

	huffmanTree = firstNode;

	height = HuffmanHeight_r(firstNode);
	assert(maxHuffmanBits == height);
}

/*
================
ShutdownHuffman
================
*/
void ShutdownHuffman(void)
{
	if (huffmanTree) {
		FreeHuffmanTree_r(huffmanTree);
	}
}

/*
================
HuffmanCompressText
================
*/
int HuffmanCompressText(const char *text, int textLength, byte *compressed, int maxCompressedSize)
{
	int i, j;
	idBitMsg msg;

	totalUncompressedLength += textLength;

	msg.Init(compressed, maxCompressedSize);
	msg.BeginWriting();

	for (i = 0; i < textLength; i++) {
		const huffmanCode_t &code = huffmanCodes[(unsigned char)text[i]];

		for (j = 0; j < (code.numBits >> 5); j++) {
			msg.WriteBits(code.bits[j], 32);
		}

		if (code.numBits & 31) {
			msg.WriteBits(code.bits[j], code.numBits & 31);
		}
	}

	totalCompressedLength += msg.GetSize();

	return msg.GetSize();
}

/*
================
HuffmanDecompressText
================
*/
int HuffmanDecompressText(char *text, int textLength, const byte *compressed, int compressedSize)
{
	int i, bit;
	idBitMsg msg;
	huffmanNode_t *node;

	msg.Init(compressed, compressedSize);
	msg.SetSize(compressedSize);
	msg.BeginReading();

	for (i = 0; i < textLength; i++) {
		node = huffmanTree;

		do {
			bit = msg.ReadBits(1);
			node = node->children[bit];
		} while (node->symbol == -1);

		text[i] = node->symbol;
	}

	text[i] = '\0';
	return msg.GetReadCount();
}

/*
================
ListHuffmanFrequencies_f
================
*/
void ListHuffmanFrequencies_f(const idCmdArgs &args)
{
	int		i;
	float compression;
	compression = !totalUncompressedLength ? 100 : 100 * totalCompressedLength / totalUncompressedLength;
	common->Printf("// compression ratio = %d%%\n", (int)compression);
	common->Printf("static int huffmanFrequencies[] = {\n");

	for (i = 0; i < MAX_HUFFMAN_SYMBOLS; i += 8) {
		common->Printf("\t0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
		               huffmanFrequencies[i+0], huffmanFrequencies[i+1],
		               huffmanFrequencies[i+2], huffmanFrequencies[i+3],
		               huffmanFrequencies[i+4], huffmanFrequencies[i+5],
		               huffmanFrequencies[i+6], huffmanFrequencies[i+7]);
	}

	common->Printf("}\n");
}

/*
====================================================================================

 idDeclFile

====================================================================================
*/

/*
================
idDeclFile::idDeclFile
================
*/
idDeclFile::idDeclFile(const char *fileName, declType_t defaultType)
{
	this->fileName = fileName;
	this->defaultType = defaultType;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
idDeclFile::idDeclFile
================
*/
idDeclFile::idDeclFile()
{
	this->fileName = "<implicit file>";
	this->defaultType = DECL_MAX_TYPES;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
idDeclFile::Reload

ForceReload will cause it to reload even if the timestamp hasn't changed
================
*/
void idDeclFile::Reload(bool force)
{
	// check for an unchanged timestamp
	if (!force && timestamp != 0) {
		ID_TIME_T	testTimeStamp;
		fileSystem->ReadFile(fileName, NULL, &testTimeStamp);

		if (testTimeStamp == timestamp) {
			return;
		}
		//karin: 2025 don't load and parse if file not exists
		if (testTimeStamp == FILE_NOT_FOUND_TIMESTAMP) {
			return;
		}
	}

	// parse the text
	LoadAndParse();
}

/*
================
idDeclFile::LoadAndParse

This is used during both the initial load, and any reloads
================
*/
int c_savedMemory = 0;

int idDeclFile::LoadAndParse()
{
	int			i, numTypes;
#ifdef _SPLASHDAMAGE //karin: idParser instead of idLexer
	idParser	src;
#else
	idLexer		src;
#endif
	idToken		token;
	int			startMarker;
	char 		*buffer;
	int			length, size;
	int			sourceLine;
	idStr		name;
	idDeclLocal *newDecl;
	bool		reparse;

#ifdef _RAVEN // quake4 guide
// jmarshall: quake 4 guide support
    bool		canUseGuides = strstr(fileName, ".mtr");
// jmarshall end
#endif

	// load the text
	common->DPrintf("...loading '%s'\n", fileName.c_str());
	length = fileSystem->ReadFile(fileName, (void **)&buffer, &timestamp);

	if (length == -1) {
#ifdef _SPLASHDAMAGE //karin: try parse binary declb file if ascii file load failed
		if(!LoadAndParseBinary())
		{
#endif
		common->FatalError("couldn't load %s", fileName.c_str());
		return 0;
#ifdef _SPLASHDAMAGE
		}
#endif
	}

#ifdef _RAVEN // quake4 guide
// jmarshall: quake 4 guide support
    idStr finalPreprocessedBuffer;

    if (!canUseGuides)
    {
        finalPreprocessedBuffer = buffer;
    }
    else
    {
        finalPreprocessedBuffer = PreprocessGuides(buffer, length);
    }

    //fileSystem->WriteFile(va("generated/%s", fileName.c_str()), finalPreprocessedBuffer.c_str(), finalPreprocessedBuffer.Length());

    //k Mem_Free(buffer);
// jmarshall end
#endif

#ifdef _RAVEN
	if (!src.LoadMemory(finalPreprocessedBuffer.c_str(), finalPreprocessedBuffer.Length(), fileName))
#else
	if (!src.LoadMemory(buffer, length, fileName))
#endif
	{
#ifdef _SPLASHDAMAGE //karin: try parse binary declb file if ascii file load failed
		Mem_Free(buffer);
		if(!LoadAndParseBinary())
		{
#endif
		common->Error("Couldn't parse %s", fileName.c_str());
#if !defined(_SPLASHDAMAGE)
		Mem_Free(buffer);
#endif
		return 0;
#ifdef _SPLASHDAMAGE
		}
#endif
	}

	// mark all the defs that were from the last reload of this file
	for (idDeclLocal *decl = decls; decl; decl = decl->nextInFile) {
		decl->redefinedInReload = false;
	}

	src.SetFlags(DECL_LEXER_FLAGS);

#ifdef _RAVEN
	checksum = MD5_BlockChecksum(finalPreprocessedBuffer.c_str(), finalPreprocessedBuffer.Length());
#else
	checksum = MD5_BlockChecksum(buffer, length);
#endif

	fileSize = length;

	// scan through, identifying each individual declaration
#ifdef _SPLASHDAMAGE //karin: don't need to add include files if first decl
	src.PushDependencies();
	bool isFirst = true; // if is first decl in this file
#endif
	while (1) {
#ifdef _SPLASHDAMAGE //karin: fisrt decl source since postion 0, the source has include depences, so it don't need add depence includes
		const bool addIncludes = !isFirst;
		isFirst = false; // mark not first decl now
#endif

		startMarker = src.GetFileOffset();
		sourceLine = src.GetLineNum();

		// parse the decl type name
		if (!src.ReadToken(&token)) {
			break;
		}

		declType_t identifiedType = DECL_MAX_TYPES;

		// get the decl type from the type name
		numTypes = declManagerLocal.GetNumDeclTypes();

		for (i = 0; i < numTypes; i++) {
#ifdef _SPLASHDAMAGE
			idDeclTypeInterface *typeInfo = declManagerLocal.GetDeclType(i);
#else
			idDeclType *typeInfo = declManagerLocal.GetDeclType(i);
#endif

			if (typeInfo && typeInfo->typeName.Icmp(token) == 0) {
				identifiedType = (declType_t) typeInfo->type;
				break;
			}
		}

		if (i >= numTypes) {

			if (token.Icmp("{") == 0) {

				// if we ever see an open brace, we somehow missed the [type] <name> prefix
				src.Warning("Missing decl name");
				src.SkipBracedSection(false);
				continue;

			} else {

				if (defaultType == DECL_MAX_TYPES) {
					src.Warning("No type");
					continue;
				}

				src.UnreadToken(&token);
				// use the default type
				identifiedType = defaultType;
			}
		}

		// now parse the name
		if (!src.ReadToken(&token)) {
			src.Warning("Type without definition at end of file");
			break;
		}

		if (!token.Icmp("{")) {
			// if we ever see an open brace, we somehow missed the [type] <name> prefix
			src.Warning("Missing decl name");
			src.SkipBracedSection(false);
			continue;
		}

		// FIXME: export decls are only used by the model exporter, they are skipped here for now
		if (identifiedType == DECL_MODELEXPORT) {
			src.SkipBracedSection();
			continue;
		}
#ifdef _SPLASHDAMAGE //karin: save decl include files
		idStrList dependencies;
		int cd = src.GetCurrentDependency();
		for(const char *d = src.GetNextDependency(cd); d; d = src.GetNextDependency(cd)) {
			dependencies.AddUnique(d);
		}
		src.PushDependencies();
#endif

		name = token;

		// make sure there's a '{'
		if (!src.ReadToken(&token)) {
			src.Warning("Type without definition at end of file");
#ifdef _SPLASHDAMAGE //karin: save decl include files
			src.PopDependencies();
#endif
			break;
		}

		if (token != "{") {
			src.Warning("Expecting '{' but found '%s'", token.c_str());
#ifdef _SPLASHDAMAGE //karin: save decl include files
			src.PopDependencies();
#endif
			continue;
		}

		src.UnreadToken(&token);

		// now take everything until a matched closing brace
		src.SkipBracedSection();
		size = src.GetFileOffset() - startMarker;

		// look it up, possibly getting a newly created default decl
		reparse = false;
		newDecl = declManagerLocal.FindTypeWithoutParsing(identifiedType, name, false);

		if (newDecl) {
			// update the existing copy
			if (newDecl->sourceFile != this || newDecl->redefinedInReload) {
				src.Warning("%s '%s' previously defined at %s:%i", declManagerLocal.GetDeclNameFromType(identifiedType),
				            name.c_str(), newDecl->sourceFile->fileName.c_str(), newDecl->sourceLine);
#ifdef _SPLASHDAMAGE //karin: save decl include files
				src.PopDependencies();
#endif
				continue;
			}

			if (newDecl->declState != DS_UNPARSED) {
				reparse = true;
			}
		} else {
			// allow it to be created as a default, then add it to the per-file list
			newDecl = declManagerLocal.FindTypeWithoutParsing(identifiedType, name, true);
			newDecl->nextInFile = this->decls;
			this->decls = newDecl;
		}

		newDecl->redefinedInReload = true;

		if (newDecl->textSource) {
			Mem_Free(newDecl->textSource);
			newDecl->textSource = NULL;
		}

#ifdef _RAVEN
		newDecl->SetTextLocal(finalPreprocessedBuffer.c_str() + startMarker, size);
#else
		newDecl->SetTextLocal(buffer + startMarker, size);
#endif
		newDecl->sourceFile = this;
		newDecl->sourceTextOffset = startMarker;
		newDecl->sourceTextLength = size;
		newDecl->sourceLine = sourceLine;
		newDecl->declState = DS_UNPARSED;

#ifdef _SPLASHDAMAGE //karin: add include files to decl text source
		if(addIncludes)
		{
			cd = src.GetCurrentDependency();
			for(const char *d = src.GetNextDependency(cd); d; d = src.GetNextDependency(cd)) {
				newDecl->AddIncludeDependency(d);
			}
			for(int m = 0; m < dependencies.Num(); m++) {
				newDecl->AddIncludeDependency(dependencies[m]);
			}
		}
#endif
		// if it is currently in use, reparse it immedaitely
		if (reparse) {
			newDecl->ParseLocal();
		}
#ifdef _SPLASHDAMAGE //karin: save decl include files
		src.PopDependencies();
#endif
	}

#ifdef _SPLASHDAMAGE //karin: save decl include files
	src.PopDependencies();
#endif
	numLines = src.GetLineNum();

	Mem_Free(buffer);

	// any defs that weren't redefinedInReload should now be defaulted
	for (idDeclLocal *decl = decls ; decl ; decl = decl->nextInFile) {
		if (decl->redefinedInReload == false) {
			decl->MakeDefault();
			decl->sourceTextOffset = decl->sourceFile->fileSize;
			decl->sourceTextLength = 0;
			decl->sourceLine = decl->sourceFile->numLines;
		}
	}

	return checksum;
}

#ifdef _SPLASHDAMAGE //karin: parse binary declb file/global binary token cache
static void Com_MakeBinaryFilename(idStr &filename, const char *type, const char *name) {
	filename = GENERATED_PREFIX "/" GENERATED_DECLB;
	if (type && type[0]) {
		filename.AppendPath(type);
	}
	if(name && name[0])
	{
		filename.AppendPath(name);
		filename.Append("b");
	}
}

// huffman compression
static int DecompressDeclb(binaryDeclEntry_t &entry, idList<byte> &out) {
	//Sys_Printf("EEE %s|%s|%d|%d|%d\n", entry.type.c_str(), entry.name.c_str(), entry.uncompressedLength, entry.compressedLength,strlen((char *)entry.data.Ptr()));

	idCompressor *compressor = idCompressor::AllocHuffman();
	idFile_Memory f("declb", (const char *)entry.data.Ptr(), entry.compressedLength); // FS_READ mode
	compressor->Init(&f, false, 16);
	out.SetNum(entry.uncompressedLength * 2 + 2);
	int uncompressedLength = compressor->Read(out.Ptr(), out.Num());

	delete compressor;
	return uncompressedLength;
}

void idDeclFile::MakeBinaryFilename(idStr &filename) {
	Com_MakeBinaryFilename(filename, NULL, fileName.c_str());
}

static int LoadBinaryDeclHeader(binaryDecl_t &header, idFile *file) {
	int magic;

	file->ReadInt(magic);
	if(magic != DCLB_MAGIC)
	{
		common->Warning("decl binary : encountered unknown fileid");
		return -1;
	}

	file->ReadInt(header.version); // version == 3
	if(header.version != DCLB_VERSION)
	{
		common->Warning("decl binary : wrong version (%i should be %i)", header.version, DCLB_VERSION);
		return -1;
	}

	file->ReadInt(header.checksum);
	file->ReadInt(header.num);

	if (header.num > 0) {
		header.entries.SetNum(header.num);
		for (int i = 0; i < header.num; ++i) {
			binaryDeclEntry_t &entry = header.entries[i];
			file->ReadString(entry.type);
			file->ReadString(entry.name);
			byte whitespace;
			file->ReadUnsignedChar(whitespace);
			file->ReadUnsignedInt(entry.offset);
			if((int)entry.offset >= file->Length())
			{
				common->Warning("Decl binary entry %s %s invalid offset: %d", entry.type.c_str(), entry.name.c_str(), entry.offset);
				return false;
			}
		}
	}

	return header.checksum;
}

int idDeclFile::LoadAndParseBinary(void)
{
	int			i, numTypes;
	idDeclLocal *newDecl;
	bool		reparse;
	idFile *file;
	idStr path;
	MakeBinaryFilename(path);

	common->DPrintf("Load decl binary '%s'...\n", path.c_str());

	file = fileSystem->OpenFileRead(path);

	if(!file)
	{
		common->Warning("Decl binary file not exists: %s", path.c_str());
		return 0;
	}

	binaryDecl_t header;
	if (LoadBinaryDeclHeader(header, file) == -1) {
		common->Warning("Decl binary read 0 entries: %s", path.c_str());
		fileSystem->CloseFile(file);
		return 0;
	}

	// mark all the defs that were from the last reload of this file
	for (idDeclLocal *decl = decls; decl; decl = decl->nextInFile) {
		decl->redefinedInReload = false;
	}

	fileSize = file->Length();

	for (int m = 0; m < header.num; ++m) {
		binaryDeclEntry_t &entry = header.entries[m];

		declType_t identifiedType = DECL_MAX_TYPES;

		// get the decl type from the type name
		numTypes = declManagerLocal.GetNumDeclTypes();

		for (i = 0; i < numTypes; i++) {
			idDeclTypeInterface *typeInfo = declManagerLocal.GetDeclType(i);

			if (typeInfo && typeInfo->typeName.Icmp(entry.type) == 0) {
				identifiedType = (declType_t) typeInfo->type;
				break;
			}
		}

		if (i >= numTypes) {
			if (defaultType == DECL_MAX_TYPES) {
				common->Warning("No btype");
				continue;
			}
			// use the default type
			identifiedType = defaultType;
		}

		//Sys_Printf("fff %s %s\n", entry.type.c_str(), entry.name.c_str());
		const idStr &name = entry.name;

		// look it up, possibly getting a newly created default decl
		reparse = false;
		newDecl = declManagerLocal.FindTypeWithoutParsing(identifiedType, name, false);

		if (newDecl) {
			// update the existing copy
			if (newDecl->sourceFile != this || newDecl->redefinedInReload) {
				common->Warning("%s '%s' previously defined at %s:%i", declManagerLocal.GetDeclNameFromType(identifiedType),
				            name.c_str(), newDecl->sourceFile->fileName.c_str(), newDecl->sourceLine);
				continue;
			}

			if (newDecl->declState != DS_UNPARSED) {
				reparse = true;
			}
		} else {
			// allow it to be created as a default, then add it to the per-file list
			newDecl = declManagerLocal.FindTypeWithoutParsing(identifiedType, name, true);
			newDecl->nextInFile = this->decls;
			this->decls = newDecl;
		}

		newDecl->redefinedInReload = true;

		if (newDecl->textSource) {
			Mem_Free(newDecl->textSource);
			newDecl->textSource = NULL;
		}
		newDecl->SetBinarySource(NULL, 0);

		//int pos = file->Tell();
		file->Seek(entry.offset, FS_SEEK_SET);
		file->ReadUnsignedInt(entry.uncompressedLength);
		file->ReadUnsignedInt(entry.compressedLength);
		entry.data.SetNum(entry.compressedLength);
		file->Read(entry.data.Ptr(), entry.compressedLength);
		//file->Seek(pos, FS_SEEK_SET);

		idList<byte> out;
		DecompressDeclb(entry, out);

		if(out[0] == 6 && !idStr::Cmpn((const char *)&out[4], LEXB_VERSION, 6)) // is binary lex: like idFile::ReadString() num:int32 chars[num]
		{
			newDecl->SetBinarySource(&out[0], entry.uncompressedLength);
			newDecl->sourceTextLength = 0;
			//Sys_Printf("is bin|%s\n", fileName.c_str());
		}
		else
		{
			newDecl->SetTextLocal((const char *)entry.data.Ptr(), entry.uncompressedLength);
			newDecl->sourceTextLength = entry.uncompressedLength;
			//Sys_Printf("is text1|%s|%s|\n", fileName.c_str(),(const char *)entry.data.Ptr());
		}

		newDecl->sourceFile = this;
		newDecl->sourceTextOffset = 0;
		newDecl->sourceLine = 1;
		newDecl->declState = DS_UNPARSED;

		// if it is currently in use, reparse it immedaitely
		if (reparse) {
			newDecl->ParseLocal();
		}
	}

	numLines = 1;

	fileSystem->CloseFile(file);

	// any defs that weren't redefinedInReload should now be defaulted
	for (idDeclLocal *decl = decls ; decl ; decl = decl->nextInFile) {
		if (decl->redefinedInReload == false) {
			decl->MakeDefault();
			decl->sourceTextOffset = decl->sourceFile->fileSize;
			decl->sourceTextLength = 0;
			decl->sourceLine = decl->sourceFile->numLines;
		}
	}

	return header.checksum;
}
#endif

/*
====================================================================================

 idDeclManagerLocal

====================================================================================
*/

const char *listDeclStrings[] = { "current", "all", "ever", NULL };

#ifdef _SPLASHDAMAGE //karin: decl info defines
sdDeclInfo declTableInfo("table", DIF_ALLOW_TEMPLATES);
sdDeclInfo declMaterialInfo("material", DIF_ALLOW_TEMPLATES, idMaterial::CacheFromDict);
sdDeclInfo declSkinInfo("skin", DIF_ALLOW_TEMPLATES, idDeclSkin::CacheFromDict);
sdDeclInfo declSoundInfo("sound", DIF_ALLOW_TEMPLATES, idSoundShader::CacheFromDict);
sdDeclInfo declEntityDefInfo("entityDef", DIF_ALLOW_TEMPLATES, idDeclEntityDef::CacheFromDict);
sdDeclInfo declMapDefInfo("mapDef");
sdDeclInfo declFxInfo("fx");
sdDeclInfo declParticleInfo("particle");
sdDeclInfo declAFInfo("articulatedFigure", DIF_ALLOW_TEMPLATES, idDeclAF::CacheFromDict);
sdDeclInfo declPDAInfo("pda");
sdDeclInfo declEmailInfo("email");
sdDeclInfo declVideoInfo("video");
sdDeclInfo declAudioInfo("audio");

sdDeclInfo declEffectInfo("effect", DIF_ALLOW_TEMPLATES/*, idDeclEntityDef::CacheFromDict*/);
sdDeclInfo declAtmosphereInfo("atmosphere", DIF_ALLOW_TEMPLATES);
sdDeclInfo declAmbientCubeMapInfo("ambientCubemap", DIF_ALLOW_TEMPLATES, sdDeclAmbientCubeMap::CacheFromDict);
sdDeclInfo declDecalInfo("decal", DIF_ALLOW_TEMPLATES);
sdDeclInfo declSurfaceTypeInfo("surfaceType", DIF_ALLOW_TEMPLATES);
sdDeclInfo declImposterInfo("imposter", DIF_ALLOW_TEMPLATES, sdDeclImposter::CacheFromDict);
sdDeclInfo declImposterGeneratorInfo("imposterGenerator");
sdDeclInfo declStuffTypeInfo("stuffType", DIF_ALLOW_TEMPLATES);
sdDeclInfo declRenderBindingInfo("renderBinding", DIF_ALLOW_TEMPLATES);
sdDeclInfo declRenderProgramInfo("renderProgram", DIF_ALLOW_TEMPLATES);
sdDeclInfo declLocStrInfo("locString", DIF_ALLOW_TEMPLATES);
sdDeclInfo declTemplateInfo("template", 0); //karin: template don't expend automatic
sdDeclInfo declSurfaceTypeMapInfo("surfaceTypeMap", DIF_ALLOW_TEMPLATES);


idDeclTypeTemplate< idDeclTable, &declTableInfo > declTableType;
idDeclTypeTemplate< idMaterial, &declMaterialInfo > declMaterialType;
idDeclTypeTemplate< idDeclSkin, &declSkinInfo > declSkinType;
idDeclTypeTemplate< idSoundShader, &declSoundInfo > declSoundType;
idDeclTypeTemplate< idDeclEntityDef, &declEntityDefInfo > declEntityDefType;
idDeclTypeTemplate< idDeclEntityDef, &declMapDefInfo > declMapDefType;
idDeclTypeTemplate< idDeclFX, &declFxInfo > declFxType;
idDeclTypeTemplate< idDeclParticle, &declParticleInfo > declParticleType;
idDeclTypeTemplate< idDeclAF, &declAFInfo > declAFType;
idDeclTypeTemplate< idDeclPDA, &declPDAInfo > declPDAType;
idDeclTypeTemplate< idDeclEmail, &declEmailInfo > declEmailType;
idDeclTypeTemplate< idDeclVideo, &declVideoInfo > declVideoType;
idDeclTypeTemplate< idDeclAudio, &declAudioInfo > declAudioType;
idDeclTypeTemplate< rvDeclEffect, &declEffectInfo > declEffectType;
idDeclTypeTemplate< sdDeclAtmosphere, &declAtmosphereInfo > declAtmosphereType;
idDeclTypeTemplate< sdDeclAmbientCubeMap, &declAmbientCubeMapInfo > declAmbientCubeMapType;
idDeclTypeTemplate< sdDeclDecal, &declDecalInfo > declDecalType;
idDeclTypeTemplate< sdDeclSurfaceType, &declSurfaceTypeInfo > declSurfaceTypeType;
idDeclTypeTemplate< sdDeclSurfaceTypeMap, &declSurfaceTypeMapInfo > declSurfaceTypeMapType;
idDeclTypeTemplate< sdDeclImposter, &declImposterInfo > declImposterType;
idDeclTypeTemplate< sdDeclImposterGenerator, &declImposterGeneratorInfo > declImposterGeneratorType;
idDeclTypeTemplate< sdDeclStuffType, &declStuffTypeInfo > declStuffTypeType;
idDeclTypeTemplate< sdDeclRenderBinding, &declRenderBindingInfo > declRenderBindingType;
idDeclTypeTemplate< sdDeclRenderProgram, &declRenderProgramInfo > declRenderProgramType;
idDeclTypeTemplate< sdDeclLocStr, &declLocStrInfo > declLocStrType;
idDeclTypeTemplate< sdDeclTemplate, &declTemplateInfo > declTemplateType;

#endif
/*
===================
idDeclManagerLocal::Init
===================
*/
void idDeclManagerLocal::Init(void)
{

	common->Printf("----- Initializing Decls -----\n");

	checksum = 0;

#ifdef _HUMANHEAD
    inLevelLoad = false;
#endif

#ifdef USE_COMPRESSED_DECLS
	SetupHuffman();
#endif

#ifdef GET_HUFFMAN_FREQUENCIES
	ClearHuffmanFrequencies();
#endif

#ifdef _RAVEN // quake4 guide
// jmarshall - template(guide) Support
    ParseGuides();
// jmarshall end
#endif

#ifdef _SPLASHDAMAGE //karin: init decl type name table
	LoadGlobalTokenCache();

	declTypeTables.Clear();
	declTypeTables.SetNum(DECL_CUSTOMER_TYPE(0));
	for(int i = 0; i <= DECL_MODELEXPORT; i++)
	{
		declTypeTables[i] = declIdentifierList[i];
	}
	// compat for DOOM3
	declTypeTables[DECL_FONT] = "font";
	declTypeTables[DECL_MODELDEF] = "modelDef";
	declTypeTables[DECL_FX] = "fx";
	declTypeTables[DECL_PARTICLE] = "particle";
	declTypeTables[DECL_PDA] = "pda";
	declTypeTables[DECL_VIDEO] = "video";
	declTypeTables[DECL_AUDIO] = "audio";
	declTypeTables[DECL_EMAIL] = "email";
	declTypeTables[DECL_MAPDEF] = "mapDef";
#endif

#ifdef _SPLASHDAMAGE //karin: register decl by info
	// decls used throughout the engine
	RegisterDeclType(&declTableType);
	RegisterDeclType(&declMaterialType);
	RegisterDeclType(&declSkinType);
	RegisterDeclType(&declSoundType);
	RegisterDeclType(&declEntityDefType);
	RegisterDeclType(&declEffectType);
	RegisterDeclType(&declAFType);
	RegisterDeclType(&declAtmosphereType);
	RegisterDeclType(&declAmbientCubeMapType);
	RegisterDeclType(&declStuffTypeType);
	RegisterDeclType(&declSurfaceTypeType);
	RegisterDeclType(&declSurfaceTypeMapType);
	RegisterDeclType(&declRenderProgramType);
	RegisterDeclType(&declRenderBindingType);
	RegisterDeclType(&declTemplateType);
	RegisterDeclType(&declImposterType);
	RegisterDeclType(&declImposterGeneratorType);
	RegisterDeclType(&declLocStrType);
	RegisterDeclType(&declDecalType);
	// DECL_MODELEXPORT
	// DECL_FONT
	// DECL_MODELDEF
	RegisterDeclType(&declFxType);
	RegisterDeclType(&declParticleType);
	RegisterDeclType(&declPDAType);
	RegisterDeclType(&declVideoType);
	RegisterDeclType(&declAudioType);
	RegisterDeclType(&declEmailType);
	RegisterDeclType(&declMapDefType);
#else
	// decls used throughout the engine
	RegisterDeclType("table",				DECL_TABLE,			idDeclAllocator<idDeclTable>);
	RegisterDeclType("material",			DECL_MATERIAL,		idDeclAllocator<idMaterial>);
	RegisterDeclType("skin",				DECL_SKIN,			idDeclAllocator<idDeclSkin>);
	RegisterDeclType("sound",				DECL_SOUND,			idDeclAllocator<idSoundShader>);

	RegisterDeclType("entityDef",			DECL_ENTITYDEF,		idDeclAllocator<idDeclEntityDef>);
	RegisterDeclType("mapDef",				DECL_MAPDEF,		idDeclAllocator<idDeclEntityDef>);
	RegisterDeclType("fx",					DECL_FX,			idDeclAllocator<idDeclFX>);
	RegisterDeclType("particle",			DECL_PARTICLE,		idDeclAllocator<idDeclParticle>);
	RegisterDeclType("articulatedFigure",	DECL_AF,			idDeclAllocator<idDeclAF>);
	RegisterDeclType("pda",				DECL_PDA,			idDeclAllocator<idDeclPDA>);
	RegisterDeclType("email",				DECL_EMAIL,			idDeclAllocator<idDeclEmail>);
	RegisterDeclType("video",				DECL_VIDEO,			idDeclAllocator<idDeclVideo>);
	RegisterDeclType("audio",				DECL_AUDIO,			idDeclAllocator<idDeclAudio>);
#endif

#ifdef _RAVEN // quake4 new decl
// jmarshall: Raven Decl Support
    RegisterDeclType(  "materialType",		DECL_MATERIALTYPE,  idDeclAllocator<rvDeclMatType>);
    RegisterDeclType(  "lipSync",			DECL_LIPSYNC,		idDeclAllocator<rvDeclLipSync>);
    RegisterDeclType(  "playback",			DECL_PLAYBACK,		idDeclAllocator<rvDeclPlayback>);
    RegisterDeclType(	"effect",			DECL_EFFECT,		idDeclAllocator<rvDeclEffect>);
    RegisterDeclType(	"playerModel",		DECL_PLAYER_MODEL, idDeclAllocator<rvDeclPlayerModel>);
// jmarshall end
#endif

#ifdef _HUMANHEAD
    RegisterDeclType(	"beam",			DECL_BEAM,		idDeclAllocator<hhDeclBeam>);
#endif

#ifdef _SPLASHDAMAGE //karin: register template first
	RegisterDeclFolder("templates",		".template",				DECL_TEMPLATE);
    RegisterDeclFolder("renderprogs",			".rprog",				DECL_RENDERPROGRAM);
    RegisterDeclFolder("renderprogs",			".rprog",				DECL_RENDERBINDING);
#endif
	RegisterDeclFolder("materials",		".mtr",				DECL_MATERIAL);
	RegisterDeclFolder("skins",			".skin",			DECL_SKIN);
#ifdef _SPLASHDAMAGE //karin: sound<s>/*.sndshd
	RegisterDeclFolder("sounds",			".sndshd",			DECL_SOUND);
#else
	RegisterDeclFolder("sound",			".sndshd",			DECL_SOUND);
#endif

#ifdef _RAVEN // quake4 new decl
// jmarshall: Raven Decl Support
    RegisterDeclFolder("materials/types",	".mtt",				DECL_MATERIALTYPE);
    RegisterDeclFolder("lipsync",			".lipsync",			DECL_LIPSYNC);
    RegisterDeclFolder("playbacks",			".playback",		DECL_PLAYBACK);
    RegisterDeclFolder("effects",			".fx",				DECL_EFFECT);
// jmarshall end
#endif

#ifdef _HUMANHEAD
    RegisterDeclFolder("beams",			".beam",				DECL_BEAM);
#endif

#ifdef _SPLASHDAMAGE
    RegisterDeclFolder("localization",			".locstr",				DECL_LOCSTR);
    RegisterDeclFolder("effects",			".effect",				DECL_EFFECT);
    RegisterDeclFolder("atmosphere",			".atm",				DECL_ATMOSPHERE);
    RegisterDeclFolder("ambientCubemap",			".atm",				DECL_AMBIENTCUBEMAP);
    RegisterDeclFolder("decals",			".decal",				DECL_DECAL);
    RegisterDeclFolder("surfacetypes",			".stp",				DECL_SURFACETYPE);
    RegisterDeclFolder("surfacetypes",			".stmap",				DECL_SURFACETYPEMAP);
    RegisterDeclFolder("imposters",			".imp",				DECL_IMPOSTER);
    RegisterDeclFolder("imposters",			".imp",				DECL_IMPOSTERGENERATOR);
    RegisterDeclFolder("stuff",			".stuff",				DECL_STUFFTYPE);
#endif

	// add console commands
	cmdSystem->AddCommand("listDecls", ListDecls_f, CMD_FL_SYSTEM, "lists all decls");

	cmdSystem->AddCommand("reloadDecls", ReloadDecls_f, CMD_FL_SYSTEM, "reloads decls");
	cmdSystem->AddCommand("touch", TouchDecl_f, CMD_FL_SYSTEM, "touches a decl");

	cmdSystem->AddCommand("listTables", idListDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "lists tables", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listMaterials", idListDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "lists materials", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listSkins", idListDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "lists skins", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listSoundShaders", idListDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "lists sound shaders", idCmdSystem::ArgCompletion_String<listDeclStrings>);

	cmdSystem->AddCommand("listEntityDefs", idListDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "lists entity defs", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listFX", idListDecls_f<DECL_FX>, CMD_FL_SYSTEM, "lists FX systems", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listParticles", idListDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "lists particle systems", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listAF", idListDecls_f<DECL_AF>, CMD_FL_SYSTEM, "lists articulated figures", idCmdSystem::ArgCompletion_String<listDeclStrings>);

	cmdSystem->AddCommand("listPDAs", idListDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "lists PDAs", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listEmails", idListDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "lists Emails", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listVideos", idListDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "lists Videos", idCmdSystem::ArgCompletion_String<listDeclStrings>);
	cmdSystem->AddCommand("listAudios", idListDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "lists Audios", idCmdSystem::ArgCompletion_String<listDeclStrings>);

	cmdSystem->AddCommand("printTable", idPrintDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "prints a table", idCmdSystem::ArgCompletion_Decl<DECL_TABLE>);
	cmdSystem->AddCommand("printMaterial", idPrintDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "prints a material", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL>);
	cmdSystem->AddCommand("printSkin", idPrintDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "prints a skin", idCmdSystem::ArgCompletion_Decl<DECL_SKIN>);
	cmdSystem->AddCommand("printSoundShader", idPrintDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "prints a sound shader", idCmdSystem::ArgCompletion_Decl<DECL_SOUND>);

	cmdSystem->AddCommand("printEntityDef", idPrintDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "prints an entity def", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
	cmdSystem->AddCommand("printFX", idPrintDecls_f<DECL_FX>, CMD_FL_SYSTEM, "prints an FX system", idCmdSystem::ArgCompletion_Decl<DECL_FX>);
	cmdSystem->AddCommand("printParticle", idPrintDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "prints a particle system", idCmdSystem::ArgCompletion_Decl<DECL_PARTICLE>);
	cmdSystem->AddCommand("printAF", idPrintDecls_f<DECL_AF>, CMD_FL_SYSTEM, "prints an articulated figure", idCmdSystem::ArgCompletion_Decl<DECL_AF>);

	cmdSystem->AddCommand("printPDA", idPrintDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "prints an PDA", idCmdSystem::ArgCompletion_Decl<DECL_PDA>);
	cmdSystem->AddCommand("printEmail", idPrintDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "prints an Email", idCmdSystem::ArgCompletion_Decl<DECL_EMAIL>);
	cmdSystem->AddCommand("printVideo", idPrintDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "prints a Audio", idCmdSystem::ArgCompletion_Decl<DECL_VIDEO>);
	cmdSystem->AddCommand("printAudio", idPrintDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "prints an Video", idCmdSystem::ArgCompletion_Decl<DECL_AUDIO>);

	cmdSystem->AddCommand("listHuffmanFrequencies", ListHuffmanFrequencies_f, CMD_FL_SYSTEM, "lists decl text character frequencies");

	cmdSystem->AddCommand("parseAllDecls", ParseAllDecls_f, CMD_FL_SYSTEM, "parse all entries of a decl");
	
#ifdef _SPLASHDAMAGE
	cmdSystem->AddCommand("declbToText", DeclbToText_f, CMD_FL_SYSTEM, "convert declb to text files");
	cmdSystem->AddCommand("exportDeclSource", ExportDeclSource_f, CMD_FL_SYSTEM, "export decl source text files");
	cmdSystem->AddCommand("exportDeclExpandSource", ExportDeclExpandSource_f, CMD_FL_SYSTEM, "export decl expand source text files");
	cmdSystem->AddCommand("exportRenderPrograms", sdDeclRenderProgram::ExportDeclRenderPrograms_f, CMD_FL_SYSTEM, "export all render programs");
#endif

	common->Printf("------------------------------\n");
}

/*
===================
idDeclManagerLocal::Shutdown
===================
*/
void idDeclManagerLocal::Shutdown(void)
{
	int			i, j;
	idDeclLocal *decl;

	// free decls
	for (i = 0; i < DECL_MAX_TYPES; i++) {
		for (j = 0; j < linearLists[i].Num(); j++) {
			decl = linearLists[i][j];

			if (decl->self != NULL) {
				decl->self->FreeData();
				delete decl->self;
			}

			if (decl->textSource) {
				Mem_Free(decl->textSource);
				decl->textSource = NULL;
			}

			delete decl;
		}

		linearLists[i].Clear();
		hashTables[i].Free();
	}

	// free decl files
	loadedFiles.DeleteContents(true);

	// free the decl types and folders
	declTypes.DeleteContents(true);
	declFolders.DeleteContents(true);

#ifdef USE_COMPRESSED_DECLS
	ShutdownHuffman();
#endif

    // clean generated folder
    Sys_Printf("Clean generated directory\n");
    fileSystem->RemoveDir(DECL_PROGRAM_GENERATED_DIRECTORY);
}

/*
===================
idDeclManagerLocal::Reload
===================
*/
void idDeclManagerLocal::Reload(bool force)
{
	for (int i = 0; i < loadedFiles.Num(); i++) {
		loadedFiles[i]->Reload(force);
	}
}

/*
===================
idDeclManagerLocal::BeginLevelLoad
===================
*/
void idDeclManagerLocal::BeginLevelLoad()
{
	insideLevelLoad = true;

	// clear all the referencedThisLevel flags and purge all the data
	// so the next reference will cause a reparse
	for (int i = 0; i < DECL_MAX_TYPES; i++) {
		int	num = linearLists[i].Num();

		for (int j = 0 ; j < num ; j++) {
			idDeclLocal *decl = linearLists[i][j];
			decl->Purge();
		}
	}
}

/*
===================
idDeclManagerLocal::EndLevelLoad
===================
*/
void idDeclManagerLocal::EndLevelLoad()
{
	insideLevelLoad = false;

	// we don't need to do anything here, but the image manager, model manager,
	// and sound sample manager will need to free media that was not referenced
}

/*
===================
idDeclManagerLocal::RegisterDeclType
===================
*/
void idDeclManagerLocal::RegisterDeclType(const char *typeName, declType_t type, idDecl *(*allocator)(void))
{
	idDeclType *declType;

	if (type < declTypes.Num() && declTypes[(int)type]) {
		common->Warning("idDeclManager::RegisterDeclType: type '%s' already exists %s", typeName, declTypes[type]->typeName.c_str());
		return;
	}

	declType = new idDeclType;
	declType->typeName = typeName;
	declType->type = type;
	declType->allocator = allocator;
#ifdef _SPLASHDAMAGE //karin: callback when decl type registered
	declType->OnRegister(type);
#endif

	if ((int)type + 1 > declTypes.Num()) {
		declTypes.AssureSize((int)type + 1, NULL);
	}

	declTypes[type] = declType;
}

/*
===================
idDeclManagerLocal::RegisterDeclFolder
===================
*/
void idDeclManagerLocal::RegisterDeclFolder(const char *folder, const char *extension, declType_t defaultType)
{
#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: find all subfolder files
	RegisterDeclFolderWrapper(folder, extension, defaultType, false, false);
	//Sys_Printf("RegisterDeclFolder(%s, %s, %d)\n", folder, extension, defaultType);
#else
	int i, j;
	idStr fileName;
	idDeclFolder *declFolder;
	idFileList *fileList;
	idDeclFile *df;

	// check whether this folder / extension combination already exists
	for (i = 0; i < declFolders.Num(); i++) {
		if (declFolders[i]->folder.Icmp(folder) == 0 && declFolders[i]->extension.Icmp(extension) == 0) {
			break;
		}
	}

	if (i < declFolders.Num()) {
		declFolder = declFolders[i];
	} else {
		declFolder = new idDeclFolder;
		declFolder->folder = folder;
		declFolder->extension = extension;
		declFolder->defaultType = defaultType;
		declFolders.Append(declFolder);
	}

	// scan for decl files
	fileList = fileSystem->ListFiles(declFolder->folder, declFolder->extension, true);

	// load and parse decl files
	for (i = 0; i < fileList->GetNumFiles(); i++)
	{
		fileName = declFolder->folder + "/" + fileList->GetFile(i);

		// check whether this file has already been loaded
		for (j = 0; j < loadedFiles.Num(); j++) {
			if (fileName.Icmp(loadedFiles[j]->fileName) == 0) {
				break;
			}
		}

		if (j < loadedFiles.Num()) {
			df = loadedFiles[j];
		} else {
			df = new idDeclFile(fileName, defaultType);
			loadedFiles.Append(df);
		}

		df->LoadAndParse();
	}

	fileSystem->FreeFileList(fileList);
#endif
}

/*
===================
idDeclManagerLocal::GetChecksum
===================
*/
int idDeclManagerLocal::GetChecksum(void) const
{
	int i, j, total, num;
	int *checksumData;

	// get the total number of decls
	total = 0;

	for (i = 0; i < DECL_MAX_TYPES; i++) {
		total += linearLists[i].Num();
	}

	checksumData = (int *) _alloca16(total * 2 * sizeof(int));

	total = 0;

	for (i = 0; i < DECL_MAX_TYPES; i++) {
		declType_t type = (declType_t) i;

		// FIXME: not particularly pretty but PDAs and associated decls are localized and should not be checksummed
		if (type == DECL_PDA || type == DECL_VIDEO || type == DECL_AUDIO || type == DECL_EMAIL) {
			continue;
		}

		num = linearLists[i].Num();

		for (j = 0; j < num; j++) {
			idDeclLocal *decl = linearLists[i][j];

			if (decl->sourceFile == &implicitDecls) {
				continue;
			}

			checksumData[total*2+0] = total;
			checksumData[total*2+1] = decl->checksum;
			total++;
		}
	}

	LittleRevBytes(checksumData, sizeof(int), total * 2);
	return MD5_BlockChecksum(checksumData, total * 2 * sizeof(int));
}

/*
===================
idDeclManagerLocal::GetNumDeclTypes
===================
*/
int idDeclManagerLocal::GetNumDeclTypes(void) const
{
	return declTypes.Num();
}

/*
===================
idDeclManagerLocal::GetDeclNameFromType
===================
*/
const char *idDeclManagerLocal::GetDeclNameFromType(declType_t type) const
{
	int typeIndex = (int)type;

	if (typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL) {
		common->FatalError("idDeclManager::GetDeclNameFromType: bad type: %i", typeIndex);
	}

	return declTypes[typeIndex]->typeName;
}

/*
===================
idDeclManagerLocal::GetDeclTypeFromName
===================
*/
declType_t idDeclManagerLocal::GetDeclTypeFromName(const char *typeName) const
{
	int i;

	for (i = 0; i < declTypes.Num(); i++) {
		if (declTypes[i] && declTypes[i]->typeName.Icmp(typeName) == 0) {
			return (declType_t)declTypes[i]->type;
		}
	}

	return DECL_MAX_TYPES;
}

/*
=================
idDeclManagerLocal::FindType

External users will always cause the decl to be parsed before returning
=================
*/
const idDecl *idDeclManagerLocal::FindType(declType_t type, const char *name, bool makeDefault)
{
	idDeclLocal *decl;

	if (!name || !name[0]) {
		name = "_emptyName";
		//common->Warning( "idDeclManager::FindType: empty %s name", GetDeclType( (int)type )->typeName.c_str() );
	}

	decl = FindTypeWithoutParsing(type, name, makeDefault);

	if (!decl) {
		return NULL;
	}

	decl->AllocateSelf();

	// if it hasn't been parsed yet, parse it now
	if (decl->declState == DS_UNPARSED) {
		decl->ParseLocal();
	}

	// mark it as referenced
	decl->referencedThisLevel = true;
	decl->everReferenced = true;

	if (insideLevelLoad) {
		decl->parsedOutsideLevelLoad = false;
	}

	return decl->self;
}

/*
===============
idDeclManagerLocal::FindDeclWithoutParsing
===============
*/
const idDecl *idDeclManagerLocal::FindDeclWithoutParsing(declType_t type, const char *name, bool makeDefault)
{
	idDeclLocal *decl;
	decl = FindTypeWithoutParsing(type, name, makeDefault);

	if (decl) {
		return decl->self;
	}

	return NULL;
}

/*
===============
idDeclManagerLocal::ReloadFile
===============
*/
void idDeclManagerLocal::ReloadFile(const char *filename, bool force)
{
	for (int i = 0; i < loadedFiles.Num(); i++) {
		if (!loadedFiles[i]->fileName.Icmp(filename)) {
			checksum ^= loadedFiles[i]->checksum;
			loadedFiles[i]->Reload(force);
			checksum ^= loadedFiles[i]->checksum;
		}
	}
}

/*
===================
idDeclManagerLocal::GetNumDecls
===================
*/
int idDeclManagerLocal::GetNumDecls(declType_t type)
{
	int typeIndex = (int)type;

	if (typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL) {
		common->FatalError("idDeclManager::GetNumDecls: bad type: %i", typeIndex);
	}

	return linearLists[ typeIndex ].Num();
}

/*
===================
idDeclManagerLocal::DeclByIndex
===================
*/
const idDecl *idDeclManagerLocal::DeclByIndex(declType_t type, int index, bool forceParse)
{
	int typeIndex = (int)type;

	if (typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL) {
		common->FatalError("idDeclManager::DeclByIndex: bad type: %i", typeIndex);
	}

#ifdef _SPLASHDAMAGE //karin: not found, return NULL if index == -1, don't throw exception
	if(index == -1)
		return NULL;
#endif
	if (index < 0 || index >= linearLists[ typeIndex ].Num()) {
		common->Error("idDeclManager::DeclByIndex: out of range");
	}

	idDeclLocal *decl = linearLists[ typeIndex ][ index ];

	decl->AllocateSelf();

	if (forceParse && decl->declState == DS_UNPARSED) {
		decl->ParseLocal();
	}

	return decl->self;
}

/*
===================
idDeclManagerLocal::ListType

list*
Lists decls currently referenced

list* ever
Lists decls that have been referenced at least once since app launched

list* all
Lists every decl declared, even if it hasn't been referenced or parsed

FIXME: alphabetized, wildcards?
===================
*/
void idDeclManagerLocal::ListType(const idCmdArgs &args, declType_t type)
{
	bool all, ever;

	if (!idStr::Icmp(args.Argv(1), "all")) {
		all = true;
	} else {
		all = false;
	}

	if (!idStr::Icmp(args.Argv(1), "ever")) {
		ever = true;
	} else {
		ever = false;
	}

	common->Printf("--------------------\n");
	int printed = 0;
	int	count = linearLists[(int)type ].Num();

	for (int i = 0 ; i < count ; i++) {
		idDeclLocal *decl = linearLists[(int)type ][ i ];

		if (!all && decl->declState == DS_UNPARSED) {
			continue;
		}

		if (!all && !ever && !decl->referencedThisLevel) {
			continue;
		}

		if (decl->referencedThisLevel) {
			common->Printf("*");
		} else if (decl->everReferenced) {
			common->Printf(".");
		} else {
			common->Printf(" ");
		}

		if (decl->declState == DS_DEFAULTED) {
			common->Printf("D");
		} else {
			common->Printf(" ");
		}

		common->Printf("%4i: ", decl->index);
		printed++;

		if (decl->declState == DS_UNPARSED) {
			// doesn't have any type specific data yet
			common->Printf("%s\n", decl->GetName());
		} else {
			decl->self->List();
		}
	}

	common->Printf("--------------------\n");
	common->Printf("%i of %i %s\n", printed, count, declTypes[type]->typeName.c_str());
}

/*
===================
idDeclManagerLocal::PrintType
===================
*/
void idDeclManagerLocal::PrintType(const idCmdArgs &args, declType_t type)
{
	// individual decl types may use additional command parameters
	if (args.Argc() < 2) {
		common->Printf("USAGE: Print<decl type> <decl name> [type specific parms]\n");
		return;
	}

	// look it up, skipping the public path so it won't parse or reference
	idDeclLocal *decl = FindTypeWithoutParsing(type, args.Argv(1), false);

	if (!decl) {
		common->Printf("%s '%s' not found.\n", declTypes[ type ]->typeName.c_str(), args.Argv(1));
		return;
	}

	// print information common to all decls
	common->Printf("%s %s:\n", declTypes[ type ]->typeName.c_str(), decl->name.c_str());
	common->Printf("source: %s:%i\n", decl->sourceFile->fileName.c_str(), decl->sourceLine);
	common->Printf("----------\n");

	if (decl->textSource != NULL) {
		char *declText = (char *)_alloca(decl->textLength + 1);
		decl->GetText(declText);
		common->Printf("%s\n", declText);
	} else {
		common->Printf("NO SOURCE\n");
	}

	common->Printf("----------\n");

	switch (decl->declState) {
		case DS_UNPARSED:
			common->Printf("Unparsed.\n");
			break;
		case DS_DEFAULTED:
			common->Printf("<DEFAULTED>\n");
			break;
		case DS_PARSED:
			common->Printf("Parsed.\n");
			break;
	}

	if (decl->referencedThisLevel) {
		common->Printf("Currently referenced this level.\n");
	} else if (decl->everReferenced) {
		common->Printf("Referenced in a previous level.\n");
	} else {
		common->Printf("Never referenced.\n");
	}

	// allow type-specific data to be printed
	if (decl->self != NULL) {
		decl->self->Print();
	}
}

/*
===================
idDeclManagerLocal::CreateNewDecl
===================
*/
idDecl *idDeclManagerLocal::CreateNewDecl(declType_t type, const char *name, const char *_fileName)
{
	int typeIndex = (int)type;
	int i, hash;

	if (typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL) {
		common->FatalError("idDeclManager::CreateNewDecl: bad type: %i", typeIndex);
	}

	char  canonicalName[MAX_STRING_CHARS];

	MakeNameCanonical(name, canonicalName, sizeof(canonicalName));

	idStr fileName = _fileName;
	fileName.BackSlashesToSlashes();

	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey(canonicalName, false);

	for (i = hashTables[typeIndex].First(hash); i >= 0; i = hashTables[typeIndex].Next(i)) {
		if (linearLists[typeIndex][i]->name.Icmp(canonicalName) == 0) {
			linearLists[typeIndex][i]->AllocateSelf();
			return linearLists[typeIndex][i]->self;
		}
	}

	idDeclFile *sourceFile;

	// find existing source file or create a new one
	for (i = 0; i < loadedFiles.Num(); i++) {
		if (loadedFiles[i]->fileName.Icmp(fileName) == 0) {
			break;
		}
	}

	if (i < loadedFiles.Num()) {
		sourceFile = loadedFiles[i];
	} else {
		sourceFile = new idDeclFile(fileName, type);
		loadedFiles.Append(sourceFile);
	}

	idDeclLocal *decl = new idDeclLocal;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->AllocateSelf();
	idStr header = declTypes[typeIndex]->typeName;
	idStr defaultText = decl->self->DefaultDefinition();


	int size = header.Length() + 1 + idStr::Length(canonicalName) + 1 + defaultText.Length();
	char *declText = (char *) _alloca(size + 1);

	memcpy(declText, header, header.Length());
	declText[header.Length()] = ' ';
	memcpy(declText + header.Length() + 1, canonicalName, idStr::Length(canonicalName));
	declText[header.Length() + 1 + idStr::Length(canonicalName)] = ' ';
	memcpy(declText + header.Length() + 1 + idStr::Length(canonicalName) + 1, defaultText, defaultText.Length() + 1);

	decl->SetTextLocal(declText, size);
	decl->sourceFile = sourceFile;
	decl->sourceTextOffset = sourceFile->fileSize;
	decl->sourceTextLength = 0;
	decl->sourceLine = sourceFile->numLines;

	decl->ParseLocal();

	// add this decl to the source file list
	decl->nextInFile = sourceFile->decls;
	sourceFile->decls = decl;

	// add it to the hash table and linear list
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add(hash, linearLists[typeIndex].Append(decl));

	return decl->self;
}

/*
===============
idDeclManagerLocal::RenameDecl
===============
*/
bool idDeclManagerLocal::RenameDecl(declType_t type, const char *oldName, const char *newName)
{

	char canonicalOldName[MAX_STRING_CHARS];
	MakeNameCanonical(oldName, canonicalOldName, sizeof(canonicalOldName));

	char canonicalNewName[MAX_STRING_CHARS];
	MakeNameCanonical(newName, canonicalNewName, sizeof(canonicalNewName));

	idDeclLocal	*decl = NULL;

	// make sure it already exists
	int typeIndex = (int)type;
	int i, hash;
	hash = hashTables[typeIndex].GenerateKey(canonicalOldName, false);

	for (i = hashTables[typeIndex].First(hash); i >= 0; i = hashTables[typeIndex].Next(i)) {
		if (linearLists[typeIndex][i]->name.Icmp(canonicalOldName) == 0) {
			decl = linearLists[typeIndex][i];
			break;
		}
	}

	if (!decl)
		return false;

	//if ( !hashTables[(int)type].Get( canonicalOldName, &declPtr ) )
	//	return false;

	//decl = *declPtr;

	//Change the name
	decl->name = canonicalNewName;


	// add it to the hash table
	//hashTables[(int)decl->type].Set( decl->name, decl );
	int newhash = hashTables[typeIndex].GenerateKey(canonicalNewName, false);
	hashTables[typeIndex].Add(newhash, decl->index);

	//Remove the old hash item
	hashTables[typeIndex].Remove(hash, decl->index);

	return true;
}

/*
===================
idDeclManagerLocal::MediaPrint

This is just used to nicely indent media caching prints
===================
*/
void idDeclManagerLocal::MediaPrint(const char *fmt, ...)
{
	if (!decl_show.GetInteger()) {
		return;
	}

	for (int i = 0 ; i < indent ; i++) {
		common->Printf("    ");
	}

	va_list		argptr;
	char		buffer[1024];
	va_start(argptr,fmt);
	idStr::vsnPrintf(buffer, sizeof(buffer), fmt, argptr);
	va_end(argptr);
	buffer[sizeof(buffer)-1] = '\0';

	common->Printf("%s", buffer);
}

/*
===================
idDeclManagerLocal::WritePrecacheCommands
===================
*/
void idDeclManagerLocal::WritePrecacheCommands(idFile *f)
{
	for (int i = 0; i < declTypes.Num(); i++) {
		int num;

		if (declTypes[i] == NULL) {
			continue;
		}

		num = linearLists[i].Num();

		for (int j = 0 ; j < num ; j++) {
			idDeclLocal *decl = linearLists[i][j];

			if (!decl->referencedThisLevel) {
				continue;
			}

			char	str[1024];
			sprintf(str, "touch %s %s\n", declTypes[i]->typeName.c_str(), decl->GetName());
			common->Printf("%s", str);
			f->Printf("%s", str);
		}
	}
}

/********************************************************************/

const idMaterial *idDeclManagerLocal::FindMaterial(const char *name, bool makeDefault)
{
	return static_cast<const idMaterial *>(FindType(DECL_MATERIAL, name, makeDefault));
}

const idMaterial *idDeclManagerLocal::MaterialByIndex(int index, bool forceParse)
{
	return static_cast<const idMaterial *>(DeclByIndex(DECL_MATERIAL, index, forceParse));
}

/********************************************************************/

const idDeclSkin *idDeclManagerLocal::FindSkin(const char *name, bool makeDefault)
{
	return static_cast<const idDeclSkin *>(FindType(DECL_SKIN, name, makeDefault));
}

const idDeclSkin *idDeclManagerLocal::SkinByIndex(int index, bool forceParse)
{
	return static_cast<const idDeclSkin *>(DeclByIndex(DECL_SKIN, index, forceParse));
}

/********************************************************************/

const idSoundShader *idDeclManagerLocal::FindSound(const char *name, bool makeDefault)
{
	return static_cast<const idSoundShader *>(FindType(DECL_SOUND, name, makeDefault));
}

const idSoundShader *idDeclManagerLocal::SoundByIndex(int index, bool forceParse)
{
	return static_cast<const idSoundShader *>(DeclByIndex(DECL_SOUND, index, forceParse));
}

/*
===================
idDeclManagerLocal::MakeNameCanonical
===================
*/
void idDeclManagerLocal::MakeNameCanonical(const char *name, char *result, int maxLength)
{
	int i, lastDot;

	lastDot = -1;

	for (i = 0; i < maxLength && name[i] != '\0'; i++) {
		int c = name[i];

		if (c == '\\') {
			result[i] = '/';
		} else if (c == '.') {
			lastDot = i;
			result[i] = c;
		} else {
			result[i] = idStr::ToLower(c);
		}
	}

	if (lastDot != -1) {
		result[lastDot] = '\0';
	} else {
		result[i] = '\0';
	}
}

/*
================
idDeclManagerLocal::ListDecls_f
================
*/
void idDeclManagerLocal::ListDecls_f(const idCmdArgs &args)
{
	int		i, j;
	int		totalDecls = 0;
	int		totalText = 0;
	int		totalStructs = 0;

	for (i = 0; i < declManagerLocal.declTypes.Num(); i++) {
		int size, num;

		if (declManagerLocal.declTypes[i] == NULL) {
			continue;
		}

		num = declManagerLocal.linearLists[i].Num();
		totalDecls += num;

		size = 0;

		for (j = 0; j < num; j++) {
			size += declManagerLocal.linearLists[i][j]->Size();

			if (declManagerLocal.linearLists[i][j]->self != NULL) {
				size += declManagerLocal.linearLists[i][j]->self->Size();
			}
		}

		totalStructs += size;

		common->Printf("%4ik %4i %s\n", size >> 10, num, declManagerLocal.declTypes[i]->typeName.c_str());
	}

	for (i = 0 ; i < declManagerLocal.loadedFiles.Num() ; i++) {
		idDeclFile	*df = declManagerLocal.loadedFiles[i];
		totalText += df->fileSize;
	}

	common->Printf("%i total decls is %i decl files\n", totalDecls, declManagerLocal.loadedFiles.Num());
	common->Printf("%iKB in text, %iKB in structures\n", totalText >> 10, totalStructs >> 10);
}

/*
===================
idDeclManagerLocal::ReloadDecls_f

Reload will not find any new files created in the directories, it
will only reload existing files.

A reload will never cause anything to be purged.
===================
*/
void idDeclManagerLocal::ReloadDecls_f(const idCmdArgs &args)
{
	bool	force;

	if (!idStr::Icmp(args.Argv(1), "all")) {
		force = true;
		common->Printf("reloading all decl files:\n");
	} else {
		force = false;
		common->Printf("reloading changed decl files:\n");
	}

	soundSystem->SetMute(true);

	declManagerLocal.Reload(force);

	soundSystem->SetMute(false);
}

/*
===================
idDeclManagerLocal::TouchDecl_f
===================
*/
void idDeclManagerLocal::TouchDecl_f(const idCmdArgs &args)
{
	int	i;

	if (args.Argc() != 3) {
		common->Printf("usage: touch <type> <name>\n");
		common->Printf("valid types: ");

		for (int i = 0 ; i < declManagerLocal.declTypes.Num() ; i++) {
			if (declManagerLocal.declTypes[i]) {
				common->Printf("%s ", declManagerLocal.declTypes[i]->typeName.c_str());
			}
		}

		common->Printf("\n");
		return;
	}

	for (i = 0; i < declManagerLocal.declTypes.Num(); i++) {
		if (declManagerLocal.declTypes[i] && declManagerLocal.declTypes[i]->typeName.Icmp(args.Argv(1)) == 0) {
			break;
		}
	}

	if (i >= declManagerLocal.declTypes.Num()) {
		common->Printf("unknown decl type '%s'\n", args.Argv(1));
		return;
	}

	const idDecl *decl = declManagerLocal.FindType((declType_t)i, args.Argv(2), false);

	if (!decl) {
		common->Printf("%s '%s' not found\n", declManagerLocal.declTypes[i]->typeName.c_str(), args.Argv(2));
	}
}

/*
===================
idDeclManagerLocal::FindTypeWithoutParsing

This finds or creats the decl, but does not cause a parse.  This is only used internally.
===================
*/
idDeclLocal *idDeclManagerLocal::FindTypeWithoutParsing(declType_t type, const char *name, bool makeDefault)
{
	int typeIndex = (int)type;
	int i, hash;

	if (typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL) {
		common->FatalError("idDeclManager::FindTypeWithoutParsing: bad type: %i", typeIndex);
	}

	char canonicalName[MAX_STRING_CHARS];

	MakeNameCanonical(name, canonicalName, sizeof(canonicalName));

	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey(canonicalName, false);

	for (i = hashTables[typeIndex].First(hash); i >= 0; i = hashTables[typeIndex].Next(i)) {
		if (linearLists[typeIndex][i]->name.Icmp(canonicalName) == 0) {
			// only print these when decl_show is set to 2, because it can be a lot of clutter
			if (decl_show.GetInteger() > 1) {
				MediaPrint("referencing %s %s\n", declTypes[ type ]->typeName.c_str(), name);
			}

			return linearLists[typeIndex][i];
		}
	}

	if (!makeDefault) {
		return NULL;
	}

	idDeclLocal *decl = new idDeclLocal;
	decl->self = NULL;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->textSource = NULL;
	decl->textLength = 0;
	decl->sourceFile = &implicitDecls;
	decl->referencedThisLevel = false;
	decl->everReferenced = false;
	decl->parsedOutsideLevelLoad = !insideLevelLoad;

	// add it to the linear list and hash table
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add(hash, linearLists[typeIndex].Append(decl));

	return decl;
}


/*
====================================================================================

	idDeclLocal

====================================================================================
*/

/*
=================
idDeclLocal::idDeclLocal
=================
*/
idDeclLocal::idDeclLocal(void)
{
	name = "unnamed";
	textSource = NULL;
	textLength = 0;
	compressedLength = 0;
	sourceFile = NULL;
	sourceTextOffset = 0;
	sourceTextLength = 0;
	sourceLine = 0;
	checksum = 0;
	type = DECL_ENTITYDEF;
	index = 0;
	declState = DS_UNPARSED;
	parsedOutsideLevelLoad = false;
	referencedThisLevel = false;
	everReferenced = false;
	redefinedInReload = false;
	nextInFile = NULL;
    self = NULL;
}

/*
=================
idDeclLocal::GetName
=================
*/
const char *idDeclLocal::GetName(void) const
{
	return name.c_str();
}

/*
=================
idDeclLocal::GetType
=================
*/
declType_t idDeclLocal::GetType(void) const
{
	return type;
}

/*
=================
idDeclLocal::GetState
=================
*/
declState_t idDeclLocal::GetState(void) const
{
	return declState;
}

/*
=================
idDeclLocal::IsImplicit
=================
*/
bool idDeclLocal::IsImplicit(void) const
{
	return (sourceFile == declManagerLocal.GetImplicitDeclFile());
}

/*
=================
idDeclLocal::IsValid
=================
*/
bool idDeclLocal::IsValid(void) const
{
	return (declState != DS_UNPARSED);
}

/*
=================
idDeclLocal::Invalidate
=================
*/
void idDeclLocal::Invalidate(void)
{
	declState = DS_UNPARSED;
}

/*
=================
idDeclLocal::EnsureNotPurged
=================
*/
void idDeclLocal::EnsureNotPurged(void)
{
	if (declState == DS_UNPARSED) {
		ParseLocal();
	}
}

/*
=================
idDeclLocal::Index
=================
*/
int idDeclLocal::Index(void) const
{
	return index;
}

/*
=================
idDeclLocal::GetLineNum
=================
*/
int idDeclLocal::GetLineNum(void) const
{
	return sourceLine;
}

/*
=================
idDeclLocal::GetFileName
=================
*/
const char *idDeclLocal::GetFileName(void) const
{
	return (sourceFile) ? sourceFile->fileName.c_str() : "*invalid*";
}

/*
=================
idDeclLocal::Size
=================
*/
size_t idDeclLocal::Size(void) const
{
	return sizeof(idDecl) + name.Allocated();
}

/*
=================
idDeclLocal::GetText
=================
*/
void idDeclLocal::GetText(char *text) const
{
#ifdef USE_COMPRESSED_DECLS
	HuffmanDecompressText(text, textLength, (byte *)textSource, compressedLength);
#else
	memcpy(text, textSource, textLength+1);
#endif
}

/*
=================
idDeclLocal::GetTextLength
=================
*/
int idDeclLocal::GetTextLength(void) const
{
	return textLength;
}

/*
=================
idDeclLocal::SetText
=================
*/
void idDeclLocal::SetText(const char *text)
{
	SetTextLocal(text, idStr::Length(text));
}

/*
=================
idDeclLocal::SetTextLocal
=================
*/
void idDeclLocal::SetTextLocal(const char *text, const int length)
{

	Mem_Free(textSource);

	checksum = MD5_BlockChecksum(text, length);

#ifdef GET_HUFFMAN_FREQUENCIES

	for (int i = 0; i < length; i++) {
		huffmanFrequencies[((const unsigned char *)text)[i]]++;
	}

#endif

#ifdef USE_COMPRESSED_DECLS
	int maxBytesPerCode = (maxHuffmanBits + 7) >> 3;
	byte *compressed = (byte *)_alloca(length * maxBytesPerCode);
	compressedLength = HuffmanCompressText(text, length, compressed, length * maxBytesPerCode);
	textSource = (char *)Mem_Alloc(compressedLength);
	memcpy(textSource, compressed, compressedLength);
#else
	compressedLength = length;
	textSource = (char *) Mem_Alloc(length + 1);
	memcpy(textSource, text, length);
	textSource[length] = '\0';
#endif
	textLength = length;
}

/*
=================
idDeclLocal::ReplaceSourceFileText
=================
*/
bool idDeclLocal::ReplaceSourceFileText(void)
{
	int oldFileLength, newFileLength;
	char *buffer;
	idFile *file;

	common->Printf("Writing \'%s\' to \'%s\'...\n", GetName(), GetFileName());

	if (sourceFile == &declManagerLocal.implicitDecls) {
		common->Warning("Can't save implicit declaration %s.", GetName());
		return false;
	}

	// get length and allocate buffer to hold the file
	oldFileLength = sourceFile->fileSize;
	newFileLength = oldFileLength - sourceTextLength + textLength;
	buffer = (char *) Mem_Alloc(Max(newFileLength, oldFileLength));

	// read original file
	if (sourceFile->fileSize) {

		file = fileSystem->OpenFileRead(GetFileName());

		if (!file) {
			Mem_Free(buffer);
			common->Warning("Couldn't open %s for reading.", GetFileName());
			return false;
		}

		if (file->Length() != sourceFile->fileSize || file->Timestamp() != sourceFile->timestamp) {
			Mem_Free(buffer);
			common->Warning("The file %s has been modified outside of the engine.", GetFileName());
			return false;
		}

		file->Read(buffer, oldFileLength);
		fileSystem->CloseFile(file);

		if (MD5_BlockChecksum(buffer, oldFileLength) != sourceFile->checksum) {
			Mem_Free(buffer);
			common->Warning("The file %s has been modified outside of the engine.", GetFileName());
			return false;
		}
	}

	// insert new text
	char *declText = (char *) _alloca(textLength + 1);
	GetText(declText);
	memmove(buffer + sourceTextOffset + textLength, buffer + sourceTextOffset + sourceTextLength, oldFileLength - sourceTextOffset - sourceTextLength);
	memcpy(buffer + sourceTextOffset, declText, textLength);

	// write out new file
	file = fileSystem->OpenFileWrite(GetFileName(), "fs_devpath");

	if (!file) {
		Mem_Free(buffer);
		common->Warning("Couldn't open %s for writing.", GetFileName());
		return false;
	}

	file->Write(buffer, newFileLength);
	fileSystem->CloseFile(file);

	// set new file size, checksum and timestamp
	sourceFile->fileSize = newFileLength;
	sourceFile->checksum = MD5_BlockChecksum(buffer, newFileLength);
	fileSystem->ReadFile(GetFileName(), NULL, &sourceFile->timestamp);

	// free buffer
	Mem_Free(buffer);

	// move all decls in the same file
	for (idDeclLocal *decl = sourceFile->decls; decl; decl = decl->nextInFile) {
		if (decl->sourceTextOffset > sourceTextOffset) {
			decl->sourceTextOffset += textLength - sourceTextLength;
		}
	}

	// set new size of text in source file
	sourceTextLength = textLength;

	return true;
}

/*
=================
idDeclLocal::SourceFileChanged
=================
*/
bool idDeclLocal::SourceFileChanged(void) const
{
	int newLength;
	ID_TIME_T newTimestamp;

	if (sourceFile->fileSize <= 0) {
		return false;
	}

	newLength = fileSystem->ReadFile(GetFileName(), NULL, &newTimestamp);

	if (newLength != sourceFile->fileSize || newTimestamp != sourceFile->timestamp) {
		return true;
	}

	return false;
}

/*
=================
idDeclLocal::MakeDefault
=================
*/
void idDeclLocal::MakeDefault()
{
	static int recursionLevel;
	const char *defaultText;

	declManagerLocal.MediaPrint("DEFAULTED\n");
	declState = DS_DEFAULTED;

	AllocateSelf();

	defaultText = self->DefaultDefinition();

	// a parse error inside a DefaultDefinition() string could
	// cause an infinite loop, but normal default definitions could
	// still reference other default definitions, so we can't
	// just dump out on the first recursion
	if (++recursionLevel > 100) {
		common->FatalError("idDecl::MakeDefault: bad DefaultDefinition(): %s", defaultText);
	}

	// always free data before parsing
	self->FreeData();

	// parse
	self->Parse(defaultText, strlen(defaultText));

	// we could still eventually hit the recursion if we have enough Error() calls inside Parse...
	--recursionLevel;
}

/*
=================
idDeclLocal::SetDefaultText
=================
*/
bool idDeclLocal::SetDefaultText(void)
{
	return false;
}

/*
=================
idDeclLocal::DefaultDefinition
=================
*/
const char *idDeclLocal::DefaultDefinition() const
{
	return "{ }";
}

/*
=================
idDeclLocal::Parse
=================
*/
#ifdef _RAVEN
bool idDeclLocal::Parse(const char *text, const int textLength, bool noCaching)
#else
bool idDeclLocal::Parse(const char *text, const int textLength)
#endif
{
	idLexer src;

	src.LoadMemory(text, textLength, GetFileName(), GetLineNum());
	src.SetFlags(DECL_LEXER_FLAGS);
	src.SkipUntilString("{");
	src.SkipBracedSection(false);
	return true;
}

/*
=================
idDeclLocal::FreeData
=================
*/
void idDeclLocal::FreeData()
{
}

/*
=================
idDeclLocal::List
=================
*/
void idDeclLocal::List() const
{
	common->Printf("%s\n", GetName());
}

/*
=================
idDeclLocal::Print
=================
*/
void idDeclLocal::Print() const
{
}

/*
=================
idDeclLocal::Reload
=================
*/
void idDeclLocal::Reload(void)
{
	this->sourceFile->Reload(false);
}

/*
=================
idDeclLocal::AllocateSelf
=================
*/
void idDeclLocal::AllocateSelf(void)
{
	if (self == NULL) {
#ifdef _SPLASHDAMAGE //karin: if has allocator of decl info
		const idDeclTypeInterface *dt = declManagerLocal.GetDeclType((int)type);
		if(dt->allocator)
			self = dt->allocator();
		else
			self = const_cast<idDeclTypeInterface *>(dt)->Alloc();
#else
		self = declManagerLocal.GetDeclType((int)type)->allocator();
#endif
		self->base = this;
	}
}

/*
=================
idDeclLocal::ParseLocal
=================
*/
void idDeclLocal::ParseLocal(void)
{
	bool generatedDefaultText = false;

	AllocateSelf();

	// always free data before parsing
	self->FreeData();

	declManagerLocal.MediaPrint("parsing %s %s\n", declManagerLocal.declTypes[type]->typeName.c_str(), name.c_str());

	// if no text source try to generate default text
#ifdef _SPLASHDAMAGE //karin: if text source and binary source are all empty
	if (textSource == NULL && !HasBinaryBuffer()) 
#else
	if (textSource == NULL) 
#endif
	{
		generatedDefaultText = self->SetDefaultText();
	}

	// indent for DEFAULTED or media file references
	declManagerLocal.indent++;

	// no text immediately causes a MakeDefault()
#ifdef _SPLASHDAMAGE //karin: if text source and binary source are all empty
	if (textSource == NULL && !HasBinaryBuffer()) 
#else
	if (textSource == NULL) 
#endif
	{
		MakeDefault();
		declManagerLocal.indent--;
		return;
	}

	declState = DS_PARSED;

	// parse
	char *declText = (char *) _alloca((GetTextLength() + 1) * sizeof(char));
	GetText(declText);
#ifdef _SPLASHDAMAGE //karin: make final decl source text(applied templates)
	idStr finalPreprocessedBuffer;
	//Sys_Printf("rrr|%s|%s|\n\n", GetFileName(), GetName()/*,idStr(declText,0,GetTextLength()).c_str()*/ );
	//karin: 1. expand template if has useTemplate keyword
	// NOTE: template should not expand template when parse, because some template use other template as parameter into this template - karin
	if (!declManagerLocal.GetDeclType(type)->AllowTemplateEvaluation() || !sdDeclTemplate::ExpandTemplate(finalPreprocessedBuffer, declText, GetTextLength()))
		finalPreprocessedBuffer.Append(declText, GetTextLength());
	//karin: include depences
	const idStrList &includeDependencies = GetIncludeDependencies();
	if(includeDependencies.Num() > 0)
	{
		sdStringBuilder_Heap buf;
		for(int i = 0; i < includeDependencies.Num(); i++)
		{
			buf.Append("#include \"");
			buf.Append(includeDependencies[i]);
			buf.Append("\"\n");
		}
		buf.Append(finalPreprocessedBuffer.c_str());
		finalPreprocessedBuffer = buf.c_str();
	}
	//Sys_Printf("OOO|%s|\n----------------\nPPP|%s|\n", idStr(declText,0,GetTextLength()).c_str(), finalPreprocessedBuffer.c_str());
	self->Parse(finalPreprocessedBuffer.c_str(), finalPreprocessedBuffer.Length());

	const idDeclType *dt = (const idDeclType *)declManagerLocal.GetDeclType((int)type);
	if (dt->ref)
		dt->ref->PostParse(self);
#else
	self->Parse(declText, GetTextLength());
#endif

	// free generated text
	if (generatedDefaultText) {
		Mem_Free(textSource);
		textSource = 0;
		textLength = 0;
	}

	declManagerLocal.indent--;
}

/*
=================
idDeclLocal::Purge
=================
*/
void idDeclLocal::Purge(void)
{
	// never purge things that were referenced outside level load,
	// like the console and menu graphics
	if (parsedOutsideLevelLoad) {
		return;
	}

	referencedThisLevel = false;
	MakeDefault();

	// the next Find() for this will re-parse the real data
	declState = DS_UNPARSED;
}

/*
=================
idDeclLocal::EverReferenced
=================
*/
bool idDeclLocal::EverReferenced(void) const
{
	return everReferenced;
}


const idDecl * idDeclManagerLocal::AddDeclDef(const char *defname, declType_t type, const idDict &args, bool force)
{
    const idDecl *decl;
    if(!force)
    {
        decl = FindDeclWithoutParsing(type, defname, false);
        if(decl)
            return decl;
    }

    const idDeclType *typeInfoFound = NULL;

    int numTypes = declManagerLocal.GetNumDeclTypes();
    for (int i = 0; i < numTypes; i++) {
#ifdef _SPLASHDAMAGE
        idDeclType *typeInfo = static_cast<idDeclType *>(declManagerLocal.GetDeclType(i));
#else
        idDeclType *typeInfo = declManagerLocal.GetDeclType(i);
#endif

        if (typeInfo && typeInfo->type == type) {
            typeInfoFound = typeInfo;
            break;
        }
    }
    if(!typeInfoFound)
    {
        common->Warning("Generate def '%s' type '%d' not found", defname, type);
        return NULL;
    }

    const idDeclFolder *declFolderFound = NULL;
    for (int i = 0; i < declFolders.Num(); i++) {
        if (declFolders[i]->defaultType == type) {
            declFolderFound = declFolders[i];
            break;
        }
    }
    if(!declFolderFound)
    {
        common->Warning("Generate def '%s' folder '%d' not found", defname, type);
        return NULL;
    }

    idStr fileName = va(DECL_PROGRAM_GENERATED_DIRECTORY "/%s/%s", declFolderFound->folder.c_str(), defname);
    fileName.SetFileExtension(declFolderFound->extension.c_str());

    idStr text;
    text.Append(typeInfoFound->typeName);
    text.Append(" ");
    text.Append(defname);
    text.Append(" {\n");
    for(int i = 0; i < args.GetNumKeyVals(); i++)
    {
        const idKeyValue *kv = args.GetKeyVal(i);
        text.Append("    ");
        text.Append("\"");
        text.Append(kv->GetKey().c_str());
        text.Append("\" \"");
        text.Append(kv->GetValue().c_str());
        text.Append("\"\n");
    }
    text.Append("}\n");

    if(fileSystem->WriteFile(fileName.c_str(), text.c_str(), text.Length()) <= 0)
    {
        common->Warning("Write generated def file '%s' to '%s' fail", defname, fileName.c_str());
        return NULL;
    }

    decl = CreateNewDecl(type, defname, fileName.c_str());
    if(!decl)
    {
        common->Warning("Generate decl '%s' fail", defname);
        //fileSystem->RemoveFile(fileName);
        return NULL;
    }

    declManager->ReloadFile(fileName, true);
    fileSystem->RemoveFile(fileName);

#if 0
    common->Printf("Generate def '%s' to '%s'\n", defname, fileName.c_str());
    idCmdArgs cmdArgs;
    cmdArgs.TokenizeString(va("printEntityDef %s", defname), false);
    PrintType(cmdArgs, DECL_ENTITYDEF);
#endif

    return decl;
}

bool idDeclManagerLocal::EntityDefSet(const char *name, const char *key, const char *value)
{
	idDeclLocal *decl;

	decl = FindTypeWithoutParsing(DECL_ENTITYDEF, name, false);

	if (!decl) {
		return false;
	}

	if (!decl->self) {
		return false;
	}

	// if it hasn't been parsed yet, parse it now
	if (decl->declState == DS_UNPARSED) {
		return false;
	}

	idDeclEntityDef *entityDef = (idDeclEntityDef *)decl->self;
	if(value)
		entityDef->dict.Set(key, value);
	else
		entityDef->dict.Delete(key);

	return true;
}

void idDeclManagerLocal::ParseAllDecls_f(const idCmdArgs &args)
{
	if(args.Argc() < 2) {
		common->Printf("Usage: %s <type>\n", args.Argv(0));
		common->Printf("valid types: ");

		for (int i = 0 ; i < declManagerLocal.declTypes.Num() ; i++) {
			if (declManagerLocal.declTypes[i]) {
				common->Printf("%s ", declManagerLocal.declTypes[i]->typeName.c_str());
			}
		}

		common->Printf("\n");
		return;
	}

	const char *type = args.Argv(1);
	const idDeclType *declType = NULL;

	for (int i = 0; i < declManagerLocal.declTypes.Num(); i++) {
		if (declManagerLocal.declTypes[i] && declManagerLocal.declTypes[i]->typeName.Icmp(args.Argv(1)) == 0) {
			declType = declManagerLocal.declTypes[i];
			break;
		}
	}

	if (!declType) {
		common->Printf("unknown decl type '%s'\n", type);
		return;
	}

	int numDecls = declManagerLocal.GetNumDecls(declType->type);
	common->Printf("Parse: %s decls %d entries\n", type, numDecls);
	soundSystem->SetMute(true);

	for(int m = 0; m < numDecls; m++)
	{
		const idDecl *decl = declManagerLocal.DeclByIndex(declType->type, m, true);
		common->Printf("%s\n", decl->GetName());
	}

	soundSystem->SetMute(false);
}

#ifdef _RAVEN // quake4 guide
/*
================
idDeclFile::PreprocessGuides
================
*/
//karin: helper struct
struct rvGuidePlaceholder
{
	// [start, end)
	int start; // include
	int end; // exclude
	idStr replaceStr;

	rvGuidePlaceholder(int start = 0, int end = 0, const idStr &str = idStr())
		: start(start),
		end(end),
		replaceStr(str)
	{ }
	void ReplaceSpace(idStr &str)
	{
		for(int m = start; m < end; m++)
		{
			if(!isspace(str[m])) //karin: keep raw format for debug
				str[m] = ' ';
		}
	}
	int Replace(int offset, idStr &toStr)
	{
		int length = end - start;
		int newLength = replaceStr.Length();
		idStr front = toStr.Left(start + offset);
		idStr back = toStr.Right(toStr.Length() - end - offset);
		toStr = front + replaceStr + back;
		return newLength - length;
	}
};
struct rvGuidePlaceholderList : public idList<rvGuidePlaceholder>
{
	void ReplaceSpace(idStr &str)
	{
		for(int i = 0; i < Num(); i++)
			this->operator[](i).ReplaceSpace(str);
	}
	void Replace(idStr &str)
	{
		int offset = 0;
		for(int i = 0; i < Num(); i++)
		{
			offset += this->operator[](i).Replace(offset, str);
		}
	}
};

idStr idDeclFile::PreprocessGuides(const char* text, int textLength)
{
    idLexer src;
    idToken	token, token2;

    idStr finalBuffer = "";

    src.LoadMemory(text, textLength, "", 0);
    src.SetFlags(DECL_LEXER_FLAGS);
	rvGuidePlaceholderList guideRanges; //karin: record a pair of read guide characters offset: start, end, characters in range will be replaced ' '

    while (1)
    {
        if (!src.ReadToken(&token))
        {
            break;
        }

        if (token == "guide")
        {
			int range_start = src.GetFileOffset() - 5; //karin: record range start before next `ReadToken`
            idToken name;
            idStr newDecl;
            rvGuideTemplate*guide = NULL;

            src.ReadToken(&name);
            src.ReadToken(&token);

            for (int i = 0; i < declManagerLocal.guides.Num(); i++)
            {
                if (declManagerLocal.guides[i].name == token)
                {
                    guide = &declManagerLocal.guides[i];
                    break;
                }
            }

            if (guide == NULL)
            {
                common->FatalError("Failed to find guide '%s'\n", token.c_str());
            }

            newDecl = name;
            newDecl += "\n";
            newDecl += guide->body;

            src.ExpectTokenString("(");
            for (int i = 0; i < guide->parms.Num(); i++ )
            {
                src.ReadToken(&token);
                newDecl.Replace(guide->parms[i].c_str(), token);
            }
            src.ExpectTokenString(")");

            newDecl += "\n";

            finalBuffer += newDecl;
			int range_end = src.GetFileOffset(); //karin: record range end after last `ReadToken`
			guideRanges.Append(rvGuidePlaceholder(range_start, range_end));
        }
    }

	//karin: replace all old guide source to space
	idStr oldText(text);
	guideRanges.ReplaceSpace(oldText);
    finalBuffer += oldText;

    //finalBuffer.Replace("inlineGuide", "// inlineGuide"); // todo support me, corpse burn
    //finalBuffer.Replace("guide", "// guide");
    return PreprocessInlineGuides(finalBuffer.c_str(), finalBuffer.Length());
}

idStr idDeclFile::PreprocessInlineGuides(const char* text, int textLength)
{
    idLexer src;
    idToken	token, token2;

    idStr finalBuffer(text);

    src.LoadMemory(text, textLength, "", 0);
    src.SetFlags(DECL_LEXER_FLAGS);
	rvGuidePlaceholderList guideRanges; //karin: record a pair of read guide characters offset: start, end, characters in range will be replaced to new source

    while (1)
    {
        if (!src.ReadToken(&token))
        {
            break;
        }

        if (!idStr::Icmp(token, "inlineGuide"))
        {
			int range_start = src.GetFileOffset() - 11; //karin: record range start before next `ReadToken`
            idStr newDecl;
            rvGuideTemplate*guide = NULL;

            src.ReadToken(&token);

            for (int i = 0; i < declManagerLocal.guides.Num(); i++)
            {
                if (declManagerLocal.guides[i].name == token)
                {
                    guide = &declManagerLocal.guides[i];
                    break;
                }
            }

            if (guide == NULL)
            {
                common->FatalError("Failed to find inlineGuide '%s'\n", token.c_str());
            }

            newDecl = guide->body;

            src.ExpectTokenString("(");
            for (int i = 0; i < guide->parms.Num(); i++ )
            {
                src.ReadToken(&token);
                newDecl.Replace(guide->parms[i].c_str(), token);
            }
            src.ExpectTokenString(")");

			int range_end = src.GetFileOffset(); //karin: record range end after last `ReadToken`
			guideRanges.Append(rvGuidePlaceholder(range_start, range_end, newDecl));
        }
    }

	//karin: replace all old inline guide source to new source
	guideRanges.Replace(finalBuffer);

    return finalBuffer;
}

// RAVEN BEGIN
// jscott: for new Raven decls
const rvDeclMatType* idDeclManagerLocal::FindMaterialType(const char* name, bool makeDefault) {
	return static_cast<const rvDeclMatType*>(FindType(DECL_MATERIALTYPE, name, makeDefault));
}

const rvDeclLipSync* idDeclManagerLocal::FindLipSync(const char* name, bool makeDefault) {
	return static_cast<const rvDeclLipSync*>(FindType(DECL_LIPSYNC, name, makeDefault));
}

const rvDeclPlayback* idDeclManagerLocal::FindPlayback(const char* name, bool makeDefault) {
	return static_cast<const rvDeclPlayback*>(FindType(DECL_PLAYBACK, name, makeDefault));
}
const rvDeclEffect* idDeclManagerLocal::FindEffect(const char* name, bool makeDefault) {
	return static_cast<const rvDeclEffect*>(FindType(DECL_EFFECT, name, makeDefault));
}

const rvDeclMatType* idDeclManagerLocal::MaterialTypeByIndex(int index, bool forceParse) {
	return static_cast<const rvDeclMatType*>(DeclByIndex(DECL_MATERIALTYPE, index, forceParse));
}

const rvDeclLipSync* idDeclManagerLocal::LipSyncByIndex(int index, bool forceParse) {
	return static_cast<const rvDeclLipSync*>(DeclByIndex(DECL_LIPSYNC, index, forceParse));
}

const rvDeclPlayback* idDeclManagerLocal::PlaybackByIndex(int index, bool forceParse) {
	return static_cast<const rvDeclPlayback*>(DeclByIndex(DECL_PLAYBACK, index, forceParse));
}

const rvDeclEffect* idDeclManagerLocal::EffectByIndex(int index, bool forceParse) {
	return static_cast<const rvDeclEffect*>(DeclByIndex(DECL_EFFECT, index, forceParse));
}

const idDeclTable* idDeclManagerLocal::FindTable(const char* name, bool makeDefault) {
	return static_cast<const idDeclTable*>(FindType(DECL_TABLE, name, makeDefault));
}

// jmarshall: Quake 4 Guide Support
/*
=========================
idDeclManagerLocal::ParseGuides
=========================
*/
void idDeclManagerLocal::ParseGuides(void) {
	idFileList* fileList = fileSystem->ListFiles("guides", ".guide");

	common->Printf("Parsing Guides...\n");

	for (int i = 0; i < fileList->GetNumFiles(); i++)
	{
		idLexer src;
		idToken	token;
		idStr fileName = fileList->GetList()[i];

		src.LoadFile(va("guides/%s", fileName.c_str()));
		src.SetFlags(DECL_LEXER_FLAGS);

		while (!src.EndOfFile())
		{
			src.ReadToken(&token);

			if (token.Length() <= 0)
				break;

			if (token == "guide" || token == "inlineGuide")
			{
				rvGuideTemplate guide;

				if (token == "inlineGuide")
				{
					guide.inlineGuide = true;
				}
				else
				{
					guide.inlineGuide = false;
				}

				src.ReadToken(&token);
				guide.name = token;

				src.ExpectTokenString("(");

				while (!src.EndOfFile())
				{
					src.ReadToken(&token);

					if (token == ")")
					{
						break;
					}

					guide.parms.Append(token);
				}

				src.ParseBracedSection(guide.body);
				if(guide.inlineGuide) //karin: inline guide remove start/end braces
				{
					guide.body.StripTrailingWhitespace();
					guide.body.StripLeadingOnce("{");
					guide.body.StripTrailingOnce("}");
				}

				guides.Append(guide);
			}
			else
			{
				src.Error("Unexpected token in guide %s\n", token.c_str());
			}
		}
	}

	common->Printf("Found %d guides...\n", guides.Num());

	fileSystem->FreeFileList(fileList);
}

// jmarshall end

//k: find map def
const idDeclEntityDef * idDeclManagerLocal::GetMapDef(const char *mapName, const char *entityFilter) const
{
	//const char *entityFilter = cvarSystem->GetCVarString("si_entityFilter");
	idStr fullMapName(mapName);
	if(entityFilter && entityFilter[0]) //k: game/map_entityFilter. e.g. game/process1 first
	{
		fullMapName += "_";
		fullMapName += entityFilter;
	}
	// find mapDef
	const idDecl *mapDecl = declManager->FindType(DECL_MAPDEF, fullMapName, false);
	if(!mapDecl) // find by origin way
		mapDecl = declManager->FindType(DECL_MAPDEF, mapName, false);

	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>(mapDecl);
	return mapDef;
}

#endif

#if defined(_RAVEN) || defined(_SPLASHDAMAGE)
/*
===================
RegisterDeclSubFolder
===================
*/
// jmarshall
void idDeclManagerLocal::RegisterDeclSubFolder(const char* folder, const char* extension, idList<idStr>& fileList, bool norecurse)
{
    // Find all
    {
        idFileList* list = fileSystem->ListFiles(folder, extension, true);

        for (int d = 0; d < list->GetNumFiles(); d++)
        {
            fileList.Append(va("%s/%s", folder, list->GetFile(d)));
        }

        fileSystem->FreeFileList(list);
    }

	if(!norecurse)
	{
		idFileList* dirList = fileSystem->ListFiles(folder, "/", true);
		for (int i = 0; i < dirList->GetNumFiles(); i++)
		{
			idStr dir = va("%s/%s", folder, dirList->GetFile(i));
			RegisterDeclSubFolder(dir, extension, fileList);
		}

		fileSystem->FreeFileList(dirList);
	}
}

void idDeclManagerLocal::RegisterDeclFolderWrapper( const char *folder, const char *extension, declType_t defaultType, bool unique, bool norecurse )
{
	int i, j;
	idDeclFolder *declFolder;
    idList<idStr> fileList;
	idDeclFile *df;

	// check whether this folder / extension combination already exists
	for (i = 0; i < declFolders.Num(); i++) {
		if (declFolders[i]->folder.Icmp(folder) == 0 && declFolders[i]->extension.Icmp(extension) == 0) {
			break;
		}
	}

	if (i < declFolders.Num()) {
		declFolder = declFolders[i];
	} else {
		declFolder = new idDeclFolder;
		declFolder->folder = folder;
		declFolder->extension = extension;
		declFolder->defaultType = defaultType;
		declFolders.Append(declFolder);
	}

	// scan for decl files
// jmarshall - decls subfolders
    RegisterDeclSubFolder(declFolder->folder, declFolder->extension, fileList, norecurse);
// jmarshall end

	// load and parse decl files
    for ( i = 0; i < fileList.Num(); i++ )
	{
        const idStr &fileName = fileList[i];

		// check whether this file has already been loaded
		for (j = 0; j < loadedFiles.Num(); j++) {
			if (fileName.Icmp(loadedFiles[j]->fileName) == 0) {
				break;
			}
		}

		if (j < loadedFiles.Num()) {
			df = loadedFiles[j];
		} else {
			df = new idDeclFile(fileName, defaultType);
			loadedFiles.Append(df);
		}

		df->LoadAndParse();
	}
	
#ifdef _SPLASHDAMAGE //karin: parse binary declb files finally
	RegisterDeclFolderWrapperBinary(declFolder, unique, norecurse);
#endif
}

// jmarshall end
#endif

#ifdef _HUMANHEAD
const hhDeclBeam *		idDeclManagerLocal::FindBeam( const char *name, bool makeDefault )
{
	return static_cast<const hhDeclBeam*>(FindType(DECL_BEAM, name, makeDefault));
}

const hhDeclBeam *		idDeclManagerLocal::BeamByIndex( int index, bool forceParse )
{
	return static_cast<const hhDeclBeam*>(DeclByIndex(DECL_BEAM, index, forceParse));
}
#endif

#ifdef _SPLASHDAMAGE //karin: parse binary declb file and binary global token cache file
void idDeclLocal::SetBinarySource( const byte* source, int length ) {
	if(length > 0)
	{
		binarySource.SetNum(length);
		memcpy(binarySource.Ptr(), source, length);
	}
	else
		binarySource.Clear();
}

void idDeclLocal::GetBinarySource( byte*& source, int& length ) const {
	if(binarySource.Num())
	{
		source = (byte *)Mem_Alloc(binarySource.Num());
		memcpy(source, binarySource.Ptr(), binarySource.Num());
		length = binarySource.Num();
	}
	else
	{
		source = NULL;
		length = 0;
	}
}

void idDeclLocal::FreeSourceBuffer( byte* buffer ) const {
	Mem_Free(buffer);
}

bool idDeclLocal::HasBinaryBuffer() const {
	return binarySource.Num() > 0;
}

const idStrList& idDeclLocal::GetIncludeDependencies() const
{
	return includeDependencies;
}

const idStrList* idDeclLocal::GetFileLevelIncludeDependencies() const {
	return NULL; //&includeDependencies;
}

void idDeclLocal::AddIncludeDependency(const char *file)
{
	includeDependencies.AddUnique(file);
}



void idDeclManagerLocal::RegisterDeclFolderWrapperBinary( idDeclFolder *declFolder, bool unique, bool norecurse )
{
	(void)unique;

	int i, j;
	idStr fileName;
    idList<idStr> fileList;
	idDeclFile *df;

	idStr binExt = declFolder->extension;
	binExt.Append("b");
	idStr binFolder;
	Com_MakeBinaryFilename(binFolder, declFolder->folder, NULL);

	// scan for decl files
    RegisterDeclSubFolder(binFolder.c_str(), binExt.c_str(), fileList, norecurse);
	idStr binDir;
	Com_MakeBinaryFilename(binDir, NULL, NULL);

	// load and parse decl files
    for ( i = 0; i < fileList.Num(); i++ )
	{
        fileName = fileList[i].Right(fileList[i].Length() - binDir.Length() - 1);
		fileName.StripTrailingOnce("b");

		// check whether this file has already been loaded
		for (j = 0; j < loadedFiles.Num(); j++) {
			if (fileName.Icmp(loadedFiles[j]->fileName) == 0) {
				break;
			}
		}

		if (j < loadedFiles.Num()) {
			//df = loadedFiles[j];
			continue;
		} else {
			df = new idDeclFile(fileName, declFolder->defaultType);
			loadedFiles.Append(df);
		}

		df->LoadAndParseBinary();
	}
}

// Returns the system token cache
idTokenCache& idDeclManagerLocal::GetGlobalTokenCache() {
	return globalTokencache;
}

void idDeclManagerLocal::RegisterDeclType( idDeclTypeInterface* type ) {
	qhandle_t h = GetDeclTypeHandle(type->GetName());
	//Sys_Printf("RegisterDeclType(%s, %d, %d)\n", type->GetName(), type->GetHandle(), h);
	type->type = h;
	type->ref = type;
	type->OnRegister(h);
	RegisterDeclType(type->GetName(), h, NULL);

	idDeclTypeInterface *declType = GetDeclType(h);
	declType->ref = type;
	//FinishedRegistering();
}

void idDeclManagerLocal::UnregisterDeclType( idDeclTypeInterface* declType ) {
	int type = declType->type;

	if (type >= declTypes.Num() || !declTypes[(int)type]) {
		common->Warning("idDeclManager::UnregisterDeclType: type '%s' not be registered", declType->typeName.c_str());
		return;
	}

	//Sys_Printf("UnregisterDeclType(%s, %d, %d)\n", declType->GetName(), declType->GetHandle(), declType->type);
	int			i = type, j;
	idDeclLocal *decl;

	// free decls
	for (j = 0; j < linearLists[i].Num(); j++) {
		decl = linearLists[i][j];

		if (decl->self != NULL) {
			decl->self->FreeData();
			delete decl->self;
		}

		if (decl->textSource) {
			Mem_Free(decl->textSource);
			decl->textSource = NULL;
		}

		delete decl;
	}

	linearLists[i].Clear();
	hashTables[i].Free();

	delete declTypes[type];
	declTypes[type] = NULL;
}

void idDeclManagerLocal::RegisterDeclFolder( const char *folder, const char *extension ) {
	// in game/decls/GameDeclIdentifiers.*
	declType_t defaultType;
	if (!idStr::Icmp(extension, ".def"))
		defaultType = DECL_ENTITYDEF;
	else if (!idStr::Icmp(extension, ".af"))
		defaultType = DECL_AF;
	else if (!idStr::Icmp(extension, ".effect"))
		defaultType = DECL_EFFECT;
	else if (!idStr::Icmp(extension, ".decal"))
		defaultType = DECL_DECAL;
	else if (!idStr::Icmp(extension, ".vscript"))
		defaultType = GetDeclTypeFromName("vehicleDef");
	else if (!idStr::Icmp(extension, ".qc"))
		defaultType = GetDeclTypeFromName("quickChatDef");
	else if (!idStr::Icmp(extension, ".txt"))
		defaultType = GetDeclTypeFromName("mapInfoDef");
	else if (!idStr::Icmp(extension, ".md"))
		defaultType = GetDeclTypeFromName("mapInfoDef");
	else if (!idStr::Icmp(extension, ".gui"))
		defaultType = GetDeclTypeFromName("gui");
	else if (!idStr::Icmp(extension, ".guitheme"))
		defaultType = GetDeclTypeFromName("guiTheme");
	else if (!idStr::Icmp(extension, ".binding"))
		defaultType = GetDeclTypeFromName("keyBindings");
	else if (!idStr::Icmp(extension, ".radialmenu"))
		defaultType = GetDeclTypeFromName("radialMenuDef ");
	else
		defaultType = DECL_ENTITYDEF;
	declManagerLocal.RegisterDeclFolder(folder, extension, defaultType);
}

void idDeclManagerLocal::UnregisterDeclFolder( const char *folder, const char *extension ) {
	//Sys_Printf("UnregisterDeclFolder(%s, %s)\n", folder, extension);

	int i, j;
	idStr fileName;
	idList<idStr> fileList;

	// check whether this folder / extension combination already exists
	for (i = 0; i < declFolders.Num(); i++) {
		if (declFolders[i]->folder.Icmp(folder) == 0 && declFolders[i]->extension.Icmp(extension) == 0) {
			break;
		}
	}

	if (i >= declFolders.Num()) {
		common->Warning("idDeclManager::UnregisterDeclFolder: folder '%s' and extension '%s' not be registered", folder, extension);
		return;
	}

	declFolders.RemoveIndex(i);

	// check whether this file has already been loaded
	idList<idDeclFile *> rmList;
	for (j = 0; j < loadedFiles.Num(); j++) {
		if (loadedFiles[j]->fileName.IcmpPrefixPath(folder)) {
			continue;
		}
		idStr ext;
		loadedFiles[j]->fileName.ExtractFileExtension(ext);
		ext.Insert('.', 0);
		if (ext.Icmp(extension)) {
			continue;
		}

		// free decl files
		rmList.AddUnique(loadedFiles[j]);
	}

	for (j = 0; j < rmList.Num(); j++) {
		loadedFiles.Remove(rmList[j]);
	}
	rmList.DeleteContents(true);
}

void idDeclManagerLocal::FinishedRegistering() {
}

void idDeclManagerLocal::ListType( const idCmdArgs &args, const char* typeName ) {
}

void idDeclManagerLocal::PrintType( const idCmdArgs &args, const char* typeName ) {
}

int idDeclManagerLocal::GetNumMaterials( void ) {
	return declManagerLocal.GetNumDecls(DECL_MATERIAL);
}

void idDeclManagerLocal::CacheFromDict( const idDict& dict ) {
	for(int i = 0; i < declTypes.Num(); i++)
	{
		const idDeclType *declType = declTypes[i];
		if(declType && !declType->NotPrecached() && declType->type <= DECL_MAPDEF)
		{
			declType->CacheFromDict(dict);
		}
	}
}

const rvDeclEffect * idDeclManagerLocal::FindEffect( const char *name, bool makeDefault ) {
	return static_cast<const rvDeclEffect*>(FindType(DECLTYPE_EFFECT, name, makeDefault));
}

idDeclTypeInterface* idDeclManagerLocal::GetDeclType( const char* typeName ) const {
	for (int i = 0; i < declTypes.Num(); ++i) {
		idDeclTypeInterface *declType = declTypes[i];
		if(!declType)
			continue;
		if (!idStr::Cmp(declType->GetName(), typeName))
			return declType;
	}
	return NULL;
}

idDeclTypeInterface* idDeclManagerLocal::GetDeclType( qhandle_t typeHandle ) const {
	return declTypes[typeHandle];
}

qhandle_t idDeclManagerLocal::GetDeclTypeHandle( const char* typeName ) const {
	// 1. Find if exists
	for(int i = 0; i < declTypeTables.Num(); i++)
	{
		if(!idStr::Icmp(declTypeTables[i], typeName))
			return i;
	}
	// 2. Find empty slot for inserting
	for(int i = DECL_CUSTOMER_TYPE(0); i < declTypeTables.Num(); i++)
	{
		if(declTypeTables[i].IsEmpty())
		{
			declTypeTables[i] = typeName;
			return i;
		}
	}
	// 3. Append last
	return declTypeTables.Append(typeName);
}

const char* idDeclManagerLocal::GetDeclTypeName( qhandle_t typeHandle ) const {
	return declTypeTables[typeHandle].c_str();
}

void idDeclManagerLocal::AddDependency( const idDecl* decl, const idDecl* dependency ) {
}

void idDeclManagerLocal::AddDependency( const idDecl* decl, const char* fileName ) {
}

void idDeclManagerLocal::AddDependencies( const idDecl* decl, const idParser& parser ) {
}

static int ReadTokenCacheData(binaryTokenCache_t &header, idFile *file) {
	header.compressedData.SetNum(header.compressedLength);
	return file->Read(header.compressedData.Ptr(), header.compressedLength);
}

// huffman compression
static int DecompressTokenCache(binaryTokenCache_t &header, idList<byte> &out) {
	idCompressor *compressor;

	compressor = idCompressor::AllocHuffman();
	idFile_Memory f("globaltokens.cacheb", (const char *)header.compressedData.Ptr(), header.compressedLength); // FS_READ mode
	compressor->Init(&f, false, 8);
	out.SetNum(header.uncompressedLength * 2 + 2); // +2
	header.uncompressedLength = compressor->Read(out.Ptr(), out.Num());
	delete compressor;
	return header.uncompressedLength;
}

bool idDeclManagerLocal::LoadGlobalTokenCache(void)
{
	int magic;
	idFile *file;
	idStr path;
	Com_MakeBinaryFilename(path, NULL, GLOBALTOKENS_CACHEB);

	common->Printf("Decompressing the global token cache '%s'...\n", path.c_str());
	globalTokencache.Clear();

	file = fileSystem->OpenFileRead(path);

	if(!file)
	{
		common->Warning("Token cache file not exists: %s", path.c_str());
		return false;
	}

	file->ReadInt(magic);
	if(magic != CACHEB_MAGIC)
	{
		common->Warning("decl token cache : encountered unknown fileid");
		fileSystem->CloseFile(file);
		return false;
	}

	binaryTokenCache_t header;
	file->ReadInt(header.version); // version == 2
	if(header.version != CACHEB_VERSION)
	{
		common->Warning("decl token cache : wrong version (%i should be %i)", header.version, CACHEB_VERSION);
		fileSystem->CloseFile(file);
		return false;
	}

	file->ReadUnsignedInt(header.uncompressedLength);
	file->ReadUnsignedInt(header.compressedLength);
	ReadTokenCacheData(header, file);
	//assert(file->Tell() == file->Length());
	fileSystem->CloseFile(file);

	// huffman compression
	idList<byte> out;
	unsigned int uncompressedLength = DecompressTokenCache(header, out);

#if 0
	int num;
	fileSystem->WriteFile("globaltokens.cacheb.bin", &out[0], uncompressedLength);

	file = fileSystem->OpenFileRead("globaltokens.cacheb.bin");
	file->ReadInt(num);
	common->Printf("tokens: %d\n", num);

	idFile *os = fileSystem->OpenFileWrite("globaltokens.cacheb.txt");
	idToken token;
	for(int i = 0; i < num; i++)
	{
		file->ReadString(token);
		char c;
		file->ReadChar( c );
		token.type = c;
		file->ReadInt( token.subtype );

		int linesCrossed, flags;
		file->ReadInt( linesCrossed );

		file->ReadInt( flags );

		char whiteSpace;
		file->ReadChar( whiteSpace );

		os->Printf("%5d: |%s|%d,%d: %d 0x%X %d\n", i, token.c_str(), token.type, token.subtype, linesCrossed, flags, whiteSpace);
		common->Printf("%5d: |%s|%d,%d: %d 0x%X %d\n", i, token.c_str(), token.type, token.subtype, linesCrossed, flags, whiteSpace);
	}

	fileSystem->CloseFile(os);
	fileSystem->CloseFile(file);
#endif

	globalTokencache.ReadBuffer(&out[0], uncompressedLength);

	common->Printf("%ziKb\n", globalTokencache.Allocated());
#if 0
	for(int i = 0; i < globalTokencache.Num(); i++)
	{
		printf("%5d: %s\n", i,globalTokencache[i].c_str());
	}
#endif

	return true;
}

extern void OutputTextSource(idParser &src, sdStringBuilder_Heap &buf);
void idDeclManagerLocal::DeclbToText_f(const idCmdArgs &args) {
	idStr folder = GENERATED_PREFIX "/" GENERATED_DECLB;
	const char *extension = "";
	idFile *file;
	idStr outPath = "";
	if (args.Argc() > 1) {
		outPath.Append(args.Argv(1));
		outPath.Append("/");
	}
	if (args.Argc() > 2) {
		folder.AppendPath(args.Argv(2));
	}
	soundSystem->SetMute(true);

	idFileList* list = fileSystem->ListFilesTree(folder, extension, true);

	for (int d = 0; d < list->GetNumFiles(); d++)
	{
		const char *path = list->GetFile(d);

		file = fileSystem->OpenFileRead(path);

		if(!file)
		{
			common->Warning("declb file can't load: %s", path);
			continue;
		}

		binaryDecl_t header;
		if (LoadBinaryDeclHeader(header, file) == -1) {
			common->Warning("Decl binary read 0 entries: %s", path);
			fileSystem->CloseFile(file);
			continue;
		}

		sdStringBuilder_Heap buf;
		for (int m = 0; m < header.num; ++m) {
			binaryDeclEntry_t &entry = header.entries[m];
			/*buf.Append(entry.type.c_str());
			buf.Append(" ");
			buf.Append(entry.name.c_str());
			buf.Append(" {\n");*/

			file->Seek(entry.offset, FS_SEEK_SET);
			file->ReadUnsignedInt(entry.uncompressedLength);
			file->ReadUnsignedInt(entry.compressedLength);
			entry.data.SetNum(entry.compressedLength);
			file->Read(entry.data.Ptr(), entry.compressedLength);
			//file->Seek(pos, FS_SEEK_SET);

			idList<byte> out;
			DecompressDeclb(entry, out);

			if(out[0] == 6 && !idStr::Cmpn((const char *)&out[4], LEXB_VERSION, 6)) // is binary lex: like idFile::ReadString() num:int32 chars[num]
			{
				idParser src;
				src.LoadMemoryBinary(&out[0], entry.uncompressedLength, "<declbToText>", &declManagerLocal.globalTokencache);
				src.SetFlags(DECL_LEXER_FLAGS);
				OutputTextSource(src, buf);
				buf.Append("\n");
			}
			else
			{
				buf.Append((const char *)entry.data.Ptr(), entry.uncompressedLength);
				buf.Append("\n");
			}

			buf.Append("\n");
		}

		idStr out = va("%s%s", outPath.c_str(), path);
		out.StripTrailingOnce("b");
		fileSystem->WriteFile(out, buf.c_str(), buf.Length());
		common->Printf("Output declb to text: %s\n", out.c_str());

		fileSystem->CloseFile(file);
	}

	fileSystem->FreeFileList(list);
	soundSystem->SetMute(false);
}

void idDeclManagerLocal::ExportDeclSource(const char *savePath, const char *target, bool expand) {
	idStr outPath;
	if (savePath && savePath[0]) {
		outPath.Append(savePath);
	}

	soundSystem->SetMute(true);

	const idDeclFile *df;
	for (int d = 0; d < declManagerLocal.loadedFiles.Num(); d++)
	{
		df = declManagerLocal.loadedFiles[d];
		if(target && df->fileName.Icmp(target))
			continue;

		idStr out = outPath;
		out.AppendPath(df->fileName);
		idFile *file = fileSystem->OpenFileWrite(out);

		int c = 0;
		for (idDeclLocal *decl = df->decls; decl; decl = decl->nextInFile) {
			char *declText = (char *) _alloca((decl->GetTextLength() + 1) * sizeof(char));
			decl->GetText(declText);
			idStr finalPreprocessedBuffer;
			if (!expand || !GetDeclType(decl->GetType())->AllowTemplateEvaluation() || !sdDeclTemplate::ExpandTemplate(finalPreprocessedBuffer, declText, decl->GetTextLength()))
				finalPreprocessedBuffer.Append(declText, decl->GetTextLength());
			const idStrList &includeDependencies = decl->GetIncludeDependencies();
			if(includeDependencies.Num() > 0)
			{
				sdStringBuilder_Heap buf;
				for(int i = 0; i < includeDependencies.Num(); i++)
				{
					buf.Append("#include \"");
					buf.Append(includeDependencies[i]);
					buf.Append("\"\n");
				}
				buf.Append(finalPreprocessedBuffer.c_str());
				finalPreprocessedBuffer = buf.c_str();
			}

			sdStringBuilder_Heap buf;
			if(expand)
			{
				idParser src;
				idStr tmpPath = df->fileName;
				tmpPath.SetFileExtension(".exportDeclSource"); //karin: using same path for find include files
				src.LoadMemory(finalPreprocessedBuffer.c_str(), finalPreprocessedBuffer.Length(), tmpPath.c_str());
				src.SetFlags(DECL_LEXER_FLAGS);
				src.SkipUntilString("{");
				idToken token;
				token = "{";
				src.UnreadToken(&token);

				buf.Append(declManager->GetDeclNameFromType(decl->type));
				buf.Append(" ");
				buf.Append(decl->GetName());
				buf.Append(" ");
				OutputTextSource(src, buf);
			}
			else
			{
				buf.Append("// ");
				buf.Append(va("%d ", c));
				buf.Append(decl->GetName());
				buf.Append(": BEGIN\n");
				buf.Append(finalPreprocessedBuffer);
				buf.Append("\n// ");
				buf.Append(decl->GetName());
				buf.Append(": END\n");
			}

			buf.Append("\n\n");
			file->Write(buf.c_str(), buf.Length());
			c++;
		}
		common->Printf("Output %d decl source to file: %s\n", c, out.c_str());
		fileSystem->CloseFile(file);
	}

	soundSystem->SetMute(false);
}

void idDeclManagerLocal::ExportDeclSource_f(const idCmdArgs &args) {
	declManagerLocal.ExportDeclSource(args.Argc() > 1 ? args.Argv(1) : NULL, args.Argc() > 2 ? args.Argv(2) : NULL, false);
}

void idDeclManagerLocal::ExportDeclExpandSource_f(const idCmdArgs &args) {
	declManagerLocal.ExportDeclSource(args.Argc() > 1 ? args.Argv(1) : NULL, args.Argc() > 2 ? args.Argv(2) : NULL, true);
}

#endif
