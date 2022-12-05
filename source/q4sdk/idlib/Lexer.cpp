
#include "precompiled.h"
#pragma hdrstop

#ifdef LEXER_READ_AHEAD

namespace {

	#define IO_BUFFER_CHUNK_SIZE (1024 * 64)

	enum {
		IOJobPending = 0,
		IOJobQuit,
		IOJobComplete,
		IOJobCount
	};
	static HANDLE mIOJobs[IOJobCount];

	static DWORD mThreadId = 0;
	static HANDLE mThreadHandle = 0;


	struct IOJobData {
		idFile *mFile;
		int mAmtToRead;
		byte mBuffer[IO_BUFFER_CHUNK_SIZE];
	};

	IOJobData gIOJobData;

	void SetPendingJob(idFile *f, int size) {
		gIOJobData.mFile = f;
		gIOJobData.mAmtToRead = size;
		
		if ( mThreadHandle ) {
			SetEvent( mIOJobs[IOJobPending] );
		}
	}

	IOJobData *GetPendingJob() {
		return &gIOJobData;
	}

	void GetPendingJobResults(byte *dest, int &size) {
		if ( mThreadHandle ) {		
			if ( WAIT_OBJECT_0 == WaitForSingleObject( mIOJobs[IOJobComplete], INFINITE ) ) {
				//ResetEvent( mIOJobs[IOJobComplete] );
				size = gIOJobData.mAmtToRead;
				memcpy(dest, gIOJobData.mBuffer, size );
			} else {
				assert( 0 );
			}
		} else {
			gIOJobData.mFile->Read( dest, gIOJobData.mAmtToRead );
			size = gIOJobData.mAmtToRead;
		}
	}

	static LONG IOThread( void * ) {
		while ( 1 ) {
			DWORD res = WaitForMultipleObjects( 2, mIOJobs, FALSE, INFINITE );		
			if ( res == WAIT_OBJECT_0 ) {
				//ResetEvent( mIOJobs[IOJobPending] );
				IOJobData *j = GetPendingJob();
				j->mFile->Read( j->mBuffer, j->mAmtToRead );
				SetEvent( mIOJobs[IOJobComplete] );
			} else if ( res == (WAIT_OBJECT_0 + 1) ) {
				ExitThread(0);
			} else { 
				assert( 0 );
			}
		}
		ExitThread(0);
	}

	void InitIO() {
		for ( int i = 0; i < IOJobCount; ++i ) {
			//mIOJobs[i] = CreateEvent( 0, TRUE, FALSE, 0 );
			mIOJobs[i] = CreateEvent( 0, FALSE, FALSE, 0 );
		}
		mThreadHandle = CreateThread( NULL, 1024 * 512, ( LPTHREAD_START_ROUTINE )IOThread, 0, CREATE_SUSPENDED, &mThreadId );
		ResumeThread( mThreadHandle );
	}

	void ShutdownIO() {
		
		SetEvent( mIOJobs[IOJobQuit] );
		
		DWORD ecode = 0;
		while ( !GetExitCodeThread( mThreadHandle, &ecode ) ) {
			Sleep( 1 );
		}
		CloseHandle( mThreadHandle );

		for ( int i = 0; i < IOJobCount; ++i ) {
			CloseHandle( mIOJobs[i] );
		}
		
		mThreadHandle = 0;
	}

}

namespace ChunkBuffer {

	idFile *mFile;
	int mFileSize;
	int mBytesRead;
	
	int mFileBytesRead;
	
	byte mChunkMem[IO_BUFFER_CHUNK_SIZE];
	int mCurrentChunkBytes;
	int mCurrentChunkBytesRead = 0;
	
	/*
	================
	ChunkBuffer::Length
	================
	*/
	int Length() {
		return mFileSize;
	}
	
	/*
	================
	ChunkBuffer::BytesRead
	================
	*/
	int BytesRead() {
		return mBytesRead;
	}
	
	/*
	================
	ChunkBuffer::Begin
	================
	*/
	void Begin( idFile *f ) {
		mFile = f;
		mFileSize = f->Length();
		mBytesRead = 0;
		
		int initialRead = (mFileSize < IO_BUFFER_CHUNK_SIZE) ? mFileSize : IO_BUFFER_CHUNK_SIZE;
		mFile->Read( mChunkMem, initialRead );
		mFileBytesRead = initialRead;
		mCurrentChunkBytesRead = 0;
		mCurrentChunkBytes = initialRead;
		
		int bytesRemaining = mFileSize - mFileBytesRead;
		if ( bytesRemaining > 0 ) {
			SetPendingJob( mFile, (bytesRemaining > IO_BUFFER_CHUNK_SIZE) ? IO_BUFFER_CHUNK_SIZE : bytesRemaining );
		}
	}
	
	/*
	================
	ChunkBuffer::End
	================
	*/
	void End() { 
	}
	
	/*
	================
	ChunkBuffer::Advance
	================
	*/
	int Advance( byte *chunkMem ) {
		int readAmt = 0;
		GetPendingJobResults( chunkMem, readAmt );
		mFileBytesRead+=readAmt;
		
		int bytesRemaining = mFileSize - mFileBytesRead;
		if ( bytesRemaining > 0 ) {
			SetPendingJob( mFile, (bytesRemaining > IO_BUFFER_CHUNK_SIZE) ? IO_BUFFER_CHUNK_SIZE : bytesRemaining );
		}
		
		return readAmt;
	}
	
	/*
	================
	ChunkBuffer::Read
	================
	*/
	int Read( byte *dest, int size ) {
		int bytesRem = mFileSize - mBytesRead;
		
		if ( size > bytesRem ) {
			size = bytesRem;
		}
		
		if ( !size ) {
			return 0;
		}

		byte *writePos = dest;
		int cnt = size;
		
		while ( cnt ) {
			if ( mCurrentChunkBytesRead >= mCurrentChunkBytes ) {
				mCurrentChunkBytes = Advance( mChunkMem );
				mCurrentChunkBytesRead = 0;
			}
			
			int chunkBytesRemaining = mCurrentChunkBytes - mCurrentChunkBytesRead;
			int toRead = (cnt > chunkBytesRemaining) ? chunkBytesRemaining : cnt;
			
			byte *readPos = &mChunkMem[mCurrentChunkBytesRead];
			
			memcpy( writePos, readPos, toRead );
			
			cnt-=toRead;
			mCurrentChunkBytesRead+=toRead;
			writePos+=toRead;
		}
		
		mBytesRead+=size;		
		return size;
	}
}

/*
================
LexerIOWrapper::LexerIOWrapper
================
*/
LexerIOWrapper::LexerIOWrapper() {
	file = 0;	
	offset = 0;
	size = 0;
	memory = 0;
}

/*
================
LexerIOWrapper::InitFromMemory
================
*/
void LexerIOWrapper::InitFromMemory(char const * const ptr, int length) {
	file = 0;	
	offset = 0;
	size = length;
	memory = (char *)ptr;
}

/*
================
LexerIOWrapper::OpenFile
================
*/
bool LexerIOWrapper::OpenFile(char const *filename, bool OSPath) {
	if ( OSPath ) {
		file = idLib::fileSystem->OpenExplicitFileRead( filename );
	} else {
		file = idLib::fileSystem->OpenFileRead( filename );
	}
	
	if ( !file ) {
		return false;
	}
	
	ChunkBuffer::Begin( file );
	
	return true;
}

/*
================
LexerIOWrapper::Length
================
*/
int LexerIOWrapper::Length() {
	if ( file ) {
		return ChunkBuffer::Length();
	}
	return size;
}

/*
================
LexerIOWrapper::Tell
================
*/
int LexerIOWrapper::Tell() {
	if ( file ) {
		return ChunkBuffer::BytesRead();
	}
	return offset;
}

/*
================
LexerIOWrapper::Seek
================
*/
void LexerIOWrapper::Seek( int loc, int mode ) { 
	if ( file ) {
		if ( loc != 0 || mode != FS_SEEK_SET ) {
			common->Error( "lol\n" );
		}
		file->Seek( 0, FS_SEEK_SET );
		ChunkBuffer::Begin( file );
		return;
	}
	if ( mode == FS_SEEK_SET ) {
		offset = loc;
	} else {
		common->Error( "lol" );
	}
}

/*
================
LexerIOWrapper::Read
================
*/
void LexerIOWrapper::Read( void *dest, int s ) {
	if ( file ) {
		ChunkBuffer::Read( (byte*)dest, s );
		return;
	}
	char *d = (char *)dest;
	char *dEnd = d + (((s + offset) > size) ? (size - offset) : (s));
	while ( d < dEnd ) {
		*d = memory[offset];
		++d;
		++offset;
	}
}

/*
================
LexerIOWrapper::Close
================
*/
void LexerIOWrapper::Close() {
	if ( file ) {
		ChunkBuffer::End();
		idLib::fileSystem->CloseFile(file);
		return;
	}
}

/*
================
LexerIOWrapper::IsLoaded
================
*/
int LexerIOWrapper::IsLoaded() {
	if ( file || memory ) {
		return 1;
	}
	return 0;
}

/*
================
Lexer::BeginLevelLoad
================
*/
void Lexer::BeginLevelLoad() {
	InitIO();
}

/*
================
Lexer::EndLevelLoad
================
*/
void Lexer::EndLevelLoad() {
	ShutdownIO();
}

/*
================
ReadValue
================
*/
template <typename type> inline type ReadValue(LexerIOWrapper *in) 
{ 
	type ret; 
	
	in->Read(&ret, sizeof(type));
	return ret;
}

/*
================
ReadValue
specialization read for idStr's
================
*/
template <> inline idStr ReadValue(LexerIOWrapper *in) 
{ 
	char c;
	idStr str;

	in->Read(&c, 1);
	while(c != '\0')
	{
		str.Append(c);
		in->Read(&c, 1);
	}

	str.Append(c);

	return str;
}

#endif

#define PUNCTABLE

