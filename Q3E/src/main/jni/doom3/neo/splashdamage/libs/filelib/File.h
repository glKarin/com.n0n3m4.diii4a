// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __FILE_H__
#define __FILE_H__

/*
==============================================================

  File Streams.

==============================================================
*/

// modes for OpenFileByMode. used as bit mask internally
typedef enum {
	FS_READ		= 0,
	FS_WRITE	= 1,
	FS_APPEND	= 2
} fsMode_t;

// mode parm for Seek - we use enums that map to the right values from system headers
typedef enum {
	FS_SEEK_SET = SEEK_SET,
	FS_SEEK_CUR	= SEEK_CUR,
	FS_SEEK_END	= SEEK_END,
} fsOrigin_t;

typedef enum {
	FS_ISFILE,
	FS_ISDIR
} fsStat_t;

class idFileSystemLocal;


class idFile {
public:
	virtual					~idFile( void ) {};
							// Get the name of the file.
	virtual const char *	GetName( void );
							// Get the full file path.
	virtual const char *	GetFullPath( void );
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length( void ) const;
							// Return a time value for reload operations.
	virtual unsigned int	Timestamp( void );
							// Returns offset in file.
	virtual int				Tell( void );
							// Forces flush on files being writting to.
	virtual void			ForceFlush( void );
							// Causes any buffered data to be written to the file.
	virtual void			Flush( void );
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
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
	virtual int				ReadCQuat( idCQuat& quat );
	virtual int				ReadAngles( idAngles& angles );
	virtual int				ReadVec4( idVec4 &vec );
	virtual int				ReadVec6( idVec6 &vec );
	virtual int				ReadMat3( idMat3 &mat );
	virtual int				Read1DFloatArray( float* dst );
	virtual int				ReadFloatArray( float* src, const int num );
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteDouble( const double value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const idVec2 &vec );
	virtual int				WriteVec3( const idVec3 &vec );
	virtual int				WriteCQuat( const idCQuat& quat );
	virtual int				WriteAngles( const idAngles& angles );
	virtual int				WriteVec4( const idVec4 &vec );
	virtual int				WriteVec6( const idVec6 &vec );
	virtual int				WriteMat3( const idMat3 &mat );
	virtual int				Write1DFloatArray( const int num, const float* src );
	virtual int				WriteFloatArray( const float* src, const int num );
};

class idFile_Memory : public idFile {
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
	virtual int				Length( void ) const;
	virtual unsigned int	Timestamp( void );
	virtual int				Tell( void );
	virtual void			ForceFlush( void );
	virtual void			Flush( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

							// changes memory file to read only
	virtual void			MakeReadOnly( void );

							// changes memory file to write
	virtual void			MakeWritable( void );
							// clear the file
	virtual void			Clear( bool freeMemory = true );
							// set data for reading
	virtual void			SetData( const char *data, int length );
							// returns const pointer to the memory buffer
	virtual const char *	GetDataPtr( void ) const { return filePtr; }
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


class idFile_BitMsg : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_BitMsg( idBitMsg &msg );
							idFile_BitMsg( const idBitMsg &msg );
	virtual					~idFile_BitMsg( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void ) const;
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


class idFile_Permanent : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_Permanent( void );
	virtual					~idFile_Permanent( void );

	virtual const char *	GetName( void ) { return name.c_str(); }
	virtual const char *	GetFullPath( void ) { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length( void ) const;
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

class idFile_Buffered : public idFile {
	friend class			idFileSystemLocal;

public:
							idFile_Buffered( const int granularity = 2048 * 1024 );	// file for reading
							idFile_Buffered( idFile* source, const int granularity = 2048 * 1024 );	// file for reading
	virtual					~idFile_Buffered( void );

	virtual const char *	GetName( void ) { return source->GetName(); }
	virtual const char *	GetFullPath( void ) { return source->GetFullPath(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Length( void ) const { return source->Length(); }
	virtual unsigned int	Timestamp( void ) { return source->Timestamp(); }
	virtual int				Tell( void );
	virtual int				Seek( long offset, fsOrigin_t origin );

	void					SetSource( idFile* source );
	void					ReleaseSource();

private:
	int						ReadInternal( void* buffer, int len );
	void					SeekInternal( long offset );

private:
	idFile *				source;			// source file pointer
	int						granularity;	// file granularity
	int						available;		// current amount of data left in buffer
	long					sourceOffset;	// offset in source file of file data
	const byte *			filePtr;		// buffer holding the file data
	const byte *			curPtr;			// current read pointer
};

#endif /* !__FILE_H__ */
