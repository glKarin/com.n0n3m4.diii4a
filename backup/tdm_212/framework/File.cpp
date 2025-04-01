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



#include <minizip/unzip.h>
#include "minizip/minizip_extra.h"	//unzSeek

/*
=================
FS_WriteFloatString
=================
*/
int FS_WriteFloatString( char *buf, const char *fmt, va_list argPtr ) {
    int i;
    unsigned int u;
	double f;
	char *str;
	int index;
	idStr tmp, format;

	index = 0;

	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
						*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
					format += *fmt++;
				}
				format += *fmt;
				switch( *fmt ) {
					case 'f':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						f = va_arg( argPtr, double );
						if ( format.Length() <= 2 ) {
							// high precision floating point number without trailing zeros
							sprintf( tmp, "%1.10f", f );
							tmp.StripTrailing( '0' );
							tmp.StripTrailing( '.' );
							index += sprintf( buf+index, "%s", tmp.c_str() );
						}
						else {
							index += sprintf( buf+index, format.c_str(), f );
						}
						break;
					case 'd':
					case 'i':
						i = va_arg( argPtr, int );
						index += sprintf( buf+index, format.c_str(), i );
						break;
					case 'u':
						u = va_arg( argPtr, unsigned int );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'o':
						u = va_arg( argPtr, unsigned int );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'x':
						u = va_arg( argPtr, unsigned int );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'X':
						u = va_arg( argPtr, unsigned int );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'c':
						i = va_arg( argPtr, int );
						index += sprintf( buf+index, format.c_str(), (char) i );
						break;
					case 's':
						str = va_arg( argPtr, char * );
						index += sprintf( buf+index, format.c_str(), str );
						break;
					case '%':
						index += sprintf( buf+index, format.c_str() );
						break;
					default:
						common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
						break;
				}
				fmt++;
				break;
			case '\\':
				fmt++;
				switch( *fmt ) {
					case 't':
						index += sprintf( buf+index, "\t" );
						break;
					case 'v':
						index += sprintf( buf+index, "\v" );
						break;
					case 'n':
						index += sprintf( buf+index, "\n" );
						break;
					case '\\':
						index += sprintf( buf+index, "\\" );
						break;
					default:
						common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
						break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}

	return index;
}

/*
=================================================================================

idFile

=================================================================================
*/

/*
=================
idFile::GetName
=================
*/
const char *idFile::GetName( void ) {
	return "";
}

/*
=================
idFile::GetFullPath
=================
*/
const char *idFile::GetFullPath( void ) {
	return "";
}

/*
=================
idFile::IsCompressed
=================
*/
bool idFile::IsCompressed( void ) {
	return false;
}

/*
=================
idFile::Read
=================
*/
int idFile::Read( void *buffer, int len ) {
	common->FatalError( "idFile::Read: cannot read from idFile" );
	return 0;
}

/*
=================
idFile::Write
=================
*/
int idFile::Write( const void *buffer, int len ) {
	common->FatalError( "idFile::Write: cannot write to idFile" );
	return 0;
}

/*
=================
idFile::Length
=================
*/
int idFile::Length( void ) {
	return 0;
}

/*
=================
idFile::Timestamp
=================
*/
ID_TIME_T idFile::Timestamp( void ) {
	return 0;
}

/*
=================
idFile::GetDomain
=================
*/
domainStatus_t idFile::GetDomain() const {
	return FDOM_UNKNOWN;
}

/*
=================
idFile::Tell
=================
*/
int idFile::Tell( void ) {
	return 0;
}

/*
=================
idFile::ForceFlush
=================
*/
void idFile::ForceFlush( void ) {
}

/*
=================
idFile::Flush
=================
*/
void idFile::Flush( void ) {
}

/*
=================
idFile::Seek
=================
*/
int idFile::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}

/*
=================
idFile::Rewind
=================
*/
void idFile::Rewind( void ) {
	Seek( 0, FS_SEEK_SET );
}