//longer punctuations first
punctuation_t default_punctuations[] = {
	//binary operators
	{">>=",P_RSHIFT_ASSIGN},
	{"<<=",P_LSHIFT_ASSIGN},
	//
	{"...",P_PARMS},
	//define merge operator
	{"##",P_PRECOMPMERGE},				// pre-compiler
	//logic operators
	{"&&",P_LOGIC_AND},					// pre-compiler
	{"||",P_LOGIC_OR},					// pre-compiler
	{">=",P_LOGIC_GEQ},					// pre-compiler
	{"<=",P_LOGIC_LEQ},					// pre-compiler
	{"==",P_LOGIC_EQ},					// pre-compiler
	{"!=",P_LOGIC_UNEQ},				// pre-compiler
	//arithmatic operators
	{"*=",P_MUL_ASSIGN},
	{"/=",P_DIV_ASSIGN},
	{"%=",P_MOD_ASSIGN},
	{"+=",P_ADD_ASSIGN},
	{"-=",P_SUB_ASSIGN},
	{"++",P_INC},
	{"--",P_DEC},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN},
	{"|=",P_BIN_OR_ASSIGN},
	{"^=",P_BIN_XOR_ASSIGN},
	{">>",P_RSHIFT},					// pre-compiler
	{"<<",P_LSHIFT},					// pre-compiler
	//reference operators
	{"->",P_POINTERREF},
	//C++
	{"::",P_CPP1},
	{".*",P_CPP2},
	//arithmatic operators
	{"*",P_MUL},						// pre-compiler
	{"/",P_DIV},						// pre-compiler
	{"%",P_MOD},						// pre-compiler
	{"+",P_ADD},						// pre-compiler
	{"-",P_SUB},						// pre-compiler
	{"=",P_ASSIGN},
	//binary operators
	{"&",P_BIN_AND},					// pre-compiler
	{"|",P_BIN_OR},						// pre-compiler
	{"^",P_BIN_XOR},					// pre-compiler
	{"~",P_BIN_NOT},					// pre-compiler
	//logic operators
	{"!",P_LOGIC_NOT},					// pre-compiler
	{">",P_LOGIC_GREATER},				// pre-compiler
	{"<",P_LOGIC_LESS},					// pre-compiler
	//reference operator
	{".",P_REF},
	//seperators
	{",",P_COMMA},						// pre-compiler
	{";",P_SEMICOLON},
	//label indication
	{":",P_COLON},						// pre-compiler
	//if statement
	{"?",P_QUESTIONMARK},				// pre-compiler
	//embracements
	{"(",P_PARENTHESESOPEN},			// pre-compiler
	{")",P_PARENTHESESCLOSE},			// pre-compiler
	{"{",P_BRACEOPEN},					// pre-compiler
	{"}",P_BRACECLOSE},					// pre-compiler
	{"[",P_SQBRACKETOPEN},
	{"]",P_SQBRACKETCLOSE},
	//
	{"\\",P_BACKSLASH},
	//precompiler operator
	{"#",P_PRECOMP},					// pre-compiler
	{"$",P_DOLLAR},
// RAVEN BEGIN
	{"¡",P_INVERTED_PLING},
	{"¿",P_INVERTED_QUERY},
// RAVEN END
	{NULL, 0}
};

int default_punctuationtable[256];
int default_nextpunctuation[sizeof(default_punctuations) / sizeof(punctuation_t)];
int default_setup;

// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
char Lexer::baseFolder[ 256 ];

// Added this to allow easy changing of the suffix that signifies a binary file
idStr const		Lexer::sCompiledFileSuffix("c");
// RAVEN END

