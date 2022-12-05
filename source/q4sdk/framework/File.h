// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __FILE_H__
#define __FILE_H__

/*
==============================================================

  File Streams.

==============================================================
*/

// mode parm for Seek
typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class idFileSystemLocal;

// RAVEN BEGIN
// jscott: made idFile a pure virtual interface
class idFile {
public:
	virtual					~idFile( void ) {}
							// Get the name of the file.
	virtual const char *	GetName( void ) = 0;
							// Get the full file path.
	virtual const char *	GetFullPath( void ) = 0;
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len ) = 0;
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len ) = 0;
							// Returns the length of the file.
	virtual int				Length( void ) = 0;
							// Return a time value for reload operations.
	virtual unsigned int	Timestamp( void ) = 0;
							// Returns offset in file.
	virtual int				Tell( void ) = 0;
							// Forces flush on files being writting to.
	virtual void			ForceFlush( void ) = 0;
							// Causes any buffered data to be written to the file.
	virtual void			Flush( void ) = 0;
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin ) = 0;
							// Go back to the beginning of the file.
	virtual void			Rewind( void ) = 0;
							// Like fprintf.
	virtual int				Printf( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg ) = 0;
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( const char *fmt, ... ) id_attribute((format(printf,2,3))) = 0;
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value ) = 0;
	virtual int				ReadUnsignedInt( unsigned int &value ) = 0;
	virtual int				ReadShort( short &value ) = 0;
	virtual int				ReadUnsignedShort( unsigned short &value ) = 0;
	virtual int				ReadChar( char &value ) = 0;
	virtual int				ReadUnsignedChar( unsigned char &value ) = 0;
	virtual int				ReadFloat( float &value ) = 0;
	virtual int				ReadBool( bool &value ) = 0;
	virtual int				ReadString( idStr &string ) = 0;
	virtual int				ReadVec2( idVec2 &vec ) = 0;
	virtual int				ReadVec3( idVec3 &vec ) = 0;
	virtual int				ReadVec4( idVec4 &vec ) = 0;
	virtual int				ReadVec5( idVec5 &vec ) = 0;
	virtual int				ReadVec6( idVec6 &vec ) = 0;
	virtual int				ReadMat3( idMat3 &mat ) = 0;
	virtual int				ReadBounds( idBounds &bounds ) = 0;
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value ) = 0;
	virtual int				WriteUnsignedInt( const unsigned int value ) = 0;
	virtual int				WriteShort( const short value ) = 0;
	virtual int				WriteUnsignedShort( unsigned short value ) = 0;
	virtual int				WriteChar( const char value ) = 0;
	virtual int				WriteUnsignedChar( const unsigned char value ) = 0;
	virtual int				WriteFloat( const float value ) = 0;
	virtual int				WriteBool( const bool value ) = 0;
	virtual int				WriteString( const char *string ) = 0;
	virtual int				WriteVec2( const idVec2 &vec ) = 0;
	virtual int				WriteVec3( const idVec3 &vec ) = 0;
	virtual int				WriteVec4( const idVec4 &vec ) = 0;
	virtual int				WriteVec5( const idVec5 &vec ) = 0;
	virtual int				WriteVec6( const idVec6 &vec ) = 0;
	virtual int				WriteMat3( const idMat3 &mat ) = 0;
	virtual int				WriteBounds( const idBounds &bounds ) = 0;

	// dluetscher: added method to write a structure array that is made up of numerics (floats, ints) from the given storage
	virtual void			WriteNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage, const char *prepend ) = 0;

	// jscott: for savegame and demo file syncing
	virtual void			WriteSyncId( void ) = 0;
	virtual void			ReadSyncId( const char *detail = "unspecified", const char *classname = NULL ) = 0;
};