/*
=================
idFile::Printf
=================
*/
int idFile::Printf(const char *fmt, ...) {
    char buf[MAX_PRINT_MSG_SIZE];
    va_list argptr;

    va_start(argptr, fmt);

#ifdef WIN32
    // Write to the buffer and replace the newlines in Windows
    idStr::vsnPrintf(buf, MAX_PRINT_MSG_SIZE - 1, fmt, argptr);
    va_end(argptr);

    // so notepad formats the lines correctly
    idStr	work( buf );
    work.Replace( "\n", "\r\n" );

    return Write(work.c_str(), work.Length());
#else
    // No need to replace newlines in platforms other than Windows
    const int length = idStr::vsnPrintf(buf, MAX_PRINT_MSG_SIZE - 1, fmt, argptr);
    va_end(argptr);

    return Write(buf, length);
#endif
}

/*
=================
idFile::VPrintf
=================
*/
int idFile::VPrintf( const char *fmt, va_list args ) {
	char buf[MAX_PRINT_MSG_SIZE];
	const int length = idStr::vsnPrintf( buf, MAX_PRINT_MSG_SIZE-1, fmt, args );

	return Write( buf, length );
}

/*
=================
idFile::WriteFloatString
=================
*/
int idFile::WriteFloatString( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG_SIZE];
	va_list argPtr;

	va_start( argPtr, fmt );
	const int len = FS_WriteFloatString( buf, fmt, argPtr );
	va_end( argPtr );

	return Write( buf, len );
}

/*
 =================
 idFile::ReadInt
 =================
 */
int idFile::ReadInt( int &value ) {
	const int result = Read( &value, sizeof( value ) );
	value = LittleInt(value);

	return result;
}

/*
 =================
 idFile::ReadUnsignedInt
 =================
 */
int idFile::ReadUnsignedInt( unsigned int &value ) {
	const int result = Read( &value, sizeof( value ) );
	value = LittleInt(value);

	return result;
}

/*
 =================
 idFile::ReadShort
 =================
 */
int idFile::ReadShort( short &value ) {
	const int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);

	return result;
}

/*
 =================
 idFile::ReadUnsignedShort
 =================
 */
int idFile::ReadUnsignedShort( unsigned short &value ) {
	const int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);

	return result;
}

/*
 =================
 idFile::ReadChar
 =================
 */
int idFile::ReadChar( char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 idFile::ReadUnsignedChar
 =================
 */
int idFile::ReadUnsignedChar( unsigned char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 idFile::ReadFloat
 =================
 */
int idFile::ReadFloat( float &value ) {
	const int result = Read( &value, sizeof( value ) );
	value = LittleFloat(value);

	return result;
}

/*
 =================
 idFile::ReadBool
 =================
 */
int idFile::ReadBool( bool &value ) {
	unsigned char c;

	const int result = ReadUnsignedChar( c );
	value = c ? true : false;

	return result;
}

/*
 =================
 idFile::ReadString
 =================
 */
int idFile::ReadString( idStr &string ) {
	int len;
	int result = 0;
	
	ReadInt( len );
	if ( len >= 0 ) {
		string.Fill( ' ', len );
		result = Read( &string[ 0 ], len );
	}

	return result;
}

/*
 =================
 idFile::ReadVec2
 =================
 */
int idFile::ReadVec2( idVec2 &vec ) {
	const int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );

	return result;
}

/*
 =================
 idFile::ReadVec3
 =================
 */
int idFile::ReadVec3( idVec3 &vec ) {
	const int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );

	return result;
}

/*
 =================
 idFile::ReadVec4
 =================
 */
int idFile::ReadVec4( idVec4 &vec ) {
	const int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );

	return result;
}

/*
 =================
 idFile::ReadVec6
 =================
 */
int idFile::ReadVec6( idVec6 &vec ) {
	const int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );

	return result;
}

/*
 =================
 idFile::ReadMat3
 =================
 */
int idFile::ReadMat3( idMat3 &mat ) {
	const int result = Read( &mat, sizeof( mat ) );
	LittleRevBytes( &mat, sizeof(float), sizeof(mat)/sizeof(float) );

	return result;
}