/*
================
idLexer::CreatePunctuationTable
================
*/
void idLexer::CreatePunctuationTable( const punctuation_t *punctuations ) {
	int i, n, lastp;
	const punctuation_t *p, *newp;

	//get memory for the table
	if ( punctuations == default_punctuations ) {
		idLexer::punctuationtable = default_punctuationtable;
		idLexer::nextpunctuation = default_nextpunctuation;
		if ( default_setup ) {
			return;
		}
		default_setup = true;
		i = sizeof(default_punctuations) / sizeof(punctuation_t);
	}
	else {
		if ( !idLexer::punctuationtable || idLexer::punctuationtable == default_punctuationtable ) {
//RAVEN BEGIN
//amccarthy: Added memory allocation tag
			idLexer::punctuationtable = (int *) Mem_Alloc(256 * sizeof(int),MA_LEXER);
//RAVEN END
		}
		if ( idLexer::nextpunctuation && idLexer::nextpunctuation != default_nextpunctuation ) {
			Mem_Free( idLexer::nextpunctuation );
		}
		for (i = 0; punctuations[i].p; i++) {
		}
//RAVEN BEGIN
//amccarthy: Added memory allocation tag
		idLexer::nextpunctuation = (int *) Mem_Alloc(i * sizeof(int),MA_LEXER);
//RAVEN END
	}
	memset(idLexer::punctuationtable, 0xFF, 256 * sizeof(int));
	memset(idLexer::nextpunctuation, 0xFF, i * sizeof(int));
	//add the punctuations in the list to the punctuation table
	for (i = 0; punctuations[i].p; i++) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for (n = idLexer::punctuationtable[(unsigned int) newp->p[0]]; n >= 0; n = idLexer::nextpunctuation[n] ) {
			p = &punctuations[n];
			if (strlen(p->p) < strlen(newp->p)) {
				idLexer::nextpunctuation[i] = n;
				if (lastp >= 0) {
					idLexer::nextpunctuation[lastp] = i;
				}
				else {
					idLexer::punctuationtable[(unsigned int) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if (n < 0) {
			idLexer::nextpunctuation[i] = -1;
			if (lastp >= 0) {
				idLexer::nextpunctuation[lastp] = i;
			}
			else {
				idLexer::punctuationtable[(unsigned int) newp->p[0]] = i;
			}
		}
	}
}

/*
================
idLexer::GetPunctuationFromId
================
*/
const char *idLexer::GetPunctuationFromId( int id ) {
	int i;

	for (i = 0; idLexer::punctuations[i].p; i++) {
		if ( idLexer::punctuations[i].n == id ) {
			return idLexer::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
idLexer::GetPunctuationId
================
*/
int idLexer::GetPunctuationId( const char *p ) {
	int i;

	for (i = 0; idLexer::punctuations[i].p; i++) {
		if ( !idStr::Cmp(idLexer::punctuations[i].p, p) ) {
			return idLexer::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
idLexer::Error
================
*/

void idLexer::Error( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	if ( idLexer::flags & LEXFL_NOERRORS ) {
		return;
	}

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);

	if ( idLexer::flags & LEXFL_NOFATALERRORS ) {
		idLib::common->Warning( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
	} else {
		idLib::common->Error( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
	}
}

/*
================
idLexer::Warning
================
*/
void idLexer::Warning( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	if ( idLexer::flags & LEXFL_NOWARNINGS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	idLib::common->Warning( "file %s, line %d: %s", idLexer::filename.c_str(), idLexer::line, text );
}

/*
================
idLexer::SetPunctuations
================
*/
void idLexer::SetPunctuations( const punctuation_t *p ) {
#ifdef PUNCTABLE
	if (p) {
		idLexer::CreatePunctuationTable( p );
	}
	else {
		idLexer::CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if (p) {
		idLexer::punctuations = p;
	}
	else {
		idLexer::punctuations = default_punctuations;
	}
}

/*
================
idLexer::ReadWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
================
*/
int idLexer::ReadWhiteSpace( void ) {
	while(1) {
		// skip white space
// RAVEN BEGIN
		while(( byte )*idLexer::script_p <= ' ') {
// RAVEN END
			if (!*idLexer::script_p) {
				return 0;
			}
			if (*idLexer::script_p == '\n') {
				idLexer::line++;
			}
			idLexer::script_p++;
		}
		// skip comments
		if (*idLexer::script_p == '/') {
			// comments //
			if (*(idLexer::script_p+1) == '/') {
				idLexer::script_p++;
				do {
					idLexer::script_p++;
					if ( !*idLexer::script_p ) {
						return 0;
					}
				}
				while( *idLexer::script_p != '\n' );
				idLexer::line++;
				idLexer::script_p++;
				if ( !*idLexer::script_p ) {
					return 0;
				}
				continue;
			}
			// comments /* */
			else if (*(idLexer::script_p+1) == '*') {
				idLexer::script_p++;
				while( 1 ) {
					idLexer::script_p++;
					if ( !*idLexer::script_p ) {
						return 0;
					}
					if ( *idLexer::script_p == '\n' ) {
						idLexer::line++;
					}
					else if ( *idLexer::script_p == '/' ) {
						if ( *(idLexer::script_p-1) == '*' ) {
							break;
						}
						if ( *(idLexer::script_p+1) == '*' ) {
							idLexer::Warning( "nested comment" );
						}
					}
				}
				idLexer::script_p++;
				if ( !*idLexer::script_p ) {
					return 0;
				}
				idLexer::script_p++;
				if ( !*idLexer::script_p ) {
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

/*
================
idLexer::ReadEscapeCharacter
================
*/
int idLexer::ReadEscapeCharacter( char *ch ) {
	int c, val, i;

	// step over the leading '\\'
	idLexer::script_p++;
	// determine the escape character
	switch(*idLexer::script_p) {
		case '\\': c = '\\'; break;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 'a': c = '\a'; break;
		case '\'': c = '\''; break;
		case '\"': c = '\"'; break;
		case '\?': c = '\?'; break;
		case 'x':
		{
			idLexer::script_p++;
			for (i = 0, val = 0; ; i++, idLexer::script_p++) {
				c = *idLexer::script_p;
				if (c >= '0' && c <= '9')
					c = c - '0';
				else if (c >= 'A' && c <= 'Z')
					c = c - 'A' + 10;
				else if (c >= 'a' && c <= 'z')
					c = c - 'a' + 10;
				else
					break;
				val = (val << 4) + c;
			}
			idLexer::script_p--;
			if (val > 0xFF) {
				idLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
		default: //NOTE: decimal ASCII code, NOT octal
		{
			if (*idLexer::script_p < '0' || *idLexer::script_p > '9') {
				idLexer::Error("unknown escape char");
			}
			for (i = 0, val = 0; ; i++, idLexer::script_p++) {
				c = *idLexer::script_p;
				if (c >= '0' && c <= '9')
					c = c - '0';
				else
					break;
				val = val * 10 + c;
			}
			idLexer::script_p--;
			if (val > 0xFF) {
				idLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
	}
	// step over the escape character or the last digit of the number
	idLexer::script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
idLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int idLexer::ReadString( idToken *token, int quote ) {
	int tmpline;
	const char *tmpscript_p;
	char ch;

	if ( quote == '\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	idLexer::script_p++;

	while(1) {
		// if there is an escape character and escape characters are allowed
		if (*idLexer::script_p == '\\' && !(idLexer::flags & LEXFL_NOSTRINGESCAPECHARS)) {
			if ( !idLexer::ReadEscapeCharacter( &ch ) ) {
				return 0;
			}
			token->AppendDirty( ch );
		}
		// if a trailing quote
		else if (*idLexer::script_p == quote) {
			// step over the quote
			idLexer::script_p++;
			// if consecutive strings should not be concatenated
			if ( (idLexer::flags & LEXFL_NOSTRINGCONCAT) &&
					(!(idLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"')) ) {
				break;
			}

			tmpscript_p = idLexer::script_p;
			tmpline = idLexer::line;
			// read white space between possible two consecutive strings
			if ( !idLexer::ReadWhiteSpace() ) {
				idLexer::script_p = tmpscript_p;
				idLexer::line = tmpline;
				break;
			}

			if ( idLexer::flags & LEXFL_NOSTRINGCONCAT ) {
				if ( *idLexer::script_p != '\\' ) {
					idLexer::script_p = tmpscript_p;
					idLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				idLexer::script_p++;
				if ( !idLexer::ReadWhiteSpace() || ( *idLexer::script_p != quote ) ) {
					idLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if ( *idLexer::script_p != quote ) {
				idLexer::script_p = tmpscript_p;
				idLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			idLexer::script_p++;
		}
		else {
			if (*idLexer::script_p == '\0') {
				idLexer::Error( "missing trailing quote" );
				return 0;
			}
			if (*idLexer::script_p == '\n') {
				idLexer::Error( "newline inside string" );
				return 0;
			}
			token->AppendDirty( *idLexer::script_p++ );
		}
	}
	token->data[token->len] = '\0';

	if ( token->type == TT_LITERAL ) {
		if ( !(idLexer::flags & LEXFL_ALLOWMULTICHARLITERALS) ) {
			if ( token->Length() != 1 ) {
				idLexer::Warning( "literal is not one character long" );
			}
		}
		token->subtype = (*token)[0];
	}
	else {
		// the sub type is the length of the string
		token->subtype = token->Length();
	}
	return 1;
}

/*
================
idLexer::ReadName
================
*/
int idLexer::ReadName( idToken *token ) {
	char c;

	token->type = TT_NAME;
	do {
		token->AppendDirty( *idLexer::script_p++ );
		c = *idLexer::script_p;
// RAVEN BEGIN
	} while ( idStr::CharIsAlpha( c ) || idStr::CharIsNumeric( c ) || c == '_' ||
// RAVEN EBD
				// if treating all tokens as strings, don't parse '-' as a seperate token
				((idLexer::flags & LEXFL_ONLYSTRINGS) && (c == '-')) ||
				// if special path name characters are allowed
				((idLexer::flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.')) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

/*
================
idLexer::CheckString
================
*/
ID_INLINE int idLexer::CheckString( const char *str ) const {
	int i;

	for ( i = 0; str[i]; i++ ) {
		if ( idLexer::script_p[i] != str[i] ) {
			return false;
		}
	}
	return true;
}

/*
================
idLexer::ReadNumber
================
*/
int idLexer::ReadNumber( idToken *token ) {
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *idLexer::script_p;
	c2 = *(idLexer::script_p + 1);

	if ( c == '0' && c2 != '.' ) {
		// check for a hexadecimal number
		if ( c2 == 'x' || c2 == 'X' ) {
			token->AppendDirty( *idLexer::script_p++ );
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while((c >= '0' && c <= '9') ||
						(c >= 'a' && c <= 'f') ||
						(c >= 'A' && c <= 'F')) {
				token->AppendDirty( c );
				c = *(++idLexer::script_p);
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if ( c2 == 'b' || c2 == 'B' ) {
			token->AppendDirty( *idLexer::script_p++ );
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( c == '0' || c == '1' ) {
				token->AppendDirty( c );
				c = *(++idLexer::script_p);
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else {
			token->AppendDirty( *idLexer::script_p++ );
			c = *idLexer::script_p;
			while( c >= '0' && c <= '7' ) {
				token->AppendDirty( c );
				c = *(++idLexer::script_p);
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else {
		// decimal integer or floating point number or ip address
		dot = 0;
		while( 1 ) {
			if ( c >= '0' && c <= '9' ) {
			}
			else if ( c == '.' ) {
				dot++;
			}
			else {
				break;
			}
			token->AppendDirty( c );
			c = *(++idLexer::script_p);
		}
		if( c == 'e' && dot == 0) {
			//We have scientific notation without a decimal point
			dot++;
		}
		// if a floating point number
		if ( dot == 1 ) {
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if ( c == 'e' ) {
				//Append the e so that GetFloatValue code works
				token->AppendDirty( c );
				c = *(++idLexer::script_p);
				if ( c == '-' ) {
					token->AppendDirty( c );
					c = *(++idLexer::script_p);
				}
				else if ( c == '+' ) {
					token->AppendDirty( c );
					c = *(++idLexer::script_p);
				}
				while( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *(++idLexer::script_p);
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if ( c == '#' ) {
				c2 = 4;
				if ( CheckString( "INF" ) ) {
					token->subtype |= TT_INFINITE;
				}
				else if ( CheckString( "IND" ) ) {
					token->subtype |= TT_INDEFINITE;
				}
				else if ( CheckString( "NAN" ) ) {
					token->subtype |= TT_NAN;
				}
				else if ( CheckString( "QNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				}
				else if ( CheckString( "SNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				}
				for ( i = 0; i < c2; i++ ) {
					token->AppendDirty( c );
					c = *(++idLexer::script_p);
				}
				while( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *(++idLexer::script_p);
				}
				if ( !(idLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS) ) {
					token->AppendDirty( 0 );	// zero terminate for c_str
					idLexer::Error( "parsed %s", token->c_str() );
				}
			}
		}
		else if ( dot > 1 ) {
			if ( !( idLexer::flags & LEXFL_ALLOWIPADDRESSES ) ) {
				idLexer::Error( "more than one dot in number" );
				return 0;
			}
			if ( dot != 3 ) {
				idLexer::Error( "ip address should have three dots" );
				return 0;
			}
			token->subtype = TT_IPADDRESS;
		}
		else {
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if ( token->subtype & TT_FLOAT ) {
		if ( c > ' ' ) {
			// single-precision: float
			if ( c == 'f' || c == 'F' ) {
				token->subtype |= TT_SINGLE_PRECISION;
				idLexer::script_p++;
			}
			// extended-precision: long double
			else if ( c == 'l' || c == 'L' ) {
				token->subtype |= TT_EXTENDED_PRECISION;
				idLexer::script_p++;
			}
			// default is double-precision: double
			else {
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else {
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if ( token->subtype & TT_INTEGER ) {
		if ( c > ' ' ) {
			// default: signed long
			for ( i = 0; i < 2; i++ ) {
				// long integer
				if ( c == 'l' || c == 'L' ) {
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if ( c == 'u' || c == 'U' ) {
					token->subtype |= TT_UNSIGNED;
				}
				else {
					break;
				}
				c = *(++idLexer::script_p);
			}
		}
	}
	else if ( token->subtype & TT_IPADDRESS ) {
		if ( c == ':' ) {
			token->AppendDirty( c );
			c = *(++idLexer::script_p);
			while( c >= '0' && c <= '9' ) {
				token->AppendDirty( c );
				c = *(++idLexer::script_p);
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

/*
================
idLexer::ReadPunctuation
================
*/
int idLexer::ReadPunctuation( idToken *token ) {
	int l, n, i;
	char *p;
	const punctuation_t *punc;

#ifdef PUNCTABLE
	for (n = idLexer::punctuationtable[(unsigned int)*(idLexer::script_p)]; n >= 0; n = idLexer::nextpunctuation[n])
	{
		punc = &(idLexer::punctuations[n]);
#else
	int i;

	for (i = 0; idLexer::punctuations[i].p; i++) {
		punc = &idLexer::punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for ( l = 0; p[l] && idLexer::script_p[l]; l++ ) {
			if ( idLexer::script_p[l] != p[l] ) {
				break;
			}
		}
		if ( !p[l] ) {
			//
			token->EnsureAlloced( l+1, false );
			for ( i = 0; i <= l; i++ ) {
				token->data[i] = p[i];
			}
			token->len = l;
			//
			idLexer::script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return 1;
		}
	}
	return 0;
}

/*
================
idLexer::ReadToken
================
*/
int idLexer::ReadToken( idToken *token ) {
	int c;

	if ( !loaded ) {
// RAVEN BEGIN
#ifndef _XENON
// nrausch: should not be an error on xenon since it prevents the precache stuff from working
		idLib::common->Error( "idLexer::ReadToken: no file loaded" );
#else
		idLib::common->Warning( "idLexer::ReadToken: no file loaded" );
#endif
// RAVEN END
		return 0;
	}

	// if there is a token available (from unreadToken)
	if ( tokenavailable ) {
		tokenavailable = 0;
		*token = idLexer::token;
		return 1;
	}
	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[0] = '\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if ( !ReadWhiteSpace() ) {
		return 0;
	}
	// end of the white space
	idLexer::whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *idLexer::script_p;

	// if we're keeping everything as whitespace deliminated strings
	if ( idLexer::flags & LEXFL_ONLYSTRINGS ) {
		// if there is a leading quote
		if ( c == '\"' || c == '\'' ) {
			if (!idLexer::ReadString( token, c )) {
				return 0;
			}
		} else if ( !idLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// if there is a number
	else if ( (c >= '0' && c <= '9') ||
			(c == '.' && (*(idLexer::script_p + 1) >= '0' && *(idLexer::script_p + 1) <= '9')) ) {
		if ( !idLexer::ReadNumber( token ) ) {
			return 0;
		}
		// if names are allowed to start with a number
		if ( idLexer::flags & LEXFL_ALLOWNUMBERNAMES ) {
			c = *idLexer::script_p;
			if ( idStr::CharIsAlpha( c ) || idStr::CharIsNumeric( c ) || c == '_' ) {
				if ( !idLexer::ReadName( token ) ) {
					return 0;
				}
			}
		}
	}
	// if there is a leading quote
	else if ( c == '\"' || c == '\'' ) {
		if (!idLexer::ReadString( token, c )) {
			return 0;
		}
	}
	// if there is a name
// RAVEN BEGIN
	else if ( idStr::CharIsAlpha( c ) || idStr::CharIsNumeric( c ) || c == '_' ) {
// RAVEN END
		if ( !idLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// names may also start with a slash when pathnames are allowed
	else if ( ( idLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( (c == '/' || c == '\\') || c == '.' ) ) {
		if ( !idLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// check for punctuations
	else if ( !idLexer::ReadPunctuation( token ) ) {
		idLexer::Error( "unknown punctuation %c", c );
		return 0;
	}
// RAVEN BEGIN
// jsinger: added to write out the binary version of this token when binary writes have been turned on
	WriteBinaryToken(token);

// RAVEN END
	// succesfully read a token
	return 1;
}

/*
================
idLexer::ExpectTokenString
================
*/
int idLexer::ExpectTokenString( const char *string ) {
	idToken token;

	if (!idLexer::ReadToken( &token )) {
		idLexer::Error( "couldn't find expected '%s'", string );
		return 0;
	}
	if ( token != string ) {
		idLexer::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}

/*
================
idLexer::ExpectTokenType
================
*/
int idLexer::ExpectTokenType( int type, int subtype, idToken *token ) {
	idStr str;

	if ( !idLexer::ReadToken( token ) ) {
		idLexer::Error( "couldn't read expected token" );
		return 0;
	}

	if ( token->type != type ) {
		switch( type ) {
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		idLexer::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
		return 0;
	}
	if ( token->type == TT_NUMBER ) {
		if ( (token->subtype & subtype) != subtype ) {
			str.Clear();
			if ( subtype & TT_DECIMAL ) str = "decimal ";
			if ( subtype & TT_HEX ) str = "hex ";
			if ( subtype & TT_OCTAL ) str = "octal ";
			if ( subtype & TT_BINARY ) str = "binary ";
			if ( subtype & TT_UNSIGNED ) str += "unsigned ";
			if ( subtype & TT_LONG ) str += "long ";
			if ( subtype & TT_FLOAT ) str += "float ";
			if ( subtype & TT_INTEGER ) str += "integer ";
			str.StripTrailing( ' ' );
			idLexer::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			idLexer::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			idLexer::Error( "expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
idLexer::ExpectAnyToken
================
*/
int idLexer::ExpectAnyToken( idToken *token ) {
	if (!idLexer::ReadToken( token )) {
		idLexer::Error( "couldn't read expected token" );
		return 0;
	}
	else {
		return 1;
	}
}

/*
================
idLexer::CheckTokenString
================
*/
int idLexer::CheckTokenString( const char *string ) {
	idToken tok;

	if (!idLexer::ReadToken( &tok )) {
		return 0;
	}

	// if the token is available
	if ( tok == string ) {
		return 1;
	}
	// token not available
	UnreadToken( &tok );
	return 0;
}

/*
================
idLexer::CheckTokenType
================
*/
int idLexer::CheckTokenType( int type, int subtype, idToken *token ) {
	idToken tok;

	if (!idLexer::ReadToken( &tok )) {
		return 0;
	}
	// if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		*token = tok;
		return 1;
	}
	// token is not available
	idLexer::script_p = lastScript_p;
	idLexer::line = lastline;
	return 0;
}

/*
================
idLexer::SkipUntilString
================
*/
int idLexer::SkipUntilString( const char *string ) {
	idToken token;

	while(idLexer::ReadToken( &token )) {
		if ( token == string ) {
			return 1;
		}
	}
	return 0;
}

/*
================
idLexer::SkipRestOfLine
================
*/
int idLexer::SkipRestOfLine( void ) {
	idToken token;

	while(idLexer::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			idLexer::script_p = lastScript_p;
			idLexer::line = lastline;
			return 1;
		}
	}
	return 0;
}

/*
=================
idLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int idLexer::SkipBracedSection( bool parseFirstBrace ) {
	idToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token == "{" ) {
				depth++;
			} else if ( token == "}" ) {
				depth--;
			}
		}
	} while( depth );
	return true;
}

/*
================
idLexer::UnreadToken
================
*/
void idLexer::UnreadToken( const idToken *token ) {
	if ( idLexer::tokenavailable ) {
		idLib::common->FatalError( "idLexer::unreadToken, unread token twice\n" );
	}
	idLexer::token = *token;
	idLexer::tokenavailable = 1;
}

/*
================
idLexer::ReadTokenOnLine
================
*/
int idLexer::ReadTokenOnLine( idToken *token ) {
	idToken tok;

	if (!idLexer::ReadToken( &tok )) {
		idLexer::script_p = lastScript_p;
		idLexer::line = lastline;
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	// restore our position
	idLexer::script_p = lastScript_p;
	idLexer::line = lastline;
	token->Clear();
	return false;
}

/*
================
idLexer::ReadRestOfLine
================
*/
const char*	idLexer::ReadRestOfLine(idStr& out) {
	while(1) {

		if(*idLexer::script_p == '\n') {
			idLexer::line++;
			break;
		}

		if(!*idLexer::script_p) {
			break;
		}

		if(*idLexer::script_p <= ' ') {
			out += " ";
		} else {
			out += *idLexer::script_p;
		}
		idLexer::script_p++;

	}

	out.Strip(' ');
	return out.c_str();
}

/*
================
idLexer::ParseInt
================
*/
int idLexer::ParseInt( void ) {
	idToken token;

	if ( !idLexer::ReadToken( &token ) ) {
		idLexer::Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		idLexer::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -((signed int) token.GetIntValue());
	}
	else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		idLexer::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
idLexer::ParseBool
================
*/
bool idLexer::ParseBool( void ) {
	idToken token;

	if ( !idLexer::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		idLexer::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
idLexer::ParseFloat
================
*/
float idLexer::ParseFloat( bool *errorFlag ) {
	idToken token;

	if ( errorFlag ) {
		*errorFlag = false;
	}

	if ( !idLexer::ReadToken( &token ) ) {
		if ( errorFlag ) {
			idLexer::Warning( "couldn't read expected floating point number" );
			*errorFlag = true;
		} else {
			idLexer::Error( "couldn't read expected floating point number" );
		}
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		idLexer::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if ( token.type != TT_NUMBER ) {
		if ( errorFlag ) {
			idLexer::Warning( "expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		} else {
			idLexer::Error( "expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
idLexer::Parse1DMatrix
================
*/
int idLexer::Parse1DMatrix( int x, float *m ) {
	int i;

	if ( !idLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = idLexer::ParseFloat();
	}

	if ( !idLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

// RAVEN BEGIN
// rjohnson: added vertex color support to proc files.  assume a default RGBA of 0x000000ff
/*
================
idLexer::Parse1DMatrixOpenEnded
================
*/
int idLexer::Parse1DMatrixOpenEnded( int MaxCount, float *m ) {
	int i;

	if ( !idLexer::ExpectTokenString( "(" ) ) {
		return 0;
	}

	for ( i = 0; i < MaxCount; i++ ) {
		idToken tok;

		if (!idLexer::ReadToken( &tok )) {
			return 0;
		}

		if ( tok == ")" ) {
			return i;
		}

		idLexer::UnreadToken( &tok );

		m[i] = idLexer::ParseFloat();
	}

	if ( !idLexer::ExpectTokenString( ")" ) ) {
		return 0;
	}

	return i;
}
// RAVEN END

/*
================
idLexer::Parse2DMatrix
================
*/
int idLexer::Parse2DMatrix( int y, int x, float *m ) {
	int i;

	if ( !idLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !idLexer::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !idLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
idLexer::Parse3DMatrix
================
*/
int idLexer::Parse3DMatrix( int z, int y, int x, float *m ) {
	int i;

	if ( !idLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0 ; i < z; i++ ) {
		if ( !idLexer::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !idLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
idLexer::ParseNumericStructArray
================
*/
void idLexer::ParseNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage )
{
	int arrayOffset, curElement;

	for ( arrayOffset = 0; arrayOffset < arrayCount; arrayOffset++ )
	{
		for ( curElement = 0; curElement < numStructElements; curElement++ )
		{
			if ( tokenSubTypeStructElements[curElement] & TT_FLOAT )
			{
				*(float*)arrayStorage = idLexer::ParseFloat();
				arrayStorage += sizeof(float);
			}
			else
			{
				*(int*)arrayStorage = idLexer::ParseInt();
				arrayStorage += sizeof(int);
			}
		}
	}
}

/*
=================
idParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
const char *idLexer::ParseBracedSectionExact( idStr &out, int tabs ) {
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if ( !idLexer::ExpectTokenString( "{" ) ) {
		return out.c_str( );
	}

	out = "{";
	depth = 1;	
	skipWhite = false;
	doTabs = tabs >= 0;

	while( depth && *idLexer::script_p ) {
		char c = *(idLexer::script_p++);

		switch ( c ) {
			case '\t':
			case ' ': {
				if ( skipWhite ) {
					continue;
				}
				break;
			}
			case '\n': {
// RAVEN BEGIN
// jscott: now gives correct line number in error reports
				line++;
// RAVEN END
				if ( doTabs ) {
					skipWhite = true;
					out += c;
					continue;
				}
				break;
			}
			case '{': {
				depth++;
				tabs++;
				break;
			}
			case '}': {
				depth--;
				tabs--;
				break;				
			}
		}

		if ( skipWhite ) {
			int i = tabs;
			if ( c == '{' ) {
				i--;
			}
			skipWhite = false;
			for ( ; i > 0; i-- ) {
				out += '\t';
			}
		}
		out += c;
	}
	return out.c_str();
}

/*
=================
idLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *idLexer::ParseBracedSection( idStr &out ) {
	idToken token;
	int i, depth;

	out.Empty();
	if ( !idLexer::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !idLexer::ReadToken( &token ) ) {
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for ( i = 0; i < token.linesCrossed; i++ ) {
			out += "\r\n";
		}

		if ( token.type == TT_PUNCTUATION ) {
			if ( token[0] == '{' ) {
				depth++;
			}
			else if ( token[0] == '}' ) {
				depth--;
			}
		}

		if ( token.type == TT_STRING ) {
			out += "\"" + token + "\"";
		}
		else {
			out += token;
		}
		out += " ";
	} while( depth );

	return out.c_str();
}

/*
=================
idLexer::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *idLexer::ParseRestOfLine( idStr &out ) {
	idToken token;

	out.Empty();
	while(idLexer::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			idLexer::script_p = lastScript_p;
			idLexer::line = lastline;
			break;
		}
		if ( out.Length() ) {
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
idLexer::GetLastWhiteSpace
================
*/
int idLexer::GetLastWhiteSpace( idStr &whiteSpace ) const {
	whiteSpace.Clear();
	for ( const char *p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ ) {
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
idLexer::GetLastWhiteSpaceStart
================
*/
int idLexer::GetLastWhiteSpaceStart( void ) const {
	return whiteSpaceStart_p - buffer;
}

/*
================
idLexer::GetLastWhiteSpaceEnd
================
*/
int idLexer::GetLastWhiteSpaceEnd( void ) const {
	return whiteSpaceEnd_p - buffer;
}

/*
================
idLexer::Reset
================
*/
void idLexer::Reset( void ) {
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// begin of white space
	idLexer::whiteSpaceStart_p = NULL;
	// end of white space
	idLexer::whiteSpaceEnd_p = NULL;
	// set if there's a token available in idLexer::token
	idLexer::tokenavailable = 0;

	idLexer::line = 1;
	idLexer::lastline = 1;
	// clear the saved token
	idLexer::token = "";
}

/*
================
idLexer::EndOfFile
================
*/
int idLexer::EndOfFile( void ) {
	return idLexer::script_p >= idLexer::end_p;
}

/*
================
idLexer::NumLinesCrossed
================
*/
int idLexer::NumLinesCrossed( void ) {
	return idLexer::line - idLexer::lastline;
}

/*
================
idLexer::LoadFile
================
*/
int idLexer::LoadFile( const char *filename, bool OSPath ) {
	idFile *fp;
	idStr pathname;
	int length;
	char *buf;

	if ( idLexer::loaded ) {
		idLib::common->Error("idLexer::LoadFile: another script already loaded");
		return false;
	}
	
	if ( !OSPath && ( baseFolder[0] != '\0' ) ) {
		pathname = va( "%s/%s", baseFolder, filename );
	} else {
		pathname = filename;
	}
	if ( OSPath ) {
		fp = idLib::fileSystem->OpenExplicitFileRead( pathname );
	} else {
		fp = idLib::fileSystem->OpenFileRead( pathname );
	}
	if ( !fp ) {
		return false;
	}
	length = fp->Length();
// RAVEN BEGIN
// amccarthy: Added memory allocation tag
	buf = (char *) Mem_Alloc( length + 1, MA_LEXER );
	if( !buf ) {
		common->FatalError( "Memory system failure : out of memory" );
	}
// RAVEN END
	buf[length] = '\0';
	fp->Read( buf, length );
	idLexer::fileTime = fp->Timestamp();
	idLexer::filename = fp->GetFullPath();
	idLib::fileSystem->CloseFile( fp );

	idLexer::buffer = buf;
	idLexer::length = length;
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// pointer to end of script buffer
	idLexer::end_p = &(idLexer::buffer[length]);

	idLexer::tokenavailable = 0;
	idLexer::line = 1;
	idLexer::lastline = 1;
	idLexer::allocated = true;
	idLexer::loaded = true;

// RAVEN BEGIN
// jsinger: initialize compiled file
	if(flags & LEXFL_WRITEBINARY)
	{
		pathname.Append(Lexer::sCompiledFileSuffix);
		if ( OSPath ) {
			mBinaryFile = idLib::fileSystem->OpenExplicitFileWrite( pathname );
		} else {
			mBinaryFile = idLib::fileSystem->OpenFileWrite( pathname );
		}
	}
// RAVEN END

	return true;
}

/*
================
idLexer::LoadMemory
================
*/
int idLexer::LoadMemory( const char *ptr, int length, const char *name, int startLine ) {
	if ( idLexer::loaded ) {
		idLib::common->Error("idLexer::LoadMemory: another script already loaded");
		return false;
	}
	idLexer::filename = name;
	idLexer::buffer = ptr;
	idLexer::fileTime = 0;
	idLexer::length = length;
	// pointer in script buffer
	idLexer::script_p = idLexer::buffer;
	// pointer in script buffer before reading token
	idLexer::lastScript_p = idLexer::buffer;
	// pointer to end of script buffer
	idLexer::end_p = &(idLexer::buffer[length]);

	idLexer::tokenavailable = 0;
	idLexer::line = startLine;
	idLexer::lastline = startLine;
	idLexer::allocated = false;
	idLexer::loaded = true;

	return true;
}

/*
================
idLexer::FreeSource
================
*/
void idLexer::FreeSource( void ) {
#ifdef PUNCTABLE
	if ( idLexer::punctuationtable && idLexer::punctuationtable != default_punctuationtable ) {
		Mem_Free( (void *) idLexer::punctuationtable );
		idLexer::punctuationtable = NULL;
	}
	if ( idLexer::nextpunctuation && idLexer::nextpunctuation != default_nextpunctuation ) {
		Mem_Free( (void *) idLexer::nextpunctuation );
		idLexer::nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if ( idLexer::allocated ) {
		Mem_Free( (void *) idLexer::buffer );
		idLexer::buffer = NULL;
		idLexer::allocated = false;
	}
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::loaded = false;
// RAVEN BEGIN
// jsinger: close compile file if it exists
	if(flags & LEXFL_WRITEBINARY)
	{
		if(mBinaryFile)
		{
			idLib::fileSystem->CloseFile(mBinaryFile);
			mBinaryFile = NULL;
		}
	}
// RAVEN END
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( void ) {
	idLexer::loaded = false;
	idLexer::filename = "";
	idLexer::flags = 0;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::fileTime = 0;
	idLexer::length = 0;
	idLexer::line = 0;
	idLexer::lastline = 0;
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
// RAVEN BEGIN
// jsinger: initialize compiled file
	idLexer::mBinaryFile = NULL;
// RAVEN END
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( int flags ) {
	idLexer::loaded = false;
	idLexer::filename = "";
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::fileTime = 0;
	idLexer::length = 0;
	idLexer::line = 0;
	idLexer::lastline = 0;
	idLexer::tokenavailable = 0;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
// RAVEN BEGIN
// jsinger: initialize compiled file
	idLexer::mBinaryFile = NULL;
// RAVEN END
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char *filename, int flags, bool OSPath ) {
	idLexer::loaded = false;
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
// RAVEN BEGIN
// jsinger: initialize compiled file
	idLexer::mBinaryFile = NULL;
// RAVEN END
	idLexer::LoadFile( filename, OSPath );
}

/*
================
idLexer::idLexer
================
*/
idLexer::idLexer( const char *ptr, int length, const char *name, int flags ) {
	idLexer::loaded = false;
	idLexer::flags = flags;
	idLexer::SetPunctuations( NULL );
	idLexer::allocated = false;
	idLexer::token = "";
	idLexer::next = NULL;
	idLexer::hadError = false;
// RAVEN BEGIN
// jsinger: initialize compiled file
	idLexer::mBinaryFile = NULL;
// RAVEN END
	idLexer::LoadMemory( ptr, length, name );
}

/*
================
idLexer::~idLexer
================
*/
idLexer::~idLexer( void ) {
	idLexer::FreeSource();
}

// RAVEN BEGIN
// jsinger: SetBaseFolder was moved to the Lexer base class to unify its functionality across all
//			derived classes
/*
================
idLexer::SetBaseFolder
================

void idLexer::SetBaseFolder( const char *path ) {
	idStr::Copynz( baseFolder, path, sizeof( baseFolder ) );
}
*/
// RAVEN END
/*
================
idLexer::HadError
================
*/
bool idLexer::HadError( void ) const {
	return hadError;
}


// RAVEN BEGIN
// jsinger: This method can write out a binary representation of a token in a format
//          suitable to be read by the Lexer
/*
================
idLexer::WriteBinaryToken
	Writes a binary representation of the token value to the compiled file
	when compiling is turned on
================
*/
void idLexer::WriteBinaryToken(idToken *tok)
{
	bool swapBytes = (flags & LEXFL_BYTESWAP) == LEXFL_BYTESWAP;

	if(flags & LEXFL_WRITEBINARY)
	{
		unsigned char prefix;

		switch(tok->type)
		{
		case TT_NUMBER:
			if(tok->subtype & TT_INTEGER)
			{
				if(tok->subtype & TT_UNSIGNED)
				{
					unsigned long val = tok->GetUnsignedLongValue();
					unsigned char byteVal = (unsigned char)val;
					unsigned short shortVal = (unsigned short)val;
				
					if(byteVal == val)
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_UNSIGNEDINT, BTT_STORED_1BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<unsigned char>(byteVal, mBinaryFile, swapBytes);
					}
					else if(shortVal == val)
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_UNSIGNEDINT, BTT_STORED_2BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<unsigned short>(shortVal, mBinaryFile, swapBytes);
					}
					else
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_UNSIGNEDINT, BTT_STORED_4BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<unsigned int>(val, mBinaryFile, swapBytes);
					}
				}
				else
				{
					long val = tok->GetIntValue();
					char byteVal = (char)val;
					short shortVal = (short)val;
				
					if(byteVal == val)
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_INT, BTT_STORED_1BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<char>(byteVal, mBinaryFile, swapBytes);
					}
					else if(shortVal == val)
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_INT, BTT_STORED_2BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<short>(shortVal, mBinaryFile, swapBytes);
					}
					else
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_INT, BTT_STORED_4BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<int>(val, mBinaryFile, swapBytes);
					}
				}
			}
			else if(tok->subtype & TT_FLOAT)
			{
				if(tok->subtype & TT_SINGLE_PRECISION)
				{				
					float val = tok->GetFloatValue();
					int intval = tok->GetIntValue();
					if(((float)intval) == val)		// integral check
					{
						char byteVal = (char)intval;
						short shortVal = (short)intval;
					
						if(byteVal == intval)
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_1BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<char>(byteVal, mBinaryFile, swapBytes);
						}
						else if(shortVal == intval)
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_2BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<short>(shortVal, mBinaryFile, swapBytes);
						}
						else
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_4BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<float>(val, mBinaryFile, swapBytes);
						}
					}
					else
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_4BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<float>(val, mBinaryFile, swapBytes);
					}
				}
				else if(tok->subtype & TT_DOUBLE_PRECISION)
				{
					// we can write out doubles as floats because the text file doesn't have double precision anyway
					float val = tok->GetDoubleValue();
					int intval = tok->GetIntValue();
					if(((float)intval) == val)		// integral check
					{
						char byteVal = (char)intval;
						short shortVal = (short)intval;
					
						if(byteVal == intval)
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_1BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<char>(byteVal, mBinaryFile, swapBytes);
						}
						else if(shortVal == intval)
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_FLOAT, BTT_STORED_2BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<short>(shortVal, mBinaryFile, swapBytes);
						}
						else
						{
							prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_DOUBLE, BTT_STORED_4BYTE);
							TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
							TextCompiler::WriteValue<float>(val, mBinaryFile, swapBytes);
						}
					}
					else
					{
						prefix = BTT_MAKENUMBER_PREFIX(BTT_NUMBER, BTT_SUBTYPE_DOUBLE, BTT_STORED_4BYTE);
						TextCompiler::WriteValue<unsigned char>(prefix, mBinaryFile, swapBytes);
						TextCompiler::WriteValue<float>(val, mBinaryFile, swapBytes);
					}
				}
			}
			break;
		case TT_STRING:
			TextCompiler::WriteValue<unsigned char>(BTT_MAKESTRING_PREFIX(BTT_STRING, tok->Length()), mBinaryFile, swapBytes);
			TextCompiler::WriteValue<idStr>(tok, mBinaryFile, swapBytes);
			break;
		case TT_LITERAL:
			TextCompiler::WriteValue<unsigned char>(BTT_MAKESTRING_PREFIX(BTT_LITERAL, tok->Length()), mBinaryFile, swapBytes);
			TextCompiler::WriteValue<idStr>(tok, mBinaryFile, swapBytes);
			break;
		case TT_NAME:
			TextCompiler::WriteValue<unsigned char>(BTT_MAKESTRING_PREFIX(BTT_NAME, tok->Length()), mBinaryFile, swapBytes);
			TextCompiler::WriteValue<idStr>(tok, mBinaryFile, swapBytes);
			break;
		case TT_PUNCTUATION:
			if(*tok == "&&")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LOGICALAND), mBinaryFile, swapBytes);
			}
			else if (*tok == "&")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_AMPERSAND), mBinaryFile, swapBytes);
			}
			else if(*tok == "=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_EQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "==")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_EQUALEQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "!=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_NOTEQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "!")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_EXCLAMATION), mBinaryFile, swapBytes);
			}
			else if(*tok == "<")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LESSTHAN), mBinaryFile, swapBytes);
			}
			else if(*tok == "<=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LESSOREQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "<<")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_SHIFTLEFT), mBinaryFile, swapBytes);
			}
			else if(*tok == ">")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_GREATERTHAN), mBinaryFile, swapBytes);
			}
			else if(*tok == ">=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_GREATEROREQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == ">>")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_SHIFTRIGHT), mBinaryFile, swapBytes);
			}
			else if(*tok == "%")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PERCENT), mBinaryFile, swapBytes);
			}
			else if(*tok == "[")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LEFTBRACKET), mBinaryFile, swapBytes);
			}
			else if(*tok == "]")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_RIGHTBRACKET), mBinaryFile, swapBytes);
			}
			else if(*tok == "-")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_MINUS), mBinaryFile, swapBytes);
			}
			else if(*tok == "--")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_MINUSMINUS), mBinaryFile, swapBytes);
			}
			else if(*tok == "-=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_MINUSEQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "+")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PLUS), mBinaryFile, swapBytes);
			}
			else if(*tok == "+=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PLUSEQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "++")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PLUSPLUS), mBinaryFile, swapBytes);
			}
			else if(*tok == "(")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LEFTPAREN), mBinaryFile, swapBytes);
			}
			else if(*tok == ")")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_RIGHTPAREN), mBinaryFile, swapBytes);
			}
			else if(*tok == "{")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_LEFTBRACE), mBinaryFile, swapBytes);
			}
			else if(*tok == "}")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_RIGHTBRACE), mBinaryFile, swapBytes);
			}
			else if(*tok == ",")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_COMMA), mBinaryFile, swapBytes);
			}
			else if(*tok == "::")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_DOUBLECOLON), mBinaryFile, swapBytes);
			}
			else if(*tok == "#")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_HASH), mBinaryFile, swapBytes);
			}
			else if(*tok == "##")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_DOUBLEHASH), mBinaryFile, swapBytes);
			}
			else if(*tok == "/")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_FORWARDSLASH), mBinaryFile, swapBytes);
			}
			else if(*tok == "\\")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_BACKSLASH), mBinaryFile, swapBytes);
			}
			else if(*tok == ";")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_SEMICOLON), mBinaryFile, swapBytes);
			}
			else if(*tok == ".")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PERIOD), mBinaryFile, swapBytes);
			}
			else if(*tok == "$")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_DOLLARSIGN), mBinaryFile, swapBytes);
			}
			else if(*tok == "~")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_TILDE), mBinaryFile, swapBytes);
			}
			else if(*tok == "|")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_PIPE), mBinaryFile, swapBytes);
			}
			else if(*tok == "||")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_DOUBLEPIPE), mBinaryFile, swapBytes);
			}
			else if(*tok == "*")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_ASTERISK), mBinaryFile, swapBytes);
			}
			else if(*tok == "*=")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_TIMESEQUAL), mBinaryFile, swapBytes);
			}
			else if(*tok == "¡")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_PUNC_INVERTEDPLING, mBinaryFile, swapBytes);
			}
			else if(*tok == "¿")
			{
				TextCompiler::WriteValue<unsigned char>(BTT_PUNC_INVERTEDQUERY, mBinaryFile, swapBytes);
			}
			else
			{
				common->Warning("Unsupported punctuation: '%s'", tok->c_str());
				
				TextCompiler::WriteValue<unsigned char>(BTT_MAKEPUNCTUATION_PREFIX(BTT_PUNC_ASNULLTERMINATED), mBinaryFile, swapBytes);
				TextCompiler::WriteValue<idStr>(tok, mBinaryFile, swapBytes);
			}
			break;
		default:
			// wtf?
			assert(false);
		}
	}
}