class idFile_Common : public idFile
{
public:
	virtual					~idFile_Common( void ) {}
							// Get the name of the file.
	virtual const char *	GetName( void ) { return( "" ); }
							// Get the full file path.
	virtual const char *	GetFullPath( void ) { return( "" ); }
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length( void ) { return( 0 ); }
							// Return a time value for reload operations.
	virtual unsigned int	Timestamp( void ) { return( 0 ); }
							// Returns offset in file.
	virtual int				Tell( void ) { return( 0 ); }
							// Forces flush on files being writting to.
	virtual void			ForceFlush( void ) {}
							// Causes any buffered data to be written to the file.
	virtual void			Flush( void ) {}
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin ) { return( -1 ); }
							// Go back to the beginning of the file.
	virtual void			Rewind( void );
							// Like fprintf.
	virtual int				Printf( const char *fmt, ... ) id_attribute((format(printf,2,3)));
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg );
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( const char *fmt, ... ) id_attribute((format(printf,2,3)));
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( idStr &string );
	virtual int				ReadVec2( idVec2 &vec );
	virtual int				ReadVec3( idVec3 &vec );
	virtual int				ReadVec4( idVec4 &vec );
	virtual int				ReadVec5( idVec5 &vec );
	virtual int				ReadVec6( idVec6 &vec );
	virtual int				ReadMat3( idMat3 &mat );
	virtual int				ReadBounds( idBounds &bounds );
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const idVec2 &vec );
	virtual int				WriteVec3( const idVec3 &vec );
	virtual int				WriteVec4( const idVec4 &vec );
	virtual int				WriteVec5( const idVec5 &vec );
	virtual int				WriteVec6( const idVec6 &vec );
	virtual int				WriteMat3( const idMat3 &mat );
	virtual int				WriteBounds( const idBounds &bounds );

	// dluetscher: added method to write a structure array that is made up of numerics (floats, ints) from the given storage
	virtual void			WriteNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage, const char *prepend );

	// jscott: for savegame and demo file syncing
	virtual void			WriteSyncId( void );
	virtual void			ReadSyncId( const char *detail = "unspecific", const char *classname = NULL );
};

class idFile_Memory : public idFile_Common {
// RAVEN END
	friend class			idFileSystemLocal;

public:
							idFile_Memory( void );	// file for writing without name
							idFile_Memory( const char *name );	// file for writing
							idFile_Memory( const char *name, char *data, int length );	// file for writing
							idFile_Memory( const char *name, const char *data, int length );	// file for reading
	virtual					~idFile_Memory( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );
	virtual void			Rewind( void );

							// changes memory file to read only
	virtual void			MakeReadOnly( void );
							// clear the file
	virtual void			Clear( bool freeMemory = true );
							// set data for reading
	void					SetData( const char *data, int length );
							// returns const pointer to the memory buffer
	const char *			GetDataPtr( void ) const { return filePtr; }
							// set the file granularity
	void					SetGranularity( int g ) { assert( g > 0 ); granularity = g; }

private:
	idStr					name;			// name of the file
	int						mode;			// open mode
	int						maxSize;		// maximum size of file
	int						fileSize;		// size of the file
	int						allocated;		// allocated size
	int						granularity;	// file granularity
	char *					filePtr;		// buffer holding the file data
	char *					curPtr;			// current read/write pointer
};


// RAVEN BEGIN
class idFile_BitMsg : public idFile_Common {
// RAVEN END
	friend class			idFileSystemLocal;

public:
							idFile_BitMsg( idBitMsg &msg );
							idFile_BitMsg( const idBitMsg &msg );
	virtual					~idFile_BitMsg( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	idStr					name;			// name of the file
	int						mode;			// open mode
	idBitMsg *				msg;
};


// RAVEN BEGIN
class idFile_Permanent : public idFile_Common {
// RAVEN END
	friend class			idFileSystemLocal;

public:
							idFile_Permanent( void );
	virtual					~idFile_Permanent( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

	// returns file pointer
	FILE *					GetFilePtr( void ) { return o; }

private:
	idStr					name;			// relative path of the file - relative path
	idStr					fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	FILE *					o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
};

class idFile_ASCII : public idFile_Permanent
{
	int						inside;

public:
							idFile_ASCII( void );

	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( idStr &string );
	virtual int				ReadVec2( idVec2 &vec );
	virtual int				ReadVec3( idVec3 &vec );
	virtual int				ReadVec4( idVec4 &vec );
	virtual int				ReadVec5( idVec5 &vec );
	virtual int				ReadVec6( idVec6 &vec );
	virtual int				ReadMat3( idMat3 &mat );
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const idVec2 &vec );
	virtual int				WriteVec3( const idVec3 &vec );
	virtual int				WriteVec4( const idVec4 &vec );
	virtual int				WriteVec5( const idVec5 &vec );
	virtual int				WriteVec6( const idVec6 &vec );
	virtual int				WriteMat3( const idMat3 &mat );

	// dluetscher: added method to write a structure array that is made up of numerics (floats, ints) from the given storage
	virtual void			WriteNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage, const char *prepend );

	// jscott: for savegame and demo file syncing
	virtual void			WriteSyncId( void );
	virtual void			ReadSyncId( const char *detail = "unspecific", const char *classname = NULL );
};

// RAVEN BEGIN
class idFile_InZip : public idFile_Common {
// RAVEN END
	friend class			idFileSystemLocal;

public:
							idFile_InZip( void );
	virtual					~idFile_InZip( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void );
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	idStr					name;			// name of the file in the pak
	idStr					fullPath;		// full file path including pak file name
	int						zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
};

#endif /* !__FILE_H__ */