/*
 =================
 idFile::WriteInt
 =================
 */
int idFile::WriteInt( const int value ) {
	const int v = LittleInt(value);

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteUnsignedInt
 =================
 */
int idFile::WriteUnsignedInt( const unsigned int value ) {
	const unsigned int v = LittleInt(value);

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteShort
 =================
 */
int idFile::WriteShort( const short value ) {
	const short v = LittleShort(value);

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteUnsignedShort
 =================
 */
int idFile::WriteUnsignedShort( const unsigned short value ) {
	const unsigned short v = LittleShort(value);

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteChar
 =================
 */
int idFile::WriteChar( const char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 idFile::WriteUnsignedChar
 =================
 */
int idFile::WriteUnsignedChar( const unsigned char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 idFile::WriteFloat
 =================
 */
int idFile::WriteFloat( const float value ) {
	const float v = LittleFloat(value);

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteBool
 =================
 */
int idFile::WriteBool( const bool value ) {
	const unsigned char c = value;

	return WriteUnsignedChar( c );
}

/*
 =================
 idFile::WriteString
 =================
 */
int idFile::WriteString( const char *value ) {
	const int len = static_cast<int>(strlen( value ));
	WriteInt( len );

    return Write( value, len );
}

/*
 =================
 idFile::WriteVec2
 =================
 */
int idFile::WriteVec2( const idVec2 &vec ) {
	idVec2 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec3
 =================
 */
int idFile::WriteVec3( const idVec3 &vec ) {
	idVec3 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec4
 =================
 */
int idFile::WriteVec4( const idVec4 &vec ) {
	idVec4 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteVec6
 =================
 */
int idFile::WriteVec6( const idVec6 &vec ) {
	idVec6 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );

	return Write( &v, sizeof( v ) );
}

/*
 =================
 idFile::WriteMat3
 =================
 */
int idFile::WriteMat3( const idMat3 &mat ) {
	idMat3 v = mat;
	LittleRevBytes(&v, sizeof(float), sizeof(v)/sizeof(float) );

	return Write( &v, sizeof( v ) );
}

/*
=================================================================================

idFile_Memory

=================================================================================
*/


/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( void ) : idFile_Memory( "*unknown*" ) {}

/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( const char *name ) {
	this->name = name;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;
	owned = true;
	timestamp = 0;

	mode = ( 1 << FS_WRITE );
	filePtr = NULL;
	curPtr = NULL;
}

/*
=================
idFile_Memory::idFile_Memory
=================
*/
idFile_Memory::idFile_Memory( const char *name, const char *data, int length, bool owned ) {
	this->name = name;
	fileSize = length;
	allocated = 0;
	granularity = 16384;
	this->owned = owned;
	timestamp = 0;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>(data);
	curPtr = const_cast<char *>(data);
}

/*
=================
idFile_Memory::~idFile_Memory
=================
*/
idFile_Memory::~idFile_Memory( void ) {
	if ( filePtr && owned ) {
		Mem_Free( filePtr );
	}
}

/*
=================
idFile_Memory::Read
=================
*/
int idFile_Memory::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_Memory::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( curPtr + len > filePtr + fileSize ) {
		len = filePtr + fileSize - curPtr;
	}

	memcpy( buffer, curPtr, len );
	curPtr += len;

	return len;
}

/*
=================
idFile_Memory::Write
=================
*/
int idFile_Memory::Write( const void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	const int alloc = curPtr + len + 1 - filePtr - allocated; // need room for len+1
	if ( alloc > 0 ) {
		const int extra = granularity * ( 1 + alloc / granularity );
		char *newPtr = (char *) Mem_Alloc( allocated + extra );
		if ( allocated ) {
			memcpy( newPtr, filePtr, allocated );
		}

		allocated += extra;
		curPtr = newPtr + ( curPtr - filePtr );

		if ( filePtr ) {
			Mem_Free( filePtr );
		}

		filePtr = newPtr;
	}

	memcpy( curPtr, buffer, len );
	curPtr += len;
	fileSize += len;
	filePtr[ fileSize ] = 0; // len + 1

	return len;
}

/*
=================
idFile_Memory::Length
=================
*/
int idFile_Memory::Length( void ) {
	return fileSize;
}

/*
=================
idFile_Memory::Timestamp
=================
*/
ID_TIME_T idFile_Memory::Timestamp( void ) {
	// usually zero, unless set explicitly via SetTimestamp method
	return timestamp;
}

/*
=================
idFile_Memory::Tell
=================
*/
int idFile_Memory::Tell( void ) {
	return ( curPtr - filePtr );
}

/*
=================
idFile_Memory::ForceFlush
=================
*/
void idFile_Memory::ForceFlush( void ) {
}

/*
=================
idFile_Memory::Flush
=================
*/
void idFile_Memory::Flush( void ) {
}

/*
=================
idFile_Memory::Seek

  returns zero on success and -1 on failure
=================
*/
int idFile_Memory::Seek( long offset, fsOrigin_t origin ) {

	switch( origin ) {
		case FS_SEEK_CUR: {
			curPtr += offset;
			break;
		}
		case FS_SEEK_END: {
			curPtr = filePtr + fileSize - offset;
			break;
		}
		case FS_SEEK_SET: {
			curPtr = filePtr + offset;
			break;
		}
		default: {
			common->FatalError( "idFile_Memory::Seek: bad origin for %s\n", name.c_str() );
			return -1;
		}
	}
	if ( curPtr < filePtr ) {
		curPtr = filePtr;
		return -1;
	}
	if ( curPtr > filePtr + fileSize ) {
		curPtr = filePtr + fileSize;
		return -1;
	}
	return 0;
}


/*
=================================================================================

idFile_BitMsg

=================================================================================
*/

/*
=================
idFile_BitMsg::idFile_BitMsg
=================
*/
idFile_BitMsg::idFile_BitMsg( idBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_WRITE );
	this->msg = &msg;
}

/*
=================
idFile_BitMsg::idFile_BitMsg
=================
*/
idFile_BitMsg::idFile_BitMsg( const idBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_READ );
	this->msg = const_cast<idBitMsg *>(&msg);
}

/*
=================
idFile_BitMsg::~idFile_BitMsg
=================
*/
idFile_BitMsg::~idFile_BitMsg( void ) {
}

/*
=================
idFile_BitMsg::Read
=================
*/
int idFile_BitMsg::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_BitMsg::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	return msg->ReadData( buffer, len );
}

/*
=================
idFile_BitMsg::Write
=================
*/
int idFile_BitMsg::Write( const void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	msg->WriteData( buffer, len );
	return len;
}

/*
=================
idFile_BitMsg::Length
=================
*/
int idFile_BitMsg::Length( void ) {
	return msg->GetSize();
}

/*
=================
idFile_BitMsg::Timestamp
=================
*/
ID_TIME_T idFile_BitMsg::Timestamp( void ) {
	return 0;
}

/*
=================
idFile_BitMsg::Tell
=================
*/
int idFile_BitMsg::Tell( void ) {
	if ( mode && FS_READ ) { // Serp: changed from bitwise & to && - since flag is 0
		return msg->GetReadCount();
	} else {
		return msg->GetSize();
	}
}

/*
=================
idFile_BitMsg::ForceFlush
=================
*/
void idFile_BitMsg::ForceFlush( void ) {
}

/*
=================
idFile_BitMsg::Flush
=================
*/
void idFile_BitMsg::Flush( void ) {
}

/*
=================
idFile_BitMsg::Seek

  returns zero on success and -1 on failure
=================
*/
int idFile_BitMsg::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}


/*
=================================================================================

idFile_Permanent

=================================================================================
*/

/*
=================
idFile_Permanent::idFile_Permanent
=================
*/
idFile_Permanent::idFile_Permanent( void ) {
	name = "invalid";
	o = NULL;
	mode = 0;
	fileSize = 0;
	handleSync = false;
	domain = FDOM_UNKNOWN;
}

/*
=================
idFile_Permanent::~idFile_Permanent
=================
*/
idFile_Permanent::~idFile_Permanent( void ) {
	if ( o ) {
		fclose( o );
	}
}

/*
=================
idFile_Permanent::Read

Properly handles partial reads
=================
*/
int idFile_Permanent::Read( void *buffer, int len ) {
	int		block;
	int		read;
	byte *	buf;

	if ( !(mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "idFile_Permanent::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	int remaining = len;
	int tries = 0;
	while( remaining ) {
		block = remaining;
		read = static_cast<int>(fread( buf, 1, block, o ));
		if ( read == 0 ) {
			// we might have been trying to read from a CD, which
			// sometimes returns a 0 read on windows
			if ( !tries ) {
				tries = 1;
			}
			else {
				fileSystem->AddToReadCount( len - remaining );
				return (len - remaining);
			}
		}

		if ( read == -1 ) {
			common->FatalError( "idFile_Permanent::Read: -1 bytes read from %s", name.c_str() );
		}

		remaining -= read;
		buf += read;
	}
	fileSystem->AddToReadCount( len );
	return len;
}

/*
=================
idFile_Permanent::Write

Properly handles partial writes
=================
*/
int idFile_Permanent::Write( const void *buffer, int len ) {
	int		block;
	int		written;
	byte *	buf;

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "idFile_Permanent::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	int remaining = len;
	int tries = 0;
	while( remaining ) {
		block = remaining;
		written = static_cast<int>(fwrite( buf, 1, block, o ));
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			}
			else {
				common->Printf( "idFile_Permanent::Write: 0 bytes written to %s\n", name.c_str() );
				return 0;
			}
		}

		if ( written == -1 ) {
			common->Printf( "idFile_Permanent::Write: -1 bytes written to %s\n", name.c_str() );
			return 0;
		}

		remaining -= written;
		buf += written;
		fileSize += written;
	}
	if ( handleSync ) {
		fflush( o );
	}
	return len;
}

/*
=================
idFile_Permanent::ForceFlush
=================
*/
void idFile_Permanent::ForceFlush( void ) {
	setvbuf( o, NULL, _IONBF, 0 );
}

/*
=================
idFile_Permanent::Flush
=================
*/
void idFile_Permanent::Flush( void ) {
	fflush( o );
}

/*
=================
idFile_Permanent::Tell
=================
*/
int idFile_Permanent::Tell( void ) {
	return ftell( o );
}

/*
================
idFile_Permanent::Length
================
*/
int idFile_Permanent::Length( void ) {
	return fileSize;
}

/*
================
idFile_Permanent::Timestamp
================
*/
ID_TIME_T idFile_Permanent::Timestamp( void ) {
	return Sys_FileTimeStamp( o );
}

/*
================
idFile_Permanent::GetDomain
================
*/
domainStatus_t idFile_Permanent::GetDomain() const {
	return domain;
}

/*
=================
idFile_Permanent::Seek

  returns zero on success and -1 on failure
=================
*/
int idFile_Permanent::Seek( long offset, fsOrigin_t origin ) {
	int _origin;

	switch( origin ) {
		case FS_SEEK_CUR: {
			_origin = SEEK_CUR;
			break;
		}
		case FS_SEEK_END: {
			_origin = SEEK_END;
			break;
		}
		case FS_SEEK_SET: {
			_origin = SEEK_SET;
			break;
		}
		default: {
			_origin = SEEK_CUR;
			common->FatalError( "idFile_Permanent::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}

	return fseek( o, offset, _origin );
}


/*
=================================================================================

idFile_InZip

=================================================================================
*/

/*
=================
idFile_InZip::idFile_InZip
=================
*/
idFile_InZip::idFile_InZip( void ) {
	name = "invalid";
	zipFilePos = 0;
	compressed = false;
	fileSize = 0;
	memset( &z, 0, sizeof( z ) );
	domain = FDOM_UNKNOWN;
}

/*
=================
idFile_InZip::~idFile_InZip
=================
*/
idFile_InZip::~idFile_InZip( void ) {
	unzCloseCurrentFile( z );
	unzClose( z );
}

/*
=================
idFile_InZip::Read

Properly handles partial reads
=================
*/
int idFile_InZip::Read( void *buffer, int len ) {
	const int l = unzReadCurrentFile( z, buffer, len );
	fileSystem->AddToReadCount( l );

	return l;
}

/*
=================
idFile_InZip::Write
=================
*/
int idFile_InZip::Write( const void *buffer, int len ) {
	common->FatalError( "idFile_InZip::Write: cannot write to the zipped file %s", name.c_str() );
	return 0;
}

/*
=================
idFile_InZip::ForceFlush
=================
*/
void idFile_InZip::ForceFlush( void ) {
	common->FatalError( "idFile_InZip::ForceFlush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
idFile_InZip::Flush
=================
*/
void idFile_InZip::Flush( void ) {
	common->FatalError( "idFile_InZip::Flush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
idFile_InZip::Tell
=================
*/
int idFile_InZip::Tell( void ) {
	return unztell( z );
}

/*
================
idFile_InZip::Length
================
*/
int idFile_InZip::Length( void ) {
	return fileSize;
}

/*
================
idFile_InZip::Timestamp
================
*/
ID_TIME_T idFile_InZip::Timestamp( void ) {
	return 0;
}

/*
================
idFile_InZip::GetDomain
================
*/
domainStatus_t idFile_InZip::GetDomain() const {
	return domain;
}

/*
================
idFile_InZip::IsCompressed
================
*/
bool idFile_InZip::IsCompressed( void) {
	return compressed;
}

/*
=================
idFile_InZip::Seek

  returns zero on success and -1 on failure
=================
*/
#define ZIP_SEEK_BUF_SIZE	(1<<15)

int idFile_InZip::Seek( long offset, fsOrigin_t origin ) {
	//stgatilov: try to seek using minizip function
	//this would work very fast if the file is uncompressed
	//see issue #4504 (and #4507) for reasoning behind this

	//Note: mode constants are different from standard ones!
	int stdioOrigin = -1;
	if (origin == FS_SEEK_SET) stdioOrigin = SEEK_SET;
	if (origin == FS_SEEK_CUR) stdioOrigin = SEEK_CUR;
	if (origin == FS_SEEK_END) stdioOrigin = SEEK_END;

	if (stdioOrigin == SEEK_CUR && offset == 0)
		return 0; //noop

	if (stdioOrigin == SEEK_END) {
		//Note: meaning of offset is non-standard here!
		offset = -offset;
	}
	//try to seek quickly in uncompressed zip:
	int simpleSeekOk = unzseek64(z, offset, stdioOrigin);
	if (simpleSeekOk == UNZ_OK)
		return 0;

	//zip is likely to be compressed, seek slowly then
	//note that we have to read and decompress all intermediate data

	int res, i;
	char *buf;

	switch( origin ) {
		case FS_SEEK_END: {
			offset = fileSize - offset;
		}
		case FS_SEEK_SET: {
			// set the file position in the zip file (also sets the current file info)
			unzSetOffset64(z, zipFilePos);
			unzOpenCurrentFile( z );
			if ( offset <= 0 ) {
				return 0;
			}
		}
		case FS_SEEK_CUR: {
			buf = (char *) _alloca16( ZIP_SEEK_BUF_SIZE );
			for ( i = 0; i < ( offset - ZIP_SEEK_BUF_SIZE ); i += ZIP_SEEK_BUF_SIZE ) {
				res = unzReadCurrentFile( z, buf, ZIP_SEEK_BUF_SIZE );
				if ( res < ZIP_SEEK_BUF_SIZE ) {
					return -1;
				}
			}
			res = i + unzReadCurrentFile( z, buf, offset - i );
			return ( res == offset ) ? 0 : -1;
		}
		default: {
			common->FatalError( "idFile_InZip::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}