// jsinger: this method takes an existing file and causes a binary tokenized
//          version of the file to be generated
void idLexer::WriteBinaryFile(char const * const filename)
{
	if(!cvarSystem->GetCVarBool("com_BinaryRead"))
	{
		unsigned int swap=0;

		switch(cvarSystem->GetCVarInteger("com_BinaryWrite"))
		{
		case 1:
			swap = 0;
			break;
		case 2:
			swap = LEXFL_BYTESWAP;
			break;
		}

		idLexer src(filename, LEXFL_WRITEBINARY | swap);
		idToken token;
		if(src.IsLoaded())
		{
            while(src.ReadToken(&token))
			{
				// we don't need to do anything, because just reading the file
				// causes it to write a tokenized version
			}
		}
	}
}

// jsinger: implementation for a class that can load binary tokenized files
using namespace TextCompiler;
Lexer::Lexer(int flags)
{
#ifndef LEXER_READ_AHEAD
	mFile = NULL;
#endif
	offset = 0;
	tokenAvailable = false;
	mFlags = flags;
	if(flags & LEXFL_READBINARY)
	{
		mDelegate = NULL;
	}
	else
	{
		mDelegate = new idLexer(flags);
	}
}

Lexer::Lexer(char const * const ptr, int length, char const * const name, int flags)
{
	if(flags & LEXFL_READBINARY)
	{
#ifdef LEXER_READ_AHEAD
		mLexerIOWrapper.InitFromMemory(ptr, length);
#else
		assert(false);
#endif
		mDelegate = NULL;
	}
	else
	{
		mDelegate = new idLexer(ptr, length, name, flags);
	}
}

Lexer::Lexer(char const * const filename, int flags, bool OSPath)
{
#ifndef LEXER_READ_AHEAD
	mFile = NULL;
#endif
	offset = 0;
	tokenAvailable = false;
	mFlags = flags;
	if(flags & LEXFL_READBINARY)
	{
		mDelegate = NULL;
        LoadFile(filename, OSPath);
	}
	else
	{
		mDelegate = new idLexer(filename, flags, OSPath);
	}
}

Lexer::~Lexer()
{
	FreeSource();
}

char const * Lexer::GetFileName()
{
	if(mDelegate)
	{
		return mDelegate->GetFileName();
	}
	else
	{
		assert(false);
		return "";
	}
}

bool Lexer::HadError() const
{
	if(mDelegate)
	{
		return mDelegate->HadError();
	}
	else
	{
		assert(false);
		return false;
	}
}

void Lexer::Warning(char const *str, ...)
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	idLib::common->Warning( "Lexer: '%s'", text );
}

void Lexer::Error(char const *str, ...)
{
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	assert(false);

	idLib::common->Error( "Lexer: offset: %d '%s'", offset, text );
}

int const Lexer::GetLineNum()
{
	if(mDelegate)
	{
		return mDelegate->GetLineNum();
	}
	else
	{
		return 1;
	}
}

unsigned int const Lexer::GetFileTime()
{
	if(mDelegate)
	{
		return mDelegate->GetFileTime();
	}
	else
	{
		return 0;
	}
}

int const Lexer::GetFileOffset()
{
	if(mDelegate)
	{
		return mDelegate->GetFileOffset();
	}
	{
		assert(false);
		return 0;
	}
}

int Lexer::EndOfFile()
{
	if(mDelegate)
	{
		return mDelegate->EndOfFile();
	}
	else
	{
#ifdef LEXER_READ_AHEAD
		return mLexerIOWrapper.Tell()>= mLexerIOWrapper.Length();
#else
		return mFile->Tell()>= mFile->Length();
#endif
	}
}

void Lexer::Reset()
{
	if(mDelegate)
	{
		mDelegate->Reset();
	}
	else
	{
#ifdef LEXER_READ_AHEAD
		mLexerIOWrapper.Seek(0, 	FS_SEEK_SET);
#else
		mFile->Seek(0, 	FS_SEEK_SET);
#endif
		offset = 0;
		tokenAvailable = false;
	}
}

int Lexer::GetFlags()
{
	if(mDelegate)
	{
		return mDelegate->GetFlags();
	}
	else
	{
		return mFlags;
	}
}

void Lexer::SetFlags(int flags)
{
	if(mDelegate)
	{
		mDelegate->SetFlags(flags);
	}
	else
	{
		mFlags = flags;
	}
}

int Lexer::GetPunctuationId(char const *str)
{
	if(mDelegate)
	{
		return mDelegate->GetPunctuationId(str);
	}
	else
	{
		// this method is not supported
		assert(false);
		return 0;
	}
}

void Lexer::SetPunctuations(struct punctuation_s const *punc)
{
	if(mDelegate)
	{
		mDelegate->SetPunctuations(punc);
	}
	else
	{
		// this method is not supported
		assert(false);
	}
}

int Lexer::GetLastWhiteSpaceEnd() const
{
	if(mDelegate)
	{
		return mDelegate->GetLastWhiteSpaceEnd();
	}
	else
	{
		return 0;
	}
}

char const *Lexer::GetPunctuationFromId(int val)
{
	if(mDelegate)
	{
		return mDelegate->GetPunctuationFromId(val);
	}
	else
	{
		// this method is not supported
		assert(false);
		return "";
	}
}

int Lexer::GetLastWhiteSpaceStart() const
{
	if(mDelegate)
	{
		return mDelegate->GetLastWhiteSpaceStart();
	}
	else
	{
		return 0;
	}
}

int Lexer::GetLastWhiteSpace(idStr &str) const
{
	if(mDelegate)
	{
		return mDelegate->GetLastWhiteSpace(str);
	}
	else
	{
		return 0;
	}
}

char const *Lexer::ParseRestOfLine(idStr &str)
{
	if(mDelegate)
	{
		return mDelegate->ParseRestOfLine(str);
	}
	else
	{
		// this method is not supported
		assert(false);
		return "";
	}
}

char const *Lexer::ParseBracedSectionExact(idStr &str, int val)
{
	if(mDelegate)
	{
		return mDelegate->ParseBracedSectionExact(str, val);
	}
	else
	{
		// this method is not supported
		assert(false);
		return "";
	}
}

char const *Lexer::ParseBracedSection(idStr &str)
{
	if(mDelegate)
	{
		return mDelegate->ParseBracedSection(str);
	}
	else
	{
		// this method is not supported
		assert(false);
		return "";
	}
}

int Lexer::Parse3DMatrix(int z, int y, int x, float *m)
{
	if(mDelegate)
	{
		return mDelegate->Parse3DMatrix(z, y, x, m);
	}
	else
	{
		int i;

		if ( !ExpectTokenString( "(" ) ) {
			return false;
		}

		for ( i = 0 ; i < z; i++ ) {
			if ( !Parse2DMatrix( y, x, m + i * x*y ) ) {
				return false;
			}
		}

		if ( !ExpectTokenString( ")" ) ) {
			return false;
		}
		return true;
	}
}

int Lexer::Parse2DMatrix(int y, int x, float *m)
{
	if(mDelegate)
	{
		return mDelegate->Parse2DMatrix(y, x, m);
	}
	else
	{
		int i;

		if ( !ExpectTokenString( "(" ) ) {
			return false;
		}

		for ( i = 0; i < y; i++ ) {
			if ( !Parse1DMatrix( x, m + i * x ) ) {
				return false;
			}
		}

		if ( !ExpectTokenString( ")" ) ) {
			return false;
		}
		return true;
	}
}

int Lexer::Parse1DMatrixOpenEnded(int MaxCount, float *m)
{
	if(mDelegate)
	{
		return mDelegate->Parse1DMatrixOpenEnded(MaxCount, m);
	}
	else
	{
		int i;

		if ( !ExpectTokenString( "(" ) ) {
			return 0;
		}

		for ( i = 0; i < MaxCount; i++ ) {
			idToken tok;

			if (!ReadToken( &tok )) {
				return 0;
			}

			if ( tok == ")" ) {
				return i;
			}

			UnreadToken( &tok );

			m[i] = ParseFloat();
		}

		if ( !ExpectTokenString( ")" ) ) {
			return 0;
		}

		return i;
	}
}

int Lexer::Parse1DMatrix(int x, float *m)
{
	if(mDelegate)
	{
		return mDelegate->Parse1DMatrix(x, m);
	}
	else
	{
		int i;

		if ( !ExpectTokenString( "(" ) ) {
			return false;
		}

		for ( i = 0; i < x; i++ ) {
			m[i] = ParseFloat();
		}

		if ( !ExpectTokenString( ")" ) ) {
			return false;
		}
		return true;
	}
}

float Lexer::ParseFloat(bool *errorFlag)
{
	if(mDelegate)
	{
		return mDelegate->ParseFloat(errorFlag);
	}
	else
	{
		idToken token;

		if(EndOfFile())
		{
			return 0;
		}

		if ( !ReadToken( &token ) ) {
			Warning( "couldn't read expected floating point number" );
			return 0;
		}
		if ( token.type == TT_PUNCTUATION && token == "-" ) {
			ExpectTokenType( TT_NUMBER, 0, &token );
			return -token.GetFloatValue();
		}
		else if ( token.type != TT_NUMBER ) {
			Warning( "expected float value, found '%s'", token.c_str() );
			}
		return token.GetFloatValue();
	}
}

bool Lexer::ParseBool()
{
	if(mDelegate)
	{
		return mDelegate->ParseBool();
	}
	else
	{
		idToken token;

		if ( !ExpectTokenType( TT_NUMBER, 0, &token ) ) {
			Error( "couldn't read expected boolean" );
			return false;
		}
		return ( token.GetIntValue() != 0 );
	}
}

int Lexer::ParseInt()
{
	if(mDelegate)
	{
		return mDelegate->ParseInt();
	}
	else
	{
		idToken token;

		if ( !ReadToken( &token ) ) {
			Error( "couldn't read expected integer" );
			return 0;
		}
		if ( token.type == TT_PUNCTUATION && token == "-" ) {
			ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
			return -((signed int) token.GetIntValue());
		}
		else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
			Error( "expected integer value, found '%s'", token.c_str() );
		}
		return token.GetIntValue();
	}
}

int Lexer::ReadTokenOnLine(idToken *token)
{
	if(mDelegate)
	{
		return mDelegate->ReadTokenOnLine(token);
	}
	else
	{
		// lines are meaningless in the binary format
		return ReadToken(token);
	}
}

void Lexer::UnreadToken(idToken const *token)
{
	if(mDelegate)
	{
		mDelegate->UnreadToken(token);
	}
	else
	{
		if ( tokenAvailable ) 
		{
			idLib::common->FatalError( "Lexer::unreadToken, unread token twice\n" );
		}
		unreadToken = *token;
		tokenAvailable = true;
		offset -= unreadSize+1;		// the +1 is for the prefix
	}
}

int Lexer::SkipBracedSection(bool parseFirstBrace)
{
	if(mDelegate)
	{
		return mDelegate->SkipBracedSection(parseFirstBrace);
	}
	else
	{
		idToken token;
		int depth;

		depth = parseFirstBrace ? 0 : 1;
		do {
			if ( !ReadToken( &token ) ) {
				return false;
			}
			if ( token.type == TT_PUNCTUATION ) {
				if ( token == "{" ) {
					depth++;
				} else if ( token == "}" ) {
					depth--;
				}
			}
		} while( depth );
		return true;
	}
}

int Lexer::SkipRestOfLine()
{
	if(mDelegate)
	{
		return mDelegate->SkipRestOfLine();
	}
	else
	{
		// this method is not supported
		assert(false);
		return 0;
	}
}

int Lexer::SkipUntilString(char const *str)
{
	if(mDelegate)
	{
		return mDelegate->SkipUntilString(str);
	}
	else
	{
		// this method is not supported
		assert(false);
		return 0;
	}
}

int Lexer::CheckTokenType(int type, int subtype, idToken *token)
{
	if(mDelegate)
	{
		return mDelegate->CheckTokenType(type, subtype, token);
	}
	else
	{
		idToken tok;

		if (!ReadToken( &tok )) {
			return 0;
		}
		// if the type matches
		if (tok.type == type && (tok.subtype & subtype) == subtype) {
			*token = tok;
			return 1;
		}
		return 0;
	}
}

int Lexer::CheckTokenString(char const *string)
{
	if(mDelegate)
	{
		return mDelegate->CheckTokenString(string);
	}
	else
	{
		idToken tok;

		if (!ReadToken( &tok )) {
			return 0;
		}

		// if the token is available
		if ( tok == string ) {
			return 1;
		}

		UnreadToken( &tok );
		return 0;
	}
}

int Lexer::ExpectAnyToken(idToken *token)
{
	if(mDelegate)
	{
		return mDelegate->ExpectAnyToken(token);
	}
	else
	{
		if (!ReadToken( token )) {
			Error( "couldn't read expected token" );
			return 0;
		}
		else {
			return 1;
		}
	}
}

int Lexer::ExpectTokenType(int type, int subtype, idToken *token)
{
	if(mDelegate)
	{
		return mDelegate->ExpectTokenType(type, subtype, token);
	}
	else
	{
		idStr str;

		if ( !ReadToken( token ) ) {
			Error( "couldn't read expected token" );
			return 0;
		}

		if ( token->type != type ) {
			switch( type ) {
				case TT_STRING: str = "string"; break;
				case TT_LITERAL: str = "literal"; break;
				case TT_NUMBER: str = "number"; break;
				case TT_NAME: str = "name"; break;
				case TT_PUNCTUATION: str = "punctuation"; break;
				default: str = "unknown type"; break;
			}
			Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
		if ( token->type == TT_NUMBER ) {
			if ( (token->subtype & subtype) != subtype ) {
				str.Clear();
				if ( subtype & TT_DECIMAL ) str = "decimal ";
				if ( subtype & TT_HEX ) str = "hex ";
				if ( subtype & TT_OCTAL ) str = "octal ";
				if ( subtype & TT_BINARY ) str = "binary ";
				if ( subtype & TT_UNSIGNED ) str += "unsigned ";
				if ( subtype & TT_LONG ) str += "long ";
				if ( subtype & TT_FLOAT ) str += "float ";
				if ( subtype & TT_INTEGER ) str += "integer ";
				str.StripTrailing( ' ' );
				Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
				return 0;
			}
		}
		else if ( token->type == TT_PUNCTUATION ) {
			if ( subtype < 0 ) {
				Error( "BUG: wrong punctuation subtype" );
				return 0;
			}
			if ( token->subtype != subtype ) {
				Error( "expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
				return 0;
			}
		}
		return 1;
	}
}

int Lexer::ExpectTokenString(char const *string)
{
	if(mDelegate)
	{
		return mDelegate->ExpectTokenString(string);
	}
	else
	{
		idToken token;

		if(!ReadToken( &token ))
		{
			Error( "couldn't find expected '%s'", string );
			return 0;
		}
		if( token != string )
		{
			Error( "expected '%s' but found '%s'", string, token.c_str() );
			return 0;
		}
		return 1;
	}
}

int Lexer::ReadToken(idToken *token)
{

#ifdef LEXER_READ_AHEAD
#define OBJ &mLexerIOWrapper
#else
#define OBJ mFile
#endif
	
	if(mDelegate)
	{
		return mDelegate->ReadToken(token);
	}
	else
	{
		// if there are any unread tokens, use them first
		if(tokenAvailable)
		{
			*token = unreadToken;
			tokenAvailable = false;
		}
		else
		{
#ifndef LEXER_READ_AHEAD
			assert(mFile);
#endif
			if(EndOfFile())
				return 0;
			unsigned char prefix = ReadValue<unsigned char>(OBJ);
			token->Clear();

			// BTT types are equivalent to the TT types except they are one less, so we add one to get the token type
			if(BTT_GET_TYPE(prefix) == BTT_PUNCTUATION2)
				token->type = TT_PUNCTUATION;
			else
				token->type = BTT_GET_TYPE(prefix)+1;

			// this is used only to determine if the punctuation was encoded in the prefix or came afterwards as a null terminated string
			bool encoded = true;

			switch(token->type)
			{
			case TT_STRING: 
			case TT_NAME:
			case TT_LITERAL:
				if(BTT_GET_STRING_LENGTH(prefix) != 0)
				{
					// short string
					int length = BTT_GET_STRING_LENGTH(prefix);
					if ( length == 0 ) {
						assert( false );
					}
					int i;
					for( i = 0; i < length; i++ )
					{
						char c = ReadValue<char>(OBJ);
						token->Append(c);
					}
					// short strings have no null, so we must add one
					//token->Append('\0');

					token->subtype = token->Length();
					unreadSize = token->Length();
				}
				else
				{
					// null terminated string
					*token = ReadValue<idStr>(OBJ);
					token->subtype = token->Length();
					unreadSize = token->Length()+1;
				}
				break;
			case TT_PUNCTUATION:
				switch(BTT_GET_PUNCTUATION(prefix))
				{
				case BTT_PUNC_ASNULLTERMINATED:
					// null terminated string
					*token = ReadValue<idStr>(OBJ);
					token->subtype = token->Length();
					unreadSize = token->Length()+1;
					encoded = false;
					break;
				case BTT_PUNC_RIGHTPAREN:
					*token = ")";
					break;
				case BTT_PUNC_LEFTBRACE:
					*token = "{";
					break;
				case BTT_PUNC_RIGHTBRACE:
					*token = "}";
					break;
				case BTT_PUNC_MINUS:
					*token = "-";
					break;
				case BTT_PUNC_PLUS:
					*token = "+";
					break;
				case BTT_PUNC_COMMA:
					*token = ",";
					break;
				case BTT_PUNC_PLUSPLUS:
					*token = "++";
					break;
				case BTT_PUNC_LEFTBRACKET:
					*token = "[";
					break;
				case BTT_PUNC_RIGHTBRACKET:
					*token = "]";
					break;
				case BTT_PUNC_EQUAL:
					*token = "=";
					break;
				case BTT_PUNC_EQUALEQUAL:
					*token = "==";
					break;
				case BTT_PUNC_NOTEQUAL:
					*token = "!=";
					break;
				case BTT_PUNC_PERCENT:
					*token = "%";
					break;
				case BTT_PUNC_LESSTHAN:
					*token = "<";
					break;
				case BTT_PUNC_GREATERTHAN:
					*token = ">";
					break;
				case BTT_PUNC_LOGICALAND:
					*token = "&&";
					break;
				case BTT_PUNC_AMPERSAND:
					*token = "&";
					break;
				case BTT_PUNC_MINUSMINUS:
					*token = "--";
					break;
				case BTT_PUNC_HASH:
					*token = "#";
					break;
				case BTT_PUNC_LESSOREQUAL:
					*token = "<=";
					break;
				case BTT_PUNC_GREATEROREQUAL:
					*token = ">=";
					break;
				case BTT_PUNC_FORWARDSLASH:
					*token = "/";
					break;
				case BTT_PUNC_SHIFTLEFT:
					*token = "<<";
					break;
				case BTT_PUNC_SHIFTRIGHT:
					*token = ">>";
					break;
				case BTT_PUNC_LEFTPAREN:
					*token = "(";
					break;
				case BTT_PUNC_SEMICOLON:
					*token = ";";
					break;
				case BTT_PUNC_ASTERISK:
					*token = "*";
					break;
				case BTT_PUNC_PERIOD:
					*token = ".";
					break;
				case BTT_PUNC_DOLLARSIGN:
					*token = "$";
					break;
				case BTT_PUNC_PLUSEQUAL:
					*token = "+=";
					break;
				case BTT_PUNC_MINUSEQUAL:
					*token = "-=";
					break;
				case BTT_PUNC_TILDE:
					*token = "~";
					break;
				case BTT_PUNC_EXCLAMATION:
					*token = "!";
					break;
				case BTT_PUNC_PIPE:
					*token = "|";
					break;
				case BTT_PUNC_BACKSLASH:
					*token = "\\";
					break;
				case BTT_PUNC_DOUBLEHASH:
					*token = "##";
					break;
				case BTT_PUNC_TIMESEQUAL:
					*token = "*=";
					break;
				case BTT_PUNC_DOUBLEPIPE:
					*token = "||";
					break;
				case BTT_PUNC_INVERTEDPLING:
					*token = "¡";
					break;
				case BTT_PUNC_INVERTEDQUERY:
					*token = "¿";
					break;
				default:
					assert(false);		// unrecognized punctuation
				}
				if(encoded)
					unreadSize = 0;		// punctuation encoded in prefix
				break;
			case TT_NUMBER:
				{
					// number of bytes actually in the stream used to represent this value
					unsigned int size = BTT_GET_STORED_SIZE(prefix);
					const int buffersize = 100;
					char buffer[buffersize];	// big enough buffer for the int to string conversion routines

					switch(BTT_GET_SUBTYPE(prefix))
					{
						case BTT_SUBTYPE_INT:
							switch(size)
							{
							case BTT_STORED_1BYTE:
								token->intvalue = ReadValue<char>(OBJ);
								unreadSize = sizeof(char);
								break;
							case BTT_STORED_2BYTE:
								token->intvalue = ReadValue<short>(OBJ);
								unreadSize = sizeof(short);
								break;
							case BTT_STORED_4BYTE:
								token->intvalue = ReadValue<int>(OBJ);
								unreadSize = sizeof(int);
								break;
							default:
								// invalid stored size for an integer
								assert(false);
							}

							token->floatvalue = token->intvalue;
							// I hate the fact that I have to copy into the string, but there's no way
							// of easily knowing whether it is used later or not

							// This conversion to a string assumes that long and int are the same
							assert(sizeof(long) == sizeof(int));

							//ltoa(token->intvalue, buffer, 10);
							idStr::snPrintf( buffer, buffersize, "%ld", token->intvalue );
							assert(token->intvalue == atol(buffer));
							*token = buffer;
							token->subtype = TT_INTEGER | TT_DECIMAL | TT_VALUESVALID;
						break;
						
						case BTT_SUBTYPE_UNSIGNEDINT:
							switch(size)
							{
							case BTT_STORED_1BYTE:
								token->intvalue = ReadValue<unsigned char>(OBJ);
								unreadSize = sizeof(unsigned char);
								break;
							case BTT_STORED_2BYTE:
								token->intvalue = ReadValue<unsigned short>(OBJ);
								unreadSize = sizeof(unsigned short);
								break;
							case BTT_STORED_4BYTE:
								token->intvalue = ReadValue<unsigned int>(OBJ);
								unreadSize = sizeof(unsigned int);
								break;
							default:
								// invalid stored size for an unsigned integer
								assert(false);
							}
							token->floatvalue = token->intvalue;

							// I hate the fact that I have to copy into the string, but there's no way
							// of easily knowing whether it is used later or not

							// This conversion to a string assumes that long and int are the same
							assert(sizeof(long) == sizeof(int));

							//ultoa(token->intvalue, buffer, 10);
							idStr::snPrintf( buffer, buffersize, "%lu", token->intvalue );
							assert(token->intvalue == ((unsigned long)atol(buffer)));
							*token = buffer;
							token->subtype = TT_INTEGER | TT_UNSIGNED | TT_DECIMAL | TT_VALUESVALID;
							break;

						case BTT_SUBTYPE_FLOAT:
							// invalid stored size for a float
							assert(sizeof(float) == 4);

							switch(size)
							{
							case BTT_STORED_4BYTE:
								token->floatvalue = ReadValue<float>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(float);
								break;
							case BTT_STORED_2BYTE:		// requested a float, but it was integral, so it was saved that way
								token->floatvalue = ReadValue<short>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(short);
								break;
							case BTT_STORED_1BYTE:		// requested a float, but it was integral, so it was saved that way
								token->floatvalue = ReadValue<char>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(char);
								break;
							default:
								assert(false);
							}

							// I hate the fact that I have to copy into the string, but there's no way
							// of easily knowing whether it is used later or not
							idStr::snPrintf( buffer, buffersize, "%f", token->floatvalue );
							*token = buffer;
							token->subtype = TT_FLOAT | TT_DECIMAL | TT_SINGLE_PRECISION | TT_VALUESVALID;

							unreadSize = sizeof(float);
							break;

						case BTT_SUBTYPE_DOUBLE:
							// invalid stored size for a double
							assert(sizeof(double) == 8);

							// doubles are stored as floats since the original text file never uses full double precision anyway
							//assert(size == BTT_STORED_4BYTE);

							switch(size)
							{
							case BTT_STORED_4BYTE:
								token->floatvalue = ReadValue<float>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(float);
								break;
							case BTT_STORED_2BYTE:		// requested a float, but it was integral, so it was saved that way
								token->floatvalue = ReadValue<short>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(short);
								break;
							case BTT_STORED_1BYTE:		// requested a float, but it was integral, so it was saved that way
								token->floatvalue = ReadValue<char>(OBJ);
								token->intvalue = token->floatvalue;
								unreadSize = sizeof(char);
								break;
							default:
								assert(false);
							}

							// I hate the fact that I have to copy into the string, but there's no way
							// of easily knowing whether it is used later or not
							idStr::snPrintf( buffer, buffersize, "%f", token->floatvalue );
							*token = buffer;
							token->subtype = TT_FLOAT | TT_DECIMAL | TT_DOUBLE_PRECISION | TT_VALUESVALID;
							break;
					}
				}
				break;

			default:
				// unsupported binary type
				assert(false);
			}
		}

		//common->Warning("Read Token: '%s    (int: %d)(float: %f)'\n", token->c_str(), token->GetIntValue(), token->GetFloatValue());
		//common->Printf("Read Token: %s\n", token->c_str());
		offset += unreadSize+1;		// the +1 is for the prefix
		return 1;
	}
}

int Lexer::IsLoaded()
{
	if(mDelegate)
	{
		return mDelegate->IsLoaded();
	}
	else
	{
#ifdef LEXER_READ_AHEAD
		return mLexerIOWrapper.IsLoaded();
#else
		return mFile != NULL;
#endif
	}
}

void Lexer::FreeSource()
{
#ifdef LEXER_READ_AHEAD
	mLexerIOWrapper.Close();
#else
	if(mFile)
	{
		idLib::fileSystem->CloseFile(mFile);
		mFile = NULL;
	}
#endif

	if(mDelegate)
	{
		delete mDelegate;
		mDelegate = NULL;
	}
}

int Lexer::LoadMemory(char const *ptr, int length, char const *name, int startLine)
{
	if(mDelegate)
	{
		return mDelegate->LoadMemory(ptr, length, name, startLine);
	}
	else
	{
		// this is essentially a no op
		return true;
	}
}

int Lexer::LoadFile(char const *filename, bool OSPath)
{
	if(mDelegate)
	{
		return mDelegate->LoadFile(filename, OSPath);
	}
	else
	{
		idStr fullName;
		FreeSource();

		fullName = filename;
		fullName.Append(sCompiledFileSuffix);
		bool binaryFound = OpenFile(fullName, OSPath);
		if(binaryFound)
		{
			return binaryFound;
		}
		else
		{
			mDelegate = new idLexer(filename, mFlags, OSPath);
			int isLoaded = mDelegate->IsLoaded();
			if(isLoaded)
			{
				// don't do this right now until I clean up the Lexer class
				if(mFlags & LEXFL_READBINARY)
				{
					Warning("%s not found, loading ascii version", fullName.c_str());
				}
			}
			else
			{
				// didn't find the ascii version either, make sure and clean up in case this lexer is reused
				delete mDelegate;
				mDelegate = NULL;
			}

			return isLoaded;
		}
	}
}

void Lexer::WriteBinaryToken(idToken *tok)
{
	if(mDelegate)
	{
		mDelegate->WriteBinaryToken(tok);
	}
	else
	{
		// can't write out an already binary file
		assert(false);
	}
}

bool Lexer::OpenFile(char const *filename, bool OSPath)
{
	idStr pathname;
	
	if ( !OSPath && ( baseFolder[0] != '\0' ) ) {
		pathname = va( "%s/%s", baseFolder, filename );
	} else {
		pathname = filename;
	}
	
#ifdef LEXER_READ_AHEAD
	if ( !mLexerIOWrapper.OpenFile( pathname, OSPath ) ) {
		return false;
	}
#else
	if ( OSPath ) {
		mFile = idLib::fileSystem->OpenExplicitFileRead( pathname );
	} else {
		mFile = idLib::fileSystem->OpenFileRead( pathname );
	}
	if ( !mFile ) {
		return false;
	}
#endif

	return true;
}

void Lexer::SetBaseFolder( const char *path ) {
	idStr::Copynz( baseFolder, path, sizeof( baseFolder ) );
}


// RAVEN BEGIN
// dluetscher: added method to parse a structure array that is made up of numerics (floats, ints), and stores them in the given storage
void Lexer::ParseNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage )
{
	if(mDelegate)
	{
		mDelegate->ParseNumericStructArray( numStructElements, tokenSubTypeStructElements, arrayCount, arrayStorage );
	}
	else
	{
		int arrayOffset, curElement;

		for ( arrayOffset = 0; arrayOffset < arrayCount; arrayOffset++ )
		{
			for ( curElement = 0; curElement < numStructElements; curElement++ )
			{
				if ( tokenSubTypeStructElements[curElement] & TT_FLOAT )
				{
					*(float*)arrayStorage = Lexer::ParseFloat();
					arrayStorage += sizeof(float);
				}
				else
				{
					*(int*)arrayStorage = Lexer::ParseInt();
					arrayStorage += sizeof(int);
				}
			}
		}
	}
}
// RAVEN END


char idLexer::baseFolder[256];
// RAVEN END
